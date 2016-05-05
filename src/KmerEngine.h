#ifndef KMERPROCESSOR_H_
#define KMERPROCESSOR_H_

#include <KmerProcessor.h>
#include <FileIO.h>

namespace kmers
{


class KmerEngine
{
public:
	KmerEngine(const std::string& filePath, int k, int n, int threadCount) : _k(k), _n(n), _threadCount(threadCount), _fileio(filePath)
	{
		
	}
private:
	size_t _k;
	size_t _n;
	size_t _threadCount;
	FileIO _fileio;
};



}

#endif