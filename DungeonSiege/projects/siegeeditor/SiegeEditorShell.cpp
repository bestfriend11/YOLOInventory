//////////////////////////////////////////////////////////////////////////////
//
//	File     		:  SiegeEditorShell.cpp
//
// 	Programmed by	:  Chad Queen
//
//	Copyright (c) 2002 Gas Powered Games Corp. All rights reserved. Gas
//	Powered Games, Dungeon Siege and Siege Editor are the exclusive trademarks of 
//	Gas Powered Games Corp.
//
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "atlx.h"
#include "BottomView.h"
#include "CameraAgent.h"
#include "ComponentList.h"
#include "config.h"
#include "CustomViewer.h"
#include "EditorCamera.h"
#include "EditorCamera.h"
#include "EditorLights.h"
#include "EditorMap.h"
#include "EditorObjects.h"
#include "EditorRegion.h"
#include "EditorTerrain.h"
#include "EditorTriggers.h"
#include "EditorGizmos.h"
#include "EditorTuning.h"
#include "EditorSfx.h"
#include "GizmoManager.h"
#include "GpConsole.h"
#include "GpProfiler.h"
#include "ImageListDefines.h"
#include "KeyMapper.h"
#include "LeftView.h"
#include "LogicalNodeCreator.h"
#include "MeshPreviewer.h"
#include "Preferences.h"
#include "ReportSysDialogs.h"
#include "ScidManager.h"
#include "Services.h"
#include "SiegeEditorDoc.h"
#include "Sim.h"
#include "Stitch_Helper.h"
#include "Stitch_Helper.h"
#include "TimeMgr.h"
#include "Weather.h"
#include "Window_Mesh_Previewer.h"
#include "World.h"
#include "WorldIndexBuilder.h"
#include "WorldOptions.h"
#include "WorldTerrain.h"
#include "WorldTime.h"
#include "GameState.h"
#include "CommandActions.h"
#include "EditorHotpoints.h"
#include "WorldState.h"
#include "GoMind.h"
#include "resource.h"
#include "siege_options.h"
#include "TimeOfDay.h"
#include "gps_manager.h"
#include "Server.h"
#include "MCP.h"
#include "WorldFx.h"
#include "FuelDb.h"
#include "RapiPrimitive.h"
#include "SiegeEditorView.h"
#include "SEVersion.h"
#include "TattooVersion.h"
#include "Mood.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Construction
SiegeEditorShell::SiegeEditorShell()
{
	SetOptions( OPTION_CHILD_DEF );
	ClearOptions( OPTION_FREEZE_TIME_DETECT );

	GetConfigInit().SetMyDocuments();
	GetConfigInit().m_IniFileName  = SE1_INI_NAME;
	GetConfigInit().m_RegistryName = SE1_INI_NAME;
	GetConfigInit().m_DocsSubDir   = DS1_APP_NAME;
	GetConfigInit().m_LogSubDir    = "Logs";

	SetAppName( SE1_APP_NAME );

	// zero out our components
	m_pEditorTerrain		= NULL;
	m_pWorldIndexBuilder	= NULL;
	m_pLogicalNodeCreator	= NULL;
	m_pEditorCamera			= NULL;
	m_pEditorRegion			= NULL;
	m_pImageList			= NULL;
	m_pComponentList		= NULL;
	m_pEditorMap			= NULL;
	m_pStitchHelper			= NULL;
	m_pPreferences			= NULL;
	m_pKeyMapper			= NULL;
	m_pCustomViewer			= NULL;
	m_pPreviewer			= NULL;
	m_pGizmoManager			= NULL;
	m_pEditorObjects		= NULL;
	m_pSCIDManager			= NULL;
	m_pEditorLights			= NULL;
	m_pOutputSink			= NULL;
	m_pEditorTriggers		= NULL;
	m_pCommandManager		= NULL;
	m_pEditorHotpoints		= NULL;
	m_pEditorGizmo			= NULL;
	m_pEditorTuning			= NULL;
	m_pEditorSfx			= NULL;

	m_hWndMain				= NULL;
	m_pDocument				= NULL;
	m_pStatusBar			= NULL;
	m_hAccel				= NULL;

	m_RapiOwner				= NULL;
	m_GpProfiler			= NULL;
	m_GpConsole				= NULL;
	m_Services				= NULL;
	m_World   				= NULL;
	m_GameState				= NULL;

	m_bShutdown				= false;
}


