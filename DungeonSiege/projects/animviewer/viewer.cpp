#include "precomp_animviewer.h"

#include "viewer.h"
#include "simpleui.h"
#include "services.h"
#include "res/resource.h"
#include "tattooversion.h"
#include "animviewerversion.h"
#include "HtmlHelp.h"

lVertex groundplane[8] = {

	{  4.0f, 0.0f,	4.0f, 	0.0f, -1.0f, 0.0f, 	0xff999999,	0,0	},
	{  4.0f, 0.0f, -4.0f, 	0.0f, -1.0f, 0.0f, 	0xff999999,	0,0	},
	{ -4.0f, 0.0f,  4.0f, 	0.0f, -1.0f, 0.0f, 	0xff999999,	0,0	},
	{ -4.0f, 0.0f, -4.0f,	0.0f, -1.0f, 0.0f, 	0xff999999,	0,0	},

	{ -4.0f, 0.0f, -4.0f,	0.0f, 1.0f, 0.0f, 	0xff666666,	0,0 },
	{  4.0f, 0.0f, -4.0f, 	0.0f, 1.0f, 0.0f, 	0xff666666,	0,0	},
	{ -4.0f, 0.0f,  4.0f, 	0.0f, 1.0f, 0.0f, 	0xff666666,	0,0	},
	{  4.0f, 0.0f,  4.0f, 	0.0f, 1.0f, 0.0f, 	0xff666666,	0,0	},

};

lVertex floorplane[8] = {

	{  4.0f, 0.0f,	4.0f, 	0.0f, -1.0f, 0.0f, 	0xffffffff,	0,0	},
	{  4.0f, 0.0f, -4.0f, 	0.0f, -1.0f, 0.0f, 	0xffffffff,	1,0	},
	{ -4.0f, 0.0f,  4.0f, 	0.0f, -1.0f, 0.0f, 	0xffffffff,	0,1	},
	{ -4.0f, 0.0f, -4.0f,	0.0f, -1.0f, 0.0f, 	0xffffffff,	1,1	},

	{ -4.0f, 0.0f, -4.0f,	0.0f, 1.0f, 0.0f, 	0xffffffff,	0,0 },
	{  4.0f, 0.0f, -4.0f, 	0.0f, 1.0f, 0.0f, 	0xffffffff,	1,0	},
	{ -4.0f, 0.0f,  4.0f, 	0.0f, 1.0f, 0.0f, 	0xffffffff,	0,1	},
	{  4.0f, 0.0f,  4.0f, 	0.0f, 1.0f, 0.0f, 	0xffffffff,	1,1	},

};

lVertex crosshair[4] = {
	{ 0.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f, 	0xffffffff,	0,0 },
	{ 0.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f, 	0xffffffff,	1,0	},
	{ 0.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f, 	0xffffffff,	0,1	},
	{ 0.0f, 0.0f, 0.0f,	0.0f, 1.0f, 0.0f, 	0xffffffff,	1,1	},
};

bool RapiCreateTextureGeneratedErrors(gpstring& errstr) {
	return (errstr.size() != 0 && errstr.find("Performance warning") == errstr.npos);
}

bool RapiCreateTextureGeneratedWarnings(gpstring& errstr) {
	return (errstr.size() != 0 && errstr.find("Performance warning") == 0);
}


void ForceEndOfFrameFlush(void)
{
	// After releasing an Aspect we need to wait for a frame
	gAspectStorage.CommitRequests();
	gDefaultRapi.Begin3D();
	gDefaultRapi.End3D();
}

void SetTextureHelper(nema::HAspect asp, DWORD texid, const gpstring& texnameIMG)
{
	gpstring basename = FileSys::GetPathOnly(texnameIMG) + FileSys::GetFileNameOnly(texnameIMG);
	
	asp->SetTextureCreatedByExternalSource(texid,0);

	ForceEndOfFrameFlush();

	if (   FileSys::DoesResourceFileExist(basename+".PSD")
		|| FileSys::DoesResourceFileExist(basename+".BMP")
		|| FileSys::DoesResourceFileExist(basename+".RAW")
		)
	{
		asp->SetTextureFromFile(texid,texnameIMG.c_str());
	}
	else
	{
		gNamingKey.BuildIMGLocation("b_i_glb_placeholder",basename);
		asp->SetTextureFromFile(texid,basename.c_str());
	}
}

Viewer::Viewer( void )
	: Inherited( OPTION_WINDOWED_DEF | OPTION_NO_PERF_MESSAGES )
	, m_GpProfiler( 0 )
	, m_EnvironmentTex( 0 )
{
	FileSys::SetProductId( DS1_FOURCC_ID );
	Services::SetAppType( Services::APP_ANIM_VIEWER );

	ConfigInit& animViewerInit = GetConfigInit();
	animViewerInit.SetMyDocuments();
	animViewerInit.m_IniFileName    = ANIMV_INI_NAME;
	animViewerInit.m_RegistryName   = ANIMV_INI_NAME;
	animViewerInit.m_DocsSubDir     = DS1_APP_NAME;
	animViewerInit.m_DisableDevMode = true;


	m_FloorTexID[0] =0;
}

Viewer::~Viewer( void )
{
}

bool Viewer::OnInitCore( void )
{
	
	SetAppName( ANIMV_APP_NAME );
	SetAppIcon( IDI_APP );

	// profiler
	m_GpProfiler = new gpprofiler;

	// done
	return ( true );
}

bool Viewer :: OnPreTranslateMessage( MSG& msg )
{
	UNREFERENCED_PARAMETER(msg);
	return false;
}


bool Viewer :: OnMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& returnValue )
{
	UNREFERENCED_PARAMETER(hwnd);
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);
	UNREFERENCED_PARAMETER(returnValue);

	bool ok = false;

	if (msg == WM_APP)
	{
		switch (wparam)
		{
			case 1:
			{
				m_PreviewLoad = true;
				ok = LoadData();
				if (ok)
				{
					OnFrame(0.0f,0.0f);
					ForceEndOfFrameFlush();
				}
				break;
			}
		}
	}

	return ok;
}

bool Viewer::LoadData( void )
{
	if ( PrepMesh(Aspect0,Mesh0FName,"Aspect0","Select MAIN mesh") )
	{
		vector_3 vc, vhd;
		Aspect0->GetBoundingBox(vc,vhd);
		SUI_SetCameraHeight(-(vc.y+vhd.y*.333f));
		PrepAnim(Aspect0,AnimFName,"anim0","Select Anim for MAIN mesh");

		SetWindowTitle();

		m_PreviewLoad = false;

		m_QuickViewMesh = false;
		m_QuickViewAnim = false;

		if (!m_FloorTexID[0])
		{
			PrepFloor(true);
		}

		return true;
	}
	return false;
}

