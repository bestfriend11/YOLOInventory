// Dialog_Preferences.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Preferences.h"
#include "Preferences.h"
#include "LeftView.h"
#include "PropPage_Properties_Lighting.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Preferences dialog


Dialog_Preferences::Dialog_Preferences(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Preferences::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Preferences)
	//}}AFX_DATA_INIT
}


void Dialog_Preferences::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Preferences)
	DDX_Control(pDX, IDC_CHECK_DRAW_CAMERA_TARGET, m_DrawCameraTarget);
	DDX_Control(pDX, IDC_CHECK_CAMERA_GAME, m_CameraRestrict);
	DDX_Control(pDX, IDC_CHECK_UPDATE_SIEGE_ENGINE, m_update_siege_engine);
	DDX_Control(pDX, IDC_CHECK_LIGHT_HUD, m_light_hud);
	DDX_Control(pDX, IDC_CHECK_LOGICAL_FLAG, m_drawLogicalFlags);
	DDX_Control(pDX, IDC_CHECK_DRAW_COMMANDS, m_drawCommands);
	DDX_Control(pDX, IDC_CHECK_RIGHT_CLICK_SCROLLING, m_right_click_scrolling);
	DDX_Control(pDX, IDC_CHECK_FLOORSONLY, m_floors_only);
	DDX_Control(pDX, IDC_CHECK_UPDATEWEATHER, m_update_weather);
	DDX_Control(pDX, IDC_CHECK_UPDATESOUND, m_update_sound);
	DDX_Control(pDX, IDC_CHECK_DRAW_STITCH_DOORS, m_draw_stitch);
	DDX_Control(pDX, IDC_CHECK_WATERDRAW, m_draw_water);
	DDX_Control(pDX, IDC_CHECK_LOGICALNODEDRAW, m_draw_lnodes);
	DDX_Control(pDX, IDC_CHECK_DRAWNORMALS, m_draw_normals);
	DDX_Control(pDX, IDC_CHECK_UPDATE_WORLD, m_update_world);
	DDX_Control(pDX, IDC_CHECK_FPS, m_fps);
	DDX_Control(pDX, IDC_EDIT_ZOOM_RATE, m_zoom_rate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Preferences, CDialog)
	//{{AFX_MSG_MAP(Dialog_Preferences)
	ON_BN_CLICKED(IDC_WIREFRAME, OnWireframe)
	ON_BN_CLICKED(IDC_DOORDRAW, OnDoordraw)
	ON_BN_CLICKED(IDC_FLOORDRAW, OnFloordraw)
	ON_BN_CLICKED(IDC_BOXES, OnBoxes)
	ON_BN_CLICKED(IDC_CHECK_DRAW_DOOR_NUM, OnCheckDrawDoorNum)
	ON_BN_CLICKED(IDC_EFFECTS, OnEffects)
	ON_BN_CLICKED(IDC_DLIGHTS, OnDlights)
	ON_BN_CLICKED(IDC_SLIGHTS, OnSlights)
	ON_BN_CLICKED(IDC_DSHADOWS, OnDshadows)
	ON_BN_CLICKED(IDC_SSHADOWS, OnSshadows)
	ON_BN_CLICKED(IDC_DSHADING, OnDshading)
	ON_BN_CLICKED(IDC_SSHADING, OnSshading)
	ON_BN_CLICKED(IDC_IGNOREVERTEXCOLOR, OnIgnorevertexcolor)
	ON_BN_CLICKED(IDC_POLYSTATS, OnPolystats)
	ON_BN_CLICKED(IDC_DISABLE, OnDisable)
	ON_CBN_SELCHANGE(IDC_COMBO_CONTENT_DISPLAY, OnEditchangeComboContentDisplay)
	ON_EN_CHANGE(IDC_EDIT_PRECISION_AMOUNT, OnChangeEditPrecisionAmount)
	ON_BN_CLICKED(IDC_CHECK_FPS, OnCheckFps)
	ON_EN_CHANGE(IDC_EDIT_ZOOM_RATE, OnChangeEditZoomRate)
	ON_BN_CLICKED(IDC_CHECK_DRAWNORMALS, OnCheckDrawnormals)
	ON_BN_CLICKED(IDC_CHECK_LOGICALNODEDRAW, OnCheckLogicalnodedraw)
	ON_BN_CLICKED(IDC_CHECK_WATERDRAW, OnCheckWaterdraw)
	ON_BN_CLICKED(IDC_CHECK_UPDATE_WORLD, OnCheckUpdateWorld)
	ON_BN_CLICKED(IDC_CHECK_DRAW_STITCH_DOORS, OnCheckDrawStitchDoors)
	ON_BN_CLICKED(IDC_CHECK_UPDATEWEATHER, OnCheckUpdateweather)
	ON_BN_CLICKED(IDC_CHECK_UPDATESOUND, OnCheckUpdatesound)
	ON_BN_CLICKED(IDC_CHECK_FLOORSONLY, OnCheckFloorsonly)
	ON_BN_CLICKED(IDC_CHECK_RIGHT_CLICK_SCROLLING, OnCheckRightClickScrolling)
	ON_BN_CLICKED(IDC_CHECK_DRAW_COMMANDS, OnCheckDrawCommands)
	ON_BN_CLICKED(IDC_CHECK_LOGICAL_FLAG, OnCheckLogicalFlag)
	ON_BN_CLICKED(IDC_BUTTON_SET_CLEAR_COLOR, OnButtonSetClearColor)
	ON_BN_CLICKED(IDC_CHECK_LIGHT_HUD, OnCheckLightHud)
	ON_BN_CLICKED(IDC_CHECK_UPDATE_SIEGE_ENGINE, OnCheckUpdateSiegeEngine)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_CHECK_CAMERA_GAME, OnCheckCameraGame)
	ON_BN_CLICKED(IDC_CHECK_DRAW_CAMERA_TARGET, OnCheckDrawCameraTarget)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Preferences message handlers

