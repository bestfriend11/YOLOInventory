// Dialog_Starting_Positions.cpp : implementation file
//

#include "PrecompEditor.h"
#include "gpcore.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Starting_Positions.h"
#include "EditorMap.h"
#include "EditorTerrain.h"
#include "EditorCamera.h"
#include "stringtool.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Positions dialog


Dialog_Starting_Positions::Dialog_Starting_Positions(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Starting_Positions::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Starting_Positions)
	//}}AFX_DATA_INIT
}


void Dialog_Starting_Positions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Starting_Positions)
	DDX_Control(pDX, IDC_CAMERA_Z, m_camera_z);
	DDX_Control(pDX, IDC_CAMERA_Y, m_camera_y);
	DDX_Control(pDX, IDC_CAMERA_X, m_camera_x);
	DDX_Control(pDX, IDC_CAMERA_NODE, m_camera_node);
	DDX_Control(pDX, IDC_EDIT_ID, m_id);
	DDX_Control(pDX, IDC_UP, m_up);
	DDX_Control(pDX, IDC_ORBIT, m_orbit);
	DDX_Control(pDX, IDC_NORTH, m_north);
	DDX_Control(pDX, IDC_LIST_POSITIONS, m_positions);
	DDX_Control(pDX, IDC_EDIT_NODE, m_node);
	DDX_Control(pDX, IDC_EAST, m_east);
	DDX_Control(pDX, IDC_DISTANCE, m_distance);
	DDX_Control(pDX, IDC_AZIMUTH, m_azimuth);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Starting_Positions, CDialog)
	//{{AFX_MSG_MAP(Dialog_Starting_Positions)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_PREVIEW, OnButtonPreview)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_SETDATA, OnButtonSetdata)
	ON_BN_CLICKED(IDC_BUTTON_USE_SELECTED_NODE, OnButtonUseSelectedNode)
	ON_BN_CLICKED(IDC_BUTTON_USE_CURRENT_POSITION, OnButtonUseCurrentPosition)
	ON_LBN_SELCHANGE(IDC_LIST_POSITIONS, OnSelchangeListPositions)
	ON_BN_CLICKED(IDC_BUTTON_USE_SELECTED_NODE_CAMERA, OnButtonUseSelectedNodeCamera)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Positions message handlers

