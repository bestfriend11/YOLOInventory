#include "precomp_nema.h"

#include "gpmath.h"
#include "filter_1.h"
#include "worldtime.h"

// Implementation of Animation events

DECLARE_EVENT
{

void OnResetChore( Skrit::HObject skrit)
	{  CHECK_PRIMARY_THREAD_ONLY;  SKRIT_EVENTV( OnResetChore, skrit );  }

void OnUpdate( Skrit::HObject skrit, float delta_t ) {
	CHECK_PRIMARY_THREAD_ONLY;
	UNREFERENCED_PARAMETER(delta_t);
	SKRIT_EVENTV( OnUpdate, skrit );
}

void OnHandleMessage( Skrit::HObject skrit, eWorldEvent event, const WorldMessage& /*msg*/ ) {  
	CHECK_PRIMARY_THREAD_ONLY;
	UNREFERENCED_PARAMETER(event);
	SKRIT_EVENTV( OnHandleMessage, skrit );  
}

void OnStartChore( Skrit::HObject skrit, int subanim, int flags ) {
	CHECK_PRIMARY_THREAD_ONLY;
	UNREFERENCED_PARAMETER(subanim);
	UNREFERENCED_PARAMETER(flags);
	SKRIT_EVENTV( OnStartChore, skrit );  
}

}

float ConvertRatioToAttackBlend( const float x, const float y,const float deg0,const float deg1) {

	float ang = atan2f(x,y);

	float rad0,rad1;

	if (deg0 < deg1) {
		rad0 = DegreesToRadians(deg0);
		rad1 = DegreesToRadians(deg1);
	} else {
		rad0 = DegreesToRadians(deg1);
		rad1 = DegreesToRadians(deg0);
	}


	if (ang < rad0) return 0.0;
	if (ang > rad1) return 1.0;

	return (ang-rad0)/(rad1-rad0);

}

float ConvertRatioToWalkBlend ( const float x, const float z) {

	// Return a number in the range 0,4 (0 & 4 both map to 180)
	//
	//           z=-1
	//			 0,5 Backwards
	//            ^ 
	//            |
	//            |
	// Right Turn | Left Turn    
	// x=-1 1<----+---->4 x=1
	//	          |    
	//            |
	//            |
	//            v Forward
	//           2,3
	//           z=1

	if (IsZero(x)) {
		if (z<0.0f) {
			// We need to turn exactly 180, pick either direction
			if (Random(1) == 0) {
				return 0.0f;
			} else {
				return 4.0f;
			}
		} else {
			// We need to walk straight ahead, lead with either foot
			if (Random(1) == 0) {
				return 2.0f;
			} else {
				return 3.0f;
			}
		}
	}
	float theta = (ATAN2F(x,z)+PI)*2/PI;
	if (x<0.0f) {
		return theta;
	}
	return theta+1.0f;

}

float SinWave(const float a, const float b, 
			  const float c, const float d, 
			  const float t, 
			  const float frequency, const float amplitude) {
	float sw = FilterSmoothPulse<float>(a,b,c,d,t) * SINF((float)gWorldTime.GetTime()*frequency) * amplitude;
	return sw;
}