void Dialog_Preferences::OnWireframe() 
{	
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_WIREFRAME);
	if ( pButton->GetCheck() ) {
		gPreferences.SetWireframe( true );
	}
	else {
		gPreferences.SetWireframe( false );
	}
}

void Dialog_Preferences::OnDoordraw() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_DOORDRAW);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDrawDoors( true );
	}
	else {
		gPreferences.SetDrawDoors( false );
	}	
}

void Dialog_Preferences::OnFloordraw() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_FLOORDRAW);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDrawFloors( true );
	}
	else {
		gPreferences.SetDrawFloors( false );
	}	
}

void Dialog_Preferences::OnBoxes() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_BOXES);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDrawBoxes( true );
	}
	else {
		gPreferences.SetDrawBoxes( false );
	}	
}

void Dialog_Preferences::OnCheckDrawDoorNum() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_CHECK_DRAW_DOOR_NUM);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDrawDoorNumbers( true );
	}
	else {
		gPreferences.SetDrawDoorNumbers( false );
	}	
}

void Dialog_Preferences::OnEffects() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_EFFECTS);
	if ( pButton->GetCheck() ) {
		gPreferences.SetUpdateEffects( true );
	}
	else {
		gPreferences.SetUpdateEffects( false );
	}	
}

void Dialog_Preferences::OnDlights() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_DLIGHTS);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDLights( true );
	}
	else {
		gPreferences.SetDLights( false );
	}	
}

void Dialog_Preferences::OnSlights() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_SLIGHTS);
	if ( pButton->GetCheck() ) {
		gPreferences.SetSLights( true );
	}
	else {
		gPreferences.SetSLights( false );
	}	
}

void Dialog_Preferences::OnDshadows() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_DSHADOWS);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDShadows( true );
	}
	else {
		gPreferences.SetDShadows( false );
	}	
}

void Dialog_Preferences::OnSshadows() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_SSHADOWS);
	if ( pButton->GetCheck() ) {
		gPreferences.SetSShadows( true );
	}
	else {
		gPreferences.SetSShadows( false );
	}	
}

void Dialog_Preferences::OnDshading() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_DSHADING);
	if ( pButton->GetCheck() ) {
		gPreferences.SetDShading( true );
	}
	else {
		gPreferences.SetDShading( false );
	}	
}

