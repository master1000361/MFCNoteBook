// MFCNoteBookDoc.cpp: CMFCNoteBookDoc 类的实现
// 使用RAII包装器进行文件操作，符合F-06需求




#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "MFCNoteBook.h"
#endif

#include "MFCNoteBookDoc.h"
#include "MFCNoteBookView.h"
#include "CryptoHelper.h"
#include "RAIIWrappers.h"  // 新增：RAII包装器
#include "ConfigManager.h"  // 新增

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 静态成员初始化
int CMFCNoteBookDoc::s_nUntitledCount = 0;

// CMFCNoteBookDoc

IMPLEMENT_DYNCREATE(CMFCNoteBookDoc, CDocument)

BEGIN_MESSAGE_MAP(CMFCNoteBookDoc, CDocument)
END_MESSAGE_MAP()


// CMFCNoteBookDoc 构造/析构

CMFCNoteBookDoc::CMFCNoteBookDoc() noexcept
    : m_nUntitledNumber(0)
    , m_fileFormat(FileFormat::PlainText)
{
}

CMFCNoteBookDoc::~CMFCNoteBookDoc()
{
}

BOOL CMFCNoteBookDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    m_strContent.Empty();
    m_fileFormat = FileFormat::PlainText;
    m_nUntitledNumber = ++s_nUntitledCount;
    UpdateDocumentTitle();

    return TRUE;
}

// 检测文件格式（使用RAII）
FileFormat CMFCNoteBookDoc::DetectFileFormat(LPCTSTR lpszPathName)
{
    CString strPath(lpszPathName);
    strPath.MakeLower();

    if (strPath.Right(7) == _T(".mynote"))
        return FileFormat::MyNote;
    else if (strPath.Right(4) == _T(".txt"))
        return FileFormat::PlainText;

    // 尝试通过文件头检测（使用RAII）
    try
    {
        CFileWrapper file(lpszPathName, CFile::modeRead | CFile::shareDenyNone);

        char magic[MYNOTE_MAGIC_SIZE + 1] = { 0 };
        if (file.Read(magic, MYNOTE_MAGIC_SIZE) == MYNOTE_MAGIC_SIZE)
        {
            if (memcmp(magic, MYNOTE_MAGIC, MYNOTE_MAGIC_SIZE) == 0)
            {
                return FileFormat::MyNote;
            }
        }
        // 文件自动关闭（RAII）
    }
    catch (...)
    {
        // 无法读取文件，默认为纯文本
    }

    return FileFormat::PlainText;
}

// 保存为纯文本（使用RAII）
BOOL CMFCNoteBookDoc::SaveAsPlainText(LPCTSTR lpszPathName)
{
    return CErrorHandler::SafeFileOperation([&]()
        {
            // RAII: 文件会在作用域结束时自动关闭
            CFileWrapper file(lpszPathName, CFile::modeCreate | CFile::modeWrite);

            // 写入 UTF-8 BOM
            BYTE bom[3] = { 0xEF, 0xBB, 0xBF };
            file.Write(bom, 3);

            // 转换为 UTF-8
            int nLen = WideCharToMultiByte(CP_UTF8, 0, m_strContent, -1, NULL, 0, NULL, NULL);
            if (nLen > 1)
            {
                std::vector<char> utf8Buffer(nLen);
                WideCharToMultiByte(CP_UTF8, 0, m_strContent, -1, utf8Buffer.data(), nLen, NULL, NULL);
                file.Write(utf8Buffer.data(), nLen - 1);
            }

            // 文件在此自动关闭（RAII）

        }, _T("保存文本文件"));
}

