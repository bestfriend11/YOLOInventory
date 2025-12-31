#include "precomp_world.h"

#include "netfubi.h"
#include "fubi.h"
#include "gofollower.h"
#include "netlog.h"
#include "netpipe.h"
#include "ReportSys.h"
#include "FubiSchema.h"
#include "server.h"
#include "DebugProbe.h"
#include "World.h"
#include "WorldTime.h"


using namespace FuBi;
using namespace std;

AutoPacketsList::~AutoPacketsList()
{
	FreeClear();
}

void AutoPacketsList::FreeClear()
{
	for( AutoPacketsList::iterator i = begin(); i != end(); ++i )
	{
		delete *i;
	}
	clear();
}

AutoMachineToPacketsMap::~AutoMachineToPacketsMap()
{
	FreeClear();
}

void AutoMachineToPacketsMap::FreeClear()
{
	for( AutoMachineToPacketsMap::iterator i = this->begin(); i != this->end(); ++i )
	{
		delete (*i).second;
	}
	clear();
}


////////////////////////////////////////////////////////////////////////////////
//	NetFuBiSend
NetFuBiSend :: NetFuBiSend()
{
	Init();
}


NetFuBiSend :: ~NetFuBiSend()
{
	Shutdown();
}


void NetFuBiSend :: Reset()
{
	Shutdown();
	Init();
}


void NetFuBiSend :: Init()
{
	m_bMute					= false;
	m_ForceUpdateRequested	= false;
	m_EnableForceUpdate		= true;
	m_CurrentDiscreteTime	= 0;
	m_TimeLastSend			= 0;
	m_SendDelay				= 1.0f / 4.0f;    // max send period

	m_TempToMachineId		= NULL;
	m_TempToMachineIdStart	= NULL;
	m_TempToMachineIdCount	= 0;
	m_TempRetry				= 0;
}


void NetFuBiSend :: Shutdown()
{
	m_machinepackets.FreeClear();
}


void NetFuBiSend :: OnMachineDisconnected( DWORD machineId )
{
	// delete packets queued to the machine
	AutoMachineToPacketsMap::iterator im = m_machinepackets.find( machineId );
	if( im != m_machinepackets.end() )
	{
		m_machinepackets.erase( im );
	}
}


void NetFuBiSend :: Update()
{
	GPPROFILERSAMPLE( "NetFuBiSend :: Update", SP_NET | SP_NET_SEND );

	kerneltool::Critical::Lock lock( m_Critical );
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if( !NetPipe::DoesSingletonExist() || !session )
	{
		return;
	}

	m_CurrentDiscreteTime = GetGlobalSeconds();

	Send();

	if( m_ForceUpdateRequested )
	{
		PrivateForceUpdate();
		m_ForceUpdateRequested = false;
	}
}


void NetFuBiSend :: ForceUpdate()
{
	m_Critical.Enter();

	//m_ForceUpdateRequested = true;
	PrivateForceUpdate();

	m_Critical.Leave();
}


void NetFuBiSend :: PrivateForceUpdate()
{
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if( !m_EnableForceUpdate || !session )
	{
		return;
	}

	Send( true );
}


FuBiCookie NetFuBiSend :: BeginRPC( DWORD MachineId, bool retry )
{
	m_Critical.Enter();

	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( BeginRPC );
	RECURSE_LOCK_FOR_WRITE( BeginRPC );

	if( !m_Buffer.empty() )
	{
		gpassertm( 0, "WARNING!  Buffer should be empty at this point.  Contents will be purged." );
		m_Buffer.clear();
	}

	// store some vars we'll need in EndRPC

	if( gServer.IsLocal() )
	{
		gpassertm( MachineId != RPC_TO_SERVER, "Server machine shouldn't try to RPC to itself at this point." );
	}

	m_TempToMachineId		= MachineId;
	m_TempToMachineIdStart	= &m_TempToMachineId;
	m_TempToMachineIdCount	= 1;

	m_TempRetry				= retry;

	return RPC_SUCCESS;
}