void Dialog_Preferences::OnSshading() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_SSHADING);
	if ( pButton->GetCheck() ) {
		gPreferences.SetSShading( true );
	}
	else {
		gPreferences.SetSShading( false );
	}	
}

void Dialog_Preferences::OnIgnorevertexcolor() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_IGNOREVERTEXCOLOR);
	if ( pButton->GetCheck() ) {
		gPreferences.SetIgnoreVertexColor( true );
	}
	else {
		gPreferences.SetIgnoreVertexColor( false );
	}	
}

void Dialog_Preferences::OnPolystats() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_POLYSTATS);
	if ( pButton->GetCheck() ) {
		gPreferences.SetPolyStats( true );
	}
	else {
		gPreferences.SetPolyStats( false );
	}	
}

void Dialog_Preferences::OnDisable() 
{
	CButton * pButton = (CButton *)this->GetDlgItem(IDC_DISABLE);
	if ( pButton->GetCheck() ) {
		gPreferences.SetPreferenceDisable( true );
		GetDlgItem( IDC_WIREFRAME )->EnableWindow( FALSE );
		GetDlgItem( IDC_DOORDRAW )->EnableWindow( FALSE );
		GetDlgItem( IDC_FLOORDRAW )->EnableWindow( FALSE );
		GetDlgItem( IDC_BOXES )->EnableWindow( FALSE );
		GetDlgItem( IDC_SLIGHTS )->EnableWindow( FALSE );
		GetDlgItem( IDC_DLIGHTS )->EnableWindow( FALSE );
		GetDlgItem( IDC_SSHADOWS )->EnableWindow( FALSE );
		GetDlgItem( IDC_DSHADOWS )->EnableWindow( FALSE );
		GetDlgItem( IDC_SSHADING )->EnableWindow( FALSE );
		GetDlgItem( IDC_DSHADING )->EnableWindow( FALSE );
		GetDlgItem( IDC_IGNOREVERTEXCOLOR )->EnableWindow( FALSE );
		GetDlgItem( IDC_POLYSTATS )->EnableWindow( FALSE );
		GetDlgItem( IDC_EFFECTS )->EnableWindow( FALSE );
		GetDlgItem( IDC_CHECK_DRAW_DOOR_NUM )->EnableWindow( FALSE );
		m_draw_water.EnableWindow( FALSE );
		m_draw_lnodes.EnableWindow( FALSE );
		m_draw_normals.EnableWindow( FALSE );		
		m_fps.EnableWindow( FALSE );
	}
	else {
		gPreferences.SetPreferenceDisable( false );
		GetDlgItem( IDC_WIREFRAME )->EnableWindow( TRUE );
		GetDlgItem( IDC_DOORDRAW )->EnableWindow( TRUE );
		GetDlgItem( IDC_FLOORDRAW )->EnableWindow( TRUE );
		GetDlgItem( IDC_BOXES )->EnableWindow( TRUE );
		GetDlgItem( IDC_SLIGHTS )->EnableWindow( TRUE );
		GetDlgItem( IDC_DLIGHTS )->EnableWindow( TRUE );
		GetDlgItem( IDC_SSHADOWS )->EnableWindow( TRUE );
		GetDlgItem( IDC_DSHADOWS )->EnableWindow( TRUE );
		GetDlgItem( IDC_SSHADING )->EnableWindow( TRUE );
		GetDlgItem( IDC_DSHADING )->EnableWindow( TRUE );
		GetDlgItem( IDC_IGNOREVERTEXCOLOR )->EnableWindow( TRUE );
		GetDlgItem( IDC_POLYSTATS )->EnableWindow( TRUE );
		GetDlgItem( IDC_EFFECTS )->EnableWindow( TRUE );
		GetDlgItem( IDC_CHECK_DRAW_DOOR_NUM )->EnableWindow( TRUE );	
		m_fps.EnableWindow( TRUE );
		m_draw_water.EnableWindow( TRUE );
		m_draw_lnodes.EnableWindow( TRUE );
		m_draw_normals.EnableWindow( TRUE );		
	}	
}



