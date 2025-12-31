//
//
//	Flamethrower_context_tattoo.cpp
//	Copyright 1999, Gas Powered Games
//
//
//#pragma warning( disable : 4786 ) // debug symbol truncated to 255 characters
#include "ignored_warnings.h"

#include "Flamethrower_context.h"
#include "Flamethrower_tracker.h"
#include "Flamethrower_static_target.h"		// needed to fake the GO object targetting
#include "Flamethrower.h"

#include "Flamethrower_siege_context.h"

#include <list>
#include <map>


#include "vector_3.h"
#include "matrix_3x3.h"

#include "muffler.h"

/*
#include "chassis_object.h"
#include "chassis_framework.h"
#include "chassis_renderer.h"
*/
#include "siege_engine.h"
#include "siege_walker.h"
#include "camera\basic_camera.h"

//#include "terrain.h"
//#include "siege_tattoo.h"

#include "aitypes.h"		// for goguid typedef 
#include "go.h"
#include "godb.h"
#include "gobody.h"
#include "Tattoo_world.h"
#include "tattoo_game.h"
#include "tp_configuration_handle.h"


using namespace std;


/*
//
// Tattoo specific target stuff
//

MainAccessor	ma;

GOTarget::GOTarget( goguid GO, const std::string &strBodyPart )
	: mRenderer( ggTattoo_game.mDisplay.GetRenderer() )
	, mID( GO )
	, mstrBodyPart( strBodyPart )
{}



Flamethrower_target*
GOTarget::Duplicate()
{
	Flamethrower_target *pTarget = new GOTarget( mID, mstrBodyPart );

	return pTarget;
}



bool
GOTarget::GetPosition( vector_3 &position )
{
	GO	&Object = gWorld.GetGODB().Find( mID );

	if( Object.IsValid() )
	{
		GOBody	*pBody = Object.mBody;
		tp_configuration_handle *pHandle = pBody->GetConfigurationHandle();
		vector_3	soulPos;
		matrix_3x3	orientation;
		vector_3	scale;

		if( pHandle && ( pHandle->GetWorldSpacePiecePlacement( mRenderer, mstrBodyPart.c_str(),
																soulPos, orientation, scale )) )
		{
			position = soulPos;
			return true;
		}
		else
		{
			Chassis_object *pObject = pBody->GetChObj();
			position = pObject->GetPosition();
			return true;
		}
	}
	return false;
}



bool
GOTarget::GetDirection( vector_3 &direction )
{
	GO	&Object = gWorld.GetGODB().Find( mID );

	if( Object.IsValid() )
	{
		GOBody	*pBody = Object.mBody;
		tp_configuration_handle *pHandle = pBody->GetConfigurationHandle();
		vector_3	soulPos;
		matrix_3x3	orientation;
		vector_3	scale;

		if( pHandle && ( pHandle->GetWorldSpacePiecePlacement( mRenderer, mstrBodyPart.c_str(),
																soulPos, orientation, scale )) )
		{
			direction.SetX( orientation.GetElement(0,0) );
			direction.SetY( orientation.GetElement(1,1) );
			direction.SetZ( orientation.GetElement(2,2) );
			return true;
		}
		else
		{
			direction = pBody->GetDirection();
			return true;
		}
	}
	return false;
}



bool
GOTarget::GetBoundingRadius( float &radius )
{
	GO	&Object = gWorld.GetGODB().Find( mID );
	
	if( Object.IsValid() )
	{
		GOBody	*pBody = Object.mBody;
		Chassis_object *pObject = pBody->GetChObj();
		radius = pObject->GetBoundingRadius()*0.75f;
		return true;
	}
	return false;
}




*/

//
// Tattoo specific tracker stuff
//

TattooTracker::~TattooTracker()
{
	def_Target::iterator it;

	for( it = mTargets.begin(); it < mTargets.end(); it++ )
	{
		delete *it;
	}
}



Flamethrower_tracker*
TattooTracker::Duplicate()
{
	Flamethrower_tracker *pTracker = new TattooTracker;

	def_Target::iterator it = mTargets.begin();

	for( ; it < mTargets.end(); ++it )
	{
		Flamethrower_target	*pTarget = (*it)->Duplicate();

		pTracker->AddTarget( pTarget );
	}

	return pTracker;
}



