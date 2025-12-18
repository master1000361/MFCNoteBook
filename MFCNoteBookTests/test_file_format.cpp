// test_file_format.cpp - 文件格式检测和 MyNote 格式测试
#include "pch.h"

using namespace TestableLogic;

// ============ 扩展名检测测试 ============

TEST(FileFormatTest, DetectByExtension_MyNote)
{
    EXPECT_EQ(DetectFormatByExtension(L"test.mynote"), TestableFileFormat::MyNote);
    EXPECT_EQ(DetectFormatByExtension(L"C:\\folder\\test.MYNOTE"), TestableFileFormat::MyNote);
    EXPECT_EQ(DetectFormatByExtension(L"test.MyNote"), TestableFileFormat::MyNote);
}

TEST(FileFormatTest, DetectByExtension_PlainText)
{
    EXPECT_EQ(DetectFormatByExtension(L"test.txt"), TestableFileFormat::PlainText);
    EXPECT_EQ(DetectFormatByExtension(L"C:\\folder\\test.TXT"), TestableFileFormat::PlainText);
}

TEST(FileFormatTest, DetectByExtension_Unknown)
{
    EXPECT_EQ(DetectFormatByExtension(L"test.doc"), TestableFileFormat::Unknown);
    EXPECT_EQ(DetectFormatByExtension(L"test"), TestableFileFormat::Unknown);
    EXPECT_EQ(DetectFormatByExtension(L""), TestableFileFormat::Unknown);
}

// ============ 魔数检测测试 ============

TEST(FileFormatTest, DetectByMagic_MyNote)
{
    BYTE data[] = "MYNOTE01rest of file...";
    EXPECT_EQ(DetectFormatByMagic(data, sizeof(data)), TestableFileFormat::MyNote);
}

TEST(FileFormatTest, DetectByMagic_PlainText)
{
    BYTE data[] = "Hello, this is plain text";
    EXPECT_EQ(DetectFormatByMagic(data, sizeof(data)), TestableFileFormat::PlainText);
}

TEST(FileFormatTest, DetectByMagic_TooShort)
{
    BYTE data[] = "MYNO";
    EXPECT_EQ(DetectFormatByMagic(data, 4), TestableFileFormat::Unknown);
}

TEST(FileFormatTest, DetectByMagic_NullData)
{
    EXPECT_EQ(DetectFormatByMagic(nullptr, 100), TestableFileFormat::Unknown);
}

// ============ MyNote 文件头测试 ============

TEST(FileFormatTest, ValidateMyNoteHeader_Valid)
{
    std::vector<BYTE> data;
    // Magic
    const char* magic = "MYNOTE01";
    for (int i = 0; i < 8; i++) data.push_back(magic[i]);
    // StudentID (20 bytes)
    for (int i = 0; i < 20; i++) data.push_back('X');
    // Content length (4 bytes)
    for (int i = 0; i < 4; i++) data.push_back(0);
    // IV (16 bytes)
    for (int i = 0; i < 16; i++) data.push_back(0);
    // Encrypted hash (32 bytes)
    for (int i = 0; i < 32; i++) data.push_back(0);

    EXPECT_TRUE(ValidateMyNoteHeader(data.data(), data.size()));
}

TEST(FileFormatTest, ValidateMyNoteHeader_InvalidMagic)
{
    std::vector<BYTE> data(80, 0);
    memcpy(data.data(), "WRONGMAG", 8);

    EXPECT_FALSE(ValidateMyNoteHeader(data.data(), data.size()));
}

TEST(FileFormatTest, ValidateMyNoteHeader_TooSmall)
{
    BYTE data[10] = "MYNOTE01";
    EXPECT_FALSE(ValidateMyNoteHeader(data, 10));
}

TEST(FileFormatTest, GenerateMyNoteHeader_Content)
{
    auto header = GenerateMyNoteHeader("20250313017Z");

    ASSERT_EQ(header.size(), MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE);

    // 验证魔数
    EXPECT_EQ(memcmp(header.data(), "MYNOTE01", 8), 0);

    // 验证学号
    EXPECT_EQ(memcmp(header.data() + 8, "20250313017Z", 12), 0);
}

TEST(FileFormatTest, GenerateMyNoteHeader_NullStudentId)
{
    auto header = GenerateMyNoteHeader(nullptr);

    ASSERT_EQ(header.size(), MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE);
    EXPECT_EQ(memcmp(header.data(), "MYNOTE01", 8), 0);
}

// ============ MyNote 完整流程测试 ============

TEST(FileFormatTest, CreateAndParse_EmptyContent)
{
    const char* studentId = "20250313017Z";
    const char* secretKey = "BIGC_AI_2025_KEY";

    auto data = CreateMyNoteContent("", studentId, secretKey);
    ASSERT_GT(data.size(), 0u);

    auto result = ParseMyNoteContent(data.data(), data.size(), secretKey);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.studentId, studentId);
    EXPECT_TRUE(result.content.empty());
    EXPECT_TRUE(result.integrityValid);
}

TEST(FileFormatTest, CreateAndParse_WithContent)
{
    const char* studentId = "20250313017Z";
    const char* secretKey = "BIGC_AI_2025_KEY";
    std::string content = "Hello, this is a test content!\n中文测试";

    // 转换为 UTF-8
    std::string utf8Content = UnicodeToUTF8(UTF8ToUnicode(content.c_str()).c_str());

    auto data = CreateMyNoteContent(content, studentId, secretKey);
    auto result = ParseMyNoteContent(data.data(), data.size(), secretKey);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.studentId, studentId);
    EXPECT_EQ(result.content, content);
    EXPECT_TRUE(result.integrityValid);
}

TEST(FileFormatTest, Parse_TamperedContent)
{
    const char* studentId = "20250313017Z";
    const char* secretKey = "BIGC_AI_2025_KEY";
    std::string content = "Original content";

    auto data = CreateMyNoteContent(content, studentId, secretKey);

    // 篡改内容（修改内容区的第一个字节）
    size_t contentOffset = MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE + sizeof(UINT32);
    data[contentOffset] = 'X';  // 篡改

    auto result = ParseMyNoteContent(data.data(), data.size(), secretKey);

    EXPECT_TRUE(result.success);  // 解析成功
    EXPECT_FALSE(result.integrityValid);  // 但完整性验证失败
}

TEST(FileFormatTest, Parse_WrongKey)
{
    const char* studentId = "20250313017Z";
    const char* correctKey = "BIGC_AI_2025_KEY";
    const char* wrongKey = "WRONG_KEY_12345!";
    std::string content = "Test content";

    auto data = CreateMyNoteContent(content, studentId, correctKey);
    auto result = ParseMyNoteContent(data.data(), data.size(), wrongKey);

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.integrityValid);  // 密钥不对，完整性验证失败
}
