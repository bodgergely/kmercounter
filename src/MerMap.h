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
#include <vector>

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
	MerMap() {}
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

	}


private:
	HashMap _map;
};


}

#endif /* MERMAP_H_ */
