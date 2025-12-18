// test_integration.cpp - 集成测试（ConfigManager + TestableLogic）
#include "pch.h"

using namespace TestableLogic;

// ============ 测试夹具 ============
class IntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // 确保配置已加载
        CConfigManager& config = CConfigManager::GetInstance();
        ASSERT_TRUE(config.IsConfigValid()) << "配置未正确加载";

        m_strStudentID = config.GetStudentID();
        m_strSecretKey = config.GetSecretKey();
    }

    CString m_strStudentID;
    CString m_strSecretKey;
};

// ============ 配置管理测试 ============

TEST_F(IntegrationTest, ConfigManager_IsValid)
{
    CConfigManager& config = CConfigManager::GetInstance();

    EXPECT_TRUE(config.IsConfigValid());
    EXPECT_FALSE(config.GetStudentID().IsEmpty());
    EXPECT_FALSE(config.GetSecretKey().IsEmpty());
}

TEST_F(IntegrationTest, ConfigManager_StudentIDFormat)
{
    EXPECT_LE(m_strStudentID.GetLength(), 20)
        << "学号长度不应超过20字符";
    EXPECT_GT(m_strStudentID.GetLength(), 0)
        << "学号不能为空";
}

TEST_F(IntegrationTest, ConfigManager_SecretKeyLength)
{
    EXPECT_GE(m_strSecretKey.GetLength(), 8)
        << "密钥长度应至少8字符";
}

// ============ 完整流程测试（使用配置） ============

TEST_F(IntegrationTest, CreateAndParse_WithConfig)
{
    std::string content = "Test content with config";

    // 转换为 ANSI
    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    // 创建文件
    auto data = CreateMyNoteContent(content, asciiStudentID, asciiSecretKey);
    ASSERT_GT(data.size(), 0u);

    // 解析文件
    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.studentId, std::string(asciiStudentID));
    EXPECT_EQ(result.content, content);
    EXPECT_TRUE(result.integrityValid);
}

TEST_F(IntegrationTest, CreateAndParse_ChineseContent)
{
    std::string content = "中文测试内容\n换行测试";

    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    auto data = CreateMyNoteContent(content, asciiStudentID, asciiSecretKey);
    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.integrityValid);
    EXPECT_EQ(result.content, content);
}

TEST_F(IntegrationTest, Parse_StudentIDMismatch)
{
    std::string content = "Test content";
    const char* wrongStudentID = "WRONG_STUDENT_ID";

    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    // 使用错误的学号创建文件
    auto data = CreateMyNoteContent(content, wrongStudentID, asciiSecretKey);

    // 解析时应该成功，但学号不匹配
    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_NE(result.studentId, std::string(CT2A(m_strStudentID, CP_UTF8)));
    EXPECT_TRUE(result.integrityValid);  // 完整性仍然有效
}

TEST_F(IntegrationTest, Parse_TamperedContent_WithConfig)
{
    std::string content = "Original content";

    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    auto data = CreateMyNoteContent(content, asciiStudentID, asciiSecretKey);

    // 篡改内容
    size_t contentOffset = MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE + sizeof(UINT32);
    if (contentOffset < data.size())
    {
        data[contentOffset] ^= 0xFF;  // 翻转一个字节
    }

    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.integrityValid) << "篡改后的文件应该验证失败";
}

TEST_F(IntegrationTest, Parse_TamperedHash)
{
    std::string content = "Test content";

    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    auto data = CreateMyNoteContent(content, asciiStudentID, asciiSecretKey);

    // 篡改加密的哈希值（文件末尾）
    if (data.size() >= MYNOTE_ENCRYPTED_SIZE)
    {
        data[data.size() - 1] ^= 0xFF;
    }

    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.integrityValid) << "篡改哈希后应该验证失败";
}

// ============ 边界条件测试 ============

TEST_F(IntegrationTest, CreateAndParse_EmptyContent)
{
    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    auto data = CreateMyNoteContent("", asciiStudentID, asciiSecretKey);
    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.content.empty());
    EXPECT_TRUE(result.integrityValid);
}

TEST_F(IntegrationTest, CreateAndParse_LargeContent)
{
    // 创建一个较大的内容（1MB）
    std::string content(1024 * 1024, 'A');

    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    auto data = CreateMyNoteContent(content, asciiStudentID, asciiSecretKey);
    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.content.size(), content.size());
    EXPECT_TRUE(result.integrityValid);
}

TEST_F(IntegrationTest, CreateAndParse_SpecialCharacters)
{
    std::string content = "Special chars: \r\n\t\0\x01\xFF";

    CT2A asciiStudentID(m_strStudentID, CP_UTF8);
    CT2A asciiSecretKey(m_strSecretKey, CP_UTF8);

    auto data = CreateMyNoteContent(content, asciiStudentID, asciiSecretKey);
    auto result = ParseMyNoteContent(data.data(), data.size(), asciiSecretKey);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.integrityValid);
}
