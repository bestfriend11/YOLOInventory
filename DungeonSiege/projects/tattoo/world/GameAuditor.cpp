/*=======================================================================================

	GameAuditor

	author:		Adam Swensen
	creation:	8/29/2000

	copyright(C) 2000 Gas Powered Games

=======================================================================================*/


#include "precomp_world.h"
#include "GameAuditor.h"
#include "FuBiTypes.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "stringtool.h"
#include "worldtime.h"
#include "victory.h"


///////////////////////////////////////////////////////////////
// eGameEvent, enum To/From string

static const char * s_GEStrings[] =
{
	"ge_player_kill_",
	"ge_player_death_",
};

COMPILER_ASSERT( ELEMENT_COUNT( s_GEStrings ) == GE_COUNT );

static stringtool::EnumStringConverter s_GEConverter( s_GEStrings, GE_BEGIN, GE_END );

bool FromString( const char* str, eGameEvent & ge )
{
	return ( s_GEConverter.FromString( str, rcast <int&> ( ge ) ) );
}

const char* ToString( eGameEvent ge )
{
	return ( s_GEConverter.ToString( ge ) );
}

///////////////////////////////////////////////////////////////


FUBI_DECLARE_SELF_TRAITS( AuditorDb::GameEntry );

bool AuditorDb::GameEntry::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer    ( "m_Key",    m_Key    );
	persist.XferHex ( "m_Tag",    m_Tag    );
	persist.Xfer    ( "m_Int",    m_Int    );
	persist.Xfer    ( "m_Bool",   m_Bool   );
	persist.Xfer    ( "m_Float",  m_Float  );
	persist.Xfer    ( "m_Double", m_Double );
	persist.Xfer    ( "m_String", m_String );

	return true;
}

bool AuditorDb::Xfer( FuBi::PersistContext& persist )
{
	persist.XferVector( "m_Entries", m_Entries );

	return true;
}


void AuditorDb::Reset()
{
	m_Entries.clear();
}


AuditorDb::GameEntry * AuditorDb::FindEntry( const char * key )
{
	for ( GameEntryColl::iterator i = m_Entries.begin(); i != m_Entries.end(); ++i )
	{
		if ( (*i).m_Key.same_no_case( key ) )
		{
			return ( i );
		}
	}
	return ( NULL );
}


AuditorDb::GameEntry * AuditorDb::FindTaggedEntry( const char * key, int tag )
{
	for ( GameEntryColl::iterator i = m_Entries.begin(); i != m_Entries.end(); ++i )
	{
		if ( ((*i).m_Tag == tag) && ((*i).m_Key.same_no_case( key )) )
		{
			return ( i );
		}
	}
	return ( NULL );
}


void AuditorDb::Set( const char * key, int value, bool bRC )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Int = value;
		pEntry->m_RC = bRC;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Int = value;
		entry.m_RC = bRC;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( const char * key, bool value, bool bRC )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Bool = value;
		pEntry->m_RC = bRC;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Bool = value;
		entry.m_RC = bRC;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( const char * key, float value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Float = value;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Float = value;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( const char * key, double value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Double = value;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Double = value;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( const char * key, const gpstring & value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_String = value;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_String = value;
		m_Entries.push_back( entry );
	}
}


bool AuditorDb::Get( const char * key, int & value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		value = pEntry->m_Int;
		return ( true );
	}
	else
	{
		value = 0;
		return ( false );
	}
}


bool AuditorDb::Get( const char * key, bool & value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		value = pEntry->m_Bool;
		return ( true );
	}
	else
	{
		value = false;
		return ( false );
	}
}


bool AuditorDb::Get( const char * key, float & value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		value = pEntry->m_Float;
		return ( true );
	}
	else
	{
		value = 0;
		return ( false );
	}
}


bool AuditorDb::Get( const char * key, double & value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		value = pEntry->m_Double;
		return ( true );
	}
	else
	{
		value = 0;
		return ( false );
	}
}


bool AuditorDb::Get( const char * key, gpstring & value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		value = pEntry->m_String;
		return ( true );
	}
	else
	{
		value.clear();
		return ( false );
	}
}


int AuditorDb::Increment( const char * key, int value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Int += value;
		return ( pEntry->m_Int );
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Int = value;
		m_Entries.push_back( entry );
		return ( value );
	}
}


float AuditorDb::IncrementFloat( const char * key, float value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Float += value;
		return ( pEntry->m_Float );
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Float = value;
		m_Entries.push_back( entry );
		return ( value );
	}
}


