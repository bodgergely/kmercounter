/*
 * Mer.h
 *
 *  Created on: May 9, 2016
 *      Author: geri
 */

#ifndef MER_H_
#define MER_H_
#include <stdint.h>
#include <string>
#include <stdexcept>
#include <cmath>
#include <cstring>

using std::string;

namespace kmers
{

const char elems[] = {'a', 'c', 'g', 't', 'n'};

static uint32_t getIndex(char c)
{
	if(c=='a' || c=='A')
		return 0;
	else if(c=='c' || c=='C')
		return 1;
	else if(c=='g' || c=='G')
		return 2;
	else if(c=='t' || c=='T')
		return 3;
	else if(c=='n' || c=='N')
		return 4;
	throw std::runtime_error("Invalid char!");
}

static char fromIndex(char index)
{
	if(index == 0x0)
		return 'a';
	else if(index == 0x1)
		return 'c';
	else if(index == 0x2)
		return 'g';
	else if(index == 0x3)
		return 't';
	else if(index == 0x4)
		return 'n';
	else
		throw std::runtime_error("Bad index!");

}


/**
 * encode the characters into this struct. Each char needs 3 bits (we have 5 possible chars) so we can have max of (64-1)/3 plus 32/3 = 31 long mers -the msf bit in the 64 bit field not used for simplicity
 */
struct mer_encoded
{
	mer_encoded() : low(0), high(0) {}
	inline mer_encoded(const mer_encoded& other);
	inline mer_encoded& operator=(const mer_encoded& rhs);
	uint32_t high;
	uint64_t low;
};

mer_encoded::mer_encoded(const mer_encoded& other)
{
	memcpy(reinterpret_cast<char*>(&(this->low)), reinterpret_cast<const char*>(&(other.low)), sizeof(uint64_t));
	memcpy(reinterpret_cast<char*>(&(this->high)), reinterpret_cast<const char*>(&(other.high)), sizeof(uint32_t));
}

mer_encoded& mer_encoded::operator=(const mer_encoded& other)
{
	memcpy(reinterpret_cast<char*>(&low), reinterpret_cast<const char*>(&(other.low)), sizeof(uint64_t));
	memcpy(reinterpret_cast<char*>(&high), reinterpret_cast<const char*>(&(other.high)), sizeof(uint32_t));
	return *this;
}

bool operator==(const mer_encoded& lhs, const mer_encoded& rhs)
{
	if( !memcmp(&(lhs.low), &(rhs.low), sizeof(uint64_t)) && !memcmp(&(lhs.high), &(rhs.high), sizeof(uint32_t)) )
		return true;
	else
		return false;
}


// TODO revise this!
class mer_encoded_hash
{
public:
	size_t operator()(const mer_encoded& mer) const
	{
		/*
		uint64_t l = integerHash(mer.low);
		uint32_t h = integerHash(mer.high);
		return l * (uint64_t)h;
		*/
		return (unsigned int)((size_t)(mer.low) + (size_t)(mer.high));
	}
private:
	inline uint32_t integerHash(uint32_t h) const
	{
	    h ^= h >> 16;
	    h *= 0x85ebca6b;
	    h ^= h >> 13;
	    h *= 0xc2b2ae35;
	    h ^= h >> 16;
	    return h;
	}

	inline uint64_t integerHash(uint64_t h) const
	{
		h ^= h >> 32;
		h *= 0x85ebca6b85ebca6b;
		h ^= h >> 26;
		h *= 0xc2b2ae35c2b2ae35;
		h ^= h >> 32;
		return h;
	}

};
mer_encoded encode(const char* s, size_t k)
{
	mer_encoded enc;
	memset(&enc, 0, sizeof(enc));
	uint64_t& v = enc.low;
	v = 0;
	int i = 0;
	uint64_t index = 0;
	for( ;i<k && i<21;i++)
	{
		index =  getIndex(*s++);
		v |= (index << (i*3));
	}
	if(i!=k)
	{
		uint32_t& h = enc.high;
		h = 0;
		int j=0;
		for( ;i<k;i++)
		{
			index = getIndex(*s++);
			h |= (index << (j*3));
			j++;
		}
	}

	return enc;
}

string decode(const mer_encoded& enc, size_t k)
{
	string s(k, 0);
	uint64_t v  = enc.low;
	int  i = 0;
	char index = 0;
	char c = 0;
	for( ;i<k && i<21;i++)
	{
		index = ((uint64_t)0x7 & (v >> (i*3)));
		c =  fromIndex(index);
		s[i] = c;
	}
	if(i!=k)
	{
		uint32_t h = enc.high;
		int j=0;
		for( ;i<k;i++)
		{
			index = ((uint64_t)0x7 & (h >> (j*3)));
			c = fromIndex(index);
			s[i] = c;
			j++;
		}
	}

	return s;
}

}


#endif /* MER_H_ */
