#ifndef KMERENGINE_H_
#define KMERENGINE_H_

#include <KmerCounter.h>
#include <StopWatch.h>
#include <FileIO.h>
#include <memory>
#include <cmath>

using io::FileReader;
using io::InputBuffer;
using std::unique_ptr;
using kmers::Chunk;
using kmers::Memory;

namespace kmers
{


class KmerResultCollector
{
	using Pair = pair<Memory, size_t>;
	using Result = vector<Pair>;
	using HashMap = unordered_map<Memory, size_t, Memory::Hash>;

public:
	// n is the top most count strings
	KmerResultCollector(size_t n, size_t k) : _n(n), _k(k), _tree(n, k)
	{
	}


	void insert(const char* begin, const char* end, unsigned int count)
	{
		_tree.insert(begin, end, count);
	}

	void insert(const string& kmer, unsigned int count)
	{
		insert(kmer.c_str(), kmer.c_str()+kmer.size(), count);
	}

	vector<pair<string, unsigned long>> getResult()
	{
		// combine the results
		cout << "Number of nodes allocated in the tree: " << _tree.numberOfNodes() << endl;

		vector<pair<string, unsigned long>> stringRes = _tree.getResults();

		return stringRes;
	}

private:


private:
	size_t _n;
	size_t _k;
	QuintTree _tree;
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
																			 _resultCollector(n, k)
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
				createCounter(buffer);

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

	void createCounter(const InputBuffer& buffer)
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
		processResults(kc);

		_counters.pop_front();

		_condvarOnCounterSize.notify_one();
		return true;

	}

	void processResults(const KmerCounterThreadedPtr& kc)
	{
		const KmerCounter::HashMap& stringMap = kc->getResults();
		unsigned long long totalCount = 0;
		int hashmapCount=0;
		// might be very expensive the string construction below plus memory problems on high k size (5^k) very high k length and
		// random pattern makes the substring count easily (filesize-kmerLen) - and at hsi point we just have a local result
		_sw.start();
		for(KmerCounter::HashMap::const_iterator it=stringMap.begin(); it!=stringMap.end(); it++)
		{
			hashmapCount++;
			totalCount+=it->second;
			const auto& pair = *it;
			const Memory& mem = pair.first;
			size_t count = pair.second;
			_resultCollector.insert(mem.begin(), mem.end(), count);
		}

		cout << "Took: " << _sw.stop() << endl;
		assert(totalCount == kc->getTotalLength()-_k+1);
		cout << "hashmap count: " << hashmapCount << "\n";

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
	StopWatch<chrono::milliseconds> _sw;
};



}

#endif
