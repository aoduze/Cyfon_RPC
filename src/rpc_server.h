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

namespace cyfon_rpc {
	enum class MethodType {
        UNARY,                 // 普通 RPC（一问一答）
        SERVER_STREAMING,      // 服务端流式（一个请求，多个响应）
        CLIENT_STREAMING,      // 客户端流式（多个请求，一个响应）
        BIDIRECTIONAL          // 双向流式（多个请求，多个响应）
	};

	class IService {
	public:		
		virtual ~IService() = default;
		virtual std::string callMethod(uint32_t method_id, const std::string& request_body) = 0;
		virtual MethodType getMethodType(uint32_t method_id) {
			return MethodType::UNARY;  // 默认是普通 RPC
		}
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

				// 
				cb(response_buffer.readableBytesView());
				});
		}
	private:
		std::unordered_map<uint32_t, std::unique_ptr<IService>> services_;
		ThreadPool thread_pool_;
	};
}