// Destruction
SiegeEditorShell::~SiegeEditorShell()
{
	Shutdown();
}


void SiegeEditorShell::SetImageList( CImageList * pImageList )
{
	if ( m_pImageList ) {
		delete m_pImageList;
	}
	m_pImageList = pImageList;
}


void SiegeEditorShell::SetAccelerator( HACCEL hAccel )
{
	DestroyAcceleratorTable( m_hAccel );
	m_hAccel = hAccel;

}


void SiegeEditorShell::GetBuildTimeString( gpstring & sBuild )
{
	sBuild = SysInfo::GetExeFileVersionText( gpversion::MODE_COMPLETE );
}


bool SiegeEditorShell::OnPaint( PAINTSTRUCT& ps )
{
	// only bother to paint if we're not active (we'd be painting anyway in that case)
	if (   !IsAppActive()
		&& (ps.rcPaint.left < ps.rcPaint.right )
		&& (ps.rcPaint.top  < ps.rcPaint.bottom) )
	{
		// convert to screen space
		GRect dstRect = ps.rcPaint;
		::ClientToScreen( GetMainWnd(), dstRect.TopLeft() );
		::ClientToScreen( GetMainWnd(), dstRect.BottomRight() );

		// tell rapi to repaint itself(ves)
		gRapiOwner.OnWmPaint( dstRect, ps.rcPaint );
	}
	return ( true );
}

bool SiegeEditorShell::OnSize( const GSize& newSize )
{
	if ( !IsFullScreen() && (newSize.cx != 0) && (newSize.cy != 0) )
	{
		gRapiOwner.InvalidateTextures();
		gRapiOwner.ForceResume();
		gRapiOwner.StopDrawing();
		gRapiOwner.PrepareToDraw( GetMainWnd(), 0, IsFullScreen(), newSize.cx, newSize.cy );
		gRapiOwner.RestoreTexturesAndLights();
	}
	return ( true );
}

bool SiegeEditorShell::OnInitCore( void )
{
	// this is where our SE prefs and keys go
	if ( gConfig.IsDevMode() )
	{
		// EXE dir is writable
		gConfig.SetPathVar( "se_user_path", "%exe_path%", Config::PVM_WRITABLE | Config::PVM_ABSOLUTE_PATH | Config::PVM_ERROR_ON_FAIL );
	}
	else
	{
		// play it safe, go to my docs
		gConfig.SetPathVar( "se_user_path", "%user_path%/Editor", Config::PVM_CREATE | Config::PVM_ABSOLUTE_PATH | Config::PVM_ERROR_ON_FAIL );
	}

	// profiler
	m_GpProfiler = new gpprofiler;

	// done
	return ( true );
}

bool SiegeEditorShell::OnInitWindow( void )
{
	// $$$ this section must change - configuration of rasterizer at game
	//     startup has to be absolutely bulletproof. this code is placeholder
	//     until polish time. -scott
/*
	// construct rapi controller
	m_RapiOwner = new RapiOwner;
	gpstring deviceDesc = gConfig.GetString( "driver_description" );

	// initialize rapi using old device if any
	TRYDX_SUCCEED( m_RapiOwner->Initialize( deviceDesc ) );

	// save back out the configuration
	deviceDesc = m_RapiOwner->GetActiveDeviceDescription();
	gConfig.Set( "driver_description", deviceDesc );

	// get some display-specific options
	bool vsync = gConfig.GetBool( "vsync", true );
	int bpp = gConfig.GetInt( "bits_per_pixel", 32 );

	// create directdraw
	return ( m_RapiOwner->PrepareToDraw( GetMainWnd(),			// HWND hwnd
										 0,						// int requestedDeviceID = 0,
										 false,
										 GetGameWidth(),
										 GetGameHeight(),
										 bpp,					// int fsBPP = 16,
										 vsync ) );				// bool bVsync = true
*/
	return true;
}