// 保存为 MyNote 格式（使用RAII）
BOOL CMFCNoteBookDoc::SaveAsMyNote(LPCTSTR lpszPathName)
{
    return CErrorHandler::SafeFileOperation([&]()
        {
            // ========== 从配置读取学号和密钥 ==========
            CConfigManager& config = CConfigManager::GetInstance();
            if (!config.IsConfigValid())
            {
                throw std::runtime_error("配置未加载或无效");
            }

            CString strStudentID = config.GetStudentID();
            CString strSecretKey = config.GetSecretKey();

            // ✅ 添加调试输出
            TRACE(_T("=== 开始保存 MyNote 文件 ===\n"));
            TRACE(_T("学号: %s\n"), strStudentID.GetString());
            TRACE(_T("密钥长度: %d\n"), strSecretKey.GetLength());
            // ==========================================

            CFileWrapper file(lpszPathName, CFile::modeCreate | CFile::modeWrite);

            // === 1. 写入文件头 ===
            file.Write(MYNOTE_MAGIC, MYNOTE_MAGIC_SIZE);

            // Student ID (固定20字节，不足补0)
            char studentId[MYNOTE_STUDENTID_SIZE] = { 0 };
            CT2A asciiStudentID(strStudentID, CP_UTF8);
            strncpy_s(studentId, asciiStudentID, _TRUNCATE);
            file.Write(studentId, MYNOTE_STUDENTID_SIZE);

            // === 2. 转换内容为 UTF-8 ===
            std::vector<char> utf8Content;
            int nLen = WideCharToMultiByte(CP_UTF8, 0, m_strContent, -1, NULL, 0, NULL, NULL);
            if (nLen > 1)
            {
                utf8Content.resize(nLen);
                WideCharToMultiByte(CP_UTF8, 0, m_strContent, -1, utf8Content.data(), nLen, NULL, NULL);
                nLen--;
            }
            else
            {
                nLen = 0;
            }

            UINT32 contentLen = (UINT32)nLen;

            // ✅ 添加调试输出
            TRACE(_T("内容长度: %d 字节\n"), contentLen);

            file.Write(&contentLen, sizeof(UINT32));

            if (nLen > 0)
            {
                file.Write(utf8Content.data(), nLen);
            }

            // === 3. 计算 SHA-1 摘要 ===
            BYTE sha1Hash[MYNOTE_HASH_SIZE] = { 0 };
            if (nLen > 0)
            {
                CCryptoHelper::ComputeSHA1((const BYTE*)utf8Content.data(), nLen, sha1Hash, MYNOTE_HASH_SIZE);
            }
            else
            {
                CCryptoHelper::ComputeSHA1((const BYTE*)"", 0, sha1Hash, MYNOTE_HASH_SIZE);
            }

            // ✅ 添加调试输出：显示 SHA-1 摘要（十六进制）
            CString strSHA1;
            for (int i = 0; i < MYNOTE_HASH_SIZE; i++)
            {
                CString strByte;
                strByte.Format(_T("%02X"), sha1Hash[i]);
                strSHA1 += strByte;
            }
            TRACE(_T("SHA-1 摘要: %s\n"), strSHA1.GetString());

            // === 4. 生成随机 IV ===
            BYTE iv[MYNOTE_IV_SIZE];
            CCryptoHelper::GenerateRandomIV(iv, MYNOTE_IV_SIZE);

            // ✅ 添加调试输出：显示 IV（十六进制）
            CString strIV;
            for (int i = 0; i < MYNOTE_IV_SIZE; i++)
            {
                CString strByte;
                strByte.Format(_T("%02X"), iv[i]);
                strIV += strByte;
            }
            TRACE(_T("IV: %s\n"), strIV.GetString());

            // === 5. AES-CBC 加密 SHA-1 摘要 ===
            BYTE encryptedHash[MYNOTE_ENCRYPTED_SIZE] = { 0 };
            DWORD dwEncryptedLen = MYNOTE_ENCRYPTED_SIZE;

            CT2A asciiSecretKey(strSecretKey, CP_UTF8);
            CCryptoHelper::AESEncrypt(sha1Hash, MYNOTE_HASH_SIZE,
                (const BYTE*)(const char*)asciiSecretKey, (DWORD)strlen(asciiSecretKey),
                iv,
                encryptedHash, dwEncryptedLen);

            // ✅ 添加调试输出：显示加密后的摘要（十六进制）
            CString strEncrypted;
            for (DWORD i = 0; i < dwEncryptedLen; i++)
            {
                CString strByte;
                strByte.Format(_T("%02X"), encryptedHash[i]);
                strEncrypted += strByte;
            }
            TRACE(_T("加密后摘要: %s\n"), strEncrypted.GetString());
            TRACE(_T("=== 保存完成 ===\n\n"));

            // === 6. 写入 IV 和加密摘要 ===
            file.Write(iv, MYNOTE_IV_SIZE);
            file.Write(encryptedHash, MYNOTE_ENCRYPTED_SIZE);

        }, _T("保存 MyNote 文件"));
}