bool Viewer::OnExecute ( void ) {
	
	//----- Initialize legacy vars
	m_WorldPosition = vector_3::ZERO;
	m_WorldRotation = Quat::IDENTITY;

	m_IgnoreMouseFlag = true;
	m_FollowAspect0Flag = false;

	m_TimeScale = 1.0f;

	m_Jogging = false;
	m_JogAngle = 0.0f;
	m_JogRadius = 3.5f;

	m_DrawRootFlag = true;

	m_NoFloorFlag = false;
	m_FloorTexID[0] = 0;
	m_NumFloorTex = 0;
	m_FloorTileFactor = 1;

	m_WireFlag = false;
	m_NoTexFlag = false;

	m_LightPosition = vector_3(0,4,4);
	m_LightDirection = vector_3(0,0.7071f,0.7071f);
	m_DrawLightRayFlag = false;
	m_DrawShadowsFlag = false;
	m_LightAngle = 0.0f;
	m_SpinLightsFlag = false;

	m_RenderNormalsFlag = false;
	m_RenderBonesFlag = false;
	m_RenderSettings = 0;

	m_DaVinciMode = false;
	m_PausedFlag = true;

	// detect from installer key in registry
	Registry registry( "Microsoft", "Microsoft Games", false );
	gpwstring installPath = registry.GetWString( "DungeonSiege/1.0/EXE Path" );
	if ( !installPath.empty() )
	{
		gConfig.SetPathVar( "ds_exe_path", installPath, Config::PVM_REQUIRED | Config::PVM_ERROR_ON_FAIL );
	}

	//----- Initialize FuBi
	{
		// build it
		stdx::make_auto_ptr( m_FuBiSysExports, new FuBi::SysExports );

		// set default dsdll path
		gConfig.SetPathVar( "dsdll_path", "%exe_path%", Config::PVM_USE_CONFIG );

		// import bindings from self
		m_FuBiSysExports->ImportBindings( NULL, NULL, false );

		// finish it off
		m_FuBiSysExports->ImportFinish();

		// what did we find?
		GPDEV_ONLY( m_FuBiSysExports->DumpStatistics() );
	}

	//----- Initialize FileSys
	{
		// create a tank mgr
		m_TankMgr = stdx::make_auto_ptr( new TankMgr );
		if ( !m_TankMgr->Init() )
		{
			return ( false );
		}
	}

	//----- FuelSys

	{
		stdx::make_auto_ptr( m_FuelSys, new FuelSys );

		gpstring outPath = gConfig.ResolvePathVars( "%out%", Config::PVM_NONE );

		// get text fuel options
		FuelDB::eOptions options = FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ;

		// init text fuel DB
		BinFuel::SetDictionaryPath( "" );
		FuelDB * pFuelDB = gFuelSys.AddTextDb( "default" );
		pFuelDB->Init( options, "", outPath );

		// get binary fuel options
		options = FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ;
		options |= FuelDB::OPTION_NO_WRITE_LIQUID;

		// init binary fuel DB
		BinaryFuelDB * dbA = gFuelSys.AddBinaryDb( "default_binary" );
		dbA->Init( options, "", outPath );

		if ( LocMgr::DoesSingletonExist() )
		{
			gLocMgr.LoadGasTranslations();
		}

#		if !GP_RETAIL
		FastFuelHandle fortune( "config:fortune:fortune:general" );
		if ( fortune )
		{
			FastFuelFindHandle finder( fortune );
			if ( finder.FindFirstValue( "*" ) )
			{
				gpstring str;
				int index = Random( finder.GetKeyAndValueCount() - 1 );
				while ( index-- >= 0 )
				{
					finder.GetNextValue( str );
				}

				gpstring name( SysInfo::GetUserName() );
				size_t space = name.find_first_of( " " );
				if ( space != name.npos )
				{
					name.erase( space );
				}
				name.to_lower();
				name.at_split( 0 ) = (char)toupper( name[ 0 ] );

				gGenericContext.OutputF( "\nYour fortune cookie, %s:\n\n", name.c_str() );
				ReportSys::AutoIndent autoIndent( gGenericContext );
				gGenericContext.OutputF( "%s\n\n", stringtool::EscapeTokensToCharacters( str ).c_str() );

				fortune.GetParent().GetParent().Unload();
			}
		}
#		endif // !GP_RETAIL
	}

	gConfig.DumpPathVars( &gGenericContext );

	m_NamingKey = stdx::make_auto_ptr(new NamingKey( "art/NamingKey.NNK" ));

	//----- Skrit
	{
		m_SkritEngine = stdx::make_auto_ptr( new Skrit::Engine );
	}

	//----- Nema
	{
		m_NemaAspectStorage = stdx::make_auto_ptr(new nema::AspectStorage);
		m_NemaKeyMgr = stdx::make_auto_ptr(new nema::PRSKeysStorage);
	}

	gDefaultRapi.SetSurfaceExpirationTime(0);
	gDefaultRapi.EnableAlphaBlending( true );
					
	// Create the env map
	gpstring texfilename;
	gpstring createerrors;
	gNamingKey.BuildIMGLocation("b_em_sphere",texfilename);
	m_EnvironmentTex = gDefaultRapi.CreateTexture( texfilename, true, 1, TEX_LOCKED, true, &createerrors );
	if (!RapiCreateTextureGeneratedErrors(createerrors)) {
		gDefaultRapi.SetEnvironmentTexture( m_EnvironmentTex );
	} else {
		m_EnvironmentTex = 0;
	}

	m_PreviewLoad = false;
	m_QuickViewMesh = false;
	m_QuickViewAnim = false;


	AnimFName = "";

	m_UsingLocalData = false;

	if ( gConfig.GetBool( "PREVIEW",false) )
	{
		m_PreviewLoad = true;
	}
	else if ( gConfig.HasKey("QUICKVIEW") )
	{
		gpstring sdata = gConfig.GetString( "QUICKVIEW" );

		stringtool::RemoveExtension(sdata);

		ParseQuickViewArg(sdata);
	}
	else
	{
		m_QuickViewMesh = m_QuickViewAnim = false;

		stringtool::CommandLineExtractor cle;
		CommandLine cl;
		stringtool::CommandLineExtractor::ConstIter i, ibegin = cle.Begin(), iend = cle.End();

		for ( i = ibegin ; i != iend ; ++i )
		{
			if (ParseQuickViewArg(*i))
			{
				break;
			}
		}
	}

	if (!LoadData())
	{
		return false;
	}

	m_HelpFile = gConfig.ResolvePathVars( "%exe_path%/DSAnimViewer.chm", Config::PVM_NONE );

	gpgeneric("DSAnimViewer Initialized!\n");

	return true;

}

