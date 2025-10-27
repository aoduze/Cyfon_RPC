#pragma once

#include "RpcClient.h"
#include "rpc_header.h"
#include "rpc_protocol_utils.h"
#include <google/protobuf/message.h>
#include <cstdint>
#include <string>

namespace cyfon_rpc {

    // RpcChannel - 提供类型安全的客户端调用接口
    class RpcChannel {
    public:
        RpcChannel(RpcClient& client, uint32_t service_id)
            : client_(client), service_id_(service_id) {
        }

        // 模板方法：自动处理序列化和反序列化
        template<typename RequestType, typename ResponseType>
        ResponseType callMethod(uint32_t method_id, const RequestType& request) {
            // 序列化请求
            std::string request_body;
            if (!request.SerializeToString(&request_body)) {
                throw std::runtime_error("Failed to serialize request");
            }

            // 构造请求缓冲区
            Buffer request_buffer;
            request_buffer.append(request_body);

            // 添加 RPC 头部
            RpcHeader header;
            header.message_size = static_cast<uint32_t>(request_body.size());
            header.service_id = service_id_;
            header.method_id = method_id;
            prepend_header(request_buffer, header);

            // 发送并接收响应
            Buffer response_buffer = client_.send_receive(request_buffer);

            // 解析响应头部
            RpcHeader response_header;
            if (!parse_header(response_buffer, response_header)) {
                throw std::runtime_error("Failed to parse response header");
            }

            // 反序列化响应
            ResponseType response;
            std::string response_body = response_buffer.retrieveAllAsString();
            if (!response.ParseFromString(response_body)) {
                throw std::runtime_error("Failed to parse response");
            }

            return response;
        }

    private:
        RpcClient& client_;
        uint32_t service_id_;
    };

    // 为特定服务创建强类型 Channel 的辅助宏
#define DEFINE_RPC_CHANNEL(ChannelClassName, ServiceID) \
class ChannelClassName : public cyfon_rpc::RpcChannel { \
public: \
    ChannelClassName(cyfon_rpc::RpcClient& client) \
        : RpcChannel(client, ServiceID) {} \
    \
private:

#define RPC_CHANNEL_METHOD(MethodName, MethodID, RequestType, ResponseType) \
public: \
    ResponseType MethodName(const RequestType& request) { \
        return callMethod<RequestType, ResponseType>(MethodID, request); \
    }

#define END_RPC_CHANNEL() \
};

} // namespace cyfon_rpc