BOOL Dialog_Preferences::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if ( gPreferences.GetWireframe() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_WIREFRAME);
		pButton->SetCheck( TRUE );		
	}
	if ( gPreferences.GetDrawDoors() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_DOORDRAW);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetDrawFloors() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_FLOORDRAW);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetDrawBoxes() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_BOXES);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetSLights() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_SLIGHTS);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetDLights() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_DLIGHTS);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetSShadows() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_SSHADOWS);
		pButton->SetCheck( TRUE );				
	}	
	if ( gPreferences.GetDShadows() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_DSHADOWS);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetSShading() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_SSHADING);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetDShading() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_DSHADING);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetIgnoreVertexColor() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_IGNOREVERTEXCOLOR);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetPolyStats() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_POLYSTATS);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetUpdateEffects() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_EFFECTS);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetDrawDoorNumbers() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_CHECK_DRAW_DOOR_NUM);
		pButton->SetCheck( TRUE );				
	}
	if ( gPreferences.GetFPS() ) {
		m_fps.SetCheck( TRUE );
	}
	if ( gPreferences.GetDrawWater() )
	{
		m_draw_water.SetCheck( TRUE );
	}
	if ( gPreferences.GetDrawLNodes() )
	{
		m_draw_lnodes.SetCheck( TRUE );
	}	
	if ( gPreferences.GetDrawNormals() )
	{
		m_draw_normals.SetCheck( TRUE );
	}
	if ( gPreferences.GetUpdateWorld() )
	{
		m_update_world.SetCheck( TRUE );
	}
	if ( gPreferences.GetUpdateSiegeEngine() )
	{
		m_update_siege_engine.SetCheck( TRUE );
	}
	if ( gPreferences.GetDrawStitchDoors() )
	{
		m_draw_stitch.SetCheck( TRUE );
	}
	if ( gPreferences.GetUpdateSound() )
	{
		m_update_sound.SetCheck( TRUE );
	}
	if ( gPreferences.GetUpdateWeather() )
	{
		m_update_weather.SetCheck( TRUE );
	}
	if ( gPreferences.GetHitFloorsOnly() )
	{
		m_floors_only.SetCheck( TRUE );
	}
	if ( gPreferences.GetRightClickScroll() )
	{
		m_right_click_scrolling.SetCheck( TRUE );
	}
	if ( gPreferences.GetDrawCommands() )
	{
		m_drawCommands.SetCheck( TRUE );
	}
	/* $ Remove for now
	if ( gPreferences.GetLockCustomView() )
	{
		m_lockCustomView.SetCheck( TRUE );
	}
	*/
	if ( gPreferences.GetDrawLogicalFlags() )
	{
		m_drawLogicalFlags.SetCheck( TRUE );
	}
	if ( gPreferences.GetDrawLightHuds() )
	{
		m_light_hud.SetCheck( TRUE );
	}
	if ( gPreferences.GetRestrictCameraToGame() )
	{
		m_CameraRestrict.SetCheck( TRUE );
	}
	if ( gPreferences.GetDrawCameraTarget() )
	{
		m_DrawCameraTarget.SetCheck( TRUE );
	}

	if ( gPreferences.GetPreferenceDisable() ) {
		CButton * pButton = (CButton *)this->GetDlgItem(IDC_DISABLE);
		pButton->SetCheck( TRUE );
		GetDlgItem( IDC_WIREFRAME )->EnableWindow( FALSE );
		GetDlgItem( IDC_DOORDRAW )->EnableWindow( FALSE );
		GetDlgItem( IDC_FLOORDRAW )->EnableWindow( FALSE );
		GetDlgItem( IDC_BOXES )->EnableWindow( FALSE );
		GetDlgItem( IDC_SLIGHTS )->EnableWindow( FALSE );
		GetDlgItem( IDC_DLIGHTS )->EnableWindow( FALSE );
		GetDlgItem( IDC_SSHADOWS )->EnableWindow( FALSE );
		GetDlgItem( IDC_DSHADOWS )->EnableWindow( FALSE );
		GetDlgItem( IDC_SSHADING )->EnableWindow( FALSE );
		GetDlgItem( IDC_DSHADING )->EnableWindow( FALSE );
		GetDlgItem( IDC_IGNOREVERTEXCOLOR )->EnableWindow( FALSE );
		GetDlgItem( IDC_POLYSTATS )->EnableWindow( FALSE );
		GetDlgItem( IDC_EFFECTS )->EnableWindow( FALSE );
		GetDlgItem( IDC_CHECK_DRAW_DOOR_NUM )->EnableWindow( FALSE );	
		m_fps.EnableWindow( FALSE );
		m_draw_water.EnableWindow( FALSE );
		m_draw_lnodes.EnableWindow( FALSE );
		m_draw_normals.EnableWindow( FALSE );		
	}


	// Content Display Settings
	CComboBox *pCombo = (CComboBox *)GetDlgItem( IDC_COMBO_CONTENT_DISPLAY );
	pCombo->AddString( "Template Name" );
	pCombo->AddString( "Screen Name" );
	pCombo->AddString( "Description" );
	
	if ( gPreferences.GetContentDisplay() == BLOCK_NAME ) {
		pCombo->SetCurSel( 0 );
	}
	else if ( gPreferences.GetContentDisplay() == SCREEN_NAME ) {		
		pCombo->SetCurSel( 1 );
	}
	else if ( gPreferences.GetContentDisplay() == DEV_NAME ) {		
		pCombo->SetCurSel( 2 );
	}
	
	// Precision Mode Settings
	gpstring sPrecision;
	sPrecision.assignf( "%f", gGizmoManager.GetPrecisionAmount() );
	GetDlgItem( IDC_EDIT_PRECISION_AMOUNT )->SetWindowText( sPrecision.c_str() );

	gpstring sZoomRate;
	sZoomRate.assignf( "%f", gPreferences.GetZoomRate() );
	m_zoom_rate.SetWindowText( sZoomRate );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Preferences::OnOK() 
{
	CDialog::OnOK();
}