bool Viewer::OnShutdown ( void ) {

	delete m_GpProfiler;

	if ( Aspect0.IsValid() )
	{
		Aspect0->DetachAllChildren();
		gAspectStorage.ReleaseAspect(Aspect0);
	}
	if ( Aspect1.IsValid() )
	{
		gAspectStorage.ReleaseAspect(Aspect1);
	}
	if ( Aspect3.IsValid() )
	{
		gAspectStorage.ReleaseAspect(Aspect3);
	}
	if ( AspectHead.IsValid() )
	{
		gAspectStorage.ReleaseAspect(AspectHead);
	}
	if ( AspectHand.IsValid() )
	{
		gAspectStorage.ReleaseAspect(AspectHand);
	}
	if ( AspectBody.IsValid() )
	{
		gAspectStorage.ReleaseAspect(AspectBody);
	}
	if ( AspectFoot.IsValid() )
	{
		gAspectStorage.ReleaseAspect(AspectFoot);
	}

	if (m_EnvironmentTex)
	{
		gDefaultRapi.DestroyTexture(m_EnvironmentTex);
	}
	if (m_FloorTexID[0])
	{
		gDefaultRapi.DestroyTexture(m_FloorTexID[0]);
	}

	ForceEndOfFrameFlush();

	delete ( m_NamingKey		.release() );
	delete ( m_NemaAspectStorage.release() );
	delete ( m_NemaKeyMgr     	.release() );
	delete ( m_SkritEngine		.release() );
	delete ( m_FuelSys			.release() );

	return true;

}

bool Viewer::OnMouseFrameDelta( int deltaX, int deltaY ) {

	if (!m_IgnoreMouseFlag) {
		SUI_UpdateMouseDeltaX(deltaX);
		SUI_UpdateMouseDeltaY(deltaY);
	}

	return true;

}

bool Viewer::OnLButtonDown( int x, int y, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	if (GetShiftKey())
	{
		SUI_CraneMode();
	}
	else
	{
		SUI_OrbitMode();
	}
	
	SetOptions( OPTION_MOUSE_FIXED, true );
	m_IgnoreMouseFlag = false;

	return true;

}

bool Viewer::OnLButtonUp( int x, int y, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	SUI_OrbitMode();
	SetOptions( OPTION_MOUSE_FIXED, false );
	m_IgnoreMouseFlag = true;

	return true;

}

bool Viewer::OnMButtonDown( int x, int y, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	if (GetShiftKey())
	{
		SUI_CraneMode();
	}
	else
	{
		SUI_ZoomMode();
	}

	SetOptions( OPTION_MOUSE_FIXED, true );
	m_IgnoreMouseFlag = false;

	return true;

}

bool Viewer::OnMButtonUp( int x, int y, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	SUI_OrbitMode();
	SetOptions( OPTION_MOUSE_FIXED, false );
	m_IgnoreMouseFlag = true;

	return true;

}

bool Viewer::OnRButtonDown( int x, int y, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	if (GetShiftKey())
	{
		SUI_ZoomMode();
	}
	else
	{
		SUI_DollyMode();
	}

	SetOptions( OPTION_MOUSE_FIXED, true );
	m_IgnoreMouseFlag = false;

	return true;

}

bool Viewer::OnRButtonUp( int x, int y, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	SUI_OrbitMode();
	SetOptions( OPTION_MOUSE_FIXED, false );
	m_IgnoreMouseFlag = true;

	return true;

}

bool Viewer::OnMouseWheel( int x, int y, int z, UINT keyFlags ) {

	UNREFERENCED_PARAMETER(x);
	UNREFERENCED_PARAMETER(y);
	UNREFERENCED_PARAMETER(keyFlags);

	if (z < 0) {
		SUI_UpdateMouseDeltaZ(100);
	} else {
		SUI_UpdateMouseDeltaZ(-100);
	}

	return true;
}

