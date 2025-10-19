#pragma once

#include <iostream>
#include "Session.h"
#include "rpc_protocol_utils.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "rpc_header.h"
#include "threadpool.h"

namespace cyfon_rpc {
	class IService {
	public:
		virtual ~IService() = default;
		virtual std::string callMethod(uint32_t method_id, const std::string& request_body) = 0;
	};

	class RpcServer {
	public:

		RpcServer(size_t thread_count = std::thread::hardware_concurrency()) : thread_pool_(thread_count){}

		// 注册函数将服务id和服务实例进行绑定
		void registerService(uint32_t service_id, std::unique_ptr<IService> service) {
			if (services_.count(service_id)) { return; }
			services_[service_id] = std::move(service);
			std::cout << "the id is success get in" << std::endl;
		}

		// 分发请求
		void enqueueTask(const RpcHeader& header, std::string bd, std::function<void(std::span<const char>)> response_callback) {
			thread_pool_.enqueue([this, header, body = std::move(bd), cb = std::move(response_callback)]() {
				
				auto it = services_.find(header.service_id);
				std::string response_payload;
				if (it != services_.end()) {
					response_payload = it->second -> callMethod(header.method_id, body);
				}
				else {
					std::cerr << "error not found id" << std::endl;
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

