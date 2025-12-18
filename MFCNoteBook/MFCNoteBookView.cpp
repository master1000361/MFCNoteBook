// MFCNoteBookView.cpp: CMFCNoteBookView 类的实现
//

#include "pch.h"
#include "framework.h"
#ifndef SHARED_HANDLERS
#include "MFCNoteBook.h"
#endif

#include "MFCNoteBookDoc.h"
#include "MFCNoteBookView.h"
#include "FindReplaceDlg.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define IDC_EDIT_CONTROL 1001

IMPLEMENT_DYNCREATE(CMFCNoteBookView, CView)

BEGIN_MESSAGE_MAP(CMFCNoteBookView, CView)
    ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_WM_CTLCOLOR()
    ON_EN_CHANGE(IDC_EDIT_CONTROL, &CMFCNoteBookView::OnEditChange)
    ON_EN_VSCROLL(IDC_EDIT_CONTROL, &CMFCNoteBookView::OnEditScroll)

    ON_COMMAND(ID_EDIT_UNDO, &CMFCNoteBookView::OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, &CMFCNoteBookView::OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CMFCNoteBookView::OnUpdateEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CMFCNoteBookView::OnUpdateEditRedo)

    ON_COMMAND(ID_EDIT_CUT, &CMFCNoteBookView::OnEditCut)
    ON_COMMAND(ID_EDIT_COPY, &CMFCNoteBookView::OnEditCopy)
    ON_COMMAND(ID_EDIT_PASTE, &CMFCNoteBookView::OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, &CMFCNoteBookView::OnUpdateEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CMFCNoteBookView::OnUpdateEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CMFCNoteBookView::OnUpdateEditPaste)

    ON_COMMAND(ID_EDIT_FIND, &CMFCNoteBookView::OnEditFind)
    ON_COMMAND(ID_EDIT_REPLACE, &CMFCNoteBookView::OnEditReplace)

    ON_COMMAND(ID_VIEW_ZOOM_IN, &CMFCNoteBookView::OnViewZoomIn)
    ON_COMMAND(ID_VIEW_ZOOM_OUT, &CMFCNoteBookView::OnViewZoomOut)
    ON_COMMAND(ID_VIEW_ZOOM_RESET, &CMFCNoteBookView::OnViewZoomReset)
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

// CMFCNoteBookView 构造/析构

CMFCNoteBookView::CMFCNoteBookView() noexcept
    : m_nLineNumWidth(50)
    , m_bInternalChange(false)
    , m_pFindReplaceDlg(nullptr)
    , m_nFontSize(FONT_SIZE_DEFAULT)
{
}

CMFCNoteBookView::~CMFCNoteBookView()
{
    if (m_pFindReplaceDlg)
    {
        m_pFindReplaceDlg->DestroyWindow();
        delete m_pFindReplaceDlg;
        m_pFindReplaceDlg = nullptr;
    }
}

BOOL CMFCNoteBookView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CView::PreCreateWindow(cs);
}

// ========== OnDraw - 使用主题颜色 ==========
void CMFCNoteBookView::OnDraw(CDC* pDC)
{
    CMFCNoteBookDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
    if (!pDoc)
        return;

    // 获取当前主题颜色
    const ThemeColors& theme = theApp.GetThemeColors();

    CRect clientRect;
    GetClientRect(&clientRect);

    // 1. 绘制行号区域背景（使用主题颜色）
    CRect lineNumRect = clientRect;
    lineNumRect.right = m_nLineNumWidth;
    pDC->FillSolidRect(lineNumRect, theme.clrLineNumBg);

    // 2. 绘制分隔线（使用主题颜色）
    pDC->FillSolidRect(m_nLineNumWidth - 1, 0, 1, clientRect.Height(), theme.clrLineNumBorder);

    // 3. 绘制行号
    if (!m_Edit.GetSafeHwnd())
        return;

    CFont* pOldFont = pDC->SelectObject(&m_Font);

    TEXTMETRIC tm;
    pDC->GetTextMetrics(&tm);
    int lineHeight = tm.tmHeight;

    int firstVisibleLine = m_Edit.GetFirstVisibleLine();
    int lineCount = m_Edit.GetLineCount();

    int visibleLines = clientRect.Height() / lineHeight + 2;

    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(theme.clrLineNumText);  // 使用主题颜色

    for (int i = 0; i < visibleLines; i++)
    {
        int lineNum = firstVisibleLine + i + 1;
        if (lineNum > lineCount)
            break;

        CString strLineNum;
        strLineNum.Format(_T("%4d"), lineNum);

        CRect numRect;
        numRect.left = 5;
        numRect.right = m_nLineNumWidth - 8;
        numRect.top = i * lineHeight;
        numRect.bottom = numRect.top + lineHeight;

        pDC->DrawText(strLineNum, numRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    pDC->SelectObject(pOldFont);
}

// 打印相关
BOOL CMFCNoteBookView::OnPreparePrinting(CPrintInfo* pInfo)
{
    return DoPreparePrinting(pInfo);
}

void CMFCNoteBookView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CMFCNoteBookView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

// 诊断
#ifdef _DEBUG
void CMFCNoteBookView::AssertValid() const
{
    CView::AssertValid();
}

void CMFCNoteBookView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CMFCNoteBookDoc* CMFCNoteBookView::GetDocument() const
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMFCNoteBookDoc)));
    return (CMFCNoteBookDoc*)m_pDocument;
}
#endif //_DEBUG