void Dialog_Preferences::OnCancel() 
{	
	CDialog::OnCancel();
}

void Dialog_Preferences::OnEditchangeComboContentDisplay() 
{
	CComboBox *pCombo = (CComboBox *)GetDlgItem( IDC_COMBO_CONTENT_DISPLAY );
	CString rSel;
	pCombo->GetLBText( pCombo->GetCurSel(), rSel );
	if ( rSel == "Template Name" ) {
		gPreferences.SetContentDisplay( BLOCK_NAME );
	}
	else if ( rSel == "Screen Name" ) {
		gPreferences.SetContentDisplay( SCREEN_NAME );
	}
	else if ( rSel == "Description" ) {
		gPreferences.SetContentDisplay( DEV_NAME );
	}	

	gLeftView.SwitchContentDisplay();	
}

void Dialog_Preferences::OnChangeEditPrecisionAmount() 
{
	CString rText;
	GetDlgItem( IDC_EDIT_PRECISION_AMOUNT )->GetWindowText( rText );
	gGizmoManager.SetPrecisionAmount( atof( rText.GetBuffer( rText.GetLength() ) ) );
}

void Dialog_Preferences::OnCheckFps() 
{
	if ( m_fps.GetCheck() ) {
		gPreferences.SetFPS( true );
	}
	else {
		gPreferences.SetFPS( false );
	}	
}

void Dialog_Preferences::OnChangeEditZoomRate() 
{
	CString rText;
	m_zoom_rate.GetWindowText( rText );
	gPreferences.SetZoomRate( atof( rText.GetBuffer( 0 ) ) );	
}


void Dialog_Preferences::OnCheckDrawnormals() 
{
	if ( m_draw_normals.GetCheck() )
	{
		gPreferences.SetDrawNormals( true );
	}
	else
	{
		gPreferences.SetDrawNormals( false );
	}
	
}

void Dialog_Preferences::OnCheckLogicalnodedraw() 
{
	if ( m_draw_lnodes.GetCheck() )
	{
		gPreferences.SetDrawLNodes( true );
	}
	else
	{
		gPreferences.SetDrawLNodes( false );
	}
	
}

