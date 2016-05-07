#ifndef KMERENGINE_H_
#define KMERENGINE_H_

#include <KmerCounter.h>
#include <FileIO.h>
#include <memory>

using io::FileReader;
using io::InputBuffer;
using std::unique_ptr;
using kmers::Chunk;

namespace kmers
{


class KmerResultCollector
{
	using Pair = pair<string, size_t>;
	using Result = vector<Pair>;
	using ResultCollection = vector<Result>;
	using HashMap = unordered_map<string, size_t>;

public:
	// n is the top most count strings
	KmerResultCollector(int n) : _n(n) {}

	unordered_map<string, size_t>& GlobalDataBase() {return _database;}

	vector<pair<string, size_t>> getResult()
	{
		// combine the results

		Result tmp;
		for(HashMap::const_iterator it=_database.begin();it!=_database.end();it++)
		{
			tmp.push_back(*it);
		}
		// sort or use brute force extraction? using brute force now
		Result res;
		extract(res, tmp, _n);
		return res;
	}

private:


private:
	size_t _n;
	HashMap _database;
	ResultCollection _results;
};


class KmerEngine
{
	using KmerCounterThreadedPtr = shared_ptr<KmerCounterThreaded>;
	using HashTableConfigPtr = unique_ptr<HashTableConfig>;
public:
	KmerEngine(const std::string& filePath, int k, int n, int threadCount) : _k(k),
																			 _n(n),
																			 _maxThreadedCounters(threadCount),
																			 _fileReader(filePath),
																			 _finishedCounting(false),
																			 _resultCollector(n)
	{
		size_t blksize = _fileReader.blocksize();
		size_t  filesize = _fileReader.filesize();
		_numOfBlocks = filesize / blksize+1;
		if(filesize%blksize == 0)
			_numOfBlocks--;
		_hashTableConfig = HashTableConfigPtr(new HashTableConfig(blksize/10, 12));
	}

	void start()
	{
		// async operation - we started reading the file into blocks which are placed into a queue
		_fileReader.startReadingBlocks();

		_threadReconciliation = thread(&KmerEngine::startReconciliation, this);

		InputBuffer buffer;
		while(!buffer.isEndofStream())
		{
			_fileReader.getNextBlock(buffer);

		// might block below
			if(_prevBuffer.getBuffer() == nullptr && !buffer.isEndofStream())
			{
				//noop wait for the second buffer
			}
			else
				addToCounterList(buffer);

			_prevBuffer = buffer;
		}
		_finishedCounting.store(true);

		_threadReconciliation.join();
	}

	const vector<pair<string, size_t>>& getResults()
	{
		if(_result.empty())
		{
			_result = _resultCollector.getResult();
		}

		return _result;
	}

private:
	void addToCounterList(const InputBuffer& buffer)
	{
		unique_lock<mutex> lock(_mutexOnCounters);
		while(_counters.size()>=_maxThreadedCounters)
			_condvarOnCounterSize.wait(lock);

		const char* begin = buffer.getBuffer();
		const char* end = begin + buffer.getLen();
		Chunk prevChunk(_prevBuffer.getBuffer(), _prevBuffer.getBuffer() + _prevBuffer.getLen());
		Chunk newChunk(begin, end);
		if(prevChunk.begin() == nullptr && buffer.isEndofStream())
			_counters.push_back(KmerCounterThreadedPtr(new KmerCounterThreaded(newChunk, _k, _n, *_hashTableConfig, true)));
		else if(prevChunk.begin() != nullptr)		// there is a previous one
			_counters.push_back(KmerCounterThreadedPtr(new KmerCounterThreaded(prevChunk, newChunk, _k, _n, *_hashTableConfig, true)));

		_condvarOnCounterSize.notify_one();
	}

	void startReconciliation()
	{
		bool keepGoing = true;
		while(true)
		{
			keepGoing = reconcileCounters();
			if(!keepGoing)
				break;
		}
	}

	bool reconcileCounters()
	{
		unique_lock<mutex> lock(_mutexOnCounters);
		while(_counters.empty())
		{
			if(_finishedCounting.load() == false){
				_condvarOnCounterSize.wait(lock);
			}
			else
				return false;
		}
		// wait for it to finish
		_counters.front()->threadHandle().join();
		KmerCounterThreadedPtr& kc = _counters.front();
		populateTopStrings(kc);

		_counters.pop_front();

		_condvarOnCounterSize.notify_one();
		return true;

	}

	void populateTopStrings(const KmerCounterThreadedPtr& kc)
	{
		kc->extractProcessingResult(_resultCollector.GlobalDataBase());
	}



private:
	size_t _k;
	size_t _n;
	size_t _numOfBlocks;
	size_t				_maxThreadedCounters;
	atomic<bool>		_finishedCounting;
	HashTableConfigPtr _hashTableConfig;
	FileReader _fileReader;
	condition_variable	 _condvarOnCounterSize;
	mutex				 _mutexOnCounters;
	list<KmerCounterThreadedPtr> _counters;
	thread						_threadReconciliation;
	KmerResultCollector			 _resultCollector;
	vector<pair<string, size_t>> _result;
	InputBuffer					 _prevBuffer;
};



}

#endif
