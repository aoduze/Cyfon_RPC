// Cyfon_RPC.cpp: 定义应用程序的入口点。
//

#include "Cyfon_RPC.h"
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

#include "person.pb.h" 

void write_person_to_file(const std::string& file_path) {
	// 创建一个 Person 对象
	tutorial::Person person;

	// 使用setter方法填充数据
	person.set_id(114514);
	person.set_name("我是希特勒");

	// 添加repeated字段数据
	person.add_emails("xihai@example.com");
	person.add_emails("2417206676@qq.com");

	// 打开文件， 并使用二进制模式
	std::fstream output(file_path, std::ios::out | std::ios::trunc | std::ios::binary);

	if (!output) {
		std::cerr << " failed to open " << file_path << std::endl;
		return;
	}

	if (!person.SerializeToOstream(&output)) {
		std::cerr << "Failed to write person." << std::endl;
		return;
	}

	std::cout << " success to write" << file_path << std::endl;
}

void read_person_from_file(const std::string& file_path) {

	// 创建一个新的 Person 对象，用于接收解析后的数据
	tutorial::Person person;

	std::fstream input(file_path, std::ios::in | std::ios::binary);
	if (!input) {
		std::cerr << "failed to open " << file_path << std::endl;
		return;
	}

	// 从输入流解析数据到Person对象
	if (!person.ParseFromIstream(&input)) {
		std::cerr << "failed to parse person from " << file_path << std::endl;
		return;
	}

	std::cout << "\n从文件 " << file_path << " 读取数据成功！" << std::endl;
	std::cout << "--------------------------------------------" << std::endl;

	// 使用getter方法读取数据
	std::cout << "ID: " << person.id() << std::endl;
	std::cout << "Name: " << person.name() << std::endl;

	// 遍历repeated字段
	for (int i = 0; i < person.emails_size(); ++i) {
		std::cout << "Email: " << i + 1 << " : " << person.emails(i) << std::endl;
	}

	std::cout << "---------------------------------------------" << std::endl;
}

int main() {
	// 指定控制台使用UTF-8编码，以正确显示中文字符
	SetConsoleOutputCP(CP_UTF8);

	const std::string data_file = "person_data.bin";

	// 1. 序列化：将对象数据写入文件
	write_person_to_file(data_file);

	// 2. 反序列化：从文件读取数据并恢复对象
	read_person_from_file(data_file);

	google::protobuf::ShutdownProtobufLibrary();

	return 0;
}