FuBiCookie NetFuBiSend :: BeginMultiRPC( DWORD* MachineIDStart, int addressCount, bool retry )
{
	m_Critical.Enter();

	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( BeginMultiRPC );
	RECURSE_LOCK_FOR_WRITE( BeginMultiRPC );

	m_TempToMachineIdStart	= MachineIDStart;
	m_TempToMachineIdCount	= addressCount;

	m_TempRetry				= retry;

	return RPC_SUCCESS;
}


DWORD NetFuBiSend :: EndRPC( void )
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( EndRPC );
	RECURSE_LOCK_FOR_WRITE( EndRPC );

	////////////////////////////////////////
	//	do the real work only if we've got a net connection

	DWORD packetSize = 0;
	if( NetPipe::DoesSingletonExist() && !m_bMute )
	{
		NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

		////////////////////////////////////////
		//	add to packet DB

		if( session )
		{
			for( DWORD iAddress = 0; iAddress != m_TempToMachineIdCount; ++iAddress )
			{
				DWORD machineId = m_TempToMachineIdStart[iAddress];

				// clients can only possibly RPC to the server

				if( IsServerRemote() )
				{
					if( machineId != RPC_TO_SERVER )
					{
						gperror( "client can only RPC to server!" );
						continue;
					}
				}

				if( (machineId == RPC_TO_ALL) || (machineId == RPC_TO_OTHERS) )
				{
					bool sentToServer = false;
					
					// convention - send to all others who already have a player
					
					for( Server::PlayerColl::iterator ip = gServer.GetPlayersBegin(); ip != gServer.GetPlayersEnd(); ++ip )
					{
						if( !IsValid(*ip) )
						{
							continue;
						}

						if( (*ip)->GetMachineId() == RPC_TO_LOCAL )
						{
							continue;
						}

						if( ( (*ip)->GetMachineId() == RPC_TO_SERVER ) && sentToServer )
						{
							continue;
						}

						if( (*ip)->GetMachineId() == RPC_TO_SERVER )
						{
							sentToServer = true;
						}

						StorePacket( (*ip)->GetMachineId(), m_TempRetry, m_Buffer );
						packetSize += m_Buffer.size();
					}
				}
				else if( machineId != RPC_TO_LOCAL )
				{
					StorePacket( machineId, m_TempRetry, m_Buffer );
					packetSize += m_Buffer.size();
				}
			}
		}
	}

	////////////////////////////////////////
	//	clear temp vars

	m_Buffer.clear();

	m_TempToMachineIdStart	= NULL;
	m_TempToMachineIdCount	= 0;
	m_TempToMachineId		= 0;
	m_TempRetry				= false;

	m_Critical.Leave();

	return ( packetSize );
}


void NetFuBiSend :: StorePacket( 	const DWORD machineId,
									const bool retry,
									ByteBuffer const & packet )
{
	kerneltool::Critical::Lock lock( m_Critical );

	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( StorePacket );
	RECURSE_LOCK_FOR_WRITE( StorePacket );

	gpassert( machineId != RPC_TO_ALL );

	////////////////////////////////////////
	//	look up machine
	AutoMachineToPacketsMap::iterator imachine = m_machinepackets.find( machineId );
	AutoPacketsList * ppl = NULL;

	if( imachine == m_machinepackets.end() )
	{
		ppl = new AutoPacketsList;
		gpverify( m_machinepackets.insert( make_pair( machineId, ppl ) ).second );
	}
	else
	{
		ppl = (*imachine).second;
	}

	////////////////////////////////////////
	//	insert packet
	PacketInfo * info = new PacketInfo( machineId, packet, retry ? PHT_RETRY_PACKET : PHT_PACKET);

	ppl->push_back( info );
}


