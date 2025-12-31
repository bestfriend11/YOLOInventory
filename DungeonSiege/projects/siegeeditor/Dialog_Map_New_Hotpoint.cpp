// Dialog_Map_New_Hotpoint.cpp : implementation file
//
#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Map_New_Hotpoint.h"
#include "SiegeEditorTypes.h"
#include "EditorRegion.h"
#include "EditorMap.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_New_Hotpoint dialog


Dialog_Map_New_Hotpoint::Dialog_Map_New_Hotpoint(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Map_New_Hotpoint::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Map_New_Hotpoint)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Map_New_Hotpoint::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Map_New_Hotpoint)
	DDX_Control(pDX, IDC_EDIT_NAME, m_edit_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Map_New_Hotpoint, CDialog)
	//{{AFX_MSG_MAP(Dialog_Map_New_Hotpoint)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_New_Hotpoint message handlers

void Dialog_Map_New_Hotpoint::OnOK() 
{
	CString rText;
	m_edit_name.GetWindowText( rText );

	FuelHandle hMapDir = gEditorRegion.GetMapHandle();
	FuelHandle hHotpoints = hMapDir->GetChildBlock( "hotpoints" );
	FuelHandleList hlHotpoints = hHotpoints->ListChildBlocks( 1, "hotpoint" );	
	FuelHandleList::iterator i;
	gEditorMap.SetNewHotpointName( rText.GetBuffer( 0 ) );
	for ( i = hlHotpoints.begin(); i != hlHotpoints.end(); ++i )
	{
		if ( gEditorMap.GetNewHotpointName().same_no_case( (*i)->GetName() ) )
		{
			MessageBox( "A hotpoint with that name already exists, please select another name.", "Hotpoint Error", MB_OK );
			return;
		}
	}
	
	CDialog::OnOK();
}

BOOL Dialog_Map_New_Hotpoint::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Map_New_Hotpoint::OnCancel() 
{
	gEditorMap.SetNewHotpointName( "" );
	
	CDialog::OnCancel();
}
