// MFCNoteBookDoc.h: CMFCNoteBookDoc 类的接口
//

#pragma once

// *.mynote 文件格式常量
#define MYNOTE_MAGIC        "MYNOTE01"
#define MYNOTE_MAGIC_SIZE   8
#define MYNOTE_STUDENTID_SIZE 20
#define MYNOTE_IV_SIZE      16
#define MYNOTE_HASH_SIZE    20
#define MYNOTE_ENCRYPTED_SIZE 32  // AES 加密后的 SHA1 (20字节 + 填充)

// 文件类型枚举
enum class FileFormat
{
    Unknown,
    PlainText,  // *.txt
    MyNote      // *.mynote
};

class CMFCNoteBookDoc : public CDocument
{
protected:
    CMFCNoteBookDoc() noexcept;
    DECLARE_DYNCREATE(CMFCNoteBookDoc)

    // 特性
public:
    CString m_strContent;  // 存储文本内容
    FileFormat m_fileFormat;  // 当前文件格式

    // 操作
public:
    void UpdateDocumentTitle();

    // 文件格式相关
    FileFormat DetectFileFormat(LPCTSTR lpszPathName);
    BOOL SaveAsPlainText(LPCTSTR lpszPathName);
    BOOL SaveAsMyNote(LPCTSTR lpszPathName);
    BOOL LoadPlainText(LPCTSTR lpszPathName);
    BOOL LoadMyNote(LPCTSTR lpszPathName);

    // 重写
public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);
    virtual void SetModifiedFlag(BOOL bModified = TRUE);
    virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);

#ifdef SHARED_HANDLERS
    virtual void InitializeSearchContent();
    virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif

    // 实现
public:
    virtual ~CMFCNoteBookDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    static int s_nUntitledCount;
    int m_nUntitledNumber;

    DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
    void SetSearchContent(const CString& value);
#endif
};
