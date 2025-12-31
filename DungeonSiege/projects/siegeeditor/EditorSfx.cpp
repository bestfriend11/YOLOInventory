//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorSfx.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "PrecompEditor.h"
#include "EditorSfx.h"


void EffectSchemaDb::Init()
{
	FuelHandle hSchema( "config:editor:effect_schema" );
	if ( hSchema.IsValid() )
	{		
		FuelHandle hCommands = hSchema->GetChildBlock( "commands" );
		if ( hCommands.IsValid() )
		{
			FuelHandleList hlCommands = hCommands->ListChildBlocks( 1 ); 
			FuelHandleList::iterator i;

			for ( i = hlCommands.begin(); i != hlCommands.end(); ++i )
			{
				Command cmd;
				cmd.sName = (*i)->GetName();
				(*i)->Get( "doc", cmd.sDoc );
				
				FuelHandle hSub = (*i)->GetChildBlock( "arguments" );
				if ( hSub.IsValid() )
				{

					FuelHandleList hlArgument = hSub->ListChildBlocks( 1, "", "argument*" );
					FuelHandleList::iterator j;
					for ( j = hlArgument.begin(); j != hlArgument.end(); ++j )
					{
						Argument arg;
						(*j)->Get( "order", arg.order );
						(*j)->Get( "type", arg.sType );
						(*j)->Get( "doc", arg.sDoc );
						
						if ( arg.sType.same_no_case( "values" ) )
						{
							if ( (*j)->FindFirst( "value*" ) )
							{
								gpstring sValue;
								while ( (*j)->GetNext( sValue ) )
								{
									arg.values.push_back( sValue );
								}
							}
						}

						cmd.arguments.push_back( arg );
					}				
					
					cmd.subCommands = RecursiveLoadSubCommand( hSub );											
				}

				m_commands.push_back( cmd );
			}
		}

		FuelHandle hMacros = hSchema->GetChildBlock( "macros" );
		if ( hMacros.IsValid() )
		{
			if ( hMacros->FindFirst( "*" ) )
			{
				gpstring sValue;
				while ( hMacros->GetNext( sValue ) )
				{
					m_macros.push_back( sValue );
				}
			}
		}

		FuelHandle hParameters = hSchema->GetChildBlock( "effect_parameters" );
		if ( hParameters.IsValid() )
		{
			FuelHandleList hlEffects = hParameters->ListChildBlocks( 1 );
			FuelHandleList::iterator o;
			for ( o = hlEffects.begin(); o != hlEffects.end(); ++o )
			{
				EffectParameters eps;
				eps.sEffectName = (*o)->GetName();
				(*o)->Get( "doc", eps.sDoc );

				FuelHandleList hlParameters = (*o)->ListChildBlocks( 1 );
				FuelHandleList::iterator l;
				for ( l = hlParameters.begin(); l != hlParameters.end(); ++l )
				{
					EffectParameter ep;
					(*l)->Get( "doc", ep.sDoc );
					(*l)->Get( "type", ep.sType );
					ep.sParameter = (*l)->GetName();

					eps.parameters.push_back( ep );
				}
				
				m_effectParameters.push_back( eps );
			}			
		}
	}
}


SubCommandVec EffectSchemaDb::RecursiveLoadSubCommand( FuelHandle hSubCommand )
{
	SubCommandVec subCommands; 
	
	FuelHandleList hlAll = hSubCommand->ListChildBlocks( 1 );
	FuelHandleList::iterator k;
	for ( k = hlAll.begin(); k != hlAll.end(); ++k )
	{
		gpstring sName = (*k)->GetName();

		// Lets filter so all we do is get subcommands
		if ( sName.same_no_case( "argument*" ) )
		{
			continue;
		}
		
		SubCommand cmd;
		cmd.sName	= (*k)->GetName();
		(*k)->Get( "doc", cmd.sDoc );
		
		FuelHandleList hlArguments = (*k)->ListChildBlocks( 1, "", "argument*" );
		FuelHandleList::iterator j;
		for ( j = hlArguments.begin(); j != hlArguments.end(); ++j )
		{
			Argument arg;
			(*j)->Get( "order", arg.order );
			(*j)->Get( "type", arg.sType );
			(*j)->Get( "doc", arg.sDoc );
			
			if ( arg.sType.same_no_case( "values" ) )
			{
				if ( (*j)->FindFirst( "value*" ) )
				{
					gpstring sValue;
					while ( (*j)->GetNext( sValue ) )
					{
						arg.values.push_back( sValue );
					}
				}
			}

			cmd.arguments.push_back( arg );
		}

		cmd.subCommands = RecursiveLoadSubCommand( *k );
		subCommands.push_back( cmd );
	}

	return subCommands;
}


EditorSfx::EditorSfx()
{
	m_effectSchemaDb.Init();
}


FuelHandle EditorSfx::GetSelectedScriptHandle()
{
	FuelHandle hEffects( "world:global:effects" );
	if ( hEffects.IsValid() )
	{
		FuelHandleList hlEffects = hEffects->ListChildBlocks( -1, "", "effect_script*" );
		FuelHandleList::iterator i;
		for ( i = hlEffects.begin(); i != hlEffects.end(); ++i )
		{
			gpstring sName;
			(*i)->Get( "name", sName );
			if ( sName.same_no_case( GetSelectedScript() ) )
			{					
				return (*i);
			}
		}
	}

	return FuelHandle();
}


bool EditorSfx::IsEffect( gpstring sEffect )
{	
	EffectParametersVec effects;
	EffectParametersVec::iterator iEffect;
	m_effectSchemaDb.GetEffectParameters( effects );
	for ( iEffect = effects.begin(); iEffect != effects.end(); ++iEffect )
	{
		if ( (*iEffect).sEffectName.same_no_case( sEffect ) )
		{
			return true;
		}
	}
	
	return false;
}
