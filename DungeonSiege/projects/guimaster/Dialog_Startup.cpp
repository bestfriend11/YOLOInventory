// Dialog_Startup.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "Dialog_Startup.h"
#include "Dialog_Create_Interface.h"
#include "Dialog_Load_Interface.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialog_Startup dialog


CDialog_Startup::CDialog_Startup(CWnd* pParent /*=NULL*/)
	: CDialog(CDialog_Startup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialog_Startup)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialog_Startup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialog_Startup)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialog_Startup, CDialog)
	//{{AFX_MSG_MAP(CDialog_Startup)
	ON_BN_CLICKED(IDC_BUTTON_CREATE, OnButtonCreate)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, OnButtonLoad)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialog_Startup message handlers

void CDialog_Startup::OnButtonCreate() 
{	
	CDialog_Create_Interface dlgCreate;
	if ( dlgCreate.DoModal() == IDOK )
	{
		CDialog::OnOK();
	}
}

void CDialog_Startup::OnButtonLoad() 
{
	CDialog_Load_Interface dlgLoad;
	if ( dlgLoad.DoModal() == IDOK )
	{
		CDialog::OnOK();
	}
}
