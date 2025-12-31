// Dialog_Stitch_Builder.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Stitch_Builder.h"
#include "Stitch_Helper.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Builder dialog


Dialog_Stitch_Builder::Dialog_Stitch_Builder(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Stitch_Builder::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Stitch_Builder)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Stitch_Builder::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Stitch_Builder)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Stitch_Builder, CDialog)
	//{{AFX_MSG_MAP(Dialog_Stitch_Builder)
	ON_BN_CLICKED(IDC_BUTTON_BUILD, OnButtonBuild)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Builder message handlers

BOOL Dialog_Stitch_Builder::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hMaps( "world:maps" );
	FuelHandleList hlMaps = hMaps->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hlMaps.begin(); i != hlMaps.end(); ++i ) 
	{
		((CListBox *)GetDlgItem( IDC_LIST_MAPS ))->AddString( (*i)->GetName() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Stitch_Builder::OnButtonBuild() 
{
	int sel = ((CListBox *)GetDlgItem( IDC_LIST_MAPS ))->GetCurSel();
	CString rMap;
	CListBox * pList = ((CListBox *)GetDlgItem( IDC_LIST_MAPS ));
	if ( sel != LB_ERR )
	{
		pList->GetText( sel, rMap );	
		gStitchHelper.BuildStitchIndex( rMap.GetBuffer( rMap.GetLength() ) );	
	}
	else
	{
		MessageBoxEx( gEditor.GetHWnd(), "Please select a map to stitch.", "Stitch Builder", MB_OK, LANG_ENGLISH );
	}
}

void Dialog_Stitch_Builder::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
