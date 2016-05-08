/*
 * kmers.h
 *
 *  Created on: May 2, 2016
 *      Author: geri
 */

#ifndef KMERCOUNTER_H_
#define KMERCOUNTER_H_

#include <StopWatch.h>
#include <QuintTree.h>

#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <utility>
#include <algorithm>
#include <thread>
#include <iterator>
#include <cassert>
#include <thread>
#include <atomic>

#define _DEBUG

#ifdef _DEBUG
#include <iostream>
using namespace std;
using namespace std::chrono;
#endif

namespace kmers
{

using std::list;
using std::pair;
using std::string;
using std::vector;
using std::set;
using std::map;
using std::thread;
using std::atomic;


class Buffer
{
public:
	Buffer(size_t len) : _buf(len, 0){}
	inline const char& getChar(size_t pos) const { return _buf[pos]; }
	inline void putChar(char val, size_t pos) {_buf[pos] = val;}	// no bounds checking for performance
private:
	vector<char> _buf;
};


struct HashTableConfig
{
	HashTableConfig(size_t initSize_, size_t maxLoadFactor_) :  initialSize(initSize_), maxLoadFactor(maxLoadFactor_) {}
	size_t initialSize;
	size_t maxLoadFactor;
};


class Memory
{
public:
	Memory(const char* begin, const char* end, bool copy) : _begin(begin), _end(end), _len(_end-_begin), _memoryOwner(false)
	{
		if(copy)
		{
			allocateAndCopy(begin, end);
			_memoryOwner = true;
		}
		calchash();
	}
	inline bool operator==(const Memory& other) const
	{
		if(!memcmp(_begin, other._begin, _len))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	inline bool operator!=(const Memory& other)
	{
		return !(*this==other);
	}


	const char* begin() const {return _begin;}
	const char* end() const {return _end;}

	string toString() const { return string(_begin, _end-_begin);}

	size_t hash() const {return _hash;}

	bool owner() {return _memoryOwner;}

	void deallocate()
	{
		delete[] _begin;
	}

	class Hash
	{
	public:
		size_t operator()(const Memory& mem) const
		{
			return mem.hash();
		}
	};

private:
	void allocateAndCopy(const char* begin, const char* end)
	{
		size_t size = end - begin;
		_begin = new char[sizeof(char)*size];
		_end = _begin + size;
		memcpy(const_cast<char*>(_begin), const_cast<char*>(begin), size);
	}
	void calchash()
	{
		size_t hash = 5381;
		char c;
		const char* str = _begin;
		while (str!=_end)
		{
			c = *str;
			hash = ((hash << 5) + hash) + c;
			++str;
		}

		_hash = hash;
	}

	const char* _begin;
	const char* _end;
	size_t _len;
	size_t _hash;
	bool	_memoryOwner;
};


class Chunk
{
public:
	Chunk() : _begin(nullptr), _end(nullptr) {}
	Chunk(const char* begin, const char* end) : _begin(begin), _end(end) {}
	const char* begin() const {return _begin;}
	const char* end() const {return _end;}

	void deallocate()
	{
		delete[] _begin;
		_begin = nullptr;
		_end = nullptr;
	}

private:
	const char* _begin;
	const char* _end;	// not included (C++ iterator style range)
};


/*
	 * k: kmer length
	 * creates contiguous memory from the crossings between the two chunks which depends on the kmer length (k)
	 * return a pointer to the allocated memory - the client needs to free it!
	 * assumes that k is smaller than the chunk length!
	 */
Chunk createCrossMemorySection(const Chunk& chunk1, const Chunk& chunk2, int k)
{
	char* mem = new char[ k-1 + k-1];		// the crossing section has k-1 elements from both chunks
	const char* start = chunk1.end() - k + 1;
	memcpy(mem, start, k-1);		// copy the remainder from the first chunk
	memcpy(mem+k-1, chunk2.begin(), k-1); // copy the first k-1 from the second chunk

	return Chunk(mem, mem+2*(k-1));

}

/*
 * brute force extraction of the top n size elements
 */
template<class T>
void extract(vector<pair<T, size_t>>& res, const vector<pair<T, size_t>>& from, int n)
{

	int count = 0;
	size_t limitSize = std::numeric_limits<size_t>::max();
	while(count < n)
	{
		size_t biggest = 0;
		for(const auto& p : from)
		{
			if(p.second > biggest && p.second < limitSize)
				biggest = p.second;
		}
		count++;
		limitSize = biggest;
		// second pass -> populate the res array with the biggest sizes
		for(const auto& p : from)
		{
			if(p.second == biggest)
			{
				res.push_back(p);
			}
		}
	}

}


/*
 * this one can compare only contiguous memory
 */
class KmerCounter
{
public:
	using HashMap = std::unordered_map<Memory, size_t, Memory::Hash>;
	KmerCounter(Chunk chunk, size_t k, size_t n, const HashTableConfig& config) :
																			_chunk1(chunk),
																			_hasTwoChunks(false),
																			_totalLen(chunk.end()-chunk.begin()),
																			_k(k),
																			_n(n),
																			_hashConfig(config)
	{
		init();
	}

