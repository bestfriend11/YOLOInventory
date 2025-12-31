// Dialog_Lighting_Ambience.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Lighting_Ambience.h"
#include "EditorRegion.h"
#include "PropPage_Properties_Lighting.h"

using namespace siege;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_Ambience dialog


Dialog_Lighting_Ambience::Dialog_Lighting_Ambience(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Lighting_Ambience::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Lighting_Ambience)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Lighting_Ambience::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Lighting_Ambience)
	DDX_Control(pDX, IDC_TERRAIN_AMBIENT_COLOR, m_terrain_color);
	DDX_Control(pDX, IDC_OBJECT_AMBIENT_COLOR, m_object_color);
	DDX_Control(pDX, IDC_ACTOR_AMBIENT_COLOR, m_actor_color);
	DDX_Control(pDX, IDC_ACTOR_AMBIENCE, m_actor_ambience);
	DDX_Control(pDX, IDC_TERRAIN_AMBIENCE, m_object_ambience);
	DDX_Control(pDX, IDC_OBJECT_AMBIENCE, m_terrain_ambience);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Lighting_Ambience, CDialog)
	//{{AFX_MSG_MAP(Dialog_Lighting_Ambience)
	ON_EN_CHANGE(IDC_TERRAIN_AMBIENCE, OnChangeTerrainAmbience)
	ON_EN_CHANGE(IDC_OBJECT_AMBIENCE, OnChangeObjectAmbience)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, OnButtonUpdate)
	ON_BN_CLICKED(IDC_BUTTON_TERRAIN_COLOR, OnButtonTerrainColor)
	ON_BN_CLICKED(IDC_BUTTON_OBJECT_COLOR, OnButtonObjectColor)
	ON_BN_CLICKED(IDC_BUTTON_ACTOR_COLOR, OnButtonActorColor)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_Ambience message handlers

void Dialog_Lighting_Ambience::OnChangeTerrainAmbience() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	
}

void Dialog_Lighting_Ambience::OnChangeObjectAmbience() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	// TODO: Add your control notification handler code here
	
}

void Dialog_Lighting_Ambience::OnOK() 
{
	OnButtonUpdate();				
	CDialog::OnOK();
}

void Dialog_Lighting_Ambience::OnButtonUpdate() 
{
	SiegeEngine & engine = gSiegeEngine;
	CString rTerrainAmbience;
	CString rObjectAmbience;
	CString rActorAmbience;
	
	FuelHandleList fhlTerrain = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, NULL, "siege_node_list" );
	if ( fhlTerrain.size() != 0 ) {
		gpstring faddress = (*fhlTerrain.begin())->GetAddress();
		FuelHandle fhNodes( faddress );		

		float terrainIntensity = 0.0f;
		float objectIntensity = 0.0f;
		float actorIntensity = 0.0f;

		int terrainColor	= 0;
		int objectColor		= 0;
		int actorColor		= 0;

		GetDlgItemText( IDC_TERRAIN_AMBIENCE, rTerrainAmbience );			
		terrainIntensity = (float)(atof( rTerrainAmbience ));
		GetDlgItemText( IDC_OBJECT_AMBIENCE, rObjectAmbience );
		objectIntensity = (float)(atof( rObjectAmbience ));
		m_actor_ambience.GetWindowText( rActorAmbience );
		actorIntensity = (float)atof( rActorAmbience );

		CString rTemp;
		m_terrain_color.GetWindowText( rTemp );		
		stringtool::Get( rTemp.GetBuffer( 0 ), terrainColor );
		m_object_color.GetWindowText( rTemp );
		stringtool::Get( rTemp.GetBuffer( 0 ), objectColor );
		m_actor_color.GetWindowText( rTemp );
		stringtool::Get( rTemp.GetBuffer( 0 ), actorColor );

		FrustumNodeColl nodeColl = engine.GetFrustum( gEditorRegion.GetFrustumId() )->GetFrustumNodeColl();
		FrustumNodeColl::iterator i;
		for( i = nodeColl.begin(); i != nodeColl.end(); ++i )
		{			
			(*i)->SetAmbientColor( fScaleDWORDColor( terrainColor, terrainIntensity ) );
			(*i)->SetObjectAmbientColor( fScaleDWORDColor( objectColor, objectIntensity ) );
			(*i)->SetActorAmbientColor( fScaleDWORDColor( actorColor, actorIntensity ) );
			(*i)->AccumulateStaticLighting();
		}
		
		fhNodes->Set( "ambient_intensity", terrainIntensity );				
		fhNodes->Set( "object_ambient_intensity", objectIntensity );
		fhNodes->Set( "actor_ambient_intensity", actorIntensity );
		fhNodes->Set( "ambient_color", terrainColor, FVP_HEX_INT );
		fhNodes->Set( "object_ambient_color", objectColor, FVP_HEX_INT );
		fhNodes->Set( "actor_ambient_color", actorColor, FVP_HEX_INT );
	}	
}