void NetFuBiSend :: Send( bool force )
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( Send );
	RECURSE_LOCK_FOR_WRITE( Send );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if( !session )
	{
		return;
	}

	if( !force )
	{
		const float secondsElapsed = max( 0.0f, float( PreciseSubtract(m_CurrentDiscreteTime, m_TimeLastSend) ) );
		if( secondsElapsed < m_SendDelay )
		{
			return;
		}
	}

	m_TimeLastSend = m_CurrentDiscreteTime;

	// iterate over all known connections

	for( NetPipeMachineColl::const_iterator ic = session->GetMachinesBegin(); ic != session->GetMachinesEnd(); ++ic )
	{
		gpassert( (*ic)->IsRemote() );

		// temp buffer
		ByteBuffer blob;

		// store packet header
		PacketCollHeader blobHeader;
		Store( blobHeader, blob );

		// extract blobs of limited size and send
		if( BuildBlob( (*ic)->m_Id, blob ) )
		{
			// revisit blob header and mark the size
			((PacketCollHeader*)&*blob.begin())->m_Size = blob.size();

			// send blob over the network
			Send( (*ic)->m_Id, blob );
		}

		// clear buffer and build MCP updates
		blob.clear();
	}
}


bool NetFuBiSend :: BuildBlob( DWORD machineId, ByteBuffer & out )
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( BuildBlob );
	RECURSE_LOCK_FOR_WRITE( BuildBlob );

	bool packetsFound = false;

	AutoMachineToPacketsMap::iterator imachine = m_machinepackets.find( machineId );

	if( imachine != m_machinepackets.end() )
	{
		// bundle packets
		packetsFound = packetsFound || ExtractPackets( (*imachine).second, out );
		DeleteMarkedForDeletionPackets( (*imachine).second );
	}

	return( packetsFound );
}


bool NetFuBiSend :: ExtractPackets( AutoPacketsList * ppackets, // just to save finding it again from machineId
									ByteBuffer & out )
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( ExtractPackets );
	RECURSE_LOCK_FOR_WRITE( ExtractPackets );

	bool extractedPackets = false;

	if (ppackets->begin() != ppackets->end())
	{
		DWORD currentSize = 0;

		////////////////////////////////////////////////////////////////////////////////
		//	extract the actual packets

		AutoPacketsList::iterator ipacket;

		for( ipacket = ppackets->begin(); ipacket != ppackets->end(); ++ipacket )
		{
			////////////////////////////////////////
			//	make sure we're within size constraint

			currentSize += (*ipacket)->m_Buffer.size();

			////////////////////////////////////////
			//	emit packet

			PacketHeader packetHeader( (*ipacket)->m_Type );
			packetHeader.m_Size	= sizeof( PacketHeader ) + (*ipacket)->m_Buffer.size();
			(*ipacket)->m_MarkedForDeletion = true;

			Store( packetHeader, out );
			Store( &*(*ipacket)->m_Buffer.begin(), (*ipacket)->m_Buffer.size(), out );

			extractedPackets = true;

#			if !GP_RETAIL
			////////////////////////////////////////
			//	debug bandwidth tracking

			unsigned char * buffer = &(*ipacket)->m_Buffer.front();
			UINT serialID = *((SHORT*)buffer);
			const FunctionSpec * spec = gFuBiSysExports.FindFunctionBySerialID( serialID );
			gpassert( spec );

			gpstring functionName = spec->BuildQualifiedName();

			StringToInstanceCountMap::iterator i = m_DebugCallHistogram.find( functionName );

			if( i != m_DebugCallHistogram.end() )
			{
				(*i).second.m_CallCount			+= 1;
				(*i).second.m_CallTotalSize		+= (*ipacket)->m_Buffer.size();
				(*i).second.m_CallInstanceSize	= (*ipacket)->m_Buffer.size();
			}
			else
			{
				m_DebugCallHistogram.insert( make_pair( functionName, DebugCallInfo( (*ipacket)->m_Buffer.size() ) ) );
			}

#			endif // !GP_RETAIL

			gplog( OutputRpcOut( **ipacket ) );
		}
	}
	return( extractedPackets );
}