bool Viewer::OnKeyDown ( UINT vkey, int repeatCount, UINT flags ) {

	UNREFERENCED_PARAMETER(repeatCount);
	UNREFERENCED_PARAMETER(flags);

	bool success;

	if  (vkey == Keys::MakeInt(Keys::KEY_BACKSLASH)) {
		m_DrawLightRayFlag = !m_DrawLightRayFlag;
		return true;

	} else if (vkey == Keys::MakeInt(Keys::KEY_MINUS)) {
		m_TimeScale -= 0.1f;
		if (m_TimeScale < 0.1f) {
			m_TimeScale = 0.1f;
		}
		return true;

	} else if (vkey == Keys::MakeInt(Keys::KEY_EQUALS)) {
		m_TimeScale += 0.1f;
		if (m_TimeScale > 3.0f) {
			m_TimeScale = 3.0f;
		}
		return true;

	}

	switch (vkey) {

		case VK_SHIFT:
			
			if (GetMButton())
			{
				Viewer::OnMButtonDown(0,0,0);
			}

			if (GetLButton()) 
			{
				Viewer::OnLButtonDown(0,0,0);
			}

			if (GetRButton()) 
			{
				Viewer::OnRButtonDown(0,0,0);
			}
			break;
			
		case 'D':

			m_DaVinciMode = !m_DaVinciMode;

			if (Aspect0.IsValid())
			{
				if (m_DaVinciMode)
				{
					Aspect0->Purge();
					Aspect0->Reconstitute(0,true);
				}
			}


			break;


		case VK_SPACE:
			m_PausedFlag = !m_PausedFlag;
			if (!m_PausedFlag)
			{
				m_DaVinciMode = false;
			}
			break;


		case VK_LEFT  :
			SUI_UpdateMouseDeltaX(-10);
			break;

		case VK_RIGHT :
			SUI_UpdateMouseDeltaX(10);
			break;

		case VK_UP  :
			SUI_UpdateMouseDeltaZ(-100);
			break;

		case VK_DOWN :
			SUI_UpdateMouseDeltaZ(100);
			break;

		case VK_F1 :
		case 'H' :
			{
				// Open the help file
				HtmlHelp(GetDesktopWindow(),m_HelpFile.c_str(),HH_DISPLAY_TOPIC,NULL);
			}
			break;

		case VK_F3 :
			break;

		case VK_F4 :
			break;

		case VK_F5 :
			break;

		case VK_F6 :
			break;

		case VK_F7 :
			break;

		case VK_BACK :
			{
				m_TimeScale = 1.0f;
			}
			break;

		case 'J':
			m_Jogging = !m_Jogging;
			break;

		case 'R':
			m_DrawRootFlag = !m_DrawRootFlag;
			break;

		case 'U':
			{
			// Reload the textures;

			if (Aspect0.IsValid() )
			{
				for (DWORD i = 0; i <Aspect0->GetNumSubTextures(); i++) 
				{

					Aspect0->SetTextureCreatedByExternalSource(i,0);
					ForceEndOfFrameFlush();

/*
						gpstring texturename;
						gpstring texfilename;

						// See if there is an override texture on the command line
						gpstring textureparam;
						textureparam.assignf("TEXTURE%d",i);
						texturename = gConfig.GetString( textureparam );
						if ( texturename.empty() )
						{
							texturename = Aspect0->GetDefaultTextureString(i);
						}

						gNamingKey.BuildIMGLocation(texturename,texfilename);
						SetTextureHelper(Aspect0,i,texfilename);
*/
					gpstring texturename;
					gpstring textureparam;
					gpstring texfilename;
					gpstring createerrors;
					DWORD texid = 0;

					textureparam.assignf("TEXTURE%d",i);
					texturename = gConfig.GetString( textureparam );
					if ( !texturename.empty() )
					{
						gNamingKey.BuildIMGLocation(texturename,texfilename);
						SetTextureHelper(Aspect0,i,texfilename);
						if (FileSys::DoesResourceFileExist(texfilename))
						{
							Aspect0->SetTextureFromFile(i,texfilename.c_str());
						}
					}

					if (!texid || RapiCreateTextureGeneratedErrors(createerrors))
					{
						texturename = Aspect0->GetDefaultTextureString(i);
						gNamingKey.BuildIMGLocation(texturename,texfilename);
						texid = gDefaultRapi.CreateTexture(texfilename,true,0,TEX_LOCKED,true,&createerrors);
					}


					if (RapiCreateTextureGeneratedErrors(createerrors))
					{
						MessageBox(0,createerrors.c_str(),"RAPI Error",MB_OK|MB_ICONEXCLAMATION);
						gNamingKey.BuildIMGLocation("b_i_glb_placeholder",texfilename);
						texid = gDefaultRapi.CreateTexture(texfilename,true,0,TEX_LOCKED);
					}

					// Transfer ownership of texid to the aspect
					Aspect0->SetTextureCreatedByExternalSource(i,texid);
					gDefaultRapi.DestroyTexture(texid);
				}
			}
			break;
		}

		case 'X':
			m_FollowAspect0Flag = !m_FollowAspect0Flag;
			break;

		case 'L':
			m_WireFlag = !m_WireFlag;
			break;

		case 'T':
			m_NoTexFlag = !m_NoTexFlag;
			break;

		case 'Z':
			if (m_NoTexFlag)
			{
				m_NoTexFlag = false;
				m_SpinLightsFlag = false;
				m_DrawLightRayFlag = false;
			}
			else
			{
				m_NoTexFlag = true;
				m_SpinLightsFlag = true;
				m_DrawLightRayFlag = true;
			}
			break;

		case 'G':
			SUI_SetCameraAzimuth(45.0f*(RealPi/180.0f));
			SUI_SetCameraDistance(11.0f);
			SUI_SetCameraTarget(vector_3(0,0,0));
			break;

		case 'M': {

			// Load new model

			nema::HAspect newasp;

			success = PrepMesh(newasp,Mesh0FName,Mesh0FName,"Select MAIN mesh");
			if (success && newasp.IsValid()) {


				//bool wasupdating = Aspect0.IsValid() && Aspect0->IsUpdateEnabled();

				if (Aspect0.IsValid())
				{
					Aspect0->DetachAllChildren();
					gAspectStorage.ReleaseAspect(Aspect0);
					ForceEndOfFrameFlush();
				}

				Aspect0 = newasp;
			
				/*
				PrepAnim(Aspect0,Mesh0FName,"anim0","Select Anim for MAIN mesh");

				if (wasupdating)
				{
					newasp->EnableUpdate();
				}
				*/

				vector_3 vc, vhd;
				Aspect0->GetBoundingBox(vc,vhd);
				SUI_SetCameraHeight(-(vc.y+vhd.y*.333f));

				SetWindowTitle();

			}

			break;
		}

		case 'W': {

			// Load new weapon

			nema::HAspect newasp;

			success = PrepMesh(newasp,Mesh1FName,Mesh1FName,"Select WEAPON mesh");

			if (success && newasp.IsValid()) {
				if (Aspect1.IsValid()) {
					if (Aspect1->GetParent().IsValid()) {
						Aspect1->GetParent()->DetachChild(Aspect1);
					}
					gAspectStorage.ReleaseAspect(Aspect1);
					ForceEndOfFrameFlush();
				}
				Aspect1 = newasp;
				//PrepAnim(Aspect1,Mesh1FName,"anim1");
			}

			if (success) {
				int ind;
				if (Aspect1->GetBoneIndex("grip",ind))
				{
					Aspect0->AttachRigidLinkedChild(Aspect1,"weapon_grip","grip");
				}
				else if (Aspect1->GetBoneIndex("AP_grip",ind))
				{
					Aspect0->AttachReverseLinkedChild(Aspect1,"weapon_grip","AP_grip");
				}

			}

			break;
		}

		case 'S': {

			// Load new shield

			nema::HAspect newasp;

			success = PrepMesh(newasp,Mesh3FName,Mesh3FName,"Select SHIELD mesh");
			if (success && newasp.IsValid()) {
				if (Aspect3.IsValid()) {
					if (Aspect3->GetParent().IsValid()) {
						Aspect3->GetParent()->DetachChild(Aspect3);
					}
					gAspectStorage.ReleaseAspect(Aspect3);
					ForceEndOfFrameFlush();
				}
				Aspect3 = newasp;

				//PrepAnim(Aspect3,Mesh3FName,"anim3");

			}

			if (success) {
				Aspect0->AttachRigidLinkedChild(Aspect3,"shield_grip","grip");
			}

			break;
		}

		case 'C': {
/*
			// Load new suit of armour BODY

			nema::HAspect newasp;

			success = PrepMesh(newasp,MeshBodyFName,MeshBodyFName,"Select ARMOR SUIT mesh");
			if (success && newasp.IsValid()) {
				if (AspectBody.IsValid()) {
					if (AspectBody->GetParent().IsValid()) {
						AspectBody->GetParent()->DetachChild(AspectBody);
					}
					gAspectStorage.ReleaseAspect(AspectBody);
					ForceEndOfFrameFlush();
				}
				AspectBody = newasp;
			}

			if (success) 
			{
				Aspect0->AttachDeformableChild(AspectBody);
				Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH0);
				AspectBody->SetInstanceAttrFlags(nema::HIDE_SUBMESH1|nema::HIDE_SUBMESH2|nema::HIDE_SUBMESH3);
			}

*/
			break;
		}

		case 'E': {
/*
			// Load new suit of armour HEAD

			nema::HAspect newasp;

			success = PrepMesh(newasp,MeshHeadFName,MeshHeadFName,"Select HELMET mesh");
			if (success && newasp.IsValid()) {
				if (AspectHead.IsValid()) {
					if (AspectHead->GetParent().IsValid()) {
						AspectHead->GetParent()->DetachChild(AspectHead);
					}
					gAspectStorage.ReleaseAspect(AspectHead);
					ForceEndOfFrameFlush();
				}
				AspectHead = newasp;
			}

			if (success)
			{
				Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH1);
				Aspect0->AttachDeformableChild(AspectHead);
			}

			// Load new suit of armour HAND

			success = PrepMesh(newasp,MeshHandFName,MeshHandFName,"Select GAUNTLET mesh");
			if (success && newasp.IsValid()) {
				if (AspectHand.IsValid()) {
					if (AspectHand->GetParent().IsValid()) {
						AspectHand->GetParent()->DetachChild(AspectHand);
					}
					gAspectStorage.ReleaseAspect(AspectHand);
					ForceEndOfFrameFlush();
				}
				AspectHand = newasp;
			}

			if (success)
			{
				Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH2);
				Aspect0->AttachDeformableChild(AspectHand);
			}

			// Load new suit of armour FOOT

			success = PrepMesh(newasp,MeshFootFName,MeshFootFName,"Select BOOT mesh");
			if (success && newasp.IsValid()) {
				if (AspectFoot.IsValid()) {
					if (AspectFoot->GetParent().IsValid()) {
						AspectFoot->GetParent()->DetachChild(AspectFoot);
					}
					gAspectStorage.ReleaseAspect(AspectFoot);
					ForceEndOfFrameFlush();
				}
				AspectFoot = newasp;
			}

			if (success)
			{
				Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH3);
				Aspect0->AttachDeformableChild(AspectFoot);
			}

			break;
*/
		}

		case 'P':
/*
			if (Aspect0->GetNumberOfChildren() > 0)
			{
				// Get rid of everything we are holding
				if (Aspect0.IsValid()) {
					Aspect0->DetachAllChildren();
					Aspect0->ClearInstanceAttrFlags(nema::HIDE_SUBMESH0|nema::HIDE_SUBMESH1|nema::HIDE_SUBMESH2|nema::HIDE_SUBMESH3);
				}
			} else {
				if (Aspect0.IsValid()) {
					// Pick up everything we can
					if (Aspect1.IsValid()) {
						int ind;
						if (Aspect1->GetBoneIndex("grip",ind)) {
							Aspect0->AttachRigidLinkedChild(Aspect1,"weapon_grip","grip");
						} else if (Aspect1->GetBoneIndex("AP_grip",ind)) {
							Aspect0->AttachReverseLinkedChild(Aspect1,"weapon_grip","AP_grip");
						}
					}
					if (AspectHead.IsValid()) {
						Aspect0->AttachDeformableChild(AspectHead);
						Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH1);
					}
					if (AspectBody.IsValid()) {
						Aspect0->AttachDeformableChild(AspectBody);
						Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH0);
					}
					if (AspectHand.IsValid()) {
						Aspect0->AttachDeformableChild(AspectHand);
						Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH2);
					}
					if (AspectFoot.IsValid()) {
						Aspect0->AttachDeformableChild(AspectFoot);
						Aspect0->SetInstanceAttrFlags(nema::HIDE_SUBMESH3);
					}
					if (Aspect3.IsValid()) {
						Aspect0->AttachRigidLinkedChild(Aspect3,"shield_grip","grip");
					}
				}
			}
*/
			break;

		case 'A':

			// Load new animation
			PrepAnim(Aspect0,Mesh0FName,"anim0","Select Anim for MAIN mesh");
			SetWindowTitle();

			break;

		case 'O':

			m_SpinLightsFlag = !m_SpinLightsFlag;

			break;

		case 'N':

			m_RenderNormalsFlag = !m_RenderNormalsFlag;
			if (m_RenderNormalsFlag) {
				m_RenderSettings |= nema::RENDER_NORMALS;
			} else {
				m_RenderSettings &= ~nema::RENDER_NORMALS;
			}

			break;

		case 'B':

			m_RenderBonesFlag = !m_RenderBonesFlag;
			if (m_RenderBonesFlag) {
				m_RenderSettings |= nema::RENDER_BONES;
				m_RenderSettings |= nema::RENDER_BOUNDBOXES;
			} else {
				m_RenderSettings &= ~nema::RENDER_BONES;
				m_RenderSettings &= ~nema::RENDER_BOUNDBOXES;
			}

			break;

		case 'F':

			if (GetControlKey())
			{
				m_FloorTileFactor++;
				if (m_FloorTileFactor > 10) {
					m_FloorTileFactor = 1;
				}

				gConfig.Set("floor_tile_factor", m_FloorTileFactor );

				for (DWORD i = 0; i < 8; i++) {
					floorplane[i].uv.u = floorplane[i].uv.u ? m_FloorTileFactor : 0.0f;
					floorplane[i].uv.v = floorplane[i].uv.v ? m_FloorTileFactor : 0.0f;
				}
				m_NoFloorFlag = false;
			}

			else if (GetShiftKey())
			{
				PrepFloor(false);
				m_NoFloorFlag = false;
			}

			else
			{
				m_NoFloorFlag = !m_NoFloorFlag;
			}

			break;

	}

	return true;

}

