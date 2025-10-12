#include <iostream>
#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <algorithm>
#include <assert.h>
#include <string_view>
#include <WinSock2.h>
#include <cstdint>
#include <type_traits>
#include <span>
#include <cstddef>
#include <concepts>

#if __cplusplus >= 202002L && __has_include(<bit>)
#include <bit>
#endif

template<std::integral T>
T hostToNetwork(T value) noexcept {
#if __cplusplus >= 202302L
	if constexpr (std::endian::native == std::endian::little) {
		return std::byteswap(value);
	}
#endif
	if constexpr (sizeof(T) == 8) {
		return static_cast<T>(htonll(static_cast<uint64_t>(value))); // Assuming htonll is available or defined
	}
	else if constexpr (sizeof(T) == 4) {
		return static_cast<T>(htonl(static_cast<uint32_t>(value)));
	}
	else if constexpr (sizeof(T) == 2) {
		return static_cast<T>(htons(static_cast<uint16_t>(value)));
	}
	return value; 
}

template<std::integral T>
T networkToHost(T value) noexcept {
	return hostToNetwork(value); 
}


namespace cyfon_rpc {
	// +-------------------+------------------+------------------+
	// | prependable bytes |  readable bytes  |  writable bytes  |
	// |                   |     (CONTENT)    |                  |
	// +-------------------+------------------+------------------+
	// |                   |                  |                  |
	// 0      <=      readerIndex   <=   writerIndex    <=     size


	class Buffer {
	public:
		// 头部预留8字节
		static constexpr size_t kCheapPrepend = 8;
		// 初始缓冲区大小1024字节
		static constexpr size_t kInitialSize = 1024;

		explicit Buffer(size_t initialSize = kInitialSize)
			: buffer_(kCheapPrepend + initialSize),
			readerIndex_(kCheapPrepend),
			writerIndex_(kCheapPrepend) 
		    {}

		[[nodiscard]] size_t readableBytes() const noexcept { return writerIndex_ - readerIndex_; }
		[[nodiscard]] size_t writableBytes() const noexcept { return buffer_.size() - writerIndex_; }
		[[nodiscard]] size_t prependableBytes() const noexcept { return readerIndex_;}
		[[nodiscard]] size_t internalCapacity() const noexcept { return buffer_.capacity(); }

		[[nodiscard]] std::span<const std::byte> readableBytesView() const noexcept {
			return { reinterpret_cast<const std::byte*>(peek()), readableBytes() };
		}

		[[nodiscard]] std::span<std::byte> writableBytesView() noexcept {
			return { reinterpret_cast<std::byte*>(beginWrite()), writableBytes() };
		}

		// 返回可读数据的起始地址
		[[nodiscard]] const char* peek() const noexcept { return begin() + readerIndex_; }

		// 在缓冲区中查找"\r\n"
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

		// 查找一个换行符
		const char* findEOL() const {
			const void* eol = memchr(peek(), '\n', readableBytes());
			return static_cast<const char*>(eol);
		}

		// 从指定位置开始查找一个换行符
		const char* findEOL(const char* start) const {
			assert(peek() <= start);
			assert(start <= beginWrite());
			const void* eol = memchr(start, '\n', beginWrite() - start);
			return static_cast<const char*>(eol);
		}

		// 对内存进行预分配
		void reserve(size_t capacity) {
			buffer_.reserve(capacity);
		}

		// 前移读指针,也代表丢弃掉len字节的数据
		void retrieve(size_t len) {
			assert(len <= readableBytes());
			if (len < readableBytes()) {
				readerIndex_ += len;
			}
			else {
				retrieveAll();
			}
		}

		// 丢弃数据直到指定的end位置
		void retrieveUntil(const char* end) {
			assert(peek() <= end);
			assert(end <= beginWrite());
			retrieve(end - peek());
		}

		void retrieveInt64() { retrieve(sizeof(int64_t)); }
		void retrieveInt32() { retrieve(sizeof(int32_t)); }
		void retrieveInt16() { retrieve(sizeof(int16_t)); }
		void retrieveInt8() { retrieve(sizeof(int8_t)); }

		// 丢弃所有可读数据
		void retrieveAll() {
			readerIndex_ = kCheapPrepend;
			writerIndex_ = kCheapPrepend;
		}

		// 转换为string格式并丢弃数据
		[[nodiscard]] std::string retrieveAsString(size_t len) {
			assert(len <= readableBytes());
			std::string str(peek(), len);
			retrieve(len);
			return str;
		}

		// 取出所有数据并清空缓冲区(string格式)
		std::string retrieveAllAsString() {
			return retrieveAsString(readableBytes());
		}

		// 可读数据(string_view格式)
		[[nodiscard]] std::string_view toStringView() const {
			return std::string_view(peek(), static_cast<int>(readableBytes()));
		}

		// string_view版本的append
		void append(std::span<const std::byte> data) {
			ensureWritableBytes(data.size());	
			std::copy(data.begin(), data.end(), writableBytesView().begin());
			hasWritten(data.size());
		}

		// 确保有足够的的可写空间
		void ensureWritableBytes(size_t len) {
			if (writableBytes() < len) {
				makeSpace(len);
			}
			assert(writableBytes() >= len);
		}

		void swap(Buffer& rhs) noexcept {
			buffer_.swap(rhs.buffer_);
			std::swap(readerIndex_, rhs.readerIndex_);
			std::swap(writerIndex_, rhs.writerIndex_);
		}

		// 返回写指针
		char* beginWrite() { return begin() + writerIndex_; }
		const char* beginWrite() const { return begin() + writerIndex_; }

