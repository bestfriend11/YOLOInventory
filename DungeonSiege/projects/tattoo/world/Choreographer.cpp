/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Choreographer.cpp

	Author(s)	: Mike Biddlecombe, Bartosz Kijanka, Scott Bilas

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/

#include "Precomp_World.h"
#include "Choreographer.h"

#include "Fuel.h"
#include "SkritEngine.h"
#include "StringTool.h"
#include "FubiTraits.h"

FUBI_EXPORT_ENUM( eAnimChore, CHORE_BEGIN, CHORE_COUNT );
FUBI_EXPORT_ENUM( eAnimStance, AS_BEGIN, AS_COUNT );

/*--------------------------------------------------------------------------*/

static const char* s_ChoreStrings[] =
{
	"chore_invalid",
	"chore_none",
	"chore_error",
	"chore_default",

	"chore_walk",
	"chore_die",
	"chore_defend",
	"chore_attack",
	"chore_magic",
	"chore_fidget",

	"chore_rotate",
	"chore_open",
	"chore_close",

	"chore_misc",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_ChoreStrings ) == CHORE_COUNT );

static stringtool::EnumStringConverter s_ChoreConverter( s_ChoreStrings, CHORE_BEGIN, CHORE_END, CHORE_NONE );

const char* ToString( eAnimChore ch )
	{  return ( s_ChoreConverter.ToString( ch ) );  }
bool FromString( const char* str, eAnimChore& ch )
	{  return ( s_ChoreConverter.FromString( str, rcast <int&> ( ch ) ) );  }

eAnimChore ChoreFromString( const char* str )
{
	eAnimChore chore = CHORE_ERROR;
	FromString( str, chore );
	return ( chore );
}

eAnimChore MakeAnimChore( DWORD x ) { return scast<eAnimChore>(x); }
int MakeInt( eAnimChore x ) { return scast<int>(x); }


/*--------------------------------------------------------------------------*/

static const char* s_AnimStanceStrings[] =
{
	"as_default",
	"as_plain",
	"as_single_melee",
	"as_single_melee_and_shield",
	"as_two_handed_melee",			// two handed but NOT a sword
	"as_two_handed_sword",
	"as_staff",
	"as_bow_and_arrow",
	"as_minigun",
	"as_shield_only",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_AnimStanceStrings ) == AS_COUNT );

static stringtool::EnumStringConverter s_AnimStanceConverter( s_AnimStanceStrings, AS_BEGIN, AS_END, AS_DEFAULT );

const char* ToString( eAnimStance e )
	{  return ( s_AnimStanceConverter.ToString( e ) );  }
bool FromString( const char* str, eAnimStance& e )
	{  return ( s_AnimStanceConverter.FromString( str, rcast <int&> ( e ) ) );  }

/*--------------------------------------------------------------------------*/

const char* Chore::ANIM_SKRIT_PATH = "art\\animations\\skrits\\";

Chore :: Chore( void )
{
	m_Verb        = CHORE_NONE;
	m_lStanceMask = 0;
}

Chore :: Chore( const Chore& chore )
{
	*this = chore;
}

void Chore :: CommitCreation()
{
	if( m_AnimSkritObject )
	{
		m_AnimSkritObject->CommitCreation();
	}
}

