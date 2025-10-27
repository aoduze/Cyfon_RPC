#include "rpc_header.h"
#include "rpc_server.h"
#include <iostream>
#include <boost/asio.hpp>
#include "Session.h"
#include "spdlog/spdlog.h"

#include "calu.pb.h"

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
					std::make_shared<Session>(std::move(socket), rpc_server_) -> start();
				}
				do_accept();
			});
	};

	boost::asio::ip::tcp::acceptor acceptor_;
	cyfon_rpc::RpcServer& rpc_server_;
};

int main() {
	try {
		boost::asio::io_context ioc;

		cyfon_rpc::RpcServer rpc_server(std::thread::hardware_concurrency());

		uint32_t service_id = std::hash<std::string>{}("CalculatorService");
		rpc_server.registerService(service_id, std::make_unique<CalculatorServiceImpl>());

		short port = 8888;
		spdlog::info("Server starting on port {} .....", port);
		TcpServer server(ioc, port, rpc_server);

		const size_t io_thread_count = std::thread::hardware_concurrency();
		std::vector<std::thread> io_threads;
		io_threads.reserve(io_thread_count);

		spdlog::info("Starting {} I/O threads.", io_thread_count);
		for (size_t i = 0; i < io_thread_count; ++i) {
			io_threads.emplace_back([&ioc]() { ioc.run(); });
		}

		for (auto& t : io_threads) {
			if (t.joinable()) {
				t.join();
			}
		}
	} catch (std::exception& e) {
		spdlog::error("Exception: {}", e.what());
	}

	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