double AuditorDb::IncrementDouble( const char * key, double value )
{
	GameEntry *pEntry = FindEntry( key );
	if ( pEntry )
	{
		pEntry->m_Double += value;
		return ( pEntry->m_Double );
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Double = value;
		m_Entries.push_back( entry );
		return ( value );
	}
}


void AuditorDb::Set( int tag, const char * key, int value, bool bRC )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Int = value;
		pEntry->m_RC = bRC;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Int = value;
		entry.m_RC = bRC;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( int tag, const char * key, bool value, bool bRC )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Bool = value;
		pEntry->m_RC = bRC;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Bool = value;
		entry.m_RC = bRC;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( int tag, const char * key, float value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Float = value;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Float = value;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( int tag, const char * key, double value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Double = value;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Double = value;
		m_Entries.push_back( entry );
	}
}


void AuditorDb::Set( int tag, const char * key, const gpstring & value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_String = value;
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_String = value;
		m_Entries.push_back( entry );
	}
}


bool AuditorDb::Get( int tag, const char * key, int & value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		value = pEntry->m_Int;
		return ( true );
	}
	else
	{
		value = 0;
		return ( false );
	}
}


bool AuditorDb::Get( int tag, const char * key, bool & value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		value = pEntry->m_Bool;
		return ( true );
	}
	else
	{
		value = false;
		return ( false );
	}
}


bool AuditorDb::Get( int tag, const char * key, float & value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		value = pEntry->m_Float;
		return ( true );
	}
	else
	{
		value = 0;
		return ( false );
	}
}


bool AuditorDb::Get( int tag, const char * key, double & value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		value = pEntry->m_Double;
		return ( true );
	}
	else
	{
		value = 0;
		return ( false );
	}
}


bool AuditorDb::Get( int tag, const char * key, gpstring & value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		value = pEntry->m_String;
		return ( true );
	}
	else
	{
		value.clear();
		return ( false );
	}
}


int AuditorDb::Increment( int tag, const char * key, int value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Int += value;
		return ( pEntry->m_Int );
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Int = value;
		m_Entries.push_back( entry );
		return ( value );
	}
}


float AuditorDb::IncrementFloat( int tag, const char * key, float value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Float += value;
		return ( pEntry->m_Float );
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Float = value;
		m_Entries.push_back( entry );
		return ( value );
	}
}


double AuditorDb::IncrementDouble( int tag, const char * key, double value )
{
	GameEntry *pEntry = FindTaggedEntry( key, tag );
	if ( pEntry )
	{
		pEntry->m_Double += value;
		return ( pEntry->m_Double );
	}
	else
	{
		GameEntry entry;
		entry.m_Key = key;
		entry.m_Tag = tag;
		entry.m_Double = value;
		m_Entries.push_back( entry );
		return ( value );
	}
}


void AuditorDb::RemoveEntry( int tag, const char * key )
{
	for ( GameEntryColl::iterator i = m_Entries.begin(); i != m_Entries.end(); ++i )
	{
		if ( (i->m_Tag == tag) && (i->m_Key.same_no_case( key )) )
		{
			m_Entries.erase( i );
			break;
		}
	}
}


void AuditorDb::ClearEntries( int tag )
{
	for ( GameEntryColl::iterator i = m_Entries.begin(); i != m_Entries.end(); ++i )
	{
		if ( i->m_Tag == tag )
		{
			i->m_Int = 0;
			i->m_Float = 0;
		}
	}
}


///////////////////////////////////////////////////////////////


GameAuditor::GameAuditor()
{
//	AddDb( "party" );
}


GameAuditor::~GameAuditor()
{
	for ( AuditorMap::iterator i = m_Auditors.begin(); i != m_Auditors.end(); ++i )
	{
		delete i->second;
	}
	m_Auditors.clear();
}


bool GameAuditor::Xfer( FuBi::PersistContext& persist )
{
	AuditorDb::Xfer( persist );

	persist.EnterColl( "m_Auditors" );
	if ( persist.IsSaving() )
	{
		for ( AuditorMap::iterator i = m_Auditors.begin(); i != m_Auditors.end(); ++i )
		{
			if ( i->second->GetXferDb() )
			{
				persist.AdvanceCollIter();
				gpstring key( i->first );
				persist.Xfer( "_key", key );
				i->second->Xfer( persist );
			}
		}
	}
	else
	{
		while ( persist.AdvanceCollIter() )
		{
			gpstring name;
			persist.Xfer( "_key", name );
			AuditorDb *pAuditor = AddDb( name );
			pAuditor->Xfer( persist );
		}
	}
	persist.LeaveColl();     

	return true;
}


