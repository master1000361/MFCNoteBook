// test_crypto.cpp - SHA1/AES 加解密测试
#include "pch.h"

using namespace TestableLogic;

// ============ SHA1 测试 ============

TEST(CryptoTest, SHA1_EmptyString)
{
    BYTE hash[20] = { 0 };
    bool result = ComputeSHA1((const BYTE*)"", 0, hash, 20);

    ASSERT_TRUE(result);

    // 空字符串的 SHA1: da39a3ee5e6b4b0d3255bfef95601890afd80709
    EXPECT_EQ(hash[0], 0xda);
    EXPECT_EQ(hash[1], 0x39);
    EXPECT_EQ(hash[19], 0x09);
}

TEST(CryptoTest, SHA1_HelloWorld)
{
    const char* text = "Hello, World!";
    BYTE hash[20] = { 0 };

    bool result = ComputeSHA1((const BYTE*)text, (DWORD)strlen(text), hash, 20);

    ASSERT_TRUE(result);
    // 验证哈希非全零
    bool allZero = true;
    for (int i = 0; i < 20; i++)
    {
        if (hash[i] != 0) allZero = false;
    }
    EXPECT_FALSE(allZero);
}

TEST(CryptoTest, SHA1_BufferTooSmall)
{
    BYTE hash[10] = { 0 };  // 太小
    bool result = ComputeSHA1((const BYTE*)"test", 4, hash, 10);

    EXPECT_FALSE(result);
}

TEST(CryptoTest, SHA1_Consistency)
{
    // 相同输入应产生相同哈希
    const char* text = "Test consistency";
    BYTE hash1[20] = { 0 };
    BYTE hash2[20] = { 0 };

    ComputeSHA1((const BYTE*)text, (DWORD)strlen(text), hash1, 20);
    ComputeSHA1((const BYTE*)text, (DWORD)strlen(text), hash2, 20);

    EXPECT_EQ(memcmp(hash1, hash2, 20), 0);
}

// ============ AES 加解密测试 ============

TEST(CryptoTest, AES_EncryptDecrypt_Basic)
{
    const char* plainText = "Hello, AES!";
    const char* key = "BIGC_AI_2025_KEY";
    BYTE iv[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };

    BYTE encrypted[64] = { 0 };
    DWORD encryptedLen = 64;

    bool encResult = AESEncrypt(
        (const BYTE*)plainText, (DWORD)strlen(plainText),
        (const BYTE*)key, (DWORD)strlen(key),
        iv, encrypted, encryptedLen);

    ASSERT_TRUE(encResult);
    EXPECT_GT(encryptedLen, 0u);

    // 解密
    BYTE decrypted[64] = { 0 };
    DWORD decryptedLen = encryptedLen;

    bool decResult = AESDecrypt(
        encrypted, encryptedLen,
        (const BYTE*)key, (DWORD)strlen(key),
        iv, decrypted, decryptedLen);

    ASSERT_TRUE(decResult);
    EXPECT_EQ(decryptedLen, strlen(plainText));
    EXPECT_EQ(memcmp(decrypted, plainText, decryptedLen), 0);
}

TEST(CryptoTest, AES_DifferentIV_DifferentResult)
{
    const char* plainText = "Same text";
    const char* key = "BIGC_AI_2025_KEY";

    BYTE iv1[16] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
    BYTE iv2[16] = { 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 };

    BYTE encrypted1[64] = { 0 };
    BYTE encrypted2[64] = { 0 };
    DWORD len1 = 64, len2 = 64;

    AESEncrypt((const BYTE*)plainText, (DWORD)strlen(plainText),
        (const BYTE*)key, (DWORD)strlen(key), iv1, encrypted1, len1);
    AESEncrypt((const BYTE*)plainText, (DWORD)strlen(plainText),
        (const BYTE*)key, (DWORD)strlen(key), iv2, encrypted2, len2);

    // 不同 IV 应产生不同密文
    EXPECT_NE(memcmp(encrypted1, encrypted2, len1), 0);
}

TEST(CryptoTest, GenerateRandomIV_NotAllZero)
{
    BYTE iv[16] = { 0 };
    bool result = GenerateRandomIV(iv, 16);

    ASSERT_TRUE(result);

    bool allZero = true;
    for (int i = 0; i < 16; i++)
    {
        if (iv[i] != 0) allZero = false;
    }
    EXPECT_FALSE(allZero);
}

TEST(CryptoTest, GenerateRandomIV_Unique)
{
    BYTE iv1[16] = { 0 };
    BYTE iv2[16] = { 0 };

    GenerateRandomIV(iv1, 16);
    GenerateRandomIV(iv2, 16);

    // 两次生成应该不同
    EXPECT_NE(memcmp(iv1, iv2, 16), 0);
}

TEST(CryptoTest, VerifyIntegrity_Match)
{
    BYTE hash1[20] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };
    BYTE hash2[20] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };

    EXPECT_TRUE(VerifyIntegrity(hash1, hash2, 20));
}

TEST(CryptoTest, VerifyIntegrity_Mismatch)
{
    BYTE hash1[20] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20 };
    BYTE hash2[20] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,99 };

    EXPECT_FALSE(VerifyIntegrity(hash1, hash2, 20));
}