BOOL Dialog_Lighting_Ambience::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandleList fhlTerrain;
	if ( !gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, "terrain_nodes", NULL, fhlTerrain ) ) {
		gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, NULL, "siege_node_list", fhlTerrain );
	}
	gpstring sAmbience;
	gpstring sObjectAmbience;
	gpstring sActorAmbience;
	fhlTerrain.front()->Get( "ambient_intensity", sAmbience );
	fhlTerrain.front()->Get( "object_ambient_intensity", sObjectAmbience );
	fhlTerrain.front()->Get( "actor_ambient_intensity", sActorAmbience );

	if ( sAmbience.empty() ) {
		sAmbience = "0.0";
	}
	if ( sObjectAmbience.empty() ) {
		sObjectAmbience = "0.0";
	}
	if ( sActorAmbience.empty() )
	{
		sActorAmbience = sObjectAmbience;
	}

	SetDlgItemText( IDC_TERRAIN_AMBIENCE, sAmbience.c_str() );			
	SetDlgItemText( IDC_OBJECT_AMBIENCE, sObjectAmbience.c_str() );
	m_actor_ambience.SetWindowText( sActorAmbience );

	// Now initialize the ambient color stuff; be sure to extract the intensity from the color.
	gpstring sAmbientColor, sObjectColor, sActorColor;
	fhlTerrain.front()->Get( "ambient_color", sAmbientColor );
	fhlTerrain.front()->Get( "object_ambient_color", sObjectColor );
	fhlTerrain.front()->Get( "actor_ambient_color", sActorColor );

	if ( sAmbientColor.empty() )
	{
		sAmbientColor = "0xffffffff";
	}
	if ( sObjectColor.empty() )
	{
		sObjectColor = "0xffffffff";
	}
	if ( sActorColor.empty() )
	{
		sActorColor = "0xffffffff";
	}
	
	m_terrain_color.SetWindowText( sAmbientColor.c_str() );
	m_object_color.SetWindowText( sObjectColor.c_str() );
	m_actor_color.SetWindowText( sActorColor.c_str() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Lighting_Ambience::OnButtonTerrainColor() 
{	
	int terrainColor = 0;
	CString rTemp;
	m_terrain_color.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), terrainColor );

	DWORD finalColor = GetColor( terrainColor );
	gpstring sTemp;
	sTemp.assignf( "0x%08X", finalColor );
	m_terrain_color.SetWindowText( sTemp.c_str() );
}

void Dialog_Lighting_Ambience::OnButtonObjectColor() 
{
	int objectColor = 0;
	CString rTemp;
	m_object_color.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), objectColor );

	DWORD finalColor = GetColor( objectColor );
	gpstring sTemp;
	sTemp.assignf( "0x%08X", finalColor );
	m_object_color.SetWindowText( sTemp.c_str() );	
}

void Dialog_Lighting_Ambience::OnButtonActorColor() 
{
	int actorColor = 0;
	CString rTemp;
	m_actor_color.GetWindowText( rTemp );
	stringtool::Get( rTemp.GetBuffer( 0 ), actorColor );

	DWORD finalColor = GetColor( actorColor );
	gpstring sTemp;
	sTemp.assignf( "0x%08X", finalColor );
	m_actor_color.SetWindowText( sTemp.c_str() );		
}


DWORD Dialog_Lighting_Ambience::GetColor( DWORD dwColor )
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );		

	BYTE red	= GetBValue( dwColor );
	BYTE green	= GetGValue( dwColor );
	BYTE blue	= GetRValue( dwColor );
	COLORREF color = RGB( red, green, blue );

	cc.rgbResult = color;	

	cc.lStructSize		= sizeof( CHOOSECOLOR );
	cc.hwndOwner		= GetSafeHwnd();
	cc.hInstance		= 0;
	cc.lpCustColors		= crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0L;
	cc.lpfnHook			= 0;
	cc.lpTemplateName	= 0;

	if ( !ChooseColor( &cc ) ) 
	{
		return dwColor;
	}

	red		= GetRValue( cc.rgbResult );
	green	= GetGValue( cc.rgbResult );
	blue	= GetBValue( cc.rgbResult );

	cc.rgbResult = RGB( blue, green, red );
	return cc.rgbResult;
}

void Dialog_Lighting_Ambience::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
