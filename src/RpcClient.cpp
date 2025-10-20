#include "RpcClient.h";
#include "buffer.h"
#include "rpc_header.h"
#include "rpc_protocol_utils.h"
#include <iostream>
#include <string>
#include <boost/asio.hpp>

#include "calu.pb.h"

namespace cyfon_rpc {

	RpcClient::RpcClient(boost::asio::io_context& ioc) : ioc_(ioc), socket_(ioc) {}

	bool RpcClient::connect(const std::string& host, short port) {
		try {
			boost::asio::ip::tcp::resolver resolver(ioc_);
			auto endpoints = resolver.resolve(host, std::to_string(port));
			boost::asio::connect(socket_, endpoints);
			return true;
		}
		catch (std::exception& e) {
			std::cerr << " connect error" << std::endl;
			return false;
		}
	}

	void RpcClient::close() {
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	}

	Buffer RpcClient::send_receive(const Buffer& buf) {
		boost::system::error_code ec;

		boost::asio::write(socket_, boost::asio::buffer(buf.readableBytesView()), ec);
		if (ec) { std::cerr << "client write error" << std::endl;  return; }

		// 用来承载服务端回传的数据
		Buffer response_buffer;
		response_buffer.ensureWritableBytes(sizeof(RpcHeader));
		boost::asio::read(socket_, boost::asio::buffer(response_buffer.writableBytesView().data(),
			sizeof(RpcHeader)), ec);
		if (ec) { std::cerr << "error response_buffer" << ec.message() << std::endl; return; }
		response_buffer.hasWritten(sizeof(RpcHeader));

		RpcHeader response_header;
		if (!deserialize_header(response_buffer, response_header)) {
			std::cerr << "deserialize error " << std::endl;
			return;
		}

		// 读取响应体
		size_t body_len = response_header.message_size - sizeof(RpcHeader);
		if (body_len > 0) {
			response_buffer.ensureWritableBytes(body_len);
			boost::asio::read(socket_, boost::asio::buffer(response_buffer.writableBytesView().data(), body_len), ec);
			if (ec) {
				std::cerr << "Failed to read response body: " << ec.message() << std::endl;
				return;
			}
			response_buffer.hasWritten(body_len);
		}
		return response_buffer;
	}
}


int main() {
	try {
		boost::asio::io_context ioc;
		cyfon_rpc::RpcClient client(ioc);

		if (!client.connect("127.0.0.1", 8888)) {
			return 1;
		}
		std::cout << "Successfully connected to server." << std::endl;

		// --- 构造 Add(15, 27) 请求 ---
		rpc_demo::AddRequest add_req;
		add_req.set_a(15);
		add_req.set_b(27);
		std::string add_req_body = add_req.SerializeAsString();

		cyfon_rpc::Buffer request_buffer;
		// 先追加Body
		request_buffer.append(add_req_body);

		// 再预置Header (这里就是 prependableBytes 的用武之地)
		cyfon_rpc::RpcHeader header;
		header.service_id = std::hash<std::string>{}("CalculatorService");
		header.method_id = std::hash<std::string>{}("Add");
		header.message_size = sizeof(cyfon_rpc::RpcHeader) + add_req_body.size();
		cyfon_rpc::prepend_header(request_buffer, header);

		// --- 发送请求并接收响应 ---
		std::cout << "Sending Add(15, 27) request..." << std::endl;
		cyfon_rpc::Buffer response_buffer = client.send_receive(request_buffer);

		if (response_buffer.readableBytes() > sizeof(cyfon_rpc::RpcHeader)) {
			// 跳过Header，处理Body
			response_buffer.retrieve(sizeof(cyfon_rpc::RpcHeader));

			rpc_demo::AddResponse add_res;
			if (add_res.ParseFromArray(response_buffer.peek(), response_buffer.readableBytes())) {
				std::cout << ">>> Add Response received. Result: " << add_res.result() << std::endl;
			}
			else {
				std::cerr << "Error: Failed to parse AddResponse protobuf." << std::endl;
			}
		}
		else {
			std::cerr << "Error: Received an invalid or empty response." << std::endl;
		}

		client.close();
		std::cout << "Connection closed." << std::endl;

	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}