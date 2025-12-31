// PropPage_Node_General.cpp : implementation file
//

#include "PrecompEditor.h"
#include "ComponentList.h"
#include "SiegeEditor.h"
#include "PropPage_Node_General.h"
#include "EditorTerrain.h"
#include "siege_database.h"

using namespace siege;


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_General dialog


PropPage_Node_General::PropPage_Node_General(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Node_General::IDD, IDS_STRING61207)
	, m_bOccludesLight( true )
	, m_bNodeVisible( true )
	, m_bBoundsCamera( true )
	, m_bOccludeCamera( true )
	, m_bLightLocked( false )
{
	//{{AFX_DATA_INIT(PropPage_Node_General)
	//}}AFX_DATA_INIT
}


void PropPage_Node_General::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Node_General)
	DDX_Control(pDX, IDC_CHECK_LIGHT_LOCKED, m_light_locked);
	DDX_Control(pDX, IDC_CHECK_CAMERA_FADE, m_camera_fade);
	DDX_Control(pDX, IDC_CHECK_BOUNDS_CAMERA, m_bounds_camera);
	DDX_Control(pDX, IDC_CHECK_OCCLUDE_CAMERA, m_occlude_camera);
	DDX_Control(pDX, IDC_CHECK_HIDE, m_hide_node);
	DDX_Control(pDX, IDC_CHECK_OCCLUDE, m_occlude);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Node_General, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Node_General)
	ON_BN_CLICKED(IDC_CHECK_OCCLUDE, OnCheckOcclude)
	ON_BN_CLICKED(IDC_CHECK_HIDE, OnCheckHide)
	ON_BN_CLICKED(IDC_CHECK_OCCLUDE_CAMERA, OnCheckOccludeCamera)
	ON_BN_CLICKED(IDC_CHECK_BOUNDS_CAMERA, OnCheckBoundsCamera)
	ON_BN_CLICKED(IDC_CHECK_CAMERA_FADE, OnCheckCameraFade)
	ON_BN_CLICKED(IDC_CHECK_LIGHT_LOCKED, OnCheckLightLocked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_General message handlers

BOOL PropPage_Node_General::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
		
	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
	
	GetDlgItem( IDC_EDIT_NODE_GUID )->SetWindowText( node.GetGUID().ToString().c_str() );
	GetDlgItem( IDC_EDIT_MESH_GUID )->SetWindowText( node.GetMeshGUID().ToString().c_str() );

	siege::MeshDatabase::MeshFileMap::iterator iMesh = gSiegeEngine.MeshDatabase().FileNameMap().find( node.GetMeshGUID() );
	if ( iMesh != gSiegeEngine.MeshDatabase().FileNameMap().end() )
	{
		GetDlgItem( IDC_EDIT_SNO_FILENAME )->SetWindowText( (*iMesh).second.c_str() );
	}	
	else
	{
		GetDlgItem( IDC_EDIT_SNO_FILENAME )->SetWindowText( gComponentList.GetMeshSnoFile( node.GetMeshGUID().ToString().c_str() ) );
	}

	m_occlude.SetCheck( (int)node.GetOccludesLight() );
	m_bOccludesLight = node.GetOccludesLight();
	m_hide_node.SetCheck( !(int)node.GetVisible() );	
	m_bNodeVisible = node.GetVisible();

	m_bOccludeCamera = node.GetOccludesCamera();
	m_occlude_camera.SetCheck( m_bOccludeCamera ? 1 : 0 );

	m_bBoundsCamera = node.GetBoundsCamera();
	m_bounds_camera.SetCheck( m_bBoundsCamera ? 1 : 0 );

	m_bCameraFade = node.GetCameraFade();
	m_camera_fade.SetCheck( m_bCameraFade ? 1 : 0 );

	m_bLightLocked = node.IsLightLocked();
	m_light_locked.SetCheck( m_bLightLocked ? 1 : 0 );
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PropPage_Node_General::SetNodeGeneric()
{	
	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		node.SetOccludesLight( m_bOccludesLight );		
		node.SetVisible( m_bNodeVisible );
		node.SetOccludesCamera( m_bOccludeCamera );
		node.SetBoundsCamera( m_bBoundsCamera );
		node.SetCameraFade( m_bCameraFade );
		node.SetLightLocked( m_bLightLocked );
	}	

	GuidVec guid_vec = gEditorTerrain.GetSelectedNodes();
	GuidVec::iterator i;
	for ( i = guid_vec.begin(); i != guid_vec.end(); ++i ) 
	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( *i ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		node.SetOccludesLight( m_bOccludesLight );
		node.SetVisible( m_bNodeVisible );
		node.SetOccludesCamera( m_bOccludeCamera );
		node.SetBoundsCamera( m_bBoundsCamera );
		node.SetCameraFade( m_bCameraFade );
		node.SetLightLocked( m_bLightLocked );
	}	
}

void PropPage_Node_General::OnCheckOcclude() 
{
	m_bOccludesLight = m_occlude.GetCheck() ? true : false;	
}

void PropPage_Node_General::OnCheckHide() 
{
	m_bNodeVisible = m_hide_node.GetCheck() ? false : true;	
}

void PropPage_Node_General::OnCheckOccludeCamera() 
{
	m_bOccludeCamera = m_occlude_camera.GetCheck() ? true : false;	
}

void PropPage_Node_General::OnCheckBoundsCamera() 
{
	m_bBoundsCamera = m_bounds_camera.GetCheck() ? true : false;	
}

void PropPage_Node_General::OnCheckCameraFade() 
{
	m_bCameraFade = m_camera_fade.GetCheck() ? true : false;
}

void PropPage_Node_General::OnCheckLightLocked() 
{
	m_bLightLocked = m_light_locked.GetCheck() ? true : false;	
}
