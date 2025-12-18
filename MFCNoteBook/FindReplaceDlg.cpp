// FindReplaceDlg.cpp - 查找替换对话框实现
#include "pch.h"
#include "framework.h"
#include "MFCNoteBook.h"
#include "FindReplaceDlg.h"
#include "MFCNoteBookView.h"
#include "Resource.h"

#include <regex>
#include <string>
#include <locale>
#include <codecvt>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CFindReplaceDlg, CDialogEx)

CFindReplaceDlg::CFindReplaceDlg(CMFCNoteBookView* pView, CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_FIND_REPLACE, pParent)
    , m_pView(pView)
{
}

CFindReplaceDlg::~CFindReplaceDlg()
{
}

void CFindReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_FIND, m_editFind);
    DDX_Control(pDX, IDC_EDIT_REPLACE, m_editReplace);
    DDX_Control(pDX, IDC_CHECK_REGEX, m_chkRegex);
    DDX_Control(pDX, IDC_CHECK_CASE, m_chkCase);
    DDX_Control(pDX, IDC_CHECK_WHOLE_WORD, m_chkWholeWord);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
}

BEGIN_MESSAGE_MAP(CFindReplaceDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_FIND_NEXT, &CFindReplaceDlg::OnBtnFindNext)
    ON_BN_CLICKED(IDC_BTN_REPLACE, &CFindReplaceDlg::OnBtnReplace)
    ON_BN_CLICKED(IDC_BTN_REPLACE_ALL, &CFindReplaceDlg::OnBtnReplaceAll)
    ON_EN_CHANGE(IDC_EDIT_FIND, &CFindReplaceDlg::OnEditFindChange)
END_MESSAGE_MAP()

BOOL CFindReplaceDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 设置初始状态
    m_chkRegex.SetCheck(m_Options.bUseRegex ? BST_CHECKED : BST_UNCHECKED);
    m_chkCase.SetCheck(m_Options.bMatchCase ? BST_CHECKED : BST_UNCHECKED);
    m_chkWholeWord.SetCheck(m_Options.bWholeWord ? BST_CHECKED : BST_UNCHECKED);

    if (!m_Options.strFind.IsEmpty())
    {
        m_editFind.SetWindowText(m_Options.strFind);
    }

    m_editFind.SetFocus();
    SetStatusText(_T(""));

    return FALSE;  // 已设置焦点
}

void CFindReplaceDlg::OnCancel()
{
    // 保存当前选项
    UpdateOptions();

    // 隐藏而不是销毁
    ShowWindow(SW_HIDE);
}

void CFindReplaceDlg::UpdateOptions()
{
    m_editFind.GetWindowText(m_Options.strFind);
    m_editReplace.GetWindowText(m_Options.strReplace);
    m_Options.bUseRegex = (m_chkRegex.GetCheck() == BST_CHECKED);
    m_Options.bMatchCase = (m_chkCase.GetCheck() == BST_CHECKED);
    m_Options.bWholeWord = (m_chkWholeWord.GetCheck() == BST_CHECKED);
}

void CFindReplaceDlg::SetStatusText(const CString& str, BOOL bError /*= FALSE*/)
{
    m_staticStatus.SetWindowText(str);
    // 可以设置不同颜色，这里简化处理
}

void CFindReplaceDlg::OnEditFindChange()
{
    // 重置搜索位置
    m_Options.nSearchPos = 0;
    SetStatusText(_T(""));
}

// ========== 查找下一个 ==========
void CFindReplaceDlg::OnBtnFindNext()
{
    FindNext(TRUE);
}

BOOL CFindReplaceDlg::FindNext(BOOL bShowNotFound /*= TRUE*/)
{
    if (!m_pView || !m_pView->GetEditCtrl().GetSafeHwnd())
        return FALSE;

    UpdateOptions();

    if (m_Options.strFind.IsEmpty())
    {
        SetStatusText(_T("请输入查找内容"), TRUE);
        return FALSE;
    }

    // 获取编辑控件文本
    CEdit& edit = m_pView->GetEditCtrl();
    CString strText;
    edit.GetWindowText(strText);

    if (strText.IsEmpty())
    {
        SetStatusText(_T("文档为空"), TRUE);
        return FALSE;
    }

    // 获取当前选择位置作为起始点
    int nStart, nEnd;
    edit.GetSel(nStart, nEnd);

    // 从选择结束位置开始搜索（避免重复找到同一个）
    int nSearchStart = nEnd;
    if (nSearchStart > strText.GetLength())
        nSearchStart = 0;

    int nFoundStart = -1, nFoundEnd = -1;
    BOOL bFound = FALSE;

    // 根据选项执行搜索
    if (m_Options.bUseRegex)
    {
        bFound = DoRegexFind(strText, m_Options.strFind, nSearchStart, nFoundStart, nFoundEnd);

        // 如果没找到，从头开始
        if (!bFound && nSearchStart > 0)
        {
            bFound = DoRegexFind(strText, m_Options.strFind, 0, nFoundStart, nFoundEnd);
            if (bFound)
            {
                SetStatusText(_T("已从头开始搜索"));
            }
        }
    }
    else
    {
        bFound = DoNormalFind(strText, m_Options.strFind, nSearchStart, nFoundStart, nFoundEnd);

        // 如果没找到，从头开始
        if (!bFound && nSearchStart > 0)
        {
            bFound = DoNormalFind(strText, m_Options.strFind, 0, nFoundStart, nFoundEnd);
            if (bFound)
            {
                SetStatusText(_T("已从头开始搜索"));
            }
        }
    }

    if (bFound)
    {
        // 选中找到的文本
        edit.SetSel(nFoundStart, nFoundEnd);
        edit.SetFocus();

        // 确保可见
        int nLine = edit.LineFromChar(nFoundStart);
        edit.LineScroll(nLine - edit.GetFirstVisibleLine());

        m_Options.nSearchPos = nFoundEnd;

        CString strStatus;
        strStatus.Format(_T("找到于位置 %d"), nFoundStart);
        SetStatusText(strStatus);
        return TRUE;
    }
    else
    {
        if (bShowNotFound)
        {
            SetStatusText(_T("找不到指定内容"), TRUE);
            MessageBeep(MB_ICONEXCLAMATION);
        }
        return FALSE;
    }
}

