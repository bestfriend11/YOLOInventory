// PropPage_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "PropPage_Properties.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties dialog


PropPage_Properties::PropPage_Properties(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Properties::IDD, IDS_STRING61209)
{
	//{{AFX_DATA_INIT(PropPage_Properties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PropPage_Properties::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Properties)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties message handlers