BOOL Dialog_Starting_Positions::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	StringVec positions;
	StringVec::iterator i;
	gEditorMap.GetStartPositions( positions );
	for ( i = positions.begin(); i != positions.end(); ++i ) {
		m_positions.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Starting_Positions::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void Dialog_Starting_Positions::OnButtonNew() 
{
	int index = 0;
	int id = 1;
	SiegePos spos;

	while ( !gEditorMap.AddStartPosition( id, spos, spos, 0.0f, 0.0f, 0.0f ) ) {
		id++;
	}	

	m_positions.ResetContent();
	StringVec positions;
	StringVec::iterator i;
	gEditorMap.GetStartPositions( positions );
	for ( i = positions.begin(); i != positions.end(); ++i ) {
		m_positions.AddString( (*i).c_str() );
	}	
}

void Dialog_Starting_Positions::OnButtonPreview() 
{
		SiegePos spos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;

	CString rID;
	m_id.GetWindowText( rID );
	int id = atoi( rID.GetBuffer( 0 ) );

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

	gEditorCamera.SetTargetPositionNode( spos.node );
	gEditorCamera.SetPosition( spos.pos.x , spos.pos.y, spos.pos.z );
	gEditorCamera.SetAzimuthAngle( azimuth );
	gEditorCamera.SetOrbitAngle( orbit );
	gEditorCamera.SetDistance( distance );	
}

void Dialog_Starting_Positions::OnButtonRemove() 
{
	if ( m_positions.GetCurSel() == LB_ERR ) {
		return;
	}

	CString rPosition;
	m_positions.GetText( m_positions.GetCurSel(), rPosition );
	gEditorMap.RemoveStartPosition( atoi( rPosition.GetBuffer( 0 ) ) );

	m_positions.ResetContent();
	StringVec positions;
	StringVec::iterator i;
	gEditorMap.GetStartPositions( positions );
	for ( i = positions.begin(); i != positions.end(); ++i ) {
		m_positions.AddString( (*i).c_str() );
	}		
}

void Dialog_Starting_Positions::OnButtonSetdata() 
{
	SiegePos spos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;

	CString rID;
	m_id.GetWindowText( rID );
	int id = atoi( rID.GetBuffer( 0 ) );

	CString rTemp;
	m_north.GetWindowText( rTemp );
	spos.pos.x = atof( rTemp );
	m_up.GetWindowText( rTemp );
	spos.pos.y = atof( rTemp );
	m_east.GetWindowText( rTemp );
	spos.pos.z = atof( rTemp );

	m_node.GetWindowText( rTemp );
	int nodeID = 0;
	stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), nodeID );
	spos.node = siege::database_guid( nodeID );

	m_azimuth.GetWindowText( rTemp );
	azimuth = atof( rTemp );

	m_orbit.GetWindowText( rTemp );
	orbit = atof( rTemp );

	m_distance.GetWindowText( rTemp );
	distance = atof( rTemp );

	SiegePos cameraPos;
	m_camera_x.GetWindowText( rTemp );
	cameraPos.pos.x = atof( rTemp );
	m_camera_y.GetWindowText( rTemp );
	cameraPos.pos.y = atof( rTemp );
	m_camera_z.GetWindowText( rTemp );
	cameraPos.pos.z = atof( rTemp );

	m_camera_node.GetWindowText( rTemp );
	int cameraId = 0;
	stringtool::Get( gpstring(rTemp.GetBuffer( 0 )), cameraId );
	cameraPos.node = siege::database_guid( cameraId );

	gEditorMap.SetPositionParameters( id, spos, cameraPos, azimuth, orbit, distance );

	m_positions.ResetContent();
	StringVec positions;
	StringVec::iterator i;
	gEditorMap.GetStartPositions( positions );
	for ( i = positions.begin(); i != positions.end(); ++i ) {
		m_positions.AddString( (*i).c_str() );
	}	
}

void Dialog_Starting_Positions::OnButtonUseSelectedNode() 
{
	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorTerrain.GetSelectedNode().GetValue() );
	m_node.SetWindowText( sTemp );	
}

void Dialog_Starting_Positions::OnButtonUseCurrentPosition() 
{
	gpstring sTemp;
	sTemp.assignf( "%f", gEditorCamera.GetAzimuth() );
	m_azimuth.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetOrbit() );
	m_orbit.SetWindowText( sTemp );

	sTemp.assignf( "%f", gEditorCamera.GetDistance() );
	m_distance.SetWindowText( sTemp );	
}


void Dialog_Starting_Positions::OnSelchangeListPositions() 
{
	if ( m_positions.GetCurSel() == LB_ERR ) {
		return;
	}

	CString rPosition;
	m_positions.GetText( m_positions.GetCurSel(), rPosition );
	gEditorMap.SetSelectedStartID( atoi ( rPosition.GetBuffer( 0 ) ) );
	
	SiegePos spos;
	SiegePos cameraPos;
	float azimuth	= 0.0f;
	float orbit		= 0.0f;
	float distance	= 0.0f;
	gEditorMap.GetPositionParameters( atoi( rPosition.GetBuffer( 0 ) ), spos, cameraPos, azimuth, orbit, distance );
	m_id.SetWindowText( rPosition );

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
	
	sTemp.assignf( "%f", cameraPos.pos.x );
	m_camera_x.SetWindowText( sTemp );

	sTemp.assignf( "%f", cameraPos.pos.y );
	m_camera_y.SetWindowText( sTemp );

	sTemp.assignf( "%f", cameraPos.pos.z );
	m_camera_z.SetWindowText( sTemp );

	sTemp.assignf( "0x%08X", cameraPos.node.GetValue() );
	m_camera_node.SetWindowText( sTemp );
}

void Dialog_Starting_Positions::OnButtonUseSelectedNodeCamera() 
{
	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorTerrain.GetSelectedNode().GetValue() );
	m_camera_node.SetWindowText( sTemp );		
}
