// ConfigManager.cpp - 配置文件管理类实现
#include "pch.h"
#include "ConfigManager.h"
#include <fstream>

const LPCTSTR CConfigManager::DEFAULT_CONFIG_FILE = _T("config.ini");

BOOL CConfigManager::LoadConfig(LPCTSTR lpszConfigPath)
{
    m_strLastError.Empty();  // 清空上次的错误

    CString strConfigPath;

    // 如果未指定路径，使用默认路径
    if (lpszConfigPath == nullptr)
    {
        TCHAR szModulePath[MAX_PATH];
        GetModuleFileName(NULL, szModulePath, MAX_PATH);
        CString strModuleDir = szModulePath;
        int nPos = strModuleDir.ReverseFind(_T('\\'));
        if (nPos > 0)
        {
            strModuleDir = strModuleDir.Left(nPos);
        }
        strConfigPath = strModuleDir + _T("\\") + DEFAULT_CONFIG_FILE;
    }
    else
    {
        strConfigPath = lpszConfigPath;
    }

    // 检查文件是否存在
    if (GetFileAttributes(strConfigPath) == INVALID_FILE_ATTRIBUTES)
    {
        CString strMsg;
        strMsg.Format(_T("配置文件不存在：%s\n\n请从 config.ini.example 复制并修改为 config.ini"),
            strConfigPath.GetString());
        ReportError(strMsg);
        return FALSE;
    }

    // 读取配置
    m_strStudentID = ReadINIValue(_T("User"), _T("StudentID"), _T(""), strConfigPath);
    m_strSecretKey = ReadINIValue(_T("Security"), _T("SecretKey"), _T(""), strConfigPath);

    // 验证配置
    if (m_strStudentID.IsEmpty())
    {
        ReportError(_T("配置错误：StudentID 不能为空"));
        return FALSE;
    }

    if (m_strSecretKey.IsEmpty())
    {
        ReportError(_T("配置错误：SecretKey 不能为空"));
        return FALSE;
    }

    // 验证学号格式
    if (!ValidateStudentID())
    {
        return FALSE;
    }

    return TRUE;
}

CString CConfigManager::ReadINIValue(const CString& strSection, const CString& strKey,
    const CString& strDefault, const CString& strFilePath)
{
    TCHAR szBuffer[1024] = { 0 };
    GetPrivateProfileString(strSection, strKey, strDefault,
        szBuffer, 1024, strFilePath);

    CString strValue = szBuffer;
    strValue.Trim();  // 去除首尾空格
    return strValue;
}

void CConfigManager::ReportError(const CString& strError)
{
    m_strLastError = strError;

    if (m_pfnErrorCallback)
    {
        m_pfnErrorCallback(strError);  // 使用回调
    }
    else
    {
        AfxMessageBox(strError, MB_ICONERROR);  // 正常运行时弹窗
    }
}

BOOL CConfigManager::ValidateStudentID()
{
    // 验证长度
    if (m_strStudentID.GetLength() < 5)
    {
        ReportError(_T("配置错误：StudentID 长度不能少于5字符"));
        return FALSE;
    }

    if (m_strStudentID.GetLength() > 20)
    {
        ReportError(_T("配置错误：StudentID 长度不能超过20字符"));
        return FALSE;
    }

    // 验证字符（只允许字母和数字）
    for (int i = 0; i < m_strStudentID.GetLength(); i++)
    {
        TCHAR ch = m_strStudentID.GetAt(i);
        if (!_istalnum(ch))
        {
            ReportError(_T("配置错误：StudentID 只能包含字母和数字"));
            return FALSE;
        }
    }

    return TRUE;
}
