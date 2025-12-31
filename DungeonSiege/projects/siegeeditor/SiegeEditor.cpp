// SiegeEditor.cpp : Defines the class behaviors for the application.
//

#include "PrecompEditor.h"
#include "MenuCommands.h"
#include "afxpriv.h"
#include "SiegeEditor.h"
#include "EditorRegion.h"
#include "MainFrm.h"
#include "SiegeEditorDoc.h"
#include "LeftView.h"
#include "BottomView.h"
#include "Preferences.h"
#include "Services.h"
#include "SiegeEditorView.h"
#include "ObjectPropertySheet.h"
#include "Config.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "Splash.h"

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorApp

BEGIN_MESSAGE_MAP(CSiegeEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CSiegeEditorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE7,MRUFileHandler)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorApp construction

CSiegeEditorApp::CSiegeEditorApp()
{
	// Place all significant initialization in InitInstance
}

CSiegeEditorApp::~CSiegeEditorApp()
{
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CSiegeEditorApp object

CSiegeEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorApp initialization

BOOL CSiegeEditorApp::InitInstance()
{
	// CG: The following block was added by the Splash Screen component.
\
	{
\
		CCommandLineInfo cmdInfo;
\
		ParseCommandLine(cmdInfo);
\

\
		CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
\
	}
	// Create a new test shell object
	m_pShell = new SiegeEditorShell;

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
	SetRegistryKey(_T(COMPANY_NAME));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CSiegeEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CLeftView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	/*
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	*/
	AfxGetApp()->OnCmdMsg(ID_FILE_NEW, 0, NULL, NULL);
	if ( m_pMainWnd == NULL )  return FALSE;
		
	gEditor.SetMainHWnd( m_pMainWnd->GetSafeHwnd() );

	// Initialize saved user settings
	if ( gPreferences.Load() )
	{
		// The one and only window has been initialized, so show and update it.
		m_pMainWnd->ShowWindow(SW_SHOW);		
		m_pMainWnd->UpdateWindow();
	}	
	
	gpstring sRegion = gConfig.GetString( "region" );
	gpstring sMap = gConfig.GetString( "map" );
	if ( !sRegion.empty() && !sMap.empty() )
	{
		gEditorRegion.LoadRegion( sRegion, sMap );
	}

	return TRUE;
}

void CSiegeEditorApp::ShowMaximized()
{
	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
	m_pMainWnd->UpdateWindow();
}


BOOL CSiegeEditorApp::OnIdle(LONG lCount)
{
	BOOL rc = CWinApp::OnIdle( lCount );

	if ( Singleton <SiegeEditorShell>::DoesSingletonExist() && gEditor.IsAppActive() )
	{
		if ( !gEditor.Run() )
		{
			PostQuitMessage( -1 );
		}
		rc = TRUE;
	}

	return ( rc );
}

int CSiegeEditorApp::ExitInstance()
{
	delete ( m_pShell );
	m_pShell = NULL;

	return CWinApp::ExitInstance();
}

BOOL CSiegeEditorApp::ProcessMessageFilter(int code, LPMSG lpMsg)
{
	if ( code < 0 ) {
		CWinApp::ProcessMessageFilter(code, lpMsg);
	}

	return CWinApp::ProcessMessageFilter( code, lpMsg );
}

BOOL CSiegeEditorApp::PreTranslateMessage(MSG* pMsg)
{	
	if ( Singleton <SiegeEditorShell>::DoesSingletonExist() ) {
		if ( m_pMainWnd->GetSafeHwnd() && gEditor.GetAccelerator() ) {
			if ( GetFocus() == gEditor.GetHWnd() ) {
				if ( ::TranslateAccelerator( m_pMainWnd->GetSafeHwnd(), gEditor.GetAccelerator(), pMsg ) ) {				
					return TRUE;
				}
			}
		}
	}

	// CG: The following line was added by the Splash Screen component.

	CSplashWnd::PreTranslateAppMessage(pMsg);
	return CWinApp::PreTranslateMessage(pMsg);
}


void CSiegeEditorApp::MRUFileHandler( UINT i )
{
	int nIndex = i - ID_FILE_MRU_FILE1;

	gpstring sLoadName = (*m_pRecentFileList)[nIndex];
	gpstring sLast;
	stringtool::GetBackDelimitedString( sLoadName, '\\', sLast );
	gpstring sRegion;
	gpstring sMap;
	stringtool::GetDelimitedValue( sLast, '-', 0, sMap );
	stringtool::GetDelimitedValue( sLast, '-', 1, sRegion );
	if ( gEditorRegion.LoadRegion( sRegion, sMap ) == false ) {
		m_pRecentFileList->Remove( nIndex );
	}
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
	CStatic	m_version;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
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
	DDX_Control(pDX, IDC_STATIC_VERSION, m_version);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSiegeEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorApp message handlers

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_version.SetWindowText( "Version " + SysInfo::GetExeFileVersionText( gpversion::MODE_AUTO_LONG ) );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

