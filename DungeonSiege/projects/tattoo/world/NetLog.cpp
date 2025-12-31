//////////////////////////////////////////////////////////////////////////////
//
// File     :  NetLog.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "NetLog.h"

#include "AppModule.h"
#include "Config.h"
#include "FileSysXfer.h"
#include "Flamethrower_interpreter.h"
#include "FuBi.h"
#include "FuBiBitPacker.h"
#include "GoBase.h"
#include "GoFollower.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoRpc.h"
#include "GpZLib.h"
#include "MCP.h"
#include "NetFuBi.h"
#include "NetPipe.h"
#include "ReportSys.h"
#include "WorldVersion.h"
#include "WorldTime.h"

//////////////////////////////////////////////////////////////////////////////
// RPC translation (for net log)

#if !GP_RETAIL

typedef void (*PodProc)( gpstring& /*appendHere*/, FuBi::eVarType /*type*/, const_mem_ptr /*object*/ );
typedef void (*MemProc)( gpstring& /*appendHere*/, int /*paramIndex*/, const_mem_ptr /*mem*/ );

struct PodTranslatorEntry
{
	const char* m_Name;
	PodProc     m_Proc;
};

struct MemTranslatorEntry
{
	const char* m_Name;
	MemProc     m_Proc;
};

void GoCreateReqRpcToString( gpstring& appendHere, FuBi::eVarType /*type*/, const_mem_ptr object )
{
	gpassert( object.size == sizeof( GoCreateReq ) );
	((const GoCreateReq*)object.mem)->RpcToString( appendHere );
}

void GoCloneReqRpcToString( gpstring& appendHere, FuBi::eVarType /*type*/, const_mem_ptr object )
{
	gpassert( object.size == sizeof( GoCloneReq ) );
	((const GoCloneReq*)object.mem)->RpcToString( appendHere );
}

void JobReqRpcToString( gpstring& appendHere, FuBi::eVarType /*type*/, const_mem_ptr object )
{
	gpassert( object.size == sizeof( JobReq ) );
	((const JobReq*)object.mem)->RpcToString( appendHere );
}

template <typename T>
void PackolaToString( T& xfer, gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	gpassert( paramIndex == 0 );

	FuBi::BitPacker packer( packola );
	xfer.Xfer( packer );
	xfer.RpcToString( appendHere );
}

void RCMarkForDeletionPackerToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	GoDbDeleteXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RCMarkForMondoDeletionPackerToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	GoDbMondoDeleteXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RSMindDoJobPackerToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	JobReq xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RSActionDoJobPackerToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	JobReqClientsXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RCRunScriptPackerToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	RunScriptXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RCPlayCombatSoundPackerToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	CombatSoundXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RCSendPackedUpdateToFollowersToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	PackedUpdateToFollowersXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RCSendPackedPositionUpdateToFollowersToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	PackedPositionUpdateToFollowersXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

void RCSendPackedClipToFollowersToString( gpstring& appendHere, int paramIndex, const_mem_ptr packola )
{
	PackedClipToFollowersXfer xfer;
	PackolaToString( xfer, appendHere, paramIndex, packola );
}

static PodTranslatorEntry s_PodTranslators[] =
{
	{  "GoCreateReq", GoCreateReqRpcToString,  },
	{  "GoCloneReq",  GoCloneReqRpcToString ,  },
	{  "JobReq",      JobReqRpcToString     ,  },
};

static MemTranslatorEntry s_MemTranslators[] =
{
	{  "GoDb::RCMarkForDeletionPacker",         RCMarkForDeletionPackerToString     ,  },
	{  "GoDb::RCMarkForMondoDeletionPacker",    RCMarkForMondoDeletionPackerToString,  },
	{  "GoMind::RSDoJobPacker",                 RSMindDoJobPackerToString           ,  },
	{  "AIAction::RSDoJobPacker",               RSActionDoJobPackerToString         ,  },
	{  "SiegeFxInterpreter::RCRunScriptPacker", RCRunScriptPackerToString           ,  },
	{  "Go::RCPlayCombatSoundPacker",           RCPlayCombatSoundPackerToString     ,  },

	{  "GoFollower::RCSendPackedUpdateToFollowers",			RCSendPackedUpdateToFollowersToString			,  },
	{  "GoFollower::RCSendPackedPositionUpdateToFollowers",	RCSendPackedPositionUpdateToFollowersToString	,  },
	{  "GoFollower::RCSendPackedClipToFollowers",			RCSendPackedClipToFollowersToString				,  },
};

void RegisterTranslators( void )
{
	const PodTranslatorEntry* i, * ibegin = s_PodTranslators, * iend = ARRAY_END( s_PodTranslators );
	for ( i = ibegin ; i != iend ; ++i )
	{
		FuBi::FunctionSpec::RegisterPodTranslateCb( gFuBiSysExports.FindType( i->m_Name ), makeFunctor( i->m_Proc ) );
	}

	const MemTranslatorEntry* j, * jbegin = s_MemTranslators, * jend = ARRAY_END( s_MemTranslators );
	for ( j = jbegin ; j != jend ; ++j )
	{
		const FuBi::FunctionIndex* funcs = gFuBiSysExports.FindFunctionsByQualifiedName( j->m_Name );
		if_gpassert( (funcs != NULL) && (funcs->size() == 1) )
		{
			FuBi::FunctionSpec::RegisterMemTranslateCb( (*funcs)[ 0 ]->m_SerialID, makeFunctor( j->m_Proc ) );
		}
	}
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

DECL_THREAD int s_Recursion = 0;

struct AutoRecursion
{
	AutoRecursion( void )  {  ++s_Recursion;  }
   ~AutoRecursion( void )  {  --s_Recursion;  }
};

struct NetLogHeader;
struct NetLogEntry;
struct NetLogGoEntry;
struct NetLogPacketEntry;

//////////////////////////////////////////////////////////////////////////////
// class NetLogSink implementation

class NetLogSink : public ReportSys::StringSink
{
public:
	SET_INHERITED( NetLogSink, ReportSys::StringSink );

	NetLogSink( void )
	{
		// hook into all streams
		gGlobalSink.AddSink( this, false );

		// hook into special net stream
		gNetContext.AddSink( this, false );

		// hook into debugger stream
		gDebuggerContext.AddSink( this, false );
	}

	virtual ~NetLogSink( void )
		{  }

	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len, bool startLine, bool endLine )
	{
		// skip if recursive!
		if ( s_Recursion > 0 )
		{
			return ( true );
		}

		// always out to the net log
		bool updated = false;
		if ( NetLog::DoesSingletonExist() && gNetLog.IsLogging() )
		{
			Inherited::OutputString( context, text, len, startLine, endLine );
			updated = true;

			if ( endLine )
			{
				gNetLog.OutputReportString( context->GetName(), NULL, GetBuffer(), GetBuffer().length() );
			}
		}

		// optional mirror to the text report stream too
		if ( gNetTextContext.IsEnabled() && (context->GetName().same_no_case( "net" ) || context->GetName().same_no_case( "netraw" )) )
		{
			AutoRecursion autoRecursion;
			if ( !updated )
			{
				Inherited::OutputString( context, text, len, startLine, endLine );
			}

			if ( endLine )
			{
				gNetTextContext.Output( GetBuffer(), GetBuffer().length() );
			}
		}

		// done with it now
		if ( endLine )
		{
			ClearBuffer();
		}

		return ( true );
	}

private:
	SET_NO_COPYING( NetLogSink );
};

