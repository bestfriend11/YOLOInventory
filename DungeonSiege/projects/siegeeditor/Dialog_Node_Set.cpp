// Dialog_Node_Set.cpp : implementation file
//
#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Node_Set.h"
#include "EditorTerrain.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Set dialog


Dialog_Node_Set::Dialog_Node_Set(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Node_Set::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Node_Set)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Node_Set::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Node_Set)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Node_Set, CDialog)
	//{{AFX_MSG_MAP(Dialog_Node_Set)
	ON_CBN_SELCHANGE(IDC_COMBO_SET, OnEditchangeComboSet)
	ON_CBN_SELCHANGE(IDC_COMBO_VERSION, OnEditchangeComboVersion)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Set message handlers

void Dialog_Node_Set::OnEditchangeComboSet() 
{
	
	CComboBox * pSetCombo = ((CComboBox *)GetDlgItem( IDC_COMBO_SET ));
	int sel = pSetCombo->GetCurSel();
	CString rSet;
	pSetCombo->GetLBText( sel, rSet );	
	gpstring sSet = rSet.GetBuffer( 0 );
	StringMap::iterator i = m_node_set_map.find( sSet );					
	if ( i != m_node_set_map.end() ) {
		m_sNodeSet = (*i).second;		
	}	
}

void Dialog_Node_Set::OnEditchangeComboVersion() 
{
	// TODO: Add your control notification handler code here
	
}

void Dialog_Node_Set::OnOK() 
{
	if ( !m_sNodeSet.empty() ) {	
		gEditorTerrain.SetTerrainNodeSet( m_sNodeSet.c_str(), m_sNodeVer.c_str() );
		gEditorTerrain.SetNodeSetCharacters( m_sNodeSet.c_str() );
	}
	
	CDialog::OnOK();
}

BOOL Dialog_Node_Set::OnInitDialog() 
{	
	CDialog::OnInitDialog();
	
	CComboBox * pSetCombo = ((CComboBox *)GetDlgItem( IDC_COMBO_SET ));

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

	gpstring sSet;
	StringMap::iterator k;
	for ( k = m_node_set_map.begin(); k != m_node_set_map.end(); ++k ) {
		if ( (*k).second.same_no_case( gEditorTerrain.GetNodeSetCharacters() ) ) {
			sSet = (*k).first;
			break;
		}
	}
	
	int sel = pSetCombo->FindString( -1, sSet.c_str() );	
	if ( sel != CB_ERR ) {
		pSetCombo->SetCurSel( sel );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Node_Set::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
