// CryptoHelper.h - 加密辅助类
#pragma once

#include <windows.h>
#include <wincrypt.h>
#include <vector>
#include <string>

#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Advapi32.lib")

// ========== 移除硬编码 ==========
// 旧代码（已删除）：
// #define STUDENT_ID      "20250313017Z"
// #define SECRET_KEY      "BIGC_AI_2025_KEY"
// ================================

class CCryptoHelper
{
public:
    // 计算 SHA-1 摘要
    static BOOL ComputeSHA1(const BYTE* pData, DWORD dwDataLen, BYTE* pHash, DWORD dwHashLen)
    {
        if (dwHashLen < 20) return FALSE;

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BOOL bResult = FALSE;

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return FALSE;

        if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        if (CryptHashData(hHash, pData, dwDataLen, 0))
        {
            DWORD dwHashSize = 20;
            if (CryptGetHashParam(hHash, HP_HASHVAL, pHash, &dwHashSize, 0))
            {
                bResult = TRUE;
            }
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }

    // AES-CBC 加密
    static BOOL AESEncrypt(const BYTE* pPlainText, DWORD dwPlainLen,
        const BYTE* pKey, DWORD dwKeyLen,
        const BYTE* pIV,
        BYTE* pCipherText, DWORD& dwCipherLen)
    {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;
        BOOL bResult = FALSE;

        if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return FALSE;

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        if (!CryptHashData(hHash, pKey, dwKeyLen, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        DWORD dwMode = CRYPT_MODE_CBC;
        CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
        CryptSetKeyParam(hKey, KP_IV, (BYTE*)pIV, 0);

        memcpy(pCipherText, pPlainText, dwPlainLen);
        dwCipherLen = dwPlainLen;

        if (CryptEncrypt(hKey, 0, TRUE, 0, pCipherText, &dwCipherLen, dwPlainLen + 16))
        {
            bResult = TRUE;
        }

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }

    // AES-CBC 解密
    static BOOL AESDecrypt(const BYTE* pCipherText, DWORD dwCipherLen,
        const BYTE* pKey, DWORD dwKeyLen,
        const BYTE* pIV,
        BYTE* pPlainText, DWORD& dwPlainLen)
    {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;
        BOOL bResult = FALSE;

        if (!CryptAcquireContext(&hProv, NULL, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return FALSE;

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        if (!CryptHashData(hHash, pKey, dwKeyLen, 0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0, &hKey))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return FALSE;
        }

        DWORD dwMode = CRYPT_MODE_CBC;
        CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&dwMode, 0);
        CryptSetKeyParam(hKey, KP_IV, (BYTE*)pIV, 0);

        memcpy(pPlainText, pCipherText, dwCipherLen);
        dwPlainLen = dwCipherLen;

        if (CryptDecrypt(hKey, 0, TRUE, 0, pPlainText, &dwPlainLen))
        {
            bResult = TRUE;
        }

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }

    // 生成随机 IV
    static BOOL GenerateRandomIV(BYTE* pIV, DWORD dwLen)
    {
        HCRYPTPROV hProv = 0;
        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return FALSE;

        BOOL bResult = CryptGenRandom(hProv, dwLen, pIV);
        CryptReleaseContext(hProv, 0);
        return bResult;
    }
};
