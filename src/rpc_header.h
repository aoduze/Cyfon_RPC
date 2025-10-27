#pragma once
#include <cstdint>

#pragma pack(push, 1)
namespace cyfon_rpc {
	// 消息类型
	enum class MessageType : uint8_t {
		REQUEST  = 0x01,  // 普通请求
		RESPONSE = 0x02,  // 普通响应
		STREAM   = 0x03,  // 流数据
		ERROR    = 0x04,  // 错误消息
		PING     = 0x05,  // 心跳请求
		PONG     = 0x06,  // 心跳响应
	};

	// 标志位
	enum Flag : uint8_t {
		NONE     = 0x00,  // 无
		STREAM_BEGIN = 0x01,   // 流的第一条消息
        STREAM_END   = 0x02,   // 流的最后一条消息
        COMPRESSED   = 0x04,   // 数据已压缩（可选，未来扩展）
        ENCRYPTED    = 0x08,   // 数据已加密（可选，未来扩展）
	};

	struct RpcHeader {
		uint32_t message_size;      // 消息总长度（包含 header）
        uint32_t service_id;        // 服务ID
        uint32_t method_id;         // 方法ID
        
        // ===== 新增字段（流式支持）=====
        uint32_t request_id;        // 请求ID（客户端生成，唯一标识一次调用）
        uint32_t stream_id;         // 流ID（0=非流式，>0=流式调用）
        uint32_t sequence_number;   // 消息序号（流中的位置，从1开始）
        
        uint8_t  message_type;      // 消息类型（MessageType）
        uint8_t  flags;             // 标志位（Flags）
        uint16_t reserved;          // 保留字段（未来扩展）
	};

	static_assert(sizeof(RpcHeader) == 32, "RpcHeader size is not 32 bytes");
}
#pragma pack(pop)