bool
TattooTracker::AtLeastOneTarget()
{
	return !mTargets.empty();
}



int
TattooTracker::GetTargetCount()
{
	return mTargets.size();
}



vector< vector_3 >
TattooTracker::GetAllPositions()
{
	vector< vector_3 > targetCoords;

	for( int i = SUBJECT; i < mTargets.size(); ++i )
	{
		vector_3	temp;
		
		if( GetPosition( temp, (Flamethrower_tracker::TARGET_INDEX)i ) )
		{
			targetCoords.push_back( temp );
		}
	}
	return targetCoords;
}



bool
TattooTracker::GetPosition( vector_3 &pos, const TARGET_INDEX n )
{
	if( (n > mTargets.size())||(mTargets.empty()) )
		return false;

	if( mTargets[n]->GetPosition( pos ) )
		return true;

	def_Target::iterator target = mTargets.begin();
	target += n;
	delete *target;
	mTargets.erase( target );
	return false;
}



bool
TattooTracker::GetDirection( vector_3 &dir, const TARGET_INDEX n )
{
	if( (n > mTargets.size())||(mTargets.empty()) )
		return false;

	if( mTargets[n]->GetDirection( dir ) )
		return true;

	def_Target::iterator target = mTargets.begin();
	target += n;
	delete *target;
	mTargets.erase( target );
	return false;
}



bool
TattooTracker::GetBoundingRadius( float &r, const TARGET_INDEX n )
{
	if( (n > mTargets.size())||(mTargets.empty()) )
		return false;

	if( mTargets[n]->GetBoundingRadius( r ) )
		return true;

	def_Target::iterator target = mTargets.begin();
	target += n;
	delete *target;
	mTargets.erase( target );
	return false;
}



void
TattooTracker::RemoveAllTargets()
{
	def_Target::iterator it;

	for( it = mTargets.begin(); it < mTargets.end(); it++ )
	{
		delete *it;
	}

	mTargets.clear();
}



void
TattooTracker::AddTarget( Flamethrower_target *pTarget )
{
	mTargets.push_back( pTarget );
}



void
TattooTracker::SetTarget( Flamethrower_target *pTarget, const TARGET_INDEX n )
{
	def_Target::iterator it = mTargets.begin();

	// If the list is empty then add to the beginning
	if( (mTargets.size() == 0) )
	{
		AddTarget( pTarget );
		return;
	}

	// If we are trying to add a subject or object then delete
	// the old target and take it's place
	if( n < mTargets.size() )
	{
		it += n;

		delete *it;
		*it = pTarget;
	}
	else
	{
		AddTarget( pTarget );
	}
}



bool
TattooTracker::GetTarget( const TARGET_INDEX n, Flamethrower_target **pTarget )
{
	if( n < mTargets.size() )
	{
		def_Target::iterator it = mTargets.begin();

		it += n;
		*pTarget = *it;

		return true;
	}

	return false;
}


//
// Flamethrower context tattoo stuff
//
Flamethrower_context_tattoo::Flamethrower_context_tattoo(
										   siege::SiegeEngine& engine,
										   Muffler &sound,
										   TNK &tnk,
										   GAS &gas )
:	mEngine( engine )
//,	mFrustum( *mRenderer.GetViewFrustum() )
,	mSound( sound )
,	mTnk( tnk ) 
,	mGas( gas )
,	IFlamethrower( new Flamethrower( *this, tnk, gas ) )
,	mSSLC(0)
{
//	gpassert(mRenderer.GetViewFrustum());

}



Flamethrower_context_tattoo::~Flamethrower_context_tattoo()
{
	delete IFlamethrower;
}



void
Flamethrower_context_tattoo::AddScriptsFromGasFile( const std::string &strFilename )
{
	IFlamethrower->AddScriptsFromGasFile( strFilename );
}



bool
Flamethrower_context_tattoo::AddScript( const std::string &strScriptName,
									   const std::string &strScript )
{
	return IFlamethrower->AddScript( strScriptName, strScript );
}



bool
Flamethrower_context_tattoo::DestroyScript( const std::string &strScriptName )
{
	return IFlamethrower->DestroyScript( strScriptName );
}



INT64
Flamethrower_context_tattoo::RunScript( const std::string &strScriptName,
									   std::auto_ptr< Flamethrower_tracker> targets )
{
	return IFlamethrower->RunScript( strScriptName, targets );
}



