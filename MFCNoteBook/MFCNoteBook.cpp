// MFCNoteBook.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "MFCNoteBook.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "MFCNoteBookDoc.h"
#include "MFCNoteBookView.h"
#include "ConfigManager.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMFCNoteBookApp

BEGIN_MESSAGE_MAP(CMFCNoteBookApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CMFCNoteBookApp::OnAppAbout)
	// 基于文件的标准文档命令
	ON_COMMAND(ID_FILE_NEW, &CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinApp::OnFileOpen)
	// 标准打印设置命令
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinApp::OnFilePrintSetup)

	// ========== 新增：主题切换 ==========
	ON_COMMAND(ID_VIEW_THEME_LIGHT, &CMFCNoteBookApp::OnViewThemeLight)
	ON_COMMAND(ID_VIEW_THEME_DARK, &CMFCNoteBookApp::OnViewThemeDark)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THEME_LIGHT, &CMFCNoteBookApp::OnUpdateViewThemeLight)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THEME_DARK, &CMFCNoteBookApp::OnUpdateViewThemeDark)
	// ====================================
END_MESSAGE_MAP()


// CMFCNoteBookApp 构造

CMFCNoteBookApp::CMFCNoteBookApp() noexcept
	: m_currentTheme(AppTheme::Light)  // 默认亮色主题
{
	// ========== 新增：初始化主题颜色 ==========
	// 亮色主题
	m_lightTheme.clrEditBg = RGB(255, 255, 255);        // 白色背景
	m_lightTheme.clrEditText = RGB(0, 0, 0);            // 黑色文字
	m_lightTheme.clrLineNumBg = RGB(240, 240, 240);     // 浅灰背景
	m_lightTheme.clrLineNumText = RGB(128, 128, 128);   // 灰色行号
	m_lightTheme.clrLineNumBorder = RGB(200, 200, 200); // 分隔线

	// 暗色主题
	m_darkTheme.clrEditBg = RGB(30, 30, 30);            // 深灰背景
	m_darkTheme.clrEditText = RGB(220, 220, 220);       // 浅色文字
	m_darkTheme.clrLineNumBg = RGB(45, 45, 45);         // 稍浅的背景
	m_darkTheme.clrLineNumText = RGB(140, 140, 140);    // 灰色行号
	m_darkTheme.clrLineNumBorder = RGB(60, 60, 60);     // 分隔线
	// ==========================================

	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	SetAppID(_T("MFCNoteBook.AppID.NoVersion"));
}

// 唯一的 CMFCNoteBookApp 对象

CMFCNoteBookApp theApp;


// CMFCNoteBookApp 初始化

BOOL CMFCNoteBookApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));
	LoadStdProfileSettings(4);

	// ========== 新增：加载配置文件 ==========
	CConfigManager& config = CConfigManager::GetInstance();
	if (!config.LoadConfig())
	{
		// 配置加载失败，退出程序
		AfxMessageBox(_T("配置文件加载失败，程序将退出。\n\n请确保 config.ini 文件存在且格式正确。"),
			MB_ICONERROR);
		return FALSE;
	}
	// ========================================

	// ========== 加载保存的主题设置 ==========
	LoadThemeFromRegistry();
	// ========================================

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_MFCNoteBookTYPE,
		RUNTIME_CLASS(CMFCNoteBookDoc),
		RUNTIME_CLASS(CChildFrame),
		RUNTIME_CLASS(CMFCNoteBookView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;

	m_pMainWnd->DragAcceptFiles();

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CMFCNoteBookApp::ExitInstance()
{
	// ========== 新增：退出时保存主题设置 ==========
	SaveThemeToRegistry();
	// =============================================

	AfxOleTerm(FALSE);
	return CWinApp::ExitInstance();
}

// ========== 新增：主题相关实现 ==========

const ThemeColors& CMFCNoteBookApp::GetThemeColors() const
{
	return (m_currentTheme == AppTheme::Dark) ? m_darkTheme : m_lightTheme;
}

void CMFCNoteBookApp::SetTheme(AppTheme theme)
{
	if (m_currentTheme == theme)
		return;

	m_currentTheme = theme;

	// 通知所有视图更新
	POSITION posTemplate = GetFirstDocTemplatePosition();
	while (posTemplate)
	{
		CDocTemplate* pTemplate = GetNextDocTemplate(posTemplate);
		POSITION posDoc = pTemplate->GetFirstDocPosition();
		while (posDoc)
		{
			CDocument* pDoc = pTemplate->GetNextDoc(posDoc);
			POSITION posView = pDoc->GetFirstViewPosition();
			while (posView)
			{
				CView* pView = pDoc->GetNextView(posView);
				if (pView && pView->IsKindOf(RUNTIME_CLASS(CMFCNoteBookView)))
				{
					// 通知视图应用新主题
					CMFCNoteBookView* pNoteView = (CMFCNoteBookView*)pView;
					pNoteView->ApplyTheme();
				}
			}
		}
	}
}

void CMFCNoteBookApp::SaveThemeToRegistry()
{
	WriteProfileInt(_T("Settings"), _T("Theme"), (int)m_currentTheme);
}

void CMFCNoteBookApp::LoadThemeFromRegistry()
{
	int nTheme = GetProfileInt(_T("Settings"), _T("Theme"), 0);
	m_currentTheme = (nTheme == 1) ? AppTheme::Dark : AppTheme::Light;
}

void CMFCNoteBookApp::OnViewThemeLight()
{
	SetTheme(AppTheme::Light);
}

void CMFCNoteBookApp::OnViewThemeDark()
{
	SetTheme(AppTheme::Dark);
}

void CMFCNoteBookApp::OnUpdateViewThemeLight(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_currentTheme == AppTheme::Light);
}

void CMFCNoteBookApp::OnUpdateViewThemeDark(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_currentTheme == AppTheme::Dark);
}
// ========================================


// CAboutDlg 对话框（保持不变）

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

void CMFCNoteBookApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
