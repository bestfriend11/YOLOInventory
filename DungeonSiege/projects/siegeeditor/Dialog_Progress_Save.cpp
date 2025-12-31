// Dialog_Progress_Save.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Progress_Save.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Progress_Save dialog


Dialog_Progress_Save::Dialog_Progress_Save(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Progress_Save::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Progress_Save)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Progress_Save::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Progress_Save)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Progress_Save, CDialog)
	//{{AFX_MSG_MAP(Dialog_Progress_Save)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Progress_Save message handlers
