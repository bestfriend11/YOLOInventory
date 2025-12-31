//////////////////////////////////////////////////////////////////////////////
//
// File     :  KeyMapper.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "PrecompEditor.h"
#include "KeyMapper.h"
#include "resource.h"
#include "Dialog_Keyboard_Shortcuts.h"
#include "FileSysUtils.h"
#include "Config.h"


void KeyMapper::LoadKeyMappings()
{
	FuelDB * pKeysDb = gFuelSys.AddTextDb( "key_mappings" );
	gpstring sDir = gConfig.ResolvePathVars( "%se_user_path%" );
	pKeysDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, sDir, sDir );
	FuelHandle hKeys( "::key_mappings:key_mappings" );
	if ( hKeys )
	{
		FuelHandleList hlKeys = hKeys->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlKeys.begin(); i != hlKeys.end(); ++i )
		{
			int cmd = 0, qualifiers = 0, key = 0;
			(*i)->Get( "command", cmd );
			(*i)->Get( "qualifiers", qualifiers );
			(*i)->Get( "key", key );
			ACCEL newKey;
			newKey.cmd		= cmd;
			newKey.fVirt	= qualifiers;
			newKey.key		= key;
			m_key_mappings.push_back( newKey );
		}
	}
	else
	{
		ACCEL *pKeys = 0;
		HACCEL hAccel = LoadAccelerators( NULL, MAKEINTRESOURCE(IDR_MAINFRAME) );
  		int num_entries = CopyAcceleratorTable( hAccel, NULL, 0 );
 		pKeys = (ACCEL *)malloc( sizeof( ACCEL ) * num_entries );
  		int num_copied = CopyAcceleratorTable( hAccel, pKeys, num_entries );
  		gpassert( num_entries == num_copied );
  			
  		for ( int i = 0; i != num_copied; ++i ) 
		{
			pKeys[i].fVirt ^= FNOINVERT;		
  			m_key_mappings.push_back( pKeys[i] );  			
		}

		free( pKeys );
	}

	ACCEL *pNewKeys = ReloadCurrentKeyMappings();
	free( pNewKeys );
}


void KeyMapper::ResetMappings()
{
	m_key_mappings.clear();
	HACCEL hAccel = LoadAccelerators( NULL, MAKEINTRESOURCE(IDR_MAINFRAME) );

	ACCEL *pKeys = 0;
	int num_entries = CopyAcceleratorTable( hAccel, NULL, 0 );
 	pKeys = (ACCEL *)malloc( sizeof( ACCEL ) * num_entries );
  	int num_copied = CopyAcceleratorTable( hAccel, pKeys, num_entries );
  	gpassert( num_entries == num_copied );
  		
  	for ( int i = 0; i != num_copied; ++i ) 
	{
		pKeys[i].fVirt ^= FNOINVERT;		
  		m_key_mappings.push_back( pKeys[i] );  			
	}

	free( pKeys );
	gEditor.SetAccelerator( hAccel );
}


void KeyMapper::SaveKeyMappings()
{	
	FuelHandle hEditor( "::key_mappings:root" );
	if ( hEditor )
	{
		// Delete the old preferences
		FuelHandle hKeys = hEditor->GetChildBlock( "key_mappings" );
		if ( hKeys )
		{
			hEditor->DestroyChildBlock( hKeys );
		}

		// Create the new preferences
		hKeys = hEditor->CreateChildBlock( "key_mappings", "key_mappings.gas" );

		if ( hKeys )
		{			
			std::vector< ACCEL >::iterator i;
			for ( i = m_key_mappings.begin(); i != m_key_mappings.end(); ++i )
			{	
				FuelHandle hKey = hKeys->CreateChildBlock( "hotkey*" );
				hKey->Set( "command", (int)(*i).cmd );
				hKey->Set( "qualifiers", (int)(*i).fVirt );
				hKey->Set( "key", (int)(*i).key );
			}		
		}

		hEditor->GetDB()->SaveChanges();
	}
}


ACCEL * KeyMapper::ReloadCurrentKeyMappings()
{
	ACCEL * pKeys = ( ACCEL * )malloc( sizeof( ACCEL ) * m_key_mappings.size() );
	std::vector< ACCEL >::iterator i;
	int index = 0;
	for ( i = m_key_mappings.begin(); i != m_key_mappings.end(); ++i ) 
	{
		pKeys[index++] = (*i);
	}

	HACCEL hAccel; 
	if ( m_key_mappings.size() != 0 ) 
	{
		hAccel = CreateAcceleratorTable( pKeys, m_key_mappings.size() );
	}
	

	gEditor.SetAccelerator( hAccel );
	return pKeys;
}


ACCEL KeyMapper::GetKeyAssignment( WORD id )
{
	std::vector< ACCEL >::iterator i;
	for ( i = m_key_mappings.begin(); i != m_key_mappings.end(); ++i ) {
		if ( (*i).cmd == id ) {
			return (*i);
		}
	}

	ACCEL blank;
	blank.cmd = 0;
	blank.fVirt = 0;
	blank.key = 0;
	return blank;
}


void KeyMapper::InsertKeyAssignment( ACCEL key_assignment ) 
{
	std::vector< ACCEL >::iterator i;
	for ( i = m_key_mappings.begin(); i != m_key_mappings.end(); ++i ) 
	{
		if ( ((*i).key == key_assignment.key) && ((*i).fVirt == key_assignment.fVirt) ) 
		{
			if ( key_assignment.cmd != (*i).cmd ) 
			{
				gpstring sMessage;
				gpstring sCommand = GetCommandName( (*i).cmd );
				if ( sCommand.empty() )
				{
					sMessage.assignf( "This key setup is currently mapped to a system operation, please choose a different key." );
					MessageBoxEx( gEditor.GetHWnd(), sMessage.c_str(), "Key Mapping", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
				sMessage.assignf( "This key is already used by command: %s.  Do you wish to continue?", sCommand.c_str() );
				int result = MessageBoxEx( gEditor.GetHWnd(), sMessage.c_str(), "Key Mapping", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
				if ( result == IDYES )
				{
					gKeyMapper.RemoveKeyAssignment( (*i).cmd );					
					break;
				}
			}
			return;
		}
	}
	
	for ( i = m_key_mappings.begin(); i != m_key_mappings.end(); ++i ) 
	{
		if ( key_assignment.cmd == (*i).cmd ) 
		{
			(*i).key = key_assignment.key;
			(*i).fVirt = key_assignment.fVirt;
			ReloadCurrentKeyMappings();
			return;
		}
	}

	m_key_mappings.push_back( key_assignment );
	ReloadCurrentKeyMappings();
}


void KeyMapper::RemoveKeyAssignment( WORD id )
{
	std::vector< ACCEL >::iterator i;
	for ( i = m_key_mappings.begin(); i != m_key_mappings.end(); ++i ) {
		if ( (*i).cmd == id ) {
			m_key_mappings.erase( i );
			return;
		}
	}
}