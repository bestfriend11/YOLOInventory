/*

	WorldFx.cpp
	Copyright 1999, Gas Powered Games

*/

#include "Precomp_World.h"

#include "Aiquery.h"
#include "AppModule.h"
#include "BlindCallback.h"
#include "BoneTranslator.h"
#include "ContentDb.h"
#include "WorldFx.h"
#include "Flamethrower.h"
#include "Flamethrower_effect_base.h"
#include "Flamethrower_FuBi_Support.h"
#include "Flamethrower_Interpreter.h"
#include "Flamethrower_Tracker.h"
#include "FuBiTraitsImpl.h"
#include "FuBiPersist.h"
#include "FuBiBitPacker.h"
#include "Gps_manager.h"
#include "Matrix_3x3.h"
#include "Nema_aspect.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "GoCore.h"
#include "GoMagic.h"
#include "GoMind.h"
#include "GoDb.h"
#include "GoPhysics.h"
#include "RapiOwner.h"
#include "RapiPrimitive.h"
#include "Rules.h"
#include "Server.h"
#include "Siege_Camera.h"
#include "Siege_Database_Guid.h"
#include "Siege_Engine.h"
#include "Siege_ViewFrustum.h"
#include "Siege_Light_Structs.h"
#include "Siege_decal.h"
#include "Siege_Walker.h"
#include "siege_options.h"
#include "Sim.h"
#include "SimObj.h"
#include "Stringtool.h"
#include "Vector_3.h"
#include "Weather.h"
#include "World.h"
#include "WorldMessage.h"
#include "WorldSound.h"
#include "gosupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"
#include "oriented_bounding_box_3.h"

#include <map>

using namespace siege;

FUBI_REPLACE_NAME( "WorldFx", SiegeFx );

//
// --- GoTargetBase		(Flamethrower_target)
//
bool
GoTargetBase::IsValid( bool* shouldDelete )
{
	GoHandle hTarget( MakeGoid( m_ID ) );
	if ( !hTarget )
	{
		if ( shouldDelete != NULL )
		{
			*shouldDelete = true;
		}
		return ( false );
	}
	else if ( shouldDelete != NULL )
	{
		*shouldDelete = false;
	}

	return ( hTarget->IsPendingEntry() || hTarget->IsInAnyWorldFrustum() || hTarget->IsOmni() );
}


bool
GoTargetBase::GetInterestOnly( void )
{
	const Flamethrower_target::eTargetType ttype = GetType();

	if( (ttype == Flamethrower_target::DEFAULT) ||
		(ttype == Flamethrower_target::STATIC)  ||
		(ttype == Flamethrower_target::EMITTER) )
	{
		GoHandle hTarget( MakeGoid( m_ID ) );

		if( hTarget && hTarget->HasAspect() )
		{
			return hTarget->GetAspect()->GetInterestOnly();
		}
	}
	return false;
}


bool
GoTargetBase::GetIsInsideInventory( void )
{
	GoHandle hTarget( MakeGoid( m_ID ) );

	return ( hTarget && hTarget->IsInsideInventory() && hTarget->HasAspect() && !hTarget->GetAspect()->GetIsVisible() );
}



//
// --- GoTarget		(GoTargetBase)
//
GoTarget::GoTarget( Goid GO, const char* boneName )
	: GoTargetBase( Flamethrower_target::DEFAULT )
	, m_nema_bone_index( 0 )
{
	gpassert( (GetGoidClass( GO ) == GO_CLASS_GLOBAL) || (GetGoidClass( GO ) == GO_CLASS_LOCAL) );

	// Id for a Flamethrower target actually uses IDs for more than just Goids so internally it is stored as a UINT32
	SetID( MakeInt( GO ) );

	// Validate the requested game object
    GoHandle hTarget( MakeGoid( m_ID ) );

	if( hTarget )
	{
		if( !hTarget->HasPlacement() )
		{
			gperrorf(( "%s does not have a placement component!\n", hTarget->GetTemplateName() ));
			return;
		}

		// Default to kill bone
		if ( boneName == NULL )
		{
			boneName = "@kill_bone";
		}

		// If a bone name is preceded with an @ it means to translate it otherwise treat it as a standard Nema name
		if( boneName[ 0 ] == '@' )
		{
			BoneTranslator::eBone bone_index = BoneTranslator::KILL_BONE;
			if( !BoneTranslator::FromString( boneName + 1, bone_index ) )
			{
				gperrorf(("Attempted to reference bone %s from %s",boneName, hTarget->GetTemplateName() ));
			}
		}
		else if( (*boneName != '\0') && !hTarget->GetAspect()->GetAspectPtr()->GetBoneIndex( boneName, m_nema_bone_index ) )
		{
			gperrorf(("%s does not have a bone named %s",hTarget->GetTemplateName(), boneName ));
		}
	}
	else
	{
		gpassertm( 0, "Invalid Goid passed to flamethrower target!");
	}
}


GoTarget::GoTarget( Goid GO, int boneIndex )
{
	SetID( MakeInt( GO ) );
	m_nema_bone_index = boneIndex;
}


bool
GoTarget::OnXfer( FuBi::PersistContext &persist )
{
	persist.Xfer( "m_nema_bone_index",	m_nema_bone_index	);

	return true;
}


bool
GoTarget::ShouldPersist( bool& hasGoid ) const
{
	if ( GetID() == 0 )
	{
		hasGoid = false;
		return ( true );
	}

	hasGoid = true;
	GoHandle go( MakeGoid( GetID() ) );
	if ( !go )
	{
		gperror( "GoTarget::ShouldPersist: I'm pointing to a Go that no longer exists!\n" );
		return ( true );
	}

	return ( go->ShouldPersist() );
}


bool
GoTarget::OnDuplicate( Flamethrower_target **pTarget )
{
	Goid target_goid = MakeGoid( m_ID );

	GoHandle hTarget( target_goid );

	if( hTarget )
	{
		*pTarget = new GoTarget( target_goid, m_nema_bone_index );
		return true;
	}
	return false;
}


bool
GoTarget::GetPlacement( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode, const gpstring &sBoneName )
{
	GoHandle hTarget( MakeGoid( m_ID ) );

	// fail if target not valid
	if( !hTarget )
	{
		return false;
	}

	// fail if no placement component
	GoPlacement *pPlacement	= hTarget->QueryPlacement();

	if( !pPlacement )
	{
		return false;
	}

	// is this a target in the frontend gui?
	const bool bGuiGo = pPlacement->IsGuiGo();

	if( !bGuiGo && !hTarget->IsInAnyWorldFrustum() && !hTarget->IsPendingEntry() )
	{
		return false;
	}

	// setup
	GoAspect	 *pAspect = hTarget->QueryAspect();
	nema::Aspect *hAsp	  = NULL;
	
	if( pAspect )
	{
		hAsp = pAspect->GetAspectPtr();

		if( hAsp->IsPurged( 2 ) && hTarget->IsPendingEntry() )
		{
			hAsp = NULL;
		}

		// If we have a parent then we want that position instead so overwrite
		// the original handle with the new handle to the parent and reference
		// that instead
		if( hTarget->IsSpell() && !pAspect->IsNemaRoot() && hTarget->HasParent() )
		{
			hTarget = hTarget->GetParent();
		}
	}

	// --- get nema bone position and orientation
	vector_3	nema_bone_pos;
	Quat		nema_bone_orient;

	// We can only look at the bone position/orientation if the GoAspect is visible
	// if its NOT visible, then we just return ZERO/IDENTITY values
	if( hAsp && pAspect && pAspect->GetIsVisible() ) 
	{
		// Initialize nema_bone_index to the original bone index defined when this
		// GoTarget class was constructed
		int nema_bone_index = m_nema_bone_index;

		// If the bone name specified with this GetPosition call was not empty then
		// we need to figure out exactly where that bone is offset in it's space
		if( !sBoneName.empty() )
		{
			// If the first character is a @ then that means that we need to use the
			// translate from the unchanging bone translator names to correct nema
			// name which was defined in the objects gas file
			if( sBoneName[ 0 ] == '@' )
			{
				// Get the objects translator
				const BoneTranslator &translator = hTarget->GetBody()->GetBoneTranslator();

				// init bone_index to a valid bone
				BoneTranslator::eBone bone_index = BoneTranslator::KILL_BONE;

				// convert the string to a bone translator enumeration
				BoneTranslator::FromString( sBoneName.c_str() + 1, bone_index );

				// Now convert to the nema bone
				if( !translator.Translate( bone_index, nema_bone_index ) )
				{
					nema_bone_index = 0;
				}
			}
			else
			{
				// The first character in the bone name didn't specify the @ character
				// so we don't have to do any translation - just use the nema bone
				hAsp->GetBoneIndex( sBoneName.c_str(), nema_bone_index );
			}
		}

		// this call always sets bone position & orientation even under a failure case so let nema do the assert
		hAsp->GetIndexedBoneOrientation( nema_bone_index, nema_bone_pos, nema_bone_orient );
	}
	else
	{
		nema_bone_pos	 = vector_3::ZERO;
		nema_bone_orient = Quat::IDENTITY;
	}


	// --- Get go placement information
	const SiegePos	&go_pos		= pPlacement->GetPosition();
	const Quat		&go_orient	= pPlacement->GetOrientation();

	SiegePos	target_position	( go_pos );

	const Quat  pre_orient		( ( orient_mode == Flamethrower_target::ORIENT_GO ) ? go_orient : go_orient * nema_bone_orient );
	matrix_3x3	target_orient	( pre_orient.BuildMatrix() );

	// is it possible to do a fast add? as in not be node accurate?
	const bool bFast = ( bGuiGo || (!hTarget->HasPhysics() && !hTarget->HasActor()) );


	// --- determine correct position and orientation
	if( pAspect )
	{
		if( hAsp && hAsp->GetNumBones() > 1 )
		{
			// setup scale stuff
			vector_3 current_scale	( DoNotInitialize );
			hAsp->GetCurrentScale	( current_scale	  );

			float render_scale			= pAspect->GetRenderScale();
			float parent_render_scale	= 1.0f;

			// calculate target offset with scale
			vector_3 offset( current_scale * render_scale * m_Offset );

			Go* pParent = hTarget->GetParent();

			if (pParent && pParent->HasAspect() )
			{
				GoAspect	* pParentGoAspect	= pParent->GetAspect();
				nema::Aspect* pParentAspect		= pParentGoAspect->GetAspectPtr();

				// get parent scale
				vector_3 current_pscale			( DoNotInitialize );
				pParentAspect->GetCurrentScale	( current_pscale  );

				parent_render_scale	= pParentGoAspect->GetRenderScale();

				// adjust offset to parent render scale
				offset *= (current_pscale * parent_render_scale);
			}

			vector_3 scaled_pos		( DoNotInitialize );
			go_orient.RotateVector	( scaled_pos, offset + nema_bone_pos * render_scale * parent_render_scale );

			bool bNodeChange = false;

			if( gWorldFx.AddPositions( target_position, scaled_pos, bFast, &bNodeChange ) )
			{
				// adjust orientation if need be - node will not change if bFast == true
				if( bNodeChange )
				{
					// must initialize to identity
					matrix_3x3 diff_orient;

					gWorldFx.GetDifferenceOrient( go_pos.node, target_position.node, diff_orient );

					target_orient = diff_orient * target_orient;
				}
			}
			else
			{
				// target entered an invalid node - give and error and fail
				gperrorf(( "SiegeFx: Error - target [%s] entered an invalid node\n", hTarget->GetTemplateName() ));
				return false;
			}
		}
		else
		{
			pAspect->GetNodeSpaceCenter( target_position.pos );
		}
	}

	// before leaving this function the node will be valid or fail
	if( !bGuiGo && !gSiegeEngine.IsNodeValid( target_position.node ) )
	{
		return false;
	}

	// set the final values
	if( pPos )
	{
		*pPos = target_position;
	}

	if( pOrient )
	{
		*pOrient = (orient_mode == Flamethrower_target::ORIENT_WORLD) ? matrix_3x3::IDENTITY : target_orient;
	}

	return true;
}


