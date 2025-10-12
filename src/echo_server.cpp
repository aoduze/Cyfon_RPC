#pragma once

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <windows.h> 

#include "buffer.h"
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

        socket_.async_read_some(
            boost::asio::buffer(socketbuffer_.writableBytesView().data(), socketbuffer_.writableBytesView().size()),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    socketbuffer_.hasWritten(length);

                    std::cout << "Received " << length << " bytes: "
                        << socketbuffer_.toStringView() << std::endl;

                    do_write();
                }
                else {
                    std::cerr << "Read error: " << ec.message() << std::endl;
                }
            });
    }

    void do_write() {
        auto self(shared_from_this());

        boost::asio::async_write(socket_,
            boost::asio::buffer(socketbuffer_.readableBytesView().data(), socketbuffer_.readableBytesView().size()),
            [this, self](boost::system::error_code ec, std::size_t bytes_written) {
                if (!ec) {
                    socketbuffer_.retrieve(bytes_written);
                    
                    do_read();
                }
                else {
                    std::cerr << "Write error: " << ec.message() << std::endl;
                }
            });
    }

    boost::asio::ip::tcp::socket socket_;
    cyfon_rpc::Buffer socketbuffer_;
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

        std::cout << "Server starting on port 8888..." << std::endl;
        Server server(io_context, 8888);

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}