void NetFuBiSend :: DeleteMarkedForDeletionPackets( AutoPacketsList * ppackets ) 
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( DeleteMarkedForDeletionPackets );
	RECURSE_LOCK_FOR_WRITE( DeleteMarkedForDeletionPackets );

	for( AutoPacketsList::iterator ipacket = ppackets->begin(); ipacket != ppackets->end(); )
	{
		if( (*ipacket)->m_MarkedForDeletion )
		{
			ipacket = ppackets->erase( ipacket );
		}
		else
		{
			++ipacket;
		}
	}
}


bool NetFuBiSend :: Send( const DWORD machineId, ByteBuffer & buffer )
{
	CHECK_PRIMARY_THREAD_ONLY;
	RECURSE_DECLARE( Send );
	RECURSE_LOCK_FOR_WRITE( Send );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	bool success = false;

	if( NetPipe::DoesSingletonExist() && session )
	{
		if( !gNetPipe.Send( machineId, &*buffer.begin(), buffer.size() ) )
		{
			gpwarning( "NetFuBiSend :: Transport - failed." );
		}
	}
	return success;
}


#if !GP_RETAIL

void NetFuBiSend :: Dump( ReportSys::ContextRef ctx )
{
	ReportSys::AutoReport autoReport( ctx );
/*
	ctx->Output( "\nnormal packet db:\n" );
	ctx->Indent();
	Dump( m_PacketDb, ctx );
	ctx->Outdent();

	ctx->Output( "\nretry packet db:\n" );
	ctx->Indent();
	Dump( m_PacketRetryDb, ctx );
	ctx->Outdent();
*/
}


static void AddFilters( std::vector <gpstring> & filters, const gpstring& pattern )
{
	stringtool::Extractor extractor( ",", pattern );

	gpstring next;
	while ( extractor.GetNextString( next ) )
	{
		filters.push_back( next );
	}
}


void NetFuBiSend :: AddReportFilters( const gpstring& filter )
{
	::AddFilters( m_ReportFilters, filter );
}


void NetFuBiSend :: SetReportFilters( const gpstring& filter )
{
	m_ReportFilters.clear();
	::AddFilters( m_ReportFilters, filter );
}


bool NetFuBiSend :: IsFilterMatch( const char* functionName ) const
{
	if ( m_ReportFilters.empty() )
	{
		return ( true );
	}

	std::vector <gpstring>::const_iterator i, ibegin = m_ReportFilters.begin(), iend = m_ReportFilters.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( stringtool::IsDosWildcardMatch( *i, functionName ) )
		{
			return ( true );
		}
	}

	return ( false );
}


gpstring NetFuBiSend :: DebugGetRpcName( unsigned char const * buffer )
{
	UINT serialID = *((SHORT*)buffer);
	const FunctionSpec * spec = gFuBiSysExports.FindFunctionBySerialID( serialID );
	if( spec )
	{
		return( spec->BuildQualifiedName() );
	}
	else
	{
		return( "unknown" );
	}
}


gpstring NetFuBiSend :: DebugGetRpcInfo( DWORD machineId, unsigned char const * buffer, DWORD packetSize, bool isRpcPacked )
{
	gpstring result;

	UINT serialID = *((SHORT*)buffer);

	const FunctionSpec * spec = gFuBiSysExports.FindFunctionBySerialID( serialID );

	gpwstring machineName( L"unknown" );

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();

	if( session && session->GetMachine( machineId ) )
	{
		machineName = session->GetMachine( machineId )->m_wsName;
	}

	result.assignf(	"machine '%S', call=%s",
					machineName.c_str(),
					spec ? spec->BuildQualifiedNameAndParams( const_mem_ptr( buffer + 2, packetSize ), NULL, isRpcPacked ).c_str() : "unknown" );

	return( result );
}