bool
GoTarget::GetDirection( vector_3 &direction )
{
	GoHandle go( MakeGoid( m_ID ) );
	if ( go )
	{
		go->GetPlacement()->Get2DDirection( direction );
		return true;
	}
	return false;
}


bool
GoTarget::GetBoundingRadius( float &radius )
{
	GoHandle go( MakeGoid( m_ID ) );
	if ( go )
	{
		radius = 1.0;
		return true;
	}
	return false;
}


void
GoTarget::SetPositionOffsetName( const gpstring &sName )
{
	GoHandle hTarget( MakeGoid(m_ID) );

	if( hTarget && hTarget->HasAspect() )
	{
		if( !sName.empty() && sName[ 0 ] == '@' )
		{
			if ( hTarget->HasBody() )
			{
				BoneTranslator::eBone bone_index = BoneTranslator::KILL_BONE;

				if( BoneTranslator::FromString( sName.c_str() + 1, bone_index ) )
				{
					if( !hTarget->GetBody()->GetBoneTranslator().Translate( bone_index, m_nema_bone_index ) )
					{
						m_nema_bone_index = 0;
					}
					return;
				}
				gpassertm( 0, "Trying to change the position offset name to an invalid bone - request ignored");
			}
		}
		else
		{
			if( !hTarget->GetAspect()->GetAspectPtr()->GetBoneIndex( sName.c_str(), m_nema_bone_index ) )
			{
				gpstring sValidBones;

				nema::Aspect* pAspect = hTarget->GetAspect()->GetAspectPtr();
				for(UINT32 i = 0; i < pAspect->GetNumBones(); ++i )
				{
					sValidBones.append( pAspect->GetBoneName( i ) );
					sValidBones.append( "\n" );
				}

				gperrorf(("GoTarget::SetPositionOffsetName - Go [%s],bone [%s] does not exist\nValid bones are:\n%s\n",hTarget->GetTemplateName(),sName.c_str(),sValidBones.c_str()));
			}
		}
	}
}



//
// --- GoEmitter	(GoTargetBase)
//
GoEmitter::GoEmitter( Goid GO ) : GoTargetBase( Flamethrower_target::EMITTER )
{
	SetID( MakeInt( GO ) );
};


bool
GoEmitter::OnDuplicate( Flamethrower_target **pTarget )
{
	*pTarget = new GoEmitter;

	(*pTarget)->SetID( m_ID );

	return true;
}


bool
GoEmitter::GetPlacement( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode, const gpstring &sBoneName )
{
	UNREFERENCED_PARAMETER( sBoneName );
	UNREFERENCED_PARAMETER( orient_mode );

	GoHandle hGo( MakeGoid( m_ID ) );

	if( hGo )
	{
		GoPlacement *pPlacement = hGo->GetPlacement();

		if( pPos )
		{
			*pPos = pPlacement->GetPosition();
		}

		if( pOrient )
		{
			*pOrient = pPlacement->GetOrientation().BuildMatrix();
		}

		return true;
	}

	return false;
}


bool
GoEmitter::GetDirection( vector_3 &direction )
{
	GoHandle go( MakeGoid(m_ID) );
	if( go )
	{
		go->GetPlacement()->Get2DDirection( direction );
		return true;
	}

	return false;
}


//
// --- GoStatic		(GoTargetBase)
//
GoStatic::GoStatic( const SiegePos &position ) : GoTargetBase( Flamethrower_target::STATIC )
{
	m_Position = position;

	if ( GoDb::DoesSingletonExist() )
	{
		GoCloneReq cloneReq( gContentDb.GetDefaultPointTemplate() );
		cloneReq.SetStartingPos( position );

		GoHandle hNewGo( gGoDb.CloneLocalGo( cloneReq ) );

		if( !hNewGo.IsValid() )
		{
			gperror("Flamethrower target GoStatic cannot be created from template 'point' - was it deleted or renamed?\n");
		}

		SetID( MakeInt(hNewGo->GetGoid()) );
	}
}


GoStatic::~GoStatic( void )
{
	if ( GoDb::DoesSingletonExist() )
	{
 		Goid target_goid = MakeGoid( (DWORD)GetID() );
		GoHandle hTarget( target_goid );

		if( hTarget && same_no_case( hTarget->GetTemplateName(), "point" ) ) 
		{
			gGoDb.SMarkForDeletion( target_goid );
		}
	}
}


bool
GoStatic::OnDuplicate( Flamethrower_target **pTarget )
{
	GoHandle hGo( MakeGoid( (DWORD)GetID() ) );

	if( hGo )
	{
		*pTarget = new GoStatic( m_Position );
		return true;
	}
	return false;
}


bool
GoStatic::GetPlacement( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode, const gpstring &sBoneName )
{
	UNREFERENCED_PARAMETER( sBoneName );
	UNREFERENCED_PARAMETER( orient_mode );

	if( pPos )
	{
		*pPos = m_Position;
	}

	if( pOrient )
	{
		*pOrient = matrix_3x3::IDENTITY;
	}
	return true;
}


FUBI_DECLARE_CAST_TRAITS( Flamethrower_target::eTargetType, int );

FUBI_DECLARE_XFER_TRAITS( Flamethrower_target* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		Flamethrower_target::eTargetType target_type = Flamethrower_target::INVALID;

		if ( persist.IsSaving() )
		{
			target_type = obj->GetType();
		}

		persist.Xfer( "_target_type", target_type );

		if ( persist.IsRestoring() )
		{
			obj = Flamethrower_target::CreateByType( target_type );
		}

		return ( obj->Xfer( persist ) );
	}
};



//
// --- Tattoo specific tracker stuff
//
TattooTracker::TattooTracker()
{
}


TattooTracker::~TattooTracker( void )
{
	RemoveAllTargets();
}


TattooTracker*
TattooTracker::Duplicate( void )
{
	TattooTracker *pTracker = new TattooTracker;

	TargetColl::iterator i, ibegin = m_Targets.begin(), iend = m_Targets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Flamethrower_target *pTarget = NULL;
		if( (*i)->Duplicate( &pTarget ) )
		{
			pTracker->SetTarget( pTarget, TARGET_INDEX( i.GetIndex() ) );
		}
	}

	return pTracker;
}


#if !GP_RETAIL

gpstring
TattooTracker::ToString( void ) const
{
	gpstring sTargets;

	TargetColl::const_iterator i, ibegin = m_Targets.begin(), iend = m_Targets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		switch( (*i)->GetType() )
		{

		case (Flamethrower_target::DEFAULT):
			{
				const GoTarget& goTarget = *(const GoTarget*)*i;

				// Define the target type as a GoTarget
				sTargets.append( "g," );

				// Append the guid of tha go that this target is going to attach to
				sTargets.appendf( "0x%08X", (*i)->GetID() );
				sTargets.append( "," );

				// Bone index
				sTargets.appendf( "%d", goTarget.GetNemaBoneIndex() );
				sTargets.append( ",*" );

			} break;

		case (Flamethrower_target::STATIC):
			{
				SiegePos	position;
				vector_3	direction;
				float		radius = 1.0f;

				(*i)->GetPlacement( &position, NULL );
				(*i)->GetDirection( direction );
				(*i)->GetBoundingRadius( radius );

				// Define the target type as a static target
				sTargets.append( "s," );

				// Append the database guid without spaces
				sTargets.append( position.node.ToString() );
				sTargets.append( "," );

				// Append the postion of this point in the node
				stringtool::Append( position.pos.x, sTargets );
				sTargets.append( "," );
				stringtool::Append( position.pos.y, sTargets );
				sTargets.append( "," );
				stringtool::Append( position.pos.z, sTargets );
				sTargets.append( "," );

				// Append the direction this point is pointing
				stringtool::Append( direction.x, sTargets );
				sTargets.append( "," );
				stringtool::Append( direction.y, sTargets );
				sTargets.append( "," );
				stringtool::Append( direction.z, sTargets );
				sTargets.append( "," );

				// Append the radius defined for this point
				stringtool::Append( radius, sTargets );
				sTargets.append( ",*" );

			} break;

		case (Flamethrower_target::EMITTER):
			{
				// Define the target type as a GOEmitter
				sTargets.append( "e," );

				// Append the guid of go emitter target
				stringtool::Append( (*i)->GetID(), sTargets );
				sTargets.append( ",*" );

			} break;
		}
	}
	return sTargets;
}

