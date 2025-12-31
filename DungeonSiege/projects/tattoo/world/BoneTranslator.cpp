/*==============================================================================

  BoneTranslator

  author:	Rick Saenz

  (C)opyright Gas Powered Games 1999

------------------------------------------------------------------------------*/

#include "Precomp_World.h"
#include "BoneTranslator.h"

#include "GoBody.h"
#include "Fuel.h"
#include "Go.h"
#include "Nema_Aspect.h"
#include "Go.h"
#include "GoAspect.h"
#include "GoCore.h"

/*----------------------------------------------------------------------------*/

// $$$$$$ what about @ symbols???

static const char* s_BoneStrings[] =
{
	"kill_bone",

	"weapon_bone",
	"shield_bone",

	"body_anterior",
	"body_mid",
	"body_posterior",

	"tip",
	"middle",
	"handle",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_BoneStrings ) == BoneTranslator::BONE_COUNT );

static stringtool::EnumStringConverter s_BoneConverter( s_BoneStrings, BoneTranslator::BONE_BEGIN, BoneTranslator::BONE_END );

const char* BoneTranslator :: ToString( eBone b )
	{  return ( s_BoneConverter.ToString( b ) );  }
bool BoneTranslator :: FromString( const char* str, eBone& b )
	{  return ( s_BoneConverter.FromString( str, rcast <int&> ( b ) ) );  }

/*----------------------------------------------------------------------------*/

BoneTranslator :: BoneTranslator( void )
{
	std::fill( m_DemBones, ARRAY_END( m_DemBones ), -1 );
}

bool BoneTranslator :: AddTranslation( nema::HAspect aspect, const char* nativeName, const char* nemaName )
{
	gpassert( (nativeName != NULL) && (nemaName != NULL) );

	// get nema bone index
	int nemaBoneIndex = 0;
	if( !aspect->GetBoneIndex( nemaName, nemaBoneIndex ) )
	{
		gperrorf(( "BoneTranslator: unable to translate Nema bone name '%s' (check spelling?)\n", nemaName ));
		return ( false );
	}

	// get bone name
	eBone bone;
	if ( !FromString( nativeName, bone ) )
	{
		gperrorf(( "BoneTranslator: illegal bone translator alias '%s' (check spelling?)\n", nativeName ));
		return ( false );
	}

	// don't allow overriding
	if ( m_DemBones[ bone ] != -1 )
	{
		gpwarningf(( "BoneTranslator: bone '%s' aliased multiple times\n", ToString( bone ) ));
	}

	// set it
	m_DemBones[ bone ] = nemaBoneIndex;
	return ( true );
}

bool BoneTranslator :: Load( nema::HAspect aspect, FastFuelHandle fuel )
{
	bool success = false;
	FastFuelFindHandle fh( fuel );

	if ( fh.FindFirstKeyAndValue() )
	{
		gpstring boneName, sourceName;

		while ( fh.GetNextKeyAndValue( boneName, sourceName ) )
		{
			if ( !AddTranslation( aspect, boneName, sourceName ) )
			{
				gperrorf(( "Illegal bone translation from '%s' to '%s' at '%s'\n",
						   sourceName.c_str(), boneName.c_str(), fuel.GetAddress().c_str() ));
			}
			else
			{
				success = true;
			}
		}
	}

	return ( success );
}

bool BoneTranslator :: Translate( eBone bone, int& nemaBoneIndex ) const
{
	if( m_DemBones[ bone ] != -1 )
	{
		nemaBoneIndex = m_DemBones[ bone ];
		return true;
	}
	return false;
}

bool BoneTranslator :: Translate( int nemaBoneIndex, eBone& bone ) const
{
	const int* found = std::find( m_DemBones, ARRAY_END( m_DemBones ), nemaBoneIndex );
	if ( found != ARRAY_END( m_DemBones ) )
	{
		bone = (eBone)(found - m_DemBones);
		return ( true );
	}
	return ( false );
}

bool BoneTranslator :: GetPosition( Go const * go, eBone bone, SiegePos& position, const vector_3& offset, bool adjustPointToTerrain ) const
{
	gpassert( go != NULL );

	// attempt to translate
	int nemaBone;
	if ( !Translate( bone, nemaBone ) )
	{
		return ( false );
	}

	// Precache the pointers
	const GoAspect* pGoAspect	= go->GetAspect();
	nema::Aspect* pAsp			= pGoAspect->GetAspectPtr();

	// Get the bone position
	vector_3 nemaPos( DoNotInitialize );
	pAsp->GetIndexedBonePosition( nemaBone, nemaPos );

	// Get scale
	float renderscale = pGoAspect->GetRenderScale();

	// Apply aspect scale to offset position
	vector_3 current_scale( DoNotInitialize );
	pAsp->GetCurrentScale( current_scale );
	vector_3 soffset	= (current_scale * renderscale) * offset;

	vector_3 scaled_offset(DoNotInitialize);

	Go* pParent = go->GetParent();
	if (pParent && pParent->HasAspect() )
	{
		GoAspect* pParentGoAspect	= pParent->GetAspect();
		nema::Aspect* pParentAspect	= pParentGoAspect->GetAspectPtr();

		// Get scale
		float parentrenderscale = pParentGoAspect->GetRenderScale();

		// Apply aspect scale to offset position
		vector_3 current_pscale( DoNotInitialize );
		pParentAspect->GetCurrentScale( current_pscale );
		soffset	*= (current_pscale * parentrenderscale);

		go->GetPlacement()->GetOrientation().RotateVector(scaled_offset, (renderscale * (parentrenderscale * nemaPos)) + soffset);
	}
	else
	{
		go->GetPlacement()->GetOrientation().RotateVector(scaled_offset, (renderscale * nemaPos) + soffset);
	}

	// Apply the offset
	position = go->GetPlacement()->GetPosition();
	position.pos += scaled_offset;

	if ( adjustPointToTerrain )
	{
		SiegePos adjusted(position);
		gSiegeEngine.AdjustPointToTerrain( adjusted, siege::LF_IS_ANY );

		// If original node is different from the one with the offset added on and adjusted
		// then the point is within a new node
		if( !(position.node == adjusted.node) )
		{
			// Create the new point with the addition putting it outside of it's node boundry
			// Convert it forward and back again
			position.FromWorldPos( position.WorldPos(), adjusted.node );
		}
	}

	gpassert( position.node != siege::UNDEFINED_GUID );

	// success
	return ( true );
}

/*----------------------------------------------------------------------------*/
