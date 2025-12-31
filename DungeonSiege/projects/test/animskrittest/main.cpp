//////////////////////////////////////////////////////////////////////////////
//
// File     :  main.cpp
// Author(s):  Scott Bilas,biddle
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_AnimSkritTest.h"
#include "SkritEngine.h"
#include "SkritStructure.h"
#include "PathFileMgr.h"
#include "StringTool.h"
#include "NetTransport.h"

#include "namingkey.h"
#include "nema_aspect.h"
#include "nema_aspectmgr.h"

#include "execution_environment.h"

#include <iostream>
#include <strstream>
#include <conio.h>

#include "nema_keymgr.h"

//////////////////////////////////////////////////////////////////////////////
// main

int ExecMain( int argc, const char* argv[] );

int main( int argc, const char* argv[] )
{
	int rc = ExecMain( argc, argv );		// handy place to put a breakpoint that won't move
	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
// Object

class Object : public Singleton <Object>
{
public:
	SET_NO_INHERITED( Object );

	Object( void )  {  Reset();  }

	FUBI_EXPORT void Quit ( void )  {  m_Quit  = true;  }

	FUBI_EXPORT void Send( const char* text )
	{
		FUBI_RPC_CALL( Send, text, true );
		puts( text );
	}

	FUBI_EXPORT void Send( int i )
	{
		FUBI_RPC_CALL( Send, i, true );
		printf( "%d\n", i );
	}

	FUBI_EXPORT void StopServer( void )
	{
		FUBI_RPC_CALL0( StopServer, true );
		m_Quit  = true;
	}

	void Reset( void )
	{
		m_Quit  = false;
	}

	bool IsQuitting( void ) const  {  return ( m_Quit  );  }

	FUBI_EXPORT void LocalTest( const char* msg, ... )
	{
		char buffer[ 6000 ];
		va_list args;
		va_start( args, msg );
		_vsnprintf( buffer, ELEMENT_COUNT( buffer ), msg, args );
		va_end( args );

		gpdevprogress( buffer );
	}

private:
	bool m_Quit;

	FUBI_SINGLETON_CLASS( Object, "Generic testing object." );
	SET_NO_COPYING( Object );
};

//////////////////////////////////////////////////////////////////////////////
// Out

class Out : public Singleton <Out>
{
public:
	Out( void )
	{
		m_Buffer[ 0 ] = '\0';
	}

	FUBI_EXPORT const char* Format( const char* msg, ... )
	{
		va_list args;
		va_start( args, msg );
		_vsnprintf( m_Buffer, ELEMENT_COUNT( m_Buffer ), msg, args );
		va_end( args );

		return ( m_Buffer );
	}

	static FUBI_EXPORT void PrintF( const char* msg, ... )
	{
		char buffer[ 6000 ];

		va_list args;
		va_start( args, msg );
		_vsnprintf( buffer, ELEMENT_COUNT( buffer ), msg, args );
		va_end( args );

		gpdevprogress( buffer );
	}

	static FUBI_EXPORT void Print( const char* msg )
	{
		gpdevprogress( msg );
	}

//	FUBI_EXPORT Out* GetThis( void )
//	{
//		return ( this );
//	}

private:
	char m_Buffer[ 6000 ];

	FUBI_SINGLETON_CLASS( Out, "Output functions." );
	SET_NO_COPYING( Out );
};

//////////////////////////////////////////////////////////////////////////////
// Globals

// Params.

	bool                g_GenerateDocs = false;

// Objects.

	Object              g_Object;
	Out                 g_Out;
	NetSendTransport    g_Sender;
	NetReceiveTransport g_Receiver;
//	HCalculatorMgr      g_CalculatorMgr( 100 );

//////////////////////////////////////////////////////////////////////////////
// Global exports

FUBI_EXPORT void SetGenerateDocs( bool set )
{
	g_GenerateDocs = set;
}

FUBI_EXPORT void Quit( void )
{
	Object::GetSingleton().Quit();
}

//////////////////////////////////////////////////////////////////////////////
// Handlers

bool Prompt( void )
{
	printf( "> " );
	char inBuffer[ 1000 ];
	if ( gets( inBuffer ) == NULL )
	{
		*inBuffer = '\0';
	}

	// compile and run
	Skrit::Object* skrit = gSkritEngine.CreateCommand( "cmd", inBuffer );
	if ( skrit != NULL )
	{
		if ( g_GenerateDocs )
		{
			skrit->GetImpl()->GetPack()->GenerateDocs( std::cout );
		}

		skrit->Call( 0 );
		delete ( skrit );
		return ( true );
	}

	return ( false );
}

// these are general events

DECLARE_EVENT
{

	FUBI_EXPORT void Ping( Skrit::HObject skrit )
		{  SKRIT_EVENTV( Ping, skrit );  }

	FUBI_EXPORT void Poke( Skrit::HObject skrit, float)
		{  SKRIT_EVENTV( Poke, skrit );  }

	FUBI_EXPORT void OnAttachAsp( Skrit::HObject skrit, nema::HAspect asp)
		{  SKRIT_EVENTV( OnAttachAsp, skrit );  }

	FUBI_EXPORT void OnAttachKeys( Skrit::HObject skrit, nema::HPRSKeys keys)
		{  SKRIT_EVENTV( OnAttachKeys, skrit );  }

	FUBI_EXPORT void OnUpdate( Skrit::HObject skrit, float delta_t )
		{  SKRIT_EVENTV( OnUpdate, skrit );  }
}

// this is a callback into a skrit

//SKRIT_IMPORT int Test( Skrit::HObject skrit, const char* funcName, int, const char*, float, HCalculator )
//	{  SKRIT_CALL( int, Test, skrit, funcName );  }

void ReportError( Skrit::Result result )
{
	std::cout << Skrit::ToString( result.first );
	if ( !Skrit::IsError( result ) )
	{
		std::cout << ", Result is " << result.second;
	}
	std::cout << std::endl;
}

namespace FuBiEvent  {  // begin of namespace FuBiEvent


//FUBI_EXPORT void TurnOn( Skrit::HObject skrit, bool /*on*/ )
//{
//	SKRIT_EVENTV( TurnOn, skrit );
//}

}  // end of namespace FuBiEvent

int StateTest( void )
{
	Skrit::HAutoObject object = gSkritEngine.GetObject( "testskrits\\statetest.skrit" );

	gpstring filename;
	gNamingKey.BuildASPLocation("m_c_eam_dga_pos_1",filename);

	nema::HAspect hasp = gAspectStorage.LoadAspect( filename );

	gNamingKey.BuildContentLocation("a_c_eam_dga_fs6_wl.prs",filename);

	nema::HPRSKeys hkeys = gPRSKeysStorage.LoadPRSKeys(filename.c_str());

	gpassertf(hkeys.IsValid(), ("Couldn't load a valid keyset"));

	if ( !object.IsNull() && hasp.IsValid() && hkeys.IsValid() )
	{
		FuBiEvent::OnAttachAsp( object, hasp );
		FuBiEvent::OnAttachKeys( object, hkeys);
		FuBiEvent::Ping( object );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::Ping( object );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::Ping( object );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );
		FuBiEvent::OnUpdate( object , 0.15f );

//		std::cout << "\nDumping " << gSkritEngine.GetObjectName( object ) << ":\n\n";
//		object->GetImpl()->GetPack()->GenerateDocs( std::cout );
	}

	while ( !g_Object.IsQuitting() )
	{
		Prompt();
	}

	return ( 0 );
}

