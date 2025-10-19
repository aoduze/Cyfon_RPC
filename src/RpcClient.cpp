#include "RpcClient.h";
#include "buffer.h"
#include "rpc_header.h"
#include "rpc_protocol_utils.h"
#include <iostream>
#include <string>

#include "calu.pb.h"

namespace cyfon_rpc {

	RpcClient::RpcClient(boost::asio::io_context& ioc) : ioc_(ioc), socket_(ioc) {}

	bool RpcClient::connect(const std::string& host, short port) {
		try {
			boost::asio::ip::tcp::resolver resolver(ioc_);
			auto endpoints = resolver.resolve(host, std::to_string(port));
			boost::asio::connect(socket_, endpoints);
			return true;
		}
		catch (std::exception& e) {
			std::cerr << " connect error" << std::endl;
			return false;
		}
	}
    
	void RpcClient::close() {
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	}

	Buffer RpcClient::send_receive(const Buffer& buf) {
		boost::asio::write(socket_, boost::asio::buffer(buf.readableBytesView().data(),
			buf.readableBytesView().size()));

		Buffer res_buf;
		size_t len = socket_.read_some(boost::asio::buffer(res_buf.readableBytesView().data(),
			res_buf.readableBytesView().size()));
		res_buf.hasWritten(len);
	}

	int main() {

	}
}