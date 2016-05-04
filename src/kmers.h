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



#ifdef _DEBUG
#include <iostream>
using namespace std;
#endif

namespace sevenbridges
{

using std::list;
using std::pair;
using std::string;
using std::vector;
using std::set;
using std::map;
using std::multimap;

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
			return true;
		else
			return false;
	}
	inline bool operator!=(const Memory& other)
	{
		return !(*this==other);
	}


	const char* begin() const {return _begin;}
	const char* end() const {return _end;}

	size_t getHash() const {return _hash;}

	class Hash
	{
	public:
		size_t operator()(const Memory& mem) const
		{
			return mem.getHash();
		}
	};

private:
	void calchash()
	{
		size_t hash = 5381;
		char c;
		const char* str = _begin;
		while (c = *str && str!=_end)
		{
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
			++str;
		}

		_hash = hash;
	}

	const char* _begin;
	const char* _end;
	size_t _len;
	size_t _hash;
};


class KmerCounter
{
	using HashMap = std::unordered_map<Memory, size_t, Memory::Hash>;
public:
	KmerCounter(const char* begin, const char* end, size_t k, size_t n, const HashTableConfig& config) :
																			_begin(begin),
																			_end(end),
																			_totalLen(_end-_begin),
																			_k(k),
																			_n(n),
																			_hashConfig(config)
	{
		_stringMap.reserve(_hashConfig.initialSize);
		_stringMap.max_load_factor(_hashConfig.maxLoadFactor);
		_sortedMems.reserve(end-begin);
	}

	void process()
	{
		count();
		extractTopStrings();
	}

	void getTopStrings(vector<pair<string,size_t> >& out) const
	{
		// TODO refactor - extract method below
		int count=0;
		size_t prevSize = 0;
		for(int i=0;count<_n && i < _sortedMems.size();i++)
		{
			const auto& tmp = _sortedMems[i];
			string s = string(tmp.first.begin(), tmp.first.end() - tmp.first.begin());
			out.push_back(std::make_pair(s, tmp.second));
			if(prevSize!=tmp.second)
				count++;
			prevSize = tmp.second;
		}
	}

private:
	void count()
	{
		for(const char* curr = _begin; curr!=_end; curr++)
		{
			Memory mem(curr, curr+_k);
			++_stringMap[mem];
		}
	}

	void extractTopStrings()
	{
		for(HashMap::iterator it=_stringMap.begin(); it!=_stringMap.end(); )
		{
			_sortedMems.push_back(*it);
			it = _stringMap.erase(it);
		}

		std::sort(_sortedMems.begin(), _sortedMems.end(), [](const pair<Memory, size_t>& lhs, const pair<Memory, size_t>& rhs)
															{
																if(lhs.second >= rhs.second)
																	return true;
																else
																	return false;
															});
	}

private:
	const char* _begin;
	const char* _end;		// not included (C++ iterator style range)
	size_t	_totalLen;
	size_t _k;
	size_t _n;
	HashMap _stringMap;
	HashTableConfig _hashConfig;
	vector<pair<Memory, size_t>>    _sortedMems;
};



class KmerProcessor
{
public:
	KmerProcessor(const string& input, int k, int n, int numOfThreads) : _processingDone(false), _k(k), _n(n), _numOfThreads(numOfThreads)
	{
		HashTableConfig hc(1000, 15);
		for(int i=0;i<numOfThreads;i++)
		{
			_counters.push_back(KmerCounter(input.c_str(), input.c_str()+input.size(), k, n, hc));
		}
	}

	// TODO maybe we want this to be async?
	void process(vector<pair<string, size_t>>& results)
	{
		for(int i=0;i<_counters.size();i++)
			_threads.push_back(std::thread(&KmerCounter::process, std::ref(_counters[i])));

		for(int i=0;i<_threads.size();i++)
			_threads[i].join();

		_processingDone = true;

		_collectMostCommons(results);

	}


protected:

	void _collectMostCommons(vector<pair<string, size_t>>& results) const
	{
		vector<pair<string, size_t>> all;
		for(const KmerCounter& km : _counters)
		{
			vector<pair<string, size_t>> tmp;
			km.getTopStrings(tmp);
			std::copy(tmp.begin(),tmp.end(), back_inserter(all));
		}
		std::sort(all.begin(), all.end(), [](const pair<string, size_t>& lhs, const pair<string, size_t>& rhs)
											{
												if(lhs.second >= rhs.second)
													return true;
												else
													return false;
											});
		// TODO refactor - extract method below
		int count=0;
		size_t prevSize = 0;
		for(int i =0;count<_n && i < all.size();i++)
		{
			const auto& tmp = all[i];
			results.push_back(tmp);
			if(prevSize!=tmp.second)
				count++;
			prevSize = tmp.second;
		}
	}

protected:
	int					_k;
	int					_n;
	int					_numOfThreads;
	bool				_processingDone;
	vector<KmerCounter> _counters;
	vector<std::thread> _threads;
};



}


#endif /* KMERS_H_ */