int ExecMain( int argc, const char* argv[] )
{

// Configuration.

	// setup debug output
	struct Output : GPDebugOutput
	{
		Output( void )
		{
			GPDEBUGOUTPUT_RegisterOutput( this, GPDEBUGOUTPUT_ERROR );
			GPDEBUGOUTPUT_RegisterOutput( this, GPDEBUGOUTPUT_WARNING );
			GPDEBUGOUTPUT_RegisterOutput( this, GPDEBUGOUTPUT_DEV_PROGRESS );
			GPDEBUGOUTPUT_RegisterOutput( this, GPDEBUGOUTPUT_PROGRESS );

			GPDEBUGOUTPUT_SetReportErrors( true );
			GPDEBUGOUTPUT_SetReportWarnings( true );
			GPDEBUGOUTPUT_SetReportProgress( true );
		}

		virtual void Print( const char* message, unsigned int /*type*/ )
		{
			::OutputDebugString( message );
			std::cout << message;
		}
	}	output;

	// setup mem
#	if GP_DEBUG
	GPmem_SetMemoryDebugNormal();
#	endif

	// Locate the CDIMAGE
	gpstring CDIMAGE;
	execution_environment myEnvironment("GPG","AnimSkritTest");
	myEnvironment.GetVariableString( "TATTOO_ASSETS", CDIMAGE );
	CDIMAGE += "\\CDIMAGE";

	// setup filesys
	FileSys::PathFileMgr pathFileMgr(CDIMAGE.c_str());
	pathFileMgr.SetMasterFileMgr( &pathFileMgr );

	// setup fubi
	FuBi::SysExports sysExports;
	sysExports.ImportBindings();

	// setup skrit
	Skrit::Engine skritEngine;

	// setup naming key
	NamingKey theNameKey("ART\\NAMINGKEY.NNK");

	// setup nema
	nema::AspectStorage myAspects;
	nema::PRSKeysStorage myKeys;

	// Command execution.

	int rc = 0;

	StateTest();

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
