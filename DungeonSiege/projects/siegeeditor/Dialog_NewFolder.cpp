// Dialog_NewFolder.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_NewFolder.h"
#include "CustomViewer.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_NewFolder dialog


Dialog_NewFolder::Dialog_NewFolder(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_NewFolder::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_NewFolder)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_NewFolder::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_NewFolder)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_NewFolder, CDialog)
	//{{AFX_MSG_MAP(Dialog_NewFolder)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_NewFolder message handlers

void Dialog_NewFolder::OnOK() 
{
	CString rEdit;
	((CEdit *)GetDlgItem( IDC_EDIT_FOLDERNAME ))->GetWindowText( rEdit );

	if ( rEdit == "" ) {
		MessageBox( "Please enter a name for the new folder.", "New Folder", MB_OK );
		return;
	}

	gCustomViewer.SetNewFolderName( rEdit.GetBuffer( rEdit.GetLength() ) );	
	CDialog::OnOK();
}

void Dialog_NewFolder::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
