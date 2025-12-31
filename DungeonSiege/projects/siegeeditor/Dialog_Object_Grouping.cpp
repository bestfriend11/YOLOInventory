// Dialog_Object_Grouping.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Object_Grouping.h"
#include "GizmoManager.h"
#include "ContentDb.h"
#include "Dialog_Object_Grouping_New.h"
#include "ComponentList.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Grouping dialog


Dialog_Object_Grouping::Dialog_Object_Grouping(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Grouping::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Grouping)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_Grouping::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Grouping)
	DDX_Control(pDX, IDC_EDIT_SCALE_MIN, m_scaleMin);
	DDX_Control(pDX, IDC_EDIT_SCALE_MAX, m_scaleMax);
	DDX_Control(pDX, IDC_CHECK_ROTATE_GROUP_MODE, m_rotateMode);
	DDX_Control(pDX, IDC_TREE_GROUPS, m_groups);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Grouping, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Grouping)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL, OnButtonDeselectAll)
	ON_BN_CLICKED(IDC_CHECK_ROTATE_GROUP_MODE, OnCheckRotateGroupMode)
	ON_BN_CLICKED(IDC_BUTTON_RANDOM_ROTATION, OnButtonRandomRotation)
	ON_BN_CLICKED(IDC_BUTTON_SHOW, OnButtonShow)
	ON_BN_CLICKED(IDC_BUTTON_HIDE, OnButtonHide)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_ADD_SELECTION, OnButtonAddSelection)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_SELECTION, OnButtonRemoveSelection)
	ON_BN_CLICKED(IDC_BUTTON_SHOW_SELECTION, OnButtonShowSelection)
	ON_BN_CLICKED(IDC_BUTTON_HIDE_SELECTION, OnButtonHideSelection)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_GROUP, OnButtonSelectGroup)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_GROUP, OnButtonDeselectGroup)
	ON_BN_CLICKED(IDC_BUTTON_SCALE, OnButtonScale)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Grouping message handlers

