#pragma once

#include "buffer.h"
#include "rpc_header.h"
#include <span>

namespace cyfon_rpc {
	inline bool deserialize_header(const Buffer& buffer, RpcHeader& header) {
		if (buffer.readableBytes() < sizeof(header)) {
			return false; 
		}

		// first������һ��span���󣬱�ʾ��������ǰsizeof(RpcHeader)���ֽڵ���ͼ
		auto header_view = buffer.readableBytesView().first(sizeof(RpcHeader));
		std::memcpy(&header, header_view.data(), sizeof(RpcHeader));

		// ��ʼת���ֽ���
		header.message_size = networkToHost(header.message_size);
		header.method_id = networkToHost(header.method_id);
		header.service_id = networkToHost(header.service_id);

		return true;
	}

	inline void serialize_header(Buffer& buffer, const RpcHeader& header) {
		RpcHeader network_header = header;

		// �������ֶδ������ֽ���ת��Ϊ�����ֽ���
		network_header.message_size = hostToNetwork(network_header.message_size);
		network_header.service_id = hostToNetwork(network_header.service_id);
		network_header.method_id = hostToNetwork(network_header.method_id);

		buffer.append({ reinterpret_cast<const char*>(&network_header), sizeof(network_header) });
	}

	// ����bufferԤ��ͷ��
	inline void prepend_header(Buffer& buffer, const RpcHeader& header) {
		RpcHeader network_header = header;

		
		network_header.message_size = hostToNetwork(network_header.message_size);
		network_header.service_id = hostToNetwork(network_header.service_id);
		network_header.method_id = hostToNetwork(network_header.method_id);

		buffer.prepend(&network_header, sizeof(network_header));
	}
}