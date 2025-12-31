// PropPage_Node_Mood.cpp : implementation file
//
#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "PropPage_Node_Mood.h"
#include "siege_engine.h"
#include "siege_node.h"
#include "EditorTerrain.h"
#include "Fuel.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Mood property page

IMPLEMENT_DYNCREATE(PropPage_Node_Mood, CPropertyPage)

PropPage_Node_Mood::PropPage_Node_Mood() : CPropertyPage(PropPage_Node_Mood::IDD)
{
	//{{AFX_DATA_INIT(PropPage_Node_Mood)
	//}}AFX_DATA_INIT
}

PropPage_Node_Mood::~PropPage_Node_Mood()
{
}

void PropPage_Node_Mood::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Node_Mood)
	DDX_Control(pDX, IDC_LIST_MOOD, m_listMood);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Node_Mood, CPropertyPage)
	//{{AFX_MSG_MAP(PropPage_Node_Mood)
	ON_BN_CLICKED(IDC_BUTTON_ADD_MOOD, OnButtonAddMood)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_MOOD, OnButtonRemoveMood)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ALL_MOODS, OnButtonRemoveAllMoods)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Mood message handlers

BOOL PropPage_Node_Mood::OnInitDialog() 
{
	
	CPropertyPage::OnInitDialog();
	m_comboMood = ((CComboBox *)GetDlgItem( IDC_COMBO_MOOD ));
	
	FuelHandle hMoodDir( "world:global:moods" );
	if ( hMoodDir.IsValid() ) {
		FuelHandleList hlMoods = hMoodDir->ListChildBlocks( -1, NULL, "mood_setting*" );
		FuelHandleList::iterator i;
		gpstring sName;
		for ( i = hlMoods.begin(); i != hlMoods.end(); ++i ) {
			(*i)->Get( "name", sName );
			m_comboMood->AddString( sName.c_str() );
		}
	}

	SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
	SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

	StringVec moods = node.GetMoods();
	StringVec::iterator i;
	for ( i = moods.begin(); i != moods.end(); ++i ) {
		m_listMood.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Node_Mood::OnButtonAddMood() 
{	
	int index = m_comboMood->GetCurSel();
	if ( index == CB_ERR ) {
		return;
	}

	CString rString;
	m_comboMood->GetLBText( index, rString );
	m_listMood.AddString( rString );
}

void PropPage_Node_Mood::OnButtonRemoveMood() 
{
	int index = m_listMood.GetCurSel();
	if ( index == LB_ERR ) {
		return;
	}

	m_listMood.DeleteString( index );	
}

void PropPage_Node_Mood::OnButtonRemoveAllMoods() 
{
	m_listMood.ResetContent();	
}


void PropPage_Node_Mood::SetNodeMood()
{
	// $$$ For now, only get the first mood and assign it; we'll probably have to change this later to
	// actually accept multiple moods

	{
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( gEditorTerrain.GetSelectedNode() ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		node.ClearMoods();

		CString rMood;
		if ( m_listMood.GetCount() != 0 )
		{
			m_listMood.GetText( 0, rMood );

			if ( rMood != "" ) 
			{
				node.AddMood( rMood.GetBuffer( 0 ) );
			}
		}
	}	

	GuidVec guid_vec = gEditorTerrain.GetSelectedNodes();
	GuidVec::iterator i;
	for ( i = guid_vec.begin(); i != guid_vec.end(); ++i ) {
		SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( *i ) );
		SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );
		node.ClearMoods();

		CString rMood;
		if ( m_listMood.GetCount() != 0 )
		{
			m_listMood.GetText( 0, rMood );

			if ( rMood != "" ) 
			{
				node.AddMood( rMood.GetBuffer( 0 ) );
			}
		}
	}
}

void PropPage_Node_Mood::OnDestroy() 
{
	CPropertyPage::OnDestroy();		
}

void PropPage_Node_Mood::OnOK() 
{
	SetNodeMood();
	
	CPropertyPage::OnOK();
}