//////////////////////////////////////////////////////////////////////////////
// class AutoNetWriter declaration

class AutoNetWriter : public FileSys::StreamWriter
{
public:
	SET_INHERITED( AutoNetWriter, FileSys::StreamWriter );

// Setup.

	AutoNetWriter( FOURCC type = 0, DWORD realSize = 0, bool raw = false );

   ~AutoNetWriter( void )
	{
		// backpatch size
		if ( !m_Raw )
		{
			*(DWORD*)(&*gNetLog.m_WriteBuffer.begin()) = gNetLog.m_WriteBuffer.size_bytes();
		}

		gNetLog.OutputRaw( const_mem_ptr( &*gNetLog.m_WriteBuffer.begin(), gNetLog.m_WriteBuffer.size_bytes() ), !m_Raw );
	}

// Transfer.

	virtual bool Write( const void* out, int size )
	{
		return ( m_Writer.Write( out, size ) );
	}

	virtual bool WriteText( const char* text, int len = -1 )
	{
		if ( len < 0 )
		{
			len = text ? ::strlen( text ) : 0;
		}

		WriteStruct( len );				// length first
		Write( text, len );				// then text
		return ( true );
	}

	void WriteBlob( const_mem_ptr mem )
	{
		WriteStruct( mem.size );		// length first
		Write( mem.mem, mem.size );		// then blob
	}

	void WriteGo( Go* go );

	void WriteRpc( const FuBi::RpcVerifySpec& rpc, bool isRpcPacked )
	{
		WriteStruct( rpc.m_Address );
		WriteStruct( rpc.m_Function->m_SerialID );
		WriteStruct( isRpcPacked );
		WriteBlob  ( const_mem_ptr( rpc.m_Params, rpc.m_Function->m_ParamSizeBytes ) );
		WriteStruct( rpc.m_This );
	}

	void WritePacket( const PacketInfo& packet )
	{
		WriteMachineId( packet.m_machineId );
		WriteStruct   ( packet.m_Type );
		WriteStruct   ( packet.m_RetryCount );
		WriteStructs  ( &*packet.m_Buffer.begin(), packet.m_Buffer.size_bytes() );
	}

	void WritePacket( const GoRpcMgr::Packet& packet )
	{
		WriteStruct ( packet.m_IsRetrying );
		WriteStruct ( packet.m_RouterGoid );
		WriteStruct ( packet.m_PendingGoids.size() );
		WriteStructs( &*packet.m_PendingGoids.begin(), packet.m_PendingGoids.size_bytes() );
		WriteRpc    ( packet.m_VerifySpec, false );
	}

	void WriteMachineId( DWORD machineId )
	{
		gpstring name;

		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
		if ( session )
		{
			NetPipeMachine* machine = session->GetMachine( machineId );
			if ( machine != NULL )
			{
				name = ::ToAnsi( machine->m_wsName );
			}
		}

		WriteStruct( machineId );
		WriteText( name );
	}

	void WriteXfer( const CreateGoXfer& xfer )
	{
		WriteStruct( xfer.m_CreateReq  );
		WriteStruct( xfer.m_StartGoid  );
		WriteStruct( xfer.m_RandomSeed );
		WriteStruct( xfer.m_Standalone );
		WriteBlob  ( xfer.m_Data       );
	}

	void WriteXfer( const CloneGoXfer& xfer )
	{
		WriteStruct( xfer.m_CloneReq     );
		WriteStruct( xfer.m_StartGoid    );
		WriteStruct( xfer.m_RandomSeed   );
		WriteText  ( xfer.m_TemplateName );
		WriteStruct( xfer.m_Standalone   );
		WriteBlob  ( xfer.m_Data         );
	}

private:
	kerneltool::Critical::Lock m_Locker;	// $ this must be first member so it locks before anything else happens
	FileSys::BufferWriter m_Writer;
	bool m_Raw;
	int m_OldSize;

	SET_NO_COPYING( AutoNetWriter );
};

//////////////////////////////////////////////////////////////////////////////
// class NetLogReader declaration and implementation

typedef stdx::fast_vector <BYTE> Buffer;

class NetLogReader : public FileSys::FileReader
{
public:
	SET_INHERITED( NetLogReader, FileSys::FileReader );

	NetLogReader( HANDLE file, bool initiallyCompressed = false )
		: Inherited( file )
	{
		m_Compressed = initiallyCompressed;
		m_EntryPtr = &*m_EntryBuffer.end();
		m_InflateStreamer.Init();

		// 1K chunks
		m_ReadBuffer.resize( 1024 );
		m_ReadPtr = &*m_ReadBuffer.end();
	}

	void SetCompressed( bool set = true )
	{
		m_Compressed = set;
	}

	int GetEntryBufferRemaining( void ) const
	{
		return ( m_EntryBuffer.end() - m_EntryPtr );
	}

	void TakeEntryData( Buffer& buffer )
	{
		m_EntryBuffer.clear();
		m_EntryBuffer.swap( buffer );
		m_EntryPtr = &*m_EntryBuffer.begin();
	}

	virtual bool Read( void* out, int size, int* bytesRead = NULL )
	{
		int remaining = GetEntryBufferRemaining();
		if ( remaining != 0 )
		{
			if_gpassert( size <= remaining )
			{
				::memcpy( out, m_EntryPtr, size );
				if ( bytesRead != NULL )
				{
					*bytesRead = size;
				}
				m_EntryPtr += size;
				return ( true );
			}
			else
			{
				return ( false );
			}
		}
		else if ( !m_Compressed )
		{
			return ( Inherited::Read( out, size, bytesRead ) );
		}

		bool success = true;

		int localBytesRead = 0;
		while ( size > 0 )
		{
			// fill if needed
			if ( m_ReadPtr == &*m_ReadBuffer.end() )
			{
				int readSize = 0;
				if ( !Inherited::Read( &*m_ReadBuffer.begin(), m_ReadBuffer.size_bytes(), &readSize ) || (readSize == 0) )
				{
					success = false;
					break;
				}

				m_ReadBuffer.resize( readSize );
				m_ReadPtr = &*m_ReadBuffer.begin();
			}

			// read out what we want
			int inUsed = 0;
			int outUsed = m_InflateStreamer.InflateChunk(
					mem_ptr( out, size ),
					const_mem_ptr( m_ReadPtr, m_ReadBuffer.end() - m_ReadPtr ),
					&inUsed );
			if ( outUsed == Z_ERRNO )
			{
				success = false;
				break;
			}

			// advance
			size -= outUsed;
			out = (BYTE*)out + outUsed;
			localBytesRead += outUsed;
			m_ReadPtr += inUsed;
		}

		// update size
		if ( bytesRead != NULL )
		{
			*bytesRead = localBytesRead;
		}

		return ( success );
	}

	bool ReadText( gpstring& out )
	{
		out.clear();

		int len = 0;
		if ( !ReadStruct( len ) )
		{
			return ( false );
		}

		if ( len > 0 )
		{
			out.resize( len );
			if ( !Read( &*out.begin_split(), len ) )
			{
				return ( false );
			}
		}

		return ( true );
	}

	void ReadMachineId( DWORD& machineId, gpstring& machineName )
	{
		ReadStruct( machineId );
		ReadText( machineName );
	}

	void ReadRpc( FuBi::RpcVerifySpec& rpc, bool& isRpcPacked, Buffer& buffer )
	{
		ReadStruct( rpc.m_Address );

		UINT serialID;
		ReadStruct( serialID );
		rpc.m_Function = gFuBiSysExports.FindFunctionBySerialID( serialID );

		ReadStruct( isRpcPacked );
		ReadBlob  ( buffer );
		ReadStruct( rpc.m_This );

		rpc.m_Params = const_mem_ptr( &*buffer.begin(), buffer.size_bytes() );
	}

