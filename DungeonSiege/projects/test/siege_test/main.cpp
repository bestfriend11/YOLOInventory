//
// Win32_app
//
// Generic Win32 application skeleton
//

#include "ignored_warnings.h"
#include <string>
#include <windows.h>

// TODO: add Seige to the include list in the DevTool
#include "siege_engine.h"
#include "pipeline.h"
#include "spacenode.h"
#include "camera\basic_camera.h"
#include "console\console.h"

#include "siege_tnk_file_system.h"
#include "siege_cache.h"
#include "siege_cache_handle.h"
#include "siege_boundary_door.h"

#include "win32_execution_environment.h"
#include "win32_opengl_context.h"
#include "win32_window.h"

// Start of all the things I need to use 'SHIFTER'
//#include "AIRBAG.h"
#include "GPGlobal.h"
#include "TNK.h"
#include "GAS.h"
#include "GASHandle.h"
#include "GASDefaultTypes.h"
#include "GASFileParser.h"
#include "OCTANE.h"
#include "OCTANE_Default_Algorithms.h"
#include "win32_SHIFTER.h"
#include "backfire.h"
#include "muffler.h"
//nclude "flamethrower_siege_context.h"
//nclude "flamethrower_static_target.h"
// End all the things I need to use 'SHIFTER'

#include "soul_handle.h"
#include "soul_configuration_handle.h"
#include "soul_animation_playback_parameters.h"

#include "siege_mouse_shadow.h"


// Declare a little test app class to hold all the 'globals'

using namespace siege;

class TestApp {
public :

	bool bActivated;
	bool bSizing;
	int	width;
	int	height;

	smart_pointer<SiegeEngine> d_engine;
	smart_pointer<siege::Pipeline> d_pipeline;

//	smart_pointer<siege::win32_file_system> d_fileSystem;
	smart_pointer<siege::file_system> d_fileSystem;

	smart_pointer<win32_opengl_context> d_renderingContext;

	smart_pointer<win32_execution_environment> d_executionEnvironment;

		// Stuff needed to use shifter

	smart_pointer<TNK> d_TNK;

	smart_pointer<GAS> d_GAS;
	smart_pointer<GAS_Default_Types> d_GASDefaultTypes;
	smart_pointer<GASFileParser> d_GASFileParser;

	smart_pointer<OCTANE> d_OCTANE;
	smart_pointer<OCTANE_Default_Algorithms> d_OCTANE_Default_Algorithms;

	smart_pointer<win32_SHIFTER> d_SHIFTER;

	smart_pointer<Muffler> d_sound;

	// end of shifter requirements

};

// FWD declaration of a simple support function
bool SiegeWorldLoader(SiegeEngine& engine, GAS& gas);

