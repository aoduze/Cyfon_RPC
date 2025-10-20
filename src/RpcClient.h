#pragma once

#include <string>
#include <boost/asio.hpp>
#include "buffer.h"

namespace cyfon_rpc {
	class RpcClient {
	public:
		RpcClient(boost::asio::io_context& io_context);

		bool connect(const std::string& host, short port);

		void close();

		Buffer send_receive(const Buffer& request_buffer);

		Buffer receive_buffer();
	private:
		boost::asio::io_context& ioc_;
		boost::asio::ip::tcp::socket socket_;
	};
}