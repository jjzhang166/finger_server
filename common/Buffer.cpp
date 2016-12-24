

#include "Buffer.h"

#include "Poco/Net/NetException.h"

using namespace Protobuf_net;

Buffer::Buffer()
	: _buffer(kInitialSize),
	_readerIndex(0),
	_writerIndex(0)
{
	assert(readableBytes() == 0);
	assert(writableBytes() == kInitialSize);
	assert(discardableBytes() == 0);
}


void Buffer::retrieve(int length)
{
	assert(length <= readableBytes());
	if (length < readableBytes())
	{
		_readerIndex += length;
	}
	else if(length == readableBytes())
	{
		retrieveAll();
	}
}


void Buffer::retrieveUntil(const char* end)
{
	assert(peek() <= end);
	assert(end <= beginWrite());
	retrieve(end - peek());
}


int Buffer::retrieve(char* buffer, int length)
{
	if (length > readableBytes())
	{
		length = readableBytes();
		std::copy(peek(), peek() + length, buffer);
		retrieveAll();
	}
	else
	{
		std::copy(peek(), peek() + length, buffer);
		retrieve(length);
	}	
	return length;
}


std::string Buffer::retrieveAllAsString()
{
	return retrieveAsString(readableBytes());;
}


std::string Buffer::retrieveAsString(int length)
{
	assert(length <= readableBytes());
	std::string result(peek(), length);
	retrieve(length);
	return result;
}


void Buffer::append(const std::string& str)
{
	append(str.data(), str.length());
}

void Buffer::append(const char* data, int length)
{
	if (writableBytes() < length)
	{
		makeSpace(length);
	}
	assert(length <= writableBytes());
	std::copy(data, data + length, beginWrite());
	_writerIndex += length;
}


void Buffer::shrink(int reserve)
{
	std::vector<char> buf(readableBytes()+reserve);
	std::copy(peek(), peek()+readableBytes(), buf.begin());
	buf.swap(_buffer);
}

int Buffer::receiveBytes(Poco::Net::StreamSocket& ss)
{
	try
	{
		char extrabuf[65536];
		int n = ss.receiveBytes(extrabuf, sizeof(extrabuf));

		if (n > 0)
		{
			append(extrabuf, n);	    
		}

		return n;
	}
	catch (Poco::Exception&)
	{
		throw;
	}
}

void Buffer::swap(Buffer& rhs)
{
	_buffer.swap(rhs._buffer);
	std::swap(_readerIndex, rhs._readerIndex);
	std::swap(_writerIndex, rhs._writerIndex);
}


void Buffer::makeSpace(int more)
{
	if (writableBytes() + discardableBytes() < more)
	{
		_buffer.resize(_writerIndex+more);
	}
	else
	{
		// move readable data to the front, make space inside buffer
		int used = readableBytes();
		std::copy(begin()+_readerIndex,
			begin()+_writerIndex,
			begin());
		_readerIndex = 0;
		_writerIndex = _readerIndex + used;
		assert(used == readableBytes());
	}
}
