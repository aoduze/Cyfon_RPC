

---------------------------------------------------------------
#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <cassert>
#include <cstddef> // For std::byte
#include <cstdint>
#include <concepts> // For std::integral
#include <ranges>   // For std::ranges::copy
#include <span>

#include <boost/asio.hpp>

// C++20 bit header for endian conversions
#if __cplusplus >= 202002L && __has_include(<bit>)
#include <bit>
#endif

// --- Network Byte Order Conversion Helpers ---
// Use std::byteswap from C++23 if available, otherwise fallback.
// This is cleaner than the manual implementation.
template<std::integral T>
T hostToNetwork(T value) noexcept {
#if __cplusplus >= 202302L
    if constexpr (std::endian::native == std::endian::little) {
        return std::byteswap(value);
    }
#endif
    // Fallback for older standards or big-endian machines
    if constexpr (sizeof(T) == 8) {
        return static_cast<T>(htonll(static_cast<uint64_t>(value))); // Assuming htonll is available or defined
    }
    else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(htonl(static_cast<uint32_t>(value)));
    }
    else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(htons(static_cast<uint16_t>(value)));
    }
    return value; // 1-byte values need no conversion
}

template<std::integral T>
T networkToHost(T value) noexcept {
    return hostToNetwork(value); // Byte swapping is symmetric
}


namespace cyfon_rpc {

    // Buffer Layout:
    // +-------------------+------------------+------------------+
    // | prependable bytes |  readable bytes  |  writable bytes  |
    // +-------------------+------------------+------------------+
    // 0      <=      readerIndex   <=   writerIndex    <=     size

    class Buffer {
    public:
        static constexpr size_t kCheapPrepend = 8;
        static constexpr size_t kInitialSize = 1024;

        explicit Buffer(size_t initialSize = kInitialSize)
            : buffer_(kCheapPrepend + initialSize),
            readerIndex_(kCheapPrepend),
            writerIndex_(kCheapPrepend) {
        }

        void swap(Buffer& rhs) noexcept {
            buffer_.swap(rhs.buffer_);
            std::swap(readerIndex_, rhs.readerIndex_);
            std::swap(writerIndex_, rhs.writerIndex_);
        }

        // --- Capacity Information ---
        [[nodiscard]] size_t readableBytes() const noexcept { return writerIndex_ - readerIndex_; }
        [[nodiscard]] size_t writableBytes() const noexcept { return buffer_.size() - writerIndex_; }
        [[nodiscard]] size_t prependableBytes() const noexcept { return readerIndex_; }
        [[nodiscard]] size_t internalCapacity() const noexcept { return buffer_.capacity(); }

        // --- Data Views (using std::span for safety) ---
        [[nodiscard]] std::span<const std::byte> readableBytesView() const noexcept {
            return { buffer_.data() + readerIndex_, readableBytes() };
        }

        [[nodiscard]] std::span<std::byte> writableBytesView() noexcept {
            return { buffer_.data() + writerIndex_, writableBytes() };
        }

        // --- Data Retrieval ---
        void retrieve(size_t len) {
            assert(len <= readableBytes());
            if (len < readableBytes()) {
                readerIndex_ += len;
            }
            else {
                retrieveAll();
            }
        }

        void retrieveUntil(const char* end) {
            const char* start = reinterpret_cast<const char*>(peek());
            assert(start <= end);
            assert(end <= reinterpret_cast<const char*>(beginWrite()));
            retrieve(end - start);
        }

        void retrieveAll() noexcept {
            readerIndex_ = kCheapPrepend;
            writerIndex_ = kCheapPrepend;
        }

        template<std::integral IntType>
        void retrieveInt() {
            retrieve(sizeof(IntType));
        }

        [[nodiscard]] std::string retrieveAsString(size_t len) {
            assert(len <= readableBytes());
            const char* data_ptr = reinterpret_cast<const char*>(peek());
            std::string result(data_ptr, len);
            retrieve(len);
            return result;
        }

        [[nodiscard]] std::string retrieveAllAsString() {
            return retrieveAsString(readableBytes());
        }

        // --- Data Appending ---
        void append(std::span<const std::byte> data) {
            ensureWritableBytes(data.size());
            std::ranges::copy(data, writableBytesView().begin());
            hasWritten(data.size());
        }

        // Convenience overload for string-like data
        void append(const std::string_view str) {
            append(std::as_bytes(std::span{ str }));
        }

        template<std::integral IntType>
        void appendInt(IntType value) {
            IntType network_value = hostToNetwork(value);
            append(std::as_bytes(std::span{ &network_value, 1 }));
        }

        // --- Data Prepending ---
        void prepend(std::span<const std::byte> data) {
            assert(data.size() <= prependableBytes());
            readerIndex_ -= data.size();
            std::ranges::copy(data, buffer_.data() + readerIndex_);
        }

        template<std::integral IntType>
        void prependInt(IntType value) {
            IntType network_value = hostToNetwork(value);
            prepend(std::as_bytes(std::span{ &network_value, 1 }));
        }

        // --- Peeking and Reading Data ---
        template<std::integral IntType>
        [[nodiscard]] IntType peekInt() const {
            assert(readableBytes() >= sizeof(IntType));
            IntType network_value = 0;
            auto view = readableBytesView().first<sizeof(IntType)>();
            std::copy(view.begin(), view.end(), std::as_writable_bytes(std::span{ &network_value, 1 }).begin());
            return networkToHost(network_value);
        }

        template<std::integral IntType>
        [[nodiscard]] IntType readInt() {
            IntType result = peekInt<IntType>();
            retrieve(sizeof(IntType));
            return result;
        }

        [[nodiscard]] std::string_view toStringView() const {
            const char* data = reinterpret_cast<const char*>(peek());
            return { data, readableBytes() };
        }

        // --- Socket Operations ---
        size_t readSock(boost::asio::ip::tcp::socket& sock, boost::system::error_code& ec) {
            auto writable_view = writableBytesView();
            // Boost.Asio can work directly with std::byte spans
            size_t n = sock.read_some(boost::asio::buffer(writable_view.data(), writable_view.size()), ec);
            if (!ec && n > 0) {
                hasWritten(n);
            }
            return n;
        }

    private:
        // Internal pointer access (for legacy compatibility if needed)
        [[nodiscard]] const std::byte* peek() const noexcept { return buffer_.data() + readerIndex_; }
        [[nodiscard]] std::byte* beginWrite() noexcept { return buffer_.data() + writerIndex_; }

        void hasWritten(size_t len) noexcept {
            assert(len <= writableBytes());
            writerIndex_ += len;
        }

        void ensureWritableBytes(size_t len) {
            if (writableBytes() < len) {
                makeSpace(len);
            }
            assert(writableBytes() >= len);
        }

        void makeSpace(size_t len) {
            if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
                // Not enough space even if we move data, so resize.
                buffer_.resize(writerIndex_ + len);
            }
            else {
                // Move readable data to the front to create writable space.
                size_t readable = readableBytes();
                std::move(buffer_.begin() + readerIndex_,
                    buffer_.begin() + writerIndex_,
                    buffer_.begin() + kCheapPrepend);
                readerIndex_ = kCheapPrepend;
                writerIndex_ = readerIndex_ + readable;
            }
        }

        std::vector<std::byte> buffer_;
        size_t readerIndex_;
        size_t writerIndex_;
    };

} // namespace cyfon_rpc