#include <iostream>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <windows.h> // For SetConsoleOutputCP

int main() {
    try {
        // ���ÿ���̨Ϊ UTF-8 ����
        SetConsoleOutputCP(CP_UTF8);

        boost::asio::io_context io_context;

        // ������������ַ�Ͷ˿�
        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("localhost", "8888"); // "lo.calhost" -> "localhost"

        // ���������� socket
        boost::asio::ip::tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        std::cout << "Successfully connected to server." << std::endl;
        std::cout << "Enter a message to send (or type 'exit' to quit):" << std::endl;

        // ʹ��ѭ�����������ͺͽ�����Ϣ
        for (;;) {
            std::string message;
            std::getline(std::cin, message);

            if (message.empty() || message == "exit") {
                break;
            }

            boost::asio::write(socket, boost::asio::buffer(message));
            std::cout << "Sent: " << message << std::endl;

            // 2. ���շ������Ļ���
            std::vector<char> response_buffer(1024);
            boost::system::error_code error;
            size_t len = socket.read_some(boost::asio::buffer(response_buffer), error);

            if (error == boost::asio::error::eof) {
                // ���ӱ��Է��ر�
                std::cout << "Connection closed by server." << std::endl;
                break;
            }
            else if (error) {
                throw boost::system::system_error(error); // ��������
            }

            // 3. ���յ���ԭʼ����תΪ�ַ�������ӡ
            // ���ǲ��ٳ����� Protobuf �����л�
            std::string received_message(response_buffer.data(), len);
            std::cout << "Received: " << received_message << std::endl << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    // ��������ı��ͻ����У����ǲ�����Ҫ Protobuf ��
    // google::protobuf::ShutdownProtobufLibrary();

    return 0;
}