void GameAuditor::Reset()
{
	AuditorDb::Reset();

	for ( AuditorMap::iterator i = m_Auditors.begin(); i != m_Auditors.end(); ++i )
	{
		i->second->Reset();
	}
}


void GameAuditor::SSyncOnMachine( DWORD machineId )
{
	CHECK_SERVER_ONLY;

	for ( GameEntryColl::iterator i = GetEntries().begin(); i != GetEntries().end(); ++i )
	{
		if ( i->m_RC )
		{
			if ( i->m_Int != 0 )
			{
				RCSet( i->m_Tag, i->m_Key, i->m_Int, machineId );
			}
			if ( i->m_Bool != false )
			{
				RCSetBool( i->m_Tag, i->m_Key, i->m_Bool, machineId );
			}
		}
	}
}


AuditorDb * GameAuditor::AddDb( const char * name, bool xferDb )
{
	gpstring sName( name );

	AuditorMap::iterator findAuditor = m_Auditors.find( sName );

	if ( findAuditor != m_Auditors.end() )
	{
		return findAuditor->second;
	}
	else
	{
		AuditorDb *pAuditor = new AuditorDb;
		
		pAuditor->SetXferDb( xferDb );

		m_Auditors.insert( AuditorMap::value_type( sName, pAuditor ) );

		return pAuditor;
	}
}


AuditorDb * GameAuditor::FindDb( const char * name )
{
	gpstring sName( name );

	AuditorMap::iterator findAuditor = m_Auditors.find( sName );

	if ( findAuditor != m_Auditors.end() )
	{
		return findAuditor->second;
	}
	return this;
}


void GameAuditor::SIncrement( eGameEvent event, int tag, const char * key, int value )
{
	CHECK_SERVER_ONLY;
	
	gpstring name( ToString( event ) );
	name += key;

	int newValue;
	Get( tag, name, newValue );
	newValue += value;

	RCSet( tag, name, newValue );
}


void GameAuditor::RCSet( const char * key, int value, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCSet, machineId );

	Set( key, value, true );
}


void GameAuditor::RCSet( int tag, const char * key, int value, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCSet, machineId );

	Set( tag, key, value, true );
}


void GameAuditor::RCSetBool( const char * key, bool value, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCSetBool, machineId );

	Set( key, value, true );
}


void GameAuditor::RCSetBool( int tag, const char * key, bool value, DWORD machineId )
{
	FUBI_RPC_THIS_CALL( RCSetBool, machineId );

	Set( tag, key, value, true );
}


#if !GP_RETAIL

void GameAuditor::Print( const char * name )
{
	AuditorDb *pAuditor = FindDb( name );

	gpstring printLine;
	printLine.append( 90, '=' );
	printLine.append( "\n" );
	printLine.append( "Tag" );
	printLine.append( 7, ' ' );
	printLine.append( "Key" );
	printLine.append( 27, ' ' );
	printLine.append( "Values\n" );
	printLine.append( 90, '-' );
	printLine.append( "\n" );
	gpgeneric( printLine );

	for ( GameEntryColl::iterator i = pAuditor->GetEntries().begin(); i != pAuditor->GetEntries().end(); ++i )
	{
		printLine.clear();
		printLine.appendf( "%i", i->m_Tag );
		printLine.append( max_t( 1, int(10 - printLine.size()) ), ' ' );
		printLine.append( i->m_Key );
		printLine.append( max_t( 1, int(38 - printLine.size()) ), ' ' );
		printLine.appendf( "%i", i->m_Int );
		printLine.append( max_t( 1, int(46 - printLine.size()) ), ' ' );
		printLine.appendf( "%2.2f", i->m_Float );
		printLine.append( max_t( 1, int(54 - printLine.size()) ), ' ' );
		printLine.append( i->m_Bool ? "true" : "false" );
		printLine.append( max_t( 1, int(60 - printLine.size()) ), ' ' );
		printLine.appendf( "\"%s\"\n", i->m_String.c_str() );
		gpgeneric( printLine );
	}

	printLine.assign( 90, '=' );
	printLine.append( "\n" );
	gpgeneric( printLine );
}

void GameAuditor::PrintDbList()
{
	for ( AuditorMap::iterator i = m_Auditors.begin(); i != m_Auditors.end(); ++i )
	{
		gpgenericf(( "%s\n", i->first.c_str() ));
	}
}

#endif

