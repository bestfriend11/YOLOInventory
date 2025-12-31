#include "precomp_services.h"
#include "services.h"

#include "appmodule.h"
#include "config.h"
#include "fubi.h"
#include "fubipersist.h"
#include "fuel.h"
#include "fueldb.h"
#include "gps_manager.h"
#include "lochelp.h"
#include "masterfilemgr.h"
#include "namingkey.h"
#include "nema_aspect.h"
#include "nema_aspectmgr.h"
#include "nema_keymgr.h"
#include "pathfilemgr.h"
#include "rapiowner.h"
#include "rapimouse.h"
#include "reportsys.h"
#include "siege_compass.h"
#include "siege_console.h"
#include "siege_engine.h"
#include "siege_options.h"
#include "skritbotmgr.h"
#include "skritengine.h"
#include "skritsupport.h"
#include "stdhelp.h"
#include "stringtool.h"
#include "tankfilemgr.h"
#include "tankmgr.h"
#include "ui.h"
#include "winx.h"
#include "worldversion.h"


//-------------------- debug output objects -----------------------
//
// these are used to give Tattoo-specific places for debug output to go
// when gpwarning() gpwarning() or gpgeneric() are called
//

#if !GP_RETAIL

class DevConsoleSink : public ReportSys::StreamSink
{
public:
	SET_INHERITED( DevConsoleSink, ReportSys::StreamSink );

	// ctor/dtor
	DevConsoleSink( siege::Console* console )
		: Inherited( "DevConsole" ), m_Console( console )
	{
		gpassert( m_Console != NULL );
		SetType( "DevConsole" );
	}

	virtual ~DevConsoleSink( void )
		{  }

	// overrides
	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len,
							   bool startLine, bool endLine )
	{
		if ( startLine )
		{
			m_Buffer.assign( GetIndentSpaces( context ), ' ' );
		}

		m_Buffer.append( text, len );		// $$$ this stupid code will go away when console is rewritten

		if ( endLine )
		{
			m_Console->Print( m_Buffer );
		}

		return ( true );
	}

private:
	siege::Console* m_Console;
	gpstring        m_Buffer;

	SET_NO_COPYING( DevConsoleSink );
};

#endif // !GP_RETAIL

struct FatalHandler : ReportSys::MultiSink
{
	virtual void OnBeginReport( const ReportSys::Context* /*context*/ )
	{
		if ( RapiOwner::DoesSingletonExist() )
		{
			// permanently pause - we're fataling anyway
			gRapiOwner.Pause( true );
		}
	}
};

//----- end debug output objects ------------------------------



FOURCC Services::msAppType = 0;



Services::Services( void )
{
	// this space intentionally left blank...
}

Services::~Services()
{
/*
	if (mSound) {
		mSound->SaveConfig();
	}
*/
	//----- delete singletons explicitly rather than let them get auto-deleted - reorder as necessary

	delete( mNamingKey         .release() );
#	if GP_ENABLE_SKRITBOT
	delete( mSkritBotMgr       .release() );
#	endif // GP_ENABLE_SKRITBOT
	delete( mNemaAspectStorage .release() );
	delete( mNemaKeyMgr        .release() );
	delete( mSoundManager      .release() );
	delete( mRenderer          .release() );
	delete( mOutputFileSink    .release() );
#	if !GP_RETAIL
	delete( mOutputConsoleSink .release() );
	delete( mTempStatsConsole  .release() );
	delete( mWatchConsole      .release() );
	delete( mFooterConsole     .release() );
#	endif // !GP_RETAIL
	delete( mOutputConsole     .release() );
	delete( mConsoleFont       .release() );
	delete( mFuelSys           .release() );
}

