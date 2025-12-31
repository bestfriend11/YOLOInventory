#pragma once
// Define some events

DECLARE_EVENT
{

FUBI_EXPORT void OnResetChore( Skrit::HObject skrit);
FUBI_EXPORT void OnUpdate( Skrit::HObject skrit, float delta_t );

// Animation with 'chores'
FUBI_EXPORT void OnStartChore( Skrit::HObject skrit, int subanim, int flags );

// Passing messages to animation skrits.
FUBI_EXPORT void OnHandleMessage( Skrit::HObject skrit, eWorldEvent event, const WorldMessage& msg );

}

FUBI_EXPORT float ConvertRatioToAttackBlend( const float x, const float y,const float deg0,const float deg1);
FUBI_EXPORT float ConvertRatioToWalkBlend ( const float x, const float z);

FUBI_EXPORT float SinWave(
			  const float a, const float b, 
			  const float c, const float d, 
			  const float t, 
			  const float frequency, const float amplitude);
