// Network_transport_testDlg.cpp : implementation file
//

#include "stdafx.h"

#include "gpcore.h"

#include "Network_transport_test.h"
#include "Network_transport_testDlg.h"

#ifdef _DEBUG
#undef new	// added -bk
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetwork_transport_testDlg dialog

CNetwork_transport_testDlg::CNetwork_transport_testDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNetwork_transport_testDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNetwork_transport_testDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	mNetPipe = 0;
}

void CNetwork_transport_testDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetwork_transport_testDlg)
	DDX_Control(pDX, IDC_NEWCONNECTIONNAME, m_NewConnectionName);
	DDX_Control(pDX, IDC_CONNECTIONSCOMBO, m_ConnectionsCombo);
	DDX_Control(pDX, IDC_SESSIONSCOMBO, m_SessionsCombo);
	DDX_Control(pDX, IDC_PROVIDERLIST, m_ProviderList);
	DDX_Control(pDX, IDC_NEWSESSIONNAME, m_NewSessionName);
	DDX_Control(pDX, IDC_LISTSESSIONS, m_ListSessions);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNetwork_transport_testDlg, CDialog)
	//{{AFX_MSG_MAP(CNetwork_transport_testDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, OnExit)
	ON_BN_CLICKED(IDC_LISTSESSIONS, OnListSessions)
	ON_EN_CHANGE(IDC_NEWSESSIONNAME, OnChangeNewSessionName)
	ON_BN_CLICKED(IDC_LISTPROVIDERS, OnListproviders)
	ON_CBN_SELCHANGE(IDC_PROVIDERLIST, OnSelchangeProviderlist)
	ON_BN_CLICKED(IDC_RESETNETWORKTRANSPORT, OnResetnetworktransport)
	ON_BN_CLICKED(IDC_CREATECONNECTION, OnCreateconnection)
	ON_EN_CHANGE(IDC_NEWCONNECTIONNAME, OnChangeNewconnectionname)
	ON_BN_CLICKED(IDC_INITPROVIDER, OnInitprovider)
	ON_BN_CLICKED(IDC_OPENSESSION, OnOpensession)
	ON_CBN_SELCHANGE(IDC_SESSIONSCOMBO, OnSelchangeSessionscombo)
	ON_BN_CLICKED(IDC_CREATEANDOPENSESSION, OnCreateandopensession)
	ON_BN_CLICKED(IDC_LISTCONNECTIONS, OnListconnections)
	ON_BN_CLICKED(IDC_CLOSECONNECTION, OnCloseconnection)
	ON_BN_CLICKED(IDEXIT, OnExit)
	ON_CBN_EDITCHANGE(IDC_CONNECTIONSCOMBO, OnEditchangeConnectionscombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




/////////////////////////////////////////////////////////////////////////////
// CNetwork_transport_testDlg message handlers

BOOL CNetwork_transport_testDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	mNetPipe = new NetPipe;
	m_NewSessionName.SetWindowText( "uber session" );
	return TRUE;  // return TRUE  unless you set the focus to a control
}




// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CNetwork_transport_testDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}




// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CNetwork_transport_testDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}




void CNetwork_transport_testDlg::OnExit() 
{
	// TODO: Add your control notification handler code here
	if( mNetPipe ) {
		delete mNetPipe;
	}
	
}

void CNetwork_transport_testDlg::OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}




void CNetwork_transport_testDlg::OnListSessions() 
{
	NetPipeSessionColl sessionlist;
	mNetPipe->ListSessions( sessionlist );

	m_SessionsCombo.ResetContent();

	NetPipeSessionColl::iterator i;
	for( i=sessionlist.begin(); i<sessionlist.end(); ++i ) {
		CString name = (*i).GetName().c_str();
		int result = m_SessionsCombo.AddString( name );
		gpassert( (result != CB_ERR) && (result != CB_ERRSPACE) );
	}
}




void CNetwork_transport_testDlg::OnChangeNewSessionName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here

}




