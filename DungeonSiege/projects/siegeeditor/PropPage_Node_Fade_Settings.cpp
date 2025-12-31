// PropPage_Node_Fade_Settings.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "EditorTerrain.h"
#include "PropPage_Node_Fade_Settings.h"
#include "EditorRegion.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Fade_Settings dialog


PropPage_Node_Fade_Settings::PropPage_Node_Fade_Settings(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Node_Fade_Settings::IDD, IDS_STRING61204)
	, m_node_section( -1 )
	, m_node_level( -1 )
	, m_node_object( -1 )
	, m_bInitialized( false )
{
	//{{AFX_DATA_INIT(PropPage_Node_Fade_Settings)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PropPage_Node_Fade_Settings::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Node_Fade_Settings)
	DDX_Control(pDX, IDC_NODESECTION, m_section);
	DDX_Control(pDX, IDC_NODEOBJECT, m_object);
	DDX_Control(pDX, IDC_NODELEVEL, m_level);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Node_Fade_Settings, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Node_Fade_Settings)
	ON_WM_CREATE()
	ON_EN_CHANGE(IDC_NODESECTION, OnChangeNodesection)
	ON_EN_CHANGE(IDC_NODELEVEL, OnChangeNodelevel)
	ON_EN_CHANGE(IDC_NODEOBJECT, OnChangeNodeobject)
	ON_BN_CLICKED(IDC_BUTTON_SHOW, OnButtonShow)
	ON_BN_CLICKED(IDC_BUTTON_HIDE, OnButtonHide)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Fade_Settings message handlers

int PropPage_Node_Fade_Settings::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}


void PropPage_Node_Fade_Settings::SetFadeSettings() 
{
	if ( m_bInitialized )
	{
		gEditorTerrain.SetSelectedNodeFadeSettings( m_node_section, m_node_level, m_node_object );
	}
}


BOOL PropPage_Node_Fade_Settings::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_bInitialized = true;
	
	SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ));
	SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());	

	gpstring sTemp;	
	sTemp.assignf( "%d", node.GetSection() );
	((CEdit *)GetDlgItem( IDC_NODESECTION ))->SetWindowText( sTemp.c_str() );
	m_node_section = node.GetSection();

	sTemp.assignf( "%d", node.GetLevel() );
	((CEdit *)GetDlgItem( IDC_NODELEVEL ))->SetWindowText( sTemp.c_str() );
	m_node_level = node.GetLevel();

	sTemp.assignf( "%d", node.GetObject() );
	((CEdit *)GetDlgItem( IDC_NODEOBJECT ))->SetWindowText( sTemp.c_str() );
	m_node_object = node.GetObject();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PropPage_Node_Fade_Settings::OnChangeNodesection() 
{
	CString rText;
	((CEdit *)GetDlgItem( IDC_NODESECTION ))->GetWindowText( rText );
	m_node_section = atoi( rText.GetBuffer( rText.GetLength() ) ); 	
}


void PropPage_Node_Fade_Settings::OnChangeNodelevel() 
{
	CString rText;
	((CEdit *)GetDlgItem( IDC_NODELEVEL ))->GetWindowText( rText );
	m_node_level = atoi( rText.GetBuffer( rText.GetLength() ) );	
}

void PropPage_Node_Fade_Settings::OnChangeNodeobject() 
{
	CString rText;
	((CEdit *)GetDlgItem( IDC_NODEOBJECT ))->GetWindowText( rText );
	m_node_object = atoi( rText.GetBuffer( rText.GetLength() ) );	
}

void PropPage_Node_Fade_Settings::OnButtonShow() 
{
	CString rSection;
	CString rLevel;
	CString rObject;

	m_section.GetWindowText( rSection );
	m_level.GetWindowText( rLevel );
	m_object.GetWindowText( rObject );
	
	int section = 0;
	int level = 0;
	int object = 0;

	stringtool::Get( rSection.GetBuffer( 0 ), section );
	stringtool::Get( rLevel.GetBuffer( 0 ), level );
	stringtool::Get( rObject.GetBuffer( 0 ), object );

	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		SiegeNode * pNode = *i;		

		if ( pNode && 
			 ( pNode->GetSection() == section ) &&
			 ( pNode->GetLevel() == level ) && 
			 ( pNode->GetObject() == object ) )
		{
			pNode->SetVisible( true );
		}
	}	
}

void PropPage_Node_Fade_Settings::OnButtonHide() 
{
	CString rSection;
	CString rLevel;
	CString rObject;

	m_section.GetWindowText( rSection );
	m_level.GetWindowText( rLevel );
	m_object.GetWindowText( rObject );
	
	int section = 0;
	int level = 0;
	int object = 0;

	stringtool::Get( rSection.GetBuffer( 0 ), section );
	stringtool::Get( rLevel.GetBuffer( 0 ), level );
	stringtool::Get( rObject.GetBuffer( 0 ), object );

	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		SiegeNode * pNode = *i;		

		if ( pNode && 
			 ( pNode->GetSection() == section ) &&
			 ( pNode->GetLevel() == level ) && 
			 ( pNode->GetObject() == object ) )
		{
			pNode->SetVisible( false );
		}
	}		
}

void PropPage_Node_Fade_Settings::OnButtonSelect() 
{
	CString rSection;
	CString rLevel;
	CString rObject;

	m_section.GetWindowText( rSection );
	m_level.GetWindowText( rLevel );
	m_object.GetWindowText( rObject );
	
	int section = 0;
	int level = 0;
	int object = 0;

	stringtool::Get( rSection.GetBuffer( 0 ), section );
	stringtool::Get( rLevel.GetBuffer( 0 ), level );
	stringtool::Get( rObject.GetBuffer( 0 ), object );

	siege::SiegeEngine& engine = gSiegeEngine;
	FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
	FrustumNodeColl::iterator i;
	for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
	{
		SiegeNode * pNode = *i;		

		if ( pNode && 
			 ( pNode->GetSection() == section ) &&
			 ( pNode->GetLevel() == level ) && 
			 ( pNode->GetObject() == object ) )
		{
			if ( i == nodeColl.begin() )
			{
				gEditorTerrain.SetSelectedNode( pNode->GetGUID() );
			}
			else
			{
				gEditorTerrain.AddNodeAsSelected( pNode->GetGUID() );
			}
		}
	}		
}