void NetFuBiSend :: GetDebugCallHistogram( ReportSys::ContextRef finalctx, bool fReset )
{
	if( m_DebugCallHistogram.empty() )
	{
		return;
	}

	if (fReset)
		m_DebugCallHistogram.clear();

	finalctx->Output( "\noutgoing traffic:\n" );

	// dump objects into sink
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext ctx( &sink, false );

	ctx.SetSchema( new ReportSys::Schema( 5, "called", "last_size", "sum_size", "pct", "call_name" ), true );

	int listSize = 0;
	for( StringToInstanceCountMap::iterator i = m_DebugCallHistogram.begin(); i != m_DebugCallHistogram.end(); ++i, ++listSize )
	{
		ReportSys::AutoReport autoReport( ctx );
		ctx.OutputFieldF( "%8d", (*i).second.m_CallCount );
		ctx.OutputFieldF( "%8d",(*i).second.m_CallInstanceSize );
		ctx.OutputFieldF( "%8d",(*i).second.m_CallTotalSize );
		ctx.OutputFieldF( "%2.2f", float( (*i).second.m_CallTotalSize ) / float( gNetPipe.GetTotalBytesSent() ) );
		ctx.OutputField( (*i).first );
	}

	// out header
	//finalctx->OutputRepeatWidth( sink.CalcDividerWidth(), "=" );

	// out table
	sink.Sort( 2, false );
	sink.OutputReport( finalctx, 10 );
}

#endif



///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


NetFuBiReceive :: NetFuBiReceive()
{
	Init();
}


NetFuBiReceive :: ~NetFuBiReceive()
{
	Shutdown();
}


void NetFuBiReceive :: Init()
{
	m_bMute							= false	  ;
	m_ReceiveFromMachineId          = 0       ;
	m_ReceiveBuffer                 = NULL    ;
	m_ReceiveBufferPoint            = NULL    ;
	m_ReceiveBufferSize             = 0       ;
	m_LastUpdateTime                = 0       ;
	m_SecondsElapsedSinceLastUpdate = 0       ;
	m_ReadCurrentMachineId          = 0       ;
	m_LocalRetryDuration            = 120.0   ;
	m_LocalRetryPeriod              = 0.25f   ;
	m_LocalRetryLastProcessedTime	= 0		  ;

	m_InExecuteCurrentRpcScope      = false   ;
	m_InUpdateScope                 = false   ;
}


void NetFuBiReceive :: Shutdown()
{
	m_machinepackets.FreeClear();
	m_machineLRpackets.FreeClear();
}


void NetFuBiReceive :: OnMachineDisconnected( DWORD machineId )
{
	AutoMachineToPacketsMap::iterator imp = m_machinepackets.find( machineId );
	if( imp != m_machinepackets.end() )
	{
		m_machinepackets.erase( imp );
	}

	imp = m_machineLRpackets.find( machineId );
	if( imp != m_machineLRpackets.end() )
	{
		m_machineLRpackets.erase( imp );
	}
}


void NetFuBiReceive :: Reset()
{
	Shutdown();
	Init();
}


void NetFuBiReceive :: Update()
{
	GPPROFILERSAMPLE( "NetFuBiReceive :: Update", SP_NET | SP_NET_RECEIVE );

	if( !NetPipe::DoesSingletonExist() )
	{
		return;
	}

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( !session )
	{
		return;
	}

	if( m_InUpdateScope )
	{
		//gpwarning( "NetFuBiReceive :: Update() - re-entry avoided.\n" );
		return;
	}

	m_InUpdateScope = true;
	double currentTime = GetGlobalSeconds();
	m_SecondsElapsedSinceLastUpdate = PreciseSubtract(currentTime, m_LastUpdateTime);

	Receive();

	//gpnet( "NetFuBiReceive :: Update() - BEGIN\n" );

	if( PreciseAdd(m_LocalRetryLastProcessedTime, m_LocalRetryPeriod) < currentTime )
	{
		m_LocalRetryLastProcessedTime = currentTime;

		ProcessLocalRetryPackets();
	}

	//gpnet( "NetFuBiReceive :: Update() - END\n\n" );

	m_LastUpdateTime = currentTime;
	m_InUpdateScope = false;
}


