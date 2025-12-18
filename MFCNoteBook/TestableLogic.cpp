// TestableLogic.cpp - 可测试逻辑实现
#include "pch.h"
#include "TestableLogic.h"
#include <wincrypt.h>
#include <algorithm>
#include <cstring>

#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Advapi32.lib")

namespace TestableLogic
{
    // ============ 主题相关实现 ============

    TestableThemeColors GetThemeColors(TestableTheme theme)
    {
        TestableThemeColors colors = {};

        if (theme == TestableTheme::Dark)
        {
            colors.clrEditBg = RGB(30, 30, 30);
            colors.clrEditText = RGB(220, 220, 220);
            colors.clrLineNumBg = RGB(45, 45, 45);
            colors.clrLineNumText = RGB(140, 140, 140);
            colors.clrLineNumBorder = RGB(60, 60, 60);
        }
        else // Light
        {
            colors.clrEditBg = RGB(255, 255, 255);
            colors.clrEditText = RGB(0, 0, 0);
            colors.clrLineNumBg = RGB(240, 240, 240);
            colors.clrLineNumText = RGB(128, 128, 128);
            colors.clrLineNumBorder = RGB(200, 200, 200);
        }

        return colors;
    }

    bool ValidateThemeContrast(const TestableThemeColors& colors)
    {
        // 计算亮度: Y = 0.299*R + 0.587*G + 0.114*B
        auto getLuminance = [](COLORREF c) -> double {
            return 0.299 * GetRValue(c) + 0.587 * GetGValue(c) + 0.114 * GetBValue(c);
            };

        double bgLum = getLuminance(colors.clrEditBg);
        double textLum = getLuminance(colors.clrEditText);

        // 对比度至少要有 50 的差值
        return std::abs(bgLum - textLum) >= 50.0;
    }

    // ============ 文件格式相关实现 ============

    TestableFileFormat DetectFormatByExtension(const std::wstring& filePath)
    {
        if (filePath.length() < 4)
            return TestableFileFormat::Unknown;

        std::wstring lower = filePath;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

        if (lower.length() >= 7 && lower.substr(lower.length() - 7) == L".mynote")
            return TestableFileFormat::MyNote;

        if (lower.length() >= 4 && lower.substr(lower.length() - 4) == L".txt")
            return TestableFileFormat::PlainText;

        return TestableFileFormat::Unknown;
    }

    TestableFileFormat DetectFormatByMagic(const BYTE* pData, size_t dataLen)
    {
        if (pData == nullptr || dataLen < MYNOTE_MAGIC_SIZE)
            return TestableFileFormat::Unknown;

        if (memcmp(pData, MYNOTE_MAGIC, MYNOTE_MAGIC_SIZE) == 0)
            return TestableFileFormat::MyNote;

        return TestableFileFormat::PlainText;
    }

    bool ValidateMyNoteHeader(const BYTE* pData, size_t dataLen)
    {
        // 最小文件大小
        size_t minSize = MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE +
            sizeof(UINT32) + MYNOTE_IV_SIZE + MYNOTE_ENCRYPTED_SIZE;

        if (pData == nullptr || dataLen < minSize)
            return false;

        // 验证魔数
        if (memcmp(pData, MYNOTE_MAGIC, MYNOTE_MAGIC_SIZE) != 0)
            return false;

        return true;
    }

    std::vector<BYTE> GenerateMyNoteHeader(const char* studentId)
    {
        std::vector<BYTE> header;
        header.reserve(MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE);

        // 写入魔数
        for (int i = 0; i < MYNOTE_MAGIC_SIZE; i++)
            header.push_back(MYNOTE_MAGIC[i]);

        // 写入学号（固定20字节，不足补0）
        char sid[MYNOTE_STUDENTID_SIZE] = { 0 };
        if (studentId)
            strncpy_s(sid, studentId, _TRUNCATE);

        for (int i = 0; i < MYNOTE_STUDENTID_SIZE; i++)
            header.push_back(sid[i]);

        return header;
    }

    // ============ 编码转换实现 ============

    std::wstring UTF8ToUnicode(const char* utf8Str, int len)
    {
        if (utf8Str == nullptr)
            return L"";

        int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, len, nullptr, 0);
        if (wideLen <= 0)
            return L"";

        std::wstring result(wideLen, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8Str, len, &result[0], wideLen);

        // 如果 len == -1，结果包含 null terminator，需要去掉
        if (len == -1 && !result.empty() && result.back() == L'\0')
            result.pop_back();

