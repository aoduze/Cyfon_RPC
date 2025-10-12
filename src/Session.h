#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "buffer.h"
#include "rpcheader.h"

class RpcServer;

class Session : public std::enable_shared_from_this<Session> {
public:
	// ���캯��������Ҫ�����ˣ���Ϊ������Ҫ���� RpcServer �ĳ�Ա
	Session(boost::asio::ip::tcp::socket sock, RpcServer& server) : socket_(std::move(socket), server_(server)) {}

	void start() { do_read(); }

private:
	void do_read();
	void processMessage();
	void do_write(std::span<const std::byte> data);


	boost::asio::ip::tcp::socket socket_;
	cyfon_rpc::Buffer socketBuffer_;
	RpcServer& server_;
};