#endif // !GP_RETAIL


void
TattooTracker::AddGoTarget( Goid target_goguid, const char* boneName )
{
	AddTarget( new GoTarget( target_goguid, boneName ) );
}


void
TattooTracker::AddGoTarget( Goid target_goguid, int boneIndex )
{
	AddTarget( new GoTarget( target_goguid, boneIndex ) );
}


void
TattooTracker::AddGoEmitter( Goid target_goguid )
{
	AddTarget( new GoEmitter( target_goguid ) );
}


void
TattooTracker::AddStaticTarget( const SiegePos &position, const vector_3 &direction, float radius )
{
	if( !siege::SiegeEngine::DoesSingletonExist() || gSiegeEngine.IsNodeValid( position.node ) )
	{
		Flamethrower_target* pTarget = new GoStatic( position );
		pTarget->SetDirection( direction );
		pTarget->SetBoundingRadius( radius );

		AddTarget( pTarget );
	}
}


bool
TattooTracker::HasInterestOnlyTarget( void )
{
	TargetColl::iterator it = m_Targets.begin(), it_end = m_Targets.end();

	for(; it != it_end; ++it )
	{
		if( (*it)->GetInterestOnly() )
		{
			return true;
		}
	}

	return false;
}


void
TattooTracker::PrepareForSave( void )
{
	ClearInvalidTargets();
}


bool
TattooTracker::Xfer( FuBi::PersistContext& persist )
{
	gpassert( persist.IsRestoring() || ShouldPersist() );

	persist.XferUnsafeBucketVector( "m_Targets", m_Targets );

	return ( true );
}


void XferTarget( FuBi::BitPacker& packer, TattooTracker& tracker, Flamethrower_target* target = NULL )
{
	Flamethrower_target::eTargetType type;
	if ( packer.IsSaving() )
	{
		gpassert( target != NULL );
		type = target->GetType();
	}

	packer.XferRaw( type, FUBI_MAX_ENUM_BITS( Flamethrower_target::FXTT_ ) );

	switch ( type )
	{
		case ( Flamethrower_target::DEFAULT ):
		{
			UINT32 id = 0;
			int bone = 0;

			if ( packer.IsSaving() )
			{
				const GoTarget& goTarget = *(const GoTarget*)target;
				id = target->GetID();
				bone = goTarget.GetNemaBoneIndex();
			}

			packer.XferRaw( id );
			packer.XferCount( bone );

			if ( packer.IsRestoring() )
			{
				tracker.AddGoTarget( MakeGoid( id ), bone );
			}
		}
		break;

		case ( Flamethrower_target::STATIC ):
		{
			SiegePos	position;
			vector_3	direction;
			float		radius;

			if ( packer.IsSaving() )
			{
				target->GetPlacement( &position, NULL );
				target->GetDirection( direction );
				target->GetBoundingRadius( radius );
			}

			packer.XferIf( position );
			packer.XferFloat( direction.x );
			packer.XferFloat( direction.y );
			packer.XferFloat( direction.z );
			packer.XferFloat( radius );

			if ( packer.IsRestoring() )
			{
				tracker.AddStaticTarget( position, direction, radius );
			}
		}
		break;

		case ( Flamethrower_target::EMITTER ):
		{
			UINT32 id = 0;

			if ( packer.IsSaving() )
			{
				id = target->GetID();
			}

			packer.XferRaw( id );

			if ( packer.IsRestoring() )
			{
				tracker.AddGoEmitter( MakeGoid( id ) );
			}
		}
		break;

		default:
		{
			gpassert( 0 );
		}
	}
}


bool
TattooTracker::Xfer( FuBi::BitPacker& packer )
{
	gpassert( packer.IsSaving() || m_Targets.empty() );
	int size = m_Targets.size();
	packer.XferCount( size );

	if ( packer.IsSaving() )
	{
		TargetColl::const_iterator i, ibegin = m_Targets.begin(), iend = m_Targets.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			XferTarget( packer, *this, *i );
		}
	}
	else
	{
		while ( size-- )
		{
			XferTarget( packer, *this );
		}
	}

	return ( true );
}


bool
TattooTracker::ShouldPersist( void ) const
{
	Flamethrower_target * const* target = m_Targets.SafeDereference( TARGET );
	bool should = true;
	if ( target != NULL )
	{
		bool hasGoid = false;
		if ( !(*target)->ShouldPersist( hasGoid ) && hasGoid )
		{
			should = false;
		}
	}

#	if !GP_RETAIL
	TargetColl::const_iterator i, ibegin = m_Targets.begin(), iend = m_Targets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		bool hasGoid = false;
		if ( ((*i)->ShouldPersist( hasGoid ) != should) && hasGoid )
		{
			gperror( "Flamethrower_target: I have a goid, but its persist state and mine do not match!\n" );
		}
	}
#	endif // !GP_RETAIL

	return ( should );
}


bool
TattooTracker::AtLeastOneTarget( void )
{
	return !m_Targets.empty();
}


stdx::fast_vector< SiegePos >
TattooTracker::GetAllPositions()
{
	stdx::fast_vector< SiegePos > targetCoords;

	TargetColl::iterator i, ibegin = m_Targets.begin(), iend = m_Targets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		SiegePos	NodePosition;

		if( (*i)->GetPlacement( &NodePosition, NULL ) )
		{
			targetCoords.push_back( NodePosition );
		}
	}
	return targetCoords;
}


bool
TattooTracker::GetPlacement( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode, TARGET_INDEX n )
{
	gpassert( n != INVALID );
	Flamethrower_target ** target = m_Targets.SafeDereference( n );

	return ( (target != NULL) && (*target)->GetPlacement( pPos, pOrient, orient_mode, gpstring::EMPTY ) );
}


bool
TattooTracker::GetDirection( vector_3 &dir, TARGET_INDEX n )
{
	gpassert( n != INVALID );
	Flamethrower_target ** target = m_Targets.SafeDereference( n );
	return ( (target != NULL) && (*target)->GetDirection( dir ) );
}


bool
TattooTracker::GetBoundingRadius( float &r, TARGET_INDEX n )
{
	gpassert( n != INVALID );
	Flamethrower_target ** target = m_Targets.SafeDereference( n );
	return ( (target != NULL) && (*target)->GetBoundingRadius( r ) );
}


Goid
TattooTracker::GetID( TARGET_INDEX n ) const
{
	gpassert( n != INVALID );
	Flamethrower_target * const* target = m_Targets.SafeDereference( n );
	return ( (target != NULL) ? MakeGoid( (*target)->GetID() ) : GOID_INVALID );
}


Flamethrower_target::eTargetType
TattooTracker::GetType( TARGET_INDEX n ) const
{
	gpassert( n != INVALID );
	Flamethrower_target * const* target = m_Targets.SafeDereference( n );
	return ( (target != NULL) ? (*target)->GetType() : Flamethrower_target::INVALID );
}


void
TattooTracker::RemoveAllTargets( void )
{
	stdx::for_each( m_Targets, stdx::delete_by_ptr() );
	m_Targets.clear();
}


void
TattooTracker::AddTarget( Flamethrower_target *pTarget )
{
	gpassertm( pTarget, "Adding a null target to a TattooTracker, crash imminent." );

	m_Targets.Alloc( pTarget );
}


void
TattooTracker::SetTarget( Flamethrower_target *pTarget, TARGET_INDEX n )
{
	gpassert( n != INVALID );
	gpassertm( pTarget, "Setting a null target to a TattooTracker, crash imminent." );

	Flamethrower_target ** target = m_Targets.SafeDereference( n );
	if ( target != NULL )
	{
		delete ( *target );
		*target = pTarget;
	}
	else
	{
		gpverify( m_Targets.AllocSpecific( pTarget, n ) );
	}
}


bool
TattooTracker::GetTarget( TARGET_INDEX n, Flamethrower_target **pTarget )
{
	gpassert( n != INVALID );

	Flamethrower_target ** target = m_Targets.SafeDereference( n );
	if ( target != NULL )
	{
		*pTarget = *target;
		return ( true );
	}
	return ( false );
}


bool
TattooTracker::SetPositionOffsetName( const gpstring &sPieceName, TARGET_INDEX n )
{
	gpassert( n != INVALID );

	Flamethrower_target ** target = m_Targets.SafeDereference( n );
	if ( target != NULL )
	{
		(*target)->SetPositionOffsetName( sPieceName );
		return ( true );
	}
	return ( false );
}


bool
TattooTracker::SetPositionOffset( const vector_3 &offset, TARGET_INDEX n )
{
	gpassert( n != INVALID );

	Flamethrower_target ** target = m_Targets.SafeDereference( n );
	if ( target != NULL )
	{
		(*target)->SetPositionOffset( offset );
		return ( true );
	}
	return ( false );
}