// 加载纯文本（使用RAII）
BOOL CMFCNoteBookDoc::LoadPlainText(LPCTSTR lpszPathName)
{
    return CErrorHandler::SafeFileOperation([&]()
        {
            // RAII: 文件会在作用域结束时自动关闭
            CFileWrapper file(lpszPathName, CFile::modeRead | CFile::shareDenyNone);
            ULONGLONG nFileLen = file.GetLength();

            if (nFileLen == 0)
            {
                m_strContent.Empty();
                return;  // 文件自动关闭
            }

            // 防止文件过大
            if (nFileLen > 100 * 1024 * 1024)  // 100MB 限制
            {
                throw std::runtime_error("文件过大，超过100MB限制");
            }

            std::vector<char> buffer(static_cast<size_t>(nFileLen + 1));
            file.Read(buffer.data(), (UINT)nFileLen);
            buffer[static_cast<size_t>(nFileLen)] = '\0';

            char* pText = buffer.data();
            int nCodePage = CP_ACP;

            // 检测 BOM
            if (nFileLen >= 3 &&
                (BYTE)buffer[0] == 0xEF &&
                (BYTE)buffer[1] == 0xBB &&
                (BYTE)buffer[2] == 0xBF)
            {
                // UTF-8 BOM
                pText = buffer.data() + 3;
                nCodePage = CP_UTF8;
            }
            else if (nFileLen >= 2 &&
                (BYTE)buffer[0] == 0xFF &&
                (BYTE)buffer[1] == 0xFE)
            {
                // UTF-16 LE BOM
                pText = buffer.data() + 2;
                m_strContent = (wchar_t*)pText;
                return;  // 文件自动关闭
            }

            // 转换为 Unicode
            int nWideLen = MultiByteToWideChar(nCodePage, 0, pText, -1, NULL, 0);
            if (nWideLen > 0)
            {
                std::vector<wchar_t> wideBuffer(nWideLen);
                MultiByteToWideChar(nCodePage, 0, pText, -1, wideBuffer.data(), nWideLen);
                m_strContent = wideBuffer.data();
            }

            // 文件在此自动关闭（RAII）

        }, _T("加载文本文件"));
}

