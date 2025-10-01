#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <windows.h>

#include "echo.pb.h"

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {}

	void start() {
		do_read();
	}

private:
	void do_read() {
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			[this, self](boost::system::error_code ec, std::size_t length) {
				if (!ec) {
					// 解析接收到的数据
					echo::EchoRequest request;
					if (request.ParseFromArray(data_, length)) {
						std::cout << "Received message: " << request.message() << std::endl;
						// 构造响应数据
						echo::EchoResponse response;
						response.set_message("Echo: " + request.message());
						response_data_ = response.SerializeAsString();
						do_write();
					} else {
						std::cerr << "Failed to parse message." << std::endl;
					}
				}
			});
	}

	void do_write() {
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(response_data_),
			[this, self](boost::system::error_code ec, std::size_t /*length*/) {
				if (!ec) {
					std::cout << "success to submit" << std::endl;
					do_read();
				}
			});
	}

private:
	/*
	 * socket_ 为当前会话的套接字
	 * max_length 定义了缓冲区的最大长度
	 * response_data_ 用于存储响应数据
	**/
	boost::asio::ip::tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
	std::string response_data_;
};

class Server {
public:
	Server(boost::asio::io_context& io_context, short port)
		: acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
		do_accept();
	}

private:
	void do_accept() {
		acceptor_.async_accept(
			[this](std::error_code ec, boost::asio::ip::tcp::socket socket) {
				if (!ec) {
					std::cout << "Connect success" << std::endl;
					std::make_shared<Session>(std::move(socket))->start();
				}
				do_accept();
			});
	}

	boost::asio::ip::tcp::acceptor acceptor_;
};

int main() {
	try {
		SetConsoleOutputCP(CP_UTF8);

		boost::asio::io_context io_context;

		Server server(io_context, 8888);
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception :" << e.what() << std::endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}