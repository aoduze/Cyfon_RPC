#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <windows.h> // For SetConsoleOutputCP

int main() {
    try {
        // 设置控制台为 UTF-8 编码
        SetConsoleOutputCP(CP_UTF8);

        boost::asio::io_context io_context;

        // 解析服务器地址和端口
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("localhost", "8888"); // "lo.calhost" -> "localhost"

        // 创建并连接 socket
        boost::asio::ip::tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::cout << "Successfully connected to server." << std::endl;
        std::cout << "Enter a message to send (or type 'exit' to quit):" << std::endl;

        // 使用循环来持续发送和接收消息
        for (;;) {
            std::string message;
            std::getline(std::cin, message);

            if (message.empty() || message == "exit") {
                break;
            }

            boost::asio::write(socket, boost::asio::buffer(message));
            std::cout << "Sent: " << message << std::endl;

            // 2. 接收服务器的回显
            std::vector<char> response_buffer(1024);
            boost::system::error_code error;
            size_t len = socket.read_some(boost::asio::buffer(response_buffer), error);

            if (error == boost::asio::error::eof) {
                // 连接被对方关闭
                std::cout << "Connection closed by server." << std::endl;
                break;
            }
            else if (error) {
                throw boost::system::system_error(error); // 其他错误
            }

            // 3. 将收到的原始数据转为字符串并打印
            // 我们不再尝试用 Protobuf 反序列化
            std::string received_message(response_buffer.data(), len);
            std::cout << "Received: " << received_message << std::endl << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // 在这个纯文本客户端中，我们不再需要 Protobuf 库
    // google::protobuf::ShutdownProtobufLibrary();

    return 0;
}