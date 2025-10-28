#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include "buffer.h"
#include "rpc_header.h"
#include <unordered_map>
#include <mutex>
#include <vector>

class RpcServer;

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(boost::asio::ip::tcp::socket sock, cyfon_rpc::RpcServer& server)
		: socket_(std::move(sock)),
		  server_(server),
		  write_strand_(socket_.get_executor()),
		  next_stream_id_(1) {}

	void start() { do_read(); }

private:
	struct Stream {
		uint32_t stream_id;					// 流ID
		uint32_t request_id;				// 请求ID
		cyfon_rpc::MethodType method_type;  // 方法类型
		uint32_t service_id;				// 服务ID	
		uint32_t method_id;					// 方法ID
		uint32_t sequence_number;			// 消息序号
		bool is_active;						// 是否活跃

		// 客户端流: 收集客户端发送来的流信息
		std::vector<std::string> collected_message;
	};

	void do_read();
	bool processMessage();
	void do_write(std::span<const char> data);

	// 消息处理方法
	void handleRequest(const cyfon_rpc::RpcHeader& header, const std::string& payload);
	void handleStreamMessage(const cyfon_rpc::RpcHeader& header, const std::string& payload);

	// 流管理方法
	uint32_t createStream(const cyfon_rpc::RpcHeader& header);
	void sendStreamMessage(uint32_t stream_id, const std::string& message, bool is_end = false);
	void closeStream(uint32_t stream_id);

	boost::asio::ip::tcp::socket socket_;
	cyfon_rpc::Buffer socketBuffer_;
	cyfon_rpc::RpcServer& server_;
	boost::asio::strand<boost::asio::io_context::executor_type> write_strand_;
	std::unordered_map<uint32_t, Stream> streams_;
	uint32_t next_stream_id_;
	std::mutex stream_mutex_;
};