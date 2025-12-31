// GUIMaster.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "GUIMaster.h"

#include "MainFrm.h"
#include "GUIMasterDoc.h"
#include "GUIMasterView.h"
#include "Dialog_Startup.h"

#include "Menu.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterApp

BEGIN_MESSAGE_MAP(CGUIMasterApp, CWinApp)
	//{{AFX_MSG_MAP(CGUIMasterApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND(ID_VIEW_CONTROLS, MenuFunctions::Controls)
	ON_COMMAND(ID_MODE_CREATION, MenuFunctions::CreationMode)
	ON_COMMAND(ID_MODE_STRUCTURE, MenuFunctions::StructureMode)
	ON_COMMAND(ID_MODE_UVSMARTMAP, MenuFunctions::UVSmartMapMode)
	ON_COMMAND(ID_MODE_UVEDIT, MenuFunctions::UVEditMode)
	ON_COMMAND(ID_MODE_GUIDEEDIT, MenuFunctions::GuideEditMode)
	ON_COMMAND(ID_VIEW_PREFERENCES, MenuFunctions::Preferences)
	ON_COMMAND(ID_FILE_OPEN_INTERFACE, MenuFunctions::LoadInterface)
	ON_COMMAND(ID_FILE_NEW_INTERFACE, MenuFunctions::CreateInterface)
	ON_COMMAND(ID_FILE_SAVE_INTERFACE, MenuFunctions::SaveInterface)
	ON_COMMAND(ID_FILE_CLOSEINTERFACES, MenuFunctions::CloseInterfaces)
	ON_COMMAND(ID_VIEW_INTERFACES, MenuFunctions::ShowInterfaces)
	ON_COMMAND(ID_EDIT_CUT, MenuFunctions::Cut)
	ON_COMMAND(ID_EDIT_COPY, MenuFunctions::Copy)
	ON_COMMAND(ID_EDIT_PASTE, MenuFunctions::Paste)
	ON_COMMAND(ID_EDIT_DELETE, MenuFunctions::Delete)
	ON_COMMAND(ID_EDIT_LEFT, MenuFunctions::Left)
	ON_COMMAND(ID_EDIT_RIGHT, MenuFunctions::Right)
	ON_COMMAND(ID_EDIT_UP, MenuFunctions::Up)
	ON_COMMAND(ID_EDIT_DOWN, MenuFunctions::Down)
	ON_COMMAND(ID_WINDOW_PROPERTIES, MenuFunctions::Properties)
	ON_COMMAND(ID_WINDOW_BRINGTOFRONT, MenuFunctions::BringToFront)
	ON_COMMAND(ID_WINDOW_MESSAGES, MenuFunctions::Messages)
	ON_COMMAND(ID_WINDOW_COMMON_SETTINGS, MenuFunctions::CommonSettings)
	ON_COMMAND(ID_LOCALIZATIONTOOLS_CREATEGUIIDMAP, MenuFunctions::CreateGuiIdMap)
	ON_COMMAND(ID_EDIT_SET_BACKGROUND,MenuFunctions::SetBackground)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterApp construction

CGUIMasterApp::CGUIMasterApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

CGUIMasterApp::~CGUIMasterApp()
{
	delete m_pShell;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGUIMasterApp object

CGUIMasterApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterApp initialization

BOOL CGUIMasterApp::InitInstance()
{
	m_pShell = new GUIMasterShell();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CGUIMasterDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CGUIMasterView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;	

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();	

	gGUIMaster.SetHWnd( m_pMainWnd->GetSafeHwnd() );

	CDialog_Startup dlgStartup;
	dlgStartup.DoModal();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CGUIMasterApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterApp message handlers


BOOL CGUIMasterApp::OnIdle(LONG lCount) 
{
	if ( GUIMasterShell::DoesSingletonExist() )
	{
		gGUIMaster.Draw();
		gGUIMaster.Update( 0.0f );
	}
	
	return CWinApp::OnIdle(lCount);
}