bool
TattooTracker::IsValid( bool* shouldDelete )
{
	TargetColl::iterator i, ibegin = m_Targets.begin(), iend = m_Targets.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( !(*i)->IsValid( shouldDelete ) )
		{
			return ( false );
		}
	}

	return ( true );
}


void
TattooTracker::ClearInvalidTargets( void )
{
	for ( TargetColl::iterator i = m_Targets.begin() ; i != m_Targets.end() ; )
	{
		// just remove anything that's invalid, skroo it
		bool shouldDelete;
		if ( !(*i)->IsValid( &shouldDelete ) && shouldDelete )
		{
			Delete( *i );
			UINT index = i.GetIndex();
			++i;
			m_Targets.Free( index );
		}
		else
		{
			++i;
		}
	}
}

//
// --- Flamethrower context tattoo stuff
//
WorldFx::WorldFx( const char *sScriptFuelAddress )
{
#	if !GP_RETAIL

	m_bShowVolumes				= false;

	m_EffectsDrawnLimit			= 300;
	m_EffectsDrawn				= 0;

	m_EffectsUpdatedLimit		= 300;
	m_EffectsUpdated			= 0;

#	endif // !GP_RETAIL

	m_SSLC						= 0;
	IFlamethrower				= new Flamethrower;
	IFlamethrower->Initialize( sScriptFuelAddress );
	
	m_LastScriptID				= 0;
	m_RandomSeed				= ::RandomDword();
	m_bAllowUndefinedNodeGUID	= false;

	gSiegeEngine.RegisterCollectEffectsCb( makeFunctor( *this, &WorldFx::CollectEffectsToDraw ) );
}


WorldFx::~WorldFx()
{
	Delete( IFlamethrower );
}


void WorldFx::Shutdown()
{
	IFlamethrower->Shutdown();
}


bool WorldFx::IsEmpty() const
{
	return ( IFlamethrower->IsEmpty() );
}


FUBI_DECLARE_SELF_TRAITS( Flamethrower );

FUBI_DECLARE_XFER_TRAITS( WorldFx::SPosVector )
{
	static bool XferDef( PersistContext &persist, eXfer /*xfer*/, const char* key, Type& obj )
	{
		return ( persist.XferVector( key, obj ) );
	}
};


void WorldFx::PrepareForSave( void )
{
	IFlamethrower->PrepareForSave();
}


bool
WorldFx::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer   ( "m_SSLC",				m_SSLC			);
	persist.Xfer   ( "IFlamethrower",		*IFlamethrower	);
	persist.Xfer   ( "m_CamOrient",			m_CamOrient		);
	persist.Xfer   ( "m_bAllowUndefinedNodeGUID",			m_bAllowUndefinedNodeGUID	);
	persist.XferHex( "m_LastScriptID",		m_LastScriptID	);
	persist.XferHex( "m_RandomSeed",		m_RandomSeed	);
	persist.XferMap( "m_SiegePosVars",	 	m_SiegePosVars	);

	if( persist.IsRestoring() )
	{
		m_RandomNumberGenerator.SetSeed( m_RandomSeed );
	}

	return ( true );
}

void
WorldFx::SetLightRadiusScaleFactor( float factor )
{
	IFlamethrower->SetLightRadiusScaleFactor( factor );
}


float
WorldFx::GetLightRadiusScaleFactor( void )
{
	return IFlamethrower->GetLightRadiusScaleFactor();
}


void
WorldFx::RebuildScriptVocabulary( void )
{
	IFlamethrower->RebuildScriptVocabulary();
}


bool
WorldFx::DestroyScript( const char *sName )
{
	return IFlamethrower->DestroyScript( sName );
}


SFxSID
WorldFx::SRunScript( const char *sName, def_tracker targets, const char *sParams, Goid Owner, eWorldEvent type, bool server, bool xfer )
{
	// parse name, there may be flags at the end
	const char* paren = ::strchr( sName, '(' );
	gpstring local;
	if ( paren != NULL )
	{
		// copy local
		local.assign( sName, paren );

		// parse flags
		bool global = false;
		for ( ++paren ; (*paren != '\0') && (*paren != ')') ; ++paren )
		{
			switch ( *paren )
			{
				case ( 's' ):
				{
					if ( !::IsServerLocal() )
					{
						return ( 0 );
					}
				}
				break;

				case ( 'g' ):
				{
					global = true;
				}
				break;

				case ( 'l' ):
				{
					server = false;
				}
				break;

				case ( 'x' ):
				{
					xfer = true;
				}
				break;

				default:
				{
					if ( !::isspace( *paren ) )
					{
						gperrorf(( "Unrecognized flag '%c' in script request for '%s'\n", *paren, sName ));
					}
				}
			}
		}

		// redirect
		stringtool::RemoveBorderingWhiteSpace( local );
		sName = local;

		// check
		if ( global || xfer )
		{
			CHECK_SERVER_ONLY;
			server = true;
		}
	}

	if( ValidTargets( targets, sName ) )
	{
		m_LastScriptID = IFlamethrower->RunScript( sName, targets, sParams, Owner, type, server, xfer );
		return m_LastScriptID;
	}
	return 0;
}


SFxSID
WorldFx::SRunScript( const char *sName, Goid target, Goid source, const char *sParams, Goid Owner, eWorldEvent type, bool server, bool xfer )
{
	return ( SRunScript( sName, MakeTracker( target, source ), sParams, Owner, type, server, xfer ) );
}


bool
WorldFx::SRunScriptSpecial( const char *sName, const char *sParams, Goid Owner, eWorldEvent type,
							const SiegePos &target_pos, float radius, float x_offset, float y_offset, float z_offset, bool server )
{
	gpassert( GoHandle( Owner ).IsValid() );
	gpassert( target_pos.node.IsValidSlow() );

	if( !gSiegeEngine.IsNodeValid( target_pos.node ) )
	{
		return false;
	}

	// Create two temporary targets from the target position target_pos
	SiegePos spawn_point( target_pos );
	SiegePos end_point( target_pos );

	float sin_val = 0;
	float cos_val = 0;

	SINCOSF( Random( (float)0, (float)PI2 ), sin_val, cos_val );

	const float rnd_radius = Random( 0.0f, radius );
	const float rand_x = rnd_radius * cos_val;
	const float rand_z = rnd_radius * sin_val;

	end_point.pos.x += rand_x;
	end_point.pos.z += rand_z;

	spawn_point.pos.x += ( rand_x + x_offset );
	spawn_point.pos.y += y_offset;
	spawn_point.pos.z += ( rand_z + z_offset );

	def_tracker targets = MakeTracker();
	targets->AddTarget( new GoStatic( end_point ) );
	targets->AddTarget( new GoStatic( spawn_point ) );

	return ( SRunScript( sName, targets, sParams, Owner, type, server ) != 0 );
}


void
WorldFx::SStopScript( SFxSID id )
{
	IFlamethrower->SStopScript( id );
}

void
WorldFx::SStopScript( Goid id, const char *sName )
{
	IFlamethrower->SStopScript( id, sName );
}

void
WorldFx::StopScript( SFxSID id )
{
	IFlamethrower->StopScript( id );
}


void
WorldFx::StopScript( Goid id, const char *sName )
{
	IFlamethrower->StopScript( id, sName );
}


void
WorldFx::FinishScript( SFxSID id )
{
	IFlamethrower->FinishScript( id );
}


void
WorldFx::ReplaceScriptTargetIDs( SFxSID script_id, Goid target, Goid source )
{
	Flamethrower_script *pScript = 0;

	if( IFlamethrower->FindScript( script_id, &pScript ) )
	{
		SFxEIDColl effect_ids = pScript->GetEffectLog();
		SFxEIDColl::iterator iEffect = effect_ids.begin(), id_end = effect_ids.end();

		// First replace any of the active effects target id's with the new specified ids
		for(; iEffect != id_end; ++iEffect )
		{
			TattooTracker *pTracker = 0;
			GetTargets( *iEffect, &pTracker );

			Flamethrower_target *pTarget = NULL;
			pTracker->GetTarget( TattooTracker::TARGET, &pTarget );
			pTarget->SetID( MakeInt( target ) );
			pTracker->GetTarget( TattooTracker::SOURCE, &pTarget );
			pTarget->SetID( MakeInt( source ) );
		}

		// Now replace any soon to be active effects with the specified ids
		TattooTracker *pTracker = pScript->GetDefaultTargets();
		Flamethrower_target *pTarget = NULL;
		pTracker->GetTarget( TattooTracker::TARGET, &pTarget );
		pTarget->SetID( MakeInt( target ));
		pTracker->GetTarget( TattooTracker::SOURCE, &pTarget );
		pTarget->SetID( MakeInt( source ));
	}
}


Goid
WorldFx::GetRegistrationGoid( SFxSID id ) const
{
	return IFlamethrower->GetRegistrationGoid( id );
}


void
WorldFx::StopAllScripts( void )
{
	IFlamethrower->StopAllScripts();
}


void
WorldFx::UnloadScript( SFxSID id )
{
	IFlamethrower->UnloadScript( id );
}


void
WorldFx::LoadScript( SFxSID id )
{
	IFlamethrower->LoadScript( id );
}


void
WorldFx::GetEffectsForScript( SFxSID id, SEVector & effectColl )
{
	// Get the script
	Flamethrower_script* pScript = NULL;
	if( IFlamethrower->FindScript( id, &pScript ) && pScript )
	{
		// Get the effects associated with this script
		Flamethrower_script::def_eidstack &effects = pScript->GetEffectLog();
		Flamethrower_script::def_eidstack::iterator effect = effects.begin(), effects_end = effects.end();

		for( ; effect != effects_end; ++effect )
		{
			SpecialEffect* pEffect = IFlamethrower->FindEffect( *effect );
			if( pEffect )
			{
				switch( pScript->GetRegistrationEvent() )
				{
					case WE_CONSTRUCTED:
					case WE_ENTERED_WORLD:
					case WE_EQUIPPED:
					case WE_REQ_CAST:
					case WE_REQ_USE:
					case WE_UNKNOWN:
					case WE_ANIM_SFX:
					{
						effectColl.push_back( pEffect );
					}
				}
			}
		}
	}
}


