#pragma once

#include "rpc_server.h" // �������ڰ���·����
#include <google/protobuf/message.h>
#include <string>
#include <memory>
#include <iostream>
#include <sstream>      // ����Ĭ�ϴ�����־
#include <type_traits>  // ���� static_assert

namespace cyfon_rpc {

    // ============================================================================
    // RPC �궨�� V2
    // Ŀ��:
    // 1. ʹ�������ռ�ǰ׺ (CYFON_RPC_) ����ȫ����Ⱦ
    // 2. �ṩ�����ص�������������� (OnRpcError)
    // 3. ����ʵ�ֵķ�������Ϊ protected��������C++�̳�ֱ��
    // ============================================================================

    /**
     * @brief �ڷ�������в���һ�������صĴ�����ʵ�֡�
     */
#define CYFON_RPC_DEFINE_ERROR_HANDLER() \
protected: \
         virtual std::string OnRpcError(const char* method_name, const char* step, const std::string& error_message) {\
             std::cerr << "[RPC_ERROR] ServiceID: " << kServiceId \
             << ", Method: " << method_name \
             << ", Step: " << step \
             << ", Error: " << error_message << std::endl; \
             return ""; /* Ĭ�Ϸ��ؿ��ַ��� */ \
     } \
private: /* �л��� private �����غ�����ʵ��ϸ�� */

#define CYFON_RPC_SERVICE_BEGIN(ServiceClassName, ServiceID) \
class ServiceClassName : public cyfon_rpc::IService { \
public: \
    static constexpr uint32_t kServiceId = ServiceID; \
    \
    enum MethodId : uint32_t { \
        kInvalidMethod = 0,

     /**
      * @brief �ڷ����ж���һ�� RPC ��������Ψһ�� MethodID��
      */
#define CYFON_RPC_METHOD(MethodName, MethodID) \
        k##MethodName = MethodID,

      /**
       * @brief ��������ö�٣�����ʼ���� callMethod ��ʵ�֡�
       */
#define CYFON_RPC_METHODS_END() \
    }; \
         std::string callMethod(uint32_t method_id, const std::string& request_body) override {\
             switch (method_id) {
#define CYFON_RPC_DISPATCH(MethodName, RequestType, ResponseType) \
            case k##MethodName: { \
                RequestType request; \
                if (!request.ParseFromString(request_body)) { \
                    return OnRpcError(#MethodName, "ParseRequest", "Failed to parse " #RequestType); \
                } \
                \
                /* ����������ʵ�ֵ��麯�� */ \
                ResponseType response = MethodName(request); \
                \
                std::string response_str; \
                if (!response.SerializeToString(&response_str)) { \
                    return OnRpcError(#MethodName, "SerializeResponse", "Failed to serialize " #ResponseType); \
                } \
                return response_str; \
            }

                  /**
                   * @brief �����ַ� switch ��䣬������δ֪����ID��
                   */
#define CYFON_RPC_DISPATCH_END() \
            default: { \
                return OnRpcError("Unknown", "Dispatch", "Unknown method_id: " + std::to_string(method_id)); \
            } \
        } /* end switch */ \
    } /* end callMethod */ \
    \
    /* ע������صĴ��������� */ \
    CYFON_RPC_DEFINE_ERROR_HANDLER() \
    \
protected: \
    /* * �������л��� protected:
     * ���ڹ������� private: ��ͬ (�����������˽���麯��),
     * ���� C++ �У����ӿ�����Ϊ protected �������ر����� "����Ϊ����׼����" ��ͼ��
     */

     /**
      * @brief ����һ���������ʵ�ֵ� RPC ���������麯������
      */
#define CYFON_RPC_DECLARE_METHOD(MethodName, RequestType, ResponseType) \
    virtual ResponseType MethodName(const RequestType& request) = 0;

      /**
       * @brief ����������Ķ��塣
       */
#define CYFON_RPC_SERVICE_END() \
}; /* end class ServiceClassName */

       /**
        * @brief ע��һ������ʵ�ֵĺꡣ
        * ������ static_assert ��ȷ��ע���������Ч�ķ���
        */
#define CYFON_RPC_REGISTER_SERVICE(server_instance, ServiceImplClass) \
    do { \
        static_assert(std::is_base_of_v<cyfon_rpc::IService, ServiceImplClass>, \
                      #ServiceImplClass " must inherit from cyfon_rpc::IService (usually via CYFON_RPC_SERVICE_BEGIN)"); \
        (server_instance).registerService(ServiceImplClass::kServiceId, std::make_unique<ServiceImplClass>()); \
    } while(0)

} // namespace cyfon_rpc