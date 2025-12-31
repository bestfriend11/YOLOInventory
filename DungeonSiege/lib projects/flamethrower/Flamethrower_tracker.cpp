//
//	Flamethrower_interpreter.cpp
//
//

#include "precomp_flamethrower.h"

#include "WorldFx.h"
#include "GoAspect.h"
#include "GoBody.h"
#include "flamethrower_effect_target.h"

#include "gosupport.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "FuBiTraitsimpl.h"


FUBI_DECLARE_CAST_TRAITS( Flamethrower_target::eTargetType, int );


Flamethrower_target* Flamethrower_target::CreateByType( eTargetType type, const SiegePos &position, UINT32 id )
{
	Flamethrower_target* target = NULL;

	switch ( type )
	{
		case ( DEFAULT ):
		{
			if( id == 0 )
			{
				target = new GoTarget;
			}
			else
			{
				target = new GoTarget( MakeGoid( id ) );
			}
		}
		break;

		case ( EMITTER ):
		{
			if( id == 0 )
			{
				target = new GoEmitter;
			}
			else
			{
				target = new GoEmitter( MakeGoid( id ) );
			}
		}
		break;

		case ( STATIC ):
		{
			if( position == SiegePos::INVALID )
			{
				target = new GoStatic;
			}
			else
			{
				target = new GoStatic( position );
			}
		}
		break;

		case ( EFFECT ):
		{
			target = new Flamethrower_effect_target;
		}
		break;

		default:
		{
			gpassert( 0 );	// $ should never get here
		}
	}

	return ( target );
}


bool
Flamethrower_target::Xfer( FuBi::PersistContext &persist )
{
#	if !GP_RETAIL
	bool hasGoid = false;
	if ( persist.IsSaving() && !ShouldPersist( hasGoid ) && hasGoid )
	{
		gperror( "Flamethrower_target: I'm not persisting, yet I have a goid!\n" );
	}
#	endif // !GP_RETAIL

	persist.Xfer	( "m_Type",				m_Type				);
	persist.XferHex	( "m_ID",				m_ID				);
	persist.Xfer	( "m_Position",			m_Position			);
	persist.Xfer	( "m_Orientation",		m_Orientation		);
	persist.Xfer	( "m_Direction",		m_Direction			);
	persist.Xfer	( "m_BoundingRadius",	m_BoundingRadius	);
	persist.Xfer	( "m_Offset",			m_Offset			);

	return OnXfer( persist );
}


bool
Flamethrower_target::OnXfer( FuBi::PersistContext &persist )
{
	UNREFERENCED_PARAMETER( persist );

	return true;
}


bool
Flamethrower_target::Duplicate( Flamethrower_target **pTarget )
{
#	if !GP_RETAIL
	if( IsMemoryBadFood(DWORD( *pTarget ) ) )
	{
		gpassert((!"Flamethrower_target:duplicate being fed bad food - can't eat...\n"));
		return false;
	}
#	endif

	if( OnDuplicate( pTarget ) )
	{
		(*pTarget)->m_Type				= m_Type;
		(*pTarget)->m_Position			= m_Position;
		(*pTarget)->m_Orientation		= m_Orientation;
		(*pTarget)->m_Direction			= m_Direction;
		(*pTarget)->m_BoundingRadius	= m_BoundingRadius;
		(*pTarget)->m_Offset			= m_Offset;

		return true;
	}
	return false;
}


bool
Flamethrower_target::GetPlacement( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 Orient, const gpstring &sBoneName )
{
	UNREFERENCED_PARAMETER( sBoneName );
	UNREFERENCED_PARAMETER( Orient );

	if( pPos )
	{
		(*pPos).pos += m_Orientation * m_Offset;
	}

	if( pOrient )
	{
		*pOrient = m_Orientation;
	}

	return true;
}

bool
Flamethrower_target::GetDirection( vector_3 &direction )
{
	direction = m_Direction;
	return true;
}


bool
Flamethrower_target::GetBoundingRadius( float &radius )
{
	radius = m_BoundingRadius;
	return true;
}


bool
Flamethrower_target::GetPositionOffset( vector_3 &offset )
{
	offset = m_Offset;
	return true;
}


bool
Flamethrower_target::HasPositionOffsetName( const gpstring &sName )
{
	if( (m_Type == DEFAULT) && (!sName.empty()) )
	{
		GoHandle hTarget( MakeGoid( m_ID ) );

		// Check to see if the target has a kill bone and use it if exists
		if( hTarget && hTarget->HasBody() )
		{
			int nema_bone_index = 0;

			// Check to see if the name is one that needs to be translated
			if( sName[0] == '@' )
			{
				// Get the objects translator
				const BoneTranslator &translator = hTarget->GetBody()->GetBoneTranslator();

				// init bone_index to a valid bone
				BoneTranslator::eBone bone_index = BoneTranslator::KILL_BONE;

				// convert the string to a bone translator enumeration
				BoneTranslator::FromString( sName.c_str() + 1, bone_index );

				// Now convert to the nema bone
				if( !translator.Translate( bone_index, nema_bone_index ) )
				{
					nema_bone_index = -1;
				}
			}
			else
			{
				// Don't translate
				return (hTarget->GetAspect()->GetAspectHandle()->GetBoneIndex( sName.c_str(), nema_bone_index ));
			}

			return (( nema_bone_index != 0 ) || ( nema_bone_index != -1 ));
		}
	}
	return false;
}