void CNetwork_transport_testDlg::OnListproviders() 
{
	NetPipeProviderColl providerlist;
	mNetPipe->ListTransportProviders( providerlist );

	m_ProviderList.ResetContent();

	NetPipeProviderColl::iterator i;
	for( i=providerlist.begin(); i<providerlist.end(); ++i ) {
		CString name = (*i).GetName().c_str();
		int result = m_ProviderList.AddString( name );
		gpassert( (result != CB_ERR) && (result != CB_ERRSPACE) );
	}
}




void CNetwork_transport_testDlg::OnSelchangeProviderlist()
{
}




void CNetwork_transport_testDlg::OnResetnetworktransport() 
{
	mNetPipe->Reset();
}




void CNetwork_transport_testDlg::OnCreateconnection() 
{
	NetPipeConnection connection;

	CString name;

	m_NewConnectionName.GetWindowText( name );
	LPTSTR pstring = name.GetBuffer(1);
	connection.SetName( std::string( pstring ) );

	bool result = mNetPipe->GetOpenSession().OpenConnection( connection );
}




void CNetwork_transport_testDlg::OnChangeNewconnectionname() 
{
}




void CNetwork_transport_testDlg::OnInitprovider() 
{
	CString name;
	m_ProviderList.GetLBText( m_ProviderList.GetCurSel(), name );
	std::string provider = name;

	NetPipeProviderColl providerlist;
	mNetPipe->ListTransportProviders( providerlist );

	NetPipeProviderColl::iterator i;
	bool found = false;
	for( i=providerlist.begin(); i<providerlist.end(); ++i ) {
		if( name == (*i).GetName().c_str() ) {
		found = true;
		break;
		}
	}

	// now initialize the provider

	mNetPipe->SetTransportProvider( *i );
}




void CNetwork_transport_testDlg::OnOpensession() 
{
	CString name;
	m_SessionsCombo.GetLBText( m_SessionsCombo.GetCurSel(), name );
	std::string session = name;

	NetPipeSessionColl sessionlist;
	mNetPipe->ListSessions( sessionlist );

	NetPipeSessionColl::iterator i;
	bool found = false;
	for( i=sessionlist.begin(); i<sessionlist.end(); ++i ) {
		if( name == (*i).GetName().c_str() ) {
		found = true;
		break;
		}
	}

	// now initialize the provider

	mNetPipe->OpenSession( *i );
}




void CNetwork_transport_testDlg::OnSelchangeSessionscombo() 
{
	// TODO: Add your control notification handler code here
}




void CNetwork_transport_testDlg::OnCreateandopensession() 
{
	NetPipeSession session;

	CString name;

	m_NewSessionName.GetWindowText( name );
	LPTSTR pstring = name.GetBuffer(1);

	session.SetName( std::string( pstring ) );
	session.SetNumConnectionsMax( 7 );

	bool result = mNetPipe->CreateAndOpenSession( session );
}




void CNetwork_transport_testDlg::OnListconnections() 
{
	NetPipeConnectionColl connectionlist;
	mNetPipe->GetOpenSession().ListConnections( connectionlist );

	m_ConnectionsCombo.ResetContent();

	NetPipeConnectionColl::iterator i;
	for( i=connectionlist.begin(); i<connectionlist.end(); ++i ) {
		CString name = (*i).GetName().c_str();
		int result = m_ConnectionsCombo.AddString( name );
		gpassert( (result != CB_ERR) && (result != CB_ERRSPACE) );
	}
}




void CNetwork_transport_testDlg::OnCloseconnection() 
{
	CString name;
	m_ConnectionsCombo.GetLBText( m_ConnectionsCombo.GetCurSel(), name );
	std::string session = name;

	NetPipeConnectionColl connectionlist;
	mNetPipe->GetOpenSession().ListConnections( connectionlist );

	NetPipeConnectionColl::iterator i;
	bool found = false;
	for( i=connectionlist.begin(); i<connectionlist.end(); ++i ) {
		if( name == (*i).GetName().c_str() ) {
		found = true;
		break;
		}
	}

	// now initialize the provider

	mNetPipe->GetOpenSession().CloseConnection( *i );
}




void CNetwork_transport_testDlg::OnEditchangeConnectionscombo() 
{
	// TODO: Add your control notification handler code here
	
}
