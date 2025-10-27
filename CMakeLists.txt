# CMake 最低版本要求
cmake_minimum_required(VERSION 3.15)

# 项目名称
project(Cyfon_RPC)

# --- 全局设置 ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(/utf-8)
add_compile_definitions(-D_WIN32_WINNT=0x0A00) # 建議加上，避免 Boost.Asio 警告

# --- 查找依赖包 ---
find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS asio)
find_package(spdlog REQUIRED) # <--- 查找 spdlog

# --- 处理 Protocol Buffers 文件 ---
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/protocol/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# --- 定义目标 (Targets) ---
# 注意: 已删除所有无效的空行和非标准空格
# 将文件列表竖向排列，更清晰且不易出错
add_library(cyfon_rpc_lib STATIC
    ${PROTO_SRCS}
    "src/buffer.h"
    "src/buffer.cpp"
    "src/rpc_header.h"
    "src/Session.h"
    "src/Session.cpp"
    "src/rpc_server.h"
    "src/rpc_protocol_utils.h"
    "src/rpc_server.cpp"
    "src/RpcClient.h"
    "src/RpcClient.cpp"
    "src/threadpool.h"
    "src/rpc_channel.h"
)

# 為庫添加包含目錄
target_include_directories(cyfon_rpc_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

# 為庫鏈接它所依賴的庫
target_link_libraries(cyfon_rpc_lib PUBLIC
    protobuf::libprotobuf
    Boost::asio
    spdlog::spdlog # <--- 链接 spdlog
)

# --- 生成可执行文件 ---
# 注意: 同样清理了源文件列表
add_executable(buffer_test
    "src/test.cpp"
    "src/rpc_header.h"
    "src/Session.h"
    "src/Session.cpp"
    "src/rpc_server.h"
    "src/rpc_protocol_utils.h"
    "src/rpc_server.cpp"
    "src/RpcClient.h"
    "src/RpcClient.cpp"
    "src/threadpool.h"
    "src/rpc_channel.h"
    "src/rpc_service_macros.h"
)

# --- 链接可执行文件 ---
# 鏈接通用庫，依賴關係會自動傳遞
target_link_libraries(buffer_test PRIVATE cyfon_rpc_lib)