int CMFCNoteBookView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    CRect rect(m_nLineNumWidth, 0, 100, 100);
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN;

    if (!m_Edit.Create(dwStyle, rect, this, IDC_EDIT_CONTROL))
    {
        TRACE0("未能创建编辑控件\n");
        return -1;
    }

    // 创建默认字体
    m_Font.CreatePointFont(m_nFontSize * 10, _T("Consolas"));
    m_Edit.SetFont(&m_Font);

    // 初始化时应用主题
    ApplyTheme();

    return 0;
}

void CMFCNoteBookView::OnSize(UINT nType, int cx, int cy)
{
    CView::OnSize(nType, cx, cy);

    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.MoveWindow(m_nLineNumWidth, 0, cx - m_nLineNumWidth, cy);
    }
}

void CMFCNoteBookView::OnSetFocus(CWnd* pOldWnd)
{
    CView::OnSetFocus(pOldWnd);

    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.SetFocus();
    }
}

void CMFCNoteBookView::OnInitialUpdate()
{
    CView::OnInitialUpdate();

    // 1. 编辑控件已在 OnCreate 中创建，这里不需要重复创建

    // 2. 创建字体并应用
    CreateEditFont();

    // 3. 应用主题
    ApplyTheme();

    // 4. 加载文档内容
    CMFCNoteBookDoc* pDoc = GetDocument();
    if (pDoc && m_Edit.GetSafeHwnd())
    {
        m_bInternalChange = true;
        m_Edit.SetWindowText(pDoc->m_strContent);
        m_bInternalChange = false;

        m_Edit.GetWindowText(m_strLastText);
        m_UndoStack.clear();
        m_RedoStack.clear();
    }

    // 5. 更新行号宽度
    UpdateLineNumberWidth();
}

void CMFCNoteBookView::SyncToDocument()
{
    CMFCNoteBookDoc* pDoc = GetDocument();
    if (pDoc && m_Edit.GetSafeHwnd())
    {
        m_Edit.GetWindowText(pDoc->m_strContent);
    }
}

void CMFCNoteBookView::OnEditChange()
{
    CMFCNoteBookDoc* pDoc = GetDocument();
    if (pDoc && m_Edit.GetSafeHwnd())
    {
        m_Edit.GetWindowText(pDoc->m_strContent);
        pDoc->SetModifiedFlag(TRUE);
    }

    if (!m_bInternalChange)
    {
        SaveUndoState();
    }

    UpdateLineNumberWidth();

    CRect lineNumRect;
    GetClientRect(&lineNumRect);
    lineNumRect.right = m_nLineNumWidth;
    InvalidateRect(&lineNumRect, FALSE);
}

void CMFCNoteBookView::OnEditScroll()
{
    CRect lineNumRect;
    GetClientRect(&lineNumRect);
    lineNumRect.right = m_nLineNumWidth;
    InvalidateRect(&lineNumRect, FALSE);
}

