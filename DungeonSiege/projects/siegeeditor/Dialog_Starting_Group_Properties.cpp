// Dialog_Starting_Group_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Starting_Group_Properties.h"
#include "EditorGizmos.h"
#include "Dialog_Starting_Group_Add_Level.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_Properties dialog


Dialog_Starting_Group_Properties::Dialog_Starting_Group_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Starting_Group_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Starting_Group_Properties)
	//}}AFX_DATA_INIT
}


void Dialog_Starting_Group_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Starting_Group_Properties)
	DDX_Control(pDX, IDC_LIST_LEVELS, m_levels);
	DDX_Control(pDX, IDC_EDIT_SCREENNAME, m_screenname);
	DDX_Control(pDX, IDC_CHECK_DEV_ONLY, m_dev_only);
	DDX_Control(pDX, IDC_STATIC_GROUP_NAME, m_name);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	DDX_Control(pDX, IDC_CHECK_DEFAULT, m_default);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Starting_Group_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_Starting_Group_Properties)
	ON_BN_CLICKED(IDC_BUTTON_ADD_LEVEL, OnButtonAddLevel)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_LEVEL, OnButtonRemoveLevel)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ALL, OnButtonRemoveAll)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_Properties message handlers

BOOL Dialog_Starting_Group_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_name.SetWindowText( gEditorGizmos.GetSelectedStartGroup().c_str() );

	bool bDefault;
	bool bDevOnly;
	gpstring sDescription;
	gpstring sScreenName;
	gEditorGizmos.GetStartingGroupProperties( gEditorGizmos.GetSelectedStartGroup(), sDescription, bDefault, bDevOnly, sScreenName );

	m_default.SetCheck( bDefault ? TRUE : FALSE );
	m_dev_only.SetCheck( bDevOnly ? TRUE : FALSE );
	m_description.SetWindowText( sDescription.c_str() );
	m_screenname.SetWindowText( sScreenName.c_str() );

	m_levels.InsertColumn( 0, "World Name", LVCFMT_LEFT, 100 );
	m_levels.InsertColumn( 1, "Level", LVCFMT_LEFT, 50 );
	
	RefreshLevels();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Starting_Group_Properties::OnOK() 
{
	CString rDesc;
	m_description.GetWindowText( rDesc );

	CString rName;
	m_screenname.GetWindowText( rName );

	bool bChecked = m_default.GetCheck() ? true : false;
	bool bDevOnly = m_dev_only.GetCheck() ? true : false;
	
	gEditorGizmos.EditStartingGroup( gEditorGizmos.GetSelectedStartGroup(), rDesc.GetBuffer( 0 ), bChecked, bDevOnly, rName.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}

void Dialog_Starting_Group_Properties::OnButtonAddLevel() 
{
	Dialog_Starting_Group_Add_Level dlg;
	dlg.DoModal();

	RefreshLevels();	
}

void Dialog_Starting_Group_Properties::OnButtonRemoveLevel() 
{
	CString rWorld = m_levels.GetItemText( m_levels.GetSelectionMark(), 0 );
	gEditorGizmos.RemoveStartingGroupWorldLevel( gEditorGizmos.GetSelectedStartGroup(), rWorld.GetBuffer( 0 ) );

	RefreshLevels();	
}

void Dialog_Starting_Group_Properties::OnButtonRemoveAll() 
{
	gEditorGizmos.RemoveAllStartingGroupWorldLevels( gEditorGizmos.GetSelectedStartGroup() );

	RefreshLevels();	
}

void Dialog_Starting_Group_Properties::RefreshLevels()
{
	m_levels.DeleteAllItems();
	WorldLevels levels;
	WorldLevels::iterator iLevel;
	gEditorGizmos.GetStartingGroupWorldLevels( gEditorGizmos.GetSelectedStartGroup(), levels );
	int index = 0;
	for ( iLevel = levels.begin(); iLevel != levels.end(); ++iLevel )
	{		
		int newIndex = m_levels.InsertItem( index, (*iLevel).sWorld.c_str() );	
		gpstring sLevel;
		sLevel.assignf( "%d", (*iLevel).level );
		m_levels.SetItem( newIndex, 1, LVIF_TEXT, sLevel.c_str(), 0, 0, 0, 0 );	
		index++;
	}
}

void Dialog_Starting_Group_Properties::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
