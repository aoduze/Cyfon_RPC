#pragma once
#include <cstdint>

// ȷ���ṹ����
#pragma pack(push,1)
namespace cyfon_rpc {
	struct RpcHeader {
		// ������Ϣ�ĳ���
		uint32_t message_size;

		// ����ID
		uint32_t service_id;

		// ����ID
		uint32_t method_id;
	};
}
#pragma pack(pop)