void CMFCNoteBookView::UpdateLineNumberWidth()
{
    if (!m_Edit.GetSafeHwnd())
        return;

    int lineCount = m_Edit.GetLineCount();

    int digits = 1;
    int temp = lineCount;
    while (temp >= 10)
    {
        temp /= 10;
        digits++;
    }
    digits = max(digits, 4);

    CClientDC dc(this);
    CFont* pOldFont = dc.SelectObject(&m_Font);
    CSize charSize = dc.GetTextExtent(_T("0"));
    dc.SelectObject(pOldFont);

    int newWidth = charSize.cx * digits + 20;

    if (newWidth != m_nLineNumWidth)
    {
        m_nLineNumWidth = newWidth;

        CRect rect;
        GetClientRect(&rect);
        m_Edit.MoveWindow(m_nLineNumWidth, 0,
            rect.Width() - m_nLineNumWidth, rect.Height());

        Invalidate();
    }
}

// ========== OnCtlColor - 使用主题颜色 ==========
HBRUSH CMFCNoteBookView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (pWnd->GetSafeHwnd() == m_Edit.GetSafeHwnd())
    {
        const ThemeColors& theme = theApp.GetThemeColors();

        // 设置文字颜色和背景颜色
        pDC->SetTextColor(theme.clrEditText);
        pDC->SetBkColor(theme.clrEditBg);

        // 确保画刷已创建
        if (!m_brEditBg.GetSafeHandle())
        {
            m_brEditBg.CreateSolidBrush(theme.clrEditBg);
        }

        return (HBRUSH)m_brEditBg.GetSafeHandle();
    }

    return CView::OnCtlColor(pDC, pWnd, nCtlColor);
}

// ========== 应用主题 ==========
void CMFCNoteBookView::ApplyTheme()
{
    const ThemeColors& theme = theApp.GetThemeColors();

    // 删除旧画刷，创建新画刷
    if (m_brEditBg.GetSafeHandle())
    {
        m_brEditBg.DeleteObject();
    }
    m_brEditBg.CreateSolidBrush(theme.clrEditBg);

    // 强制重绘编辑控件
    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.Invalidate(TRUE);
        m_Edit.UpdateWindow();

        Invalidate(TRUE);
        UpdateWindow();
    }
}

// ========== 撤销/重做实现 ==========

void CMFCNoteBookView::SaveUndoState()
{
    if (!m_Edit.GetSafeHwnd())
        return;

    CString strCurrentText;
    m_Edit.GetWindowText(strCurrentText);

    if (strCurrentText == m_strLastText)
        return;

    m_UndoStack.push_back(m_strLastText);

    while (m_UndoStack.size() > MAX_UNDO)
    {
        m_UndoStack.erase(m_UndoStack.begin());
    }

    m_RedoStack.clear();

    m_strLastText = strCurrentText;
}

void CMFCNoteBookView::OnEditUndo()
{
    if (m_UndoStack.empty() || !m_Edit.GetSafeHwnd())
        return;

    CString strCurrentText;
    m_Edit.GetWindowText(strCurrentText);
    m_RedoStack.push_back(strCurrentText);

    CString strPrevText = m_UndoStack.back();
    m_UndoStack.pop_back();

    m_bInternalChange = true;
    m_Edit.SetWindowText(strPrevText);
    m_strLastText = strPrevText;
    m_bInternalChange = false;

    m_Edit.SetSel(-1, -1);

    SyncToDocument();

    UpdateLineNumberWidth();
    Invalidate();
}

void CMFCNoteBookView::OnEditRedo()
{
    if (m_RedoStack.empty() || !m_Edit.GetSafeHwnd())
        return;

    CString strCurrentText;
    m_Edit.GetWindowText(strCurrentText);
    m_UndoStack.push_back(strCurrentText);

    CString strNextText = m_RedoStack.back();
    m_RedoStack.pop_back();

    m_bInternalChange = true;
    m_Edit.SetWindowText(strNextText);
    m_strLastText = strNextText;
    m_bInternalChange = false;

    m_Edit.SetSel(-1, -1);

    SyncToDocument();

    UpdateLineNumberWidth();
    Invalidate();
}

void CMFCNoteBookView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_UndoStack.empty());
}

void CMFCNoteBookView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_RedoStack.empty());
}

// ========== 剪切/复制/粘贴实现 ==========

