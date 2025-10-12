#include <iostream>
#include <cassert>
#include "buffer.h"

// 為了方便，我們使用 cyfon_rpc 命名空間
using namespace cyfon_rpc;

// 測試函式宣告
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
    buf.append({ reinterpret_cast<const std::byte*>(str.data()), str.size() });

    assert(buf.readableBytes() == str.size());
    assert(buf.writableBytes() == Buffer::kInitialSize - str.size());
    assert(buf.toStringView() == str);

    std::string retrieved_str = buf.retrieveAsString(5);
    assert(retrieved_str == "hello");
    assert(buf.readableBytes() == str.size() - 5);
    assert(buf.toStringView() == " world");

    buf.retrieveAll();
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == Buffer::kInitialSize); // retrieveAll 應該重置指標
    assert(buf.prependableBytes() == Buffer::kCheapPrepend);

    std::cout << "testAppendAndRetrieve PASSED" << std::endl;
}

// 測試3：測試緩衝區是否會自動增長
void testBufferGrowth() {
    std::cout << "--- Running testBufferGrowth ---" << std::endl;
    Buffer buf;
    std::string long_str(1200, 'x');
    buf.append({ reinterpret_cast<const std::byte*>(long_str.data()), long_str.size() });

    assert(buf.readableBytes() == 1200);
    // 檢查 writableBytes 是否大於 0，表示已經擴容
    assert(buf.writableBytes() > 0);
    assert(buf.toStringView() == long_str);
    std::cout << "testBufferGrowth PASSED" << std::endl;
}

// 測試4：測試內部空間的回收利用 (makeSpace 的 else 分支)
void testBufferRecycle() {
    std::cout << "--- Running testBufferRecycle ---" << std::endl;
    Buffer buf;
    std::string str(200, 'x');
    buf.append({ reinterpret_cast<const std::byte*>(str.data()), str.size() });

    buf.retrieve(100); // 釋放前面 100 位元組
    assert(buf.prependableBytes() == Buffer::kCheapPrepend + 100);

    std::string str2(1000, 'y');
    buf.append({ reinterpret_cast<const std::byte*>(str2.data()), str2.size() });

    assert(buf.readableBytes() == 100 + 1000);
    assert(buf.prependableBytes() == Buffer::kCheapPrepend); // 空間被回收，指標回到初始位置

    std::string expected_str = std::string(100, 'x') + std::string(1000, 'y');
    assert(buf.retrieveAllAsString() == expected_str);

    std::cout << "testBufferRecycle PASSED" << std::endl;
}

// 測試5：測試整數的讀寫
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
    buf.append({ reinterpret_cast<const std::byte*>(content.data()), content.size() });

    int32_t header = 4; // 假設是長度標頭
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
    buf.append({ reinterpret_cast<const std::byte*>(data.data()), data.size() });

    const char* crlf = buf.findCRLF();
    assert(crlf != nullptr);
    assert(std::string_view(buf.peek(), crlf - buf.peek()) == "hello");

    buf.retrieveAll();
    buf.append({ reinterpret_cast<const std::byte*>("no crlf"), 7 });
    assert(buf.findCRLF() == nullptr);
    std::cout << "testFindCRLF PASSED" << std::endl;
}

// 測試8：測試 shrink 功能
void testShrink() {
    std::cout << "--- Running testShrink ---" << std::endl;
    Buffer buf(20); // 使用小容量方便測試
    std::string data(10, 'z');
    buf.append({ reinterpret_cast<const std::byte*>(data.data()), data.size() });
    buf.retrieve(5);

    buf.shrink();

    // shrink 後，prependable 空間應該消失
    assert(buf.prependableBytes() == 0);
    assert(buf.readableBytes() == 5);
    // 內部容量應該也縮小了
    assert(buf.internalCapacity() == 5);
    assert(buf.toStringView() == "zzzzz");
    std::cout << "testShrink PASSED" << std::endl;
}