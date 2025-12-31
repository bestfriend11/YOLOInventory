//////////////////////////////////////////////////////////////////////////////
//
// File     :  ScidManager.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "ScidManager.h"
#include "EditorRegion.h"


SCIDManager::SCIDManager()
	: m_nextScid( SCID_INVALID )
{
}


Scid SCIDManager::GetScid()
{	
	int		i			= 0;
	bool	bContinue	= true;
	while ( bContinue ) {		
		if ( IsScidUnique( MakeScid(i + MakeInt(m_nextScid) ) ) == true ) {
			m_usedScids.insert( MakeScid(i + MakeInt(m_nextScid) ) );			
			m_nextScid = MakeScid(i + MakeInt(m_nextScid) );
			return m_nextScid;
		}
		++i;
	}		
	
	gpassert( !"Invalid Scid being generated, object will NOT BE VALID!!! This is VERY bad!" );
	return SCID_INVALID;	
}


bool SCIDManager::IsScidUnique( Scid scid )
{	
	std::set< Scid >::iterator i = m_usedScids.find( scid );
	if ( i != m_usedScids.end() ) {
		return false;
	}

	return true;	
}


void SCIDManager::CompileScidSet()
{	
	m_usedScids.clear();
	FuelHandleList::iterator i;
	FuelHandleList hlIndicies = gEditorRegion.GetRegionHandle()->ListChildBlocks( -1, "", "streamer_node_content_index" );	
	for ( i = hlIndicies.begin(); i != hlIndicies.end(); ++i ) {
		gpstring sKey;
		gpstring sValue;		
		(*i)->FindFirstKeyAndValue();
		while ( (*i)->GetNextKeyAndValue( sKey, sValue ) ) {
			int scidInt = 0;
			stringtool::Get( sValue, scidInt );
			m_usedScids.insert( MakeScid(scidInt) );
		}
	}	

	FuelHandle hRegion = gEditorRegion.GetRegionHandle()->GetChildBlock( "region" );
	if ( hRegion.IsValid() )
	{
		int range = 0;
		hRegion->Get( "scid_range", range );
		range = (range << 20 );
		m_nextScid = MakeScid( range );
	}

	if ( m_nextScid == MakeScid(0) )
	{
		m_nextScid = MakeScid(1);
	}
}


void SCIDManager::ClearScids()
{
	m_usedScids.clear();
}


void SCIDManager::SetScidRange( int scidRange )
{
	scidRange = scidRange << 20;
	m_nextScid = MakeScid( scidRange );

		if ( m_nextScid == MakeScid(0) )
	{
		m_nextScid = MakeScid(1);
	}
}