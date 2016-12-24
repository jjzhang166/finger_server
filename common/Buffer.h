

#ifndef Protobuf_net_Buffer_H
#define Protobuf_net_Buffer_H


#include "Poco/Net/StreamSocket.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <assert.h>


namespace Protobuf_net
{

/// A buffer class modeled after netty's ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | discardable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
public:
	static const int kInitialSize = 1024;

	Buffer();

	// default copy-ctor, dtor and assignment are fine

	void swap(Buffer& rhs);

	int readableBytes();

	int writableBytes();

	int discardableBytes();

	const char* peek() const;

	// retrieve returns void, to prevent
	// string str(retrieve(readableBytes()), readableBytes());
	// the evaluation of two functions are unspecified
	void retrieve(int length);

	// length of buffer must >= length
	int retrieve(char* buffer, int length);

	void retrieveAll();

	void retrieveUntil(const char* end);

	std::string retrieveAllAsString();

	std::string retrieveAsString(int length);

	void append(const std::string& str);

	void append(const char* data, int length);

	void shrink(int reserve);

	/// Read data directly into buffer.
	int receiveBytes(Poco::Net::StreamSocket& ss);

private:
	char* beginWrite();

	const char* beginWrite() const;

	char* begin();

	const char* begin() const;

	void makeSpace(int more);

private:
	std::vector<char> _buffer;
	int _readerIndex;
	int _writerIndex;
};

//public:

inline int Buffer::readableBytes()
{ 
	return _writerIndex - _readerIndex; 
}

inline int Buffer::writableBytes()
{ 
	return _buffer.size() - _writerIndex; 
}

inline int Buffer::discardableBytes()
{
	return _readerIndex; 
}

inline const char* Buffer::peek() const
{ 
	return begin() + _readerIndex; 
}

inline void Buffer::retrieveAll()
{
	_readerIndex = 0;
	_writerIndex = 0;
}

//private:

inline char* Buffer::beginWrite()
{ 
	return begin() + _writerIndex; 
}

inline const char* Buffer::beginWrite() const
{ 
	return begin() + _writerIndex; 
}

inline char* Buffer::begin()
{ 
	return &*_buffer.begin(); 
}

inline const char* Buffer::begin() const
{ 
	return &*_buffer.begin(); 
}

}

#endif  // Protobuf_net_Buffer_H
