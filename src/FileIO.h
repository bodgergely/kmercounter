#ifndef FILEIO_H_
#define FILEIO_H_

#include <string>
#include <fstream>
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
using std::queue;
using std::mutex;
using std::condition_variable;

class InputBuffer
{
public:
	InputBuffer() : _buffer(nullptr), _allocated(0), _len(0) {}

	InputBuffer(size_t len) : _buffer(new char[len]), _allocated(len), _len(0)
	{
	}
	
	InputBuffer(char* buffer, size_t len) : _buffer(buffer), _allocated(len), _len(len)
	{	
	}
	
	inline char* getBuffer() const {return _buffer;}
	inline size_t 	   getLen()	const {return _len;}
	inline void		   setLen(size_t len) {_len = len;}
	inline size_t	   getAllocSize() const {return _allocated;}
	inline void		   setEndOfStream() {_endOfStream = true;}
	inline bool		   isEndofStream() const {return _endOfStream;}

	
	
	~InputBuffer()
	{
	}
private:
	char* _buffer;
	size_t _allocated;
	size_t _len;
	bool	_endOfStream = false;
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
	~FileReader()
	{
		_ioThread.join();
	}
	
	size_t blocksize() const {return _blockSize;}
	void   blocksize(size_t size) {_blockSize = size;}
	const string& filepath() const {return _filePath;}
	size_t filesize() const {return _fileSize;}
	

	/*
	 * Async reading into the queue - retrieve using the getNextBlock function
	 */
	void startReadingBlocks()
	{
		_ioThread = thread(&FileReader::doRead, this);
	}

	void getNextBlock(InputBuffer& buffer)
	{
		std::unique_lock<mutex> lock(_mutexBufferQueue);
		while(_bufferQueue.empty())
			_condvarQueue.wait(lock);
		buffer = _bufferQueue.front();
		_bufferQueue.pop();
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
			buf.setLen(_stream.gcount());
			buf.setEndOfStream();
		}
		
		return buf;
	}
	
	void doRead()
	{
		while(true)
		{
			InputBuffer buf = readNextBlock();
			pushToQueue(buf);
			if(buf.isEndofStream())
			{
				break;
			}
		}
		_finishedReadingFile = true;
	}

	void pushToQueue(const InputBuffer& buffer)
	{
		std::unique_lock<mutex> lock(_mutexBufferQueue);
		_bufferQueue.push(buffer);
		_condvarQueue.notify_one();
	}



protected:
	bool 	  _finishedReadingFile = false;
	ifstream _stream;
	size_t	 _fileSize;
	string   _filePath;
	size_t	 _blockSize;
	mutex	 _mutexBufferQueue;
	condition_variable _condvarQueue;
	queue<InputBuffer> _bufferQueue;
	thread	 _ioThread;
};



}

#endif