	void ReadPacket( PacketInfo& packet, gpstring& machineName )
	{
		ReadMachineId( packet.m_machineId, machineName );
		ReadStruct   ( packet.m_Type );
		ReadStruct   ( packet.m_RetryCount );

		packet.m_Buffer.resize( GetEntryBufferRemaining() );
		Read( &*packet.m_Buffer.begin(), GetEntryBufferRemaining() );
	}

	void ReadBlob( Buffer& buffer )
	{
		int size = 0;
		ReadStruct( size );
		buffer.resize( size );
		Read( &*buffer.begin(), size );
	}

	bool ReadEntry( NetLogHeader& header );
	bool ReadEntry( NetLogEntry& entry );

private:
	bool                  m_Compressed;
	Buffer                m_ReadBuffer;
	const BYTE*           m_ReadPtr;
	Buffer                m_EntryBuffer;
	const BYTE*           m_EntryPtr;
	zlib::InflateStreamer m_InflateStreamer;

	SET_NO_COPYING( NetLogReader );
};

//////////////////////////////////////////////////////////////////////////////
// NetLog helper types

struct NetLogHeader
{
	DWORD  m_Size;
	DWORD  m_RealSize;
	FOURCC m_Type;
	DWORD  m_ThreadId;
	double m_SystemSeconds;
	double m_GlobalSeconds;
	double m_WorldTime;
	float  m_DeltaTime;
	DWORD  m_SimCount;

	NetLogHeader( bool init = true )
	{
		if ( init )
		{
			::ZeroObject( *this );
		}
	}

	void Load( FOURCC type, DWORD realSize )
	{
		m_Size          = (DWORD)0;
		m_RealSize      = realSize;
		m_Type          = type;
		m_ThreadId      = ::GetCurrentThreadId();
		m_SystemSeconds = ::GetSystemSeconds();
		m_GlobalSeconds = ::GetGlobalSeconds();

		if ( WorldTime::DoesSingletonExist() )
		{
			m_WorldTime = gWorldTime.GetTime();
			m_DeltaTime = gWorldTime.GetSecondsElapsed();
			m_SimCount  = gWorldTime.GetSimCount();
		}
		else
		{
			m_WorldTime = 0;
			m_DeltaTime = 0;
			m_SimCount  = 0;
		}
	}

	void Write( AutoNetWriter* writer )
	{
		writer->WriteStruct( *this );
	}

	bool Read( NetLogReader* reader )
	{
		bool rc = reader->ReadStruct( *this );
		m_Size -= sizeof( NetLogHeader );
		return ( rc );
	}

	bool DefinitelyNotBad( void ) const
	{
		return ( (m_Type != 'Strm') && (m_Type != 'NInD') );
	}
};

struct NetLogEntry : NetLogHeader
{
	Buffer m_EntryData;
};

struct NetLogGoEntry
{
	const Go* m_Go;
	Goid      m_Goid;
	Goid      m_CloneSourceGoid;
	Scid      m_Scid;
	PlayerId  m_PlayerId;
	gpstring  m_TemplateName;
	gpstring  m_VariationName;
	gpstring  m_PrefixName;
	gpstring  m_SuffixName;
	FrustumId m_FrustumMembership;
	double    m_CreateTime;
	DWORD     m_RandomSeed;
	bool      m_IsOmni;
	bool      m_IsAllClients;
	bool      m_IsLodfi;
	bool      m_IsServerOnly;
	bool      m_IsBucketDirty;
	bool      m_IsInsideInventory;
	bool      m_HasPlacement;
	bool      m_DoesInheritPlacement;
	SiegePos  m_RawPosition;
	SiegePos  m_Position;
	Quat      m_Orientation;

	NetLogGoEntry( bool init = true )
	{
		if ( init )
		{
			Init();
		}
	}

	void Init( void )
	{
		m_Go                   = NULL;
		m_Goid                 = GOID_INVALID;
		m_CloneSourceGoid      = GOID_INVALID;
		m_Scid                 = SCID_INVALID;
		m_PlayerId             = PLAYERID_INVALID;
		m_FrustumMembership    = FRUSTUMID_INVALID;
		m_CreateTime           = 0;
		m_RandomSeed           = 0;
		m_IsOmni               = false;
		m_IsAllClients         = false;
		m_IsLodfi              = false;
		m_IsServerOnly         = false;
		m_IsBucketDirty        = false;
		m_IsInsideInventory    = false;
		m_HasPlacement         = false;
		m_DoesInheritPlacement = false;
	}

	void Load( const Go* go )
	{
		__try
		{
			m_Go                = go;
			m_Goid              = go->GetGoid();
			m_CloneSourceGoid   = go->GetCloneSourceGoid();
			m_PlayerId          = go->GetPlayer() ? go->GetPlayer()->GetId() : 0;
			m_Scid              = go->GetScid();
			m_TemplateName      = go->GetTemplateName();

			if ( go->HasGui() )
			{
				m_VariationName = go->GetGui()->GetVariation();
			}
			if ( go->HasMagic() )
			{
				m_PrefixName = go->GetMagic()->GetPrefixModifierName();
				m_SuffixName = go->GetMagic()->GetSuffixModifierName();
			}

			m_FrustumMembership = go->GetWorldFrustumMembership();
	#		if GP_RETAIL
			m_CreateTime        = 0;
	#		else // GP_RETAIL
			m_CreateTime        = go->GetCreateTime();
	#		endif // GP_RETAIL
			m_RandomSeed        = go->GetRandomSeed();
			m_IsOmni            = go->IsOmni();
			m_IsAllClients      = go->IsAllClients();
			m_IsLodfi           = go->IsLodfi();
			m_IsServerOnly      = go->IsServerOnly();
			m_IsBucketDirty     = go->IsBucketDirty();
			m_IsInsideInventory = go->IsInsideInventory();
			m_HasPlacement      = go->HasPlacement();

			if ( m_HasPlacement )
			{
				m_DoesInheritPlacement = go->GetPlacement()->DoesInheritPlacement();
				if ( m_DoesInheritPlacement )
				{
					m_RawPosition = go->GetPlacement()->GetRawPosition();
				}

				m_Position    = go->GetPlacement()->GetPosition();
				m_Orientation = go->GetPlacement()->GetOrientation();
			}
		}
		__except( ::GlobalExceptionFilter( GetExceptionInformation(), EXCEPTION_CONTINUE_SEARCH, EXCEPTION_EXECUTE_HANDLER ) )
		{
			// oh well, whatever...
			Init();
		}
	}

	void Write( AutoNetWriter* writer )
	{
		writer->WriteStruct( m_Go );
		writer->WriteStruct( m_Goid );
		writer->WriteStruct( m_CloneSourceGoid );
		writer->WriteStruct( m_Scid );
		writer->WriteStruct( m_PlayerId );
		writer->WriteText  ( m_TemplateName );
		writer->WriteText  ( m_VariationName );
		writer->WriteText  ( m_PrefixName );
		writer->WriteText  ( m_SuffixName );
		writer->WriteStruct( m_FrustumMembership );
		writer->WriteStruct( m_CreateTime );
		writer->WriteStruct( m_RandomSeed );
		writer->WriteStruct( m_IsOmni );
		writer->WriteStruct( m_IsAllClients );
		writer->WriteStruct( m_IsLodfi );
		writer->WriteStruct( m_IsServerOnly );
		writer->WriteStruct( m_IsBucketDirty );
		writer->WriteStruct( m_IsInsideInventory );
		writer->WriteStruct( m_HasPlacement );

		if ( m_HasPlacement )
		{
			writer->WriteStruct( m_DoesInheritPlacement );
			if ( m_DoesInheritPlacement )
			{
				writer->WriteStruct( m_RawPosition );
			}

			writer->WriteStruct( m_Position );
			writer->WriteStruct( m_Orientation );
		}
	}

