// Dialog_Find.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Find.h"
#include "ComponentList.h"
#include "EditorObjects.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Find dialog


Dialog_Find::Dialog_Find(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Find::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Find)
	//}}AFX_DATA_INIT
}


void Dialog_Find::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Find)
	DDX_Control(pDX, IDC_OBJECTSCREENNAME, m_screen_name);
	DDX_Control(pDX, IDC_EDIT_SCID, m_scid);
	DDX_Control(pDX, IDC_OBJECTDESCRIPTION, m_description);
	DDX_Control(pDX, IDC_OBJECTBLOCKNAME, m_block_name);
	DDX_Control(pDX, IDC_OBJECT_TYPE, m_type_combo);
	DDX_Control(pDX, IDC_CONTENT_COMBO, m_content_combo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Find, CDialog)
	//{{AFX_MSG_MAP(Dialog_Find)
	ON_BN_CLICKED(IDC_FIND, OnFind)
	ON_BN_CLICKED(IDC_FIND_NEXT, OnFindNext)
	ON_BN_CLICKED(IDC_SELECT_ALL, OnSelectAll)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_REPLACE, OnReplace)
	ON_BN_CLICKED(IDC_REPLACEALL, OnReplaceall)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Find message handlers

void Dialog_Find::OnFind() 
{
	CString rScid;
	CString rDesc;
	CString rBlockName;
	CString rType;
	CString rScreenName;

	m_scid.GetWindowText( rScid );
	m_description.GetWindowText( rDesc );
	m_block_name.GetWindowText( rBlockName );
	m_screen_name.GetWindowText( rScreenName );

	if ( m_type_combo.GetCurSel() == CB_ERR ) {
		rType = "";
	}
	else {
		m_type_combo.GetLBText( m_type_combo.GetCurSel(), rType );
	}

	FIND_COMPONENT fc;
	
	if ( rScid != "" ) {
		stringtool::Get( rScid.GetBuffer( 0 ), fc.dwScid );		
	}
	else {
		fc.dwScid	= MakeInt(SCID_INVALID);
	}

	fc.sDevName		= rDesc;
	fc.sFuelName	= rBlockName;
	fc.sScreenName	= rScreenName;
	fc.sType		= rType;

	gGizmoManager.DeselectAll();
	if ( !gEditorObjects.Find( fc ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "No matches found", "Find Object", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}
}

void Dialog_Find::OnFindNext() 
{
	gGizmoManager.DeselectAll();
	gEditorObjects.FindNext();	
}

void Dialog_Find::OnSelectAll() 
{
	gGizmoManager.DeselectAll();
	gEditorObjects.FindSelectAll();	
}

void Dialog_Find::OnClose() 
{
	CloseWindow();
	DestroyWindow();
}

void Dialog_Find::OnReplace() 
{
	int index = m_content_combo.GetCurSel();
	if ( index != CB_ERR ) {
		CString rString;
		m_content_combo.GetLBText( index, rString );
		gEditorObjects.ReplaceObject( rString.GetBuffer( 0 ) );
	}	
}

void Dialog_Find::OnReplaceall() 
{
	// TODO: Add your control notification handler code here
	
}

BOOL Dialog_Find::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	ComponentVec objects;
	ComponentVec::iterator i;
	gComponentList.RetrieveObjectList( objects );
	for ( i = objects.begin(); i != objects.end(); ++i ) {
		m_content_combo.AddString( (*i).sFuelName.c_str() );
	}

	StringVec types;
	gComponentList.GetTemplateTypes( types );
	StringVec::iterator j;
	for ( j = types.begin(); j != types.end(); ++j ) {
		m_type_combo.AddString( (*j).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Find::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
