// Dialog_Custom_View_Rename.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Custom_View_Rename.h"
#include "LeftView.h"
#include "CustomViewer.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Custom_View_Rename dialog


Dialog_Custom_View_Rename::Dialog_Custom_View_Rename(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Custom_View_Rename::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Custom_View_Rename)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Custom_View_Rename::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Custom_View_Rename)
	DDX_Control(pDX, IDC_EDIT_CUSTOM_VIEW, m_Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Custom_View_Rename, CDialog)
	//{{AFX_MSG_MAP(Dialog_Custom_View_Rename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Custom_View_Rename message handlers

BOOL Dialog_Custom_View_Rename::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CTreeCtrl & wndCustom = gLeftView.GetCustomView();
	CString rItem = wndCustom.GetItemText( wndCustom.GetSelectedItem() );

	m_Name.SetWindowText( rItem.GetBuffer( 0 ) );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Custom_View_Rename::OnOK() 
{
	CString rName;
	m_Name.GetWindowText( rName );
	if ( rName.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please enter a name.", "Rename Custom View", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	CTreeCtrl & wndCustom = gLeftView.GetCustomView();
	wndCustom.SetItemText( wndCustom.GetSelectedItem(), rName );
	
	CDialog::OnOK();
}
