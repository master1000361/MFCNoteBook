// ConfigManager.h - 配置文件管理类
#pragma once

#include <afxwin.h>
#include <atlconv.h>
#include <string>
#include <map>
#include <functional>

class CConfigManager
{
public:
    // 错误回调函数类型
    typedef std::function<void(const CString&)> ErrorCallback;

    // 单例模式
    static CConfigManager& GetInstance()
    {
        static CConfigManager instance;
        return instance;
    }

    // 加载配置文件
    BOOL LoadConfig(LPCTSTR lpszConfigPath = nullptr);

    // 获取配置值
    CString GetStudentID() const { return m_strStudentID; }
    CString GetSecretKey() const { return m_strSecretKey; }
    CString GetLastError() const { return m_strLastError; }

    // 测试用的 Set 方法
    void SetStudentID(const CString& strStudentID)
    {
        m_strStudentID = strStudentID;
    }

    void SetSecretKey(const CString& strSecretKey)
    {
        m_strSecretKey = strSecretKey;
    }

    // 设置错误回调（用于测试）
    void SetErrorCallback(ErrorCallback callback)
    {
        m_pfnErrorCallback = callback;
    }

    // 检查配置是否有效
    BOOL IsConfigValid() const
    {
        return !m_strStudentID.IsEmpty() && !m_strSecretKey.IsEmpty();
    }

    // 默认配置文件名
    static const LPCTSTR DEFAULT_CONFIG_FILE;

private:
    CConfigManager() : m_pfnErrorCallback(nullptr) {}
    ~CConfigManager() {}

    // 禁止拷贝
    CConfigManager(const CConfigManager&) = delete;
    CConfigManager& operator=(const CConfigManager&) = delete;

    // 读取 INI 值
    CString ReadINIValue(const CString& strSection, const CString& strKey,
        const CString& strDefault, const CString& strFilePath);

    // 报告错误
    void ReportError(const CString& strError);

    // 验证学号格式
    BOOL ValidateStudentID();

    // 配置数据
    CString m_strStudentID;
    CString m_strSecretKey;
    CString m_strLastError;

    // 错误回调
    ErrorCallback m_pfnErrorCallback;
};
