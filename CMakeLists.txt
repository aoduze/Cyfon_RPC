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

# --- 处理 Protocol Buffers 文件 ---
file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/protocol/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# --- 定义目标 (Targets) ---
add_library(cyfon_rpc_lib STATIC
    src/Cyfon_RPC.cpp
    src/Cyfon_RPC.h
    ${PROTO_SRCS}
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
)

# --- 生成可执行文件 ---
add_executable(echo_server src/echo_server.cpp)
add_executable(echo_client src/echo_client.cpp)

# --- 【IntelliSense 增強設定】 ---
# 雖然這些路徑可以透過 cyfon_rpc_lib 傳遞，但明確地再次為執行檔指定，
# 這可以幫助 Visual Studio IntelliSense 正確找到標頭檔，消除紅線。
target_include_directories(echo_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories(echo_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# --- 链接可执行文件 ---
# 鏈接通用庫，依賴關係會自動傳遞
target_link_libraries(echo_server PRIVATE cyfon_rpc_lib)
target_link_libraries(echo_client PRIVATE cyfon_rpc_lib)