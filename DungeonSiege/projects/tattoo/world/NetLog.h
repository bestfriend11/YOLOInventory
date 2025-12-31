//////////////////////////////////////////////////////////////////////////////
//
// File     :  NetLog.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains code to do automatic logging and decoding of net
//             traffic for debugging purposes.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __NETLOG_H
#define __NETLOG_H

//////////////////////////////////////////////////////////////////////////////

#include "GoDefs.h"

//////////////////////////////////////////////////////////////////////////////

enum ePacketHeaderType;
class AutoNetWriter;
struct PacketInfo;
struct CreateGoXfer;
struct CloneGoXfer;

namespace FileSys
{
	class File;
}

namespace zlib
{
	class DeflateStreamer;
}

namespace FuBi
{
	class SerialBinaryWriter;
}

namespace kerneltool
{
	class Critical;
}

//////////////////////////////////////////////////////////////////////////////
// class NetLog declaration

class NetLog : public Singleton <NetLog>
{
public:
	SET_NO_INHERITED( NetLog );

	enum
	{
		HEADER_VERSION   = MAKEVERSION( 1, 0, 9 ),
		NETLOG_FOURCC_ID = 'NLog',
	};

// Setup.

	// ctor/dtor
	NetLog( void );
   ~NetLog( void );

	// config
	bool StartLogging( bool appendExisting );
	void StopLogging ( void );
	bool IsLogging   ( void ) const;

// API.

	void OutputBinary           ( FOURCC type, const_mem_ptr mem );
	void OutputReportString     ( const char* streamName, const char* eventName, const char* text, int len );
	void OutputDirectPlayMessage( DWORD messageId );
	void OutputGoCreatePacker   ( const CreateGoXfer& xfer, bool send );
	void OutputGoClonePacker    ( const CloneGoXfer& xfer, bool send );
	void OutputGoCreation       ( Go* go );
	void OutputGoDeletionMarked ( Go* go, bool retire, bool fadeOut, bool mpdelayed );
	void OutputGoDeletion       ( Go* go );
	void OutputGoDeletionDenied ( Goid goid );
	void OutputGoInfo           ( const char* reason, Go* go );
	void OutputRpcMcpIn         ( int followerCount, int packetCount, const_mem_ptr blob );
	void OutputRpcMcpOut        ( DWORD machineId, int followerCount, int packetCount, const_mem_ptr blob );
	void OutputRpcInStore       ( const PacketInfo& packet );
	void OutputRpcInDispatch    ( const PacketInfo& packet );
	void OutputRpcInResult      ( FuBiCookie result );
	void OutputRpcOut           ( const PacketInfo& packet );
	void OutputRpcOutDeferred   ( const FuBi::RpcVerifySpec& spec, Goid routerGoid, const GoidColl& pendingGoids );
	void OutputRaw              ( const_mem_ptr mem, bool compress = true );

// Extraction.

	static bool Extract( const char* fileName );

private:

	typedef stdx::fast_vector <BYTE> Buffer;
	friend AutoNetWriter;

	// objects
	my kerneltool::Critical*  m_Critical;				// serializer
	my ReportSys::Sink*       m_Sink;					// global sink hook for catching other streams
	my FileSys::File*         m_OutFile;				// output goes here
	my zlib::DeflateStreamer* m_DeflateStreamer;		// chunk compressor

	// buffers
	Buffer m_WriteBuffer;								// all writes go here in chunks
	Buffer m_CompressBuffer;							// all compression goes here on its way out

	// temp
	int               m_DispatchTempSize;				// temporary storage for dispatch size
	gpstring          m_DispatchTempText;				// temporary storage for dispatch text
	ePacketHeaderType m_DispatchTempType;				// temporary storage for dispatch type

	SET_NO_COPYING( NetLog );
};

#define gNetLog NetLog::GetSingleton()
#define gplog( CALL )  {  if ( NetLog::DoesSingletonExist() )  {  gNetLog.CALL;  }  }

//////////////////////////////////////////////////////////////////////////////

#endif  // __NETLOG_H

//////////////////////////////////////////////////////////////////////////////
