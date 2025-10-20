#include "Session.h"
#include "rpc_server.h"
#include "rpc_protocol_utils.h"

void Session::do_read() {
	auto self = shared_from_this();
	socket_.async_read_some(
		boost::asio::buffer(socketBuffer_.writableBytesView().data(), socketBuffer_.writableBytesView().size()),
		[this, self](boost::system::error_code ec, size_t length) {
			if (!ec) {
				socketBuffer_.hasWritten(length);
				while (processMessage()) {
					std::cout << "cerr" << std::endl;
				}
				do_read();
			}
			else {
				std::cerr << "Read error: " << ec.message() << std::endl;
			}
		});
}

bool Session::processMessage() {
    // 检测是否足够解析出一个完整的消息头
	cyfon_rpc::RpcHeader header;
	if (!cyfon_rpc::deserialize_header(socketBuffer_, header)) {
		return false;
	}

	if (socketBuffer_.readableBytes() < header.message_size) {
		return false;
	}

	//------------------------------
	// 至此，我们解析出了一个完整的消息
	// 开始消费信息
	socketBuffer_.retrieve(sizeof(cyfon_rpc::RpcHeader));
	std::string payload = socketBuffer_.retrieveAsString(header.message_size - sizeof(cyfon_rpc::RpcHeader));
	server_.enqueueTask(header, std::move(payload),
		[self = shared_from_this()](std::span<const char> response_data) {
			self -> do_write(response_data);
		});
	return true;
}

void Session::do_write(std::span<const char> data) {
	// 为了确保数据在异步写操作完成前不会被销毁，我们将数据拷贝到一个shared_ptr管理的vector中
	auto shared_data = std::make_shared<std::vector<char>>(data.begin(), data.end());

	boost::asio::post(write_strand_, [self = shared_from_this(), shared_data]() {
		boost::asio::async_write(self->socket_, boost::asio::buffer(*shared_data),
			[self, shared_data](boost::system::error_code ec, std::size_t /*length*/) {
				if (ec) {
					std::cerr << "Write error: " << ec.message() << std::endl;
				}
			});
		});
}

