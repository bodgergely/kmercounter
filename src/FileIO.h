#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>
#include <ifstream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <mutex>

namespace io
{

using std::string;
using std::ifstream;
using std::thread;
using std::unique_ptr;
using std:queue;
using std::mutex;

class InputBuffer
{
public:
	InputBuffer(size_t len) : _buffer(new char[len]), _allocated(len), _len(0)
	{
	}
	
	InputBuffer(const char* buffer, size_t len) : _buffer, _allocated(len), _len(len)
	{	
	}
	
	inline char* getBuffer() const {return _buffer.get();}
	inline size_t 	   getLen()	const {return _len;}
	inline void		   setLen(size_t len) {_len = len;}
	inline size_t		getAllocSize const {return _allocated;}
	
	
	~InputBuffer()
	{
	}
private:
	unique_ptr<char> _buffer;
	size_t _allocated;
	size_t _len;
};

class FileReader
{
public:
	FileReader(const std::string& path, size_t blockSize=1<<15) : _stream(path, ifstream::binary),_filePath(path), _blockSize(blockSize)
	{
		_stream.seekg(0, _stream.end);
		_fileSize = _stream.tellg();
		_stream.seekg(0, _stream.beg);
	}
	~FileReader(){}
	
	size_t blocksize() const {return _blockSize;}
	void   blocksize(size_t size) {_blockSize = size;}
	size_t filepath() const {return _filePath;}
	size_t filesize() const {return _fileSize;}
	

	void asyncReadBlocks()
	{
		//TODO
	}

protected:
	InputBuffer readNextBlock()
	{
		InputBuffer buf(_blockSize);
		_stream.read(buf.getBuffer(), _blockSize);
		
		if(_stream)
		{
			buf.setLen(_blockSize);
		}
		else
		{
			bytesRead.setLen(_stream.gcount());
		}
		
		return buf;
	}
	



protected:
	ifstream _stream;
	size_t	 _fileSize;
	string   _filePath;
	size_t	 _blockSize;
	mutex	 _mutexBufferQueue;
	queue<InputBuffer> _bufferQueue;
	thread	 _ioThread;
};



}

#endif