bool SiegeEditorShell::OnInitServices( void )
{

// Low-level core components.

	// user input console
	m_GpConsole = new GpConsole;

	// set product id
	FileSys::SetProductId( DS1_FOURCC_ID );

// Major subsystems.

	// system services
	m_Services = new Services;
	if ( !m_Services->InitServices( 'SE' ) )
	{
		return ( false );
	}

	m_GameState = new GameState;
	gpassert( m_GameState );

	// game world
	m_World = new World( true );
	if ( !m_World->Init() )
	{
		return ( false );
	}

	// Initialize Shell components
	InitShell();

	gDefaultRapi.IgnoreVertexColor( true );

	// Since it is an editor session, there is no multiplayer
	gWorld.InitServer( false );

	gGoDb.SetEditMode( true );

	gEditorTerrain.InitNodeCallback();

	// done
	return ( true );
}

bool SiegeEditorShell::OnShutdown( void )
{
	// $ general rule: shut down in reverse order of construction
	//                 (be sure to note exceptions here)

	m_bShutdown = true;

	// other
	if ( m_pImageList != NULL )
	{
		m_pImageList->DeleteImageList();
	}
	if ( m_hAccel != NULL )
	{
		DestroyAcceleratorTable( m_hAccel );
	}

	if ( GizmoManager::DoesSingletonExist() )
	{
		gGizmoManager.Deinit();
	}

	ShutdownShell();
	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.Shutdown();
	}	

	// services
	Delete( m_GpConsole );
	Delete( m_World     );	

	Delete( m_GameState );
	Delete( m_Services  );

	// window
	Delete( m_RapiOwner );

	// core
	Delete( m_GpProfiler );

	// done
	return ( true );
}

void SiegeEditorShell::OnRestoreSurfaces( bool force )
{
	//$$$
	UNREFERENCED_PARAMETER( force );
}

bool SiegeEditorShell::OnCheckFocus( void )
{
	// $$$ SUCCEEDED(lpDD4->TestCooperativeLevel())
	return ( true );
}

