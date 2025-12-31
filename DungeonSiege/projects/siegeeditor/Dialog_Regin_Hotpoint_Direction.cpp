// Dialog_Regin_Hotpoint_Direction.cpp : implementation file
//

#include "stdafx.h"
#include "siegeeditor.h"
#include "Dialog_Regin_Hotpoint_Direction.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Regin_Hotpoint_Direction dialog


Dialog_Regin_Hotpoint_Direction::Dialog_Regin_Hotpoint_Direction(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Regin_Hotpoint_Direction::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Regin_Hotpoint_Direction)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Regin_Hotpoint_Direction::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Regin_Hotpoint_Direction)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Regin_Hotpoint_Direction, CDialog)
	//{{AFX_MSG_MAP(Dialog_Regin_Hotpoint_Direction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Regin_Hotpoint_Direction message handlers

void Dialog_Regin_Hotpoint_Direction::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}
