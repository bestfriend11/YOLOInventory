// Dialog_Region_Settings.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Region_Settings.h"
#include "EditorRegion.h"
#include "vector_3.h"
#include "stringtool.h"
#include "FuelDB.h"
#include "FileSysUtils.h"
#include "FileSys.h"
#include "EditorTerrain.h"
#include "siege_engine.h"
#include "siege_node.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Settings dialog


Dialog_Region_Settings::Dialog_Region_Settings(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Region_Settings::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Region_Settings)
	//}}AFX_DATA_INIT
}


void Dialog_Region_Settings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Region_Settings)
	DDX_Control(pDX, IDC_COMBO_ENVIRONMENTMAP, m_environment_map);
	DDX_Control(pDX, IDC_EDIT_REGION_GUID, m_region_guid);
	DDX_Control(pDX, IDC_EDIT_NAME, m_name);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_description);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Region_Settings, CDialog)
	//{{AFX_MSG_MAP(Dialog_Region_Settings)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Settings message handlers

BOOL Dialog_Region_Settings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_name.SetWindowText( gEditorRegion.GetRegionName() );
	m_description.SetWindowText( gEditorRegion.GetRegionDescription() );
	
	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorRegion.GetRegionGUID() );
	m_region_guid.SetWindowText( sTemp );
	
	FileSys::RecursiveFinder finder( "art/bitmaps/envmaps/*", 0 );
	gpstring fileName;
	while ( finder.GetNext( fileName ) )
	{
		if ( FileSys::IFileMgr::IsAliasedFrom( fileName, ".%img%" ) )
		{
			m_environment_map.AddString( FileSys::GetFileNameOnly( fileName ) );
		}
	}

	SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject(gEditorTerrain.GetTargetNode()));
	SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());						
	gpstring sTexture = gSiegeEngine.Renderer().GetTexturePathname( node.GetEnvironmentMap() );
	int pos = sTexture.find_last_of( "\\" );
	if ( pos != gpstring::npos )
	{
		sTexture = sTexture.substr( pos+1, sTexture.size() );
		int ppos = sTexture.find_last_of( "." );
		sTexture = sTexture.substr( 0, ppos );
		int found = m_environment_map.FindString( 0, sTexture );

		if ( found != CB_ERR )
		{
			m_environment_map.SetCurSel( found );
		}
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Region_Settings::OnOK() 
{
	CString rTemp;
	m_name.GetWindowText( rTemp );
	if ( rTemp != gEditorRegion.GetRegionName() ) {
		gEditorRegion.SetRegionName( rTemp.GetBuffer( 0 ) );
	}

	m_description.GetWindowText( rTemp );
	gEditorRegion.SetRegionDescription( rTemp.GetBuffer( 0 ) );

	gpstring sTemp;
	sTemp.assignf( "0x%08X", gEditorRegion.GetRegionGUID() );	
	m_region_guid.GetWindowText( rTemp );
	if ( sTemp.same_no_case( rTemp.GetBuffer( 0 ) ) == false )
	{
		MessageBox( "You have changed the region guid.  You must now save and reload the region in order for the change to take place and to prevent any unpredictable behavior.", "Region GUID Change", MB_OK );

		int regionID = 0;
		stringtool::Get( rTemp.GetBuffer( 0 ), regionID );
		gEditorRegion.SetRegionGUID( (RegionId)regionID );

	}	

	int sel = m_environment_map.GetCurSel();
	if ( sel != CB_ERR )
	{
		CString rTemp;
		m_environment_map.GetWindowText( rTemp );
		gEditorTerrain.SetEnvironmentMap( rTemp.GetBuffer( 0 ) );
	}

	CDialog::OnOK();
}

void Dialog_Region_Settings::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