bool
WorldFx::SetScriptCallback( SFxSID id, const CBFunctor1< SFxSID > callback )
{
	return IFlamethrower->SetScriptCallback( id, callback );
}


bool
WorldFx::ClearScriptCallback( SFxSID id )
{
	return IFlamethrower->SetScriptCallback( id, CBFunctor1< SFxSID > () );
}


void
WorldFx::PostSignal( SFxSID id, const gpstring &sSignal )
{
	if( ScriptUsesSignals( id ) )
	{
		IFlamethrower->PostSignal( id, sSignal );
	}
}


void
WorldFx::AddFriendly( SFxEID effect_id, Goid id )
{
	IFlamethrower->AddFriendly( effect_id, id );
}

void
WorldFx::SetFinishing( SFxEID effect_id )
{
	IFlamethrower->SetFinishing( effect_id );
}


SFxEID
WorldFx::CreateEffect( const char *sName, const char *sParams, def_tracker targets, Goid owner_id, SFxSID script_id, UINT32 seed )
{
	return IFlamethrower->CreateEffect( sName, sParams, targets, owner_id, script_id, seed );
}


SpecialEffect*
WorldFx::CreateEffect( const char *sName )
{
	return IFlamethrower->CreateEffect( sName );
}


void
WorldFx::SetTargets( SFxEID id,	def_tracker targets )
{
	IFlamethrower->SetTargets( id, targets );
}


bool
WorldFx::GetTargets( SFxEID id, TattooTracker **pTracker )
{
	return IFlamethrower->GetTargets( id, pTracker );
}


void
WorldFx::DestroyEffect( SFxEID id )
{
	IFlamethrower->DestroyEffect( id );
}


void
WorldFx::DestroyAllEffects()
{
	IFlamethrower->DestroyAllEffects();
}


void
WorldFx::UnloadEffect( SFxEID id )
{
	IFlamethrower->UnloadEffect( id );
}


SpecialEffect*
WorldFx::FindEffect( SFxEID id )
{
	return IFlamethrower->FindEffect( id );
}


void
WorldFx::LoadEffect( SFxEID id )
{
	IFlamethrower->LoadEffect( id );
}


bool
WorldFx::GetDirection( SFxEID id, vector_3 &dir )
{
	return IFlamethrower->GetDirection( id, dir );
}


bool
WorldFx::SetDirection( SFxEID id, const vector_3 &dir )
{
	return IFlamethrower->SetDirection( id, dir );
}


bool
WorldFx::GetOrientation( SFxEID id, matrix_3x3 &orientation )
{
	return IFlamethrower->GetOrientation( id, orientation );
}


bool
WorldFx::SetOrientation( SFxEID id, const matrix_3x3 &orientation )
{
	return IFlamethrower->SetOrientation( id, orientation );
}


bool
WorldFx::GetPosition( SFxEID id, SiegePos &pos )
{
	return IFlamethrower->GetPosition( id, pos );
}


bool
WorldFx::SetPosition( SFxEID id, const SiegePos &pos )
{
	return IFlamethrower->SetPosition( id, pos );
}


bool
WorldFx::OffsetPosition( SFxEID id, vector_3 &offset, matrix_3x3 &basis )
{
	return IFlamethrower->OffsetPosition( id, offset, basis );
}


bool
WorldFx::OffsetTargetPosition( SFxEID id, const TattooTracker::TARGET_INDEX index,
											const vector_3 &offset )
{
	gpassert( index != 0 );
	return IFlamethrower->OffsetTargetPosition( id, index, offset );
}


bool
WorldFx::MakeTargetsStatic( SFxEID id, bool snap_to_ground )
{
	return IFlamethrower->MakeTargetsStatic( id, snap_to_ground );
}


def_tracker
WorldFx::MakeTrackerTargetsStatic( def_tracker old_targets, bool snap_to_ground )
{
	return IFlamethrower->MakeTrackerTargetsStatic( old_targets, snap_to_ground );
}


void
WorldFx::ConvertTargetToStatic( Flamethrower_target *pOldTarget, Flamethrower_target *&pNewTarget, bool snap_to_ground )
{
	IFlamethrower->ConvertTargetToStatic( pOldTarget, pNewTarget, snap_to_ground );
}


void
WorldFx::SetInterestOnly( SFxEID id, bool val )
{
	SpecialEffect *pEffect = IFlamethrower->FindEffect( id );

	if( pEffect )
	{
		pEffect->SetInterestOnly( val );
	}
}


bool
WorldFx::GetBoundingRadius( SFxEID id, float &radius )
{
	return IFlamethrower->GetBoundingRadius( id, radius );
}


void
WorldFx::AddEffectTarget( SFxEID id, SFxEID target_id )
{
	IFlamethrower->AddEffectTarget( id, target_id );
}


void
WorldFx::AddTarget( SFxEID id, Flamethrower_target *pTarget )
{
	IFlamethrower->AddTarget( id, pTarget );
}


void
WorldFx::RemoveAllTargets( SFxEID id )
{
	IFlamethrower->RemoveAllTargets( id );
}


bool
WorldFx::AttachEffect( SFxEID parent_id, SFxEID child_id )
{
	return IFlamethrower->AttachEffect( parent_id, child_id );
}


void
WorldFx::StartEffect( SFxEID id )
{
	IFlamethrower->StartEffect( id );
}


void
WorldFx::StopEffect( SFxEID id )
{
	IFlamethrower->StopEffect( id );
}


void
WorldFx::StopAllEffects( void )
{
	IFlamethrower->StopAllEffects();
}


void
WorldFx::StartAllEffects( void )
{
	IFlamethrower->StartAllEffects();
}


void
WorldFx::Update( float SecondsElapsed, float UnpausedSecondsElapsed )
{
#if !GP_RETAIL
	m_EffectsDrawn	 = 0;
	m_EffectsUpdated = 0;
#endif

	m_SSLC	 = SecondsElapsed;
	m_CamPos = gSiegeEngine.GetCamera().GetCameraPosition();
	m_CamDir = gSiegeEngine.GetCamera().GetMatrixOrientation().GetColumn2_T();

	IFlamethrower->Update( SecondsElapsed, UnpausedSecondsElapsed );
}


void
WorldFx::CollectEffectsToDraw( siege::WorldEffectColl &collection, const siege::SiegeNode &node )
{
	GPSTATS_GROUP( GROUP_RENDER_FX );

	IFlamethrower->CollectEffectsToDraw( collection, node );
}


void
WorldFx::PostCollision( CollisionInfo *pTakeCollision )
{
	gpassert( pTakeCollision != NULL );

	if( IsInWorldFrustum( pTakeCollision->GetCollisionPoint().node ) )
	{
		CollisionInfo *pTemp = NULL;
		if( !IFlamethrower->FindCollision( pTakeCollision->GetID(), &pTemp ) )
		{
			WorldMessage( WE_SPELL_COLLISION, pTakeCollision->GetTargetID(), pTakeCollision->GetOwnerID(),
							(DWORD)((SiegePos*)(&pTakeCollision->GetCollisionPoint())) ).Send();

			IFlamethrower->AddCollision( pTakeCollision );
		}
		else
		{
			// already in there, delete this to prevent leak
			delete ( pTakeCollision );
		}
	}
}


bool
WorldFx::EffectHasCollided( SFxEID id, CollisionInfo &info )
{
	return IFlamethrower->EffectHasCollided( id, info );
}


bool
WorldFx::GetCollisionPoint( SFxEID id, SiegePos &point )
{
	return IFlamethrower->GetCollisionPoint( id, point );
}


bool
WorldFx::GetCollisionNormal( SFxEID id, SiegePos &normal )
{
	return IFlamethrower->GetCollisionNormal( id, normal );
}


bool
WorldFx::GetCollisionDirection( SFxEID id, vector_3 &dir )
{
	return IFlamethrower->GetCollisionDirection( id, dir );
}


bool
WorldFx::GetCollisionTargetID( SFxEID id, Goid &target_id )
{
	return IFlamethrower->GetCollisionTargetID( id, target_id );
}


void
WorldFx::Message( const char *sEvent, Goid from, Goid to, DWORD data )
{
	if( gServer.IsLocal() )
	{
		eWorldEvent world_event = WE_UNKNOWN;

		if( FromString( sEvent, world_event ) )
		{
			WorldMessage( world_event, from, to, data ).Send();
		}
	}
}


def_tracker
WorldFx::MakeTracker( void )
{
	return def_tracker( new TattooTracker );
}


def_tracker
WorldFx::MakeTracker( Goid target, Goid source )
{
	// First setup the target target
	Flamethrower_target *pTarget = NULL;
	Flamethrower_target *pSource = NULL;

	pTarget = new GoTarget( target );

	// If the source targets GUID is GOID_INVALID then the caller either specified an invalid guid or
	// the source target was not specified so just recycle the target target as the source target so
	// that the script doesn't error out
	if( source == GOID_INVALID )
	{
		pSource = new GoTarget( target );
	}
	else
	{
		pSource = new GoTarget( source );
	}

	def_tracker pTracker( new TattooTracker );
	pTracker->SetTarget( pTarget, TattooTracker::TARGET );
	pTracker->SetTarget( pSource, TattooTracker::SOURCE );

	return pTracker;
}



//
// Start of internal Flamethrower calls
//

bool
WorldFx::IsServerLocal( void )
{
	return gServer.IsLocal();
}