void
Flamethrower_context_tattoo::StopScript( const INT64 id )
{
	IFlamethrower->StopScript( id );
}



void
Flamethrower_context_tattoo::StopAllScripts()
{
	IFlamethrower->StopAllScripts();
}



INT64
Flamethrower_context_tattoo::CreateEffect( const string &strEffect, 
											const string &strParameters,
											auto_ptr< Flamethrower_tracker > targets )
{
	return IFlamethrower->CreateEffect( strEffect, strParameters, targets );
}



void
Flamethrower_context_tattoo::SetTargets( const INT64 id, 
										auto_ptr< Flamethrower_tracker > targets )
{
	IFlamethrower->SetTargets( id, targets );	
}



void
Flamethrower_context_tattoo::DestroyEffect( const INT64 id )
{
	IFlamethrower->DestroyEffect( id );
}



void
Flamethrower_context_tattoo::DestroyAllEffects()
{
	IFlamethrower->DestroyAllEffects();
}



bool
Flamethrower_context_tattoo::GetDirection( const INT64 id, vector_3 &dir )
{
	return IFlamethrower->GetDirection( id, dir );
}



bool
Flamethrower_context_tattoo::SetDirection( const INT64 id, const vector_3 &dir )
{
	return IFlamethrower->SetDirection( id, dir );
}



bool
Flamethrower_context_tattoo::GetPosition( const INT64 id, vector_3 &pos )
{
	return IFlamethrower->GetPosition( id, pos );
}



bool
Flamethrower_context_tattoo::SetPosition( const INT64 id, const vector_3 &pos )
{
ma	return IFlamethrower->SetPosition( id, pos );
}



bool
Flamethrower_context_tattoo::GetBoundingRadius( const INT64 id, float &radius )
{
	return IFlamethrower->GetBoundingRadius( id, radius );
}



void
Flamethrower_context_tattoo::AddEffectTarget( const INT64 id, const INT64 target_id )
{
	IFlamethrower->AddEffectTarget( id, target_id );
}



void
Flamethrower_context_tattoo::RemoveAllTargets( const INT64 id )
{
	IFlamethrower->RemoveAllTargets( id );
}



bool
Flamethrower_context_tattoo::AttachEffect( const INT64 parent_id, const INT64 child_id )
{
	return IFlamethrower->AttachEffect( parent_id, child_id );
}



void
Flamethrower_context_tattoo::StartEffect( const INT64 id )
{
	IFlamethrower->StartEffect( id );
}



void
Flamethrower_context_tattoo::StopEffect( const INT64 id )
{
	IFlamethrower->StopEffect( id );
}



void
Flamethrower_context_tattoo::StopAllEffects()
{
	IFlamethrower->StopAllEffects();
}


void
Flamethrower_context_tattoo::StartAllEffects()
{
	IFlamethrower->StartAllEffects();
}


void
Flamethrower_context_tattoo::EnableEffect( const INT64 id, const bool enable )
{
	IFlamethrower->EnableEffect( id, enable );
}



void
Flamethrower_context_tattoo::Update( const double SecondsSinceLastCall )
{
	IFlamethrower->Update( SecondsSinceLastCall );

	mSSLC = SecondsSinceLastCall;
}



void
Flamethrower_context_tattoo::PostCollision( CollisionInfo *pCollision )
{
	IFlamethrower->PostCollision( pCollision );
}



bool
Flamethrower_context_tattoo::EffectHasCollided( const INT64 id, bool &bGroundCollision )
{
	return IFlamethrower->EffectHasCollided( id, bGroundCollision );
}


bool
Flamethrower_context_tattoo::GetCollisionPoint( const INT64 id, vector_3 &point )
{
	return IFlamethrower->GetCollisionPoint( id, point );
}



bool
Flamethrower_context_tattoo::GetCollisionNormal( const INT64 id, vector_3 &point )
{
	return IFlamethrower->GetCollisionNormal( id, point );
}

bool
Flamethrower_context_tattoo::GetCollisionDirection( const INT64 id, vector_3 &point )
{
	return IFlamethrower->GetCollisionDirection( id, point );
}


std::auto_ptr< Flamethrower_tracker >
Flamethrower_context_tattoo::MakeTracker()
{
	std::auto_ptr< Flamethrower_tracker > pTracker( new TattooTracker );

	return pTracker;
}



