#include <iostream>
#include <cassert>
#include <string_view> // 确保包含了 string_view
#include "buffer.h"

// 为了方便，我们使用 cyfon_rpc 命名空间
using namespace cyfon_rpc;

// 测试函式宣告
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

// 測試1：檢查初始狀態是否正確
void testInitialState() {
    std::cout << "--- Running testInitialState ---" << std::endl;
    Buffer buf;
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);
    std::cout << "testInitialState PASSED" << std::endl;
}

// 測試2：測試基本的 append 和 retrieve 功能
void testAppendAndRetrieve() {
    std::cout << "--- Running testAppendAndRetrieve ---" << std::endl;
    Buffer buf;
    std::string_view str = "hello world";
    // <--- 优化: 直接使用 append(string_view)
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

// 測試3：測試緩衝區是否會自動增長
void testBufferGrowth() {
    std::cout << "--- Running testBufferGrowth ---" << std::endl;
    Buffer buf;
    std::string long_str(1200, 'x');
    // <--- 优化: 直接使用 append(string)
    buf.append(long_str);

    assert(buf.readableBytes() == 1200);
    assert(buf.writableBytes() > 0);
    assert(buf.toStringView() == long_str);
    std::cout << "testBufferGrowth PASSED" << std::endl;
}

// 測試4：測試內部空間的回收利用
void testBufferRecycle() {
    std::cout << "--- Running testBufferRecycle ---" << std::endl;
    Buffer buf;
    std::string str(200, 'x');
    // <--- 优化
    buf.append(str);

    buf.retrieve(100);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend + 100);

    std::string str2(1000, 'y');
    // <--- 优化
    buf.append(str2);

    assert(buf.readableBytes() == 100 + 1000);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    std::string expected_str = std::string(100, 'x') + std::string(1000, 'y');
    assert(buf.retrieveAllAsString() == expected_str);

    std::cout << "testBufferRecycle PASSED" << std::endl;
}

// 測試5：測試整數的讀寫 (无需改动)
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

// 測試6：測試 prepend 功能
void testPrepend() {
    std::cout << "--- Running testPrepend ---" << std::endl;
    Buffer buf;
    std::string_view content = "data";
    // <--- 优化
    buf.append(content);

    int32_t header = 4;
    buf.prependInt(header);

    assert(buf.readableBytes() == sizeof(header) + content.size());
    assert(buf.prependableBytes() == Buffer::kCheapPrepend - sizeof(header));
    assert(buf.readInt<int32_t>() == header);
    assert(buf.retrieveAllAsString() == content);

    std::cout << "testPrepend PASSED" << std::endl;
}

// 測試7：測試尋找 CRLF
void testFindCRLF() {
    std::cout << "--- Running testFindCRLF ---" << std::endl;
    Buffer buf;
    std::string_view data = "hello\r\nworld";
    // <--- 优化
    buf.append(data);

    const char* crlf = buf.findCRLF();
    assert(crlf != nullptr);
    assert(std::string_view(buf.peek(), crlf - buf.peek()) == "hello");

    buf.retrieveAll();
    // <--- 优化
    buf.append("no crlf");
    assert(buf.findCRLF() == nullptr);
    std::cout << "testFindCRLF PASSED" << std::endl;
}

// 測試8：測試 shrink 功能
void testShrink() {
    std::cout << "--- Running testShrink ---" << std::endl;
    Buffer buf(20);
    std::string data(10, 'z');
    // <--- 优化
    buf.append(data);
    buf.retrieve(5);

    buf.shrink();

    assert(buf.prependableBytes() == 0);
    assert(buf.readableBytes() == 5);
    assert(buf.internalCapacity() == 5);
    assert(buf.toStringView() == "zzzzz");
    std::cout << "testShrink PASSED" << std::endl;
}