void
WorldFx::UnRegisterScriptsFromGO( Flamethrower_script &Script )
{
	if ( GoDb::DoesSingletonExist() )
	{
		// Make sure the go still exists before trying to unregister an effect
		// the go could have been destroyed by burning to death

		if( GoHandle( Script.GetOwner() ) )
		{
			gGoDb.UnregisterEffectScript( Script.GetOwner(), Script.GetID() );
		}
	}
}


bool
WorldFx::GetCullState( void ) const
{
	return gSiegeEngine.GetOptions().GetEffectsCulling();
}


bool
WorldFx::IsEffectVisible( SpecialEffect * pEffect )
{
	bool bIsVisible = false;

	Flamethrower_target *pTargetTarget = NULL, *pTargetSource = NULL;

	const def_tracker & targets = pEffect->GetTargets();

	if( targets->GetTarget( TattooTracker::TARGET, &pTargetTarget ) ||
		targets->GetTarget( TattooTracker::SOURCE, &pTargetSource ) )
	{
		bool bCulledByInventory = false;

		if( pTargetTarget && IsTargetVisible( pEffect, pTargetTarget, bCulledByInventory ) )
		{
			bIsVisible = true;
		}
		else if( !bCulledByInventory && pTargetSource && (pTargetSource != pTargetTarget) && IsTargetVisible( pEffect, pTargetSource, bCulledByInventory ) )
		{
			bIsVisible = true;
		}
		else if( !bCulledByInventory )
		{
			SiegePos CullPosition;
			if( !pEffect->GetInterestOnly() && pEffect->GetCullPosition( CullPosition ) )
			{
				const vector_3 center( CullPosition.WorldPos() );
				const float radius	 = pEffect->GetBoundingRadius();

				return ( !gSiegeEngine.GetCamera().GetFrustum().CullSphere( center, radius ) );
			}
		}
	}
	return bIsVisible;
}


bool
WorldFx::IsTargetVisible( SpecialEffect * pEffect, Flamethrower_target *pTarget, bool & bCulledByInventory )
{
	Goid go_target = GOID_INVALID;
	const Flamethrower_target::eTargetType ttype = pTarget->GetType();

	if( (ttype == Flamethrower_target::DEFAULT) || (ttype == Flamethrower_target::EMITTER ) )
	{
		go_target = MakeGoid( pTarget->GetID() );
	}
	else if( ttype == Flamethrower_target::EFFECT )
	{
		if( pTarget->GetIsInsideInventory() )
		{
			bCulledByInventory = true;
			return false;
		}

		SiegePos position;

		if( pEffect->GetInterestOnly() || !pEffect->GetCullPosition( position ) )
		{
			return false;
		}

		const float radius = pEffect->GetBoundingRadius();
		return ( !gSiegeEngine.GetCamera().GetFrustum().CullSphere( position.WorldPos(), radius ) );
	}

	GoHandle hGoTarget( go_target );
	if( hGoTarget && hGoTarget->HasAspect() )
	{
		const SiegePos & position = hGoTarget->GetPlacement()->GetPosition();
		siege::SiegeNodeHandle Handle( gSiegeEngine.NodeCache().UseObject( position.node ) );
		siege::SiegeNode const &Node = Handle.RequestObject( gSiegeEngine.NodeCache() );

		bool bIsVisible = true;

		if( !hGoTarget->IsInViewFrustum() || (Node.GetNodeAlpha() == 0x00000000) )
		{
			bIsVisible = false;
		}
		else if( hGoTarget->IsInsideInventory() && !hGoTarget->GetAspect()->GetIsVisible() )
		{
			bIsVisible = false;
			bCulledByInventory = true;
		}
		else if( hGoTarget->GetAspect()->GetInterestOnly() && !Node.IsInterestByFrustum( gSiegeEngine.GetRenderFrustum() ) )
		{
			bIsVisible = false;
		}

		if( bIsVisible )
		{
			const float radius = pEffect->GetBoundingRadius();

			return ( !gSiegeEngine.GetCamera().GetFrustum().CullSphere( position.WorldPos(), radius ) );
		}
	}
	return false;
}


bool
WorldFx::IsInWorldFrustum( const siege::database_guid &node )
{
	return gSiegeEngine.IsNodeInAnyFrustum( node ) != NULL;
}


bool
WorldFx::IsNodeVisible( const siege::database_guid & node )
{
	if( m_bAllowUndefinedNodeGUID )
	{
		return true;
	}

	siege::SiegeNode *pNode = gSiegeEngine.IsNodeValid( node );

	return ( (pNode != NULL) && (pNode->GetNodeAlpha() != 0x00000000) );
}


bool
WorldFx::IsScriptOwnerVisible( const SFxSID& script_id )
{
	// Get the go
	GoHandle hGo( GetRegistrationGoid( script_id ) );
	if( hGo.IsValid() && (hGo->HasAspect() && hGo->IsInsideInventory()) )
	{
		return hGo->GetAspect()->GetIsVisible();
	}

	return true;
}


void
WorldFx::RestoreRenderingState( void ) const
{
	Rapi&	renderer = gSiegeEngine.Renderer();
	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	TRUE );
	renderer.GetDevice()->SetRenderState( D3DRS_CULLMODE,		D3DCULL_CW );
	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,		D3DSHADE_GOURAUD );
}


void
WorldFx::SaveRenderingState( void ) const
{
	Rapi&	renderer = gSiegeEngine.Renderer();

	renderer.SetTextureStageState(	0,
									D3DTOP_MODULATE,
									D3DTOP_MODULATE,

									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DTEXF_LINEAR,
									D3DBLEND_SRCALPHA,
									D3DBLEND_ONE,
									false );

	renderer.GetDevice()->SetRenderState( D3DRS_ZWRITEENABLE,	FALSE );
	renderer.GetDevice()->SetRenderState( D3DRS_CULLMODE,		D3DCULL_NONE );
	renderer.GetDevice()->SetRenderState( D3DRS_SHADEMODE,		D3DSHADE_FLAT );
}


float
WorldFx::GetGroundHeight( const SiegePos &position )
{
	SiegePos temp_pos = position;
	gSiegeEngine.AdjustPointToTerrain( temp_pos );
	return temp_pos.pos.y;
}


bool
WorldFx::RayCast( const SiegePos &start_pos, const SiegePos &end_pos,
									 vector_3 &normal, SiegePos &point )
{
	if( start_pos == end_pos )
	{
		return false;
	}

	if( gSiegeEngine.HitTestGlobalTerrain( start_pos, end_pos, point, normal, (siege::LF_IS_WALL | siege::LF_IS_FLOOR) ) )
	{
		return true;
	}
	return false;
}


bool
WorldFx::UpdateNodePosition( SiegePos &position )
{
	bool success = gSiegeEngine.UpdateNodePosition( position );

	gpassert (!(position.node == UNDEFINED_GUID));

	return success;
}


bool
WorldFx::AdjustPointToTerrain( SiegePos &position )
{
	if( IsNodeVisible( position.node ) )
	{
		return gSiegeEngine.AdjustPointToTerrain( position );
	}
	return false;
}


bool
WorldFx::CheckForCollision(	bool bIgnoreObjects, const SiegePos &start_position, const SiegePos &end_position,
							float Width, float Height, const GoidColl & ignore_list,
							Goid &ContactGUID, SiegePos &ContactPoint, SiegePos &ContactNormal, Goid Owner )
{
	if( m_SSLC == 0 )
	{
		return false;
	}

	// Make sure the owner of the collision is an actual actor and not a spell
	GoHandle hOwner( Owner );
	Goid OwnerOwner( GOID_INVALID );
	if( hOwner && hOwner->HasParent() )
	{
		OwnerOwner = hOwner->GetParent()->GetGoid();
	}

	if( gSim.CheckForTerrainCollision( ContactPoint, ContactNormal, start_position, end_position ) )
	{
		ContactGUID	= GOID_INVALID;
		return true;
	}

	if( bIgnoreObjects )
	{
		return false;
	}

	return gSim.CheckForObjectCollision( ContactPoint, ContactNormal, ContactGUID, ignore_list, start_position, end_position, Width, Height, OwnerOwner );
}


bool
WorldFx::CreateLightSource( siege::database_guid &light_id, const SiegePos &position, const vector_3 &color,
												float inner_falloff, float outer_falloff, bool bCastShadow )
{
	if( gWorldFx.IsNodeVisible( position.node ) )
	{
		light_id = gSiegeEngine.LightDatabase().InsertPointLightSource( position, color, UNDEFINED_GUID,
															false,								// dynamic light
															true,								// activate
															inner_falloff, outer_falloff,		// light radii
															1.0f,								// intensity
															bCastShadow,						// draw shadow
															false );							// don't calculate occlusion
		return ( light_id != UNDEFINED_GUID );
	}
	return false;
}


bool
WorldFx::PositionLightSource( const siege::database_guid &light_id, const SiegePos &position, unsigned long color )
{
	if( gSiegeEngine.LightDatabase().HasLightDescriptor( light_id ) && IsNodeVisible( position.node ) )
	{
		gSiegeEngine.LightDatabase().SetLightPosition( light_id, position );
		siege::LightDescriptor ld = gSiegeEngine.LightDatabase().GetLightDescriptor( light_id );

		if( color != ld.m_Color )
		{
			ld.m_Color = color;
			gSiegeEngine.LightDatabase().SetLightDescriptor( light_id, ld );
		}

		return true;
	}

	return false;
}


void
WorldFx::DestroyLightSource( const siege::database_guid &light_id )
{
	gSiegeEngine.LightDatabase().DestroyLight( light_id );
}


void
WorldFx::SpawnLocalPhysicsObject( const char *sName, const SiegePos &initial_position,
							 const vector_3 &linear_acceleration, const vector_3 &linear_velocity,
							 const vector_3 &angular_acceleration, const vector_3 &angular_velocity,
							 const GoidColl & ignore_list )
{
	gSim.SpawnLocalPhysicsObject( sName, initial_position, linear_acceleration, linear_velocity,
							angular_acceleration, angular_velocity, ignore_list );
}