void Dialog_Preferences::OnCheckWaterdraw() 
{
	if ( m_draw_water.GetCheck() )
	{
		gPreferences.SetDrawWater( true );
	}
	else
	{
		gPreferences.SetDrawWater( false );
	}
	
}

void Dialog_Preferences::OnCheckUpdateWorld() 
{
	if ( m_update_world.GetCheck() )
	{
		int result = MessageBoxEx( GetSafeHwnd(), "Updating the world while editing can cause numerous problems and will cause saving problems. Do you wish to continue?", "Preferences - Update World", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
		if ( result == IDYES )
		{
			gPreferences.SetUpdateWorld( true );
		}
		else
		{
			m_update_world.SetCheck( FALSE );
		}
	}
	else
	{
		gPreferences.SetUpdateWorld( false );
	}	
}

void Dialog_Preferences::OnCheckDrawStitchDoors() 
{
	if ( m_draw_stitch.GetCheck() )
	{
		gPreferences.SetDrawStitchDoors( true );
	}
	else
	{
		gPreferences.SetDrawStitchDoors( false );
	}
}

void Dialog_Preferences::OnCheckUpdateweather() 
{
	if ( m_update_weather.GetCheck() )
	{
		gPreferences.SetUpdateWeather( true );
	}
	else
	{
		gPreferences.SetUpdateWeather( false );
	}
}

void Dialog_Preferences::OnCheckUpdatesound() 
{
	if ( m_update_sound.GetCheck() )
	{
		gPreferences.SetUpdateSound( true );
	}
	else
	{
		gPreferences.SetUpdateSound( false );
	}
}

void Dialog_Preferences::OnCheckFloorsonly() 
{
	if ( m_floors_only.GetCheck() )
	{
		gPreferences.SetHitFloorsOnly( true );
	}
	else
	{
		gPreferences.SetHitFloorsOnly( false );
	}		
}

void Dialog_Preferences::OnCheckRightClickScrolling() 
{
	if ( m_right_click_scrolling.GetCheck() )
	{
		gPreferences.SetRightClickScroll( true );
	}
	else
	{
		gPreferences.SetRightClickScroll( false );
	}
}

void Dialog_Preferences::OnCheckDrawCommands() 
{
	if ( m_drawCommands.GetCheck() )
	{
		gPreferences.SetDrawCommands( true );
	}
	else
	{
		gPreferences.SetDrawCommands( false );
	}	
}

/*
void Dialog_Preferences::OnCheckLockCustomView() 
{
	if ( m_lockCustomView.GetCheck() )
	{
		gPreferences.SetLockCustomView( true );
	}
	else
	{
		gPreferences.SetLockCustomView( false );
	}
}
*/

void Dialog_Preferences::OnCheckLogicalFlag() 
{
	gPreferences.SetDrawLogicalFlags( m_drawLogicalFlags.GetCheck() ? true : false );	
}

void Dialog_Preferences::OnButtonSetClearColor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	cc.rgbResult = 0x00FFFFFF;

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
		return;
	}

	float blue	= (float)((cc.rgbResult & 0x00FF0000) >> 16 ) / 255.0f;
	float green	= (float)((cc.rgbResult & 0x0000FF00) >> 8 ) / 255.0f;
	float red	= (float)( cc.rgbResult & 0x000000FF ) / 255.0f; 
	vector_3 color( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(color);	

	gPreferences.SetClearColor( dcolor );	
}

void Dialog_Preferences::OnCheckLightHud() 
{
	gPreferences.SetDrawLightHuds( m_light_hud.GetCheck() ? true : false );
}

void Dialog_Preferences::OnCheckUpdateSiegeEngine() 
{
	gPreferences.SetUpdateSiegeEngine( m_update_siege_engine.GetCheck() ? true : false );
}

void Dialog_Preferences::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Preferences::OnCheckCameraGame() 
{
	gPreferences.SetRestrictCameraToGame( m_CameraRestrict.GetCheck() ? true : false );	
}

void Dialog_Preferences::OnCheckDrawCameraTarget() 
{
	gPreferences.SetDrawCameraTarget( m_DrawCameraTarget.GetCheck() ? true : false );	
}
