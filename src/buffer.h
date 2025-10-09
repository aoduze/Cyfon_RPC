#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <algorithm>
#include <assert.h>
#include <string_view>
#include <WinSock2.h>
#include <cstdint>

// +-------------------+------------------+------------------+
// | prependable bytes |  readable bytes  |  writable bytes  |
// |                   |     (CONTENT)    |                  |
// +-------------------+------------------+------------------+
// |                   |                  |                  |
// 0      <=      readerIndex   <=   writerIndex    <=     size


class Buffer {
public:
	// 头部预留8字节
	static const size_t kCheapPrepend = 8;
	// 初始缓冲区大小1024字节
	static const size_t kInitialSize = 1024;

	explicit Buffer(size_t initialSize = kInitialSize) : 
		buffer_(kCheapPrepend + initialSize),
		readerIndex_(kCheapPrepend),
		writerIndex_(kCheapPrepend) 
	    {
		assert(readableBytes() == 0);
		assert(writableBytes() == initialSize);
		assert(prependableBytes() == kCheapPrepend);
		}

	size_t readableBytes() const { return writerIndex_ - readerIndex_; }
	size_t writableBytes() const { return buffer_.size() - writerIndex_; }
	size_t prependableBytes() const { return readerIndex_; }

	/*
	* 需要设计的功能
	* 2.数据读取功能 retrieve() retrieveAll() retrieveAsString()
	* 3.数据写入功能 append() ensureWritableBytes()
	* 4.缓冲区扩展功能 makeSpace()
	* 5.数据转换 GetBufferAllString()
	*/

	// 返回可读数据的起始地址
	const char* peek() const { return begin() + readerIndex_; }
	
	// 在数据中查找"\r\n"
	const char* findCRLF() const {
		const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}

	// 从指定的start开始查找"\r\n"
	const char* findCRLF(const char* start) const {
		assert(peek() <= start);
		assert(start <= beginWrite());
		const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}

	// 查找换行符
	const char* findEOL() const {
		const void* eol = memchr(peek(), '\n', readableBytes());
		return static_cast<const char*>(eol);
	}

	// 前移读指针,也就是消费掉len字节数据
	void retrieve(size_t len) {
		assert(len <= readableBytes());
		if (len < readableBytes()) {
			readerIndex_ += len;
		}
		else {
			retrieveAll();
		}
	}

	// 消费数据直到指定的end位置
	void retrieveUntil(const char* end) {
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}

	void retrieveInt64() { retrieve(sizeof(int64_t)); }
	void retrieveInt32() { retrieve(sizeof(int32_t)); }
	void retrieveInt16() { retrieve(sizeof(int16_t)); }
	void retrieveInt8() { retrieve(sizeof(int8_t)); }

	// 消费掉所有数据
	void retrieveAll() {
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}

	// 转换为string形式并消费掉数据
	std::string retrieveAsString(size_t len) {
		assert(len <= readableBytes());
		std::string str(peek(), readableBytes());
		retrieveAll();
		return str;
	}

	// 取出所有数据并清空缓冲区(string形式)
	std::string retrieveAllAsString() {
		return retrieveAsString(readableBytes()); 
	}

	// 可读区域(string_view形式)
	std::string_view toStringView() const {
		return std::string_view(peek(), static_cast<int>(readableBytes()));
	}

	// string_view版本的append
	void append(const std::string_view str) {
		append(str.data(), str.size());
	}

	// 写入数据
	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}

	// void*版本的append
	void append(const void* data, size_t len) {
		append(static_cast<const char*>(data), len);
	}

	// 确保有足够的可写空间
	void ensureWritableBytes(size_t len) {
		if (writableBytes() < len) {
			makeSpace(len);
		}
		assert(writableBytes() >= len);
	}

	void swap(Buffer& rhs) {
		buffer_.swap(rhs.buffer_);
		std::swap(readerIndex_, rhs.readerIndex_);
		std::swap(writerIndex_, rhs.writerIndex_);
	}

	// 返回写区指针
	char* beginWrite() { return begin() + writerIndex_; }
	const char* beginWrite() const { return begin() + writerIndex_; }

	// 前移数据后，移动写指针
	void hasWritten(size_t len) {
		assert(len <= writableBytes());
		writerIndex_ += len;
	}

	// 撤销操作,后移写指针
	void unwrite(size_t len) {
		assert(len <= readableBytes());
		writerIndex_ -= len;
	}

	// 大端转换
	inline uint64_t hostToNetwork64(uint64_t host64) {
		// 提取原始 64 位整数的高 32 位和低 32 位
		uint32_t high_part = static_cast<uint32_t>(host64 >> 32);
		uint32_t low_part = static_cast<uint32_t>(host64 & 0xFFFFFFFFLL); 

		// 分别转换高低 32 位，然后交换位置重新组装
		return (static_cast<uint64_t>(htonl(low_part)) << 32) | htonl(high_part);
	}

	// 网络字节序追加数据
	void appendInt64(int64_t x) {
		uint64_t be64 = hostToNetwork64(static_cast<uint64_t>(x));
		append(&be64, sizeof be64);
	}

private:
	char* begin() { return &*buffer_.begin(); }
	const char* begin() const { return &*buffer_.begin(); }

	std::vector<char> buffer_;
	size_t readerIndex_;
	size_t writerIndex_;

	// 回车换行符
	static const char kCRLF[];
};

