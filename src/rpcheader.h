#pragma once
#include <cstdint>

// 确保结构紧凑
#pragma pack(push,1)
struct RpcHeader {
	// 整个消息的长度
	uint32_t message_size;

    // 服务ID
	uint32_t service_id;

    // 方法ID
	uint32_t method_id;
};
#pragma pack(pop)

