#include <iostream>
#include <boost/asio.hpp>
#include <fstream>
#include <string>

#include "echo.pb.h"

int main() {
	try {
		SetConsoleOutputCP(CP_UTF8);

		boost::asio::io_context io_context;

		// 解析端口
		boost::asio::ip::tcp::resolver resolver(io_context);
		auto endpoints = resolver.resolve("lo.calhost", "8888");

		// 创建并连接套接字
		boost::asio::ip::tcp::socket socket(io_context);
		boost::asio::connect(socket, endpoints);

		std::cout << "Connect Success" << std::endl;

		// 用户输出消息
		std::string Message;
		std::cout << "Please enter the message" << std::endl;
		std::getline(std::cin, Message);

		// 创建并序列化请求
		echo::EchoRequest request;
		request.set_message(Message);

		std::string serialized_request;
		if (!request.SerializeToString(&serialized_request)) {
			std::cerr << "Error " << std::endl;
			return 1;
		}

		// 发送请求
		boost::asio::write(socket, boost::asio::buffer(serialized_request));
		std::cout << "Request sent: " << Message << std::endl;

		// 接收响应
		std::vector<char> response_buffer(1024);
		size_t len = socket.read_some(boost::asio::buffer(response_buffer));

		// 反序列化响应
		echo::EchoResponse response;
		if (!response.ParsePartialFromArray(response_buffer.data(), static_cast<int>(len))) {
			std::cout << "success" << std::endl;
		}
		else {
			std::cout << "Response received: " << std::endl;
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception" << e.what() << std::endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}