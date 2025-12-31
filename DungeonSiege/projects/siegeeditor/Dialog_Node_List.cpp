// Dialog_Node_List.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Node_List.h"
#include "EditorTerrain.h"
#include "ComponentList.h"
#include "EditorCamera.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_List dialog


Dialog_Node_List::Dialog_Node_List(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Node_List::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Node_List)
	//}}AFX_DATA_INIT
}


void Dialog_Node_List::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Node_List)
	DDX_Control(pDX, IDC_LIST_NODE, m_node_listbox);
	DDX_Control(pDX, IDC_EDIT_GUID, m_guid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Node_List, CDialog)
	//{{AFX_MSG_MAP(Dialog_Node_List)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_NODE, OnButtonSelectNode)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_EN_CHANGE(IDC_EDIT_GUID, OnChangeEditGuid)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_List message handlers

BOOL Dialog_Node_List::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_node_list.clear();
	gComponentList.RetrieveUsedNodeList( m_node_list );
	
	CListBox * pList = (CListBox *)GetDlgItem( IDC_LIST_NODE );
	pList->ResetContent();

	StringVec::iterator i;
	for ( i = m_node_list.begin(); i != m_node_list.end(); ++i ) {
		pList->AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Node_List::OnButtonSelectNode() 
{
	CListBox * pList = (CListBox *)GetDlgItem( IDC_LIST_NODE );
	int sel = pList->GetCurSel();
	if ( sel == LB_ERR ) {
		return;
	}
	CString rNode;
	pList->GetText( sel, rNode );
	siege::database_guid nodeGUID( rNode.GetBuffer( rNode.GetLength() ) );
	gEditorTerrain.SetSelectedNode( nodeGUID );

	SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( nodeGUID ));
	SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());						
	vector_3 worldPos = node.NodeToWorldSpace( vector_3() );
	gEditorCamera.SetPosition( worldPos.x, worldPos.y, worldPos.z );	
}

void Dialog_Node_List::OnOK() 
{		
	CDialog::OnOK();
}

void Dialog_Node_List::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Node_List::OnChangeEditGuid() 
{
	CString rText;
	m_guid.GetWindowText( rText );

	int found = m_node_listbox.FindString( 0, rText );
	if ( found != LB_ERR )
	{
		m_node_listbox.SetCurSel( found );
	}	
}
