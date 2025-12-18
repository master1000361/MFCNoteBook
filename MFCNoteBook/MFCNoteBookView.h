// MFCNoteBookView.h: CMFCNoteBookView 类的接口
//

#pragma once

#include <vector>
#include <memory>

class CMFCNoteBookDoc;
class CFindReplaceDlg;

class CMFCNoteBookView : public CView
{
protected:
    CMFCNoteBookView() noexcept;
    DECLARE_DYNCREATE(CMFCNoteBookView)

public:
    CMFCNoteBookDoc* GetDocument() const;

protected:
    CEdit m_Edit;
    CFont m_Font;
    int m_nLineNumWidth;

    // ========== 撤销/重做相关 ==========
    std::vector<CString> m_UndoStack;
    std::vector<CString> m_RedoStack;
    CString m_strLastText;
    bool m_bInternalChange;
    static const size_t MAX_UNDO = 50;

    // ========== 查找替换对话框 ==========
    CFindReplaceDlg* m_pFindReplaceDlg;

    // ========== 主题相关 ==========
    CBrush m_brEditBg;  // 编辑区背景画刷

    // ========== 字体相关 ==========
    CFont m_EditFont;           // 编辑区字体
    CFont m_LineNumFont;        // 行号区字体
    int m_nFontSize;            // 当前字号（单位：点）
    static const int FONT_SIZE_MIN = 8;
    static const int FONT_SIZE_MAX = 72;
    static const int FONT_SIZE_DEFAULT = 11;

public:
    CEdit& GetEditCtrl() { return m_Edit; }
    void UpdateLineNumberWidth();
    void ApplyTheme();

public:
    virtual void OnDraw(CDC* pDC);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();

protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

public:
    virtual ~CMFCNoteBookView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnEditChange();
    afx_msg void OnEditScroll();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    // 撤销/重做消息处理
    afx_msg void OnEditUndo();
    afx_msg void OnEditRedo();
    afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);

    // 剪切/复制/粘贴消息处理
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);

    // 查找替换消息处理
    afx_msg void OnEditFind();
    afx_msg void OnEditReplace();

    // 缩放消息处理
    afx_msg void OnViewZoomIn();
    afx_msg void OnViewZoomOut();
    afx_msg void OnViewZoomReset();
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

    DECLARE_MESSAGE_MAP()

public:
    void SyncToDocument();

private:
    void SaveUndoState();
    void CreateEditFont();
};

#ifndef _DEBUG
inline CMFCNoteBookDoc* CMFCNoteBookView::GetDocument() const
{
    return reinterpret_cast<CMFCNoteBookDoc*>(m_pDocument);
}
#endif