void
Flamethrower_context_tattoo::Render( vector_3 pivot )
{
	mgPivot = pivot;
	IFlamethrower->Update( mSSLC );
}



//
// Start of internal Flamethrower calls
//
void
Flamethrower_context_tattoo::RestoreRenderingState() const
{
	glPopAttrib();
}


void
Flamethrower_context_tattoo::SaveRenderingState() const
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	
	glDisable( GL_LIGHTING );
	glDepthMask( GL_FALSE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	glDisable( GL_CULL_FACE );				// This was for lightning
	glEnable( GL_TEXTURE_2D );

}


vector_3
Flamethrower_context_tattoo::GetGlobalPivot() const
{
	return mgPivot;
}



vector_3
Flamethrower_context_tattoo::GetCameraPosition() const
{

	vector_3 camPos = mEngine.NodeWalker().CurrentCamera().CameraPosition();

	return camPos;
}


#define UP_VECTOR_3 vector_3(0,1,0)

matrix_3x3
Flamethrower_context_tattoo::GetCameraOrientation() const
{

	vector_3 z_axis = mEngine.NodeWalker().CurrentCamera().TargetPosition()
						- mEngine.NodeWalker().CurrentCamera().CameraPosition();

	vector_3 x_axis = CrossProduct(UP_VECTOR_3,z_axis);

	vector_3 y_axis = CrossProduct(z_axis,x_axis);

	matrix_3x3 camOrient(MatrixColumns(Normalize(x_axis),Normalize(y_axis),Normalize(z_axis)));

	return camOrient;
}



GLcolor
Flamethrower_context_tattoo::GetLightDiffuse( int nLight ) const
{
	GLcolor color;
	
#ifdef LIGHTS_OK
	color.r = mRenderer.GetDiffuseLight( nLight, 0 );
	color.g = mRenderer.GetDiffuseLight( nLight, 1 );
	color.b = mRenderer.GetDiffuseLight( nLight, 2 );
	color.a = mRenderer.GetDiffuseLight( nLight, 3 );
#else 
	color.r = 1.0f;
	color.g = 1.0f;
	color.b = 1.0f;
	color.a = 1.0f;
#endif

	return color;
}



GLcolor
Flamethrower_context_tattoo::GetLightAmbient( int nLight ) const
{
	GLcolor color;

#ifdef LIGHTS_OK
	color.r = mRenderer.GetAmbientLight( nLight, 0 );
	color.g = mRenderer.GetAmbientLight( nLight, 1 );
	color.b = mRenderer.GetAmbientLight( nLight, 2 );
	color.a = mRenderer.GetAmbientLight( nLight, 3 );
#else 
	color.r = 1.0f;
	color.g = 1.0f;
	color.b = 1.0f;
	color.a = 1.0f;
#endif


	return color;
}



GLcolor
Flamethrower_context_tattoo::GetLightSpecular( int nLight ) const
{
	GLcolor color;

#ifdef LIGHTS_OK
	color.r = mRenderer.GetSpecularLight( nLight, 0 );
	color.g = mRenderer.GetSpecularLight( nLight, 1 );
	color.b = mRenderer.GetSpecularLight( nLight, 2 );
	color.a = mRenderer.GetSpecularLight( nLight, 3 );
#else 
	color.r = 1.0f;
	color.g = 1.0f;
	color.b = 1.0f;
	color.a = 1.0f;
#endif

	return color;
}



bool
Flamethrower_context_tattoo::GetLightEnable( int nLight ) const
{
	// no support
	return false;
}



vector_3
Flamethrower_context_tattoo::GetLightPosition( int nLight ) const
{

#ifdef LIGHTS_OK
	return mRenderer.GetLightPosition( nLight );
#else
	return vector_3(0,0,0);
#endif

}



matrix_3x3
Flamethrower_context_tattoo::GetLightOrientation( int nLight) const
{
	matrix_3x3	dir;

	// no support
	return dir;
}



