#include "rpc_header.h"
#include "rpc_server.h"
#include <iostream>
#include <boost/asio.hpp>
#include "Session.h"
#include "echo.pb.h"

class CalculatorServiceImpl : public cyfon_rpc::IService {
public:
	std::string callMethod(uint32_t method_id, const std::string& request_body) override {
		static const uint32_t METHOD_ADD = std::hash<std::string>{}("Add");
		static const uint32_t METHOD_SUBTRACT = std::hash<std::string>{}("Subtract");

		if (method_id == METHOD_ADD) {
			rpc_demo::AddRequest req;
			req.ParseFromString(request_body);

			rpc_demo::AddResponse res;
			res.set_result(req.a() + req.b());

			return res.SerializeAsString();
		}
		else if (method_id == METHOD_SUBTRACT) {
			rpc_demo::SubtractRequest req;
			req.ParseFromString(request_body);

			rpc_demo::SubtractResponse res;
			res.set_result(req.a() - req.b());

			return res.SerializeAsString();
		}

		return "";
	}
};

class TcpServer {
public:
	TcpServer(boost::asio::io_context& ioc_, short port, cyfon_rpc::RpcServer& rpc_server)
		: acceptor_(ioc_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
		, rpc_server_(rpc_server) {
		do_accept();
	}

private:
	void do_accept() {
		acceptor_.async_accept(
			[this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
				if (!ec) {
					std::make_shared<Session>(std::move(socket), rpc_server_)->start();
				}
				do_accept();
			});
	};

	boost::asio::ip::tcp::acceptor acceptor_;
	cyfon_rpc::RpcServer& rpc_server_;
};

int main() {
	try {
		// 1. 初始化 Asio 上下文
		boost::asio::io_context ioc_;

		// 2. 创建 RPC 服务注册和分发中心
		cyfon_rpc::RpcServer rpc_server;

		// 3. 注册 CalculatorService
		uint32_t service_id = std::hash<std::string>{}("CalculatorService");
		rpc_server.registerService(service_id, std::make_unique<CalculatorServiceImpl>());

		// 4. 创建并启动 TCP 服务器，将 RpcServer 注入
		short port = 8888;
		std::cout << "Server starting on port " << port << "..." << std::endl;
		TcpServer server(ioc_, port, rpc_server);

		// 5. 运行事件循环
		ioc_.run();

	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