	KmerCounter(Chunk chunk1, Chunk chunk2, size_t k, size_t n, const HashTableConfig& config) : _chunk1(chunk1),
																								 _chunk2(chunk2),
																								 _hasTwoChunks(true),
																								 _totalLen(chunk1.end()-chunk1.begin() + k-1),
																								 _k(k),
																								 _n(n),
																								 _hashConfig(config)
	{
		init();
	}

	virtual ~KmerCounter()
	{
		_chunk1.deallocate();
		_crossing.deallocate();
	}

	virtual void process()
	{
		count();
	}

	const HashMap& getResults()
	{
		return _stringMap;
	}

	size_t getTotalLength() const {return _totalLen;}

protected:
	void init()
	{
		_stringMap.reserve(_hashConfig.initialSize);
		_stringMap.max_load_factor(_hashConfig.maxLoadFactor);
	}

	void count()
	{
		if(_hasTwoChunks)
		{
			countInChunk(_chunk1);
			// deal with the crossing into the second chunk -- this is tricky (and annoying) - since k is assumed to be much smaller than the chunk size: need to create contiguous memory from the two (malloc? then free later)
			// we own! chunk! will have to free later (when is later? once we created the strings from under these raw char pointers -> then we can get rid of the memory)
			_crossing = createCrossMemorySection(_chunk1, _chunk2, _k);
			countInChunk(_crossing);
		}
		else
		{
			countInChunk(_chunk1);
		}
	}


	void countInChunk(const Chunk& chunk)
	{
		for(const char* curr = chunk.begin(); curr+_k<=chunk.end(); curr++)
		{
			Memory mem(curr, curr+_k, false);
			++_stringMap[mem];
		}
	}


protected:
	Chunk	_chunk1;
	Chunk	_chunk2;
	Chunk   _crossing;
	bool	_hasTwoChunks;
	size_t	_totalLen;
	size_t _k;
	size_t _n;
	HashMap _stringMap;
	HashTableConfig _hashConfig;
	StopWatch<chrono::milliseconds> _sw;
};



class KmerCounterThreaded : public KmerCounter
{
public:
	KmerCounterThreaded(Chunk chunk1, size_t k, size_t n, const HashTableConfig& config, bool startOnConstruction) :
																					KmerCounter(chunk1, k, n, config),
																					_startOnConstruction(startOnConstruction),
																					_finished(false)

	{
		if(_startOnConstruction)
			kickoff();
	}

	KmerCounterThreaded(Chunk chunk1, Chunk chunk2, size_t k, size_t n, const HashTableConfig& config, bool startOnConstruction) :
																						KmerCounter(chunk1, chunk2, k, n, config),
																						_startOnConstruction(startOnConstruction),
																						_finished(false)

	{
		if(_startOnConstruction)
			kickoff();
	}

	~KmerCounterThreaded()
	{
	}

	thread&		threadHandle() {return _processingThread;}

	void start()
	{
		if(!_startOnConstruction)
		{
			kickoff();
		}
		else
			throw std::runtime_error("Already started in constructor!");
	}

protected:
	void kickoff()
	{
		_processingThread = thread(&KmerCounterThreaded::process, this);
	}

	virtual void process()
	{
		KmerCounter::process();
		_finished.store(true);
	}

protected:
	thread _processingThread;
	bool _startOnConstruction;
	atomic<bool> _finished;
};




}


#endif /* KMERS_H_ */
