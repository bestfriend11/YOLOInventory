// Dialog_Create_Background.cpp : implementation file
//

#include "GUIMasterShell.h"
#include "stdafx.h"
#include "guimaster.h"
#include "Dialog_Create_Background.h"
#include "ui_shell.h"
#include "ui_textureman.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Create_Background dialog


Dialog_Create_Background::Dialog_Create_Background(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Create_Background::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Create_Background)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Create_Background::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Create_Background)
	DDX_Control(pDX, IDC_EDIT_BACKGROUND, m_editBg);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Create_Background, CDialog)
	//{{AFX_MSG_MAP(Dialog_Create_Background)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Create_Background message handlers

void Dialog_Create_Background::OnOK() 
{
	CString rText;
	m_editBg.GetWindowText( rText );
	unsigned int texture = gUIShell.GetTextureman()->LoadTexture( rText.GetBuffer( 0 ) );
	gGUIMaster.SetBackgroundTexture( texture );	
	
	CDialog::OnOK();
}