// 加载 MyNote 格式（使用RAII）
BOOL CMFCNoteBookDoc::LoadMyNote(LPCTSTR lpszPathName)
{
    return CErrorHandler::SafeFileOperation([&]()
        {
            // ========== 从配置读取学号和密钥 ==========
            CConfigManager& config = CConfigManager::GetInstance();
            if (!config.IsConfigValid())
            {
                throw std::runtime_error("配置未加载或无效");
            }

            CString strStudentID = config.GetStudentID();
            CString strSecretKey = config.GetSecretKey();

            // ✅ 添加调试输出
            TRACE(_T("=== 开始加载 MyNote 文件 ===\n"));
            TRACE(_T("当前学号: %s\n"), strStudentID.GetString());
            // ==========================================

            CFileWrapper file(lpszPathName, CFile::modeRead | CFile::shareDenyNone);
            ULONGLONG nFileLen = file.GetLength();

            UINT minLen = MYNOTE_MAGIC_SIZE + MYNOTE_STUDENTID_SIZE +
                sizeof(UINT32) + MYNOTE_IV_SIZE + MYNOTE_ENCRYPTED_SIZE;
            if (nFileLen < minLen)
            {
                throw std::runtime_error("无效的 MyNote 文件格式：文件过小");
            }

            // === 1. 读取并验证 Magic ===
            char magic[MYNOTE_MAGIC_SIZE + 1] = { 0 };
            file.Read(magic, MYNOTE_MAGIC_SIZE);
            if (memcmp(magic, MYNOTE_MAGIC, MYNOTE_MAGIC_SIZE) != 0)
            {
                throw std::runtime_error("无效的 MyNote 文件头");
            }

            // === 2. 读取学号 ===
            char studentId[MYNOTE_STUDENTID_SIZE + 1] = { 0 };
            file.Read(studentId, MYNOTE_STUDENTID_SIZE);

            // ✅ 添加调试输出
            TRACE(_T("文件中的学号: %S\n"), studentId);

            CT2A asciiStudentID(strStudentID, CP_UTF8);
            if (strcmp(studentId, asciiStudentID) != 0)
            {
                TRACE(_T("警告：学号不匹配！\n"));
                CString strMsg;
                strMsg.Format(_T("文件学号 [%S] 与当前学号 [%s] 不匹配，继续打开？"),
                    studentId, strStudentID.GetString());
                if (AfxMessageBox(strMsg, MB_YESNO | MB_ICONWARNING) != IDYES)
                {
                    throw std::runtime_error("用户取消打开文件");
                }
            }

            // === 3. 读取内容长度 ===
            UINT32 contentLen = 0;
            file.Read(&contentLen, sizeof(UINT32));

            // ✅ 添加调试输出
            TRACE(_T("内容长度: %d 字节\n"), contentLen);

            if (contentLen > 100 * 1024 * 1024)
            {
                throw std::runtime_error("文件内容过大，超过100MB限制");
            }

            // === 4. 读取内容 ===
            std::vector<char> content(contentLen + 1);
            if (contentLen > 0)
            {
                file.Read(content.data(), contentLen);
            }
            content[contentLen] = '\0';

            // === 5. 读取 IV 和加密摘要 ===
            BYTE iv[MYNOTE_IV_SIZE];
            BYTE encryptedHash[MYNOTE_ENCRYPTED_SIZE];
            file.Read(iv, MYNOTE_IV_SIZE);
            file.Read(encryptedHash, MYNOTE_ENCRYPTED_SIZE);

            // ✅ 添加调试输出：显示读取的 IV
            CString strIV;
            for (int i = 0; i < MYNOTE_IV_SIZE; i++)
            {
                CString strByte;
                strByte.Format(_T("%02X"), iv[i]);
                strIV += strByte;
            }
            TRACE(_T("读取的 IV: %s\n"), strIV.GetString());

            // === 6. 验证摘要 ===
            BYTE computedHash[MYNOTE_HASH_SIZE] = { 0 };
            if (contentLen > 0)
            {
                CCryptoHelper::ComputeSHA1((const BYTE*)content.data(), contentLen,
                    computedHash, MYNOTE_HASH_SIZE);
            }
            else
            {
                CCryptoHelper::ComputeSHA1((const BYTE*)"", 0, computedHash, MYNOTE_HASH_SIZE);
            }

            // ✅ 添加调试输出：显示计算的 SHA-1
            CString strComputedSHA1;
            for (int i = 0; i < MYNOTE_HASH_SIZE; i++)
            {
                CString strByte;
                strByte.Format(_T("%02X"), computedHash[i]);
                strComputedSHA1 += strByte;
            }
            TRACE(_T("计算的 SHA-1: %s\n"), strComputedSHA1.GetString());

            BYTE decryptedHash[MYNOTE_ENCRYPTED_SIZE] = { 0 };
            DWORD dwDecryptedLen = MYNOTE_ENCRYPTED_SIZE;

            CT2A asciiSecretKey(strSecretKey, CP_UTF8);
            if (CCryptoHelper::AESDecrypt(encryptedHash, MYNOTE_ENCRYPTED_SIZE,
                (const BYTE*)(const char*)asciiSecretKey, (DWORD)strlen(asciiSecretKey),
                iv,
                decryptedHash, dwDecryptedLen))
            {
                // ✅ 添加调试输出：显示解密的 SHA-1
                CString strDecryptedSHA1;
                for (int i = 0; i < MYNOTE_HASH_SIZE; i++)
                {
                    CString strByte;
                    strByte.Format(_T("%02X"), decryptedHash[i]);
                    strDecryptedSHA1 += strByte;
                }
                TRACE(_T("解密的 SHA-1: %s\n"), strDecryptedSHA1.GetString());

                if (memcmp(computedHash, decryptedHash, MYNOTE_HASH_SIZE) != 0)
                {
                    TRACE(_T("错误：摘要不匹配！文件可能被篡改\n"));
                    AfxMessageBox(_T("警告：文件完整性校验失败，内容可能已被篡改！"),
                        MB_ICONWARNING);
                }
                else
                {
                    TRACE(_T("成功：摘要验证通过\n"));
                }
            }
            else
            {
                TRACE(_T("错误：解密失败\n"));
                AfxMessageBox(_T("警告：无法解密文件摘要，可能密钥不匹配"), MB_ICONWARNING);
            }

            TRACE(_T("=== 加载完成 ===\n\n"));

            // === 7. 转换内容为 Unicode ===
            if (contentLen > 0)
            {
                int nWideLen = MultiByteToWideChar(CP_UTF8, 0, content.data(), -1, NULL, 0);
                if (nWideLen > 0)
                {
                    std::vector<wchar_t> wideBuffer(nWideLen);
                    MultiByteToWideChar(CP_UTF8, 0, content.data(), -1,
                        wideBuffer.data(), nWideLen);
                    m_strContent = wideBuffer.data();
                }
            }
            else
            {
                m_strContent.Empty();
            }

        }, _T("加载 MyNote 文件"));
}