BOOL Dialog_Object_Grouping::OnInitDialog() 
{
	CDialog::OnInitDialog();

	Refresh();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Object_Grouping::Refresh()
{
	m_groups.DeleteAllItems();

	HTREEITEM hMetaGroups	= m_groups.InsertItem( "Metagroups", 0, 0 );
	HTREEITEM hCategories	= m_groups.InsertItem( "Categories", 0, 0 );
	HTREEITEM hGroups		= m_groups.InsertItem( "Groups", 0, 0 );

	ContentDb::StringDb::iterator iMetaGroups;
	ContentDb::StringDb metaGroups;
	gContentDb.GetAllMetaGroupNames( metaGroups );
	for ( iMetaGroups = metaGroups.begin(); iMetaGroups != metaGroups.end(); ++iMetaGroups )
	{		
		m_groups.InsertItem( (*iMetaGroups).c_str(), 0, 0, hMetaGroups );
	}

	StringVec groups;
	StringVec::iterator iGroups;
	gGizmoManager.GetGizmoGroupNames( groups );
	for ( iGroups = groups.begin(); iGroups != groups.end(); ++iGroups )
	{
		m_groups.InsertItem( (*iGroups).c_str(), 0, 0, hGroups );
	}

	StringSet categories;
	StringSet::iterator iCategory;
	gComponentList.GetCategoryNames( categories );
	for ( iCategory = categories.begin(); iCategory != categories.end(); ++iCategory )
	{
		m_groups.InsertItem( (*iCategory).c_str(), 0, 0, hCategories );
	}
}

void Dialog_Object_Grouping::OnButtonSelectAll() 
{
	HTREEITEM hParent = m_groups.GetSelectedItem();
	if ( hParent )
	{
		m_groups.SetCheck( hParent, TRUE );
		HTREEITEM hChild = m_groups.GetNextItem( hParent, TVGN_CHILD );
		while ( hChild )
		{
			m_groups.SetCheck( hChild, TRUE );
			hChild = m_groups.GetNextSiblingItem( hChild );
		}
	}	
}

void Dialog_Object_Grouping::OnButtonDeselectAll() 
{
	HTREEITEM hParent = m_groups.GetSelectedItem();
	if ( hParent )
	{
		m_groups.SetCheck( hParent, FALSE );
		HTREEITEM hChild = m_groups.GetNextItem( hParent, TVGN_CHILD );
		while ( hChild )
		{
			m_groups.SetCheck( hChild, FALSE );
			hChild = m_groups.GetNextSiblingItem( hChild );
		}
	}		
}

void Dialog_Object_Grouping::OnCheckRotateGroupMode() 
{
	
}

bool Dialog_Object_Grouping::GetRotateMode()
{
	return ( m_rotateMode.GetCheck() ? true : false );
}

void Dialog_Object_Grouping::OnButtonRandomRotation() 
{
	gGizmoManager.RandomRotateSelection();
}

void Dialog_Object_Grouping::OnButtonShow() 
{
	StringVec groups;
	GetSelectedGroups( groups );
	gGizmoManager.ShowGroupings( groups );	
}

void Dialog_Object_Grouping::OnButtonHide() 
{
	StringVec groups;
	GetSelectedGroups( groups );
	gGizmoManager.HideGroupings( groups );		
}

void Dialog_Object_Grouping::OnButtonNew() 
{
	Dialog_Object_Grouping_New dlg;
	dlg.DoModal();

	Refresh();	
}

void Dialog_Object_Grouping::OnButtonAddSelection() 
{
	int result = MessageBoxEx( GetSafeHwnd(), "Add selection to all checked groups?", "Add Selection", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
	if ( result == IDYES )
	{
		StringVec groups;
		GetSelectedGroups( groups );
		gGizmoManager.AddSelectionToGroup( groups );
	}
}

void Dialog_Object_Grouping::OnButtonRemoveSelection() 
{
	int result = MessageBoxEx( GetSafeHwnd(), "Are you sure you wish to remove the checked groups?", "Remove Groups", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
	if ( result == IDYES ) 
	{
		StringVec groups;
		GetSelectedGroups( groups );
		gGizmoManager.RemoveGroups( groups );
	}
}


void Dialog_Object_Grouping::GetSelectedGroups( StringVec & groups )
{
	HTREEITEM hItem = m_groups.GetNextItem( TVI_ROOT, TVGN_ROOT );
	while ( hItem )
	{	
		HTREEITEM hChild = m_groups.GetNextItem( hItem, TVGN_CHILD );
		while ( hChild )
		{
			if ( m_groups.GetCheck( hChild ) )
			{
				HTREEITEM hParent = m_groups.GetParentItem( hChild );
				if ( !m_groups.GetCheck( hParent ) )
				{
					hChild = m_groups.GetNextItem( hChild, TVGN_NEXT );
					continue;
				}
				
				CString rGroup = m_groups.GetItemText( hChild );
				groups.push_back( rGroup.GetBuffer( 0 ) );
			}	

			hChild = m_groups.GetNextItem( hChild, TVGN_NEXT );
		}

		hItem = m_groups.GetNextItem( hItem, TVGN_NEXT );
	}
}

void Dialog_Object_Grouping::OnButtonShowSelection() 
{
	gGizmoManager.ShowSelectedObjects();	
}

void Dialog_Object_Grouping::OnButtonHideSelection() 
{
	gGizmoManager.HideSelectedObjects();
	
}

void Dialog_Object_Grouping::OnButtonSelectGroup() 
{
	StringVec groups;
	GetSelectedGroups( groups );	
	gGizmoManager.SelectGroupings( groups );	
}

void Dialog_Object_Grouping::OnButtonDeselectGroup() 
{
	StringVec groups;
	GetSelectedGroups( groups );	
	gGizmoManager.DeselectGroupings( groups );
}

void Dialog_Object_Grouping::OnButtonScale() 
{
	float min = 0.0f;
	float max = 0.0f;
	CString rTemp;

	m_scaleMin.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), min );

	m_scaleMax.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), max );
	
	gGizmoManager.RandomScaleSelection( min, max );
}

void Dialog_Object_Grouping::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