Goid
WorldFx::CreateEffectObject( Goid owner, const char *sName, const SiegePos &initial_position )
{
	UNREFERENCED_PARAMETER( owner );

	GoCloneReq cloneReq( sName );
	cloneReq.SetStartingPos( initial_position );
	GoHandle hNewGo( gGoDb.CloneLocalGo( cloneReq ) );

	hNewGo->GetAspect()->SetIsSelectable( false );

	if( hNewGo )
	{
		return hNewGo->GetGoid();
	}

	gperrorf(("SiegeFx effect is referenceing a go that doesn't exist - [%xs]\n", sName));
	return GOID_INVALID;
}


void
WorldFx::SetEffectObjectPosition( Goid model, const SiegePos &position )
{
	GoHandle hModel( model );

	if( hModel && IsNodeVisible( position.node ) )
	{
		hModel->GetPlacement()->SetPosition( position, false );
	}
}


void
WorldFx::SetEffectObjectOrientation( Goid model, const matrix_3x3 &orient )
{
	GoHandle hModel( model );

	if( hModel )
	{
		Quat qorient( orient );
		hModel->GetPlacement()->SetOrientation( qorient );
	}
}


void
WorldFx::RotateEffectObject( Goid model, const vector_3 &angles )
{
	GoHandle hModel( model );

	if( hModel )
	{
		Quat orient = hModel->GetPlacement()->GetOrientation();
		orient.RotateX( angles.x );
		orient.RotateY( angles.y );
		orient.RotateZ( angles.z );

		hModel->GetPlacement()->SetOrientation( orient );
	}
}


void
WorldFx::DestroyEffectObject( Goid model )
{
	gGoDb.SMarkForDeletion( model );

}


float
WorldFx::GetEffectObjectScale( Goid model )
{
	GoHandle hModel( model );

	if( hModel && hModel->HasAspect() )
	{
		return hModel->GetAspect()->GetRenderScaleMultiplier();
	}
	return 0.0f;
}


void
WorldFx::SetEffectObjectScale( Goid model, float scale_factor )
{
	GoHandle hModel( model );

	if( hModel && hModel->HasAspect() )
	{
		hModel->GetAspect()->SetRenderScaleMultiplier( scale_factor );
	}
}


bool
WorldFx::CreateDecal(	unsigned int &decal_id, const SiegePos &pos,
											const matrix_3x3 &orient, float hMeters, float vMeters,
											float nearPlane, float farPlane,
											const char *sTexture )
{
	decal_id = gSiegeEngine.DecalDatabase().CreateDecal( pos, orient, hMeters, vMeters, nearPlane, farPlane, sTexture );
	return !!decal_id;
}


void
WorldFx::DestroyDecal( unsigned int decal_id, bool remove_texture )
{
	gSiegeEngine.DecalDatabase().DestroyDecal( decal_id, remove_texture );
}


bool
WorldFx::SetDecalAlpha( unsigned int decal_id, float alpha )
{
	siege::SiegeDecal *pDecal = gSiegeEngine.DecalDatabase().GetDecalPointer( decal_id );

	if( pDecal )
	{
		pDecal->SetAlpha( char(255.0f*alpha) );

		return true;
	}
	return false;
}


void
WorldFx::GetWorldWindVector( vector_3 &vector ) const
{
	vector = gWeather.GetWind()->GetWorldDirection();
}


void
WorldFx::GetNodeWindVector( const siege::database_guid &node, vector_3 &vector ) const
{
	vector = gWeather.GetWind()->GetNodeDirection( node );
}


void
WorldFx::GetTargetNodeGUID( siege::database_guid &guid ) const
{
	siege::SiegeWalker &walker = gSiegeEngine.NodeWalker();
	guid = walker.TargetNodeGUID();
}


const matrix_3x3&
WorldFx::GetNodeOrientation( const SiegePos &position )
{
	siege::SiegeNode* pNode	= gSiegeEngine.IsNodeInAnyFrustum( position.node );
	if( pNode )
	{
		return pNode->GetCurrentOrientation();
	}
	return matrix_3x3::IDENTITY;
}


const vector_3&
WorldFx::GetNodeCenter( const SiegePos &position )
{
	siege::SiegeNode* pNode	= gSiegeEngine.IsNodeInAnyFrustum( position.node );
	if( pNode )
	{
		return pNode->GetCurrentCenter();
	}
	return vector_3::ZERO;
}


void
WorldFx::GetNodeOrientAndCenter( const SiegePos &position, matrix_3x3 &orient, vector_3 &center )
{
	siege::SiegeNode* pNode	= gSiegeEngine.IsNodeInAnyFrustum( position.node );
	if( pNode )
	{
		orient = pNode->GetCurrentOrientation();
		center = pNode->GetCurrentCenter();
	}
}


void
WorldFx::GetNodeOrientAndCenter( const siege::database_guid node, matrix_3x3 &orient, vector_3 &center )
{
	siege::SiegeNode* pNode	= gSiegeEngine.IsNodeInAnyFrustum( node );
	if( pNode )
	{
		orient = pNode->GetCurrentOrientation();
		center = pNode->GetCurrentCenter();
	}
}


DWORD
WorldFx::PlaySample( const char *sName, const SiegePos &position, bool loop, float min_dist, float max_dist )
{
	return gWorldSound.PlaySample( sName, position, loop, min_dist, max_dist );
}


DWORD
WorldFx::PlaySample( const char *sName, const SiegePos &position, bool loop )
{
	return gWorldSound.PlaySample( sName, position, loop );
}


DWORD
WorldFx::PlaySample( const char *sName, bool loop )
{
	return gSoundManager.PlaySample( sName, loop );
}

void
WorldFx::StopSample( DWORD id )
{
	gWorldSound.ClearSampleStopCallback( id );
	gSoundManager.StopSample( id );
}

float
WorldFx::GetSampleLengthInSeconds( const char *sName )
{
	return gSoundManager.GetSampleLengthInSeconds( sName );
}

void
WorldFx::SetSampleStopCallback( DWORD id, Flamethrower_script *pScript )
{
	if( pScript )
	{
		gWorldSound.SetSampleStopCallback( id, makeFunctor( *pScript, &Flamethrower_script::RemoveSoundID ) );
	}
}


bool
WorldFx::FindScript( SFxSID id, Flamethrower_script **pScript )
{
	return IFlamethrower->FindScript( id, pScript );
}

void
WorldFx::ApplyDamage( Goid Attacker, Goid Victim, float damage_min, float damage_max, bool bIgnite, float t, bool bCalcFireResist )
{
	if( gServer.IsLocal() )
	{
		GoHandle hAttacker( Attacker );

		// Determine who the actor is that should recieve the experience points
		Goid AttackerWeapon = Attacker;
		bool bAwardXP = false;

		if( hAttacker && !hAttacker->HasActor() && hAttacker->HasMagic() && hAttacker->HasParent() )
		{
			Go *pParent = hAttacker->GetParent();

			gpassert( pParent->HasActor() );

			if( pParent->HasActor() )
			{	// the caster
				Attacker = pParent->GetGoid();
				bAwardXP = pParent->GetActor()->GetCanLevelUp();
			}

			// don't allow the attcaker to damage his friends
			if( pParent && pParent->HasMind() )
			{
				GoHandle hVictim( Victim );

				if( hVictim )
				{
					if( pParent->GetMind()->IsFriend( hVictim ) )
					{
						return;
					}
				}
			}
		}

		// don't allow the attacker to damage himself
		if( Attacker == Victim )
		{
			return;
		}

		// Damage the object based on exposure
		gRules.DamageGoParticle( Victim, Attacker, AttackerWeapon, damage_min, damage_max, t, bAwardXP, bIgnite, bCalcFireResist );
	}
}


void
WorldFx::DamageWithinBox( const SiegePos &position, const matrix_3x3 &orient, const vector_3 &half_diag,
						float damage_min, float damage_max, bool bIgnite, float t, bool bCalcFireResist, const GoidColl &ignore, Goid Owner )
{
	CHECK_SERVER_ONLY;

#	if !GP_RETAIL
	if( m_bShowVolumes )
	{
		SiegePos world_pos( position );
		world_pos.pos = world_pos.WorldPos();

		gWorld.DrawDebugBox( world_pos, orient , half_diag, 0xffFF8000, 0.17f, true );
	}
#endif

	if( t == 0 )
	{
		return;
	}

	// Create the collision volume
	oriented_bounding_box_3 a_obb( position.WorldPos(), half_diag, orient );

	// Lookup the owner
	GoHandle hOwner( Owner );
	Go *pParent = hOwner;

	// The spell
	if( hOwner && !hOwner->HasActor() )
	{
		// The spell book or the caster
		if( hOwner->GetParent( pParent ) )
		{
			// The caster or the party
			if( !hOwner->HasActor() )
			{
				hOwner->GetParent( pParent );
			}
		}
	}


	// Use a sphere test with radius of half_diag to get possible collision candidates capable of burning
	GopColl occupants;
	gAIQuery.GetOccupantsInSphere(
			position,
			a_obb.GetHalfDiagonal().Length(),
			pParent,
			&ignore,
			0,
			OF_CAN_BURN | OF_CULL_PARTY_MEMBERS | OF_CULL_SAME_MEMBERSHIP | OF_CULL_GHOSTS | OF_CULL_INVINCIBLE,
			&occupants,
			false );

	// Check all the occupants to see if they qualify to recieve damage
	for( GopColl::iterator i = occupants.begin(), occ_end = occupants.end(); i != occ_end; ++i )
	{
		matrix_3x3	b_orient;
		vector_3	b_center, b_half_d;

		// Get the positional information for the current collision candidate
		(*i)->GetAspect()->GetWorldSpaceOrientedBoundingVolume( b_orient, b_center, b_half_d );
		oriented_bounding_box_3 b_obb( b_center, b_half_d, b_orient );

		// Check to see if the bounding volumes intersect
		if( a_obb.CheckForContact( b_obb ) )
		{
#			if !GP_RETAIL
			if( m_bShowVolumes )
			{
				SiegePos center( b_center, (*i)->GetPlacement()->GetPosition().node );
				gWorld.DrawDebugBox( center, b_orient, b_half_d, 0xffFF0000, 2.0f, true );
			}
#			endif

			// Do the damage
			ApplyDamage( Owner, (*i)->GetGoid(), damage_min, damage_max, bIgnite, t, bCalcFireResist );
		}
	}
}