bool SiegeEditorShell::OnFrame( float deltaTime, float actualDeltaTime )
{
	// make sure we're paused
	// gWorldOptions.RSSetPaused( true );

	// commit any godb requests
	if ( GoDb::DoesSingletonExist() )
	{
		gGoDb.CommitAllRequests();
	}

	// this should only be called once in the main update loop
	gWorldTime.Update( deltaTime );
	gWorldState.Update();

	if ( gPreferences.GetUpdateWorld() )
	{
		gMCP.Update();
		gWorld.Update( gWorldTime.GetSecondsElapsed(), gWorldTime.GetSecondsElapsed() );
		gSiegeEngine.SetDeltaFrameTime( deltaTime );
		
		// Update the time of day lighting
		gTimeOfDay.Update( gWorldTime.GetSecondsElapsed() );				
	}
	else if ( gPreferences.GetUpdateSiegeEngine() )
	{
		gSiegeEngine.SetDeltaFrameTime( deltaTime );
	}
	else
	{
		gSiegeEngine.SetDeltaFrameTime( 0.0f );
	}

	if ( gPreferences.GetUpdateWeather() )
	{
		gWeather.Update( deltaTime );
	}
	else
	{
		gWeather.Update( 0.0f );
	}

	// special effects
	if( ( gPreferences.GetPreferenceDisable() == false ) && ( gPreferences.GetUpdateEffects() == true) )
	{
		gWorldFx.Update( deltaTime, deltaTime );
		gSim.Update( deltaTime );
	}
	else 
	{
		gWorldFx.Update( 0, 0 );
	}

	// Must draw after logic
	gSiegeEngine.BeginRender();
	gSiegeEngine.Renderer().IgnoreVertexColor( false );	

	gWorld.Draw( gWorldTime.GetSecondsElapsed() );

	GPoint pt;
	::GetCursorPos( pt );
	::ScreenToClient( GetHWnd(), pt );
	GPoint center = GetClientRect().Center();

	float normX = -((float)(pt.x - center.x)/(float)(center.x));
	float normY = -((float)(pt.y - center.y)/(float)(center.y));

	gSiegeEngine.GetMouseShadow().Update( normX, normY );
	gGoDb.UpdateMouseShadow();

	if (( gPreferences.GetIgnoreVertexColor() ) || ( gPreferences.GetPreferenceDisable() )) 
	{
		gSiegeEngine.Renderer().IgnoreVertexColor( true );
	}
	else 
	{
		gSiegeEngine.Renderer().IgnoreVertexColor( false );
	}

	if ( gPreferences.GetDrawCameraTarget() )
	{
		gEditorCamera.DrawCameraTarget();
	}
	
	gSiegeEngine.NodeWalker().RenderVisibleNodes();
	gSiegeEngine.Renderer().IgnoreVertexColor( false );
	gSiegeEngine.GetWorldSpaceLines().Render();
	gSiegeEngine.GetNodeSpaceLines().Render();
	gSiegeEngine.GetScreenSpaceLines().Render();
	gEditorTerrain.DrawGrid();
	gEditorTerrain.DrawFrustum();
	if ( gPreferences.GetModeNodePlacement() )
	{
		gEditorTerrain.UpdateNodeCalcTimer();
	}
	gEditorTerrain.DrawSelectedNode();

	if ( gPreferences.GetUpdateWeather() )
	{
		gWeather.Draw();
		gMood.Update( gWorldTime.GetSecondsElapsed() );
	}	

	matrix_3x3 camOrient	= gSiegeEngine.GetCamera().GetMatrixOrientation();
	gSiegeEngine.GetLabels().Render( camOrient );
	gWorldFx.SetCameraOrientation( camOrient );
	gWeather.SetCameraOrientation( camOrient );

	if ( gSiegeEngine.GetOptions().ShowPolyStatsEnabled() )
	{
		gSiegeEngine.PrintPolyStats( gServices.GetConsoleFont() );
	}
	if ( gSiegeEngine.GetOptions().ShowFPSEnabled() )
	{
		gSiegeEngine.PrintFrameRate( 1.0f / actualDeltaTime, GetTimeMultiplier(), gServices.GetConsoleFont() );
	}

	gDefaultRapi.SetTexture( 1, NULL );

	// Draw the selected source door
	gEditorTerrain.DrawSelectedNodeBoxes();
	gEditorTerrain.DrawSelectedLogicalNodeBoxes();
	gEditorTerrain.DrawDoor( gEditorTerrain.GetSelectedNode(), gEditorTerrain.GetSelectedSourceDoorID() );

	if ( gPreferences.GetDrawDoorNumbers() )
	{
		gEditorTerrain.DrawDoorNumbers( gEditorTerrain.GetSelectedNode() );
	}

	if ( gPreferences.GetDrawStitchDoors() )
	{
		gEditorTerrain.DrawStitchDoors();
	}

	if ( gPreferences.GetDrawLogicalFlags() )
	{
		gEditorTerrain.DrawLogicalFlags();
	}

	DrawMainFocus();
	gGizmoManager.Update();
	gGizmoManager.DrawGizmos();
	gGizmoManager.CursorRollover();
	gSiegeEngine.Renderer().SetClipFarDist( 10000.0f );
	gDefaultRapi.SetClipFarDist( 10000.0f );

	// Render the preview node
	if ( MeshPreviewer::DoesSingletonExist() && gPreviewer.GetPreviewerWindow() )
	{
		gPreviewer.RenderPreviewer();
	}

	// Render any light source boxes we have available
	if ( gPreferences.GetDrawDirectionalLights() == true )
	{
		gEditorLights.DrawDirectionalLights();
	}

	gEditorLights.DrawLightHuds();

	if ( gEditorHotpoints.GetCanDraw() == true )
	{
		gEditorHotpoints.Draw();
	}

	gSiegeEngine.EndRender();

	// Update the Siege frustums
	gSiegeEngine.UpdateFrustums( deltaTime );

	int type = 0;
	char szString[256] = "";

	vector_3	vCamera = gEditorCamera.GetPosition();
	gpstring sCamera;
	sCamera.assignf( "Camera: %0.3f, %0.3f, %0.3f", vCamera.x, vCamera.y, vCamera.z );
	GetStatusBar()->GetStatusBarCtrl().GetText( szString, 1, &type );
	if ( strcmp( szString, sCamera.c_str() ) != 0 ) {
		GetStatusBar()->GetStatusBarCtrl().SetText( sCamera.c_str(), 1, type );
	}

	gpstring sDistance;
	sDistance.assignf( "Distance %0.3f", gEditorCamera.GetDistance() );
	GetStatusBar()->GetStatusBarCtrl().GetText( szString, 2, &type );
	if ( strcmp( szString, sDistance.c_str() ) != 0 ) {
		GetStatusBar()->GetStatusBarCtrl().SetText( sDistance.c_str(), 2, type );
	}

	gpstring sRendered;
	sRendered.assignf( "SNOs: %d/%d", gSiegeEngine.GetRenderedNodeCount(),
								gSiegeEngine.GetRenderedNodeCount() + gSiegeEngine.GetCulledNodeCount() );
	GetStatusBar()->GetStatusBarCtrl().GetText( szString, 3, &type );
	if ( strcmp( szString, sRendered.c_str() ) != 0 ) {
		GetStatusBar()->GetStatusBarCtrl().SetText( sRendered.c_str(), 3, type );
	}

	gpstring sNode;
	sNode.assign( "Node: " );
	sNode += gEditorTerrain.GetSelectedNode().ToString();
	GetStatusBar()->GetStatusBarCtrl().GetText( szString, 4, &type );
	if ( strcmp( szString, sNode.c_str() ) != 0 ) {
		GetStatusBar()->GetStatusBarCtrl().SetText( sNode.c_str(), 4, type );
	}

	return ( true );
}