bool Services::InitServices( FOURCC app )
{
	SetAppType( app );

	// build a simple font
	winx::LogFont logFont( "terminal", 7 );
	logFont.lfCharSet = OEM_CHARSET;
	winx::Font font;
	if ( !font.Create( logFont ) )
	{
		font.CreateStock();
	}

	// get our default console font
	stdx::make_auto_ptr( mConsoleFont, gDefaultRapi.CreateFont( font, true ) );

	//----- display console - a display console so that we can display progress early on...
	{
		// TODO: I added this quickly for Steve, so someone should probably clean
		// up this font stuff.  $$$ make font size and face configurable per console...

		// main output console
		stdx::make_auto_ptr( mOutputConsole, new siege::Console( GetConsoleFont() ) );
		mOutputConsole->SetSize( 1.0f, 1.0f );
		mOutputConsole->SetPosition( 0, 0 );
		mOutputConsole->SetVisibility( true );

#		if GP_RETAIL

		mOutputConsole->SetInputLineVisibility( false );
		mOutputConsole->SetRenderOutline( false );

#		else // GP_RETAIL

		// footer
		stdx::make_auto_ptr( mFooterConsole, new siege::Console( GetConsoleFont() ) );
		//mFooterConsole->SetSize( 1.0f, 0.2f );
		//mFooterConsole->SetPosition( 0, 0.8f );
		mFooterConsole->SetVisibility( false );
		mFooterConsole->SetRenderFilled( true );
		mFooterConsole->SetFillColor( 0xD0221F70 );

		// debug watch probe
		stdx::make_auto_ptr( mWatchConsole, new siege::Console( GetConsoleFont() ) );
		mWatchConsole->SetSize( 1.0f, 0.8f );
		mWatchConsole->SetPosition( 0, 0 );
		mWatchConsole->SetVisibility( false );

		// temp stats console
		stdx::make_auto_ptr( mTempStatsConsole, new siege::Console( GetConsoleFont() ) );
		mTempStatsConsole->SetSize( (1.0f - 0.406f) / 2.0f, (0.971f - 0.631f) / 2.0f );
		mTempStatsConsole->SetPosition( (1.0f + 0.400f) / 4.0f, (-0.971f - 0.631f) / 4.0f );
		mTempStatsConsole->SetVisibility( true );

		// misc settings
		mOutputConsole->SetInputLineVisibility(true);
		mFooterConsole->SetInputLineVisibility(false);
		mWatchConsole->SetInputLineVisibility(false);
		mTempStatsConsole->SetInputLineVisibility(false);

#		endif // GP_RETAIL
	}

	//--------------------------------------------------------------------
	// report system services
	//--------------------------------------------------------------------
	{
		using namespace ReportSys;

#		if !GP_RETAIL

		// get computer name
		gpstring computerName( SysInfo::GetComputerName() );
		computerName.to_lower();

		// init user feedback message sinks
		stdx::make_auto_ptr( mOutputConsoleSink, new DevConsoleSink( mOutputConsole.get() ) );
		gGlobalSink.AddSink( mOutputConsoleSink.get(), false );

		// init mirroring file sink (this is optional)
		gpstring fileName;
		fileName.assignf( "log-%s.log", computerName.c_str() );
		GPDEV_ONLY( fileName.insert( 0, "dev-" ) );
		GPRETAIL_ONLY( DrWatson::RegisterFileForReport( fileName ) );
		stdx::make_auto_ptr( mOutputFileSink, new ReportSys::LogFileSink <> ( fileName, GP_RETAIL, "OutputLog" ) );
		mOutputFileSink->Enable( gConfig.GetBool( "log_all", false ) );
		gGlobalSink.AddSink( mOutputFileSink.get(), false );

		// register warning file logging
		fileName.assignf( "warnings-%s.log", computerName.c_str() );
		GPDEV_ONLY( fileName.insert( 0, "dev-" ) );
		GPRETAIL_ONLY( DrWatson::RegisterFileForReport( fileName ) );
		gWarningContext.AddSink( new ReportSys::LogFileSink <> ( fileName, GP_RETAIL, "WarningLog" ), true );

		// register error file logging
		fileName.assignf( "errors-%s.log", computerName.c_str() );
		GPDEV_ONLY( fileName.insert( 0, "dev-" ) );
		GPRETAIL_ONLY( DrWatson::RegisterFileForReport( fileName ) );
		gErrorContext.AddSink( new ReportSys::LogFileSink <> ( fileName, GP_RETAIL, "ErrorLog" ), true );

		// register fatal callback to flip to GDI - note that we disable then
		// reenable the fatal exception to push it to the back of the list
		fileName.assignf( "fatals-%s.log", computerName.c_str() );
		GPDEV_ONLY( fileName.insert( 0, "dev-" ) );
		GPRETAIL_ONLY( DrWatson::RegisterFileForReport( fileName ) );
		ReportSys::EnableFatalException( false );
		gFatalContext.AddSink( new ReportSys::LogFileSink <> ( fileName, GP_RETAIL, "FatalLog" ), true );
		gFatalContext.AddSink( new FatalHandler, true );
		ReportSys::EnableFatalException( true );

#		else // !GP_RETAIL

		// register fatal callback to flip to GDI - note that we disable then
		// reenable the fatal exception to push it to the back of the list
		ReportSys::EnableFatalException( false );
		gFatalContext.AddSink( new FatalHandler, true );
		ReportSys::EnableFatalException( true );

#		endif // !GP_RETAIL
	}

	gpgenericf(( "\n\n\n\n\n\n\n\n=========================== NEW SESSION ===========================\n\n"
				 "%S initializing\n%s\n\n",
				 gAppModule.GetAppName().c_str(), SysInfo::MakeExeSessionInfo().c_str() ));


	//--------------------------------------------------------------------
	// "lower level" services
	//--------------------------------------------------------------------


	//----- Initialize ScopeTimer
	{
		// **** ScopeTimer can be used only after this point... -BK
		ScopeTimer initTimer( gDefaultRapiPtr, mOutputConsole.get() );
	}

	//----- Initialize FuBi
	{
		ScopeTimer timer( "init FuBi" );

		// build it
		stdx::make_auto_ptr( mFuBiSysExports, CreateFuBi() );

		// what did we find?
		GPDEV_ONLY( mFuBiSysExports->DumpStatistics() );
	}

	//----- Initialize FileSys
	{
		ScopeTimer timer( "init FileSys" );

		// create a tank mgr
		mTankMgr = stdx::make_auto_ptr( new TankMgr );
		if ( !mTankMgr->Init() )
		{
			return ( false );
		}

		// add copy local types. currently only lnc needed because it's kept
		// open while in use, yet it changes constantly due to level designer
		// checkin (unlike sound files).
		FileSys::IFileMgr::AddCopyLocalType( ".lnc" );
	}

	//----- FuelSys

	{
		ScopeTimer timer( "init FuelSys" );

		stdx::make_auto_ptr( mFuelSys, new FuelSys );

		gpstring outPath = gConfig.ResolvePathVars( "%out%", IsEditor() ? Config::PVM_ERROR_ON_FAIL : Config::PVM_NONE );
		if ( outPath.empty() && IsEditor() )
		{
			// if we're the editor, a bad out path is impossible to work with
			return ( false );
		}

		// get text fuel options
		FuelDB::eOptions options = FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ;
		if ( IsEditor() )
		{
			// if editor, we will need to be able to write out .gas files
			options |= FuelDB::OPTION_WRITE;
		}

		// init text fuel DB
		BinFuel::SetDictionaryPath( "" );
		FuelDB * pFuelDB = gFuelSys.AddTextDb( "default" );
		pFuelDB->Init( options, "", outPath );

		// get binary fuel options
		options = FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ;
		if ( outPath.empty() )
		{
			// if no 'out' path is available, don't write lqd files
			options |= FuelDB::OPTION_NO_WRITE_LIQUID;
		}

		// disable writing out lqd if we're running off tanks
		if( mTankMgr->HasAnyTanks() )
		{
			options |= FuelDB::OPTION_NO_WRITE_LIQUID;
		}

		// special editor options
		if ( !IsDungeonSiege() && !gConfig.IsDevMode() && !gConfig.GetBool( "unsafe", false ) )
		{
			// we don't want to mess with .lqd files! we've had too many
			// problems with the community using SE and having mods installed
			// screwing up everything, so the .lqd system breaks down there.
			// the .lqd will be generated using the dialogs, though, when
			// creating tanks.
			options |= FuelDB::OPTION_NO_WRITE_LIQUID | FuelDB::OPTION_NO_READ_LIQUID;
		}

		// init binary fuel DB
		BinaryFuelDB * dbA = gFuelSys.AddBinaryDb( "default_binary" );
		dbA->Init( options, "", outPath );

		// optionally delete all lqd files
		bool deleteLqd = gConfig.GetBool( "deletelqd" );
		if( deleteLqd )
		{
			gpgeneric( "Deleting all LQD files...\n" );
			dbA->DeleteAllLqdFiles();
		}
		
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

	// build system detail driver
	BinaryFuelDB* detailDb = NULL;
	{
		std::auto_ptr <FileSys::PathFileMgr> pathFileMgr( new FileSys::PathFileMgr );
		if ( pathFileMgr->SetRoot( gConfig.ResolvePathVars( "%home%" ) ) )
		{
			// install it
			FileSys::MasterFileMgr::GetMasterFileMgr()->AddDriver( pathFileMgr.release(), true, "system_detail" );

			// build system_detail fuel db
			detailDb = gFuelSys.AddBinaryDb( "system_detail" );
			detailDb->Init(   FuelDB::OPTION_READ
						    | FuelDB::OPTION_JIT_READ
						    | FuelDB::OPTION_NO_WRITE_LIQUID
							| FuelDB::OPTION_SIMPLE_ABORT_ON_FAIL,
						    "system_detail://" );
		}
	}

	//----- System detail
	{
		SystemDetailList sysDetList;

		FastFuelHandle detail( "::system_detail:system_detail" );
		if ( !detail )
		{
			// someone deleted the gas file, fall back
			detail = FastFuelHandle( "config:system_detail" );
			gpassert( detail.IsValid() );
		}

		FastFuelHandleColl detail_blocks;
		detail.ListChildren( detail_blocks );
		for( FastFuelHandleColl::const_iterator i = detail_blocks.begin(); i != detail_blocks.end(); ++i )
		{
			// Make a new block
			SysDetail sys_det;

			// Get the memory and bpp of this block
			sscanf( i->GetName(), "%dm%db", &sys_det.memory, &sys_det.bpp );

			// Multiply by one megabyte to convert from meg to bytes
			sys_det.memory	<<= 20;

			// Get the detail value
			i->Get( "detail", sys_det.detail, false );

			// Get the shadow size value
			sys_det.shadowTexSize = i->GetInt( "shadow_tex_size", gDefaultRapi.GetShadowTexSize() ); 

			// Get the resolution block
			FastFuelHandle hResolutions = i->GetChildNamed( "resolutions" );
			gpassert( hResolutions.IsValid() );

			FastFuelHandleColl resolution_blocks;
			hResolutions.ListChildren( resolution_blocks );
			for( FastFuelHandleColl::const_iterator r = resolution_blocks.begin(); r != resolution_blocks.end(); ++r )
			{
				ValidSysMode valid_mode;

				// Get the width and height of this resolution
				sscanf( r->GetName(), "%dx%d", &valid_mode.m_Width, &valid_mode.m_Height );

				// Get the maximum allowable number of backbuffers for this resolution
				r->Get( "max_back_buffers", valid_mode.m_MaxBackBuffers, false );

				// Put this mode into our valid listing
				sys_det.valid_modes.push_back( valid_mode );
			}

			// Put the new detail on our list
			sysDetList.push_back( sys_det );
		}

		// Submit our detail list to Rapi
		gDefaultRapi.SetSystemDetailList( sysDetList );
	}

	//----- Content Naming Key
	{
		stdx::make_auto_ptr( mNamingKey, new NamingKey( "art\\NamingKey.NNK" ) );
	}

	//-----  Siege Engine
	{
		ScopeTimer timer( "init Siege Engine" );

		// Set up the Siege Engine
		stdx::make_auto_ptr( mRenderer, new siege::SiegeEngine );

		// Test out the Attach methods
		mRenderer->AttachRenderer( gDefaultRapi );

		//------ Video configuration
		{
			FastFuelHandle video( "::system_detail:video_capabilities" );
			if ( !video )
			{
				video = FastFuelHandle( "config:video_capabilities" );
			}

			if( video.IsValid() )
			{
				FastFuelHandle capabilityBlock;
				FastFuelHandleColl video_blocks;

				video.ListChildrenNamed( video_blocks, "vendor*" );
				for( FastFuelHandleColl::iterator o = video_blocks.begin(); o != video_blocks.end(); ++o )
				{
					FastFuelHandleColl device_blocks;
					(*o).ListChildrenNamed( device_blocks, "device*" );
					for( FastFuelHandleColl::const_iterator d = device_blocks.begin(); d != device_blocks.end(); ++d )
					{
						if( (DWORD)(*d).GetInt( "vendorid" ) == gDefaultRapi.GetVendorId() &&
							(DWORD)(*d).GetInt( "deviceid" ) == gDefaultRapi.GetDeviceId() )
						{
							// Get overall difference between driver versions
							if( ((DWORD)(gDefaultRapi.GetDriverVersion()>>32) >= (DWORD)(*d).GetInt( "driver_product" )) &&
								((DWORD)(gDefaultRapi.GetDriverVersion())	  >= (DWORD)(*d).GetInt( "driver_version" )) )
							{
								if( capabilityBlock.IsValid() )
								{
									if( ((DWORD)(*d).GetInt( "driver_product" ) >= (DWORD)capabilityBlock.GetInt( "driver_product" )) &&
										((DWORD)(*d).GetInt( "driver_version" ) >= (DWORD)capabilityBlock.GetInt( "driver_version" )) )
									{
										capabilityBlock = (*d);
									}
								}
								else
								{
									capabilityBlock = (*d);
								}
							}
						}
					}
				}

				if( capabilityBlock.IsValid() )
				{
					FastFuelHandle vendor_block = capabilityBlock.GetParent();

					gpstring card_name	= vendor_block.GetString( "vendor" );
					card_name			+= " ";
					card_name			+= capabilityBlock.GetString( "name" );

					gpgenericf(( "Initializing video hardware for %s, driver %d.%d.%d.%d\n",
								 card_name.c_str(),
								 (DWORD)(gDefaultRapi.GetDriverVersion() >> 48),
								 (DWORD)(gDefaultRapi.GetDriverVersion() >> 32) & 0x0000FFFF,
								 (DWORD)(gDefaultRapi.GetDriverVersion() >> 16) & 0x0000FFFF,
								 (DWORD)(gDefaultRapi.GetDriverVersion())		& 0x0000FFFF ));

					// Set the hardware preferences for this video configuration
					if( !gConfig.HasKey( "bltonly" ) )
					{
						gDefaultRapi.SetFlipAllowed( !capabilityBlock.GetBool( "no_flip", vendor_block.GetBool( "no_flip" ) ) );
					}
					if( gConfig.HasKey( "simplerender" ) )
					{
						gDefaultRapi.SetMultiTexBlend( !gConfig.GetBool( "simplerender" ) );
					}
					else
					{
						gDefaultRapi.SetMultiTexBlend( !capabilityBlock.GetBool( "simple_render", vendor_block.GetBool( "simple_render" ) ) );
					}
					if( gConfig.HasKey( "shadowrendertarget" ) )
					{
						gDefaultRapi.SetShadowRenderTarget( !gConfig.GetBool( "shadowrendertarget" ) );
					}
					else
					{
						gDefaultRapi.SetShadowRenderTarget( capabilityBlock.GetBool( "shadow_render_target", vendor_block.GetBool( "shadow_render_target" ) ) );
					}
					if( gConfig.HasKey( "asynccursor" ) )
					{
						gDefaultRapi.SetAsyncCursor( gConfig.GetBool( "asynccursor" ) );
					}
					gDefaultRapi.SetColorControl( !capabilityBlock.GetBool( "no_color_control", vendor_block.GetBool( "no_color_control" ) ) );
					if( !gDefaultRapi.Is16bitOnly() )
					{
						gDefaultRapi.Set16bitOnly( capabilityBlock.GetBool( "only_16_bit", vendor_block.GetBool( "only_16_bit" ) ) );
					}
					gDefaultRapi.SetForceTextureRestore( capabilityBlock.GetBool( "full_tex_restore", vendor_block.GetBool( "full_tex_restore" ) ) );
					gDefaultRapi.SetMipFilter( capabilityBlock.GetBool( "trilinear_filt", vendor_block.GetBool( "trilinear_filt" ) ) ? D3DTEXF_LINEAR : D3DTEXF_POINT );
					gDefaultRapi.SetBuffersFrames( capabilityBlock.GetBool( "buffers_frames", vendor_block.GetBool( "buffers_frames" ) ) );
					gDefaultRapi.SetSceneFade( capabilityBlock.GetBool( "scene_fade", vendor_block.GetBool( "scene_fade" ) ) );
					gDefaultRapi.SetTextureStateReset( capabilityBlock.GetBool( "tex_state_reset", vendor_block.GetBool( "tex_state_reset" ) ) );
					gDefaultRapi.SetModulateOnly( capabilityBlock.GetBool( "modulate_only", vendor_block.GetBool( "modulate_only" ) ) );
					
					if( capabilityBlock.GetBool( "below_min_spec", vendor_block.GetBool( "below_min_spec" ) ) )
					{
						if( !gConfig.HasKey( "min_spec_warning" ) )
						{
							gpmessagebox( $MSG$ "Your hardware configuration is below minimum specification for Dungeon Siege and may not run the game optimally. This is most likely because:\n    1) Your video hardware does not have enough memory (8MB or better).\n    2) Your video chipset has been detected as not supporting the features required by Dungeon Siege.\nContinuing to play may result in graphical glitches or other problems." );
							gConfig.Set( "min_spec_warning", true );
						}
					}

					// Set the shadow default
					if( !gSiegeEngine.GetOptions().IsExpensiveShadows() &&
						!capabilityBlock.GetBool( "no_complex_shadows", vendor_block.GetBool( "no_complex_shadows" ) ) )
					{
						gSiegeEngine.GetOptions().ToggleExpensiveShadows();
					}

					// Async mouse manual copy
					gRapiMouse.SetManualCopy( capabilityBlock.GetBool( "manual_mouse_copy", vendor_block.GetBool( "manual_mouse_copy" ) ) );
				}
				else
				{
					gDefaultRapi.SetColorControl( true );
					gperrorf(( "Unable to find video card entry for your hardware!\n    VendorID = %4x\n    DeviceID = %4x\n    Name = %s\n", gDefaultRapi.GetVendorId(), gDefaultRapi.GetDeviceId(), gDefaultRapi.GetDeviceName() ));
				}
			}
		}

		// Setup timeout
		FastFuelHandle hTimeout( "config:global_settings:timeouts" );
		if( hTimeout.IsValid() )
		{
			gDefaultRapi.SetSurfaceExpirationTime( hTimeout.GetFloat( "surface_expiration_time" ) );
		}

		FastFuelHandle hEngine( "config:engine_settings" );
		if( hEngine.IsValid() )
		{
			gSiegeEngine.SetNodeFadeTime( hEngine.GetFloat( "node_fade_time" ) );
			gSiegeEngine.SetNodeFadeAlpha( (BYTE)hEngine.GetInt( "node_fade_alpha" ) );
			gSiegeEngine.SetNodeFadeBlack( (BYTE)hEngine.GetInt( "node_fade_black" ) );

			gSiegeEngine.SetDiscoveryRadius( hEngine.GetFloat( "discovery_radius" ) );
			gSiegeEngine.SetDiscoverySampleLength( hEngine.GetFloat( "discovery_sample_length" ) );

			gSiegeEngine.SetDragSelectThreshold( hEngine.GetFloat( "drag_select_threshold" ) );

			gSiegeEngine.NodeWalker().SetObjectFadeTime( hEngine.GetFloat( "object_fade_time" ) );

			gSiegeEngine.NodeWalker().SetMiniMapMinMeters( hEngine.GetFloat( "minimap_min_size" ) );
			gSiegeEngine.NodeWalker().SetMiniMapMaxMeters( hEngine.GetFloat( "minimap_max_size" ) );
			gSiegeEngine.NodeWalker().SetMiniMapMeters( hEngine.GetFloat( "default_minimap_size" ) );
			gSiegeEngine.NodeWalker().SetMiniMapMetersZoomPerSecond( hEngine.GetFloat( "minimap_zoom_meters_per_second" ) );
			gSiegeEngine.NodeWalker().SetMiniMapStepZoomMeters( hEngine.GetFloat( "minimap_step_zoom_meters" ) );
			gSiegeEngine.NodeWalker().SetMinimapInnerFrustumScale( hEngine.GetFloat( "minimap_innerfrustum_scale" ) );			

			// Set the minimap textures
			gpstring texture, filename;
			if( hEngine.Get( "minimap_orientarrow_tex", texture ) && mNamingKey->BuildIMGLocation( texture, filename ) )
			{
				gSiegeEngine.NodeWalker().SetMiniMapOrientArrowTexture( gDefaultRapi.CreateTexture( filename, false, 0, TEX_LOCKED ) );
			}
			if( hEngine.Get( "minimap_selectionring_tex", texture ) && mNamingKey->BuildIMGLocation( texture, filename ) )
			{
				gSiegeEngine.NodeWalker().SetMiniMapSelectionRingTexture( gDefaultRapi.CreateTexture( filename, false, 0, TEX_LOCKED ) );
			}
			if( hEngine.Get( "minimap_discovery_tex", texture ) && mNamingKey->BuildIMGLocation( texture, filename ) )
			{
				gSiegeEngine.NodeWalker().SetMiniMapDiscoveryTexture( gDefaultRapi.CreateTexture( filename, false, 0, TEX_LOCKED ) );
			}
			if( hEngine.Get( "minimap_selectiontri_tex", texture ) && mNamingKey->BuildIMGLocation( texture, filename ) )
			{
				gSiegeEngine.NodeWalker().SetMiniMapSelectionTriTexture( gDefaultRapi.CreateTexture( filename, false, 0, TEX_LOCKED ) );
			}
			if( hEngine.Get( "minimap_tombstone_tex", texture ) && mNamingKey->BuildIMGLocation( texture, filename ) )
			{
				gSiegeEngine.NodeWalker().SetMiniMapTombstoneTexture( gDefaultRapi.CreateTexture( filename, false, 0, TEX_LOCKED ) );
			}
		}

		FastFuelHandle hCompass( "config:compass_settings" );
		if( hCompass.IsValid() )
		{
			gSiegeEngine.GetCompass().SetCompassRadius( hCompass.GetInt( "compass_radius" ) );
			gSiegeEngine.GetCompass().SetActive( hCompass.GetBool( "compass_isactive" ) );
			gSiegeEngine.GetCompass().SetAlpha( (BYTE)hCompass.GetInt( "compass_alpha", 255 ) );
			gSiegeEngine.GetCompass().SetNeedleTexture( hCompass.GetString( "compass_needle" ) );
			gSiegeEngine.GetCompass().SetDirectionTexture( hCompass.GetString( "compass_direction" ) );
			gSiegeEngine.GetCompass().SetCardinalTextures( hCompass.GetString( "compass_north" ),
														   hCompass.GetString( "compass_east" ),
														   hCompass.GetString( "compass_south" ),
														   hCompass.GetString( "compass_west" ) );
			gSiegeEngine.GetCompass().SetHotpointTexture( hCompass.GetString( "compass_hotpoint" ) );
			gSiegeEngine.GetCompass().SetMinimizedTexture( hCompass.GetString( "compass_minimized" ) );
			gSiegeEngine.GetCompass().SetCardinalDistance( hCompass.GetInt( "compass_cardinal_dist" ) );
			gSiegeEngine.GetCompass().SetVisible( false );
		}
	}

	// don't need the system_detail db any more
	if ( detailDb != NULL )
	{
		gFuelSys.Remove( detailDb );
	}


	//--------------------------------------------------------------------
	// "higher level" services
	//--------------------------------------------------------------------




	//----- init sound
	{
		ScopeTimer timer( "init GPGSound" );

		stdx::make_auto_ptr( mSoundManager, new GPGSound::SoundManager() );

		// Sound initialize
		mSoundManager->Initialize( 2, 44100, 16, gConfig.ResolvePathVars( "%home%/system/mss" ), "", false );

		// Read in sound settings
		DWORD latency = 0;
		FastFuelHandle hSound( "config:sound_settings" );
		if( hSound.IsValid() )
		{
			mSoundManager->SetSampleFileExpirationTime( hSound.GetFloat( "sound_cache_time", 30.0f ) );
			latency	= (DWORD)hSound.GetInt( "sound_latency", 100 );
		}

		// Look for command line override
		latency = gConfig.GetInt( "sound_latency", latency );
		if( latency )
		{
			mSoundManager->SetSoundLatency( latency );
		}

		if( gConfig.HasKey( "speakertype" ) )
		{
			gpstring speaker_type	= gConfig.GetString( "speakertype" );
			if( speaker_type.same_no_case( "stereo" ) )
			{
				mSoundManager->SetSpeakerType( GPGSound::SPK_STEREO );
			}
			else if( speaker_type.same_no_case( "headphone" ) )
			{
				mSoundManager->SetSpeakerType( GPGSound::SPK_HEADPHONE );
			}
			else if( speaker_type.same_no_case( "surround" ) )
			{
				mSoundManager->SetSpeakerType( GPGSound::SPK_SURROUND );
			}
			else if( speaker_type.same_no_case( "multisurround" ) )
			{
				mSoundManager->SetSpeakerType( GPGSound::SPK_4SPEAKER );
			}
			else if( speaker_type.same_no_case( "system" ) )
			{
				mSoundManager->SetSpeakerType( GPGSound::SPK_SYSTEM );
			}
		}
	}

	//----- init Skrit
	{
		ScopeTimer timer( "init SiegeSkrit Engine" );
		stdx::make_auto_ptr( mSkritEngine, CreateSkrit() );

#		if GP_ENABLE_SKRITBOT
		stdx::make_auto_ptr( mSkritBotMgr, new SkritBotMgr );
#		endif // GP_ENABLE_SKRITBOT

#		if !GP_ERROR_FREE
		mSkritEngine->SetOptions( Skrit::Engine::OPTION_DEFER_ERRORS );
#		endif // !GP_ERROR_FREE

#		if ( !GP_RETAIL )
		{
			if ( gConfig.GetBool( "skrit_retry" ) )
			{
				mSkritEngine->SetOptions( Skrit::Engine::OPTION_RETRY_COMPILE );
			}
		}
#		endif
	}

	//----- init NeMa
	{
		ScopeTimer timer( "init Nema Storage" );
		stdx::make_auto_ptr( mNemaAspectStorage, new nema::AspectStorage );
		stdx::make_auto_ptr( mNemaKeyMgr, new nema::PRSKeysStorage );
	}

	// report final configs
	GPDEBUG_ONLY( gConfig.DumpPathVars() );

	return ( true );
}

bool Services::Reinit( void )
{
	if ( !mNemaAspectStorage.get() )
	{
		stdx::make_auto_ptr( mNemaAspectStorage, new nema::AspectStorage );
	}
	if ( !mNemaKeyMgr.get() )
	{
		stdx::make_auto_ptr( mNemaKeyMgr, new nema::PRSKeysStorage );
	}
	return ( true );
}

void Services::Shutdown( bool fullShutdown )
{
	UNREFERENCED_PARAMETER( fullShutdown );

	delete ( mNemaAspectStorage.release() );
	mNemaAspectStorage = std::auto_ptr <nema::AspectStorage> ();
	delete ( mNemaKeyMgr.release() );
	mNemaKeyMgr = std::auto_ptr <nema::PRSKeysStorage> ();
}

bool Services::Xfer( FuBi::PersistContext& persist )
{
	persist.EnterBlock( "mNemaAspectStorage" );
	mNemaAspectStorage->Xfer( persist );
	persist.LeaveBlock();

	return ( true );
}

void Services::EnableLogAll( bool enable )
{
	mOutputFileSink->Enable( enable );
}

bool Services::IsLogAllEnabled( void ) const
{
	return ( mOutputFileSink->IsEnabled() );
}

FuBi::SysExports* Services::CreateFuBi( void )
{
	FuBi::SysExports* fubi = new FuBi::SysExports;

	// set default dsdll path
	gConfig.SetPathVar( "dsdll_path", "%exe_path%", Config::PVM_USE_CONFIG );

	// import bindings for native interface extensions that override the game (CAREFUL!)
	fubi->ImportModules( gConfig.ResolvePathVars( "%dsdll_path%/*.dsdl0", Config::PVM_USE_CONFIG ), NULL, false );

	// import bindings from self
	fubi->ImportBindings( NULL, NULL, false );

	// now import bindings for native interface extensions
	fubi->ImportModules( gConfig.ResolvePathVars( "%dsdll_path%/*.dsdll", Config::PVM_USE_CONFIG ), NULL, false );

	// finish it off
	fubi->ImportFinish();
	return ( fubi );
}

Skrit::Engine* Services::CreateSkrit( void )
{
	Skrit::Engine* skrit = new Skrit::Engine();
	Skrit::SetProductId( DS1_FOURCC_ID );
	skrit->RegisterOnlyCondition( "game", IsDungeonSiege() );
	skrit->RegisterOnlyCondition( "editor", IsEditor() );
	return ( skrit );
}

#if !GP_RETAIL
void Services::DumpHelp( void )
{
	// once this is called, the app must exit! this is only intended as a
	// helper function for analysis purposes, and it will mess things up if the
	// app continues to run.

	FuBi::SysExports sysExports;
	ReportSys::RetryFileSink sink( "fubi.log", true );
	gGenericContext.AddSink( &sink, false );
	ReportSys::FileSink::OutputGenericHeader( &gGenericContext, "Log" );
	sysExports.ImportBindings();
	sysExports.DumpStatistics();
	gHelp.All();

	gpmessageboxf(( "FuBi dumped to '%s'\n", sink.GetFileName().c_str() ));
}
#endif

void Services::CheckSkrit( const gpstring& skritFileName )
{
	// once this is called, the app must exit! this is only intended as a
	// helper function for analysis purposes, and it will mess things up if the
	// app continues to run.

	// open file
	FileSys::MemoryFile skritFile;
	bool wasEmpty = false;
	if ( skritFile.Create( skritFileName, &wasEmpty ) )
	{
		// set up output
		ReportSys::EnableErrorAssert( false );
		std::auto_ptr <ReportSys::RetryFileSink> sink( new ReportSys::RetryFileSink( skritFileName + ".out", true ) );
		gGenericContext.AddSink( sink.get(), false );
		gWarningContext.AddSink( sink.get(), false );
		gErrorContext  .AddSink( sink.get(), false );

		// construct objects
		std::auto_ptr <FuBi::SysExports> sysExports( Services::CreateFuBi() );
		std::auto_ptr <Skrit::Engine> engine( Services::CreateSkrit() );

		// compile skrit
		Skrit::CreateReq createReq( skritFileName );
		createReq.m_ForceSummary = true;
		const_mem_ptr data( skritFile.GetData(), skritFile.GetSize() );
		std::auto_ptr <Skrit::Object> skrit( engine->CreateNewObject( createReq, data ) );
	}
	else
	{
		if ( !wasEmpty )
		{
			gperrorf(( "Unable to open file '%s', error is '%s'\n",
					skritFileName.c_str(),
					stringtool::GetLastErrorText().c_str() ));
		}
	}
}

siege::Console & Services::GetOutputConsole()
{
	gpassert(mOutputConsole.get() != 0);
	return ( *mOutputConsole );
}

#if !GP_RETAIL

siege::Console & Services::GetFooterConsole()
{
	gpassert(mFooterConsole.get() != 0);
	return ( *mFooterConsole );
}

siege::Console & Services::GetWatchConsole()
{
	gpassert(mWatchConsole.get() != 0);
	return ( *mWatchConsole );
}

#endif // !GP_RETAIL

RapiFont& Services::GetConsoleFont()
{
	gpassert( mConsoleFont.get() != 0 );
	return ( *mConsoleFont );
}

siege::Console* ScopeTimer::mOutputConsole = 0;
Rapi* ScopeTimer::mRenderer = 0;

ScopeTimer::ScopeTimer( Rapi* renderer, siege::Console* console )
{
	mOutputConsole = console;
	mRenderer = renderer;
	mIgnoreDestruction = true;
}

ScopeTimer::ScopeTimer( gpstring const & scope_name )
{
	mIgnoreDestruction = false;

	if ( mOutputConsole && mRenderer)
	{
#		if GP_DEBUG
		mBeginNumAllocs		= gpmem::GetNumAllocations();
		mBeginSizeAllocs	= gpmem::GetSizeAllocations();
		mBeginSerialAllocID	= gpmem::GetLastSerialID();
#		endif // GP_DEBUG

#		if !GP_RETAIL

		mBeginTime = ::GetSystemSeconds();
		mName = scope_name;

		gpstring out;
		out += "[ ";
		out += scope_name;
		out += " ] begin";
		mLastLinePrintedTo = mOutputConsole->Print( out );

		if( mRenderer->Begin3D() )
		{
			mOutputConsole->Render( *mRenderer );
			mRenderer->End3D( true, true );
		}

#		endif // !GP_RETAIL
	}
}

ScopeTimer::~ScopeTimer()
{
	if ( mOutputConsole && mRenderer && !mIgnoreDestruction )
	{
#		if !GP_RETAIL

		double TimeTaken = ::GetSystemSeconds() - mBeginTime;
		char tempstr[256];

		stringtool::PadBackToLength( mOutputConsole->GetLine( mLastLinePrintedTo ), '.', 40 );

		if( mLastLinePrintedTo != mOutputConsole->GetBackBufferLineCount()-1 )
		{
			mLastLinePrintedTo = mOutputConsole->Print( "" );
			stringtool::PadBackToLength( mOutputConsole->GetLine( mLastLinePrintedTo ), '.', 40 );
		}

#		if GP_DEBUG
		sprintf( tempstr, "end in %2.3f sec.  NNA[%6d] GNA[%6d] NSA[%6dK]"
				, TimeTaken
				, (int)gpmem::GetNumAllocations() - (int)mBeginNumAllocs
				, (int)gpmem::GetLastSerialID() - (int)mBeginSerialAllocID
				, ((int)gpmem::GetSizeAllocations() - (int)mBeginSizeAllocs)/1000
				);
#		else
		sprintf( tempstr, "end in %2.3f sec.", TimeTaken );
#		endif // GP_DEBUG

		mOutputConsole->PrintAppend( tempstr, mLastLinePrintedTo );

#		if GP_DEBUG
		gpdebuggerf(( "%s: init time = %2.3f, committed allocs: %d, performed allocs: %d, memory used: %dK\n"
					, mName.c_str(), TimeTaken
					, (int)gpmem::GetNumAllocations() - (int)mBeginNumAllocs
					, (int)gpmem::GetLastSerialID() - (int)mBeginSerialAllocID
					, ((int)gpmem::GetSizeAllocations() - (int)mBeginSizeAllocs)/1000 ));
#		endif // GP_DEBUG

#		else // !GP_RETAIL

		mOutputConsole->PrintAppend( ".", 0 );

#		endif // !GP_RETAIL

		if( mRenderer->Begin3D() )
		{
			mOutputConsole->Render( *mRenderer );
			mRenderer->End3D( true, true );
		}
	}
}
