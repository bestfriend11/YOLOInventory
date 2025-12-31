// PropPage_Node_Stitching.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "PropPage_Node_Stitching.h"
#include "Stitch_Helper.h"
#include "EditorTerrain.h"
#include "EditorRegion.h"
#include "Stitch_Helper.h"
#include "Dialog_Stitch_Manager.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Stitching dialog


PropPage_Node_Stitching::PropPage_Node_Stitching(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Node_Stitching::IDD, IDS_STRING61206)
	, m_stitch_id( 0 )
{
	//{{AFX_DATA_INIT(PropPage_Node_Stitching)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PropPage_Node_Stitching::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Node_Stitching)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Node_Stitching, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Node_Stitching)
	ON_WM_CREATE()
	ON_EN_CHANGE(IDC_STITCH_ORDER, OnChangeStitchOrder)
	ON_CBN_SELCHANGE(IDC_STITCH_REGION, OnEditchangeStitchRegion)
	ON_BN_CLICKED(IDC_BUTTON_STITCH_MANAGER, OnButtonStitchManager)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Stitching message handlers

int PropPage_Node_Stitching::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;	
	
	return 0;
}

void PropPage_Node_Stitching::OnChangeStitchOrder() 
{
	CString rValue;
	GetDlgItem( IDC_STITCH_ORDER )->GetWindowText( rValue );
	gpstring sValue = rValue.GetBuffer( 0 );
	stringtool::Get( sValue, m_stitch_id );
}

void PropPage_Node_Stitching::OnEditchangeStitchRegion() 
{
	CComboBox * pCombo = (CComboBox *)GetDlgItem( IDC_STITCH_REGION );
	int sel = pCombo->GetCurSel();
	CString rValue;
	pCombo->GetLBText( sel, rValue );
	m_sRegion = rValue;
}


void PropPage_Node_Stitching::SetNodeStitching()
{	
	if ( m_stitch_id != 0 ) {
		gStitchHelper.SetNodeDoorStitchHelperInfo(  gEditorTerrain.GetSelectedNode(),
													m_stitch_id,
													gEditorTerrain.GetSelectedSourceDoorID(),
													m_sRegion );
	}				
}

BOOL PropPage_Node_Stitching::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	GetDlgItem( IDC_NODE_GUID )->SetWindowText( gEditorTerrain.GetSelectedNode().ToString().c_str() );
	gpstring sTemp;
	sTemp.assignf( "%d", gEditorTerrain.GetSelectedSourceDoorID() );
	GetDlgItem( IDC_DOOR_ID )->SetWindowText( sTemp.c_str() );
	CComboBox * pCombo = (CComboBox *)GetDlgItem( IDC_STITCH_REGION );

	StringVec region_list;
	StringVec::iterator j;
	gEditorRegion.GetRegionList( gEditorRegion.GetMapName().c_str(), region_list );
	for ( j = region_list.begin(); j != region_list.end(); ++j ) 
	{
		pCombo->AddString( (*j).c_str() );
	}

	int			nodeID = 0;
	gpstring sRegion = "";
	gStitchHelper.FindNodeDoorInStitchHelper(	gEditorTerrain.GetSelectedNode(),
												gEditorTerrain.GetSelectedSourceDoorID(),
												nodeID, sRegion );
	sTemp.assignf( "0x%08X", nodeID );
	GetDlgItem( IDC_STITCH_ORDER )->SetWindowText( sTemp.c_str() );
	int sel = pCombo->FindString( -1, sRegion.c_str() );
	if ( sel != CB_ERR ) {
		pCombo->SetCurSel( sel );
		m_sRegion = sRegion.c_str();
	}	

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Node_Stitching::OnButtonStitchManager() 
{
	Dialog_Stitch_Manager dlgStitch;
	dlgStitch.DoModal();
}
