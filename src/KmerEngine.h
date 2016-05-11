#ifndef KMERENGINE_H_
#define KMERENGINE_H_

#include <KmerCounter.h>
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
	using Pair = pair<mer_encoded, size_t>;
	using Result = vector<Pair>;
	using HashMap = unordered_map<mer_encoded, size_t, mer_encoded_hash>;

public:
	// n is the top most count strings
	KmerResultCollector(size_t n, size_t k) : _n(n), _k(k),_hc(0,0), _totalKmerCount(0)
	{
	}

	KmerResultCollector(size_t n, size_t k,  HashTableConfig hc) : _n(n), _k(k), _hc(hc), _totalKmerCount(0)
	{
		_database.reserve(hc.initialSize);
		_database.reserve(hc.maxLoadFactor);
	}

	void setHashTableConfig(HashTableConfig hc)
	{
		_hc = hc;
		_database.reserve(hc.initialSize);
		_database.reserve(hc.maxLoadFactor);
	}


	HashMap& GlobalDataBase() {return _database;}

	vector<pair<string, size_t>> getResult()
	{
		// combine the results

		Result tmp;
		for(HashMap::const_iterator it=_database.begin();it!=_database.end();it++)
		{
			tmp.push_back(*it);
			_totalKmerCount+=it->second;
		}
		// sort or use brute force extraction? using brute force now
		Result res;
		extract(res, tmp, _n);

		//stringify results
		vector<pair<string, size_t>> stringRes;
		for(const auto& r : res)
		{
			const mer_encoded& mem = r.first;
			size_t count = r.second;
			string s = decode(mem, _k);
			stringRes.push_back(make_pair(s, count));
		}


		_database.clear();

		cout << "Number of strings: " << tmp.size() << endl;

		return stringRes;
	}

	unsigned long long totalKmerCount() const {return _totalKmerCount;}

private:


private:
	size_t _n;
	size_t _k;
	unsigned long long _totalKmerCount;
	HashTableConfig _hc;
	HashMap _database;
};


class KmerEngine
{
	using KmerCounterThreadedPtr = shared_ptr<KmerCounterThreaded>;
	using HashTableConfigPtr = unique_ptr<HashTableConfig>;
public:
	KmerEngine(const std::string& filePath, int k, int n, int threadCount) : _k(k),
																			 _n(n),
																			 _numOfCountersCreated(0),
																			 _maxThreadedCounters(threadCount),
																			 _fileReader(filePath),
																			 _finishedCounting(false),
																			 _resultCollector(n, k)
	{
		size_t blksize = _fileReader.blocksize();
		size_t  filesize = _fileReader.filesize();

		size_t recommendedbuckets = calculateInitialHashTableSize(filesize, _k);
		_resultCollector.setHashTableConfig(HashTableConfig(recommendedbuckets, 5));

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

			// if last section then we have to deal with it now
			if(_prevBuffer.getBuffer()!=nullptr && buffer.isEndofStream())
			{
				// need to do this so we will process just one buffer inside createCounter
				_prevBuffer.setBuffer(nullptr);
				createCounter(buffer);
			}
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
			cout << "Number of counters created: " << _numOfCountersCreated << endl;
			auto totalkmers = _resultCollector.totalKmerCount();
			cout << "Total kmers: " << totalkmers << "Expected: " <<  _fileReader.filesize()-_k+1 << endl;
			assert(totalkmers == _fileReader.filesize()-_k+1);
		}
		return _result;
	}

	unsigned long long totalKmerCount() const {return _resultCollector.totalKmerCount();}

private:
	size_t calculateInitialHashTableSize(size_t filesize, size_t kmerLength)
	{
		// the longer the kmer length the more likely we need lots of buckets - not sure hwo to determine this size efficiently yet
		// we could have some statistics gathered from the genomes to determine the expected bucket count using some heuristics
		// the possible permutations of a kmer is 5^k - which is exponentially blows up but okay for small k-s
		if(kmerLength <= 10)
			return pow(5,kmerLength);
		else
			// using cap of kmer lenght of 11
			return pow(5, 11);

	}

	void createCounter(const InputBuffer& buffer)
	{
		unique_lock<mutex> lock(_mutexOnCounters);
		while(_counters.size()>=_maxThreadedCounters)
			_condvarOnCounterSize.wait(lock);

		++_numOfCountersCreated;
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
	size_t _numOfCountersCreated;
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