bool SiegeEditorShell::OnSysFrame( float deltaTime, float /*actualDeltaTime*/ )
{
	// update the camera
	gCameraAgent.Update( deltaTime );
	//gEditorCamera.Update();

	// update the sound system
	if ( gPreferences.GetUpdateSound() )
	{
		gSoundManager.Update( deltaTime );
	}
	else
	{
		gSoundManager.Update( 0.0f );
	}

	// all ok
	return ( true );
}

// Initialization
void SiegeEditorShell::InitShell()
{
	// Construct Editor Components
	m_pEditorTerrain		= new EditorTerrain		();
	m_pWorldIndexBuilder	= new WorldIndexBuilder ();
	m_pLogicalNodeCreator	= new LogicalNodeCreator();
	m_pEditorCamera			= new EditorCamera		( &gSiegeEngine.GetCamera() );
	m_pEditorRegion			= new EditorRegion		();
	m_pImageList			= new CImageList		();
	m_pComponentList		= new ComponentList		();
	m_pEditorMap			= new EditorMap			();
	m_pStitchHelper			= new Stitch_Helper		();
	m_pPreferences			= new Preferences		();
	m_pKeyMapper			= new KeyMapper			();
	m_pCustomViewer			= new CustomViewer		();
	m_pPreviewer			= new MeshPreviewer		();
	m_pGizmoManager			= new GizmoManager		();
	m_pEditorObjects		= new EditorObjects		();
	m_pSCIDManager			= new SCIDManager		();
	m_pEditorLights			= new EditorLights		();
	m_pEditorTriggers		= new EditorTriggers	();
	m_pCommandManager		= new CommandManager	();
	m_pEditorHotpoints		= new EditorHotpoints	();
	m_pEditorGizmo			= new EditorGizmos		();
	m_pEditorTuning			= new EditorTuning		();
	m_pEditorSfx			= new EditorSfx			();
	m_pOutputSink			= new ReportSys::EditCtlSink( gBottomView.GetConsoleEditCtrl().GetSafeHwnd() );

	// Add the sink to the global sink
	gGlobalSink.AddSink( m_pOutputSink, false );

	// Initialize the gizmo manager
	m_pGizmoManager->Init();

	// Load in key mapper data
	m_pKeyMapper->LoadKeyMappings();

	ConstructImageList();

	gLeftView.InitializeMainTreeData();
	gLeftView.InitializeCustomTreeData();

	gWorldOptions.SetShowGizmos( true );
	gWorldOptions.SetIndicateItems( false );

	gSiegeEngine.SetMultithreaded( false );
	gSiegeEngine.GetOptions().ToggleObjectFading();
	gSiegeEngine.Renderer().SetEditMode( true );

	GoMind::SetOptions( GoMind::OPTION_UPDATE_COMPUTER_ACTIONS );

	gComponentList.BuildCategoryNames();

	if ( ReportSys::GetErrorCount() != 0 )
	{
		MessageBox( GetMainHWnd(), "Errors occured on SE startup.\r\nPlease double check any content described in the error boxes.", "Warning", MB_OK );
	}

	gSoundManager.Enable( false, false );

	ReportSys::EnableContext( &gPerfContext, false );

	if ( gSiegeEngine.GetOptions().ErrorReporting() )
	{
		gSiegeEngine.GetOptions().ToggleErrorReporting();
	}

	FuelHandle hRoot( "root" );
	if ( hRoot )
	{
		hRoot->GetDB()->SetOptions( FuelDB::OPTION_SAVE_EMPTY_BLOCKS );
	}
}