GLcolor
Flamethrower_context_tattoo::GetBackgroundColor() const
{
	GLcolor	clearColor;


#ifdef LIGHTS_OK
	clearColor.r = mRenderer.GetBackgroundColor( nLight, 0 );
	clearColor.g = mRenderer.GetBackgroundColor( nLight, 1 );
	clearColor.b = mRenderer.GetBackgroundColor( nLight, 2 );
	clearColor.a = mRenderer.GetBackgroundColor( nLight, 3 );
#else 
	clearColor.r = 1.0f;
	clearColor.g = 1.0f;
	clearColor.b = 1.0f;
	clearColor.a = 1.0f;
#endif

	return	clearColor;
}



vector_3
Flamethrower_context_tattoo::GetGroundHeight(const vector_3& position ) const
{
#ifdef TERRAIN_OK
	return LocatePointOnTerrain( position );
#else
	vector_3 new_pos(position);
	new_pos(2) = 0.0f;
	return new_pos;
#endif
}


vector_3
Flamethrower_context_tattoo::GetGroundNormal(const vector_3& position ) const
{
#ifdef TERRAIN_OK
	return CalculateTerrainNormal( position );
#else
	vector_3 norm(0,1,0);
	return norm;
#endif
}


bool
Flamethrower_context_tattoo::CheckForCollision( vector_3 &contactPoint, vector_3 &contactNormal,
											   const vector_3 &startPos,const vector_3 &endPos,
											   const float width, const float height )
{
#ifdef TERRAIN_OK
	return ::CheckForCollision( contactPoint, contactNormal, startPos, endPos, width, height );
#else
	return false;
#endif
}



void
Flamethrower_context_tattoo::SetLightEnable( int nLight, bool state )
{

#ifdef LIGHTS_OK
	mRenderer.EnableLight( nLight, state );
#endif

}



void
Flamethrower_context_tattoo::SetCameraPosition( vector_3 &pos )
{

//	mEngine.NodeWalker().CurrentCamera().TargetPosition(pos);
	return;
}



void
Flamethrower_context_tattoo::SetCameraOrientation( matrix_3x3 &dir )
{

//	mEngine.NodeWalker().CurrentCamera().CameraOrientation(dir);

	return;
}



void
Flamethrower_context_tattoo::SetLightPosition( int nLight, vector_3 &pos )
{
//	mRenderer.SetLightPosition( nLight, pos.GetX(), pos.GetY(), pos.GetZ(), 1.0f );
}



void
Flamethrower_context_tattoo::SetLightOrientation( int nLight, matrix_3x3 &dir )
{
	// No suppport
	return;
}



void
Flamethrower_context_tattoo::SetLightDiffuse( int nLight, GLcolor color )
{
//	mRenderer.SetDiffuseLight( nLight, color.r, color.g, color.b, color.a );
}



void
Flamethrower_context_tattoo::SetLightAmbient( int nLight, GLcolor color )
{
//	mRenderer.SetAmbientLight( nLight, color.r, color.g, color.b, color.a );
}



void
Flamethrower_context_tattoo::SetLightSpecular( int nLight, GLcolor color )
{
//	mRenderer.SetSpecularLight( nLight, color.r, color.g, color.b, color.a );
}



void
Flamethrower_context_tattoo::SetBackgroundColor( GLcolor &color )
{
//	mRenderer.SetClearColor( color.r, color.g, color.b, color.a );
}



std::auto_ptr< Flamethrower_tracker >
Flamethrower_context_tattoo::GetTargets( const std::string &filter )
{
	std::auto_ptr< Flamethrower_tracker > pTracker( new TattooTracker );

#ifdef GO_TARGETS_OK
	if( filter == "all" )
	{
		GODBiterator goit, goend;

		goend = gWorld.GetGODB().End();
		
		for( goit = gWorld.GetGODB().Begin(); goit != goend; ++goit )
		{
			if( (*goit)->IsSelected() )
			{
				GOTarget *pTarget = new GOTarget( (*goit)->GetGUID() );
				pTracker->AddTarget( pTarget );
			}
		}
	}
#else

	Flamethrower_static_target targ( vector_3(0,0,0), vector_3(0,0,0), 1.0f );
	
	pTracker->AddTarget( &targ );

#endif

	return pTracker;
}

#ifdef PlaySound
#undef PlaySound
#endif

INT64
Flamethrower_context_tattoo::PlaySound( const std::string &strName, bool loop )
{
	return mSound.PlaySample( strName, loop );
}



void
Flamethrower_context_tattoo::StopSound( const INT64 id )
{
	mSound.StopSample( id );
}