// ========== 正则表达式查找 ==========
BOOL CFindReplaceDlg::DoRegexFind(const CString& strText, const CString& strPattern,
    int nStartPos, int& nFoundStart, int& nFoundEnd)
{
    try
    {
        // 转换为std::wstring
        std::wstring text((LPCTSTR)strText);
        std::wstring pattern((LPCTSTR)strPattern);

        // 设置正则选项
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!m_Options.bMatchCase)
        {
            flags |= std::regex_constants::icase;
        }

        std::wregex regex(pattern, flags);
        std::wsmatch match;

        // 从指定位置开始搜索
        if (nStartPos < (int)text.length())
        {
            std::wstring searchText = text.substr(nStartPos);
            if (std::regex_search(searchText, match, regex))
            {
                nFoundStart = nStartPos + (int)match.position();
                nFoundEnd = nFoundStart + (int)match.length();
                return TRUE;
            }
        }

    }
    catch (const std::regex_error& e)
    {
        CString strError;
        strError.Format(_T("正则表达式错误: %S"), e.what());
        SetStatusText(strError, TRUE);
    }

    return FALSE;
}

// ========== 普通查找 ==========
BOOL CFindReplaceDlg::DoNormalFind(const CString& strText, const CString& strFind,
    int nStartPos, int& nFoundStart, int& nFoundEnd)
{
    CString strSearchText = strText;
    CString strSearchPattern = strFind;

    // 不区分大小写时转换为小写
    if (!m_Options.bMatchCase)
    {
        strSearchText.MakeLower();
        strSearchPattern.MakeLower();
    }

    int nPos = strSearchText.Find(strSearchPattern, nStartPos);

    if (nPos >= 0)
    {
        // 全字匹配检查
        if (m_Options.bWholeWord)
        {
            while (nPos >= 0)
            {
                BOOL bWordStart = (nPos == 0) || !_istalnum(strText.GetAt(nPos - 1));
                BOOL bWordEnd = (nPos + strFind.GetLength() >= strText.GetLength()) ||
                    !_istalnum(strText.GetAt(nPos + strFind.GetLength()));

                if (bWordStart && bWordEnd)
                {
                    break;  // 找到全字匹配
                }

                // 继续搜索
                nPos = strSearchText.Find(strSearchPattern, nPos + 1);
            }
        }

        if (nPos >= 0)
        {
            nFoundStart = nPos;
            nFoundEnd = nPos + strFind.GetLength();
            return TRUE;
        }
    }

    return FALSE;
}

// ========== 替换 ==========
void CFindReplaceDlg::OnBtnReplace()
{
    if (!m_pView || !m_pView->GetEditCtrl().GetSafeHwnd())
        return;

    UpdateOptions();

    CEdit& edit = m_pView->GetEditCtrl();

    // 获取当前选择
    int nStart, nEnd;
    edit.GetSel(nStart, nEnd);

    // 如果有选中内容，先替换
    if (nStart != nEnd)
    {
        CString strSelected;
        edit.GetWindowText(strSelected);
        strSelected = strSelected.Mid(nStart, nEnd - nStart);

        // 验证选中内容是否匹配
        BOOL bMatch = FALSE;
        if (m_Options.bUseRegex)
        {
            try
            {
                std::wstring text((LPCTSTR)strSelected);
                std::wstring pattern((LPCTSTR)m_Options.strFind);

                std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
                if (!m_Options.bMatchCase)
                    flags |= std::regex_constants::icase;

                std::wregex regex(pattern, flags);
                bMatch = std::regex_match(text, regex);
            }
            catch (...)
            {
                bMatch = FALSE;
            }
        }
        else
        {
            CString strCmp1 = strSelected;
            CString strCmp2 = m_Options.strFind;
            if (!m_Options.bMatchCase)
            {
                strCmp1.MakeLower();
                strCmp2.MakeLower();
            }
            bMatch = (strCmp1 == strCmp2);
        }

        if (bMatch)
        {
            // 执行替换
            CString strReplacement = m_Options.strReplace;

            // 如果是正则替换，处理捕获组
            if (m_Options.bUseRegex)
            {
                try
                {
                    std::wstring text((LPCTSTR)strSelected);
                    std::wstring pattern((LPCTSTR)m_Options.strFind);
                    std::wstring replacement((LPCTSTR)m_Options.strReplace);

                    std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
                    if (!m_Options.bMatchCase)
                        flags |= std::regex_constants::icase;

                    std::wregex regex(pattern, flags);
                    std::wstring result = std::regex_replace(text, regex, replacement);
                    strReplacement = result.c_str();
                }
                catch (...)
                {
                    // 使用普通替换
                }
            }

            edit.ReplaceSel(strReplacement, TRUE);
            SetStatusText(_T("已替换"));
        }
    }

    // 查找下一个
    FindNext(TRUE);
}

