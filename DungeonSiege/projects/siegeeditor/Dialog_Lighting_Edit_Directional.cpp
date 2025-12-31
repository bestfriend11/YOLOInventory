// Dialog_Lighting_Edit_Directional.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Lighting_Edit_Directional.h"
#include "EditorLights.h"
#include "MenuCommands.h"
#include "EditorTerrain.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_Edit_Directional dialog


Dialog_Lighting_Edit_Directional::Dialog_Lighting_Edit_Directional(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Lighting_Edit_Directional::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Lighting_Edit_Directional)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Lighting_Edit_Directional::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Lighting_Edit_Directional)
	DDX_Control(pDX, IDC_DIRECTIONAL_LIGHT_LIST, m_light_list);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Lighting_Edit_Directional, CDialog)
	//{{AFX_MSG_MAP(Dialog_Lighting_Edit_Directional)
	ON_BN_CLICKED(IDC_NEW_LIGHT, OnNewLight)
	ON_BN_CLICKED(IDC_EDIT_LIGHT, OnEditLight)
	ON_BN_CLICKED(IDC_REMOVE_LIGHT, OnRemoveLight)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, OnButtonUpdate)
	ON_LBN_SELCHANGE(IDC_DIRECTIONAL_LIGHT_LIST, OnSelchangeDirectionalLightList)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_Edit_Directional message handlers

void Dialog_Lighting_Edit_Directional::OnNewLight() 
{
	SiegePos spos;
	spos.pos	= gSiegeEngine.GetMouseShadow().GetFloorHitPos().pos;
	spos.node	= gEditorTerrain.GetTargetNode();	
	gEditorLights.AddDefaultDirectionalLight( spos );	

	m_light_list.ResetContent();
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetDirectionalLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) 
	{
		siege::database_guid lightGUID( *i );
		m_light_list.AddString( lightGUID.ToString().c_str() );
	}
	OnButtonUpdate();
}

void Dialog_Lighting_Edit_Directional::OnEditLight() 
{
	int index = m_light_list.GetCurSel();
	if ( index == LB_ERR ) {
		return;
	}

	CString rString;
	m_light_list.GetText( index, rString );
	siege::database_guid lightGUID( rString );
		
	gGizmoManager.DeselectAll();
	//gGizmoManager.SelectGizmo( lightGUID.GetValue() );
	gGizmoManager.ForceSelectGizmo( lightGUID.GetValue() );
	gMenuCommands.ObjectProperties();
}

void Dialog_Lighting_Edit_Directional::OnRemoveLight() 
{
	int index = m_light_list.GetCurSel();
	if ( index == LB_ERR ) {
		return;
	}

	CString rString;
	m_light_list.GetText( index, rString );
	siege::database_guid lightGUID( rString );
	gGizmoManager.DeleteGizmo( lightGUID.GetValue() );

	m_light_list.ResetContent();
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetDirectionalLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) {
		siege::database_guid lightGUID( *i );
		m_light_list.AddString( lightGUID.ToString().c_str() );
	}
}

void Dialog_Lighting_Edit_Directional::OnButtonUpdate() 
{	
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetDirectionalLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) {
		siege::database_guid lightGUID( *i );
		gEditorLights.ReinsertLightSource( lightGUID );
	}	
	gEditorTerrain.UpdateAllNodeLighting();
}


void Dialog_Lighting_Edit_Directional::OnOK() 
{		
	gGizmoManager.DeselectAll();
	CDialog::OnOK();
}


void Dialog_Lighting_Edit_Directional::OnCancel() 
{	
	CDialog::OnCancel();
}


BOOL Dialog_Lighting_Edit_Directional::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	std::vector< DWORD > light_ids;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetDirectionalLightIDs( light_ids );
	for ( i = light_ids.begin(); i != light_ids.end(); ++i ) {
		siege::database_guid lightGUID( *i );
		m_light_list.AddString( lightGUID.ToString().c_str() );
	}	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Lighting_Edit_Directional::OnSelchangeDirectionalLightList() 
{
	int index = m_light_list.GetCurSel();
	if ( index == LB_ERR ) {
		return;
	}

	CString rString;
	m_light_list.GetText( index, rString );
	siege::database_guid lightGUID( rString );
		
	gGizmoManager.DeselectAll();
	//gGizmoManager.SelectGizmo( lightGUID.GetValue() );	
	gGizmoManager.ForceSelectGizmo( lightGUID.GetValue() );
}

void Dialog_Lighting_Edit_Directional::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
