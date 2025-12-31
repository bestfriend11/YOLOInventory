//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoGizmo.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the gizmo component for Go's.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOGIZMO_H
#define __GOGIZMO_H
#if !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////

#include "Nema_Types.h"
#include "Go.h"

//////////////////////////////////////////////////////////////////////////////
// class GoGizmo declaration

class GoGizmo : public GoComponent
{
public:
	SET_INHERITED( GoGizmo, GoComponent );

// Setup.

	// ctor/dtor
	GoGizmo( Go* parent );
	GoGizmo( const GoGizmo& source, Go* parent );
	virtual ~GoGizmo( void );

// Interface.

	// rendering
	nema::HAspect        GetAspectHandle  ( void ) const;
	siege::eCollectFlags BuildCollectFlags( void ) const;
	bool                 BuildRenderInfo  ( siege::ASPECTINFO& info ) const;

	// visibility
	bool GetIsVisible( void ) const					{  return ( m_IsVisible );  }
	void SetIsVisible( bool set = true );

// Overrides.

	// required overrides
	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newParent );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );
	virtual void		 HandleMessage( const WorldMessage& msg );

private:
	virtual bool CommitCreation( void );

	void UpdateNemaProperties( void );

	nema::HAspect m_Aspect;							// nema aspect (representation)
	bool          m_IsVisible;						// am i visible?
	float         m_Scale;							// render scale
	float         m_Alpha;							// global aspect alpha level
	vector_3      m_DiffuseColor;					// diffuse color to render with
	bool          m_UseDiffuseColor;				// whether or not to use the diffuse color

	SET_NO_COPYING( GoGizmo );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // !GP_RETAIL
#endif  // __GOGIZMO_H

//////////////////////////////////////////////////////////////////////////////
