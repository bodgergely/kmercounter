/*
 * FileSerializer.h
 *
 *  Created on: May 14, 2016
 *      Author: geri
 */

#ifndef FILESERIALIZER_H_
#define FILESERIALIZER_H_

#include <fstream>

#include <Serializer.h>

using std::ofstream;
using std::ifstream;

struct SerializationInfo
{
	SerializationInfo(string filename_, size_t byteCount_) : filename(filename_), byteCount(byteCount_){}
	string filename;
	size_t byteCount;
};

class FileSerializer
{
public:
	static SerializationInfo write(const Serializable& obj, string filename)
	{
		Encoded enc = obj.serialize();
		ofstream f(filename.c_str(), std::ios_base::binary);

		const char* from = enc.getBuffer();
		size_t totalSize = enc.getSize();
		size_t blockSize = 1 << 15;
		int numOfIters = totalSize / blockSize + 1;
		size_t remainder = totalSize / blockSize;

		int j = 0;
		for(int i=numOfIters;i>1;i--)
		{
			f.write(from + j*blockSize, blockSize);
			j++;
		}
		if(remainder)
			f.write(from + j*blockSize, remainder);

		return SerializationInfo(filename, totalSize);

	}
	static void read(Serializable& obj, const SerializationInfo& info)
	{
		size_t totalSize = info.byteCount;
		char* buff = new char[totalSize];
		ifstream f(info.filename, std::ios_base::binary);

		size_t blockSize = 1 << 15;
		int numOfIters = totalSize / blockSize + 1;
		size_t remainder = totalSize / blockSize;

		int j = 0;
		for(int i=numOfIters;i>1;i--)
		{
			f.read(buff + j*blockSize, blockSize);
			j++;
		}
		if(remainder)
			f.read(buff + j*blockSize, remainder);

		obj.deserialize(Encoded(buff, totalSize));

	}
};


#endif /* FILESERIALIZER_H_ */