void NetFuBiReceive :: Receive()
{
	if( !NetPipe::DoesSingletonExist() )
	{
		return;
	}

	kerneltool::Critical::Lock lock1( gNetPipe.GetReceiveCritical() );
	kerneltool::Critical::Lock lock2( gNetPipe.GetNotifyCritical()  );	

	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( !session )
	{
		return;
	}

	if( !gNetPipe.FindFirstReceivePacket() )
	{
		return;
	}

	gpnet( "NetFuBiReceive :: Receive() - receive buffers present - begin processing\n" );

//	gpnet( "\n-----===== BEGIN store incoming packets\n" );

	DWORD machineId	= 0;
	DWORD size		= 0;
	BYTE * buffer	= NULL;

	while( gNetPipe.HasOpenSession() && gNetPipe.GetReceivePacket( machineId, buffer, size ) )
	{
		FuBi::SetCallerMachineId( machineId );

		m_ReceiveFromMachineId	= machineId;

		m_ReceiveBuffer 		= buffer;
		m_ReceiveBufferPoint 	= buffer;
		m_ReceiveBufferSize		= size;

		if( !m_bMute )
		{
			ReadBuffer();
			gpassertm( m_ReceiveBufferPoint == ( m_ReceiveBuffer + m_ReceiveBufferSize ), "Irregular buffer read." );
		}

		m_ReceiveFromMachineId	= 0;

		m_ReceiveBuffer 		= NULL;
		m_ReceiveBufferPoint 	= NULL;
		m_ReceiveBufferSize		= 0;

		if( gNetPipe.IsCompressed() )
		{
			Delete( buffer );
		}

		machineId	= 0;
		size		= 0;
		buffer		= NULL;
	}

	FuBi::SetCallerMachineId( RPC_TO_LOCAL );

	gNetPipe.ReturnProcessedReceivePackets();

//	gpnet( "-----===== END store incoming packets\n\n" );
}


void NetFuBiReceive :: ReadBuffer()
{
	while( m_ReceiveBufferPoint < ( m_ReceiveBuffer + m_ReceiveBufferSize ) )
	{
		switch( (ePacketHeaderType)(*m_ReceiveBufferPoint) )
		{
			case PHT_PACKET_COLL:
			{
				ReadBlob();
				break;
			}
			default:
			{
				gpassertm( 0, "Unknown packet type.  Read will abort." );
				return;
			}
		}
	}

	ProcessReceivePackets();

	gpassertm( m_ReceiveBufferPoint == ( m_ReceiveBuffer + m_ReceiveBufferSize ), "irregular buffer growth." );
}


void NetFuBiReceive :: ReadBlob()
{
	BYTE * blobEndPoint = m_ReceiveBufferPoint;

	PacketCollHeader header;
	ReadCopy( header );

	blobEndPoint += header.m_Size;

	while( m_ReceiveBufferPoint < blobEndPoint )
	{
		switch( (ePacketHeaderType)(*m_ReceiveBufferPoint) )
		{
			case PHT_RETRY_PACKET:
			case PHT_PACKET:
			{
				ReadPacket();
				break;
			}
			default:
			{
				gpassertm( 0, "Unsupported packet type.  Read will abort." );
				return;
			}
		}
	}

//	gpassertm( timeRead, "Every read must contain a time-stamp." );
}


