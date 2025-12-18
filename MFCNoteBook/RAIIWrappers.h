// RAIIWrappers.h - RAII包装器用于MFC文件操作
// 符合F-06需求：使用RAII包装CFile, CArchive，捕获异常弹出对话框

#pragma once
#include <memory>
#include <functional>
#include <stdexcept>

// ============================================================
// 文件操作异常类
// ============================================================
class CFileOperationException : public std::runtime_error
{
public:
    enum class Operation { Open, Read, Write, Close };

    CFileOperationException(Operation op, const CString& path, const CString& detail)
        : std::runtime_error("File operation failed")
        , m_operation(op)
        , m_strPath(path)
        , m_strDetail(detail)
    {
    }

    Operation GetOperation() const { return m_operation; }
    const CString& GetPath() const { return m_strPath; }
    const CString& GetDetail() const { return m_strDetail; }

    CString GetFullMessage() const
    {
        CString strOp;
        switch (m_operation)
        {
        case Operation::Open:  strOp = _T("打开"); break;
        case Operation::Read:  strOp = _T("读取"); break;
        case Operation::Write: strOp = _T("写入"); break;
        case Operation::Close: strOp = _T("关闭"); break;
        }

        CString strMsg;
        strMsg.Format(_T("%s文件失败\n路径: %s\n详情: %s"),
            strOp.GetString(),
            m_strPath.GetString(),
            m_strDetail.GetString());
        return strMsg;
    }

private:
    Operation m_operation;
    CString m_strPath;
    CString m_strDetail;
};

// ============================================================
// RAII CFile 包装器
// ============================================================
class CFileWrapper
{
public:
    // 构造函数 - 自动打开文件
    CFileWrapper(LPCTSTR lpszFileName, UINT nOpenFlags)
        : m_strPath(lpszFileName)
        , m_bOpen(false)
    {
        CFileException ex;
        if (!m_file.Open(lpszFileName, nOpenFlags, &ex))
        {
            CString strError;
            TCHAR szError[1024];
            ex.GetErrorMessage(szError, 1024);
            strError = szError;

            throw CFileOperationException(
                CFileOperationException::Operation::Open,
                m_strPath,
                strError
            );
        }
        m_bOpen = true;
    }

    // 析构函数 - 自动关闭文件
    ~CFileWrapper()
    {
        if (m_bOpen)
        {
            try
            {
                m_file.Close();
            }
            catch (...)
            {
                // 析构函数不抛异常
            }
        }
    }

    // 禁止拷贝
    CFileWrapper(const CFileWrapper&) = delete;
    CFileWrapper& operator=(const CFileWrapper&) = delete;

    // 允许移动
    CFileWrapper(CFileWrapper&& other) noexcept
        : m_file()
        , m_strPath(std::move(other.m_strPath))
        , m_bOpen(other.m_bOpen)
    {
        if (other.m_bOpen)
        {
            // CFile不支持移动，需要特殊处理
            m_file.m_hFile = other.m_file.m_hFile;
            other.m_file.m_hFile = CFile::hFileNull;
            other.m_bOpen = false;
        }
    }

    // 访问器
    CFile* Get() { return &m_file; }
    CFile* operator->() { return &m_file; }
    CFile& operator*() { return m_file; }

    // 获取文件长度
    ULONGLONG GetLength() const { return m_file.GetLength(); }

    // 安全读取
    UINT Read(void* lpBuf, UINT nCount)
    {
        try
        {
            return m_file.Read(lpBuf, nCount);
        }
        catch (CFileException* e)
        {
            CString strError;
            TCHAR szError[1024];
            e->GetErrorMessage(szError, 1024);
            strError = szError;
            e->Delete();

            throw CFileOperationException(
                CFileOperationException::Operation::Read,
                m_strPath,
                strError
            );
        }
    }

    // 安全写入
    void Write(const void* lpBuf, UINT nCount)
    {
        try
        {
            m_file.Write(lpBuf, nCount);
        }
        catch (CFileException* e)
        {
            CString strError;
            TCHAR szError[1024];
            e->GetErrorMessage(szError, 1024);
            strError = szError;
            e->Delete();

            throw CFileOperationException(
                CFileOperationException::Operation::Write,
                m_strPath,
                strError
            );
        }
    }

    // 显式关闭（可选，析构会自动关闭）
    void Close()
    {
        if (m_bOpen)
        {
            m_file.Close();
            m_bOpen = false;
        }
    }

private:
    CFile m_file;
    CString m_strPath;
    bool m_bOpen;
};

// ============================================================
// RAII CArchive 包装器
// ============================================================
class CArchiveWrapper
{
public:
    CArchiveWrapper(CFile* pFile, UINT nMode, int nBufSize = 4096)
        : m_pArchive(nullptr)
    {
        m_pArchive = new CArchive(pFile, nMode, nBufSize);
    }

    ~CArchiveWrapper()
    {
        if (m_pArchive)
        {
            try
            {
                m_pArchive->Close();
            }
            catch (...)
            {
            }
            delete m_pArchive;
            m_pArchive = nullptr;
        }
    }

    // 禁止拷贝
    CArchiveWrapper(const CArchiveWrapper&) = delete;
    CArchiveWrapper& operator=(const CArchiveWrapper&) = delete;

    CArchive* Get() { return m_pArchive; }
    CArchive* operator->() { return m_pArchive; }
    CArchive& operator*() { return *m_pArchive; }

private:
    CArchive* m_pArchive;
};

// ============================================================
// 统一错误处理辅助函数
// ============================================================
class CErrorHandler
{
public:
    // 显示文件操作错误对话框
    static void ShowFileError(const CFileOperationException& ex)
    {
        AfxMessageBox(ex.GetFullMessage(), MB_ICONERROR | MB_OK);
    }

    // 显示通用错误对话框
    static void ShowError(const CString& strTitle, const CString& strDetail)
    {
        CString strMsg;
        strMsg.Format(_T("%s\n\n详情: %s"), strTitle.GetString(), strDetail.GetString());
        AfxMessageBox(strMsg, MB_ICONERROR | MB_OK);
    }

    // 安全执行文件操作的模板函数
    template<typename Func>
    static BOOL SafeFileOperation(Func&& func, const CString& strOperationName)
    {
        try
        {
            func();
            return TRUE;
        }
        catch (const CFileOperationException& ex)
        {
            ShowFileError(ex);
            return FALSE;
        }
        catch (const std::exception& ex)
        {
            CString strDetail;
            strDetail.Format(_T("%S"), ex.what());
            ShowError(strOperationName + _T("失败"), strDetail);
            return FALSE;
        }
        catch (CException* e)
        {
            TCHAR szError[1024];
            e->GetErrorMessage(szError, 1024);
            e->Delete();
            ShowError(strOperationName + _T("失败"), szError);
            return FALSE;
        }
        catch (...)
        {
            ShowError(strOperationName + _T("失败"), _T("未知错误"));
            return FALSE;
        }
    }
};

// ============================================================
// 便捷宏定义
// ============================================================
#define SAFE_FILE_OP(operation, operationName) \
    CErrorHandler::SafeFileOperation([&]() { operation; }, operationName)