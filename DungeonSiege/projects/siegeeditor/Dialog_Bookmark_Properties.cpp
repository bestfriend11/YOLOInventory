// Dialog_Bookmark_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Bookmark_Properties.h"
#include "EditorMap.h"
#include "EditorTerrain.h"
#include "EditorCamera.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Bookmark_Properties dialog


Dialog_Bookmark_Properties::Dialog_Bookmark_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Bookmark_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Bookmark_Properties)
	//}}AFX_DATA_INIT
}


void Dialog_Bookmark_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Bookmark_Properties)
	DDX_Control(pDX, IDC_Z, m_z);
	DDX_Control(pDX, IDC_Y, m_y);
	DDX_Control(pDX, IDC_X, m_x);
	DDX_Control(pDX, IDC_ORBIT, m_orbit);
	DDX_Control(pDX, IDC_EDIT_NODE, m_node);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	DDX_Control(pDX, IDC_EDIT_BOOKMARK, m_bookmark);
	DDX_Control(pDX, IDC_DISTANCE, m_distance);
	DDX_Control(pDX, IDC_AZIMUTH, m_azimuth);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Bookmark_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_Bookmark_Properties)
	ON_BN_CLICKED(IDC_BUTTON_USE_SELECTED_NODE, OnButtonUseSelectedNode)
	ON_BN_CLICKED(IDC_BUTTON_USE_CURRENT_POSITION, OnButtonUseCurrentPosition)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Bookmark_Properties message handlers

void Dialog_Bookmark_Properties::OnOK() 
{
	SiegePos spos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;

	CString rName;
	m_bookmark.GetWindowText( rName );

	CString rTemp;
	m_x.GetWindowText( rTemp );
	spos.pos.x = atof( rTemp );
	m_y.GetWindowText( rTemp );
	spos.pos.y = atof( rTemp );
	m_z.GetWindowText( rTemp );
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
	
	CDialog::OnOK();
}

BOOL Dialog_Bookmark_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SiegePos spos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;
	gpstring sDescription;
	gEditorMap.GetBookmarkParameters( gEditorMap.GetSelectedBookmark(), spos, azimuth, orbit, distance, sDescription );

	m_bookmark.SetWindowText( gEditorMap.GetSelectedBookmark().c_str() );

	gpstring sTemp;
	sTemp.assignf( "%f", spos.pos.x );
	m_x.SetWindowText( sTemp );
	
	sTemp.assignf( "%f", spos.pos.y );
	m_y.SetWindowText( sTemp );

	sTemp.assignf( "%f", spos.pos.z );
	m_z.SetWindowText( sTemp );

	sTemp.assignf( "0x%08X", spos.node.GetValue() );
	m_node.SetWindowText( sTemp );

	sTemp.assignf( "%f", azimuth );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", orbit );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", distance );
	m_distance.SetWindowText( sTemp );

	m_description.SetWindowText( sDescription.c_str() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Bookmark_Properties::OnButtonUseSelectedNode() 
{
	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorTerrain.GetSelectedNode().GetValue() );
	m_node.SetWindowText( sTemp );
}

void Dialog_Bookmark_Properties::OnButtonUseCurrentPosition() 
{
	gpstring sTemp;
	sTemp.assignf( "%f", gEditorCamera.GetAzimuth() );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetOrbit() );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetDistance() );
	m_distance.SetWindowText( sTemp );	
}

void Dialog_Bookmark_Properties::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
