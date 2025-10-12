#include "buffer.h"
#include <boost/system.hpp>

namespace cyfon_rpc {

	const char Buffer::kCRLF[] = "\r\n";

	const size_t Buffer::kCheapPrepend;
	const size_t Buffer::kInitialSize;

    size_t Buffer::readSock(boost::asio::ip::tcp::socket& sock, boost::system::error_code& ec) {
        auto writable_view = writableBytesView();
        size_t n = sock.read_some(boost::asio::buffer(writable_view.data(), writable_view.size()), ec);
        if (!ec && n > 0) {
            hasWritten(n);
        }
        return n;
    }
}