bool Chore :: Load( FastFuelHandle hchore, const gpstring& chorePrefix )
{

	bool success = true;

// Get verb.

	if ( !FromString( hchore.GetName(), m_Verb ) )
	{
		gperrorf(( "Unrecognized Chore name '%s'\n", hchore.GetName() ));
		success = false;
	}

// Get Skrit.

	gpstring skritName;
	if( hchore.Get( "skrit", skritName ) )
	{
		skritName.insert( 0, Chore::ANIM_SKRIT_PATH );

		Skrit::CreateReq createReq( skritName );
		createReq.SetPersist( false );
		// skrits are commited for creation when loading the game right before loading lodfi objects, so if we are loading lofi
		// we need to make sure to commit our skrits.
		createReq.m_DeferConstruction = (gWorldState.GetCurrentState() == WS_LOADING_SAVE_GAME && gGoDb.IsPostLoading() == false);

		if ( !m_AnimSkritObject.CreateSkrit( createReq ) )
		{
			gperrorf(( "Failed to load/find/compile Chore Skrit\n" ));
			success = false;
		}
	}
	else
	{
		gperror( "No Skrit defined for chore!" );
		success = false;
	}

// Get animations and stances.

	// reset mask
	m_lStanceMask = 0;

	// find the animation files
	FastFuelHandle hanims = hchore.GetChildNamed( "anim_files" );

	if ( hanims )
	{
		FastFuelFindHandle fh( hanims );
		if( fh.FindFirstKeyAndValue() )
		{
			gpstring key, value;

			bool FoundQFFG = false;

			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				int fourCCKey;
				if ( !FuBi::IntFromFourCc(key.c_str(),fourCCKey) )
				{
					fourCCKey = 'BAD0' + m_vlAnimFourCC.size();
				}
				m_vlAnimFourCC.push_back(fourCCKey);
				if (fourCCKey == 'qffg')
				{
					FoundQFFG = true;
				}
			}
			if (((m_Verb == CHORE_ATTACK) || (m_Verb == CHORE_MAGIC)) && !FoundQFFG)
			{
				m_vlAnimFourCC.push_back('qffg');
			}
		}
	}

	// break up into stances
	gpstring stances;
	gpstring tempprefix;

	if ( hchore.Get( "chore_stances", stances ) )
	{
		gpstring key, value, temp;
		if (stances.same_no_case("ignore"))
		{
			LoadStance( hanims, AS_PLAIN, "" );
		}
		else
		{
			for ( DWORD i = 0 ; i < stringtool::GetNumDelimitedStrings( stances, ',' ) ; i++ )
			{
				int stance;
				if (   stringtool::GetDelimitedValue( stances, ',', i, stance )
					&& (stance >= 0) && (stance < nema::MAXIMUM_STANCES) )
				{
					tempprefix.assignf( "%s%d_", chorePrefix.c_str(), stance );
					LoadStance( hanims, (eAnimStance)stance, tempprefix );
				}
				else
				{
					// report what screwed up
					gpstring badStance;
					gpverify( stringtool::GetDelimitedValue( stances, ',', i, badStance ) );
					gperrorf(( "Not a valid stance index: '%s'\n", badStance.c_str() ));
				}
			}
		}
	}
	else
	{
		// this is an item (door, chest etc) that has no stances
		if ( !chorePrefix.empty() )
		{
			tempprefix.assignf( "%s_", chorePrefix.c_str() );
		}
		else
		{
			tempprefix.clear();
		}
		LoadStance( hanims, AS_PLAIN, tempprefix );
	}


	FastFuelHandle hanimdurs = hchore.GetChildNamed( "anim_durations" );

	if ( hanimdurs )
	{
		FastFuelFindHandle fh( hanimdurs );

		if( fh.FindFirstKeyAndValue() )
		{
			m_vfAnimDurations.reserve(nema::MAXIMUM_STANCES);

			gpstring key;
			float value;

			for (DWORD i = 0; i < nema::MAXIMUM_STANCES; ++i)
			{
				key.assignf("fs%d",i);
				if ( !hanimdurs.Get( key, value ) )
				{
					// This is an untuned parameter
					value = -1;
				}
				m_vfAnimDurations.push_back(value);
			}
		}
	}

	// done
	return ( success );
}

void Chore :: LoadStance( FastFuelHandle hanims, eAnimStance stance, const gpstring& tempPrefix )
{
	// set the bit in the stance mask
	SetStanceMask( stance );

	// load any anim files we may have
	if ( hanims )
	{
		FastFuelFindHandle fh( hanims );
		if( fh.FindFirstKeyAndValue() )
		{
			gpstring key, value, temp, temp2;

			bool FoundQFFG = false;

			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				temp.assignf( "%s%s", tempPrefix.c_str(), value.c_str() );
				m_vsAnimFiles[ stance ].push_back( temp );

				if (key.same_no_case("qffg")) 
				{
					FoundQFFG = true;
				}
			}

			// Assign any missing quick fidget to the DFF animation
			if (((m_Verb == CHORE_ATTACK) || (m_Verb == CHORE_MAGIC)) && !FoundQFFG)
			{
				temp.assignf( "%sdff", tempPrefix.c_str(), value.c_str() );
				m_vsAnimFiles[ stance ].push_back( temp );
			}
		}

	}
}

Chore& Chore :: operator = ( const Chore& chore )
{
	m_Verb            = chore.m_Verb;
	m_AnimSkritObject = chore.m_AnimSkritObject.GetHandle();
	m_lStanceMask     = chore.m_lStanceMask;
	m_vlAnimFourCC    = chore.m_vlAnimFourCC;
	m_vfAnimDurations = chore.m_vfAnimDurations;

	for ( int i = 0 ; i < nema::MAXIMUM_STANCES ; ++i )
	{
		m_vsAnimFiles[ i ] = chore.m_vsAnimFiles[ i ];
	}

	if ( m_AnimSkritObject != NULL )
	{
		gSkritEngine.AddRefObject( m_AnimSkritObject );
	}

	return ( *this );
}

/*--------------------------------------------------------------------------*/

bool ChoreDictionary :: Load( FastFuelHandle hdict )
{
	bool success = false;

// Get chore prefix.

	gpstring chorePrefix = hdict.GetString( "chore_prefix" );

// Fill out chore dictionary.

	FastFuelHandleColl hchores;
	hdict.ListChildren( hchores );

	FastFuelHandleColl::iterator i, begin = hchores.begin(), end = hchores.end();

	for ( i = begin ; i != end ; ++i )
	{
		Chore chore;

		if ( chore.Load( *i, chorePrefix ) )
		{
			if ( insert( value_type( chore.m_Verb, chore ) ).second )
			{
				success = true;
			}
			else
			{
				gperrorf(( "Duplicate chore '%s' detected at fuel '%s'\n",
						   ToString( chore.m_Verb ), (*i).GetAddress().c_str() ));
			}
		}
		else
		{
			gperrorf(( "Failure to load chore from fuel '%s'\n", (*i).GetAddress().c_str() ));
		}
	}

	return ( success );
}

/*--------------------------------------------------------------------------*/

// $$$$ check for min requirements on chore dictionary