BOOL CMFCNoteBookDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    // 检测文件格式
    m_fileFormat = DetectFileFormat(lpszPathName);

    BOOL bResult = FALSE;

    if (m_fileFormat == FileFormat::MyNote)
    {
        bResult = LoadMyNote(lpszPathName);
    }
    else
    {
        bResult = LoadPlainText(lpszPathName);
    }

    if (bResult)
    {
        SetModifiedFlag(FALSE);

        // 调用基类设置路径（这会自动调用SetPathName）
        m_strPathName = lpszPathName;

        UpdateDocumentTitle();

        // 通知视图更新
        UpdateAllViews(NULL);
    }

    return bResult;
}

BOOL CMFCNoteBookDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
    // 保存前，先从 View 同步数据
    POSITION pos = GetFirstViewPosition();
    while (pos != NULL)
    {
        CView* pView = GetNextView(pos);
        CMFCNoteBookView* pNoteView = DYNAMIC_DOWNCAST(CMFCNoteBookView, pView);
        if (pNoteView)
        {
            pNoteView->SyncToDocument();
        }
    }

    // 根据扩展名决定格式
    CString strPath(lpszPathName);
    strPath.MakeLower();

    BOOL bResult = FALSE;

    if (strPath.Right(7) == _T(".mynote"))
    {
        m_fileFormat = FileFormat::MyNote;
        bResult = SaveAsMyNote(lpszPathName);
    }
    else
    {
        m_fileFormat = FileFormat::PlainText;
        bResult = SaveAsPlainText(lpszPathName);
    }

    if (bResult)
    {
        SetModifiedFlag(FALSE);
        UpdateDocumentTitle();
    }

    return bResult;
}

void CMFCNoteBookDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
    CDocument::SetPathName(lpszPathName, bAddToMRU);
    UpdateDocumentTitle();
}

void CMFCNoteBookDoc::SetModifiedFlag(BOOL bModified)
{
    BOOL bOldModified = IsModified();
    CDocument::SetModifiedFlag(bModified);

    if (bOldModified != bModified)
    {
        UpdateDocumentTitle();
    }
}

void CMFCNoteBookDoc::UpdateDocumentTitle()
{
    CString strTitle;
    CString strAppName = _T("MFCNoteBook");

    CString strPathName = GetPathName();
    if (strPathName.IsEmpty())
    {
        strTitle.Format(_T("无标题%d"), m_nUntitledNumber);
    }
    else
    {
        int nPos = strPathName.ReverseFind(_T('\\'));
        if (nPos >= 0)
        {
            strTitle = strPathName.Mid(nPos + 1);
        }
        else
        {
            strTitle = strPathName;
        }
    }

    if (IsModified())
    {
        strTitle += _T("*");
    }

    SetTitle(strTitle);

    POSITION pos = GetFirstViewPosition();
    if (pos != NULL)
    {
        CView* pView = GetNextView(pos);
        if (pView != NULL)
        {
            CFrameWnd* pFrame = pView->GetParentFrame();
            if (pFrame != NULL)
            {
                CString strFrameTitle;
                strFrameTitle.Format(_T("%s - %s"), strTitle.GetString(), strAppName.GetString());
                pFrame->SetWindowText(strFrameTitle);
            }
        }
    }
}

// Serialize 现在只做基本的备用处理
void CMFCNoteBookDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // 不应该调用到这里，OnSaveDocument 已处理
    }
    else
    {
        // 不应该调用到这里，OnOpenDocument 已处理
    }
}


#ifdef SHARED_HANDLERS
void CMFCNoteBookDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
    dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));
    CString strText = _T("MyNote Document");
    LOGFONT lf;
    CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
    pDefaultGUIFont->GetLogFont(&lf);
    lf.lfHeight = 36;
    CFont fontDraw;
    fontDraw.CreateFontIndirect(&lf);
    CFont* pOldFont = dc.SelectObject(&fontDraw);
    dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
    dc.SelectObject(pOldFont);
}

void CMFCNoteBookDoc::InitializeSearchContent()
{
    CString strSearchContent;
    SetSearchContent(strSearchContent);
}

void CMFCNoteBookDoc::SetSearchContent(const CString& value)
{
    if (value.IsEmpty())
    {
        RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
    }
    else
    {
        CMFCFilterChunkValueImpl* pChunk = nullptr;
        ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
        if (pChunk != nullptr)
        {
            pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
            SetChunkValue(pChunk);
        }
    }
}
#endif // SHARED_HANDLERS

#ifdef _DEBUG
void CMFCNoteBookDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CMFCNoteBookDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG
