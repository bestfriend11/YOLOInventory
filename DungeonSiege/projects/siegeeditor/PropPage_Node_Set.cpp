// PropPage_Node_Set.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "PropPage_Node_Set.h"
#include "EditorTerrain.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Set dialog


PropPage_Node_Set::PropPage_Node_Set(CWnd* pParent /*=NULL*/)
	: CPropertyPage(PropPage_Node_Set::IDD, IDS_STRING61205)
{
	//{{AFX_DATA_INIT(PropPage_Node_Set)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PropPage_Node_Set::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Node_Set)
	DDX_Control(pDX, IDC_LIST_TEXTURES, m_textures);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Node_Set, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Node_Set)
	ON_WM_CREATE()
	ON_CBN_SELCHANGE(IDC_COMBO_NODESET, OnEditchangeComboNodeset)
	ON_CBN_SELCHANGE(IDC_COMBO_NODEVERSION, OnEditchangeComboNodeversion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Set message handlers

int PropPage_Node_Set::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;	
	
	return 0;
}

void PropPage_Node_Set::OnEditchangeComboNodeset() 
{
	CComboBox * pSetCombo = ((CComboBox *)GetDlgItem( IDC_COMBO_NODESET ));
	int sel = pSetCombo->GetCurSel();
	CString rSet;
	pSetCombo->GetLBText( sel, rSet );
	StringMap::iterator i = m_node_set_map.find( (gpstring)rSet.GetBuffer( rSet.GetLength() ) );				
	if ( i != m_node_set_map.end() ) {
		m_sNodeSet = (*i).second;		
	}	

	Refresh();
}


void PropPage_Node_Set::OnEditchangeComboNodeversion() 
{
	// TODO: Add your control notification handler code here	
}



void PropPage_Node_Set::SetNodeSet()
{
	if ( m_sNodeSet.empty() ) {
		return;
	}
	gEditorTerrain.SetTerrainNodeSet( m_sNodeSet.c_str(), m_sNodeVer.c_str() );
}

BOOL PropPage_Node_Set::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	CComboBox * pSetCombo = ((CComboBox *)GetDlgItem( IDC_COMBO_NODESET ));

	m_node_set_map.clear();				
	FuelHandle hNodeSets( "world:global:siege_nodes:Generic:generic_node_naming_key" );
	if ( hNodeSets.IsValid() ) {
		if ( hNodeSets->FindFirstKeyAndValue() ) {
			gpstring abbreviation;
			gpstring fullname;

			while ( hNodeSets->GetNextKeyAndValue( abbreviation, fullname ) ) {
				pSetCombo->AddString( fullname.c_str() );				
				m_node_set_map.insert( std::pair< gpstring, gpstring >( fullname, abbreviation ) );
			}
		}
	}

	// Get the engine
	SiegeEngine & engine = gSiegeEngine;
	
	// Accumulate static lights for the currently selected node
	SiegeNodeHandle handle( engine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( engine.NodeCache() );				
	gpstring sSet = node.GetTextureSetAbbr();
	StringMap::iterator k;
	for ( k = m_node_set_map.begin(); k != m_node_set_map.end(); ++k ) {
		if ( (*k).second.same_no_case( sSet ) ) {
			sSet = (*k).first;
			break;
		}
	}
	
	int sel = pSetCombo->FindString( -1, sSet.c_str() );	
	if ( sel != CB_ERR ) {
		pSetCombo->SetCurSel( sel );
	}		
	
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PropPage_Node_Set::Refresh()
{
	m_textures.ResetContent();

	// Get the engine
	SiegeEngine & engine = gSiegeEngine;
	
	SiegeNodeHandle handle( engine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( engine.NodeCache() );				
	
	NodeTexColl textures = node.GetTextureListing();
	NodeTexColl::iterator i;
	for ( i = textures.begin(); i != textures.end(); ++i )
	{
		gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( (*i).textureId );
		int pos = sTexture.find_last_of( "\\" );
		if ( pos != gpstring::npos )
		{
			sTexture = sTexture.substr( pos+1, sTexture.size() );
		}

		m_textures.AddString( sTexture.c_str() );
	}

}