	void Read( NetLogReader* reader )
	{
		reader->ReadStruct( m_Go );
		reader->ReadStruct( m_Goid );
		reader->ReadStruct( m_CloneSourceGoid );
		reader->ReadStruct( m_Scid );
		reader->ReadStruct( m_PlayerId );
		reader->ReadText  ( m_TemplateName );
		reader->ReadText  ( m_VariationName );
		reader->ReadText  ( m_PrefixName );
		reader->ReadText  ( m_SuffixName );
		reader->ReadStruct( m_FrustumMembership );
		reader->ReadStruct( m_CreateTime );
		reader->ReadStruct( m_RandomSeed );
		reader->ReadStruct( m_IsOmni );
		reader->ReadStruct( m_IsAllClients );
		reader->ReadStruct( m_IsLodfi );
		reader->ReadStruct( m_IsServerOnly );
		reader->ReadStruct( m_IsBucketDirty );
		reader->ReadStruct( m_IsInsideInventory );
		reader->ReadStruct( m_HasPlacement );

		if ( m_HasPlacement )
		{
			reader->ReadStruct( m_DoesInheritPlacement );
			if ( m_DoesInheritPlacement )
			{
				reader->ReadStruct( m_RawPosition );
			}

			reader->ReadStruct( m_Position );
			reader->ReadStruct( m_Orientation );
		}
	}

	void Print( FileSys::Writer* writer, const char* prefix )
	{
		gpstring flags;

		if ( m_IsOmni               )  {  flags += " +isOmni";                }
		if ( m_IsAllClients         )  {  flags += " +isAllClients";          }
		if ( m_IsLodfi              )  {  flags += " +isLodfi";               }
		if ( m_IsServerOnly         )  {  flags += " +isServerOnly";          }
		if ( m_IsBucketDirty        )  {  flags += " +isBucketDirty";         }
		if ( m_IsInsideInventory    )  {  flags += " +isInsideInventory";     }
		if ( m_HasPlacement         )  {  flags += " +hasPlacement";          }
		if ( m_DoesInheritPlacement )  {  flags += " +doesInheritPlacement";  }

		writer->WriteF(
				"%s, ptr = 0x%08X, goid = 0x%08X (%s), cloneSourceGoid = 0x%08X (%s), "
				"scid = 0x%08X, playerId = 0x%08X, template = %s, frustum = 0x%08X, createTime = %g, "
				"randomSeed = 0x%08X, flags:%s",
				prefix, m_Go, m_Goid, GoidClassToString( m_Goid ),
				m_CloneSourceGoid, GoidClassToString( m_CloneSourceGoid ),
				m_Scid, m_PlayerId, m_TemplateName.c_str(), m_FrustumMembership,
				m_CreateTime, m_RandomSeed, flags.empty() ? " <none>" : flags.c_str() );

		if ( !m_VariationName.empty() )
		{
			writer->WriteF( ", variation = '%s'", m_VariationName.c_str() );
		}
		if ( !m_PrefixName.empty() )
		{
			writer->WriteF( ", prefix = '%s'", m_PrefixName.c_str() );
		}
		if ( !m_SuffixName.empty() )
		{
			writer->WriteF( ", suffix = '%s'", m_SuffixName.c_str() );
		}

		if ( m_HasPlacement )
		{
			writer->WriteF( ", position = [%g,%g,%g,%s], orientation = [%g,%g,%g,%g]",
					m_Position.pos.x,
					m_Position.pos.y,
					m_Position.pos.z,
					m_Position.node.ToString().c_str(),
					m_Orientation.m_x,
					m_Orientation.m_y,
					m_Orientation.m_z,
					m_Orientation.m_w );

			if ( m_DoesInheritPlacement )
			{
				writer->WriteF( ", rawPosition = [%g,%g,%g,%s]",
						m_RawPosition.pos.x,
						m_RawPosition.pos.y,
						m_RawPosition.pos.z,
						m_RawPosition.node.ToString().c_str() );
			}
		}
	}
};

struct NetLogPacketEntry : PacketInfo
{
	gpstring m_MachineName;

	void Read( NetLogReader* reader )
	{
		reader->ReadPacket( *this, m_MachineName );
	}

	void Print( FileSys::Writer* writer, const char* prefix, bool compatible )
	{
		UINT serialID = *(SHORT*)&*m_Buffer.begin();
		const FuBi::FunctionSpec* spec = compatible ? gFuBiSysExports.FindFunctionBySerialID( serialID ) : NULL;

		gpstring rpc;
		bool printBits = false;
		if ( spec != NULL )
		{
#			if GP_RETAIL

			printBits = true;
			rpc.assignf( "%s [0x%04X]", spec->BuildQualifiedName().c_str(), serialID );

#			else // GP_RETAIL

			printBits = false;
			rpc = spec->BuildQualifiedNameAndParams( const_mem_ptr( &*(m_Buffer.begin() + 2), m_Buffer.size() ), NULL, true, true );

			// if there's any embedded newlines we have to postprocess
			size_t found = 0;
			while ( (found = rpc.find_first_of( "\n\r", found )) != rpc.npos )
			{
				while ( (rpc[ found ] == '\n') || (rpc[ found ] == '\r') )
				{
					++found;
				}

				if ( rpc[ found ] != '\0' )
				{
					rpc.insert( found, "--> " );
				}
				found += 4;
			}

#			endif // GP_RETAIL
		}
		else
		{
			rpc = "<INCOMPATIBLE>";
			printBits = true;
		}

		if ( printBits )
		{
			rpc += " [";
			const BYTE* b, * bbegin = &*m_Buffer.begin() + 2, * bend = &*m_Buffer.end();
			if ( bbegin != bend )
			{
				for ( b = bbegin ; b != bend ; ++b )
				{
					if ( b != bbegin )
					{
						rpc += " ";
					}
					rpc.appendf( "%02x", *b );
				}
			}

			rpc.appendf( "] [0x%04X]", serialID );
		}

		writer->WriteF(
				"%s, machine = 0x%08X (%s), type = %s, retries = %d, rpc = %s",
				prefix, m_machineId, m_MachineName.c_str(), ::ToScreenString( m_Type ),
				m_RetryCount, rpc.c_str() );
	}
};

//////////////////////////////////////////////////////////////////////////////

struct NetLogCreateGoXferEntry
{
	CreateGoXfer m_Xfer;

	void Read( NetLogReader* reader )
	{
		reader->ReadStruct( m_Xfer.m_CreateReq  );
		reader->ReadStruct( m_Xfer.m_StartGoid  );
		reader->ReadStruct( m_Xfer.m_RandomSeed );
		reader->ReadStruct( m_Xfer.m_Standalone );
		reader->ReadBlob  ( m_Xfer.m_DataBuffer );
		m_Xfer.m_Data = const_mem_ptr( &*m_Xfer.m_DataBuffer.begin(), m_Xfer.m_DataBuffer.size() );
	}

