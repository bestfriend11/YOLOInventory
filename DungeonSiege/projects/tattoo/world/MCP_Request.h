#pragma once
/*==================================================================================================


	MCP_Request -- MCP request interface

	purpose:	API that allows AI to change the plans stored within the MCP

	author:	biddle & Bart

  (c)opyright Gas Powered Games, 2000

--------------------------------------------------------------------------------------------------*/
#ifndef __MCP_REQUEST_H
#define __MCP_REQUEST_H

#include "mcp_defs.h"

namespace MCP {

////////////////////////////////////////
//	enum eRequest
enum eRequest
{
	SET_BEGIN_ENUM( PL_, 0 ),

	PL_NONE,

	PL_APPROACH,				// Get to within a specified distance (was WALK)
	PL_FOLLOW,					// Maintain your position equal to the target's
	PL_INTERCEPT,				// Meet up with another object
	PL_AVOID,					// Stay beyond a specified distance

	PL_FACE,					// Rotate to a specific direction

	PL_DIE,

	PL_ATTACK_OBJECT_MELEE,
	PL_ATTACK_OBJECT_RANGED,
	PL_ATTACK_POSITION_MELEE,
	PL_ATTACK_POSITION_RANGED,

	PL_CAST,
	PL_CAST_ON_OBJECT,
	PL_CAST_ON_POSITION,

	PL_FIDGET,

	PL_OPEN,
	PL_CLOSE,
	
	PL_WAIT_FOR_CONTACT,		// Plan used by triggers so the MCP can 'see' them
								// Contact occurs when a trigger is within specified 
								// range

	PL_PLAYANIM,				// Force a particular animation to be played

	SET_END_ENUM( PL_ )

};


/*===========================================================================
	Enum		: eReqRetCode RequestReturnCode (not in MCP namespace)
---------------------------------------------------------------------------*/

enum eReqRetCode
{
	SET_BEGIN_ENUM( REQRETCODE_, 0 ),

	// --- Successful codes

	REQUEST_OK,
	REQUEST_OK_IN_RANGE,

	// --- Semi successful
	REQUEST_OK_INSIDE_RANGE,
	REQUEST_OK_BEYOND_RANGE,

	REQUEST_OK_CROWDED,

	// --- Failure codes

	REQUEST_FAILED,
	REQUEST_FAILED_BAD_TYPE,
	REQUEST_FAILED_BAD_OWNER,
	REQUEST_FAILED_BAD_TARGET,
	REQUEST_FAILED_MISSING_FOLLOWER,
	REQUEST_FAILED_NO_PATH,
	REQUEST_FAILED_BAD_PATH_BEGIN,
	REQUEST_FAILED_BAD_PATH_END,
	REQUEST_FAILED_ZERO_VELOCITY,
	REQUEST_FAILED_OVERCROWDED,
	REQUEST_FAILED_PATH_LEAVES_WORLD,

	SET_END_ENUM( REQRETCODE_ )
};


}; //namespace MCP

FUBI_EXPORT const char * ToString( MCP::eRequest x );
FUBI_EXPORT bool FromString( const char* str, MCP::eRequest & x );

FUBI_EXPORT const char * ToString( MCP::eReqRetCode x );
FUBI_EXPORT bool FromString( const char* str, MCP::eReqRetCode & x );

FUBI_EXPORT const char * ToString( MCP::eOrientMode x );
FUBI_EXPORT bool FromString( const char* str, MCP::eOrientMode & x );

FUBI_EXPORT const char * ToString( MCP::eRID x );
FUBI_EXPORT bool FromString( const char* str, MCP::eRID & x );

FUBI_EXPORT bool RequestOk( MCP::eReqRetCode code );
FUBI_EXPORT bool RequestFailed( MCP::eReqRetCode code );
FUBI_EXPORT bool RequestBeyondRange( MCP::eReqRetCode code );

            bool DecodeAttackParameters( const DWORD flags, float& loopdur, float& elevation );
FUBI_EXPORT float DecodeAttackLoopDuration( const DWORD flags );
FUBI_EXPORT float DecodeAttackElevation( const DWORD flags );
FUBI_EXPORT float DecodeAttackDeviation( const DWORD flags );

FUBI_EXPORT bool EncodeAttackParameters( float loopdur, float elevation, float deviation, DWORD& flags);

// Stuff the elapse chore offset time (normalized to 0..1) into the chore flags
float DecodeChoreElapsedTime( DWORD flags );
DWORD DecodeChoreFlags( DWORD flags );

bool EncodeChoreParameters( float elapsed, DWORD& flags);

#endif  // __MCP_REQUEST_H
