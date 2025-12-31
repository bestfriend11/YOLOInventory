// Dialog_Progress_Load.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Progress_Load.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Progress_Load dialog


Dialog_Progress_Load::Dialog_Progress_Load(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Progress_Load::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Progress_Load)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Progress_Load::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Progress_Load)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Progress_Load, CDialog)
	//{{AFX_MSG_MAP(Dialog_Progress_Load)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Progress_Load message handlers
