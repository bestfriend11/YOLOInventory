/*************************************************************************************
**
**									RapiLight
**
**		This is a test class for the purposes of planning out the different elements
**		and implementation methods of a high quality 3D abstraction layer.
**
**		Date:	06/7/99
**		Author: James Loe
**
*************************************************************************************/

#include "precomp_rapi.h"
#include "gpcore.h"

#if !defined(_RAPI_OWNER_)
#include "RapiOwner.h"
#endif


// Construction
RapiLight::RapiLight()
: m_bEnabled( false )
{
	// Setup default params
	ZeroMemory( &m_lightparams, sizeof( D3DLIGHT8 ) );
	D3DCOLORVALUE color = { 1.0f, 1.0f, 1.0f, 1.0f };
	SetLight( D3DLIGHT_POINT, color, vector_3::ZERO, vector_3::ZERO, 0, 1.0f, 0.0f, 0.0f, 0.52f, 0.79f );
}

// Destruction
RapiLight::~RapiLight()
{
}

// Set the properties of the light
void RapiLight::SetLight(	LTYPE			lType,		// Type of light
							LCOLOR&			lColor,		// Color of light
							const vector_3&	vPos,		// World position of light
							const vector_3&	vDir,		// Direction the light is pointing
							float			fRange,		// Range beyond which the light has no effect
							float			fLinearA,	// Linear attenuation value
							float			fConstA,	// Constant attenuation value
							float			fQuadA,		// Quadratic attenuation value
							float			fInnerAngle,// Inner cone angle (only used for spotlights)
							float			fOuterAngle	// Outer cone angle (only used for spotlights)
						)
{
	UNREFERENCED_PARAMETER( lType );
	UNREFERENCED_PARAMETER( lColor );
	UNREFERENCED_PARAMETER( vPos );
	UNREFERENCED_PARAMETER( vDir );
	UNREFERENCED_PARAMETER( fRange );
	UNREFERENCED_PARAMETER( fLinearA );
	UNREFERENCED_PARAMETER( fConstA );
	UNREFERENCED_PARAMETER( fQuadA );
	UNREFERENCED_PARAMETER( fInnerAngle );
	UNREFERENCED_PARAMETER( fOuterAngle );
}
