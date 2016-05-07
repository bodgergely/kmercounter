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
public:
	void insert(const vector<pair<string, size_t>>& input)
	{
		std::copy(input.begin(),input.end(), back_inserter(_partialResults));
	}

	vector<pair<string, size_t>> getResults()
	{
		accumulate();

		// sort by count
		std::sort(_results.begin(), _results.end(), [](const pair<string, size_t>& lhs, const pair<string, size_t>& rhs)
															{
																if(lhs.second >= rhs.second)
																	return true;
																else
																	return false;
															});

		return _results;

	}

private:
	// TODO test if correct!
	void accumulate()
	{
		// sort by string first so that we can accumulate
		std::sort(_partialResults.begin(), _partialResults.end(), [](const pair<string, size_t>& lhs, const pair<string, size_t>& rhs)
															{
																if(lhs.first <= rhs.first)
																	return true;
																else
																	return false;
															});



		string currStr(_partialResults[0].first);
		size_t count = 0;
		for(int i=0;i<_partialResults.size();i++)
		{
			const auto& elem = _partialResults[i];
			if(elem.first!=currStr)
			{
				_results.push_back(make_pair(currStr, count));
				currStr = elem.first;
				count = 0;
			}

			count+=elem.second;
		}

		_results.push_back(make_pair(currStr, count));

	}

private:
	vector<pair<string, size_t>> _partialResults;
	vector<pair<string, size_t>> _results;
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
																			 _finishedCounting(false)
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
			_result = _resultCollector.getResults();
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
		vector<pair<string, size_t>> res;
		//kc->getTopStrings(res);
		_resultCollector.insert(res);
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
