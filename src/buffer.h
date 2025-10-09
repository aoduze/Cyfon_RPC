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
	// ͷ��Ԥ��8�ֽ�
	static const size_t kCheapPrepend = 8;
	// ��ʼ��������С1024�ֽ�
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
	* ��Ҫ��ƵĹ���
	* 2.���ݶ�ȡ���� retrieve() retrieveAll() retrieveAsString()
	* 3.����д�빦�� append() ensureWritableBytes()
	* 4.��������չ���� makeSpace()
	* 5.����ת�� GetBufferAllString()
	*/

	// ���ؿɶ����ݵ���ʼ��ַ
	const char* peek() const { return begin() + readerIndex_; }
	
	// �������в���"\r\n"
	const char* findCRLF() const {
		const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}

	// ��ָ����start��ʼ����"\r\n"
	const char* findCRLF(const char* start) const {
		assert(peek() <= start);
		assert(start <= beginWrite());
		const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
		return crlf == beginWrite() ? nullptr : crlf;
	}

	// ���һ��з�
	const char* findEOL() const {
		const void* eol = memchr(peek(), '\n', readableBytes());
		return static_cast<const char*>(eol);
	}

	// ǰ�ƶ�ָ��,Ҳ�������ѵ�len�ֽ�����
	void retrieve(size_t len) {
		assert(len <= readableBytes());
		if (len < readableBytes()) {
			readerIndex_ += len;
		}
		else {
			retrieveAll();
		}
	}

	// ��������ֱ��ָ����endλ��
	void retrieveUntil(const char* end) {
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}

	void retrieveInt64() { retrieve(sizeof(int64_t)); }
	void retrieveInt32() { retrieve(sizeof(int32_t)); }
	void retrieveInt16() { retrieve(sizeof(int16_t)); }
	void retrieveInt8() { retrieve(sizeof(int8_t)); }

	// ���ѵ���������
	void retrieveAll() {
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}

	// ת��Ϊstring��ʽ�����ѵ�����
	std::string retrieveAsString(size_t len) {
		assert(len <= readableBytes());
		std::string str(peek(), readableBytes());
		retrieveAll();
		return str;
	}

	// ȡ���������ݲ���ջ�����(string��ʽ)
	std::string retrieveAllAsString() {
		return retrieveAsString(readableBytes()); 
	}

	// �ɶ�����(string_view��ʽ)
	std::string_view toStringView() const {
		return std::string_view(peek(), static_cast<int>(readableBytes()));
	}

	// string_view�汾��append
	void append(const std::string_view str) {
		append(str.data(), str.size());
	}

	// д������
	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}

	// void*�汾��append
	void append(const void* data, size_t len) {
		append(static_cast<const char*>(data), len);
	}

	// ȷ�����㹻�Ŀ�д�ռ�
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

	// ����д��ָ��
	char* beginWrite() { return begin() + writerIndex_; }
	const char* beginWrite() const { return begin() + writerIndex_; }

	// ǰ�����ݺ��ƶ�дָ��
	void hasWritten(size_t len) {
		assert(len <= writableBytes());
		writerIndex_ += len;
	}

	// ��������,����дָ��
	void unwrite(size_t len) {
		assert(len <= readableBytes());
		writerIndex_ -= len;
	}

	// ���ת��
	inline uint64_t hostToNetwork64(uint64_t host64) {
		// ��ȡԭʼ 64 λ�����ĸ� 32 λ�͵� 32 λ
		uint32_t high_part = static_cast<uint32_t>(host64 >> 32);
		uint32_t low_part = static_cast<uint32_t>(host64 & 0xFFFFFFFFLL); 

		// �ֱ�ת���ߵ� 32 λ��Ȼ�󽻻�λ��������װ
		return (static_cast<uint64_t>(htonl(low_part)) << 32) | htonl(high_part);
	}

	// �����ֽ���׷������
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

	// �س����з�
	static const char kCRLF[];
};

