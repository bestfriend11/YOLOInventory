#pragma once
#ifndef _RAPI_LIGHT_
#define _RAPI_LIGHT_
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



class RapiLight
{

public:

	// Construction
	RapiLight();

	// Destruction
	~RapiLight();

	// Set the properties of the light
	void SetLight(	LTYPE			lType,		// Type of light
					LCOLOR&			lColor,		// Color of light
					const vector_3&	vPos,		// World position of light
					const vector_3&	vDir,		// Direction the light is pointing
					float			fRange,		// Range beyond which the light has no effect
					float			fLinearA,	// Linear attenuation value
					float			fConstA,	// Constant attenuation value
					float			fQuadA,		// Quadratic attenuation value
					float			fInnerAngle,// Inner cone angle (only used for spotlights)
					float			fOuterAngle // Outer cone angle (only used for spotlights)
				);

	// Get light parameters
	D3DLIGHT8&	GetLight()					{ return m_lightparams; }

	// Get current enabled state
	bool		IsEnabled()					{ return m_bEnabled; }

	// Set the current enabled state
	void		SetEnabled( bool bEnable )	{ m_bEnabled = bEnable; }

private:

	// Light parameters
	D3DLIGHT8			m_lightparams;

	// Is this light currently enabled?
	bool				m_bEnabled;
};

#endif