LRESULT	WINAPI ProcessMessages( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{

	RECT	rcWindow;
	TestApp *app;

	switch( msg )
	{
		case WM_ACTIVATEAPP:
			app = (TestApp *)GetProp(hWnd,"APP_ADDRESS");
			app->bActivated = ((wParam) != 0);
			break;


		case WM_PAINT:
//			app = (TestApp *)GetProp(hWnd,"APP_ADDRESS");
//			if( app->bActivated ) {
//				UpdateWindow( hWnd );
//			}
			break;

		case WM_MOVE:
			app = (TestApp *)GetProp(hWnd,"APP_ADDRESS");
			if( app->bActivated && !app->bSizing )
			{
				//coords passed: LOWORD(lParam), HIWORD(lParam)
				GetWindowRect( hWnd, &rcWindow );
			}
			break;

		case WM_SIZE:
			app = (TestApp *)GetProp(hWnd,"APP_ADDRESS");
			// Check to see if we are losing our window...
			if( SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam )
				app->bActivated = false;

			if( app->bActivated )
			{
				app->bSizing = true;
				GetWindowRect( hWnd, &rcWindow );
				app->bSizing = false;
			}
			break;

		case WM_CLOSE:
			DestroyWindow( hWnd );
			return 0;
    
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_COMMAND:

/*
			switch( LOWORD(wParam) )
			{
				case IDM_EXIT:
					SendMessage( hWnd, WM_CLOSE, 0, 0 );
				case IDM_MAXIMIZE:
					ShowWindow( hWnd, SW_MAXIMIZE );
					UpdateWindow( hWnd );
				case IDM_NORMALIZE:
					ShowWindow( hWnd, SW_NORMAL );
					UpdateWindow( hWnd );
			}
*/
			return 0;
			break;

		case WM_KEYDOWN:

			return 0;

		case WM_MOUSEMOVE:

			float xc = LOWORD(lParam);
			float yc = HIWORD(lParam);

			switch(wParam)
			{
				case MK_LBUTTON:
				{
				} break;

				case MK_RBUTTON:
				{
				} break;
			}
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}
            

void MouseRDown(void) {
};


soul_configuration_handle *gGrunt = 0;

class TriggerME {
public :
	void trigger(void) {
		gGrunt->PlayAnimation("stance","c","d",soul_animation_playback_parameters(0,0.5,1,0.5));
		gGrunt->PlayAnimation("salute","e","f",soul_animation_playback_parameters(0,0.5,1,0.15));
	}
};

TriggerME myTriggerME;



// TODO: GET RID OF THIS TEST STUFF
// Get the test meshes into memory

#include "siege_boundary_mesh.h"

HWND InitWin32App( HINSTANCE hInst, TestApp& app )
{


	// Get us a window 
	WNDCLASS wndClass = { CS_OWNDC | CS_BYTEALIGNCLIENT| CS_HREDRAW | CS_VREDRAW, ProcessMessages, 0, 0, hInst,
			  LoadIcon( hInst, MAKEINTRESOURCE( IDC_ICON )),
			  NULL /*LoadCursor( NULL, IDC_CROSS )*/, 
			  ( HBRUSH )GetStockObject( BLACK_BRUSH ), 0,
			  TEXT("SiegeEngineTestAppClass") };

	RegisterClass( &wndClass );

	
	app.bSizing = false;
	app.bActivated = false;

	app.width = GetSystemMetrics( SM_CXSCREEN )/2;
	app.height = GetSystemMetrics( SM_CYSCREEN )/2;

	app.width = 800;
	app.height = 600;

	HWND hWnd = CreateWindow( TEXT("SiegeEngineTestAppClass"), TEXT("Siege Engine Test App Window"),
				  WS_OVERLAPPEDWINDOW, 100, 100,
				  app.width, app.height, 0L, 0L, hInst, 0L );

	// Stow the global pointer so we can see it in the MessageHandler
	SetProp(hWnd,"APP_ADDRESS",(HANDLE)&app);

	ShowWindow(hWnd, SW_SHOWDEFAULT);

	UpdateWindow( hWnd );

	// Set up the Win32 handles
	app.d_executionEnvironment.TransferAndOwn(new win32_execution_environment( "GPG", "Siege Test" ));
	app.d_renderingContext.TransferAndOwn(new win32_opengl_context(hWnd));

/*

  *** Can't do anything until TNK is available.....
  *** Had to move the Siege init code....

////app.d_fileSystem.TransferAndOwn(new win32_file_system());

	app.d_fileSystem.TransferAndOwn(new tnk_file_system());

	// Set up our rendering pipeline
	app.d_pipeline.TransferAndOwn(new siege::Pipeline(*app.d_renderingContext,*app.d_systemClock));
	
	// Set up the Siege Engine
	app.d_engine.TransferAndOwn(new SiegeEngine(*app.d_fileSystem,*app.d_systemClock));


	// Test out the Attach methods
	app.d_engine->AttachRenderPipe(*app.d_pipeline);
	app.d_engine->RenderPipe().AttachGraphicsContext(*app.d_renderingContext);

*/

	// Prep the RingOS/RingApp/Services...

	//----- GAS
	{
		app.d_GAS.TransferAndOwn( new GAS );
		app.d_GASDefaultTypes.TransferAndOwn( new GAS_Default_Types( *(app.d_GAS.GetPointer()) ) );
	}

	//----- OCTANE
	{
		app.d_OCTANE.TransferAndOwn( new OCTANE );
		app.d_OCTANE_Default_Algorithms.TransferAndOwn( new OCTANE_Default_Algorithms( *(app.d_OCTANE.GetPointer()) ) );
	}

	//----- TNK
	{

		app.d_TNK.TransferAndOwn( new TNK( *(app.d_GAS.GetPointer()), *(app.d_OCTANE_Default_Algorithms.GetPointer()) ));
		GPString Execution_Home = "."; // Get the home directory for the app
		app.d_executionEnvironment->GetVariableString("home", Execution_Home );
		app.d_TNK->AddFiles( Execution_Home );
	}

	//----- init siege filesystem
	{

		SystemTime TankStartDate;
		::GetLocalTime( &TankStartDate );

		app.d_fileSystem.TransferAndOwn(new tnk_file_system(*app.d_TNK,TankStartDate));

		// Set up the Siege Engine
		app.d_engine.TransferAndOwn(new SiegeEngine(*app.d_fileSystem));

		// Set up our rendering pipeline
		app.d_pipeline.TransferAndOwn(new siege::Pipeline(*app.d_renderingContext));

		// Test out the Attach methods
		app.d_engine->AttachRenderPipe(*app.d_pipeline);
		app.d_engine->RenderPipe().AttachGraphicsContext(*app.d_renderingContext);
	}

	//----- init sound
	{
		app.d_sound.TransferAndOwn( new Muffler( *app.d_TNK ) );
		bool OK;
		OK = app.d_sound->Initialize( Win32FindTopLevelWindow(), 2, 22050, 16 );
		if( OK ) {
			app.d_sound->SetWorkingDirectory( "sound\\" );
			app.d_sound->SetDefaultSample( "DefaultSound.wav" );
		}
	}

	//----- GASFileParser
	{
		app.d_GASFileParser.TransferAndOwn( new GASFileParser( *(app.d_GAS.GetPointer()), *(app.d_TNK.GetPointer()) ) );
	}

/*	//----- IMG
	{
		app.d_IMG.TransferAndOwn( new IMG );
		app.d_IMG_Default_File_Formats.TransferAndOwn( new IMG_Default_File_Formats( *(mIMG.GetPointer()) ) );
	}
*/

	//----- SHIFTER
	{
		app.d_SHIFTER.TransferAndOwn( new win32_SHIFTER( *(app.d_GAS.GetPointer()) ) );
	}

	//----- FLAMETHROWER
	{
//		app.d_flamethrower.TransferAndOwn( new Flamethrower_context_tattoo( *app.d_engine, 
//						*app.d_sound, *app.d_TNK, *app.d_GAS ) );

	}

	SiegeWorldLoader(*app.d_engine,*app.d_GAS);

 
	return hWnd;
}

#include "SpaceOccupant.h"

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, int )
{

	smart_pointer<TestApp> myApp(new TestApp);

    HWND hWnd = InitWin32App( hInst, *myApp);

	double CurrTime = ::GetSystemSeconds();

	double FireInterval = 9;

	smart_pointer<soul_handle> hSoul(new soul_handle(*myApp->d_executionEnvironment));
	smart_pointer<soul_configuration_handle> hGrunt(hSoul->QueryForConfiguration("demodude2"));

	gGrunt = hGrunt.GetPointer();
	
	siege::SoulOccupant GruntOcc(hGrunt.GetPointer());
	
	GruntOcc.SetPosition(vector_3(0,3,0));
	GruntOcc.SetOrientation(matrix_3x3());
	
	siege::SoulOccupantList testList;
	testList.Add(GruntOcc);

	database_guid targGUID = myApp->d_engine->NodeWalker().TargetNodeGUID(); 
	SpaceNodeHandle handle(myApp->d_engine->NodeCache().UseObject(targGUID));
	SpaceNodeHandle::read_write_lock NodeLock(myApp->d_engine->NodeCache(),handle);
	SpaceNode& node = NodeLock.GetObject();
	
	node.SoulOccupants().Add(GruntOcc);
	
	hGrunt->PlayAnimation("walk","a","b",soul_animation_playback_parameters(true));

	for( ;; )
	{
		MSG msg;
		if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
		{
			if( WM_QUIT == msg.message )
			return msg.wParam;

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		
		double PrevTime = CurrTime;
		CurrTime = ::GetSystemSeconds();
		double ElapsedSeconds = CurrTime - PrevTime;

		myApp->d_sound->Update(ElapsedSeconds);

		myApp->d_SHIFTER->Update();

 //		myApp->d_engine->RenderPipe().BeginRender(myApp->d_engine->NodeWalker().CurrentCamera());;
		myApp->d_engine->RenderPipe().BeginRender(*myApp->d_engine);;

		myApp->d_engine->NodeWalker().RenderVisibleNodes(*myApp->d_engine);


		// Center the dude a little better
		glScalef(0.1f,0.1f,0.1f);
		glTranslatef(-2.0f,0.0f,-2.0f);


		if (hSoul.GetPointer()) {
			hSoul->DynamicUpdate(ElapsedSeconds);
			if (hGrunt.GetPointer()) {
				hGrunt->FOR_TEMPORARY_SIEGE_INTEGRATION_ONLY_Render(myApp->d_engine->RenderPipe().ContextLock());
			}
		}
	
		{
			myApp->d_engine->GetMouseShadow().Update(*myApp->d_engine);					
		}
		
		myApp->d_engine->RenderPipe().EndRender();
    }


	return 0;
}


bool SiegeWorldLoader(SiegeEngine& engine, GAS& gas) {


	// Load in the 'world'

 	GASHandle hSiegeWorld( gas, "SIEGE_WORLD");

	assert(hSiegeWorld.Valid());

	// Enumerate all the nodes in the world database
	GPString inDir;

	hSiegeWorld.GetDelimitedValue("Directory", '"', 1, inDir); 

	GASHandle::ArrayEnumeration meshes( hSiegeWorld, "MESH_FILE" );


	{for( GASHandle::ArrayEnumeration::iterator i = meshes.begin(); i != meshes.end(); ++i ) { 
			 

		GPString inGUIDstring;
		GPString inFName;

		database_guid inGUID;

		(*i)->GetDelimitedValue("GUID", '"', 1, inGUIDstring); 
		(*i)->GetDelimitedValue("FileName", '"', 1, inFName); 

		inGUID = database_guid(inGUIDstring);

		engine.Database().FileNameMap().insert(database::mapping::value_type(database_guid(inGUID),inDir+"\\"+inFName));

	}}

	GASHandle::ArrayEnumeration nodes( hSiegeWorld, "SIEGE_NODE" );


	{for( GASHandle::ArrayEnumeration::iterator i = nodes.begin(); i != nodes.end(); ++i ) { 
			 
		GPString inGUIDstring;
		GPString inMeshGUIDstring;
		int inDoorCount;

		database_guid inGUID;

		(*i)->GetDelimitedValue("GUID", '"', 1, inGUIDstring); 
		(*i)->GetDelimitedValue("MESH_GUID", '"', 1, inMeshGUIDstring); 
		(*i)->Get("NumDoors", inDoorCount); 

		inGUID = database_guid(inGUIDstring);
/*
		engine.Database().FileNameMap().insert(database::mapping::value_type(database_guid(inGUID),inDir+"\\"+inFName));
*/
		SpaceNodeHandle handle(engine.NodeCache().UseObject(inGUID));
		SpaceNodeHandle::read_write_lock NodeLock(engine.NodeCache(),handle);
		SpaceNode& node = NodeLock.GetObject();

		node.SetGUID(inGUID);

		inGUID = database_guid(inMeshGUIDstring);

		node.SetBoundaryMeshGUID(inGUID);
	
		BoundaryMeshHandle meshhandle(engine.BoundaryMeshCache().UseObject(inGUID));
		BoundaryMeshHandle::read_lock MeshLock(engine.BoundaryMeshCache(),meshhandle);
		const BoundaryMesh& bmesh = MeshLock.GetObject();

		GASHandle::ArrayEnumeration doors( (**i), "DOOR" );

		int door_index = 0;

		{for( GASHandle::ArrayEnumeration::iterator j = doors.begin(); j != doors.end(); ++j ) { 

			int inDoorID;
			GPString inFarGUIDstring;
			int inFarDoorID;

			database_guid inFarGUID;

			(*j)->Get("ID", inDoorID); 
			(*j)->GetDelimitedValue("FarGUID", '"', 1, inFarGUIDstring); 
			(*j)->Get("FarDoor", inFarDoorID); 

			inFarGUID = database_guid(inFarGUIDstring);

			BoundaryDoor* bd = bmesh.GetDoorByIndex(door_index++);

			// Here we should verify that the underlying mesh has a matching door
			node.AddDoor(inDoorID,bd->GetCenter(),bd->GetOrientation(),inFarGUID,inFarDoorID);

		}}


	}}

	GPString inTarg;

	if (!hSiegeWorld.GetDelimitedValue("TargetNode", '"', 1, inTarg)) {
		inTarg = "0 0 0 0";
	}

	database_guid targGUID = database_guid(inTarg);

	engine.NodeWalker().SetTargetNode(targGUID);

	//root.MeshOccupants().Add(museum_piece);

	return true;
}
