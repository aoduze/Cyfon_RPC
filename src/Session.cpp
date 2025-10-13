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
			else
			{
				std::cerr << "Read error: " << ec.message() << std::endl;
			}
		});
}

bool Session::processMessage() {
    // 检测是否足够解析出一个完整的消息头
	cyfon_rpc::RpcHeader header;
	if (!cyfon_rpc::deserialize_header(socketBuffer_, header)) {
		
	}
}

