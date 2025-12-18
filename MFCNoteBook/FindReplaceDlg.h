// FindReplaceDlg.h - 查找替换对话框
#pragma once

#include <regex>
#include <string>

// 前向声明
class CMFCNoteBookView;

// 搜索选项结构
struct FindOptions
{
    CString strFind;           // 查找内容
    CString strReplace;        // 替换内容
    BOOL bUseRegex;            // 使用正则表达式
    BOOL bMatchCase;           // 区分大小写
    BOOL bWholeWord;           // 全字匹配
    int nSearchPos;            // 当前搜索位置

    FindOptions() : bUseRegex(FALSE), bMatchCase(FALSE),
        bWholeWord(FALSE), nSearchPos(0) {
    }
};

// 查找替换对话框类
class CFindReplaceDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CFindReplaceDlg)

public:
    CFindReplaceDlg(CMFCNoteBookView* pView, CWnd* pParent = nullptr);
    virtual ~CFindReplaceDlg();

    // 对话框数据
    enum { IDD = IDD_FIND_REPLACE };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnCancel();

    DECLARE_MESSAGE_MAP()

public:
    // 控件变量
    CEdit m_editFind;
    CEdit m_editReplace;
    CButton m_chkRegex;
    CButton m_chkCase;
    CButton m_chkWholeWord;
    CStatic m_staticStatus;

    // 数据
    FindOptions m_Options;
    CMFCNoteBookView* m_pView;

    // ========== 新增：设置查找文本 ==========
    void SetFindText(const CString& str)
    {
        m_Options.strFind = str;
        if (m_editFind.GetSafeHwnd())
            m_editFind.SetWindowText(str);
    }
    // ========================================


    // 消息处理
    afx_msg void OnBtnFindNext();
    afx_msg void OnBtnReplace();
    afx_msg void OnBtnReplaceAll();
    afx_msg void OnEditFindChange();

private:
    // 辅助函数
    void UpdateOptions();
    void SetStatusText(const CString& str, BOOL bError = FALSE);

    // 核心搜索函数
    BOOL FindNext(BOOL bShowNotFound = TRUE);
    BOOL DoRegexFind(const CString& strText, const CString& strPattern,
        int nStartPos, int& nFoundStart, int& nFoundEnd);
    BOOL DoNormalFind(const CString& strText, const CString& strFind,
        int nStartPos, int& nFoundStart, int& nFoundEnd);

    // 替换函数
    CString DoRegexReplace(const CString& strText, const CString& strPattern,
        const CString& strReplace, int& nCount);
};
