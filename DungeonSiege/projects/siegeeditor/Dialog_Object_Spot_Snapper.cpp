// Dialog_Object_Spot_Snapper.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Object_Spot_Snapper.h"
#include "EditorRegion.h"


// Namespaces
using namespace siege;


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Spot_Snapper dialog


Dialog_Object_Spot_Snapper::Dialog_Object_Spot_Snapper(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Spot_Snapper::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Spot_Snapper)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_Spot_Snapper::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Spot_Snapper)
	DDX_Control(pDX, IDC_LIST_SPOTS, m_list_spots);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Spot_Snapper, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Spot_Snapper)
	ON_BN_CLICKED(IDC_BUTTON_SNAP, OnButtonSnap)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Spot_Snapper message handlers

void Dialog_Object_Spot_Snapper::OnButtonSnap() 
{
	if ( gGizmoManager.GetNumObjectsSelected() == 0 ) {
		return;
	}

	if ( m_list_spots.GetSelectionMark() == LB_ERR ) {
		return;
	}

	CString rLabel = m_list_spots.GetItemText( m_list_spots.GetSelectionMark(), 0 );
	siege::database_guid nodeGUID( m_list_spots.GetItemData( m_list_spots.GetSelectionMark() ) );
	if ( nodeGUID == UNDEFINED_GUID ) {
		return;
	}

	SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( nodeGUID ));
	SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());
	const SiegeMesh& bmesh = node.GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());

	const Spot spot = bmesh.GetSpotByLabel( rLabel );
	
	SiegePos spos;
	spos.node	= nodeGUID;
	spos.pos	= spot.m_position;
	gGizmoManager.SetPosition( spos );
	gGizmoManager.SetOrientation( spot.m_orientation );
}


BOOL Dialog_Object_Spot_Snapper::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_list_spots.InsertColumn( 0, "Label", LVCFMT_LEFT, 100 );
	m_list_spots.InsertColumn( 1, "Node GUID", LVCFMT_LEFT, 100 );

	int index = 0;
	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{				
		const SiegeMesh& bmesh			= (*i)->GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());			
		for ( int j = 0; j != bmesh.NumSpots(); ++j ) 
		{
			const Spot spot = bmesh.GetSpotByIndex( j );
			index = m_list_spots.InsertItem( index, spot.m_label.c_str() );
			m_list_spots.SetItem( index, 1, LVIF_TEXT, (*i)->GetGUID().ToString().c_str(), 0, 0, 0, 0 );
			m_list_spots.SetItemData( index, (*i)->GetGUID().GetValue() );
			index++;
		}		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
