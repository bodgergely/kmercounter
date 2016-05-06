/*
 * kmers.h
 *
 *  Created on: May 2, 2016
 *      Author: geri
 */

#ifndef KMERS_H_
#define KMERS_H_

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
	Memory(const char* begin, const char* end) : _begin(begin), _end(end), _len(_end-_begin)
	{
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

	class Hash
	{
	public:
		size_t operator()(const Memory& mem) const
		{
			return mem.hash();
		}
	};

private:
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
};


class Chunk
{
public:
	Chunk() : _begin(nullptr), _end(nullptr) {}
	Chunk(const char* begin, const char* end) : _begin(begin), _end(end) {}
	const char* begin() const {return _begin;}
	const char* end() const {return _end;}
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
 * this one can compare only contiguous memory
 */
class KmerCounter
{
	using HashMap = std::unordered_map<Memory, size_t, Memory::Hash>;
public:
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

	}

	virtual void process()
	{
		count();
		extractTopStrings();
	}

	void getTopStrings(vector<pair<string,size_t> >& out) const
	{
		// TODO refactor - extract method below
		int count=0;
		size_t prevSize = 0;
		for(int i=0;i < _sortedMems.size();i++)
		{
			const auto& tmp = _sortedMems[i];
			if(prevSize!=tmp.second)
				count++;
			prevSize = tmp.second;
			if(count>_n)
				break;

			string s = string(tmp.first.begin(), tmp.first.end() - tmp.first.begin());
			out.push_back(std::make_pair(s, tmp.second));

		}
	}

protected:
	void init()
	{
		_stringMap.reserve(_hashConfig.initialSize);
		_stringMap.max_load_factor(_hashConfig.maxLoadFactor);
		_sortedMems.reserve(_chunk1.end()-_chunk1.begin());
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
			Memory mem(curr, curr+_k);
			++_stringMap[mem];
		}
	}

	void extractTopStrings()
	{
		unsigned long long totalCount = 0;
		for(HashMap::iterator it=_stringMap.begin(); it!=_stringMap.end(); )
		{
			totalCount+=it->second;
			_sortedMems.push_back(*it);
			it = _stringMap.erase(it);
		}

		assert(totalCount == _totalLen-_k+1);

		std::sort(_sortedMems.begin(), _sortedMems.end(), [](const pair<Memory, size_t>& lhs, const pair<Memory, size_t>& rhs)
															{
																if(lhs.second >= rhs.second)
																	return true;
																else
																	return false;
															});
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
	vector<pair<Memory, size_t>>    _sortedMems;
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