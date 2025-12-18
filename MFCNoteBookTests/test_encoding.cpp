// test_encoding.cpp - 编码转换测试
#include "pch.h"

using namespace TestableLogic;

// ============ UTF-8 转 Unicode 测试 ============

TEST(EncodingTest, UTF8ToUnicode_Empty)
{
    EXPECT_EQ(UTF8ToUnicode(""), L"");
    EXPECT_EQ(UTF8ToUnicode(nullptr), L"");
}

TEST(EncodingTest, UTF8ToUnicode_ASCII)
{
    EXPECT_EQ(UTF8ToUnicode("Hello"), L"Hello");
}

TEST(EncodingTest, UTF8ToUnicode_Chinese)
{
    // UTF-8 编码的 "中文"
    const char* utf8 = "\xE4\xB8\xAD\xE6\x96\x87";
    std::wstring result = UTF8ToUnicode(utf8);
    EXPECT_EQ(result.length(), 2u);
    EXPECT_EQ(result[0], L'中');
    EXPECT_EQ(result[1], L'文');
}

TEST(EncodingTest, UTF8ToUnicode_Mixed)
{
    // "Hello中文"
    const char* utf8 = "Hello\xE4\xB8\xAD\xE6\x96\x87";
    std::wstring result = UTF8ToUnicode(utf8);
    EXPECT_EQ(result, L"Hello中文");
}

// ============ Unicode 转 UTF-8 测试 ============

TEST(EncodingTest, UnicodeToUTF8_Empty)
{
    EXPECT_EQ(UnicodeToUTF8(L""), "");
    EXPECT_EQ(UnicodeToUTF8(nullptr), "");
}

TEST(EncodingTest, UnicodeToUTF8_ASCII)
{
    EXPECT_EQ(UnicodeToUTF8(L"Hello"), "Hello");
}

TEST(EncodingTest, UnicodeToUTF8_Chinese)
{
    std::string result = UnicodeToUTF8(L"中文");
    // 每个中文字符在 UTF-8 中占 3 字节
    EXPECT_EQ(result.length(), 6u);
}

// ============ 往返转换测试 ============

TEST(EncodingTest, RoundTrip_ASCII)
{
    const wchar_t* original = L"Hello World!";
    std::string utf8 = UnicodeToUTF8(original);
    std::wstring back = UTF8ToUnicode(utf8.c_str());
    EXPECT_EQ(back, original);
}

TEST(EncodingTest, RoundTrip_Chinese)
{
    const wchar_t* original = L"中文测试内容";
    std::string utf8 = UnicodeToUTF8(original);
    std::wstring back = UTF8ToUnicode(utf8.c_str());
    EXPECT_EQ(back, original);
}

TEST(EncodingTest, RoundTrip_Mixed)
{
    const wchar_t* original = L"Hello世界123测试ABC";
    std::string utf8 = UnicodeToUTF8(original);
    std::wstring back = UTF8ToUnicode(utf8.c_str());
    EXPECT_EQ(back, original);
}

// ============ BOM 检测测试 ============

TEST(EncodingTest, DetectBOM_None)
{
    BYTE data[] = "Hello World";
    EXPECT_EQ(DetectBOM(data, sizeof(data)), 0);
}

TEST(EncodingTest, DetectBOM_UTF8)
{
    BYTE data[] = { 0xEF, 0xBB, 0xBF, 'H', 'i' };
    EXPECT_EQ(DetectBOM(data, sizeof(data)), 1);
}

TEST(EncodingTest, DetectBOM_UTF16LE)
{
    BYTE data[] = { 0xFF, 0xFE, 'H', 0, 'i', 0 };
    EXPECT_EQ(DetectBOM(data, sizeof(data)), 2);
}

TEST(EncodingTest, DetectBOM_UTF16BE)
{
    BYTE data[] = { 0xFE, 0xFF, 0, 'H', 0, 'i' };
    EXPECT_EQ(DetectBOM(data, sizeof(data)), 3);
}

TEST(EncodingTest, DetectBOM_TooShort)
{
    BYTE data[] = { 0xEF };
    EXPECT_EQ(DetectBOM(data, 1), 0);
}

TEST(EncodingTest, DetectBOM_Null)
{
    EXPECT_EQ(DetectBOM(nullptr, 100), 0);
}