        return result;
    }

    std::string UnicodeToUTF8(const wchar_t* unicodeStr, int len)
    {
        if (unicodeStr == nullptr)
            return "";

        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, unicodeStr, len, nullptr, 0, nullptr, nullptr);
        if (utf8Len <= 0)
            return "";

        std::string result(utf8Len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, unicodeStr, len, &result[0], utf8Len, nullptr, nullptr);

        // 如果 len == -1，结果包含 null terminator，需要去掉
        if (len == -1 && !result.empty() && result.back() == '\0')
            result.pop_back();

        return result;
    }

    int DetectBOM(const BYTE* pData, size_t dataLen)
    {
        if (pData == nullptr || dataLen < 2)
            return 0;

        // UTF-8 BOM: EF BB BF
        if (dataLen >= 3 && pData[0] == 0xEF && pData[1] == 0xBB && pData[2] == 0xBF)
            return 1;

        // UTF-16 LE BOM: FF FE
        if (pData[0] == 0xFF && pData[1] == 0xFE)
            return 2;

        // UTF-16 BE BOM: FE FF
        if (pData[0] == 0xFE && pData[1] == 0xFF)
            return 3;

        return 0;
    }

    // ============ 行号计算实现 ============

    int CountLines(const wchar_t* pText)
    {
        if (pText == nullptr || *pText == L'\0')
            return 1;  // 空文本至少1行

        int count = 1;
        const wchar_t* p = pText;

        while (*p)
        {
            if (*p == L'\n')
                count++;
            p++;
        }

        return count;
    }

    int CalculateLineNumberWidth(int lineCount, int charWidth)
    {
        if (lineCount <= 0)
            lineCount = 1;
        if (charWidth <= 0)
            charWidth = 8;

        // 计算需要的数字位数
        int digits = 1;
        int temp = lineCount;
        while (temp >= 10)
        {
            digits++;
            temp /= 10;
        }

        // 最少显示3位，加上左右边距
        digits = (std::max)(digits, 3);

        return (digits + 2) * charWidth;  // +2 是左右各一个字符的边距
    }

    int GetLineStartPosition(const wchar_t* pText, int lineIndex)
    {
        if (pText == nullptr || lineIndex < 0)
            return 0;

        if (lineIndex == 0)
            return 0;

        int currentLine = 0;
        int pos = 0;

        while (pText[pos])
        {
            if (pText[pos] == L'\n')
            {
                currentLine++;
                if (currentLine == lineIndex)
                    return pos + 1;
            }
            pos++;
        }

        // 超出范围，返回末尾
        return pos;
    }

    int GetLineFromCharPosition(const wchar_t* pText, int charPos)
    {
        if (pText == nullptr || charPos < 0)
            return 0;

        int line = 0;
        int pos = 0;

        while (pText[pos] && pos < charPos)
        {
            if (pText[pos] == L'\n')
                line++;
            pos++;
        }

        return line;
    }

    // ============ 加密辅助实现 ============

    bool ComputeSHA1(const BYTE* pData, DWORD dwDataLen, BYTE* pHash, DWORD dwHashLen)
    {
        if (dwHashLen < 20)
            return false;

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        bool bResult = false;

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return false;

        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (CryptHashData(hHash, pData, dwDataLen, 0))
        {
            DWORD dwHashSize = 20;
            if (CryptGetHashParam(hHash, HP_HASHVAL, pHash, &dwHashSize, 0))
            {
                bResult = true;
            }
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }

    bool AESEncrypt(const BYTE* pPlainText, DWORD dwPlainLen,
        const BYTE* pKey, DWORD dwKeyLen,
        const BYTE* pIV,
        BYTE* pCipherText, DWORD& dwCipherLen)
    {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;
        bool bResult = false;

        if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return false;

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptHashData(hHash, pKey, dwKeyLen, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        DWORD dwMode = CRYPT_MODE_CBC;
        CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
        CryptSetKeyParam(hKey, KP_IV, (BYTE*)pIV, 0);

        memcpy(pCipherText, pPlainText, dwPlainLen);
        dwCipherLen = dwPlainLen;

        if (CryptEncrypt(hKey, 0, TRUE, 0, pCipherText, &dwCipherLen, dwPlainLen + 16))
        {
            bResult = true;
        }

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }

    bool AESDecrypt(const BYTE* pCipherText, DWORD dwCipherLen,
        const BYTE* pKey, DWORD dwKeyLen,
        const BYTE* pIV,
        BYTE* pPlainText, DWORD& dwPlainLen)
    {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;
        bool bResult = false;

        if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return false;

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptHashData(hHash, pKey, dwKeyLen, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return false;
        }

        DWORD dwMode = CRYPT_MODE_CBC;
        CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
        CryptSetKeyParam(hKey, KP_IV, (BYTE*)pIV, 0);

        memcpy(pPlainText, pCipherText, dwCipherLen);
        dwPlainLen = dwCipherLen;

        if (CryptDecrypt(hKey, 0, TRUE, 0, pPlainText, &dwPlainLen))
        {
            bResult = true;
        }

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }

    bool GenerateRandomIV(BYTE* pIV, DWORD dwLen)
    {
        HCRYPTPROV hProv = 0;
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return false;

        BOOL bResult = CryptGenRandom(hProv, dwLen, pIV);
        CryptReleaseContext(hProv, 0);
        return bResult != FALSE;
    }

    bool VerifyIntegrity(const BYTE* pHash1, const BYTE* pHash2, size_t hashLen)
    {
        if (pHash1 == nullptr || pHash2 == nullptr)
            return false;
        return memcmp(pHash1, pHash2, hashLen) == 0;
    }

    // ============ MyNote 完整流程实现 ============

    std::vector<BYTE> CreateMyNoteContent(const std::string& utf8Content,
        const char* studentId,
        const char* secretKey)
    {
        std::vector<BYTE> result;

        // 1. 写入文件头（Magic + StudentID）
        std::vector<BYTE> header = GenerateMyNoteHeader(studentId);
        result.insert(result.end(), header.begin(), header.end());

        // 2. 写入内容长度
        UINT32 contentLen = (UINT32)utf8Content.length();
        result.push_back((BYTE)(contentLen & 0xFF));
        result.push_back((BYTE)((contentLen >> 8) & 0xFF));
        result.push_back((BYTE)((contentLen >> 16) & 0xFF));
        result.push_back((BYTE)((contentLen >> 24) & 0xFF));

        // 3. 写入内容
        for (char c : utf8Content)
            result.push_back((BYTE)c);

        // 4. 计算 SHA-1
        BYTE sha1Hash[MYNOTE_HASH_SIZE] = { 0 };
        if (contentLen > 0)
        {
            ComputeSHA1((const BYTE*)utf8Content.data(), contentLen, sha1Hash, MYNOTE_HASH_SIZE);
        }
        else
        {
            ComputeSHA1((const BYTE*)"", 0, sha1Hash, MYNOTE_HASH_SIZE);
        }

        // 5. 生成 IV
        BYTE iv[MYNOTE_IV_SIZE];
        GenerateRandomIV(iv, MYNOTE_IV_SIZE);

        // 6. AES 加密 SHA-1
        BYTE encryptedHash[MYNOTE_ENCRYPTED_SIZE] = { 0 };
        DWORD dwEncryptedLen = MYNOTE_ENCRYPTED_SIZE;
        AESEncrypt(sha1Hash, MYNOTE_HASH_SIZE,
            (const BYTE*)secretKey, (DWORD)strlen(secretKey),
            iv, encryptedHash, dwEncryptedLen);

        // 7. 写入 IV 和加密摘要
        for (int i = 0; i < MYNOTE_IV_SIZE; i++)
            result.push_back(iv[i]);
        for (DWORD i = 0; i < MYNOTE_ENCRYPTED_SIZE; i++)
            result.push_back(encryptedHash[i]);

        return result;
    }

    MyNoteParseResult ParseMyNoteContent(const BYTE* pData, size_t dataLen,
        const char* secretKey)
    {
        MyNoteParseResult result = {};
        result.success = false;
        result.integrityValid = false;

        // 验证文件头
        if (!ValidateMyNoteHeader(pData, dataLen))
        {
            result.errorMessage = "Invalid MyNote header";
            return result;
        }

        size_t offset = MYNOTE_MAGIC_SIZE;

        // 读取学号
        char studentId[MYNOTE_STUDENTID_SIZE + 1] = { 0 };
        memcpy(studentId, pData + offset, MYNOTE_STUDENTID_SIZE);
        result.studentId = studentId;
        offset += MYNOTE_STUDENTID_SIZE;

        // 读取内容长度
        UINT32 contentLen = 0;
        contentLen = pData[offset] | (pData[offset + 1] << 8) |
            (pData[offset + 2] << 16) | (pData[offset + 3] << 24);
        offset += sizeof(UINT32);

        // 验证数据长度
        size_t expectedSize = offset + contentLen + MYNOTE_IV_SIZE + MYNOTE_ENCRYPTED_SIZE;
        if (dataLen < expectedSize)
        {
            result.errorMessage = "File too small";
            return result;
        }

        // 读取内容
        if (contentLen > 0)
        {
            result.content.assign((const char*)(pData + offset), contentLen);
        }
        offset += contentLen;

        // 读取 IV 和加密摘要
        BYTE iv[MYNOTE_IV_SIZE];
        BYTE encryptedHash[MYNOTE_ENCRYPTED_SIZE];
        memcpy(iv, pData + offset, MYNOTE_IV_SIZE);
        offset += MYNOTE_IV_SIZE;
        memcpy(encryptedHash, pData + offset, MYNOTE_ENCRYPTED_SIZE);

        // 计算当前内容的 SHA-1
        BYTE computedHash[MYNOTE_HASH_SIZE] = { 0 };
        if (contentLen > 0)
        {
            ComputeSHA1((const BYTE*)result.content.data(), contentLen,
                computedHash, MYNOTE_HASH_SIZE);
        }
        else
        {
            ComputeSHA1((const BYTE*)"", 0, computedHash, MYNOTE_HASH_SIZE);
        }

        // 解密文件中的摘要
        BYTE decryptedHash[MYNOTE_ENCRYPTED_SIZE] = { 0 };
        DWORD dwDecryptedLen = MYNOTE_ENCRYPTED_SIZE;

        if (AESDecrypt(encryptedHash, MYNOTE_ENCRYPTED_SIZE,
            (const BYTE*)secretKey, (DWORD)strlen(secretKey),
            iv, decryptedHash, dwDecryptedLen))
        {
            result.integrityValid = VerifyIntegrity(computedHash, decryptedHash, MYNOTE_HASH_SIZE);
        }

        result.success = true;
        return result;
    }
}
