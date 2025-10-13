#include <iostream>
#include <cassert>
#include <string_view> // ȷ�������� string_view
#include "buffer.h"

// Ϊ�˷��㣬����ʹ�� cyfon_rpc �����ռ�
using namespace cyfon_rpc;

// ���Ժ�ʽ����
void testInitialState();
void testAppendAndRetrieve();
void testBufferGrowth();
void testBufferRecycle();
void testIntegerOperations();
void testPrepend();
void testFindCRLF();
void testShrink();

int main() {
    std::cout << "Starting Buffer tests..." << std::endl;

    testInitialState();
    testAppendAndRetrieve();
    testBufferGrowth();
    testBufferRecycle();
    testIntegerOperations();
    testPrepend();
    testFindCRLF();
    testShrink();

    std::cout << "\nAll Buffer tests passed successfully!" << std::endl;

    return 0;
}

// �yԇ1���z���ʼ��B�Ƿ����_
void testInitialState() {
    std::cout << "--- Running testInitialState ---" << std::endl;
    Buffer buf;
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);
    std::cout << "testInitialState PASSED" << std::endl;
}

// �yԇ2���yԇ������ append �� retrieve ����
void testAppendAndRetrieve() {
    std::cout << "--- Running testAppendAndRetrieve ---" << std::endl;
    Buffer buf;
    std::string_view str = "hello world";
    // <--- �Ż�: ֱ��ʹ�� append(string_view)
    buf.append(str);

    assert(buf.readableBytes() == str.size());
    assert(buf.writableBytes() == Buffer::kInitialSize - str.size());
    assert(buf.toStringView() == str);

    std::string retrieved_str = buf.retrieveAsString(5);
    assert(retrieved_str == "hello");
    assert(buf.readableBytes() == str.size() - 5);
    assert(buf.toStringView() == " world");

    buf.retrieveAll();
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    std::cout << "testAppendAndRetrieve PASSED" << std::endl;
}

// �yԇ3���yԇ���n�^�Ƿ���Ԅ����L
void testBufferGrowth() {
    std::cout << "--- Running testBufferGrowth ---" << std::endl;
    Buffer buf;
    std::string long_str(1200, 'x');
    // <--- �Ż�: ֱ��ʹ�� append(string)
    buf.append(long_str);

    assert(buf.readableBytes() == 1200);
    assert(buf.writableBytes() > 0);
    assert(buf.toStringView() == long_str);
    std::cout << "testBufferGrowth PASSED" << std::endl;
}

// �yԇ4���yԇ�Ȳ����g�Ļ�������
void testBufferRecycle() {
    std::cout << "--- Running testBufferRecycle ---" << std::endl;
    Buffer buf;
    std::string str(200, 'x');
    // <--- �Ż�
    buf.append(str);

    buf.retrieve(100);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend + 100);

    std::string str2(1000, 'y');
    // <--- �Ż�
    buf.append(str2);

    assert(buf.readableBytes() == 100 + 1000);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    std::string expected_str = std::string(100, 'x') + std::string(1000, 'y');
    assert(buf.retrieveAllAsString() == expected_str);

    std::cout << "testBufferRecycle PASSED" << std::endl;
}

// �yԇ5���yԇ�������x�� (����Ķ�)
void testIntegerOperations() {
    std::cout << "--- Running testIntegerOperations ---" << std::endl;
    Buffer buf;
    int64_t val64 = 0x123456789ABCDEF0;
    int32_t val32 = 12345;

    buf.appendInt(val64);
    buf.appendInt(val32);

    assert(buf.readableBytes() == sizeof(val64) + sizeof(val32));
    assert(buf.peekInt<int64_t>() == val64);
    assert(buf.readInt<int64_t>() == val64);
    assert(buf.readInt<int32_t>() == val32);
    assert(buf.readableBytes() == 0);

    std::cout << "testIntegerOperations PASSED" << std::endl;
}

// �yԇ6���yԇ prepend ����
void testPrepend() {
    std::cout << "--- Running testPrepend ---" << std::endl;
    Buffer buf;
    std::string_view content = "data";
    // <--- �Ż�
    buf.append(content);

    int32_t header = 4;
    buf.prependInt(header);

    assert(buf.readableBytes() == sizeof(header) + content.size());
    assert(buf.prependableBytes() == Buffer::kCheapPrepend - sizeof(header));
    assert(buf.readInt<int32_t>() == header);
    assert(buf.retrieveAllAsString() == content);

    std::cout << "testPrepend PASSED" << std::endl;
}

// �yԇ7���yԇ���� CRLF
void testFindCRLF() {
    std::cout << "--- Running testFindCRLF ---" << std::endl;
    Buffer buf;
    std::string_view data = "hello\r\nworld";
    // <--- �Ż�
    buf.append(data);

    const char* crlf = buf.findCRLF();
    assert(crlf != nullptr);
    assert(std::string_view(buf.peek(), crlf - buf.peek()) == "hello");

    buf.retrieveAll();
    // <--- �Ż�
    buf.append("no crlf");
    assert(buf.findCRLF() == nullptr);
    std::cout << "testFindCRLF PASSED" << std::endl;
}

// �yԇ8���yԇ shrink ����
void testShrink() {
    std::cout << "--- Running testShrink ---" << std::endl;
    Buffer buf(20);
    std::string data(10, 'z');
    // <--- �Ż�
    buf.append(data);
    buf.retrieve(5);

    buf.shrink();

    assert(buf.prependableBytes() == 0);
    assert(buf.readableBytes() == 5);
    assert(buf.internalCapacity() == 5);
    assert(buf.toStringView() == "zzzzz");
    std::cout << "testShrink PASSED" << std::endl;
}