bool
WorldFx::AddTargetsWithinSphere( const SiegePos &position, float radius, const GoidColl &ignore, Goid Owner, UINT32 &AddMax, def_tracker &targets )
{
	bool	bTargetAdded = false;

	if( AddMax > 0 )
	{
		// Lookup the owner
		GoHandle hOwner( Owner );

		// Use a sphere test with radius of half_diag to get possible collision candidates capable of burning
		GopColl occupants;
		gAIQuery.GetOccupantsInSphere(	position,
										radius,
										hOwner,
										&ignore,
										0,
										OF_CULL_PARTY_MEMBERS | OF_ALIVE | OF_BREAKABLES | OF_CULL_SAME_MEMBERSHIP | OF_CULL_GHOSTS | OF_CULL_INVINCIBLE,
										&occupants );

		// Check all the occupants to see if they have already been added
		for( GopColl::iterator i = occupants.begin(), occ_end = occupants.end(); i != occ_end; ++i )
		{
			bool bAddTarget = true;

			TattooTracker::TargetColl::iterator iTarget = targets->GetTargetsBegin(), targets_end = targets->GetTargetsEnd();
			for( ; iTarget != targets_end; ++iTarget )
			{
				if( (MakeGoid( (*iTarget)->GetID() ) == (*i)->GetGoid()) && (AddMax > 0) )
				{
					bAddTarget = false;
					break;
				}
			}

			if( bAddTarget )
			{
				bTargetAdded = true;

				targets->AddTarget( new GoTarget( (*i)->GetGoid() ) );

				--AddMax;

				if( AddMax == 0 )
				{
					break;
				}
			}
		}
	}

	return bTargetAdded;
}


bool
WorldFx::RegisterEffectScript( Goid object, SFxSID id, eWorldEvent type, bool xfer, const Flamethrower_params* originalParams )
{
	GoHandle hGameObject( object );

	if( hGameObject )
	{
		gGoDb.RegisterEffectScript( object, id, type, xfer, originalParams );
		return true;
	}

	return false;
}


bool
WorldFx::AddPositions( SiegePos & pos, const vector_3 & add, bool bFast, bool * pbNodeChange )
{
	if( bFast || m_bAllowUndefinedNodeGUID )
	{
		pos.pos += add;
		return true;
	}

	// add the offset
	const vector_3 offset_pos( pos.pos + add );

	// sanity check 2500 meters below zero is crazy - must have fallen through the world.
	gpassert( offset_pos.y > -2500 );

	// get the starting node
	siege::SiegeNode *pNode = gSiegeEngine.IsNodeValid( pos.node );

	// verifiy it was valid
	if( pNode == NULL )
	{
		return false;
	}

	// create a copy in case AdjustPointToTerrainPriority fails - because it will modify the position
	SiegePos adjusted_pos( offset_pos, pos.node );

	// attempt to offset position to new node - ignore failure, is ok and will happen
	gSiegeEngine.AdjustPointToTerrainPriority( pNode, adjusted_pos, siege::LF_IS_FLOOR, siege::LF_IS_WALL | siege::LF_IS_WATER, 3 );

	// new node?
	if( pos.node != adjusted_pos.node )
	{
		if( pbNodeChange )
		{
			*pbNodeChange = true;
		}

		// new_position is an illegal nodespace position
		SiegePos new_position( offset_pos, pos.node );

		// convert illegal node position to world
		pos.pos	= new_position.WorldPos();

		// convert world to node in the new node
		pos.FromWorldPos( pos.pos, adjusted_pos.node );
		return true;
	}

	// same node
	if( pbNodeChange )
	{
		*pbNodeChange = false;
	}
	pos.pos = offset_pos;

	return true;
}


void
WorldFx::ReOrientVector( const siege::database_guid &old_node, const siege::database_guid &new_node,
											vector_3 &vector )
{
	if( !(old_node == new_node) )
	{
		siege::SiegeNode* pOldNode	= gSiegeEngine.IsNodeValid( old_node );
		siege::SiegeNode* pNewNode	= gSiegeEngine.IsNodeValid( new_node );
		if( pOldNode && pNewNode )
		{
			vector = ( Transpose( pNewNode->GetCurrentOrientation() * pOldNode->GetTransposeOrientation() ) ) * vector;
		}
	}
}


void
WorldFx::GetDifferenceOrient( const siege::database_guid &old_node, const siege::database_guid &new_node, matrix_3x3 &difference )
{
	if( !(old_node == new_node) )
	{
		siege::SiegeNode* pOldNode	= gSiegeEngine.IsNodeValid( old_node );
		siege::SiegeNode* pNewNode	= gSiegeEngine.IsNodeValid( new_node );
		if( pOldNode && pNewNode )
		{
			difference = ( Transpose( pNewNode->GetCurrentOrientation() * pOldNode->GetTransposeOrientation() ) );
		}
	}
}


void
WorldFx::ClearVariables( Goid owner )
{
	SPosMap::iterator i_var = m_SiegePosVars.find( owner );

	if( i_var != m_SiegePosVars.end() )
	{
		m_SiegePosVars.erase( i_var );
	}
}


UINT32
WorldFx::AddVariable( SiegePos &position, Goid owner )
{
#	if GP_DEBUG
	GoHandle go( owner );
	gpassert( go && !go->IsLodfi() );
#	endif // GP_DEBUG

	SPosVector& vars = m_SiegePosVars[ owner ];
	vars.push_back( position );

	if ( vars.size() > 50 )
	{
		gperrorf(( "Go 0x%08X (%s) is using a lot of SiegeFx variables!\n", owner, GoHandle( owner )->GetTemplateName() ));
	}

	return ( vars.size() - 1 );
}


SiegePos&
WorldFx::GetVariable( UINT32 index, Goid owner )
{
#	if GP_DEBUG
	GoHandle go( owner );
	gpassert( go && !go->IsLodfi() );
#	endif // GP_DEBUG

	SPosMap::iterator i_var = m_SiegePosVars.find( owner );

	if( (i_var != m_SiegePosVars.end()) && (index < i_var->second.size()) )
	{
		return i_var->second[ index ];
	}

	return m_NullPosition;
}


void
WorldFx::SetVariable( UINT32 index, SiegePos &position, Goid owner )
{
#	if GP_DEBUG
	GoHandle go( owner );
	gpassert( go && !go->IsLodfi() );
#	endif // GP_DEBUG

	SPosMap::iterator i_var = m_SiegePosVars.find( owner );

	if( (i_var != m_SiegePosVars.end()) && (index < i_var->second.size()) )
	{
		i_var->second[index] = position;
	}
}


bool
WorldFx::ValidTargets( const def_tracker& targets, const char *sName )
{
	bool bValid_target = true;
	bool bValid_source= true;

	const bool target_gotarget = (Flamethrower_target::DEFAULT == targets->GetType( TattooTracker::TARGET ));
	const bool source_gotarget = (Flamethrower_target::DEFAULT == targets->GetType( TattooTracker::SOURCE ));

	Goid target = (target_gotarget) ? targets->GetID( TattooTracker::TARGET ) : GOID_INVALID;
	Goid source = (source_gotarget) ? targets->GetID( TattooTracker::SOURCE ) : GOID_INVALID;


	if( target_gotarget && (GOID_INVALID == target) )
	{
		bValid_target = false;
	}

	if( source_gotarget && (GOID_INVALID == source) )
	{
		bValid_source = false;
	}

	gpassertf( bValid_source, ("Flamethrower::RunScript '%s' using invalid source.", sName) );
	gpassertf( bValid_target, ("Flamethrower::RunScript '%s' using invalid target.", sName) );

	// Can't do anything if both targets are invalid
	if( !bValid_target || !bValid_source )
	{
		return false;
	}
	return true;
}


#if	!GP_RETAIL
void
WorldFx::DoDrawSanityCheck( void )
{
	++m_EffectsDrawn;

	if( m_EffectsDrawn > m_EffectsDrawnLimit )
	{
		gpperff(( "PERFORMANCE: DRAWING %d effects! - Ignore once!\nThis could be a level design issue so:\n\n1) Save the game\n2) Write this up in Raid as a performance issue\n3) Tell Eric or Rick\n", m_EffectsDrawn ));

		m_EffectsDrawnLimit += m_EffectsDrawn>>1;
	}
}
#endif

#if	!GP_RETAIL
void
WorldFx::DoUpdateSanityCheck( void )
{
	++m_EffectsUpdated;

	if( m_EffectsUpdated > m_EffectsUpdatedLimit )
	{
		gpperff(( "PERFORMANCE: UPDATING %d effects! - Ignore once!\nThis could be a level design issue so:\n\n1) Save the game\n2) Write this up in Raid as a performance issue\n3) Tell Eric or Rick\n", m_EffectsUpdated ));

		m_EffectsUpdatedLimit += m_EffectsUpdated>>1;
	}
}
#endif


EffectParams&
WorldFx::GetDefaultEffectParams( const char *sName )
{
	return IFlamethrower->GetDefaultEffectParams( sName );
}