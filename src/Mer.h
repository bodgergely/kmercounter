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
	uint32_t high;
	uint64_t low;
};

bool operator==(const mer_encoded& lhs, const mer_encoded& rhs)
{
	if((lhs.high == rhs.high) && (lhs.low == lhs.low))
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

		return (unsigned int)((size_t)(mer.low) + (size_t)(mer.high));
	}
};
mer_encoded encode(const char* s, size_t k)
{
	mer_encoded enc;
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
