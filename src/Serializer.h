/*
 * Serializer.h
 *
 *  Created on: May 13, 2016
 *      Author: bodger
 */

#ifndef SERIALIZER_H_
#define SERIALIZER_H_

#include <string>

using std::string;

class Encoded
{
public:
	Encoded(char* buffer, size_t size) : _memory(buffer), _size(size) {}
	~Encoded()
	{
		delete[] _memory;
	}

	const char* getBuffer() const { return _memory;}
	size_t      getSize() const {return _size;}
protected:
	char* _memory;
	size_t _size;

};


class Serializable
{
public:
	Serializable() {}
	virtual ~Serializable(){}
	Encoded serialize() = 0;
	void deserialize(const Encoded& enc) = 0;
};





#endif /* SERIALIZER_H_ */
