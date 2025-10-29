#include "Session.h"
#include "rpc_server.h"
#include "rpc_protocol_utils.h"
#include "spdlog/spdlog.h"

void Session::do_read() {
	auto self = shared_from_this();
	socket_.async_read_some(
		boost::asio::buffer(socketBuffer_.writableBytesView().data(), socketBuffer_.writableBytesView().size()),
		[this, self](boost::system::error_code ec, size_t length) {
			if (!ec) {
				socketBuffer_.hasWritten(length);
				spdlog::debug("Socket read {} bytes.", length);
				while (processMessage()) {
					spdlog::debug("Processed one complete message in buffer.");
				}
				do_read();
			}
			else {
				if (ec == boost::asio::error::eof) {
					spdlog::info("Client disconnected gracefully. (EOF)");
				}
				else {
					spdlog::error("Read error: {}", ec.message()); 
				}
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

	// 根据消息类型分发
	auto msg_type = static_cast<cyfon_rpc::MessageType>(header.message_type);

	switch (msg_type) {
		case cyfon_rpc::MessageType::REQUEST:
		    handleRequest(header, payload);
			break;
		
		case cyfon_rpc::MessageType::STREAM:
		    handleStreamMessage(header, payload);
			break;

		// 检测心跳
		case cyfon_rpc::MessageType::PING:
			spdlog::debug("Received PING message");
			break;
			
		default:
			spdlog::warn("warn message type: {}", (int)header.message_type);
			break;
	}

	return true;
}

void Session::do_write(std::span<const char> data) {
	// 为了确保数据在异步写操作完成前不会被销毁，我们将数据拷贝到一个shared_ptr管理的vector中
	auto shared_data = std::make_shared<std::vector<char>>(data.begin(), data.end());

	boost::asio::post(write_strand_, [self = shared_from_this(), shared_data]() {
		boost::asio::async_write(self->socket_, boost::asio::buffer(*shared_data),
			[self, shared_data](boost::system::error_code ec, std::size_t /*length*/) {
				if (ec) {
					spdlog::error("read error {}", ec.message());
				}
			});
		});
}

void Session::handleRequest(const cyfon_rpc::RpcHeader& header, const std::string& payload) {
	auto service= server_.getService(header.service_id);
	if(!service) {
		spdlog::error(" Service not found : {}", header.service_id);
		return;
	}

	// 获取方法类型
	auto method_type = service -> getMethodType(header.method_id);

	// 根据方法类型处理
	if(method_type == cyfon_rpc::MethodType::UNARY) {
		// 普通RPC
		server_.enqueueTask(header, payload, 
			[self = shared_from_this()](std::span<const char> response_data) {
				self -> do_write(response_data);
			});
	}
	else if (method_type == cyfon_rpc::MethodType::SERVER_STREAMING) {
		// 服务端流式
		uint32_t stream_id = createStream(header);

		spdlog::info("Created server streaming, stream_id={}, method_id={}",
					stream_id, header.method_id);

		cyfon_rpc::StreamContext stream_ctx(
			[self = shared_from_this(), stream_id] (const std::string&message) {
				self -> sendStreamMessage(stream_id, message);
			},
			[self = shared_from_this(), stream_id]() {
				self -> closeStream(stream_id);
			});
		
		server_.enqueueStreamTask(header, payload, stream_ctx);
	} 
	else if (method_type == cyfon_rpc::MethodType::BIDIRECTIONAL) {
		spdlog::warn("Bidirectional streaming not implemented yet");
	}
	else if(method_type == cyfon_rpc::MethodType::CLIENT_STREAMING) {
		// 客户端流式
		uint32_t stream_id = createStream(header);
		spdlog::info("Created client streaming, stream_id = {}, method_id = {}",
			stream_id, header.method_id);
	}
}

void Session::handleStreamMessage(const cyfon_rpc::RpcHeader& header, const std::string& payload) {
	std::lock_guard<std::mutex> lock(stream_mutex_);

	// 根据stream_id 查找流
	auto it = streams_.find(header.stream_id);
	if (it == streams_.end()) {
		spdlog::warn("Stream not found: {}", header.stream_id);
		return ;
	}

	// Stream具体方法，这里是string流结构体
	auto& stream = it -> second;

	// 根据流方法推断
	if (stream.method_type == cyfon_rpc::MethodType::CLIENT_STREAMING) {
		stream.collected_message.push_back(payload);
		stream.sequence_number++;

		// 检查是不是最后一条消息
		if(header.flags & cyfon_rpc::Flag::STREAM_END) {
			spdlog::info("Client streaming finished, stream_id= {}, total message = {}",
				header.stream_id, stream.collected_message.size());
			
			auto service = server_.getService(stream.service_id);
			if (service) {
				std::string response = service -> callClientStreaming(
					stream.method_id,
					stream.collected_message
				);

				cyfon_rpc::Buffer response_buffer;
				response_buffer.append(response);

				cyfon_rpc::RpcHeader response_header;
                response_header.message_size = sizeof(cyfon_rpc::RpcHeader) + response.size();
                response_header.service_id = stream.service_id;
                response_header.method_id = stream.method_id;
                response_header.request_id = stream.request_id;
                response_header.stream_id = stream.stream_id;
                response_header.sequence_number = 0;
                response_header.message_type = static_cast<uint8_t>(cyfon_rpc::MessageType::RESPONSE);
                response_header.flags = cyfon_rpc::Flag::NONE;
                response_header.reserved = 0;

				cyfon_rpc::prepend_header(response_buffer, response_header);
				self -> do_write(response_buffer.readableBytesView());
			}
			closeStream(header.stream_id);
		}
	}
	else if (stream.method_type == cyfon_rpc::MethodType::BIDIRECTIONAL) {
		spdlog::warn("Bidirectional streaming not implemented yet");
	}
}

uint32_t Session::createStream(const cyfon_rpc::RpcHeader& header) {
	std::lock_guard<std::mutex> lock(stream_mutex_);

	uint32_t stream_id = next_stream_id_++;

	Stream stream;
	stream.stream_id = stream_id;
	stream.request_id = header.request_id;
	stream.method_type = server_.getMethodType(header.service_id) -> getMethodType(header.method_id);
	stream.service_id = header.service_id;
	stream.method_id = header.method_id;
	stream.is_active = true;

	streams_[stream_id] = stream;
	return stream_id;
}

void Session::sendStreamMessage(uint32_t stream_id, const std::string& message, bool is_end) {
	std::lock_guard<std::mutex> lock(stream_mutex_);

	auto it = streams_.find(stream_id);
	if (it == streams.end()) {
		spdlog::warn("Cannot send message: stream not found {}", stream_id);
		return;
	}

	auto& stream = it -> second;
	stream.sequence_number++;

	cyfon_rpc::Buffer buffer;
	buffer.append(message);

	cyfon_rpc::RpcHeader header;
	header.message_size = sizeof(cyfon_rpc::RpcHeader) + message.size();
	header.service_id = stream.service_id;
	header.method_id = stream.method_id;
	header.request_id = stream.request_id;
	header.stream_id = stream.stream_id;
	header.sequence_number = stream.sequence_number;
	header.message_type = static_cast<uint8_t>(cyfon_rpc::MessageType::STREAM);
	header.flags = is_end ? cyfon_rpc::Flag::STREAM_END : cyfon_rpc::Flag::NONE;
	header.reserved = 0;

	cyfon_rpc::prepend_header(buffer, header);
	do_write(buffer.readableBytesView());

	spdlog::debug("Sent stream message, stream_id={}, sequence_number={}, is_end={}",
		 stream_id, stream.sequence_number, is_end);
}

void Session::closeStream(uint32_t stream_id) {
	std::lock_guard<std::mutex> lock(stream_mutex_);

	auto it = streams_find(stream_id);
	if(it != streams_.end()) {
		spdlog::info("Closed stream, stream_id={}", stream_id);
		streams_.erase(it);
	}
}

