#pragma once

#include "buffer.h"
#include "rpc_header.h"
#include <span>

namespace cyfon_rpc {
	inline bool deserialize_header(const Buffer& buffer, RpcHeader& header) {
		if (buffer.readableBytes() < sizeof(header)) {
			return false; 
		}

		// first方返回一个span对象，表示缓冲区中前sizeof(RpcHeader)个字节的视图
		auto header_view = buffer.readableBytesView().first(sizeof(RpcHeader));
		std::memcpy(&header, header_view.data(), sizeof(RpcHeader));

		// 开始转换字节序
		header.message_size = networkToHost(header.message_size);
		header.method_id = networkToHost(header.method_id);
		header.service_id = networkToHost(header.service_id);
		header.request_id = networkToHost(header.request_id);
		header.stream_id = networkToHost(header.stream_id);
		header.sequence_number = networkToHost(header.sequence_number);
		header.reserved = networkToHost(header.reserved);
		// message_type 和 flags 是单字节，不需要转换

		return true;
	}

	inline void serialize_header(Buffer& buffer, const RpcHeader& header) {
		RpcHeader network_header = header;

		// 将所有字段从主机字节序转换为网络字节序
		network_header.message_size = hostToNetwork(network_header.message_size);
		network_header.service_id = hostToNetwork(network_header.service_id);
		network_header.method_id = hostToNetwork(network_header.method_id);
		network_header.request_id = hostToNetwork(network_header.request_id);
		network_header.stream_id = hostToNetwork(network_header.stream_id);
		network_header.sequence_number = hostToNetwork(network_header.sequence_number);
		network_header.reserved = hostToNetwork(network_header.reserved);
		// message_type 和 flags 是单字节，不需要转换

		buffer.append({ reinterpret_cast<const char*>(&network_header), sizeof(network_header) });
	}

	// 插入buffer预制头部
	inline void prepend_header(Buffer& buffer, const RpcHeader& header) {
		RpcHeader network_header = header;

		network_header.message_size = hostToNetwork(network_header.message_size);
		network_header.service_id = hostToNetwork(network_header.service_id);
		network_header.method_id = hostToNetwork(network_header.method_id);
		network_header.request_id = hostToNetwork(network_header.request_id);
		network_header.stream_id = hostToNetwork(network_header.stream_id);
		network_header.sequence_number = hostToNetwork(network_header.sequence_number);
		network_header.reserved = hostToNetwork(network_header.reserved);
		// message_type 和 flags 是单字节，不需要转换

		buffer.prepend(&network_header, sizeof(network_header));
	}
}