void CMFCNoteBookView::OnEditCut()
{
    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.Cut();
    }
}

void CMFCNoteBookView::OnEditCopy()
{
    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.Copy();
    }
}

void CMFCNoteBookView::OnEditPaste()
{
    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.Paste();
    }
}

void CMFCNoteBookView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
    if (m_Edit.GetSafeHwnd())
    {
        int nStart, nEnd;
        m_Edit.GetSel(nStart, nEnd);
        pCmdUI->Enable(nStart != nEnd);
    }
    else
    {
        pCmdUI->Enable(FALSE);
    }
}

void CMFCNoteBookView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
    if (m_Edit.GetSafeHwnd())
    {
        int nStart, nEnd;
        m_Edit.GetSel(nStart, nEnd);
        pCmdUI->Enable(nStart != nEnd);
    }
    else
    {
        pCmdUI->Enable(FALSE);
    }
}

void CMFCNoteBookView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    BOOL bCanPaste = FALSE;

    if (m_Edit.GetSafeHwnd() && ::IsClipboardFormatAvailable(CF_TEXT))
    {
        bCanPaste = TRUE;
    }

    if (m_Edit.GetSafeHwnd() && ::IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        bCanPaste = TRUE;
    }

    pCmdUI->Enable(bCanPaste);
}

// ========== 查找替换实现 ==========

void CMFCNoteBookView::OnEditFind()
{
    if (!m_pFindReplaceDlg)
    {
        m_pFindReplaceDlg = new CFindReplaceDlg(this);
        m_pFindReplaceDlg->Create(IDD_FIND_REPLACE, this);
    }

    if (m_Edit.GetSafeHwnd())
    {
        int nStart, nEnd;
        m_Edit.GetSel(nStart, nEnd);
        if (nStart != nEnd)
        {
            CString strText;
            m_Edit.GetWindowText(strText);
            m_pFindReplaceDlg->m_Options.strFind = strText.Mid(nStart, nEnd - nStart);
            m_pFindReplaceDlg->m_editFind.SetWindowText(m_pFindReplaceDlg->m_Options.strFind);
        }
    }

    m_pFindReplaceDlg->ShowWindow(SW_SHOW);
    m_pFindReplaceDlg->SetFocus();
}

void CMFCNoteBookView::OnEditReplace()
{
    OnEditFind();
}

// ========== 字体缩放功能 ==========

void CMFCNoteBookView::CreateEditFont()
{
    // 删除旧字体
    if (m_EditFont.GetSafeHandle())
    {
        m_EditFont.DeleteObject();
    }
    if (m_LineNumFont.GetSafeHandle())
    {
        m_LineNumFont.DeleteObject();
    }
    if (m_Font.GetSafeHandle())
    {
        m_Font.DeleteObject();
    }

    // 创建新字体
    m_EditFont.CreatePointFont(m_nFontSize * 10, _T("Consolas"));
    m_LineNumFont.CreatePointFont(m_nFontSize * 10, _T("Consolas"));
    m_Font.CreatePointFont(m_nFontSize * 10, _T("Consolas"));

    // 应用到编辑控件
    if (m_Edit.GetSafeHwnd())
    {
        m_Edit.SetFont(&m_EditFont);
    }
}

void CMFCNoteBookView::OnViewZoomIn()
{
    if (m_nFontSize < FONT_SIZE_MAX)
    {
        m_nFontSize += 2;
        CreateEditFont();
        UpdateLineNumberWidth();
    }
}

void CMFCNoteBookView::OnViewZoomOut()
{
    if (m_nFontSize > FONT_SIZE_MIN)
    {
        m_nFontSize -= 2;
        CreateEditFont();
        UpdateLineNumberWidth();
    }
}

void CMFCNoteBookView::OnViewZoomReset()
{
    m_nFontSize = FONT_SIZE_DEFAULT;
    CreateEditFont();
    UpdateLineNumberWidth();
}

BOOL CMFCNoteBookView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // Ctrl + 滚轮 = 缩放
    if (nFlags & MK_CONTROL)
    {
        if (zDelta > 0)
        {
            OnViewZoomIn();
        }
        else
        {
            OnViewZoomOut();
        }
        return TRUE;
    }

    return CView::OnMouseWheel(nFlags, zDelta, pt);
}