	void Print( FileSys::Writer* writer, const char* prefix )
	{
		gpstring str;
#		if !GP_RETAIL
		m_Xfer.RpcToString( str );
#		endif // !GP_RETAIL
		writer->WriteF( "%s, %s", prefix, str.c_str() );
	}
};

struct NetLogCloneGoXferEntry
{
	CloneGoXfer m_Xfer;

	void Read( NetLogReader* reader )
	{
		reader->ReadStruct( m_Xfer.m_CloneReq  );
		reader->ReadStruct( m_Xfer.m_StartGoid  );
		reader->ReadStruct( m_Xfer.m_RandomSeed );
		reader->ReadText  ( m_Xfer.m_TemplateNameBuffer );
		reader->ReadStruct( m_Xfer.m_Standalone );
		reader->ReadBlob  ( m_Xfer.m_DataBuffer );

		if ( m_Xfer.m_TemplateNameBuffer.empty() )
		{
			m_Xfer.m_TemplateName = NULL;
		}
		else
		{
			m_Xfer.m_TemplateName = m_Xfer.m_TemplateNameBuffer;
		}

		m_Xfer.m_Data = const_mem_ptr( &*m_Xfer.m_DataBuffer.begin(), m_Xfer.m_DataBuffer.size() );
	}

	void Print( FileSys::Writer* writer, const char* prefix )
	{
		gpstring str;
#		if !GP_RETAIL
		m_Xfer.RpcToString( str );
#		endif // !GP_RETAIL
		writer->WriteF( "%s, %s", prefix, str.c_str() );
	}
};

//////////////////////////////////////////////////////////////////////////////
// class AutoNetWriter implementation

AutoNetWriter :: AutoNetWriter( FOURCC type, DWORD realSize, bool raw )
	: m_Locker( *gNetLog.m_Critical ), m_Writer( gNetLog.m_WriteBuffer, true )
{
	m_Raw = raw;
	if ( !m_Raw )
	{
		NetLogHeader header( false );
		header.Load( type, realSize );
		header.Write( this );
	}
}

void AutoNetWriter :: WriteGo( Go* go )
{
	NetLogGoEntry goEntry( false );
	goEntry.Load( go );
	goEntry.Write( this );
}

//////////////////////////////////////////////////////////////////////////////
// class NetLogReader implementation

bool NetLogReader :: ReadEntry( NetLogHeader& header )
{
	m_EntryPtr = &*m_EntryBuffer.end();

	bool success = header.Read( this );
	if ( success )
	{
		m_EntryBuffer.resize( header.m_Size );
		m_EntryPtr = &*m_EntryBuffer.end();
		success = Read( &*m_EntryBuffer.begin(), header.m_Size );
		m_EntryPtr = &*m_EntryBuffer.begin();
	}

	return ( success );
}

bool NetLogReader :: ReadEntry( NetLogEntry& entry )
{
	bool success = ReadEntry( scast <NetLogHeader&> ( entry ) );
	if ( success )
	{
		m_EntryBuffer.swap( entry.m_EntryData );
		m_EntryBuffer.clear();
		m_EntryPtr = &*m_EntryBuffer.end();
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class NetLog implementation

// $$$ todo: add in tracking status reports for things like save game, errors
// asserts ignored, etc...

// $$$ todo: add ability to keep old logs around. always keep a log in its own
// file but permit multiples to exist on drive using timestamp built up from
// build # as well as day/time to exist. possibly set a max size and it cleans
// out old logs as necessary.

inline DWORD MakeBuildBits( void )
{
	return ( (GP_DEBUG << 0) | (GP_RELEASE << 1) | (GP_RETAIL << 2) );
}

inline const char* MakeBuildBitsName( DWORD bits )
{
	if ( bits & (1 << 0) )
	{
		return ( "debug" );
	}
	else if ( bits & (1 << 1) )
	{
		return ( "release" );
	}
	else if ( bits & (1 << 2) )
	{
		return ( "retail" );
	}
	return ( "<unknown>" );
}

NetLog :: NetLog( void )
{
	// register translators
	GPDEV_ONLY( ::RegisterTranslators() );

	// permanent objects
	m_Critical = new kerneltool::Critical;
	m_Sink = new NetLogSink;

	// on-demand objects
	m_OutFile         = NULL;
	m_DeflateStreamer = NULL;

	// other stuff
	m_DispatchTempSize = 0;
	m_DispatchTempType = PHT_INVALID;

	// start logging right away
	if (   gConfig.GetBool( "dslog", !GP_RETAIL )
		|| gConfig.GetBool( "netlog", false )
		|| (AppModule::DoesSingletonExist() && gAppModule.TestOptions( AppModule::OPTION_TEST_MODE )) )
	{
		StartLogging( gConfig.GetBool( "netlogappend", false ) );
	}
}

NetLog :: ~NetLog( void )
{
	StopLogging();

	delete ( m_Sink );
	delete ( m_Critical );
}

bool NetLog :: StartLogging( bool appendExisting )
{
	kerneltool::Critical::Lock locker( *m_Critical );

	StopLogging();

	// enable net stream
	gNetContext.Enable();

	// build compressor
	m_DeflateStreamer = new zlib::DeflateStreamer;
	m_DeflateStreamer->Init();
	m_DeflateStreamer->SetChunkBased();

	// build us a name, yah...
	gpstring name;
	name.assignf( "%s_%s.dslog", FileSys::GetFileNameOnly( FileSys::GetModuleFileName() ).c_str(), SysInfo::GetComputerName() );
	name = ReportSys::MakeLogFileName( name );

	// create file, optionally overwriting old
	m_OutFile = new FileSys::File;
	if ( appendExisting )
	{
		m_OutFile->OpenAlways( name );
	}
	else
	{
		m_OutFile->CreateAlways( name );
	}

	// failed?
	if ( !m_OutFile->IsOpen() )
	{
		gperrorf(( "Unable to open log file '%s', error is 0x%08X ('%s')\n",
				   name.c_str(), ::GetLastError(), stringtool::GetLastErrorText().c_str() ));

		StopLogging();
		return ( false );
	}

	// output header
	{
		// $ DO NOT CHANGE THIS WITHOUT UPDATING EXTRACT() AND ++VERSION!

		AutoNetWriter writer( 0, 0, true );

		// product ID
		writer.WriteStruct( REVERSE_FOURCC( DS1_FOURCC_ID ) );
		writer.WriteStruct( REVERSE_FOURCC( NETLOG_FOURCC_ID ) );

		// version
		writer.WriteStruct( HEADER_VERSION );

		// build type
		writer.WriteStruct( MakeBuildBits() );

		// fubi checksum
		FuBi::SysExports::SyncSpec syncSpec;
		if ( FuBi::SysExports::DoesSingletonExist() )
		{
			syncSpec = gFuBiSysExports.MakeSynchronizeSpec();
		}
		syncSpec.Write( writer );

		// product version
		writer.WriteText( SysInfo::GetExeFileVersionText( gpversion::MODE_COMPLETE ) );

		// primary thread ID
		writer.WriteStruct( ::GetCurrentThreadId() );
	}

	// output text header
	{
		ReportSys::StringSink sink;
		ReportSys::LocalContext ctx( &sink, false );
		ReportSys::FileSink::OutputGenericHeader( &ctx, "NetLog" );
		OutputReportString( "[event]", "startup", sink.GetBuffer(), sink.GetBuffer().size() );
	}

	// done
	return ( true );
}

void NetLog :: StopLogging( void )
{
	kerneltool::Critical::Lock locker( *m_Critical );

	// close immediately
	if ( m_OutFile != NULL )
	{
		m_OutFile->Close();
	}

	// kill the rest
	Delete( m_OutFile         );
	Delete( m_DeflateStreamer );

	// clean memory
	m_WriteBuffer.destroy();
	m_CompressBuffer.destroy();

	// not needed any more
	gNetContext.Disable();
}

bool NetLog :: IsLogging( void ) const
{
	return ( (m_OutFile != NULL) && m_OutFile->IsOpen() );
}

void NetLog :: OutputBinary( FOURCC type, const_mem_ptr mem )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( type, mem.size );
		writer.Write( mem.mem, mem.size );
	}
}

