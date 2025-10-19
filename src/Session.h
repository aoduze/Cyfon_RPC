#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "buffer.h"
#include "rpc_header.h"

class RpcServer;

class Session : public std::enable_shared_from_this<Session> {
public:
	// 构造函数现在需要引用了，因为我们需要访问 RpcServer 的成员
	Session(boost::asio::ip::tcp::socket sock, cyfon_rpc::RpcServer& server) : socket_(std::move(sock)), server_(server) {}

	void start() { do_read(); }

private:
	void do_read();
	bool processMessage();
	void do_write(std::span<const char> data);

	boost::asio::ip::tcp::socket socket_;
	cyfon_rpc::Buffer socketBuffer_;
	cyfon_rpc::RpcServer& server_;
	// 可以把strand看作一个缓冲队列来理解即可
	boost::asio::strand<boost::asio::io_context::executor_type> write_strand_;
};