void NetFuBiReceive :: ReadPacket()
{
	gpassertm( (ePacketHeaderType)(*m_ReceiveBufferPoint) == PHT_PACKET ||
				(ePacketHeaderType)(*m_ReceiveBufferPoint) == PHT_RETRY_PACKET, "Invalid data." );

	PacketHeader header( PHT_INVALID );
	ReadCopy( header );

	ByteBuffer packet( m_ReceiveBufferPoint, m_ReceiveBufferPoint + header.m_Size - sizeof( PacketHeader ) );

	StoreReceivePacket( m_ReceiveFromMachineId, static_cast<ePacketHeaderType>(header.m_Type), packet );

	m_ReceiveBufferPoint += header.m_Size - sizeof( PacketHeader );
}


////////////////////////////////////////////////////////////////////////////////
//
void NetFuBiReceive :: StoreReceivePacket( DWORD machineId, const ePacketHeaderType packetType, ByteBuffer const & packet )
{
	////////////////////////////////////////
	//	look up machine

	AutoMachineToPacketsMap::iterator imp = m_machinepackets.find(machineId);

	if( imp == m_machinepackets.end() )
	{
		std::pair <AutoMachineToPacketsMap::iterator, bool> rc =
				m_machinepackets.insert( make_pair( machineId, new AutoPacketsList ) );
		gpassert(rc.second);
		imp = rc.first;
	}

	AutoPacketsList * ppackets = (*imp).second;
	PacketInfo * packetInfo = new PacketInfo( machineId, packet, packetType);
	ppackets->insert(ppackets->end(), packetInfo);

	gplog( OutputRpcInStore( *packetInfo ) );
}


bool NetFuBiReceive :: ProcessReceivePackets()
{
	for( AutoMachineToPacketsMap::iterator imp = m_machinepackets.begin(); imp != m_machinepackets.end(); ++imp )
	{
		AutoPacketsList * ppackets = (*imp).second;
		for (AutoPacketsList::iterator ip = ppackets->begin(); ip != ppackets->end(); ++ip)
		{
			const DWORD fromMachineId	= (*imp).first;
			PacketInfo * packetInfo		= (*ip);

			gplog( OutputRpcInDispatch( *packetInfo ) );

			m_Buffer = packetInfo->m_Buffer;
			FuBiCookie result = ExecuteCurrentRpc( fromMachineId );

			gplog( OutputRpcInResult( result ) );

			if( packetInfo->IsRetry() && result == RPC_FAILURE )
			{
				InsertLocalRetryPacket( fromMachineId, packetInfo );
			}
		}
		ppackets->FreeClear();
	}

	return( true );
}


////////////////////////////////////////////////////////////////////////////////
//
void NetFuBiReceive :: InsertLocalRetryPacket( DWORD machineId, PacketInfo const * packetInfo )
{
	////////////////////////////////////////
	//	look up machine

	AutoMachineToPacketsMap::iterator imp = m_machineLRpackets.find( machineId );

	if( imp == m_machineLRpackets.end() )
	{
		std::pair <AutoMachineToPacketsMap::iterator, bool> rc =
				m_machineLRpackets.insert( make_pair( machineId, new AutoPacketsList ) );
		gpassert( rc.second );
		imp = rc.first;
	}

	AutoPacketsList * ppackets = (*imp).second;
	PacketInfo * info = new PacketInfo(*packetInfo);
	info->m_TimeStored = GetGlobalSeconds();

	ppackets->insert( ppackets->end(), info );
}


