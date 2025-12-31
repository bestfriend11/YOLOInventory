// Dialog_Save_Custom_View.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Save_Custom_View.h"
#include "Preferences.h"
#include "FileSys.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Custom_View dialog


Dialog_Save_Custom_View::Dialog_Save_Custom_View(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Save_Custom_View::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Save_Custom_View)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Save_Custom_View::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Save_Custom_View)
	DDX_Control(pDX, IDC_EDIT_CUSTOM_VIEW, m_customView);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Save_Custom_View, CDialog)
	//{{AFX_MSG_MAP(Dialog_Save_Custom_View)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Custom_View message handlers

void Dialog_Save_Custom_View::OnOK() 
{
	CString rView;
	m_customView.GetWindowText( rView );
	if ( rView.IsEmpty() == false )
	{
		if( !FileSys::IsValidFileName( rView.GetBuffer( 0 ), false, false ) )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Invalid Region Name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;			
		}
		else
		{
			gPreferences.SetCustomViewSaveName( rView.GetBuffer( 0 ) );
		}
	}
	else
	{
		return;
	}
	
	CDialog::OnOK();
}

void Dialog_Save_Custom_View::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