void SiegeEditorShell::ConstructImageList()
{
	m_pImageList->Create( CX_ICON, CY_ICON, true, NUM_ICONS, 0 );	
		
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_GO) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_SN) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_REGION) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_3BOX) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_BBOX) );	
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_CLOSE_FOLDER) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_OPEN_FOLDER) );				
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_REDO) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_UNDO) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_LIGHT) );				
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_HOTPOINT) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_DECAL) );
	m_pImageList->Add( AfxGetApp()->LoadIcon(IDI_ICON_STARTINGPOS) );
}

void SiegeEditorShell::ShutdownShell()
{
	gPreviewer.Deinit();

	Delete( m_pOutputSink         );
	Delete( m_pEditorLights       );
	Delete( m_pSCIDManager	      );
	Delete( m_pEditorObjects      );
	Delete( m_pGizmoManager       );
	Delete( m_pPreviewer          );
	Delete( m_pCustomViewer       );
	Delete( m_pKeyMapper          );
	Delete( m_pPreferences        );
	Delete( m_pStitchHelper       );
	Delete( m_pEditorMap          );
	Delete( m_pComponentList      );
	Delete( m_pImageList          );
	Delete( m_pEditorRegion       );
	Delete( m_pEditorCamera       );
	Delete( m_pLogicalNodeCreator );
	Delete( m_pWorldIndexBuilder  );
	Delete( m_pEditorTerrain      );
	Delete( m_pEditorTriggers	  );
	Delete( m_pCommandManager	  );
	Delete( m_pEditorHotpoints	  );
	Delete( m_pEditorGizmo		  );
	Delete( m_pEditorTuning		  );
	Delete( m_pEditorSfx		  );
}


void SiegeEditorShell::DrawMainFocus()
{
	if ( gAppModule.GetMainWnd() == GetFocus() )
	{			
		RP_DrawEmptyRect( gSiegeEngine.Renderer(), 1, 1, gAppModule.GetGameWidth(), gAppModule.GetGameHeight(), 0xff00ff00 ); 
	}
}