		// 写入数据后移动写指针
		void hasWritten(size_t len) {
			assert(len <= writableBytes());
			writerIndex_ += len;
		}

		// 回退数据, 回退写指针
		void unwrite(size_t len) {
			assert(len <= readableBytes());
			writerIndex_ -= len;
		}

		// 字节序转换
		static inline int64_t hostToNetwork64(int64_t host64) {
			uint64_t Temp_val = static_cast<uint64_t>(host64);

			// 获取原始 64 位数的高 32 位和低 32 位
			uint32_t high_part = static_cast<uint32_t>(Temp_val >> 32);
			uint32_t low_part = static_cast<uint32_t>(Temp_val & 0xFFFFFFFFLL);

			// 分别转换高低 32 位然后交换位置重新组装
			uint64_t swapped_unsigned = (static_cast<uint64_t>(htonl(low_part)) << 32) | htonl(high_part);

			return static_cast<int64_t>(swapped_unsigned);
		}

		// 查看，并转换为host字节序
		template<typename IntType>
		[[nodiscard]] IntType peekInt() const {
			static_assert(std::is_integral_v<IntType>);
			assert(readableBytes() >= sizeof(IntType));

			IntType network_value = 0;
			::memcpy(&network_value, peek(), sizeof(network_value));

			if constexpr (sizeof(IntType) == 8) {
				return networkToHost64(network_value);
			}
			else if constexpr (sizeof(IntType) == 4) {
				return boost::asio::detail::socket_ops::network_to_host_long(static_cast<uint32_t>(network_value));
			}
			else if constexpr (sizeof(IntType) == 2) {
				return boost::asio::detail::socket_ops::network_to_host_short(static_cast<uint16_t>(network_value));
			}
			else if constexpr (sizeof(IntType) == 1) {
				return network_value;
			}
			else {
				static_assert(sizeof(IntType) == 1 || sizeof(IntType) == 2 || sizeof(IntType) == 4 || sizeof(IntType) == 8, "Unsupported integer size");
				return 0; // Should not be reached
			}
		}

		template<typename IntType>
		IntType readInt() {
			IntType result = peekInt<IntType>();
			retrieve(sizeof(IntType));
			return result;
		}

		// 添加数据
		template<typename IntType>
		void appendInt(IntType value) {
			// 确保只用于整数类型
			static_assert(std::is_integral_v<IntType>, "Integer required.");

			IntType network_value = 0;
			if constexpr (sizeof(IntType) == 8) {
				network_value = hostToNetwork64(value); // 使用您文件中的64位转换
			}
			else if constexpr (sizeof(IntType) == 4) {
				network_value = boost::asio::detail::socket_ops::host_to_network_long(static_cast<uint32_t>(value));
			}
			else if constexpr (sizeof(IntType) == 2) {
				network_value = boost::asio::detail::socket_ops::host_to_network_short(static_cast<uint16_t>(value));
			}
			else if constexpr (sizeof(IntType) == 1) {
				network_value = value; // 1字节整数无需转换
			}
			else {
				// 在编译时报错，如果使用了不支持的整数大小
				static_assert(sizeof(IntType) == 1 || sizeof(IntType) == 2 || sizeof(IntType) == 4 || sizeof(IntType) == 8, "Unsupported integer size");
			}
			append(&network_value, sizeof(network_value));
		}

		// 头部预置数据
		template<typename IntType>
		void prependInt(IntType value) {
			static_assert(std::is_integral_v<IntType>);

			IntType network_value = 0;
			if constexpr (sizeof(IntType) == 8) {
				network_value = hostToNetwork64(value);
			}
			else if constexpr (sizeof(IntType) == 4) {
				network_value = boost::asio::detail::socket_ops::host_to_network_long(value);
			}
			else if constexpr (sizeof(IntType) == 2) { // Note: 'consteval' was a typo, corrected to 'constexpr'
				network_value = boost::asio::detail::socket_ops::host_to_network_short(value);
			}
			else if constexpr (sizeof(IntType) == 1) {
				network_value = value; // 1字节整数无需转换
			}
			else {
				static_assert(sizeof(IntType) == 1 || sizeof(IntType) == 2 || sizeof(IntType) == 4 || sizeof(IntType) == 8, "Unsupported integer size");
			}
			prepend(&network_value, sizeof(network_value));
		}

		// 在缓冲区头部预置一段长度为len的数据
		void prepend(const void* data, size_t len) {
			assert(len <= prependableBytes());
			readerIndex_ -= len;
			const char* d = static_cast<const char*>(data);
			std::copy(d, d + len, begin() + readerIndex_);
		}

		// 收缩内存
		void shrink() {
			buffer_.shrink_to_fit();
		}

		// 返回底层vector大小
		size_t internalCapacity() const {
			return buffer_.capacity();
		}

		// 直接从socket fd读取数据到缓冲区
		size_t readSock(boost::asio::ip::tcp::socket&, boost::system::error_code&);

	private:

		char* begin() { return &*buffer_.begin(); }
		const char* begin() const { return &*buffer_.begin(); }

		void makeSpace(size_t len) {
			if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
				buffer_.resize(len + writerIndex_);
				// FIX:: 移动read指针
			}
			else {
				// 空间足够的话就移动空间
				assert(kCheapPrepend < readerIndex_);
				size_t readable = readableBytes();
				std::move(begin() + readerIndex_,
					begin() + writerIndex_,
					begin() + kCheapPrepend);
				readerIndex_ = kCheapPrepend;
				writerIndex_ = readerIndex_ + readable;
				assert(readable == readableBytes());
			}
		}

		std::vector<char> buffer_;
		size_t readerIndex_;
		size_t writerIndex_;

		// 回车换行符
		static const char kCRLF[];
	};
}