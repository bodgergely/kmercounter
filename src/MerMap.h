/*
 * MerMap.h
 *
 *  Created on: May 14, 2016
 *      Author: geri
 */

#ifndef MERMAP_H_
#define MERMAP_H_

#include <Mer.h>
#include <Serializer.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <cassert>

using std::unordered_set;
using std::vector;

using std::cout;
using serialization::Serializable;
using serialization::Encoded;

namespace kmers
{
using std::unordered_map;


struct mer_count
{
	mer_count() : count(0){}
	mer_count(const mer_encoded& m, size_t c) : mer(m), count(c) {}
	mer_encoded mer;
	size_t	    count;
};


class MerMap : public Serializable
{
	using HashMap = unordered_map<mer_encoded, size_t, mer_encoded_hash>;
public:
	using const_iterator = HashMap::const_iterator;
	MerMap(size_t k) : _k(k){}
	~MerMap() {}

	// unordered_map interface
	inline void reserve(size_t s) {_map.reserve(s);}
	inline HashMap::const_iterator begin() const {return _map.begin();}
	inline HashMap::const_iterator end() const {return _map.end();}
	inline void					   clear() {_map.clear();}
	inline size_t				   size() {return _map.size();}
	size_t& operator[](const mer_encoded& key)
	{
		return _map[key];
	}

	// Serializable interface
	Encoded serialize() const
	{
		int i = 0;
		mer_count* buff = new mer_count[_map.size()];
		for(HashMap::const_iterator it = _map.begin(); it!=_map.end();it++)
		{
			const mer_encoded& mer = it->first;
			size_t			   count = it->second;
			buff[i++] = mer_count(mer, count);
		}
		Encoded encoded((char*)buff, sizeof(mer_count)*_map.size());
		return encoded;
	}


	void deserialize(const Encoded& enc)
	{
		mer_count* mers = (mer_count*)(enc.getBuffer());
		size_t bytes = enc.getSize();
		size_t count = bytes / sizeof(mer_count);
		assert((bytes % sizeof(mer_count)) == 0);

		for(int i=0;i<count;i++)
		{
			//cout << decode(mers[i].mer, _k) << std::endl;
			_merCountList.push_back(mers[i]);
		}

		//cout << "Count: " << count << "\n";

		_deserialized = true;

	}


	// my interface
	size_t totalCount()
	{
		size_t tc = 0;
		if(_deserialized)
		{
			for(const auto& m : _merCountList)
			{
				tc+=m.count;
			}
		}
		else
		{
			for(MerMap::const_iterator it=_map.begin();it!=_map.end();it++)
			{
				tc+=it->second;
			}
		}

		return tc;
	}

	/*
	 * brute force extraction of the top n size elements
	 */
	vector<mer_count> extract(size_t n)
	{
		vector<mer_count>* from = nullptr;
		if(_deserialized)
			from = &_merCountList;
		else
		{
			from = new vector<mer_count>();
			for(const auto& p : _map)
			{
				from->push_back(mer_count(p.first, p.second));
			}
		}

		vector<mer_count> res;
		int count = 0;
		size_t limitSize = std::numeric_limits<size_t>::max();
		while(count < n)
		{
			size_t biggest = 0;
			for(const auto& p : *from)
			{
				if(p.count > biggest && p.count < limitSize)
					biggest = p.count;
			}
			count++;
			limitSize = biggest;
			// second pass -> populate the res array with the biggest sizes
			for(const auto& p : *from)
			{
				if(p.count == biggest)
				{
					res.push_back(p);
				}
			}
		}

		return res;

	}

	vector<mer_count> extract(unordered_set<mer_encoded, mer_encoded_hash> mers)
	{
		vector<mer_count>* from = nullptr;
		if(_deserialized)
			from = &_merCountList;
		else
		{
			from = new vector<mer_count>();
			for(const auto& p : _map)
			{
				from->push_back(mer_count(p.first, p.second));
			}
		}

		vector<mer_count> res;
		for(const auto& p : *from)
		{
			if(mers.find(p.mer)!=mers.end())
				res.push_back(p);
		}

		return res;
	}


private:
	HashMap _map;
	vector<mer_count> _merCountList;
	bool _deserialized = false;
	size_t _k;
};


}

#endif /* MERMAP_H_ */
