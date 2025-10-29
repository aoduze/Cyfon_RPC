#pragma once

#include <string>
#include <functional>
#include "rpc_header.h"
#include <boost/asio.hpp>
#include "buffer.h"

namespace cyfon_rpc {

	using StreamMessageCallback = std::function<void(const std::string& message)>;
	using StreamEndCallback = std::function<void()>;
	using StreamErrorCallback = std::function<void(const std::string& error)>;

	class RpcClient {
	public:
		// 调度器初始化
		RpcClient(boost::asio::io_context& io_context);

		bool connect(const std::string& host, short port);
		void close();

		// 普通RPC
		Buffer send_receive(const Buffer& request_buffer);

		// 流式服务端
		void callServerStreaming(
			uint32_t service_id,
			uint32_t method_id,
			const std::string& request_body,
			StreamMessageCallback on_message,
			StreamEndCallback on_end,
			StreamErrorCallback on_error
		);

		// 客户端流式
		class ClientStreamContext {
		public:
			void send(const std::string& message);
			void finish();
			
		private:
			friend class RpcClient;
			ClientStreamContext(RpcClient* client, uint32_t stream_id);
			RpcClient* client_;
			uint32_t stream_id_;
			uint32_t sequence_number_;
		};

		// 开启客户端流式调用
		std::shared_ptr<ClientStreamContext> callClientStreaming(

		)

		Buffer receive_buffer();
	private:
		boost::asio::io_context& ioc_;
		boost::asio::ip::tcp::socket socket_;
	};
}