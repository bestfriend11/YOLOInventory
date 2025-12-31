// Dialog_Load_Custom_View.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Load_Custom_View.h"
#include "CustomViewer.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Load_Custom_View dialog


Dialog_Load_Custom_View::Dialog_Load_Custom_View(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Load_Custom_View::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Load_Custom_View)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Load_Custom_View::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Load_Custom_View)
	DDX_Control(pDX, IDC_LIST_CUSTOM_VIEWS, m_customViews);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Load_Custom_View, CDialog)
	//{{AFX_MSG_MAP(Dialog_Load_Custom_View)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Load_Custom_View message handlers

BOOL Dialog_Load_Custom_View::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	StringVec customViews;
	StringVec::iterator i;
	gCustomViewer.GetCustomViews( customViews );
	for ( i = customViews.begin(); i != customViews.end(); ++i )
	{
		m_customViews.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Load_Custom_View::OnOK() 
{
	if ( m_customViews.GetCurSel() == LB_ERR )
	{
		return;
	}

	CString rView;
	m_customViews.GetText( m_customViews.GetCurSel(), rView );		
	
	CDialog::OnOK();
}
