// TestableLogic.h - 可测试的核心逻辑（不依赖MFC）
#pragma once

#include <windows.h>
#include <string>
#include <vector>

// ============ 文件格式常量 ============
#define MYNOTE_MAGIC        "MYNOTE01"
#define MYNOTE_MAGIC_SIZE   8
#define MYNOTE_STUDENTID_SIZE 20
#define MYNOTE_IV_SIZE      16
#define MYNOTE_HASH_SIZE    20
#define MYNOTE_ENCRYPTED_SIZE 32

// ============ 主题相关 ============
enum class TestableTheme
{
    Light = 0,
    Dark = 1
};

struct TestableThemeColors
{
    COLORREF clrEditBg;
    COLORREF clrEditText;
    COLORREF clrLineNumBg;
    COLORREF clrLineNumText;
    COLORREF clrLineNumBorder;
};

// ============ 文件格式相关 ============
enum class TestableFileFormat
{
    Unknown,
    PlainText,
    MyNote
};

// ============ 可测试函数声明 ============
namespace TestableLogic
{
    // -------- 主题相关 --------
    // 获取主题颜色
    TestableThemeColors GetThemeColors(TestableTheme theme);

    // 验证主题颜色是否有效（非零对比度）
    bool ValidateThemeContrast(const TestableThemeColors& colors);

    // -------- 文件格式相关 --------
    // 根据扩展名检测文件格式
    TestableFileFormat DetectFormatByExtension(const std::wstring& filePath);

    // 根据文件头魔数检测格式
    TestableFileFormat DetectFormatByMagic(const BYTE* pData, size_t dataLen);

    // 验证 MyNote 文件头结构
    bool ValidateMyNoteHeader(const BYTE* pData, size_t dataLen);

    // 生成 MyNote 文件头
    std::vector<BYTE> GenerateMyNoteHeader(const char* studentId);

    // -------- 编码转换 --------
    // UTF-8 转 Unicode
    std::wstring UTF8ToUnicode(const char* utf8Str, int len = -1);

    // Unicode 转 UTF-8
    std::string UnicodeToUTF8(const wchar_t* unicodeStr, int len = -1);

    // 检测 BOM 类型 (返回: 0=无BOM, 1=UTF8, 2=UTF16LE, 3=UTF16BE)
    int DetectBOM(const BYTE* pData, size_t dataLen);

    // -------- 行号计算 --------
    // 计算文本行数
    int CountLines(const wchar_t* pText);

    // 计算行号区域需要的宽度（像素）
    int CalculateLineNumberWidth(int lineCount, int charWidth);

    // 获取指定行的起始字符位置
    int GetLineStartPosition(const wchar_t* pText, int lineIndex);

    // 根据字符位置计算行号
    int GetLineFromCharPosition(const wchar_t* pText, int charPos);

    // -------- 加密辅助 --------
    // 计算 SHA1 摘要
    bool ComputeSHA1(const BYTE* pData, DWORD dwDataLen, BYTE* pHash, DWORD dwHashLen);

    // AES-CBC 加密
    bool AESEncrypt(const BYTE* pPlainText, DWORD dwPlainLen,
        const BYTE* pKey, DWORD dwKeyLen,
        const BYTE* pIV,
        BYTE* pCipherText, DWORD& dwCipherLen);

    // AES-CBC 解密
    bool AESDecrypt(const BYTE* pCipherText, DWORD dwCipherLen,
        const BYTE* pKey, DWORD dwKeyLen,
        const BYTE* pIV,
        BYTE* pPlainText, DWORD& dwPlainLen);

    // 生成随机 IV
    bool GenerateRandomIV(BYTE* pIV, DWORD dwLen);

    // 验证完整性（比较两个哈希）
    bool VerifyIntegrity(const BYTE* pHash1, const BYTE* pHash2, size_t hashLen);

    // -------- MyNote 完整流程 --------
    // 创建 MyNote 文件内容（返回完整文件字节）
    std::vector<BYTE> CreateMyNoteContent(const std::string& utf8Content,
        const char* studentId,
        const char* secretKey);

    // 解析 MyNote 文件内容
    struct MyNoteParseResult
    {
        bool success;
        std::string studentId;
        std::string content;
        bool integrityValid;
        std::string errorMessage;
    };

    MyNoteParseResult ParseMyNoteContent(const BYTE* pData, size_t dataLen,
        const char* secretKey);
}