// ========== 全部替换 ==========
void CFindReplaceDlg::OnBtnReplaceAll()
{
    if (!m_pView || !m_pView->GetEditCtrl().GetSafeHwnd())
        return;

    UpdateOptions();

    if (m_Options.strFind.IsEmpty())
    {
        SetStatusText(_T("请输入查找内容"), TRUE);
        return;
    }

    CEdit& edit = m_pView->GetEditCtrl();
    CString strText;
    edit.GetWindowText(strText);

    int nCount = 0;

    if (m_Options.bUseRegex)
    {
        // 正则全部替换
        strText = DoRegexReplace(strText, m_Options.strFind, m_Options.strReplace, nCount);
    }
    else
    {
        // 普通全部替换
        CString strFind = m_Options.strFind;
        CString strReplace = m_Options.strReplace;
        CString strSearchText = strText;

        if (!m_Options.bMatchCase)
        {
            strSearchText.MakeLower();
            CString strFindLower = strFind;
            strFindLower.MakeLower();

            int nPos = 0;
            while ((nPos = strSearchText.Find(strFindLower, nPos)) >= 0)
            {
                // 全字匹配检查
                BOOL bReplace = TRUE;
                if (m_Options.bWholeWord)
                {
                    BOOL bWordStart = (nPos == 0) || !_istalnum(strText.GetAt(nPos - 1));
                    BOOL bWordEnd = (nPos + strFind.GetLength() >= strText.GetLength()) ||
                        !_istalnum(strText.GetAt(nPos + strFind.GetLength()));
                    bReplace = bWordStart && bWordEnd;
                }

                if (bReplace)
                {
                    strText = strText.Left(nPos) + strReplace +
                        strText.Mid(nPos + strFind.GetLength());
                    strSearchText = strText;
                    strSearchText.MakeLower();
                    nPos += strReplace.GetLength();
                    nCount++;
                }
                else
                {
                    nPos++;
                }
            }
        }
        else
        {
            int nPos = 0;
            while ((nPos = strText.Find(strFind, nPos)) >= 0)
            {
                BOOL bReplace = TRUE;
                if (m_Options.bWholeWord)
                {
                    BOOL bWordStart = (nPos == 0) || !_istalnum(strText.GetAt(nPos - 1));
                    BOOL bWordEnd = (nPos + strFind.GetLength() >= strText.GetLength()) ||
                        !_istalnum(strText.GetAt(nPos + strFind.GetLength()));
                    bReplace = bWordStart && bWordEnd;
                }

                if (bReplace)
                {
                    strText = strText.Left(nPos) + strReplace +
                        strText.Mid(nPos + strFind.GetLength());
                    nPos += strReplace.GetLength();
                    nCount++;
                }
                else
                {
                    nPos++;
                }
            }
        }
    }

    if (nCount > 0)
    {
        edit.SetWindowText(strText);
        edit.SetSel(0, 0);

        CString strStatus;
        strStatus.Format(_T("已替换 %d 处"), nCount);
        SetStatusText(strStatus);
    }
    else
    {
        SetStatusText(_T("没有找到匹配项"), TRUE);
    }
}

// ========== 正则表达式全部替换 ==========
CString CFindReplaceDlg::DoRegexReplace(const CString& strText, const CString& strPattern,
    const CString& strReplace, int& nCount)
{
    nCount = 0;

    try
    {
        std::wstring text((LPCTSTR)strText);
        std::wstring pattern((LPCTSTR)strPattern);
        std::wstring replacement((LPCTSTR)strReplace);

        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!m_Options.bMatchCase)
        {
            flags |= std::regex_constants::icase;
        }

        std::wregex regex(pattern, flags);

        // 计算替换次数
       // 计算替换次数
        std::wsregex_iterator it(text.begin(), text.end(), regex);
        std::wsregex_iterator end;
        while (it != end)
        {
            nCount++;
            ++it;
        }


        // 执行替换
        std::wstring result = std::regex_replace(text, regex, replacement);
        return CString(result.c_str());
    }
    catch (const std::regex_error& e)
    {
        CString strError;
        strError.Format(_T("正则表达式错误: %S"), e.what());
        SetStatusText(strError, TRUE);
        return strText;
    }
}
