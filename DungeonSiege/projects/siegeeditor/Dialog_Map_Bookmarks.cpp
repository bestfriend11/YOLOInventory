// Dialog_Map_Bookmarks.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Map_Bookmarks.h"
#include "EditorMap.h"
#include "EditorCamera.h"
#include "EditorTerrain.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Bookmarks dialog


Dialog_Map_Bookmarks::Dialog_Map_Bookmarks(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Map_Bookmarks::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Map_Bookmarks)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Map_Bookmarks::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Map_Bookmarks)
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	DDX_Control(pDX, IDC_EDIT_BOOKMARK, m_bookmark_name);
	DDX_Control(pDX, IDC_UP, m_up);
	DDX_Control(pDX, IDC_ORBIT, m_orbit);
	DDX_Control(pDX, IDC_NORTH, m_north);
	DDX_Control(pDX, IDC_LIST_BOOKMARKS, m_list_bookmarks);
	DDX_Control(pDX, IDC_EDIT_NODE, m_node);
	DDX_Control(pDX, IDC_EAST, m_east);
	DDX_Control(pDX, IDC_DISTANCE, m_distance);
	DDX_Control(pDX, IDC_AZIMUTH, m_azimuth);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Map_Bookmarks, CDialog)
	//{{AFX_MSG_MAP(Dialog_Map_Bookmarks)
	ON_BN_CLICKED(IDC_BUTTON_USE_CURRENT_POSITION, OnButtonUseCurrentPosition)
	ON_BN_CLICKED(IDC_BUTTON_USE_SELECTED_NODE, OnButtonUseSelectedNode)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_LBN_SELCHANGE(IDC_LIST_BOOKMARKS, OnSelchangeListBookmarks)
	ON_BN_CLICKED(IDC_BUTTON_SET_DATA, OnButtonSetData)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Bookmarks message handlers

BOOL Dialog_Map_Bookmarks::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	StringVec bookmarks;
	StringVec::iterator i;
	gEditorMap.GetBookmarks( bookmarks );
	for ( i = bookmarks.begin(); i != bookmarks.end(); ++i ) {
		m_list_bookmarks.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Map_Bookmarks::OnButtonUseCurrentPosition() 
{
	gpstring sTemp;
	sTemp.assignf( "%f", gEditorCamera.GetAzimuth() );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetOrbit() );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetDistance() );
	m_distance.SetWindowText( sTemp );
}

void Dialog_Map_Bookmarks::OnButtonUseSelectedNode() 
{
	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorTerrain.GetSelectedNode().GetValue() );
	m_node.SetWindowText( sTemp );
}

void Dialog_Map_Bookmarks::OnButtonRemove() 
{
	if ( m_list_bookmarks.GetCurSel() == LB_ERR ) {
		return;
	}

	CString rBookmark;
	m_list_bookmarks.GetText( m_list_bookmarks.GetCurSel(), rBookmark );
	gEditorMap.RemoveBookmark( rBookmark.GetBuffer( 0 ) );

	m_list_bookmarks.ResetContent();
	StringVec bookmarks;
	StringVec::iterator i;
	gEditorMap.GetBookmarks( bookmarks );
	for ( i = bookmarks.begin(); i != bookmarks.end(); ++i ) {
		m_list_bookmarks.AddString( (*i).c_str() );
	}	
}

void Dialog_Map_Bookmarks::OnButtonNew() 
{
	int index = 0;
	gpstring sName = "new";
	SiegePos spos;
	gpstring sNewName = sName;	
	while ( !gEditorMap.AddBookmark( sNewName, spos, 0.0f, 0.0f, 0.0f ) ) {			
		sNewName.assignf( "%s_%d", sName.c_str(), index++ );
	}	

	m_list_bookmarks.ResetContent();
	StringVec bookmarks;
	StringVec::iterator i;
	gEditorMap.GetBookmarks( bookmarks );
	for ( i = bookmarks.begin(); i != bookmarks.end(); ++i ) {
		m_list_bookmarks.AddString( (*i).c_str() );
	}
}

void Dialog_Map_Bookmarks::OnSelchangeListBookmarks() 
{
	if ( m_list_bookmarks.GetCurSel() == LB_ERR ) {
		return;
	}

	CString rBookmark;
	m_list_bookmarks.GetText( m_list_bookmarks.GetCurSel(), rBookmark );
	gEditorMap.SetSelectedBookmark( rBookmark.GetBuffer( 0 ) );
	
	SiegePos spos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;
	gpstring sDescription;
	gEditorMap.GetBookmarkParameters( rBookmark.GetBuffer( 0 ), spos, azimuth, orbit, distance, sDescription );

	m_bookmark_name.SetWindowText( rBookmark );

	gpstring sTemp;
	sTemp.assignf( "%f", spos.pos.x );
	m_north.SetWindowText( sTemp );
	
	sTemp.assignf( "%f", spos.pos.y );
	m_up.SetWindowText( sTemp );

	sTemp.assignf( "%f", spos.pos.z );
	m_east.SetWindowText( sTemp );

	sTemp.assignf( "0x%08X", spos.node.GetValue() );
	m_node.SetWindowText( sTemp );

	sTemp.assignf( "%f", azimuth );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", orbit );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", distance );
	m_distance.SetWindowText( sTemp );

	m_description.SetWindowText( sDescription.c_str() );
}

void Dialog_Map_Bookmarks::OnButtonSetData() 
{
	SiegePos spos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;

	CString rName;
	m_bookmark_name.GetWindowText( rName );

	CString rTemp;
	m_north.GetWindowText( rTemp );
	spos.pos.x = atof( rTemp );
	m_up.GetWindowText( rTemp );
	spos.pos.y = atof( rTemp );
	m_east.GetWindowText( rTemp );
	spos.pos.z = atof( rTemp );

	m_node.GetWindowText( rTemp );	
	spos.node = siege::database_guid( rTemp );

	m_azimuth.GetWindowText( rTemp );
	azimuth = atof( rTemp );

	m_orbit.GetWindowText( rTemp );
	orbit = atof( rTemp );

	m_distance.GetWindowText( rTemp );
	distance = atof( rTemp );

	CString rDesc;
	m_description.GetWindowText( rDesc );

	gEditorMap.SetBookmarkParameters(	gEditorMap.GetSelectedBookmark(), 
										rName.GetBuffer( 0 ), spos, azimuth, orbit, distance, rDesc.GetBuffer( 0 ) );

	m_list_bookmarks.ResetContent();
	StringVec bookmarks;
	StringVec::iterator i;
	gEditorMap.GetBookmarks( bookmarks );
	for ( i = bookmarks.begin(); i != bookmarks.end(); ++i ) {
		m_list_bookmarks.AddString( (*i).c_str() );
	}
}
