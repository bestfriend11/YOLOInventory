// Dialog_Movment_Locking.cpp : implementation file
//

#include "stdafx.h"
#include "siegeeditor.h"
#include "Dialog_Movment_Locking.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Movment_Locking dialog


Dialog_Movment_Locking::Dialog_Movment_Locking(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Movment_Locking::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Movment_Locking)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Movment_Locking::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Movment_Locking)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Movment_Locking, CDialog)
	//{{AFX_MSG_MAP(Dialog_Movment_Locking)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Movment_Locking message handlers