void NetLog :: OutputReportString( const char* streamName, const char* eventName, const char* text, int len )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'Strm', len );
		writer.WriteText( streamName );
		writer.WriteText( eventName );
		writer.WriteText( text, len );
	}
}

void NetLog :: OutputDirectPlayMessage( DWORD messageId )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'DPly' );
		writer.WriteStruct( messageId );
	}
}

void NetLog :: OutputGoCreatePacker( const CreateGoXfer& xfer, bool send )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'GoC0' );
		writer.WriteStruct( send );
		writer.WriteXfer( xfer );
	}
}

void NetLog :: OutputGoClonePacker( const CloneGoXfer& xfer, bool send )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'GoC1' );
		writer.WriteStruct( send );
		writer.WriteXfer( xfer );
	}
}

void NetLog :: OutputGoCreation( Go* go )
{
	if ( go == NULL )
	{
		return;
	}

	if ( IsLogging() )
	{
		// pre-deref template in case fuel has to cache in, which may generate
		// gpgenerics/errors, which may deadlock
		go->GetTemplateName();

		AutoNetWriter writer( 'GoCr' );
		writer.WriteGo( go );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() && go->IsGlobalGo() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] Go creation (%s), goid = 0x%08X (%s%s), scid = 0x%08X, template = '%s'",
				 gWorldTime.GetTime(), ::IsPrimaryThread() ? "PRI" : "LDR", go->GetGoid(),
				 GoidClassToString( go->GetGoid() ), go->IsServerOnly() ? ", SERVER-ONLY" : "",
				 go->GetScid(), go->GetTemplateName() ));
		if ( go->HasPlacement() )
		{
			gpnettextf(( ", position = %g,%g,%g,%s",
					go->GetPlacement()->GetPosition().pos.x,
					go->GetPlacement()->GetPosition().pos.y,
					go->GetPlacement()->GetPosition().pos.z,
					go->GetPlacement()->GetPosition().node.ToString().c_str() ));
		}
		gpnettext( "\n" );
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputGoDeletionMarked( Go* go, bool retire, bool fadeOut, bool mpdelayed )
{
	if ( IsLogging() )
	{
		// pre-deref template in case fuel has to cache in, which may generate
		// gpgenerics/errors, which may deadlock
		go->GetTemplateName();

		AutoNetWriter writer( 'GoDM' );
		writer.WriteGo( go );
		writer.WriteStruct( retire );
		writer.WriteStruct( fadeOut );
		writer.WriteStruct( mpdelayed );
	}
}

void NetLog :: OutputGoDeletion( Go* go )
{
	if ( IsLogging() )
	{
		// pre-deref template in case fuel has to cache in, which may generate
		// gpgenerics/errors, which may deadlock
		go->GetTemplateName();

		AutoNetWriter writer( 'GoDl' );
		writer.WriteGo( go );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() && go->IsGlobalGo() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] Go deletion, goid = 0x%08X (%s), scid = 0x%08X, template = '%s'\n",
				 gWorldTime.GetTime(), go->GetGoid(), GoidClassToString( go->GetGoid() ),
				 go->GetScid(), go->GetTemplateName() ));
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputGoDeletionDenied( Goid goid )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'GoDD' );
		writer.WriteStruct( goid );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] Go deletion DENIED (%s) - goid 0x%08X (%s) not found (special case), adding to recently-deleted collection\n",
				 gWorldTime.GetTime(), ::IsPrimaryThread() ? "PRI" : "LDR", goid,
				 GoidClassToString( goid ) ));
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputGoInfo( const char* reason, Go* go )
{
	if ( go == NULL )
	{
		return;
	}

	if ( IsLogging() )
	{
		// pre-deref template in case fuel has to cache in, which may generate
		// gpgenerics/errors, which may deadlock
		go->GetTemplateName();

		AutoNetWriter writer( 'Go' );
		writer.WriteGo( go );
		writer.WriteText( reason );
	}
}

void NetLog :: OutputRpcMcpIn( int followerCount, int packetCount, const_mem_ptr blob )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'MIn', blob.size );
		writer.WriteBlob( blob );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] RPC-IN MCP Update: %d followers, %d commands, size=%d\n",
				gWorldTime.GetTime(), followerCount, packetCount, blob.size ));
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputRpcMcpOut( DWORD machineId, int followerCount, int packetCount, const_mem_ptr blob )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'MOut', blob.size );
		writer.WriteMachineId( machineId );
		writer.WriteBlob( blob );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] RPC-OUT MCP Update to 0x%08X: %d followers, %d commands, size=%d\n",
				gWorldTime.GetTime(), machineId, followerCount, packetCount, blob.size ));
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputRpcInStore( const PacketInfo& packet )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'NInS', packet.m_Buffer.size() );
		writer.WritePacket( packet );
	}

	// $ no text version - too much clutter
}

void NetLog :: OutputRpcInDispatch( const PacketInfo& packet )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'NInD', packet.m_Buffer.size() );
		writer.WritePacket( packet );
	}

#	if !GP_RETAIL
	if (   gNetTextContext.IsEnabled()
		&& Singleton <NetFuBiSend>::DoesSingletonExist()
		&& gNetFuBiSend.IsFilterMatch( gNetFuBiSend.DebugGetRpcName( &*packet.m_Buffer.begin() ) ) )
	{
		m_DispatchTempSize = packet.m_Buffer.size_bytes();
		m_DispatchTempText = gNetFuBiSend.DebugGetRpcInfo( packet.m_machineId, &*packet.m_Buffer.begin(), packet.m_Buffer.size(), true );
		m_DispatchTempType = packet.m_Type;
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputRpcInResult( FuBiCookie result )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'NInR' );
		writer.WriteStruct( result );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() && !m_DispatchTempText.empty() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] RPC-IN-%s=%s: %s, size = %d\n",
				gWorldTime.GetTime(),
				::ToScreenString( m_DispatchTempType ),
				FuBi::ToString( result ),
				m_DispatchTempText.c_str(),
				m_DispatchTempSize ));
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputRpcOut( const PacketInfo& packet )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'NOut', packet.m_Buffer.size() );
		writer.WritePacket( packet );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() )
	{
		AutoRecursion autoRecursion;
		gpnettextf(( "[%7.4f] RPC-OUT to %s, size = %d\n",
				gWorldTime.GetTime(),
				NetFuBiSend::DebugGetRpcInfo( packet.m_machineId, &*packet.m_Buffer.begin(), packet.m_Buffer.size(), true ).c_str(),
				packet.m_Buffer.size_bytes() ));
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputRpcOutDeferred( const FuBi::RpcVerifySpec& spec, Goid routerGoid, const GoidColl& pendingGoids )
{
	if ( IsLogging() )
	{
		AutoNetWriter writer( 'NDef' );
		writer.WriteRpc    ( spec, false );
		writer.WriteStruct ( routerGoid );
		writer.WriteStructs( &*pendingGoids.begin(), pendingGoids.size() );
	}

#	if !GP_RETAIL
	if ( gNetTextContext.IsEnabled() )
	{
		AutoRecursion autoRecursion;

		ReportSys::AutoReport autoReport( &gNetContext );
		gpnettextf(( "[%7.4f] DEFERRED-rpc, call = %s, router goid = 0x%08X (%s)",
				 gWorldTime.GetTime(),
				 spec.m_Function->BuildQualifiedNameAndParams( spec.m_Params, spec.m_This, false ).c_str(),
				 routerGoid, GoHandle( routerGoid )->GetTemplateName() ));

		if ( !pendingGoids.empty() )
		{
			gpnet( ", pending goids: " );
			GoidColl::const_iterator i, ibegin = pendingGoids.begin(), iend = pendingGoids.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i != ibegin )
				{
					gpnet( ", " );
				}

				gpnetf(( "0x%08X (%s)", *i, GoHandle( *i )->GetTemplateName() ));
			}
		}

		gpnet( "\n" );
	}
