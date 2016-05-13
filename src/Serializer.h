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

// abstract
class Encoded
{
public:
	Encoded();
	virtual ~Encoded() = 0;
};

Encoded::~Encoded()
{
}

class Serializable
{
public:
	Serializable() {}
	virtual ~Serializable(){}
	Encoded serialize() = 0;
	void deserialize(const Encoded& enc) = 0;
};


class Serializer
{
public:
	Serializer(Serializable& obj) : _obj(obj) {}
	virtual ~Serializer() {}
	void write()
	{
		Encoded enc = _obj.serialize();
		_strat.write(enc);
	}
	void read(T& res)
	{
		_strat.read(enc);
		_obj.read(res);
	}
};



#endif /* SERIALIZER_H_ */