bool Viewer::OnKeyUp ( UINT vkey, int repeatCount, UINT flags )
{
	UNREFERENCED_PARAMETER(repeatCount);
	UNREFERENCED_PARAMETER(flags);
	
	switch (vkey)
	{
		case VK_SHIFT:
			
			if (GetMButton())
			{
				Viewer::OnMButtonDown(0,0,0);
			}

			if (GetLButton()) 
			{
				Viewer::OnLButtonDown(0,0,0);
			}

			if (GetRButton()) 
			{
				Viewer::OnRButtonDown(0,0,0);
			}

		break;
	}

	return true;
}


bool Viewer::OnFrame ( float frameDeltaTime, float /*actualDeltaTime*/ ) {

	float nearzdist = 0.25f;

	float delta_time;
	static float scrub_time = 0.0;

	delta_time = frameDeltaTime * m_TimeScale;
	scrub_time += frameDeltaTime;
	scrub_time = floorf(scrub_time);

	gDefaultRapi.SetGameElapsedTime(delta_time);

	gDefaultRapi.SetPerspectiveMatrix( RadiansFrom( 60 ) , nearzdist );


	nema::LightSource SunLight;

	SunLight.m_Direction	= m_LightDirection;
	SunLight.m_Type			= nema::NLS_INFINITE_LIGHT;
	SunLight.m_Color		= 0xFFFFFFFF;
	SunLight.m_Intensity	= 1.0f;

	if (m_Jogging && Aspect0.IsValid() && Aspect0->IsUpdateEnabled()) {
		double len = Aspect0->GetCurrentVelocity();

		m_JogAngle += ((float)len/m_JogRadius)*delta_time;
		if (m_JogAngle > 2*PI) {
			m_JogAngle -= 2*PI;
		}


	}

	if (m_SpinLightsFlag) {
		m_LightAngle += (2.0f/m_JogRadius)*delta_time;
		if (m_LightAngle > 2*PI) {
			m_LightAngle -= 2*PI;
		}
	}

	gDefaultRapi.SetViewMatrix(
		vector_3( 0.0f, 0.0f, -SUI_GetCameraDistance() ),
		vector_3( 0.0f, 0.0f, 0.0f ),
		vector_3( 0.0f, 1.0f, 0.0f ) );

	matrix_3x3	mat = matrix_3x3();

	gDefaultRapi.SetWorldMatrix( mat, vector_3( 0.0f, 0.0f, 0.0f ));

	const DWORD ambience(0xff808080);

	if (delta_time > 0.0f)
	{
		if (!m_PausedFlag)
		{
			// Animate

			if (Aspect0.IsValid())
			{

				if (!m_DaVinciMode)
				{
					Aspect0->UpdateAnimationStateMachine(delta_time);
					Aspect0->Deform();
				}
			}
			if (Aspect1.IsValid() && !Aspect1->GetParent())
			{
				Aspect1->UpdateAnimationStateMachine(delta_time);
				Aspect1->Deform();
			}
			if (AspectHead.IsValid() && !AspectHead->GetParent())
			{
				AspectHead->UpdateAnimationStateMachine(delta_time);
				AspectHead->Deform();
			}
			if (AspectBody.IsValid() && !AspectBody->GetParent())
			{
				AspectBody->UpdateAnimationStateMachine(delta_time);
				AspectBody->Deform();
			}
			if (AspectHand.IsValid() && !AspectHand->GetParent())
			{
				AspectHand->UpdateAnimationStateMachine(delta_time);
				AspectHand->Deform();
			}
			if (AspectFoot.IsValid() && !AspectFoot->GetParent())
			{
				AspectFoot->UpdateAnimationStateMachine(delta_time);
				AspectFoot->Deform();
			}
			if (Aspect3.IsValid() && !Aspect3->GetParent())
			{
				Aspect3->UpdateAnimationStateMachine(delta_time);
				Aspect3->Deform();
			}
		}

		// Apply lights

		if (Aspect0.IsValid())
		{
			Aspect0->InitializeLighting(ambience);
		}
		if (Aspect1.IsValid() && !Aspect1->GetParent())
		{
			Aspect1->InitializeLighting(ambience);
		}
		if (AspectHead.IsValid() && !AspectHead->GetParent())
		{
			AspectHead->InitializeLighting(ambience);
		}
		if (AspectBody.IsValid() && !AspectBody->GetParent())
		{
			AspectBody->InitializeLighting(ambience);
		}
		if (AspectHand.IsValid() && !AspectHand->GetParent())
		{
			AspectHand->InitializeLighting(ambience);
		}
		if (AspectFoot.IsValid() && !AspectFoot->GetParent())
		{
			AspectFoot->InitializeLighting(ambience);
		}
		if (Aspect3.IsValid() && !Aspect3->GetParent())
		{
			Aspect3->InitializeLighting(ambience);
		}
	}

	gDefaultRapi.SetWireframe(m_WireFlag);

	gDefaultRapi.Begin3D( );

	float mousex = -GetNormalizedCursorX();
	float mousey = GetNormalizedCursorY();

	float aspect_ratio = (float)GetGameHeight()/GetGameWidth();

	const float cursor_size = 0.025f;

	crosshair[0].x = nearzdist * ((0.5f * (mousex)) + cursor_size);
	crosshair[1].x = nearzdist * ((0.5f * (mousex)) - cursor_size);
	crosshair[2].x = nearzdist *   0.5f * (mousex);
	crosshair[3].x = nearzdist *   0.5f * (mousex);

	crosshair[0].y = nearzdist *   0.5f * (aspect_ratio * mousey);
	crosshair[1].y = nearzdist *   0.5f * (aspect_ratio * mousey);
	crosshair[2].y = nearzdist * ((0.5f * (aspect_ratio * mousey)) + cursor_size);
	crosshair[3].y = nearzdist * ((0.5f * (aspect_ratio * mousey)) - cursor_size);

	crosshair[0].z = crosshair[1].z = crosshair[2].z = crosshair[3].z
					= (-SUI_GetCameraDistance()+(nearzdist));

	gDefaultRapi.DrawPrimitive(
		D3DPT_LINELIST,
		(lVertex *)crosshair,	4,
		LVERTEX, 0,	0
	);

	// Note here that I am using the negative Azm/Orb. These are camera angles relative to
	// the ground plane (the frame of reference). We need to rotate the ground (World) so that
	// the Camera is located in the correct relative position
	gDefaultRapi.RotateWorldMatrix( XRotationColumns( -SUI_GetCameraAzimuth()) * YRotationColumns( SUI_GetCameraOrbit() + PI));

	gDefaultRapi.TranslateWorldMatrix( SUI_GetCameraTarget());

	if (m_FollowAspect0Flag && m_Jogging) {
		gDefaultRapi.TranslateWorldMatrix( -m_JogRadius*vector_3(Cos(m_JogAngle),0.0f,Sin(m_JogAngle)));
	}

	gDefaultRapi.TranslateWorldMatrix(-m_WorldPosition);

	gDefaultRapi.SetTexture(0,NULL);
	gDefaultRapi.SetTexture(1,NULL);

	gDefaultRapi.SetSingleStageState( 0, D3DTSS_COLOROP,	D3DTOP_DISABLE ) ;
	gDefaultRapi.SetSingleStageState( 0, D3DTSS_ALPHAOP,	D3DTOP_DISABLE ) ;

	gDefaultRapi.SetSingleStageState( 1, D3DTSS_COLOROP,	D3DTOP_DISABLE ) ;
	gDefaultRapi.SetSingleStageState( 1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE ) ;

	if (!m_NoFloorFlag) {

		if (m_NumFloorTex == 0) {

			gDefaultRapi.SetTextureStageState(	0,
											D3DTOP_SELECTARG2,
											D3DTOP_SELECTARG2,
											D3DTADDRESS_CLAMP,
											D3DTADDRESS_CLAMP,
											D3DTFG_LINEAR,
											D3DTFN_LINEAR,
											D3DTFP_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );
			gDefaultRapi.DrawPrimitive(
				D3DPT_TRIANGLESTRIP,
				(lVertex *)groundplane,	4,
				LVERTEX, 0,	0
				);

			gDefaultRapi.SetTextureStageState(	0,
											m_NoTexFlag ? D3DTOP_SELECTARG2 : D3DTOP_MODULATE,
											m_NoTexFlag ? D3DTOP_SELECTARG2 : D3DTOP_SELECTARG1,
											D3DTADDRESS_CLAMP,
											D3DTADDRESS_CLAMP,
											D3DTFG_LINEAR,
											D3DTFN_LINEAR,
											D3DTFP_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );
		} else {

			gDefaultRapi.SetTextureStageState(	0,
											D3DTOP_MODULATE,
											D3DTOP_SELECTARG1,
											D3DTADDRESS_WRAP,
											D3DTADDRESS_WRAP,
											D3DTFG_LINEAR,
											D3DTFN_LINEAR,
											D3DTFP_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );
			gDefaultRapi.DrawPrimitive(
				D3DPT_TRIANGLESTRIP,
				(lVertex *)floorplane,	4,	// 8,
				LVERTEX, (unsigned int *)m_FloorTexID, 1
				);

			gDefaultRapi.SetTextureStageState(	0,
											m_NoTexFlag ? D3DTOP_SELECTARG2 : D3DTOP_MODULATE,
											m_NoTexFlag ? D3DTOP_SELECTARG2 : D3DTOP_SELECTARG1,
											D3DTADDRESS_CLAMP,
											D3DTADDRESS_CLAMP,
											D3DTFG_LINEAR,
											D3DTFN_LINEAR,
											D3DTFP_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );
		}
	} else {

			gDefaultRapi.SetTextureStageState(	0,
											m_NoTexFlag ? D3DTOP_SELECTARG2 : D3DTOP_MODULATE,
											m_NoTexFlag ? D3DTOP_SELECTARG2 : D3DTOP_SELECTARG1,
											D3DTADDRESS_CLAMP,
											D3DTADDRESS_CLAMP,
											D3DTFG_LINEAR,
											D3DTFN_LINEAR,
											D3DTFP_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );
	}

	if (m_Jogging) {
		gDefaultRapi.TranslateWorldMatrix( m_JogRadius*vector_3(Cos(m_JogAngle),0.0f,Sin(m_JogAngle)));
	}

	vector_3 lightv = m_LightDirection;

	lightv = YRotationColumns(m_LightAngle) * lightv;

	lightv = m_WorldRotation.Inverse().RotateVector_T(lightv);

	SunLight.m_Direction = lightv;

	if (m_DrawLightRayFlag) {
		RP_DrawRay(gDefaultRapi,lightv,MAKEDWORDCOLOR(vector_3(1.0,0.0,1.0)));
	}

	if (m_Jogging) {
		lightv = YRotationColumns(m_JogAngle) * lightv;
		gDefaultRapi.RotateWorldMatrix(YRotationColumns(-m_JogAngle));
	}


	gDefaultRapi.TranslateWorldMatrix(m_WorldPosition);
	gDefaultRapi.RotateWorldMatrix(m_WorldRotation.BuildMatrix());

	if (Aspect0.IsValid())
	{
		if (m_DrawRootFlag)
		{
			RP_DrawOrigin(gDefaultRapi,0.15f);
		}

		if (m_NoTexFlag)
		{
			Aspect0->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			Aspect0->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}

		Aspect0->CalculateShading(SunLight, m_DrawLightRayFlag);
		Aspect0->Render(&gDefaultRapi,m_RenderSettings);
	}

	gDefaultRapi.TranslateWorldMatrix(vector_3(1,0,0));

	if (Aspect1.IsValid() && !Aspect1->GetParent()) {
		if (m_NoTexFlag)
		{
			Aspect1->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			Aspect1->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		Aspect1->CalculateShading(SunLight, m_DrawLightRayFlag);
		Aspect1->Render(&gDefaultRapi,m_RenderSettings);
	}

	gDefaultRapi.TranslateWorldMatrix(vector_3(-2,0,0));

	if (AspectHead.IsValid() && !AspectHead->GetParent()) {
		if (m_NoTexFlag)
		{
			AspectHead->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			AspectHead->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		AspectHead->CalculateShading(SunLight, m_DrawLightRayFlag);
		AspectHead->Render(&gDefaultRapi,m_RenderSettings);
	}
	if (AspectBody.IsValid() && !AspectBody->GetParent()) {
		if (m_NoTexFlag)
		{
			AspectBody->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			AspectBody->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		AspectBody->CalculateShading(SunLight, m_DrawLightRayFlag);
		AspectBody->Render(&gDefaultRapi,m_RenderSettings);
	}
	if (AspectHand.IsValid() && !AspectHand->GetParent()) {
		if (m_NoTexFlag)
		{
			AspectHand->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			AspectHand->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		AspectHand->CalculateShading(SunLight, m_DrawLightRayFlag);
		AspectHand->Render(&gDefaultRapi,m_RenderSettings);
	}
	if (AspectFoot.IsValid() && !AspectFoot->GetParent()) {
		if (m_NoTexFlag)
		{
			AspectFoot->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			AspectFoot->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		AspectFoot->CalculateShading(SunLight, m_DrawLightRayFlag);
		AspectFoot->Render(&gDefaultRapi,m_RenderSettings);
	}

	gDefaultRapi.TranslateWorldMatrix(vector_3(1,0,1));

	if (Aspect3.IsValid() && !Aspect3->GetParent()) {
		if (m_NoTexFlag)
		{
			Aspect3->SetInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		else
		{
			Aspect3->ClearInstanceAttrFlags(nema::RENDER_DISABLE_TEXTURE);
		}
		Aspect3->CalculateShading(SunLight, m_DrawLightRayFlag);
		Aspect3->Render(&gDefaultRapi,m_RenderSettings);
	}

	gDefaultRapi.End3D( );

	return true;

}

bool Viewer::PrepMesh(nema::HAspect& asp, gpstring namebuff, gpstring meshname, gpstring Title) {

	RapiPause pauser;

	bool success;

	if (m_PreviewLoad)
	{
		MeshPathBuff = gConfig.ResolvePathVars("%temp_path%/temp_siegemax_preview.asp", Config::PVM_NONE);
		if (asp.IsValid())
		{
			asp->DetachAllChildren();
			gAspectStorage.ReleaseAspect(asp);
			ForceEndOfFrameFlush();
		}
	}
	else if (m_QuickViewMesh)
	{
		// The buffer is already filled out
	}
	else
	{

		gpstring DefaultMeshDir = gConfig.ResolvePathVars("%out%/art/meshes", Config::PVM_NONE);

		MeshPathBuff.clear();
		success = SUI_GetFileName(Title,DefaultMeshDir,"Mesh Files","ASP",MeshPathBuff);
		if (!success)
		{
			return false;
		}
		namebuff = FileSys::GetFileNameOnly(MeshPathBuff);
		DefaultMeshDir = FileSys::GetPathOnly(MeshPathBuff);

	}

	asp = gAspectStorage.LoadAspect(MeshPathBuff,meshname);

	if (!asp.IsValid()) {
		return false;
	}

	if (m_PreviewLoad) {

		gpstring createerrors;
		for (DWORD i = 0; i <asp->GetNumSubTextures(); i++)
		{
			gpstring texnameIMG = gConfig.ResolvePathVars("%temp_path%/", Config::PVM_NONE);
			texnameIMG.appendf("temp_siegemax_preview%d.%%IMG%%",i);
			SetTextureHelper(asp, i, texnameIMG);
		}

	}
	else
	{

		for (DWORD i = 0; i <asp->GetNumSubTextures(); i++)
		{
			gpstring texturename;
			gpstring texfilename;

			// See if there is an override texture on the command line
			gpstring textureparam;
			textureparam.assignf("TEXTURE%d",i);
			texturename = gConfig.GetString( textureparam );
			if ( texturename.empty() )
			{
				texturename = asp->GetDefaultTextureString(i);
			}

			gNamingKey.BuildIMGLocation(texturename,texfilename);
			SetTextureHelper(asp,i,texfilename);
		}

	}

	asp->Reconstitute(0,true);

	asp->DisableUpdate();

	return true;
}

bool Viewer::PrepAnim(nema::HAspect& asp,gpstring namebuff, gpstring animname, gpstring Title)
{

	if (!asp.IsValid()) return false;

	RapiPause pauser;

	bool success;

	if (m_PreviewLoad)
	{
		AnimPathBuff = gConfig.ResolvePathVars("%temp_path%/temp_siegemax_preview.prs", Config::PVM_NONE);
		if (!FileSys::DoesResourceFileExist(AnimPathBuff))
		{
			return false;
		}

	}
	else if (m_QuickViewAnim)
	{
		// The buffer is already filled out
	}
	else if (m_QuickViewMesh)
	{
		return false;
	}
	else
	{
		AnimFName = asp->GetDefaultTextureString(0);

		AnimFName.replace(0,1,1,'a');
		AnimFName.to_upper();

		AnimFName.append("_*");

		gpstring animpath;
		gpstring AnimPathInitDir;

		if (gNamingKey.BuildContentPath(AnimFName.c_str(), animpath))
		{

			AnimPathInitDir = gConfig.ResolvePathVars( "%out%/"+animpath , Config::PVM_NONE);
			if (!FileSys::DoesResourcePathExist(AnimPathInitDir))
			{
				AnimPathInitDir.clear();
			}
		}

		if (AnimPathInitDir.empty())
		{
			AnimPathInitDir = gConfig.ResolvePathVars( "%out%/art/animations"+animpath , Config::PVM_NONE);
		}


		AnimPathBuff.clear();

		success = SUI_GetFileName(Title,AnimPathInitDir,"Anim Files","PRS",AnimPathBuff);
		if (!success)
		{
			AnimFName.clear();
			return false;
		}
		AnimFName = FileSys::GetFileNameOnly(AnimPathBuff);
	}

	Chore newChore;

	newChore.m_Verb = CHORE_DEFAULT;
	gpstring skritfile;
	skritfile.assignf("art\\animations\\skrits\\simple_loop");
	newChore.m_AnimSkritObject = gSkritEngine.CreateObject(skritfile.c_str());
	newChore.m_vsAnimFiles[0].push_back(AnimPathBuff);
	newChore.m_lStanceMask = 0xffffffff;

	asp->AddChore(&newChore);

	asp->SetNextChore(CHORE_DEFAULT);

	asp->EnableUpdate();


	return true;
}

bool Viewer::PrepFloor(bool preload)
{
	DWORD oldfloor = m_FloorTexID[0];

	gpstring FloorPath;
	gpstring FloorTexture;

	FloorPath = gConfig.GetString( "floor_texture", "" );
	FloorTexture = FileSys::GetFileNameOnly(FloorPath);

	if (FloorTexture.size() == 0)
	{
		FloorTexture = "b_t_grs01_flr_04x04-a";
		gNamingKey.BuildIMGLocation(FloorTexture.c_str(),FloorPath);
	}
	
	if (preload && FloorPath.size() > 0)
	{

		m_FloorTileFactor = gConfig.GetInt( "floor_tile_factor", 1 );

		for (DWORD i = 0; i < 8; i++)
		{
			floorplane[i].uv.u = floorplane[i].uv.u ? m_FloorTileFactor : 0.0f;
			floorplane[i].uv.v = floorplane[i].uv.v ? m_FloorTileFactor : 0.0f;
		}

		if ( FileSys::DoesResourceFileExist( FloorPath) )
		{
			gpstring createerrors;
			m_FloorTexID[0] = gDefaultRapi.CreateTexture(FloorPath.c_str(),true,0,TEX_LOCKED,true,&createerrors);
			if (m_FloorTexID[0])
			{
				m_NumFloorTex = 1;
				if (oldfloor) gDefaultRapi.DestroyTexture(oldfloor);
				return true;
			}
			else
			{
				m_FloorTexID[0] = oldfloor;
				return false;
			}
		}
		else
		{
			if (m_PreviewLoad)
			{
				return false;
			}
		}
	}


	gNamingKey.BuildContentPath("b_t_*", TexturePathBuff);

	TexturePathBuff = gConfig.ResolvePathVars("%out%/"+TexturePathBuff, Config::PVM_NONE);

	FloorTexture.clear();

	bool success = SUI_GetFileName("Select a FLOOR texture",TexturePathBuff,"Texture Files","PSD",FloorTexture);
	if (success)
	{
		gpstring createerrors;
		m_FloorTexID[0] = gDefaultRapi.CreateTexture(FloorTexture.c_str(),true,0,TEX_LOCKED,true,&createerrors);
		if (m_FloorTexID[0])
		{
			m_NumFloorTex = 1;

			TexturePathBuff = FloorTexture;

			gConfig.Set("floor_texture", TexturePathBuff );

			if (oldfloor) gDefaultRapi.DestroyTexture(oldfloor);
		}
		else
		{
			m_FloorTexID[0] = oldfloor;
			return false;
		}
	}
	else
	{
		m_FloorTexID[0] = oldfloor;
		return false;
	}

	return true;
}


bool Viewer::ParseQuickViewArg(gpstring sdata)
{
	m_QuickViewMesh = m_QuickViewAnim = false;

	const char* ext = FileSys::GetExtension( sdata );
	if (ext != NULL)
	{
		if ( ::same_no_case( ext, "PRS" ) )
		{
			// Look for an ASP that we can apply this PRS to...
			AnimPathBuff = sdata;
			MeshPathBuff.clear();

			if (same_no_case(AnimPathBuff,"a_",2))
			{
				Mesh0FName = FileSys::GetFileNameOnly(AnimPathBuff);

				size_t last_break = Mesh0FName.find_last_of("_");
				Mesh0FName.erase(last_break);

				Mesh1FName = Mesh0FName;

				Mesh0FName.append("_pos_a1");
				Mesh1FName.append("_pos_1");

				gNamingKey.BuildASPLocation(Mesh0FName,MeshPathBuff);
				m_QuickViewMesh = m_QuickViewAnim = FileSys::DoesResourceFileExist(MeshPathBuff);

				if (!m_QuickViewAnim)
				{
					Mesh0FName = Mesh1FName;
					gNamingKey.BuildASPLocation(Mesh0FName,MeshPathBuff);
					m_QuickViewMesh = m_QuickViewAnim = FileSys::DoesResourceFileExist(MeshPathBuff);
				}

			}
		}
		else if ( ::same_no_case( ext, "ASP" ) )
		{
			// Look for an ASP that we can apply this PRS to...
			MeshPathBuff = sdata;
			AnimPathBuff.clear();
			Mesh0FName = FileSys::GetFileNameOnly(MeshPathBuff);
			m_QuickViewMesh = FileSys::DoesResourceFileExist(MeshPathBuff);
		}
	}

	return m_QuickViewMesh;
}

void Viewer::SetWindowTitle()
{
	gpstring titlestring;

	if (m_PreviewLoad)
	{
		titlestring.assign("DSAnimViewer - [Preview Mode]");
	}
	else
	{
		if (m_UsingLocalData)
		{
			titlestring.assignf("DSAnimViewer - [Local Data] - %s : %s", Mesh0FName.c_str(), AnimFName.c_str());
		}
		else
		{
			titlestring.assignf("DSAnimViewer - [Bits Data] - %s : %s", Mesh0FName.c_str(), AnimFName.c_str());
		}
	}

	winx::SetWindowText( GetMainWnd() ,titlestring.c_str());
}

