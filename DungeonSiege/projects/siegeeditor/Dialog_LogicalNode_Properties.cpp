// Dialog_LogicalNode_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_LogicalNode_Properties.h"
#include "EditorTerrain.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_LogicalNode_Properties dialog


Dialog_LogicalNode_Properties::Dialog_LogicalNode_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_LogicalNode_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_LogicalNode_Properties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_LogicalNode_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_LogicalNode_Properties)
	DDX_Control(pDX, IDC_LIST_LOGICAL_FLAGS, m_logicalFlags);
	DDX_Control(pDX, IDC_COMBO_AVAILABLE_FLAGS, m_availableFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_LogicalNode_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_LogicalNode_Properties)
	ON_BN_CLICKED(IDC_BUTTON_ADD_FLAG, OnButtonAddFlag)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_FLAG, OnButtonRemoveFlag)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_FLAGS, OnButtonClearFlags)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_LogicalNode_Properties message handlers

BOOL Dialog_LogicalNode_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_availableFlags.ResetContent();	

	AddLogicalFlag( LF_HUMAN_PLAYER );
	AddLogicalFlag( LF_COMPUTER_PLAYER );	
	AddLogicalFlag( LF_DIRT );
	AddLogicalFlag( LF_SHALLOW_WATER );
	AddLogicalFlag( LF_DEEP_WATER );
	AddLogicalFlag( LF_ICE );
	AddLogicalFlag( LF_LAVA );
	AddLogicalFlag( LF_SIZE1_MOVER );
	AddLogicalFlag( LF_SIZE2_MOVER );
	AddLogicalFlag( LF_SIZE3_MOVER );
	AddLogicalFlag( LF_SIZE4_MOVER );
	AddLogicalFlag( LF_MIST );		
	AddLogicalFlag( LF_HOVER );
	AddLogicalFlag( LF_BOSS );
	AddLogicalFlag( LF_CLEAR );				
	
	siege::eLogicalNodeFlags currFlags = (siege::eLogicalNodeFlags)gEditorTerrain.GetSelectedLogicalFlags();
	if ( currFlags != siege::LF_IS_ANY )
	{
		CheckLogicalFlag( currFlags, LF_HUMAN_PLAYER );
		CheckLogicalFlag( currFlags, LF_COMPUTER_PLAYER );		
		CheckLogicalFlag( currFlags, LF_DIRT );
		CheckLogicalFlag( currFlags, LF_SHALLOW_WATER );
		CheckLogicalFlag( currFlags, LF_DEEP_WATER );
		CheckLogicalFlag( currFlags, LF_ICE );
		CheckLogicalFlag( currFlags, LF_LAVA );
		CheckLogicalFlag( currFlags, LF_SIZE1_MOVER );
		CheckLogicalFlag( currFlags, LF_SIZE2_MOVER );
		CheckLogicalFlag( currFlags, LF_SIZE3_MOVER );
		CheckLogicalFlag( currFlags, LF_SIZE4_MOVER );
		CheckLogicalFlag( currFlags, LF_MIST );		
		CheckLogicalFlag( currFlags, LF_HOVER );
		CheckLogicalFlag( currFlags, LF_BOSS );
		CheckLogicalFlag( currFlags, LF_CLEAR );						
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_LogicalNode_Properties::AddLogicalFlag( siege::eLogicalNodeFlags flag )
{
	int index = m_availableFlags.AddString( ToString( flag ) );
	m_availableFlags.SetItemData( index, flag );

}

void Dialog_LogicalNode_Properties::CheckLogicalFlag( siege::eLogicalNodeFlags currentFlags, siege::eLogicalNodeFlags testFlag )
{
	if ( testFlag & currentFlags )
	{
		int index = m_logicalFlags.AddString( ToString( testFlag ) );
		m_logicalFlags.SetItemData( index, (DWORD)testFlag );
	}
}

void Dialog_LogicalNode_Properties::OnButtonAddFlag() 
{
	int sel = m_availableFlags.GetCurSel();
	if ( sel != CB_ERR )
	{
		CString rFlag;
		m_availableFlags.GetLBText( sel, rFlag );
		DWORD data = m_availableFlags.GetItemData( sel );

		int index = m_logicalFlags.AddString( rFlag.GetBuffer( 0 ) );
		m_logicalFlags.SetItemData( index, data );
	}
}

void Dialog_LogicalNode_Properties::OnOK() 
{
	DWORD flags = 0;
	
	siege::eLogicalNodeFlags currFlags = (siege::eLogicalNodeFlags)gEditorTerrain.GetSelectedLogicalFlags();
	for ( int j = 29; j != 32; ++j )
	{
		DWORD flag = 1<<j;
		if ( flag & currFlags )
		{
			flags |= flag;
		}		
	}

	for ( int i = 0; i != m_logicalFlags.GetCount(); ++i )
	{
		flags |= m_logicalFlags.GetItemData( i );
	}

	gEditorTerrain.SetSelectedLogicalFlags( siege::eLogicalNodeFlags(flags) );
	
	CDialog::OnOK();
}

void Dialog_LogicalNode_Properties::OnButtonRemoveFlag() 
{
	int sel = m_logicalFlags.GetCurSel();
	if ( sel != LB_ERR )
	{
		m_logicalFlags.DeleteString( sel );
	}
}

void Dialog_LogicalNode_Properties::OnButtonClearFlags() 
{
	m_logicalFlags.ResetContent();	
}

void Dialog_LogicalNode_Properties::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
