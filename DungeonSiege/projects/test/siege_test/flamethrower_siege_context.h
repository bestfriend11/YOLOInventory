//
//	Flamethrower_context_tattoo.h
//	Copyright 1999, Gas Powered Games
//
//	
//

#ifndef _FLAMETHROWER_CONTEXT_TATTOO_H_
#define	_FLAMETHROWER_CONTEXT_TATTOO_H_

//#pragma warning( disable : 4786 ) // debug symbol truncated to 255 characters
class	TNK;
class	GAS;
class	Muffler;

/*
class	Chassis_services;
class	Chassis_renderer;
class	Chassis_object;
*/

namespace siege {
	class SiegeEngine;
}

class	Flamethrower_target;
class	Flamethrower_context;	// Main Client interface for Flamethrower
class	Flamethrower_tracker;	// Helper class for tracking Tattoo GOs
class	Flamethrower;

#include <string>
#include <vector>

#include "vector_3.h"
#include "chassis_object.h"
#include "GLcolor.h"

#ifdef RICK_HAS_FIXED_THIS
	#include "aitypes.h"		// for goguid typedef 
#else
	typedef UINT32 goguid;
#endif

#include "chassis_renderer.h"

#include "Flamethrower.h"
#include "Flamethrower_context.h"
#include "Flamethrower_tracker.h"



//
// Tattoo specific class for getting positional information out of the GODB
//

class GOTarget : public Flamethrower_target
{
	Chassis_renderer	&mRenderer;
	goguid				mID;
	std::string			mstrBodyPart;

public:
	GOTarget( goguid GO, const std::string &strBodyPart = "Crotch" );

	Flamethrower_target	*Duplicate();

	bool		GetPosition( vector_3 &position );
	bool		GetDirection( vector_3 &direction );
	bool		GetBoundingRadius( float &radius );
};




class TattooTracker : public Flamethrower_tracker
{
	typedef std::vector< Flamethrower_target* >	def_Target;

	def_Target		mTargets;

public:
	~TattooTracker();

	Flamethrower_tracker	*Duplicate();

	// Implementation
	//
	bool		AtLeastOneTarget();
	// Get total number of targets
	int			GetTargetCount();

	// Get list of all target coordinates
	std::vector< vector_3 >
				GetAllPositions();


	bool GetPosition( vector_3 &pos, const TARGET_INDEX n = SUBJECT );
	bool GetDirection( vector_3 &dir, const TARGET_INDEX n = SUBJECT );
	bool GetBoundingRadius( float &r, const TARGET_INDEX n = SUBJECT );

	// Remove all targets
	void		RemoveAllTargets();
	// Add a default target
	void		AddTarget( Flamethrower_target *pTarget );
	// Set a target - Calls AddTarget if n > number of targets
	void		SetTarget( Flamethrower_target *pTarget, const TARGET_INDEX n );
	// Get a target
	bool		GetTarget( const TARGET_INDEX n,	Flamethrower_target **pTarget );
};




//
// Flamethrower interface for Tattoo - all requests from Tattoo go through here
//

class Flamethrower_context_tattoo : public Flamethrower_context
{
	
	siege::SiegeEngine		&mEngine;
//	Chassis_renderer		&mRenderer;
//	Chassis_frustum			&mFrustum;
	Muffler					&mSound;
	TNK						&mTnk;
	GAS						&mGas;

	vector_3				mgPivot;
	double					mSSLC;

public:


	// Existence
	Flamethrower_context_tattoo( siege::SiegeEngine& engine, Muffler &sound, TNK &tnk, GAS& gas );
	~Flamethrower_context_tattoo();


	// Implementation
	// 
	// External (calls made from Tattoo)
	//

	// Scripted effect control
	void		AddScriptsFromGasFile( const std::string &strFilename );	// GAS functionality dependency

	bool		AddScript( const std::string &strScriptName, const std::string &strScript );
	bool		DestroyScript( const std::string &strScriptName );

	INT64		RunScript( const std::string &strScriptName,
							std::auto_ptr< Flamethrower_tracker> targets );

	void		StopScript( const INT64 id );

