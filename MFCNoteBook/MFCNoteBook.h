// MFCNoteBook.h: MFCNoteBook 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"       // 主符号

// ========== 新增：主题枚举 ==========
enum class AppTheme
{
	Light,
	Dark
};

// 主题颜色结构
struct ThemeColors
{
	COLORREF clrEditBg;         // 编辑区背景
	COLORREF clrEditText;       // 编辑区文字
	COLORREF clrLineNumBg;      // 行号区背景
	COLORREF clrLineNumText;    // 行号文字
	COLORREF clrLineNumBorder;  // 行号分隔线
};
// ====================================

// CMFCNoteBookApp:
// 有关此类的实现，请参阅 MFCNoteBook.cpp
//

class CMFCNoteBookApp : public CWinApp
{
public:
	CMFCNoteBookApp() noexcept;

	// ========== 新增：主题相关 ==========
private:
	AppTheme m_currentTheme;
	ThemeColors m_lightTheme;
	ThemeColors m_darkTheme;

public:
	AppTheme GetCurrentTheme() const { return m_currentTheme; }
	void SetTheme(AppTheme theme);
	const ThemeColors& GetThemeColors() const;
	void SaveThemeToRegistry();
	void LoadThemeFromRegistry();
	// ====================================

	// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	// 实现
	afx_msg void OnAppAbout();

	// ========== 新增：主题菜单处理 ==========
	afx_msg void OnViewThemeLight();
	afx_msg void OnViewThemeDark();
	afx_msg void OnUpdateViewThemeLight(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewThemeDark(CCmdUI* pCmdUI);
	// ========================================

	DECLARE_MESSAGE_MAP()
};

extern CMFCNoteBookApp theApp;
