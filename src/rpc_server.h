#pragma once

#include "Session.h"
#include "rpc_protocol_utils.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "rpc_header.h"
#include "threadpool.h"
#include "spdlog/spdlog.h"
#include <vector>

namespace cyfon_rpc {
	enum class MethodType {
        UNARY,                 // 普通 RPC（一问一答）
        SERVER_STREAMING,      // 服务端流式（一个请求，多个响应）
        CLIENT_STREAMING,      // 客户端流式（多个请求，一个响应）
        BIDIRECTIONAL          // 双向流式（多个请求，多个响应）
	};

	// 流式调用的上下文
	class StreamContext {
	public:
		using SendCallback = std::function<void(std::span<const char>)>;
		using FinishCallback = std::function<void()>;

		StreamContext(SendCallback send, FinishCallback finish)
			: send_(send), finish_(finish) {}

		void send(const string& message) {
			if (send_)  send_(message); 
		}

		void finish() {
			if (finish_) finish_();
		}

	private:
		SendCallback send_;
		FinishCallback finish_;
	};

	class IService {
	public:		
		virtual ~IService() = default;

		// 获取方法类型
		virtual MethodType getMethodType(uint32_t method_id) {
			return MethodType::UNARY; 
			// 我们这里默认是普通RPC
		}

		// 兼容普通RPC
		virtual std::string callMethod(uint32_t method_id, const std::string& request_body) = 0;

		// 服务端流式 RPC
		virtual void callServerStreaming(
			uint32_t method_id,
			const std::string& request_body,
			StreamContext& stream
		) { stream.finish(); }

		// 客户端流式 RPC
		virtual std::string callClientStreaming(
			uint32_t method_id,
			const std::vector<std::string>& requests
		) { return ""; }

		// 双向
		virtual void callBidirectionalStreaming(
			uint32_t method_id,
			StreamContext& stream
		) { stream.finish(); }
	};

	class RpcServer {
	public:
		RpcServer(size_t thread_count = std::thread::hardware_concurrency()) : thread_pool_(thread_count){}

		// 注册函数将服务id和服务实例进行绑定
		void registerService(uint32_t service_id, std::unique_ptr<IService> service) {
			if (services_.count(service_id)) { return; }
			services_[service_id] = std::move(service);
			spdlog::info("the id is success get in");
		}

		// 获取服务
		IService* getService(uint32_t service_id) {
			auto it = services_.find(service_id);
			return it != services_.end() ? it -> second.get() : nullptr;
		}

		void enqueueStreamTask(const RpcHeader& header,
							   const std::string& body,
							   StreamContext& stream_ctx) 
		{
			auto it = services_.find(header.service_id);
			if (it == services_.end()) {
				spdlog::error("Service not found for stream: {}", header.service_id);
				stream_ctx.finish();
				return;
			}
		
			auto service = it -> second.get();

			thread_pool.enqueue([service, header, body, &stream_ctx]() {
				service -> callServerStreaming(header.method_id, body, stream_ctx);
			});
		}

		// 分发请求
		void enqueueTask(const RpcHeader& header, std::string bd, std::function<void(std::span<const char>)> response_callback) {
			thread_pool_.enqueue([this, header, body = std::move(bd), cb = std::move(response_callback)]() {
				
				auto it = services_.find(header.service_id);
				std::string response_payload;
				if (it != services_.end()) {
					response_payload = it -> second -> callMethod(header.method_id, body);
				}
				else {
					spdlog::error("error not found id");
				}

				Buffer response_buffer;
				response_buffer.append(response_payload);

				RpcHeader response_header;
				response_header.message_size = header.message_size;
				response_header.method_id = header.method_id;
				response_header.service_id = header.service_id;

				prepend_header(response_buffer, response_header);

				cb(response_buffer.readableBytesView());
				});
		}
	private:
		std::unordered_map<uint32_t, std::unique_ptr<IService>> services_;
		ThreadPool thread_pool_;
	};
}

