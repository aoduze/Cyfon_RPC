#pragma once

#include <iostream>
#include "Session.h"
#include "rpc_protocol_utils.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "rpc_header.h"

namespace cyfon_rpc {
	class IService {
	public:
		virtual ~IService() = default;
		virtual std::string callMethod(uint32_t method_id, const std::string& request_body) = 0;
	};

	class RpcServer {
	public:
		void registerService(uint32_t service_id, std::unique_ptr<IService> service) {
			if (services_.count(service_id)) {
				return;
			}

			services_[service_id] = std::move(service);
			std::cout << "the id is success get in" << std::endl;
		}

		// 分发请求
		std::string dispatch(const RpcHeader& header, std::string& body) {
			// 首先转换字节序
			uint32_t service_id = networkToHost(header.service_id);
			uint32_t method_id = networkToHost(header.method_id);

			// 我们开始解析服务
			auto it = services_.find(service_id);
			if (it != services_.end() ) {
			    return 	it->second->callMethod(method_id, body);
			} 

			std::cerr << " not fine the id " << std::endl;
			return "";
		}
	private:
		std::unordered_map<uint32_t, std::unique_ptr<IService>> services_;
	};
}