	void		StopAllScripts();


	// Singular effect control
	INT64		CreateEffect( const std::string &strEffect, 
								const std::string &strParameters,
							std::auto_ptr< Flamethrower_tracker > targets );

	void		DestroyEffect( const INT64 id );
	void		DestroyAllEffects();

	bool		GetDirection( const INT64 id, vector_3 &dir );
	bool		SetDirection( const INT64 id, const vector_3 &dir );

	bool		GetPosition( const INT64 id, vector_3 &pos );
	bool		SetPosition( const INT64 id, const vector_3 &pos );

	bool		GetBoundingRadius( const INT64 id, float &radius );

	void		AddEffectTarget( const INT64 id, const INT64 target_id );
	void		RemoveAllTargets( const INT64 id );
	bool		AttachEffect( const INT64 parent_id, const INT64 child_id );

	void		StartEffect( const INT64 id );
	void		StartAllEffects();

	void		StopEffect( const INT64 id );
	void		StopAllEffects();

	void		SetTargets( const INT64 id, std::auto_ptr< Flamethrower_tracker > targets );
	void		EnableEffect( const INT64 id, const bool enabled );

	void		Update( const double SecondsSinceLastCall );
	void		Render( vector_3 pivot );


	// Accessors
	bool		IsReady() const					{ return IFlamethrower->IsReady(); }
	std::string	GetActiveIDs() const			{ return IFlamethrower->GetActiveIDs(); }

	std::string	GetAvailableEffects() const		{ return IFlamethrower->GetAvailableEffects(); }
	std::string GetAvailableScripts() const		{ return IFlamethrower->GetAvailableScripts(); }

	TNK*		GetTNK() const					{ return &mTnk; }

	void		PostCollision( CollisionInfo *pCollision );
	
	bool		EffectHasCollided( const INT64 id, bool &bGroundCollision );
	bool		GetCollisionPoint( const INT64 id, vector_3 &point );
	bool		GetCollisionNormal( const INT64 id, vector_3 &point );
	bool		GetCollisionDirection( const INT64 id, vector_3 &point );

	std::auto_ptr< Flamethrower_tracker >
				MakeTracker();


	//
	// Internal	( Calls made from Flamethrower )
	//

	void		SaveRenderingState() const;
	void		RestoreRenderingState() const;

	vector_3	GetGlobalPivot() const;

	vector_3	GetCameraPosition() const;
	matrix_3x3	GetCameraOrientation() const;

	bool		GetLightEnable( int nLight ) const;
	vector_3	GetLightPosition( int nLight ) const;
	matrix_3x3	GetLightOrientation( int nLight ) const;
	GLcolor		GetLightDiffuse( int nLight ) const;
	GLcolor		GetLightAmbient( int nLight ) const;
	GLcolor		GetLightSpecular( int nLight ) const;

	GLcolor		GetBackgroundColor() const;

	vector_3	GetGroundHeight(const vector_3& position ) const;
	vector_3	GetGroundNormal(const vector_3& position ) const;

	bool		CheckForCollision( vector_3 &contactPoint, vector_3 &contactNormal,
											   const vector_3 &startPos,const vector_3 &endPos,
											   const float width, const float height );

	void		SetLightEnable( int nLight, bool state );

	void		SetCameraPosition( vector_3 &pos );
	void		SetCameraOrientation( matrix_3x3 &dir );

	void		SetLightPosition( int nLight, vector_3 &pos );
	void		SetLightOrientation( int nLight, matrix_3x3 &dir);

	void		SetLightDiffuse( int nLight, GLcolor color );
	void		SetLightAmbient( int nLight, GLcolor color );
	void		SetLightSpecular( int nLight, GLcolor color );


	void		SetBackgroundColor( GLcolor &color );

	std::auto_ptr< Flamethrower_tracker >
				GetTargets( const std::string &filter );

	INT64		PlaySound( const std::string &strName, bool loop = false );
	void		StopSound( const INT64 id );

protected:

	Flamethrower	*IFlamethrower;
};



#endif	//	_FLAMETHROWER_CONTEXT_TATTOO_H_