////////////////////////////////////////////////////////////////////////////////
//
bool NetFuBiReceive :: ProcessLocalRetryPackets()
{
	////////////////////////////////////////
	//	iterate over all machine buffers

	double currentTime = GetGlobalSeconds();

	for( AutoMachineToPacketsMap::iterator imp = m_machineLRpackets.begin(); imp != m_machineLRpackets.end(); ++imp )
	{
		AutoPacketsList * ppackets = (*imp).second;
		for( AutoPacketsList::iterator ip = ppackets->begin(); ip != ppackets->end(); )
		{
			gpassert( (*ip)->IsRetry() );

			const DWORD fromMachineId	= (*imp).first;
			PacketInfo * packetInfo		= (*ip);

			packetInfo->m_RetryCount++;

#if !GP_RETAIL
			if( (packetInfo->m_RetryCount % 100 ) == 0 )
			{
				gperrorf(( "Getting %d RPC local retries for %s\n", packetInfo->m_RetryCount, NetFuBiSend::DebugGetRpcName( &*packetInfo->m_Buffer.begin() ) ));
			}
#endif
			////////////////////////////////////////
			//	setup for call

			gplog( OutputRpcInDispatch( *packetInfo ) );

			m_Buffer = packetInfo->m_Buffer;
			FuBiCookie result = ExecuteCurrentRpc( fromMachineId );

			gplog( OutputRpcInResult( result ) );

			////////////////////////////////////////
			//	treat RPC success/failure

			bool retryExpired = PreciseSubtract(currentTime, packetInfo->m_TimeStored) > m_LocalRetryDuration;

			if( RPC_FAILURE != result || retryExpired )
			{
				if( retryExpired )
				{
					gpnet( "\tLOCAL RETRY PACKET EXPIRED\n" );
				}

				// great, now we can get rid of this sucker
				ip = ppackets->erase(ip);
			}
			else
			{
				++ip;
			}
		}
	}

	return( true );

}


////////////////////////////////////////////////////////////////////////////////
//
FuBiCookie NetFuBiReceive :: ExecuteCurrentRpc( const DWORD fromMachineId )
{
	if( !NetPipe::DoesSingletonExist() )
	{
		return RPC_FAILURE;
	}
	NetPipeSessionHandle session = gNetPipe.GetOpenSessionHandle();
	if( !session )
	{
		return RPC_FAILURE;
	}

	if( m_InExecuteCurrentRpcScope )
	{
		gperror( "NetFuBiReceive :: ExecuteCurrentRpc - re-entry error\n" );
		return RPC_FAILURE;
	}

	m_InExecuteCurrentRpcScope = true;

	gpassert( !m_Buffer.empty() );

	////////////////////////////////////////
	//	process packet

	m_ReceivePoint = &*m_Buffer.begin();
	FuBiCookie result = gFuBiSysExports.DispatchNextRpc( fromMachineId, m_Buffer.size() );

	gpassert( (m_ReceivePoint == &*m_Buffer.end()) || (result != RPC_SUCCESS) );

	m_Buffer.clear();

	m_InExecuteCurrentRpcScope = false;

	return result;
}


////////////////////////////////////////////////////////////////////////////////
//

#if !GP_RETAIL


void NetFuBiReceive :: Dump( ReportSys::ContextRef ctx )
{
	ReportSys::AutoReport autoReport( ctx );
	ctx->Output( "\nNetFuBiReceive local retries:\n\n" );
	ctx->Output( "==============\n" );
	Dump( m_machineLRpackets, ctx );
}


void NetFuBiReceive :: Dump( AutoMachineToPacketsMap & mp, ReportSys::ContextRef ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	AutoMachineToPacketsMap::iterator imp;
	for( imp = mp.begin(); imp != mp.end(); ++imp )
	{
		for( AutoPacketsList::iterator ip = (*imp).second->begin(); ip != (*imp).second->end(); ++ip )
		{
			ctx->OutputF( "%d retries for %s (from machine=%u)\n",
				(*ip)->m_RetryCount,
				NetFuBiSend::DebugGetRpcInfo( (*imp).first, &*(*ip)->m_Buffer.begin(), (*ip)->m_Buffer.size(), true ).c_str(),
				(*imp).first );
		}
	}
}



void NetFuBiReceive :: AddReportFilters( const gpstring& filter )
{
	::AddFilters( m_ReportFilters, filter );
}


void NetFuBiReceive :: SetReportFilters( const gpstring& filter )
{
	m_ReportFilters.clear();
	::AddFilters( m_ReportFilters, filter );
}


#endif