#	endif // !GP_RETAIL
}

void NetLog :: OutputRaw( const_mem_ptr mem, bool compress )
{
	if ( IsLogging() && (mem.size > 0) )
	{
		if ( compress )
		{
			kerneltool::Critical::Lock locker( *m_Critical );

			// reserve space
			m_CompressBuffer.resize( zlib::GetMaxDeflateSize( mem.size ) );

			// deflate into there
			int size = m_DeflateStreamer->DeflateChunk(
					mem_ptr( &*m_CompressBuffer.begin(), m_CompressBuffer.size_bytes() ),
					mem,
					false );

			// redirect memory
			mem.mem  = &*m_CompressBuffer.begin();
			mem.size = size;
		}

		// store
		m_OutFile->Write( mem.mem, mem.size );
	}
}

bool NetLog :: Extract( const char* fileName )
{

// Open file.

	// first try to open
	FileSys::File inFile;
	if ( !inFile.OpenExisting( fileName, FileSys::File::ACCESS_READ_ONLY, FileSys::File::SHARE_READ_WRITE ) )
	{
		gperrorf(( "Unable to open file '%s' for extraction, error is 0x%08X ('%s')\n",
				   fileName, ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
		return ( false );
	}

	// get size and amount to skip
	DWORD size = inFile.GetSize();
	DWORD skipUntil = 0;
	float skipRatio = gConfig.GetFloat( "skip", 0 );
	if ( skipRatio > 0 )
	{
		skipUntil = (DWORD)(size * skipRatio);
	}

	// get world times to grab
	double minTime = gConfig.GetFloat( "min_world_time" );
	double maxTime = gConfig.GetFloat( "max_world_time" );

	// other options
	bool findBad = gConfig.GetBool( "find_bad", false );

	// get reader
	NetLogReader reader( inFile );

// Read in header.

	// alloc header data
	DWORD tattooId = 0, netlogId = 0, headerVersion = 0, buildBits = 0, primaryThreadId = 0;
	FuBi::SysExports::SyncSpec logSyncSpec;
	gpstring buildText;

	// get header data
	reader.ReadStruct( tattooId );
	reader.ReadStruct( netlogId );
	reader.ReadStruct( headerVersion );

	// mismatch here is bad
	if (   (tattooId != REVERSE_FOURCC( DS1_FOURCC_ID ))
		|| (netlogId != REVERSE_FOURCC( NETLOG_FOURCC_ID ))
		|| (headerVersion != HEADER_VERSION) )
	{
		gperrorf(( "Unable to use file '%s' for extraction, file signature not found/unrecognized\n", fileName ));
		return ( false );
	}

	// get some more fun
	reader.ReadStruct( buildBits );
	reader.ReadStruct( logSyncSpec );
	reader.ReadText  ( buildText );
	reader.ReadStruct( primaryThreadId );

	// that's the end of the uncompressed data
	reader.SetCompressed();

// Check for compatibility.

	// build us a fubi
	FuBi::SysExports sysExports;
	sysExports.ImportBindings();
	FuBi::SysExports::SyncSpec syncSpec = sysExports.MakeSynchronizeSpec();

	// register callbacks for conversions
	GPDEV_ONLY( ::RegisterTranslators() );

	// check compatibility
	bool compatible = true;
	if ( buildBits != MakeBuildBits() )
	{
		compatible = false;
	}
	else if ( logSyncSpec != syncSpec )
	{
		compatible = false;
	}
	else if ( !buildText.same_no_case( SysInfo::GetExeFileVersionText( gpversion::MODE_COMPLETE ) ) )
	{
		compatible = false;
	}

	// warn!
	if ( !compatible )
	{
		gperrorf(( "This build is incompatible with the build used to generate "
				   "the dslog. Extraction is still possible, but no decoding "
				   "of RPC's is possible.\n"
				   "\n"
				   "Log file   : %s\n"
				   "Log build  : %s (type = %s, crc = 0x%08X)\n"
				   "Local build: %s (type = %s, crc = 0x%08X)\n",
				   fileName,
				   buildText.c_str(),
				   MakeBuildBitsName( buildBits ),
				   logSyncSpec.m_Digest,
				   SysInfo::GetExeFileVersionText( gpversion::MODE_COMPLETE ).c_str(),
				   MakeBuildBitsName( MakeBuildBits() ),
				   syncSpec.m_Digest ));
		if ( gConfig.GetBool( "force_compatible", false ) )
		{
			compatible = true;
		}
	}

// Create out file.

	// make filename
	gpstring outFileName( fileName );
	outFileName += "x";

	// make file
	FileSys::File outFile;
	if ( !outFile.CreateAlways( outFileName ) )
	{
		gperrorf(( "Unable to create file '%s' for extraction output, error is 0x%08X ('%s')\n",
				   outFileName, ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
		return ( false );
	}

	// make writer
	FileSys::FileWriter writer( outFile );

// Process.

	// new entries get read in here
	NetLogHeader header;
	NetLogEntry entry;

	// skipped ones go here
	typedef std::list <NetLogEntry> EntryList;
	EntryList skippedList;

	// write header
	writer.WriteText(
			"type (THRED) <system   , global   , world    , delta    , sim    ] #size:real#\n"
			"----------------------------------------------------------------------------------------------------------\n"
			"\n" );

	// process all
	int recordSkipCount = 0;
	gpstring streamName, eventName, text;
	for ( int recordCount = 0 ; ; ++recordCount )
	{

	// Get next entry.

		// fill out from list first
		if ( !skippedList.empty() )
		{
			header = skippedList.front();
			reader.TakeEntryData( skippedList.front().m_EntryData );
			skippedList.pop_front();
		}
		else if ( !reader.ReadEntry( header ) )
		{
			// no more to read or error! all done.
			break;
		}

	// Detect skip.

		bool skipIt = false, streamReadIn = false;

		// skipping?
		if ( reader.GetPos() < (int)skipUntil )
		{
			skipIt = true;
		}
		else if ( (minTime != 0) && (header.m_WorldTime < minTime) )
		{
			skipIt = true;
		}
		else if ( (maxTime != 0) && (header.m_WorldTime > maxTime) )
		{
			// note: don't bail out early here because with multiple
			// sessions we'll get a reset of world time.
			skipIt = true;
		}
		else if ( findBad )
		{
			if ( header.DefinitelyNotBad() )
			{
				skipIt = true;
			}
			else if ( header.m_Type == 'Strm' )
			{
				// check type
				reader.ReadText( streamName );
				streamReadIn = true;
				if (   !streamName.same_no_case( "assert"   )
					&& !streamName.same_no_case( "error"    )
					&& !streamName.same_no_case( "warning"  )
					&& !streamName.same_no_case( "fatal"    )
					&& !streamName.same_no_case( "debugger" )
					&& !streamName.same_no_case( "[event]"  ) )
				{
					skipIt = true;
				}
			}
		}

	// Skip.

		if ( skipIt )
		{
			++recordSkipCount;
			continue;
		}

	// Output.

		// out header
		writer.WriteF( "%-4.4s (%s) <%09.4f, %09.4f, %09.4f, %09.4f, %07d> #%04d:%04d# ",
				stringtool::MakeFourCcString( header.m_Type ).c_str(),
				(header.m_ThreadId == primaryThreadId) ? "*PRI*" : gpstringf( "%05d", header.m_ThreadId ).c_str(),
				header.m_SystemSeconds,
				header.m_GlobalSeconds,
				header.m_WorldTime,
				header.m_DeltaTime,
				header.m_SimCount,
				header.m_Size,
				header.m_RealSize );

		switch ( header.m_Type )
		{
			case ( 'Strm' ):
			{
				if ( !streamReadIn )
				{
					reader.ReadText( streamName );
				}

				reader.ReadText( eventName );
				reader.ReadText( text );
				writer.WriteF( "%s%s%s: %s",
						streamName.c_str(),
						eventName.empty() ? "" : ":",
						eventName.c_str(),
						text.c_str() );
			}
			break;

			case ( 'GoCr' ):
			{
				NetLogGoEntry goEntry( false );
				goEntry.Read( &reader );
				goEntry.Print( &writer, "Go creation" );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'GoDl' ):
			{
				NetLogGoEntry goEntry( false );
				goEntry.Read( &reader );
				goEntry.Print( &writer, "Go deletion" );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'GoDM' ):
			{
				NetLogGoEntry goEntry( false );
				goEntry.Read( &reader );
				goEntry.Print( &writer, "Go marked for deletion" );

				bool retire, fadeOut, mpdelayed;
				reader.ReadStruct( retire );
				reader.ReadStruct( fadeOut );
				reader.ReadStruct( mpdelayed );

				if ( retire )
				{
					writer.WriteText( " +retire" );
				}
				if ( fadeOut )
				{
					writer.WriteText( " +fadeOut" );
				}
				if ( mpdelayed )
				{
					writer.WriteText( " +mpdelayed" );
				}

				writer.WriteText( "\n" );
			}
			break;

			case ( 'GoDD' ):
			{
				Goid goid = GOID_INVALID;
				reader.ReadStruct( goid );
				writer.WriteF(
						"Go deletion DENIED, goid 0x%08X (%s) not found "
						"(special case), adding to recently-deleted collection\n",
						goid, GoidClassToString( goid ) );
			}
			break;

			case ( 'Go' ):
			{
				NetLogGoEntry goEntry( false );
				goEntry.Read( &reader );
				gpstring reason;
				reader.ReadText( reason );
				goEntry.Print( &writer, gpstringf( "Go info (%s)", reason.c_str() ) );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'NInS' ):
			{
				NetLogPacketEntry packetEntry;
				packetEntry.Read( &reader );
				packetEntry.Print( &writer, "RPC-IN-STORE", compatible );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'NInD' ):
			{
				// get dispatch entry
				NetLogPacketEntry dispatchEntry;
				dispatchEntry.Read( &reader );

				// cookie result goes here, default to junk
				FuBiCookie cookie = RPC_FAILURE_IGNORE;

				// read until we get our result key, packing skipped ones away
				// until we find it (may be intermediate junk in the way)
				bool found = false;
				while ( reader.ReadEntry( entry ) )
				{
					if ( entry.m_Type == 'NInR' )
					{
						reader.TakeEntryData( entry.m_EntryData );
						reader.ReadStruct( cookie );
						found = true;
						break;
					}
					else
					{
						skippedList.push_back( entry );

						// got to the next dispatch, abort
						if ( entry.m_Type == 'NInD' )
						{
							break;
						}
					}
				}

				// convert to cookie
				const char* cookieStr = found ? FuBi::ToString( cookie ) : "<NOT FOUND>";
				if ( cookieStr == NULL )
				{
					cookieStr = "<INVALID>";
				}

				// output
				if ( !findBad || !found || (cookie == RPC_FAILURE_IGNORE) || (cookie == RPC_FAILURE) )
				{
					dispatchEntry.Print( &writer, gpstringf( "RPC-IN-DISPATCH=%s", cookieStr ).c_str(), compatible );
					writer.WriteText( "\n" );
				}
			}
			break;

			case ( 'NOut' ):
			{
				NetLogPacketEntry packetEntry;
				packetEntry.Read( &reader );
				packetEntry.Print( &writer, "RPC-OUT", compatible );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'GoC0' ):
			{
				bool send;
				reader.ReadStruct( send );

				NetLogCreateGoXferEntry goCreateXferEntry;
				goCreateXferEntry.Read( &reader );
				goCreateXferEntry.Print( &writer, send ? "GO-CREATE-OUT" : "GO-CREATE-IN" );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'GoC1' ):
			{
				bool send;
				reader.ReadStruct( send );

				NetLogCloneGoXferEntry goCloneXferEntry;
				goCloneXferEntry.Read( &reader );
				goCloneXferEntry.Print( &writer, send ? "GO-CLONE-OUT" : "GO-CLONE-IN" );
				writer.WriteText( "\n" );
			}
			break;

			case ( 'NDef' ):
			{
				FuBi::RpcVerifySpec rpc;
				bool isRpcPacked;
				Buffer buffer;

				reader.ReadRpc( rpc, isRpcPacked, buffer );

				writer.WriteF( "RPC-OUT-DEFERRED, address = 0x%08X, rpc = %s [0x%04X]",
							   rpc.m_Address,
							   rpc.m_Function ? rpc.m_Function->BuildQualifiedName().c_str() : "<UNKNOWN>",
							   rpc.m_Function ? rpc.m_Function->m_SerialID : 0 );
				if ( isRpcPacked )
				{
					writer.WriteText( " +rpcPacked" );
				}
				writer.WriteF( ", size = %d, 'this' = 0x%08X\n", rpc.m_Params.size, rpc.m_This );
			}
			break;

			case ( 'DPly' ):
			{
				DWORD messageId;
				reader.ReadStruct( messageId );

				const char* message = ::LookupDPlayMessage( messageId );
				writer.WriteF(
						"DirectPlay message 0x%08X ('%s')\n",
						messageId, message ? message : "<UNKNOWN>" );
			}
			break;

			case ( 'GRng' ):
			{
				GoRngNR::DwordColl coll;
				coll.resize( header.m_Size / sizeof( DWORD ) );
				reader.Read( &*coll.begin(), header.m_Size );

				writer.WriteF( "Go RNG, count = %d, random_numbers = ", coll.size() );

				GoRngNR::DwordColl::const_iterator i, ibegin = coll.begin(), iend = coll.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( i != ibegin )
					{
						writer.WriteText( ", " );
					}
					writer.WriteF( "0x%08X", *i );
				}

				writer.WriteText( "\n" );
			}
			break;

			default:
			{
				writer.WriteText( "UNEXPECTED/UNHANDLED TYPE!!!\n" );
			}
		}
	}

	// success!
	gpmessageboxf(( "Conversion complete, %d records skipped, %d records extracted! Output file in '%s'.\n",
					recordSkipCount, recordCount - recordSkipCount, outFileName.c_str() ));
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
