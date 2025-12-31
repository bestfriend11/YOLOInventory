/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: Trigger_sys.h

	Author(s)	: Rick Saenz, Adam Swensen, biddle

	Purpose		:

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/
#include "precomp_world.h"
#include "trigger_sys.h"
#include <algorithm>

#include "trigger_conditions.h"
#include "trigger_actions.h"
#include "trigger_groups.h"

#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "GoSupport.h"
#include "MCP.h"
#include "Services.h"
#include "worldtime.h"

#include "kerneltool.h"

// debugging includes
#include "appmodule.h"


FUBI_REPLACE_NAME("triggerTriggerSys",TriggerSys);

FUBI_DECLARE_SELF_TRAITS( trigger::Truid );
FUBI_DECLARE_SELF_TRAITS( trigger::Scuid );
FUBI_DECLARE_SELF_TRAITS( trigger::BoundaryCrossing );
FUBI_DECLARE_PAIR_TRAITS( trigger::SubGroupInfo );

//FUBI_DECLARE_CAST_TRAITS( trigger::TrigBits, DWORD );

FUBI_DECLARE_XFER_TRAITS( trigger::Condition* )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{
		gpstring name;
		if ( persist.IsSaving() )
		{
			name = obj->GetName();
		}

		persist.Xfer( "_name", name );

		if ( persist.IsRestoring() )
		{
			obj = gTriggerSys.NewCondition( name );
		}

		return ( obj->Xfer( persist ) );
	}
};


FUBI_DECLARE_CAST_TRAITS( trigger::ActionInfo::eCallType, DWORD );
//FUBI_DECLARE_CAST_TRAITS( FADETYPE, DWORD );

FUBI_DECLARE_XFER_TRAITS( trigger::ActionInfo * )
{
	static bool XferDef( PersistContext& persist, eXfer /*xfer*/, const char* /*key*/, Type& obj )
	{

		if ( persist.IsRestoring() )
		{
			obj = new trigger::ActionInfo;
		}

		return ( obj->Xfer( persist ) );
	}
};

using namespace kerneltool;

namespace trigger
{

////////////////////////////////////////////////////////////////////////////////////

// Party Action helpers

bool ActionInfo::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer(		"m_Time"	 , m_Time	  );
	persist.Xfer(		"m_CallType" , m_CallType );
	persist.XferVector( "m_Members"	 , m_Members  );
	
	if (m_CallType == CT_MOOD_CHANGE)
	{
		persist.Xfer( "m_MoodName", m_MoodName  );
	}
	else
	{
		persist.Xfer( "m_Region"	, m_Region	);
		persist.Xfer( "m_Section"	, m_Section	);
		persist.Xfer( "m_Level"		, m_Level	);
		persist.Xfer( "m_Object"	, m_Object	);
		persist.Xfer( "m_FT"		, m_FT		);
	}

	return ( true );
}

void ActionInfo::XferForSync( FuBi::BitPacker &packer )
{
	packer.XferRaw( m_Time );
	packer.XferRaw( m_CallType, FUBI_MAX_ENUM_BITS ( ActionInfo::CT_ ) );

	int sz;
	if (packer.IsSaving())
	{
		sz = m_Members.size();
		packer.XferCount( sz );
		GoidColl::const_iterator i, ibegin = m_Members.begin(), iend = m_Members.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			DWORD xfergoid = (DWORD)(*i);
			packer.XferRaw( xfergoid );
		}
	}
	else
	{
		packer.XferCount( sz );
		while (sz>0)
		{
			DWORD newbie;
			packer.XferRaw( newbie );
			m_Members.push_back( (Goid)newbie );
			--sz;
		}
	}

	if (m_CallType == CT_MOOD_CHANGE)
	{
		packer.XferString( m_MoodName );
	}
	else
	{
		packer.XferRaw( m_Region );
		packer.XferRaw( m_Section );
		packer.XferRaw( m_Level );
		packer.XferRaw( m_Object );
		packer.XferRaw( m_FT, FUBI_MAX_ENUM_BITS ( FT_ ) );
	}
	
}

////////////////////////////////////////////////////////////////////////////////////

// Trigger identification

bool Truid::Xfer ( FuBi::PersistContext& persist )
{
	persist.XferHex( "m_OwnGoid", m_OwnGoid );
	persist.XferHex( "m_TrigNum", m_TrigNum );
	return true;
}

bool Scuid::Xfer ( FuBi::PersistContext& persist )
{
	persist.XferHex( "m_OwnScid", m_OwnScid );
	persist.XferHex( "m_TrigNum", m_TrigNum );
	return true;
}

// Trigger container


Storage::Storage( WORD InitialTrigNum, Go* owner )
{
	m_Owner = owner;
	m_TrigNumAssigned = InitialTrigNum;
	m_UpdateNesting = 0;
	m_CanSelfDestruct = true;
}


Storage::Storage( const Storage& other,  WORD InitialTrigNum, Go* newOwner )
{
	m_Owner = newOwner;
	gpassert ( (InitialTrigNum & 0x8000) == (other.m_TrigNumAssigned & 0x8000) );
	m_TrigNumAssigned = (WORD)(InitialTrigNum | (other.m_TrigNumAssigned & 0x8000));
	m_UpdateNesting = 0;
	m_CanSelfDestruct = true;

	TriggerColl::const_iterator i, begin = other.m_Triggers.begin();
	for ( i = begin ; i != other.m_Triggers.end() ; ++i )
	{
		Add_Trigger( new Trigger( **i ) );
	}
}


Storage::~Storage()
{
	gpassert( m_UpdateNesting == 0 );

	TriggerColl::iterator iTrigger = m_Triggers.begin();
	for ( ;iTrigger != m_Triggers.end(); ++iTrigger )
	{
		delete (*iTrigger);
	}
	m_Triggers.clear();
}

void
Storage::HandleMessage( const WorldMessage& message )
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::HandleMessage", SP_TRIGGER_MISC );

	++m_UpdateNesting;

	TriggerColl::iterator iTrigger = m_Triggers.begin();
	for ( ; iTrigger != m_Triggers.end() ; ++iTrigger )
	{
		if ( (*iTrigger)->IsClientTrigger() || ::IsServerLocal() )
		{
			(*iTrigger)->HandleMessage( message );
		}
	}

	--m_UpdateNesting;
}


bool
Storage::CommitDeleteRequests()
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::CommitDeleteRequests", SP_TRIGGER_MISC );

	// don't delete anything while we're processing nested updates
	if ( m_UpdateNesting != 0 )
	{
		return ( true );
	}
	else
	{
		TriggerColl::iterator iTrigger = m_Triggers.begin();
		while ( iTrigger != m_Triggers.end() )
		{
			if ( (*iTrigger)->GetMarkedForDeletion() )
			{
				(*iTrigger)->PreserveInTrigBits();
				iTrigger = Remove_Trigger( *iTrigger );
			}
			else
			{
				++iTrigger;
			}
		}

		return ( Size() > 0 );
	}
}

void
Storage::Update( const double& CurrentTime )
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::Update", SP_TRIGGER_MISC );

	TriggerColl::iterator iTrigger = m_Triggers.begin();

	for(; iTrigger != m_Triggers.end(); ++iTrigger )
	{
		if ( !(*iTrigger)->IsClientTrigger() && !gServer.IsLocal() )
		{
			continue;
		}

		// 'Pure' messaging triggers are updated right away by the message handler method
		// --- UNLESS they have delayed actions pending, in which case they get updated twice
		if (!(*iTrigger)->GetIsPureMessageHandler() || (*iTrigger)->GetHasDelayedActions() )
		{
			(*iTrigger)->Update( CurrentTime );
		}
	}

}


bool
Storage::Load( FastFuelHandle fuel, bool IsTemplateStorage )
{

	GPPROFILERSAMPLE( "0Trigger :: Storage::Load", SP_TRIGGER_MISC );

	// don't load over existing triggers
	gpassert( m_Triggers.empty() );

	// Each type of storage has its own assigned range
	m_TrigNumAssigned = (WORD)((IsTemplateStorage) ? 0x0000 : 0x8000);

	bool triggers_loaded = false;
	Trigger * pTrigger = NULL;

	gpstring sCondition, sAction;

	FastFuelHandleColl trigger_list;
	fuel.ListChildren( trigger_list );

	for ( FastFuelHandleColl::iterator i = trigger_list.begin(); i != trigger_list.end(); ++i )
	{
		pTrigger = new Trigger;

		bool is_active = true;
		bool single_shot = false;
		bool no_trig_bits = false;
		bool can_self_destruct = true;
		bool flip_flop = false;
		float reset_duration = 0;
		float delay = 0;
		gpstring occupantsGroup;

		// auto delete if bad
		std::auto_ptr <Trigger> autoTrigger( pTrigger );

		// Set trigger parameters
		pTrigger->SetIsSingle( i->GetBool( "single_player", true ) );
		pTrigger->SetIsMulti( i->GetBool( "multi_player", true ) );

		if ( !gGoDb.IsEditMode() &&
			 ((gWorld.IsSinglePlayer() && !pTrigger->GetIsSingle()) ||
			 (gWorld.IsMultiPlayer() && !pTrigger->GetIsMulti())) )
		{
			// don't load if the trigger isn't set for this mode of play.
			continue;
		}

		if( (*i).Get( "start_active", is_active ) )
		{
			pTrigger->SetInitiallyActive( is_active );
			pTrigger->SetRequestedActive( is_active );
		}
		else
		{
			pTrigger->SetInitiallyActive( true );
			pTrigger->SetRequestedActive( true );
		}

		if( (*i).Get( "delay", delay ) )
		{
			pTrigger->SetDelay( delay );
		}

		if( (*i).Get( "single_shot", single_shot ) )
		{
			pTrigger->SetIsOneShot( single_shot );
		}

		if( (*i).Get( "no_trig_bits", no_trig_bits ) )
		{
			pTrigger->SetNoTrigBits( no_trig_bits );
		}
		else if ( IsTemplateStorage )
		{
			// convention: template triggers by default do not use trigbits
			pTrigger->SetNoTrigBits( true );
		}

		if( (*i).Get( "can_self_destruct", can_self_destruct ) )
		{
			pTrigger->SetCanSelfDestruct( can_self_destruct );
		}

		if( (*i).Get( "reset_duration", reset_duration ) )
		{
			pTrigger->SetResetDuration( reset_duration );
		}

		if( (*i).Get( "flip_flop", flip_flop ) )
		{
			pTrigger->SetIsFlipFlop( flip_flop ); // $$ todo: doc this
		}

		if( (*i).Get( "occupants_group", occupantsGroup ) )
		{
			pTrigger->SetGroupName(occupantsGroup);
		}

		// Add any defined conditions to the trigger

		FastFuelFindHandle fh( *i );
		fh.FindFirstValue( "condition*" );

		std::vector<Condition*> to_add_list;

		while( fh.GetNextValue( sCondition ) )
		{
			const UINT32	condition_offset0 = sCondition.find( "(" );
			const UINT32	condition_offset1 = sCondition.find( ")", condition_offset0 );

			if( (condition_offset0 == gpstring::npos) || (condition_offset1 == gpstring::npos) )
			{
				gperrorf(("Trigger condition formatted without parenthesis at location %s", fuel.GetAddress().c_str() ));
				continue;
			}

			gpstring conditionName = sCondition.substr( 0, condition_offset0 );
			gpstring sCondition_params = sCondition.substr( condition_offset0 + 1, condition_offset1 - condition_offset0 - 1 );

			Params condition_params(0);	// Use 0 for now, paramID is set only if condition is added

			// Get condition group
			UINT32 groupsOffset = sCondition.find( "group(", condition_offset1 );
			if ( groupsOffset != gpstring::npos )
			{
				UINT32 groupsLeft	= sCondition.find( "(", groupsOffset );
				UINT32 groupsRight	= sCondition.find( ")", groupsLeft );

				if ( (groupsLeft == gpstring::npos) || (groupsRight == gpstring::npos) )
				{
					gperrorf(( "Trigger condition group formatted without parenthesis at location %s", fuel.GetAddress().c_str() ));
				}
				else
				{
					stringtool::Get( sCondition.substr( groupsLeft + 1, groupsRight - groupsLeft - 1 ), condition_params.SubGroup );
				}
			}

			// Get doc
#if !GP_RETAIL
			UINT32 docOffset = sCondition.find( "doc(", condition_offset1 );
			if ( docOffset != gpstring::npos )
			{
				UINT32 docLeft = sCondition.find( "\"", docOffset );
				UINT32 docRight = sCondition.find( "\"", docLeft + 1 );
				if ( (docLeft != gpstring::npos) && (docRight != gpstring::npos) )
				{
					condition_params.Doc = sCondition.substr( docLeft + 1, docRight - docLeft - 1 );
				}
			}
#endif

//
			
			Condition* pCondition = gTriggerSys.NewCondition(conditionName);

			if (pCondition)
			{
				// Initialize parameters
				Parameter ParameterList;
				pCondition->FetchParameterList(ParameterList);
				for ( unsigned int i = 0; i != ParameterList.Size(); ++i )
				{
					Parameter::format *pParamFormat = NULL;
					ParameterList.Get( i, &pParamFormat );

					if ( pParamFormat->isString )
					{
						condition_params.strings.push_back( pParamFormat->s_default );
					}
					else if ( pParamFormat->isFloat )
					{
						condition_params.floats.push_back( pParamFormat->f_default );
					}
					else if ( pParamFormat->isInt || pParamFormat->isGoid || pParamFormat->isBool )
					{
						condition_params.ints.push_back( pParamFormat->i_default );
					}
				}

				int string_index = 0;
				int float_index = 0;
				int int_index = 0;

				// Load condition parameters
				gpstring sParameter;
				UINT32 thisPos = 0, nextPos = 0;
				int param = 0;

				while ( thisPos < sCondition_params.size() )
				{
					Parameter::format *pParamFormat = NULL;
					ParameterList.Get( param, &pParamFormat );
					if ( pParamFormat == NULL )
					{
						gpassertf( 0, ("Found trigger condition with more parameters than specified in the format.  Location: %s", fuel.GetAddress().c_str()) );
						break;
					}

					if ( pParamFormat->isString )
					{
						thisPos = sCondition_params.find( "\"", thisPos );
						nextPos = sCondition_params.find( "\"", thisPos + 1 );
						if ( (thisPos == gpstring::npos) || (nextPos == gpstring::npos) )
						{
							gpassertf( 0, ("Found a trigger condition string parameter formatted without quotes at location %s", fuel.GetAddress().c_str()) );
						}
						else
						{
							condition_params.strings[ string_index++ ] = sCondition_params.mid( thisPos + 1, nextPos - thisPos - 1 );
						}
					}
					else
					{
						nextPos = sCondition_params.find( ",", thisPos + 1 );
						if ( nextPos == gpstring::npos )
						{
							nextPos = sCondition_params.size();
						}
						sParameter = sCondition_params.mid( thisPos, nextPos - thisPos );
						stringtool::RemoveBorderingWhiteSpace( sParameter );

						if ( pParamFormat->isInt || pParamFormat->isGoid )
						{
							int value = 0;
							stringtool::Get( sParameter, value );
							condition_params.ints[ int_index++ ] = value;
						}
						else if ( pParamFormat->isBool )
						{
							bool value = false;
							stringtool::Get( sParameter, value );
							condition_params.ints[ int_index++ ] = (int)value;
						}
						else if ( pParamFormat->isFloat )
						{
							float value = 0.0f;
							stringtool::Get( sParameter, value );
							condition_params.floats[ float_index++ ] = value;
						}
					}

					thisPos = sCondition_params.find( ",", nextPos );
					if ( thisPos == gpstring::npos )
					{
						break;
					}
					++thisPos;
					++param;
				}

				if (pCondition->StoreParameterValues(condition_params))
				{
					DWORD sg = condition_params.SubGroup;
//					pTrigger->AddSubGroup(sg);
					pCondition->SetSubGroup(sg);

					// Collect all the new conditions to add
					to_add_list.push_back(pCondition);

				}
				else
				{
					gperrorf(("TRIGGER_SYS: LOAD failed to StoreParameterValues in [%s] @ %s ", pCondition->GetName(), fuel.GetAddress().c_str()));
					delete pCondition;
				}

			}
		}

		// Add any defined actions to the trigger
		FastFuelFindHandle fa( *i );
		fa.FindFirstValue( "action*" );

		while( fa.GetNextValue( sAction ) )
		{
			// Note as a parameter if this action should be applied when the conditional evaluation is false
			bool bWhen_False = false;

			UINT32	start_offset = sAction.find( "when_false", 0, 10 );

			if( gpstring::npos == start_offset )
			{
				start_offset = 0;
			}
			else
			{
				bWhen_False = true;
				start_offset = 11;
			}

			const UINT32 action_offset0 = sAction.find( "(" );

			UINT32 findCloseParens = gpstring::npos;
			if(	action_offset0 != gpstring::npos )
			{
				int openParens = 1;
				UINT32 currentPos = action_offset0 + 1;

				while ( openParens > 0 )
				{
					UINT32 nextOpen = sAction.find( "(", currentPos );
					UINT32 nextClose = sAction.find( ")", currentPos );
					if ( ((nextClose < nextOpen) || (nextOpen == gpstring::npos)) && (nextClose != gpstring::npos) )
					{
						--openParens;
					}
					else if ( (nextOpen < nextClose) && (nextOpen != gpstring::npos) )
					{
						++openParens;
					}
					else if ( nextClose == gpstring::npos )
					{
						break;
					}

					if ( openParens == 0 )
					{
						findCloseParens = nextClose;
					}

					currentPos = min_t( nextOpen, nextClose ) + 1;
				}
			}

			const UINT32 action_offset1 = findCloseParens;

			if(	(action_offset0 == gpstring::npos) || (action_offset1 == gpstring::npos) )
			{
				gperrorf(( "Trigger action formatted without parenthesis at location %s",
							fuel.GetAddress().c_str() ));
				continue;
			}

			gpstring actionName = sAction.substr( start_offset, action_offset0 - start_offset );
			gpstring sAction_params = sAction.substr( action_offset0 + 1, action_offset1 - action_offset0 - 1 );

			Params action_params(pTrigger->GetNextParamID());
			
			action_params.bWhenFalse = bWhen_False;

			// Get action delay
			UINT32 delayOffset = sAction.find( "delay(", action_offset1 );
			if ( delayOffset != gpstring::npos )
			{
				UINT32 delayLeft = sAction.find( "(", delayOffset );
				UINT32 delayRight = sAction.find( ")", delayLeft );
				if ( (delayLeft == gpstring::npos) || (delayRight == gpstring::npos) )
				{
					gperrorf(( "Trigger action delay formatted without parenthesis at location %s",
								fuel.GetAddress().c_str() ));
				}
				else
				{
					stringtool::Get( sAction.substr( delayLeft + 1, delayRight - delayLeft - 1 ), action_params.Delay );
				}
			}

			// Get action group
			UINT32 groupsOffset = sAction.find( "group(", action_offset1 );
			if ( groupsOffset != gpstring::npos )
			{
				UINT32 groupsLeft = sAction.find( "(", groupsOffset );
				UINT32 groupsRight = sAction.find( ")", groupsLeft );
				if ( (groupsLeft == gpstring::npos) || (groupsRight == gpstring::npos) )
				{
					gperrorf(( "Trigger action group formatted without parenthesis at location %s",
								fuel.GetAddress().c_str() ));
				}
				else
				{
					stringtool::Get( sAction.substr( groupsLeft + 1, groupsRight - groupsLeft - 1 ), action_params.SubGroup );
				}
			}

			// Get doc
#if !GP_RETAIL
			UINT32 docOffset = sAction.find( "doc(", action_offset1 );
			if ( docOffset != gpstring::npos )
			{
				UINT32 docLeft = sAction.find( "\"", docOffset );
				UINT32 docRight = sAction.find( "\"", docLeft + 1 );
				if ( (docLeft != gpstring::npos) && (docRight != gpstring::npos) )
				{
					action_params.Doc = sAction.substr( docLeft + 1, docRight - docLeft - 1 );
				}
			}
#endif

			// Initialize Parameters
			{
				Parameter *pFormat = gTriggerSys.GetActionFormat( actionName );
				if ( pFormat )
				{
					for ( unsigned int i = 0; i != pFormat->Size(); ++i )
					{
						Parameter::format *pParamFormat = 0;
						pFormat->Get( i, &pParamFormat );

						if ( pParamFormat->isFloat ) {
							action_params.floats.push_back( pParamFormat->f_default );
						}
						else if ( pParamFormat->isInt || pParamFormat->isGoid ) {
							action_params.ints.push_back( pParamFormat->i_default );
						}
						else if ( pParamFormat->isString ) {
							action_params.strings.push_back( pParamFormat->s_default );
						}
					}
				}
			}

			int string_index = 0;
			int float_index = 0;
			int int_index = 0;

			// Load action parameters
			gpstring sParameter;
			UINT32 thisPos = 0, nextPos = 0;

			while ( (nextPos != gpstring::npos) && (thisPos < sAction_params.size()) )
			{
				if ( (sAction_params[thisPos] == ' ') || (sAction_params[thisPos] == '\t') )
				{
					++thisPos;
					continue;
				}
				else if ( sAction_params[thisPos] == '"' )
				{
					nextPos = sAction_params.find( '"', thisPos + 1 );
					if ( nextPos != gpstring::npos )
					{
						sParameter = sAction_params.mid( thisPos + 1, nextPos - thisPos - 1 );
						if (string_index < (int)action_params.strings.size())
						{
							action_params.strings[string_index++] = sParameter;
						}
					}
					nextPos = sAction_params.find( ',', nextPos );
				}
				else
				{
					nextPos = sAction_params.find( ',', thisPos + 1 );
					if ( nextPos == gpstring::npos )
					{
						sParameter = sAction_params.mid( thisPos, sAction_params.size() - thisPos );
					}
					else
					{
						sParameter = sAction_params.mid( thisPos, nextPos - thisPos );
					}

					// Look for float value 
					// must NOT be a hexval or FourCC, and have either a decimal or 'f' 
					if ( (sParameter.find( '\'' ) == gpstring::npos) && 
						 (sParameter.find( 'x' ) == gpstring::npos) && 
						 ((sParameter.find( 'f' ) != gpstring::npos) ||
						  (sParameter.find( '.' ) != gpstring::npos))
						)
					{
						float value = 0;
						stringtool::Get( sParameter, value );
						if ( float_index < (int)action_params.floats.size() )
						{
							action_params.floats[float_index++] = value;
						}
					}
					else
					{
						int value = 0;
						stringtool::Get( sParameter, value );
						if ( int_index < (int)action_params.ints.size() )
						{
							action_params.ints[int_index++] = value;
						}
					}
				}

				thisPos = nextPos + 1;
			}


			Action* pAction = gTriggerSys.NewAction( actionName );

			pTrigger->AddAction( action_params.ParamID, pAction, action_params);
		}

		// Now that we have created all the actions , add all the condtions in so that 
		// the message handling conditions are processed first

		// Update the trigger flags based on the Actions we just loaded,
		// this will cause the ClientTriggerFlag to be set correctly
		pTrigger->UpdateTriggerFlags();

		if (to_add_list.empty())
		{
			gperrorf(("TRIGGER_SYS: ERROR failed load any conditions for trigger @ %s",fuel.GetAddress().c_str()));
		}

		std::vector<Condition*>::iterator addit = to_add_list.begin();

		for (addit = to_add_list.begin(); addit != to_add_list.end(); ++addit)
		{
			// Correct any wonky settings (if possible!)
			if (pTrigger->GetGroupName().empty())
			{
				if (pTrigger->GetActions().empty())
				{
					gpwarningf(("TRIGGER_SYS: WARNING failed load any actions for ungrouped trigger @ %s",fuel.GetAddress().c_str()));
				}
			}
			else
			{
				// Leave triggers that are named as members of a group ALONE
			}
		}

		// Add the message handling conditions
		for (addit = to_add_list.begin(); addit != to_add_list.end();)
		{
			if ((*addit)->IsMessageHandler())
			{
				if (single_shot)
				{
					if (!(*addit)->HasOneShotTest())
					{
						(*addit)->ConvertToOneShotTest();
						//(*addit)->AppendDocs("[*one-shot check (trigger is one-shot)*]");
					}
				}
				if (pTrigger->AddCondition(pTrigger->GetNextParamID(),(*addit)))
				{
					pTrigger->AddSubGroup((*addit)->GetSubGroup());
				}
				else
				{
					gperrorf(("TRIGGER_SYS: LOAD failed to add condition [%s] @ %s", (*addit)->GetName(),fuel.GetAddress().c_str()));
					delete (*addit);
				}

				addit = to_add_list.erase(addit);
			}
			else
			{
				++addit;
			}
		}

		// Add the remaining conditions
		for (addit = to_add_list.begin(); addit != to_add_list.end(); ++addit)
		{
			if (pTrigger->GetIsSubGroupMessageHandler((*addit)->GetSubGroup()))
			{
				if (!(*addit)->HasWaitForMessageTest())
				{
					(*addit)->ConvertToWaitForMessageTest();
					//(*addit)->AppendDocs("[*message checker (trigger is non-pure )*]");
				}
			} 
			if (single_shot)
			{
				if (!pTrigger->GetIsSubGroupMessageHandler((*addit)->GetSubGroup()))
				{
					if (to_add_list.size()>1)
					{
						if (!(*addit)->HasWhileInsideTest())
						{
							(*addit)->ConvertToWhileInsideTest();
							//(*addit)->AppendDocs("[*multiple-boundary (trigger is one-shot)*]");
						}
					}
					else
					{
						if (!(*addit)->HasOneShotTest())
						{
							(*addit)->ConvertToOneShotTest();
							//(*addit)->AppendDocs("[*one-shot check (trigger is one-shot)*]");
						}
					}
				}
			}
			if (pTrigger->AddCondition(pTrigger->GetNextParamID(),(*addit)))
			{
				pTrigger->AddSubGroup((*addit)->GetSubGroup());
			}
			else
			{
				gperrorf(("TRIGGER_SYS: LOAD failed to add condition [%s] @ %s", (*addit)->GetName(),fuel.GetAddress().c_str()));
				delete (*addit);
			}
		}

#if !GP_RETAIL
		gpstring errmsg;
		errmsg.assignf("Error loading trigger @ %s\n",fuel.GetAddress().c_str());
		bool ok = pTrigger->Validate(errmsg);
		if (ok || gGoDb.IsEditMode())
		{
			Add_Trigger( autoTrigger.release() );
		}

		if (!ok)
		{
			gperrorf((errmsg.c_str()));
		}
#else
		Add_Trigger( autoTrigger.release() );
#endif

		triggers_loaded = true;
	}

#if !GP_RETAIL
	gpstring errheader;
	errheader.assignf("Error loading triggers @ %s\n",fuel.GetAddress().c_str());
	ValidateTriggers(errheader);
#endif // !GP_RETAIL

	return triggers_loaded;
}



void
Storage::Save( FuelHandle fuel ) const
{

	GPPROFILERSAMPLE( "0Trigger :: Storage::Save", SP_TRIGGER_MISC );

#if !GP_RETAIL
	gpstring errheader;
	errheader.assignf("Error saving triggers @ %s\n",fuel->GetAddress().c_str());
	ValidateTriggers(errheader);
#endif // !GP_RETAIL

	TriggerColl::const_iterator i, begin = m_Triggers.begin();
	for ( i = begin ; i != m_Triggers.end() ; ++i )
	{
		FuelHandle hTrigger = fuel->CreateChildBlock( "*" );

		Trigger *pTrigger = *i;

		unsigned int float_index = 0;
		unsigned int string_index = 0;
		unsigned int integer_index = 0;

		// Set trigger parameters
		hTrigger->Set( "single_player", pTrigger->GetIsSingle() );
		hTrigger->Set( "multi_player", pTrigger->GetIsMulti() );
		hTrigger->Set( "start_active", pTrigger->GetRequestedActive() );
		hTrigger->Set( "delay", pTrigger->GetDelay() );
		hTrigger->Set( "flip_flop", pTrigger->IsFlipFlop() );
		hTrigger->Set( "no_trig_bits", pTrigger->IsNoTrigBits() );
		hTrigger->Set( "single_shot", pTrigger->IsOneShot() );
		hTrigger->Set( "can_self_destruct", pTrigger->CanSelfDestruct() );
		hTrigger->Set( "reset_duration", pTrigger->GetResetDuration() ) ;
		hTrigger->Set( "occupants_group", pTrigger->GetGroupName() );

		if ( pTrigger->GetConditions().empty() )
		{
			gpwarningf(( "You're saving a trigger without conditions at location %s in file %s",
				fuel->GetAddress().c_str(), fuel->GetOriginPath().c_str() ));
		}

		for (ActionConstIter ait = pTrigger->GetActions().begin(); ait != pTrigger->GetActions().end(); ++ait)
		{

			float_index = 0;
			string_index = 0;
			integer_index = 0;

			//pTrigger->GetActionInfo( (*ait).first, sName, pFormat, pParams );

			const gpstring& sName = (*ait).second.second->GetName();
			Parameter* pFormat = (*ait).second.second->GetFormat();
			const Params* pParms = &(*ait).second.first;

			gpstring sParameters = "(";


			for ( unsigned int o = 0; o != pFormat->Size(); ++o ) {
				Parameter::format *pParamFormat = 0;
				pFormat->Get( o, &pParamFormat );
				char szValue[256] = "";

				if ( pParamFormat->isFloat && (float_index < pParms->floats.size()) ) {
					if ( o != 0 ) {
						sParameters += ",";
					}
					sprintf( szValue, "%gf", pParms->floats[float_index] );
					sParameters += szValue;
					float_index++;
				}
				else if ( pParamFormat->isString && (string_index < pParms->strings.size()) ) {
					if ( o != 0 ) {
						sParameters += ",";
					}
					sprintf( szValue, "\"%s\"", pParms->strings[string_index].c_str() );
					sParameters += szValue;
					string_index++;
				}
				else if ( pParamFormat->isInt && (integer_index < pParms->ints.size()) ) {
					if ( o != 0 ) {
						sParameters += ",";
					}
					if ( !pParamFormat->bSaveAsHex )
					{
						sprintf( szValue, "%i", pParms->ints[integer_index] );
						sParameters += szValue;
					}
					else
					{
						stringtool::Setx( pParms->ints[integer_index], sParameters, true );
					}
					integer_index++;
				}
				else if ( pParamFormat->isGoid && (integer_index < pParms->ints.size()) ) {
					if ( o != 0 )
					{
						sParameters += ",";
					}
					stringtool::Setx( pParms->ints[integer_index], sParameters, true );
					integer_index++;
				}
				else if ( pParamFormat->isBool && (integer_index < pParms->ints.size()) ) {
					if ( o != 0 )
					{
						sParameters += ",";
					}
					stringtool::Set( (bool)(pParms->ints[integer_index] != 0), sParameters, true );
					integer_index++;
				}
			}
			sParameters += ")";

			gpstring sTags;

			if ( pParms->Delay != 0 )
			{
				sTags.appendf( ", delay(%g)", pParms->Delay );
			}

			if ( pParms->SubGroup != 0 )
			{
				sTags.appendf( ", group(%i)", pParms->SubGroup );
			}

			if ( !pParms->Doc.empty() )
			{
				sTags.appendf( ", doc(\"%s\")", pParms->Doc.c_str() );
			}

			if ( pParms->bWhenFalse )
			{
				hTrigger->Set( "action*", "when_false " + sName + sParameters + sTags );
			}
			else
			{
				hTrigger->Set( "action*", sName + sParameters + sTags );
			}
		}

		for (ConditionConstIter cit = pTrigger->GetConditions().begin(); cit != pTrigger->GetConditions().end(); ++cit)
		{

			float_index = 0;
			string_index = 0;
			integer_index = 0;

			gpstring sName;
			Parameter pFormat;
			Params parms;

			if (!pTrigger->GetConditionInfo( (*cit).first, sName, pFormat, parms ))
			{
				continue;
			}

			gpstring sParameters = "(";
			for ( unsigned int o = 0; o != pFormat.Size(); ++o )
			{
				Parameter::format *pParamFormat = 0;
				pFormat.Get( o, &pParamFormat );
				char szValue[256] = "";

				if ( pParamFormat->isFloat && (float_index < parms.floats.size()) )
				{
					if ( o != 0 )
					{
						sParameters += ",";
					}
					sprintf( szValue, "%f", parms.floats[float_index] );
					sParameters += szValue;
					float_index++;
				}
				else if ( pParamFormat->isString && (string_index < parms.strings.size()) )
				{
					if ( o != 0 )
					{
						sParameters += ",";
					}
					sprintf( szValue, "\"%s\"", parms.strings[string_index].c_str() );
					sParameters += szValue;
					string_index++;
				}
				else if ( pParamFormat->isInt && (integer_index < parms.ints.size()) )
				{
					if ( o != 0 )
					{
						sParameters += ",";
					}
					if ( !pParamFormat->bSaveAsHex )
					{
						sprintf( szValue, "%i", parms.ints[integer_index] );
						sParameters += szValue;
					}
					else
					{
						stringtool::Setx( parms.ints[integer_index], sParameters, true );
					}
					integer_index++;
				}
				else if ( pParamFormat->isGoid && (integer_index < parms.ints.size()) )
				{
					if ( o != 0 )
					{
						sParameters += ",";
					}
					stringtool::Setx( parms.ints[integer_index], sParameters, true );
					integer_index++;
				}
				else if ( pParamFormat->isBool &&  (integer_index < parms.ints.size()) )
				{
					if ( o != 0 )
					{
						sParameters += ",";
					}
					stringtool::Set( (bool)(parms.ints[ integer_index ] != 0), sParameters, true );
					integer_index++;
				}
			}
			sParameters += ")";

			gpstring sTags;

			if ( parms.SubGroup != 0 )
			{
				sTags.appendf( ", group(%i)", parms.SubGroup );
			}

			if ( !parms.Doc.empty() )
			{
				sTags.appendf( ", doc(\"%s\")", parms.Doc.c_str() );
			}

			hTrigger->Set( "condition*", sName + sParameters + sTags );
		}

	}
}


#if !GP_RETAIL

bool
Storage::ValidateTriggers(const gpstring& errheader) const
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::ValidateTriggers", SP_TRIGGER_MISC );

	bool ret=true;
	if ( m_Owner != NULL )
	{
		TriggerColl::const_iterator i, ibegin = m_Triggers.begin();
		gpstring errbuff = errheader;
		for ( i = ibegin ; i != m_Triggers.end() ; ++i )
		{
			if (!(*i)->Validate(errbuff))
			{
				gperrorf((errbuff.c_str()));
				ret = false;
			}
		}
	}
	return ret;
}

#endif // !GP_RETAIL


bool Storage::Xfer( FuBi::PersistContext& persist )
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::Xfer", SP_TRIGGER_MISC );
	gpassert( m_UpdateNesting == 0 );

	persist.XferHex( "m_TrigNumAssigned", m_TrigNumAssigned );
	persist.Xfer( "m_CanSelfDestruct", m_CanSelfDestruct );

	gpassert( persist.IsSaving() || m_Triggers.empty() );
	persist.EnterColl( "m_Triggers" );

	size_t count = m_Triggers.size();
	persist.Xfer( "_count", count );
	m_Triggers.resize( count, NULL );

	gpstring type;
	TriggerColl::iterator i, begin = m_Triggers.begin();
	for ( i = begin ; i != m_Triggers.end() ; ++i )
	{
		// advance
		if ( !persist.AdvanceCollIter() )
		{
			return ( false );
		}

		// construct the trigger if restoring
		if ( persist.IsRestoring() )
		{
			*i = new Trigger;
		}

		// xfer the component
		(*i)->Xfer( persist );
	}

	persist.LeaveColl();

	return ( true );
}

bool Storage::XferPost()
{

	gpassert( m_Owner && (m_Owner->GetGoid() != GOID_INVALID) );

	Goid owngoid = m_Owner->GetGoid();
	TriggerColl::iterator iTrigger = m_Triggers.begin();

	// Run through all triggers, assigning them the proper Owner
	while ( iTrigger != m_Triggers.end() )
	{
		// if we're the server or this is a client-ok trigger, set its id
		if ( (*iTrigger)->IsClientTrigger() || ::IsServerLocal() )
		{
			// Make sure that the truid was persisted correctly
			Truid tr = (*iTrigger)->GetTruid();
			gpassert(tr.GetOwnGoid() == owngoid);
			
			if (tr.GetOwnGoid() == owngoid)
			{
				gTriggerSys.AddValidTrigger( (*iTrigger) );
				// Now that it has a Truid, the trigger is 'valid'
				(*iTrigger)->SetOwner( m_Owner );

				if (m_Owner->IsInAnyWorldFrustum())
				{
					if ((*iTrigger)->GetIsSpatial())
					{
						gTriggerSys.AddSpatialTrigger( (*iTrigger) );
					}
				}
				++iTrigger;
			}
			else
			{
				iTrigger = Remove_Trigger( *iTrigger );
			}

		}
		else
		{
			// nope? just delete it right now
			iTrigger = Remove_Trigger( *iTrigger );
		}
	}
	return true;
}

void
Storage::SetOwner( Go* go )
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::SetOwner", SP_TRIGGER_MISC );
	gpassert( go != NULL );
	gpassert( m_Owner == NULL );

	if ( m_Owner != go )
	{
		m_Owner = go;

		TriggerColl::iterator i, begin = m_Triggers.begin();
		for ( i = begin ; i != m_Triggers.end() ; ++i )
		{
			(*i)->SetOwner( go );
		}
	}
}

void
Storage::RefreshOwner()
{
	// Must be called if the owner of the storage is moved
	TriggerColl::iterator i, begin = m_Triggers.begin();
	for ( i = begin ; i != m_Triggers.end() ; ++i )
	{
		(*i)->RefreshOwner( );
	}
}


bool
Storage::GetMustCreateOnClients() const
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::GetMustCreateOnClients", SP_TRIGGER_MISC );
	TriggerColl::const_iterator i, ibegin = m_Triggers.begin();
	for ( i = ibegin ; i != m_Triggers.end() ; ++i )
	{
		if ( (*i)->GetMustCreateOnClients() )
		{
			return ( true );
		}
	}

	return ( false );
}

bool
Storage::GetCanExpire() const
{

	TriggerColl::const_iterator i = m_Triggers.begin();
	for ( ; i != m_Triggers.end() ; ++i )
	{
		if ( (*i)->GetIsTooComplicated() )
		{
			return ( false );
		}
	}
	return true;	
}


#if !GP_RETAIL
void
Storage::Draw()
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::Draw", SP_TRIGGER_MISC );
	TriggerColl::iterator iTrigger = m_Triggers.begin();

	for(; iTrigger != m_Triggers.end(); ++iTrigger )
	{
		(*iTrigger)->Draw();
	}
}
#endif


void
Storage::CommitCreation()
{
	if ( m_Owner->IsCloneSourceGo() )
	{
		// Clone sources don't have valid triggers
		return;
	}

	gpassert( m_Owner && (m_Owner->GetGoid() != GOID_INVALID) );
	gpassert( m_TrigNumAssigned == 0 || m_TrigNumAssigned == 0x8000);	// only do it once!
	
	Goid owngoid = m_Owner->GetGoid();
	TriggerColl::iterator iTrigger = m_Triggers.begin();

	// Run through all triggers, assigning them the proper Owner
	// Verfiy that Lodfi triggers are pure message handlers
	while ( iTrigger != m_Triggers.end() )
	{
		// Make sure that triggers that are not pure message handlers 
		// are neither lodfi nor clientside
		if (!(*iTrigger)->GetIsPureMessageHandler())
		{
			if (m_Owner->IsLodfi())
			{
				gpstring txt;
				(*iTrigger)->GetNonMessageConditions(txt);
				gperrorf(("TRIGGER_SYS: Storage CONSTRUCTION failed. Lodfi object [%s:0x%08x]"
						  " has a non-message handling condition: %s",
						  m_Owner->GetTemplateName(),m_Owner->GetScid(),txt.c_str()));
				iTrigger = Remove_Trigger( *iTrigger );
				continue;
			}
		}

		// Account for the trigger, whether we keep it or not
		++m_TrigNumAssigned;

		// if we're the server or this is a client-ok trigger, set its id
		if ( (*iTrigger)->IsClientTrigger() || ::IsServerLocal() )
		{
			gpassert((*iTrigger)->GetTruid() == Truid());
			(*iTrigger)->SetTruid( Truid( owngoid, m_TrigNumAssigned ) );

			// Now that it has a Truid, the trigger is 'valid'
			gTriggerSys.AddValidTrigger( (*iTrigger) );

			(*iTrigger)->SetOwner( m_Owner );

			++iTrigger;
		}
		else
		{
			// nope? just delete it right now
			iTrigger = Remove_Trigger( *iTrigger );
		}
	}
}

void
Storage::Add_Trigger( Trigger *pTrigger )
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::Add_Trigger", SP_TRIGGER_MISC );

	// if any one of them can't self-destruct or it's not single shot, then
	// the whole storage is out of consideration for self-destructo.
	if ( !pTrigger->CanSelfDestruct() || !pTrigger->IsOneShot() )
	{
		m_CanSelfDestruct = false;
	}

	pTrigger->UpdateTriggerFlags();

	// Gets added to "valid" triggers in commit creation
	m_Triggers.push_back( pTrigger );

}



Storage::TriggerColl::iterator
Storage::Remove_Trigger( Trigger *pTrigger )
{
	GPPROFILERSAMPLE( "0Trigger :: Storage::Remove_Trigger", SP_TRIGGER_MISC );

	TriggerColl::iterator iTrigger = m_Triggers.begin();
	for ( ; iTrigger != m_Triggers.end(); ++iTrigger )
	{
		if ( *iTrigger == pTrigger )
		{
			delete pTrigger;
			return ( m_Triggers.erase( iTrigger ) );
		}
	}
	return ( m_Triggers.end() );
}



bool
Storage::Get( UINT32 i, Trigger **pTrigger )
{
	if( i > Size() ) {
		return false;
	}

	*pTrigger = m_Triggers[i];
	return true;
}

// Storage synchronization (server to clients)

void Storage::XferForSync( FuBi::BitPacker& packer )
{

	if ( packer.IsSaving() )
	{
		TriggerCollIter trit = m_Triggers.begin();
		TriggerCollIter tend = m_Triggers.end();

		// The number of triggers was xferred-out in the GoCommon::XferForSync

		for ( ; trit != tend ; ++trit )
		{
			if (!(*trit)->IsClientTrigger())
			{
				continue;
			}

			WORD trig_id;
			trig_id = (*trit)->GetTruid().GetTrigNum();
			packer.XferRaw(trig_id);

			bool pure_message = (*trit)->GetIsPureMessageHandler();
			packer.XferBit(pure_message);

			if ( !pure_message )
			{
				// Only sync data for non-pure message handlers
				(*trit)->XferForSync(packer);
			}
		}
	}
	else
	{

		TriggerCollIter tbeg = m_Triggers.begin();
		TriggerCollIter tend = m_Triggers.end();
		TriggerCollIter trit;

		for ( trit = tbeg; trit != tend; ++trit ) 
		{
			if ( !(*trit)->GetIsPureMessageHandler() )
			{
				// Mark all non-pure message handling triggers for deletion
				(*trit)->SetMarkedForDeletion(true);
			}
		}

		DWORD num_trigs;
		packer.XferCount( num_trigs );

		if ( num_trigs > m_Triggers.size() )
		{
			gpwarningf( (
				"TRIGGERSYS: The client trigger storage has fewer triggers than the server has XferForSync-ed ( %d < %d )",
				m_Triggers.size(),
				num_trigs
				));
		}

		for (  ; num_trigs ;  --num_trigs )
		{
			// Fetch the ID of the trigger to update
			WORD trig_id;
			packer.XferRaw(trig_id);

			for ( trit = m_Triggers.begin(); trit != tend  ; ++trit ) 
			{
				if ( trig_id == (*trit)->GetTruid().GetTrigNum() )
				{
					break;
				}
			}

			if (trit != tend)
			{
				bool pure_message;
				packer.XferBit(pure_message);

				if (!pure_message)
				{

					if ( !(*trit)->GetIsPureMessageHandler() )
					{
						// We've found the trigger to update, so don't delete it!
						(*trit)->SetMarkedForDeletion(false);

						// We only sync data for non-pure message handlers
						(*trit)->XferForSync(packer);
					}
					else
					{
						gperrorf(("TRIGGERSYS: The server says a trigger is NOT a pure message handler, but the client thinks it IS a pure message handler"));
						DWORD num_conditions;
						packer.XferCount( num_conditions );

						for ( ; num_conditions ; num_conditions--)
						{
							DWORD num_boccs;
							DWORD num_soccs;
							packer.XferCount( num_boccs );
							packer.XferCount( num_soccs );
							for ( ; num_boccs>0 ; num_boccs--)
							{
								Goid g;
								DWORD c;
								packer.XferRaw( g );
								packer.XferCount( c );
							}
							for ( ; num_soccs>0 ; num_soccs--)
							{
								Goid g;
								DWORD c;
								packer.XferRaw( g );
								packer.XferCount( c );
							}
						}
					}
				}
			}
			else
			{
				gpwarningf((
					"TRIGGERSYS: A client has received an XferForSync for a trigger that no longer exists [%04x]",
					trig_id
					));

				// We need to read this trigger entry out of the packer and discard it

				SkipTriggerXfer(packer);
			}
		}
	}
}

// If the Trigger Sync was sent in error, we need to skip it

void Storage::SkipTriggerXfer( FuBi::BitPacker& packer )
{

	bool pure_message = false;
	packer.XferBit(pure_message);
	
	if (!pure_message)
	{
		DWORD num_conditions;
		packer.XferCount( num_conditions );

		for ( ; num_conditions ; num_conditions--)
		{
			DWORD num_boccs;
			DWORD num_soccs;
			packer.XferCount( num_boccs );
			packer.XferCount( num_soccs );
			for ( ; num_boccs>0 ; num_boccs--)
			{
				Goid g;
				DWORD c;
				packer.XferRaw( g );
				packer.XferCount( c );
			}
			for ( ; num_soccs>0 ; num_soccs--)
			{
				Goid g;
				DWORD c;
				packer.XferRaw( g );
				packer.XferCount( c );
			}
		}
	}
}

DWORD Storage::NumClientTriggers()
{
	TriggerCollIter tbeg = m_Triggers.begin();
	TriggerCollIter tend = m_Triggers.end();
	TriggerCollIter trit;

	DWORD count = 0;
	for ( trit = tbeg; trit != tend; ++trit ) 
	{
		if ((*trit)->IsClientTrigger())
		{
			count++;
		}
	}
	return count;
}

// Parameter storage

bool Params::Xfer( FuBi::PersistContext& persist )
{

	GPPROFILERSAMPLE( "0Trigger :: Params::Xfer", SP_TRIGGER_MISC );
	persist.Xfer( "ParamID", ParamID );
	persist.XferVector( "ints", ints );
	persist.XferVector( "floats", floats );
	persist.XferVector( "strings", strings );
	persist.Xfer( "bWhenFalse", bWhenFalse );
	persist.Xfer( "Delay", Delay );
	persist.Xfer( "CountdownTimeout", CountdownTimeout );
	persist.Xfer( "SubGroup", SubGroup );
	return ( true );
}

// Message Headers

bool MessageHeader::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer    ( "m_Event"		, m_Event    );
	persist.Xfer    ( "m_SendFrom"	, m_SendFrom );
	persist.Xfer    ( "m_SendTo"	, m_SendTo   );
	persist.XferHex ( "m_Data1"		, m_Data1    );
	return true;
}

// Boundary Crossing signalling 

bool BoundaryCrossing::Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer    ( "m_Time"        , m_Time        );
	persist.XferHex ( "m_TriggerID"   , m_TriggerID   );
	persist.XferHex ( "m_Data"		  , m_Data        );
	persist.Xfer    ( "m_CrossType"   , m_CrossType   );
	persist.Xfer    ( "m_ConditionID" , m_ConditionID );
	persist.Xfer    ( "m_SequenceNum" , m_SequenceNum );
	return true;
}

// Trigger mechanism

Trigger::Trigger()
{
	m_bInitiallyActiveState = true;
	m_bCurrentActiveState	= false;	// Always start false and get set to the correct 
	m_bRequestedActiveState	= true;		// active state on the next update() call

	m_bMarkedForDeletion	= false;
	m_InitialDelay			= 0;
	m_ResetTimeout			= 0;
	m_bSpatialTrigger		= false;
	m_bMessageTrigger		= false;
	m_bPureMessageTrigger	= false;
	m_bUniqueEnterTrigger	= false;
	m_bIsPartySpecific		= false;
	m_bIsGroupWatcher		= false;
	m_bIsOneShot			= false;
	m_bNoTrigBits			= false;
	m_bOneShotHasTriggered  = false;
	m_bCanSelfDestruct		= true;
	m_bWaitsForConstruction	= false;
	m_ResetDuration			= 0;
	m_bIsFlipFlop			= false;
	m_bClientTrigger		= false;
	m_bIsMulti				= true;
	m_bIsSingle				= true;
	m_bAllActionsExecuted	= true;
	m_pOwnerGO				= NULL;
	m_LastParamIDAssigned	= 0;
}

Trigger::Trigger( const Trigger& source )
{
	m_Truid = Truid();

	m_bInitiallyActiveState	= source.m_bInitiallyActiveState;
	m_bCurrentActiveState	= false;							// Always start false and get set to the correct 
	m_bRequestedActiveState	= true;								// active state on the next update() call
	
	m_bMarkedForDeletion = false;

	m_ResetDuration			= source.m_ResetDuration;
	m_InitialDelay			= source.m_InitialDelay;
	m_ResetTimeout			= 0;

	m_bIsOneShot		    = source.m_bIsOneShot;
	m_bNoTrigBits			= source.m_bNoTrigBits;
	m_bOneShotHasTriggered  = false;
	m_bCanSelfDestruct		= source.m_bCanSelfDestruct;

	m_bWaitsForConstruction = false;	//  derived from conditions -- 

	m_bSpatialTrigger		= source.m_bSpatialTrigger;
	m_bMessageTrigger		= source.m_bMessageTrigger;
	m_bPureMessageTrigger	= source.m_bPureMessageTrigger;
	m_bUniqueEnterTrigger	= source.m_bUniqueEnterTrigger;
	m_bIsPartySpecific		= source.m_bIsPartySpecific;
	m_bIsGroupWatcher		= source.m_bIsGroupWatcher;

	m_bIsFlipFlop		= source.m_bIsFlipFlop;
											
	m_bClientTrigger	= source.m_bClientTrigger;

	m_bIsMulti			= source.m_bIsMulti;	
	m_bIsSingle			= source.m_bIsSingle;	

	// for delayed actions
	m_bAllActionsExecuted = true;

	m_pOwnerGO			= NULL;				

	m_LastParamIDAssigned	= source.m_LastParamIDAssigned;

	{
		ConditionDb::const_iterator i, ibegin = source.m_Conditions.begin(), iend = source.m_Conditions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			Condition* p = gTriggerSys.NewCondition(i->second->GetName());
			Params parms;
			
			bool ok = (p!=0);
			ok = ok && i->second->FetchParameterValues( parms );

			p->SetID(parms.ParamID);
			ok = ok && p->StoreParameterValues(parms);
			p->SetID(0);

			ok = ok && AddCondition(parms.ParamID,p);

			if (ok)
			{
				AddSubGroup(p->GetSubGroup());
			}
			else
			{
				delete p;
			}
		}
	}

	{
		ActionDb::const_iterator i, ibegin = source.m_Actions.begin(), iend = source.m_Actions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			m_Actions.insert(*i);		
		}
	}

	m_GroupName			= "";

//	m_CurrentInsideConditions.clear();
}

Trigger::~Trigger()
{
#	if !GP_RETAIL
	if ( !m_bAllActionsExecuted && (gWorldState.GetCurrentState() != WS_RELOADING) && !gGoDb.IsShuttingDown() )
	{
		gpwarnassertf(("WARNING: Deleting trigger with delayed actions pending [%s:0x%08x:0x%04x:0x%08x]\n",
			GetOwner() ? GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
			m_Truid.GetOwnGoid(),
			m_Truid.GetTrigNum(),
			GetOwner() ? MakeInt(GetOwner()->GetScid()) : -1
			));
	}
#	endif // !GP_RETAIL

	// Delete all of the conditions
	stdx::for_each( m_Conditions, stdx::delete_second_by_ptr() );
	m_Conditions.clear();

	if (m_GroupName)
	{
		// remove a member from all group
		gTriggerSys.RemoveGroupMember(m_GroupName,m_Truid);
	}

	if ( gTriggerSys.IsSpatialTrigger(this) )
	{
		gTriggerSys.RemoveSpatialTrigger(this);
	}

	if ( gTriggerSys.IsValidTrigger(this) )
	{
		gTriggerSys.RemoveValidTrigger( this );
	}

}

void Trigger::Shutdown()
{
	m_bAllActionsExecuted = true;
}

void Trigger::SetOwner(Go* go)
{
	if (go && go->HasValidGoid())
	{
		m_pOwnerGO = go;
	
		if (!m_GroupName.empty())
		{
			gTriggerSys.AddGroupMember(m_GroupName,m_Truid);
		}

		ConditionDb::iterator i, begin = m_Conditions.begin();
		for ( i = begin ; i != m_Conditions.end() ; ++i )
		{
			(*i).second->SetTrigger(this);
		}
	}
}

void Trigger::RefreshOwner()
{
	// Must be called if the owner of the trigger is moved
	ConditionDb::iterator i, begin = m_Conditions.begin();
	for ( i = begin ; i != m_Conditions.end() ; ++i )
	{
		(*i).second->SetTrigger(this);
	}
}

void Trigger::UpdateTriggerFlags()
{
	m_bSpatialTrigger		= false;
	m_bMessageTrigger		= false;
	m_bPureMessageTrigger	= true;
	m_bUniqueEnterTrigger	= false;
	m_bIsPartySpecific		= true;
	m_bIsGroupWatcher		= false;

	ConditionDb::iterator i, begin = m_Conditions.begin();
	for ( i = begin ; i != m_Conditions.end() ; ++i )
	{
		m_bSpatialTrigger |= (*i).second->IsSpatial();
		m_bUniqueEnterTrigger |= (*i).second->HasUniqueEnterTest();
		m_bIsGroupWatcher |= (*i).second->IsGroupWatcher();

		if ((*i).second->IsSpatial() && !(*i).second->IsPartySnooper())
		{
			m_bIsPartySpecific = false;
		}
		
		if ((*i).second->IsMessageHandler())
		{
			m_bMessageTrigger = true;
		}
		else
		{
			m_bPureMessageTrigger = false;
		}
	}

	m_bIsPartySpecific &= m_bSpatialTrigger;	// Verify that we are in fact, spatial

	// The trigger is a 'client trigger' iff is has at least one action that
	// can (must!) be executed on a client OR it is a member of a group!
	
	m_bClientTrigger = !m_GroupName.empty();
	ActionDb::iterator a, abegin = m_Actions.begin();
	for ( a = abegin ; a != m_Actions.end() ; ++a )
	{
		if ( a->second.second->GetAllowClientExecution() )
		{
			m_bClientTrigger = true;
		}
	}
	
	m_SubGroupStates.sort();
}

void Trigger::SetMarkedForDeletion(bool set)
{
	m_bMarkedForDeletion = set;

	gpassert(GetOwner());

	if ( GetOwner() )
	{
		GetOwner()->SetBucketDirty(true);
	}
}

bool Trigger::GetIsSubGroupSpatial(int sg) const
{
	bool flag = false;
	ConditionConstIter i, begin = m_Conditions.begin();
	for ( i = begin ; !flag && (i != m_Conditions.end()) ; ++i )
	{
		if ((*i).second->GetSubGroup() == sg)
		{
			flag = (*i).second->IsSpatial();
		}
	}
	return flag;
}

bool Trigger::GetIsSubGroupMessageHandler(int sg) const
{
	bool flag = false;
	ConditionConstIter i, begin = m_Conditions.begin();
	for ( i = begin ; !flag && (i != m_Conditions.end()) ; ++i )
	{
		if ((*i).second->GetSubGroup() == sg)
		{
			flag = (*i).second->IsMessageHandler();
		}
	}
	return flag;
}

bool Trigger::GetIsSubGroupPureMessageHandler(int sg) const
{
	bool flag = true;
	ConditionConstIter i, begin = m_Conditions.begin();
	for ( i = begin ; flag && (i != m_Conditions.end()) ; ++i )
	{
		if ((*i).second->GetSubGroup() == sg)
		{
			flag = (*i).second->IsMessageHandler();
		}
	}
	return flag;
}

bool Trigger::GetIsTooComplicated() const
{
	if (m_bIsOneShot && m_bOneShotHasTriggered)
	{
		return false;
	}

	ConditionConstIter i, begin = m_Conditions.begin();
	for ( i = begin ; i != m_Conditions.end() ; ++i )
	{
		if ((*i).second->IsComplicated())
		{
			return true;
		}
	}
	return false;
}

bool Trigger::GetMustPreserveState() const
{
	if (m_pOwnerGO->IsSpawned() || IsNoTrigBits())
	{
		return false;
	}

	bool flag = (m_bRequestedActiveState != m_bInitiallyActiveState);

	// DON'T persist triggers that are too complicated
	ConditionConstIter i, begin = m_Conditions.begin();
	for ( i = begin ; flag && (i != m_Conditions.end()) ; ++i )
	{
		flag = !(*i).second->IsComplicated();
	}
	return flag;
}

bool Trigger::Xfer( FuBi::PersistContext& persist )
{
	GPPROFILERSAMPLE( "0Trigger :: Xfer", SP_TRIGGER_MISC );

	if (persist.IsRestoring())
	{
		gpassert(m_Truid == Truid());
		m_SignalsExpected.clear();
	}
	
	persist.EnterBlock( "Truid" );
	m_Truid.Xfer( persist );
	persist.LeaveBlock();

	persist.Xfer(		"m_bInitiallyActiveState",	m_bInitiallyActiveState );
	persist.Xfer(		"m_bRequestedActiveState",	m_bRequestedActiveState );
	persist.Xfer(		"m_bCurrentActiveState",	m_bCurrentActiveState	);

	persist.Xfer(		"m_InitialDelay",			m_InitialDelay			);
	persist.Xfer(		"m_ResetTimeout",			m_ResetTimeout			);
	
	persist.XferSet(	"m_SignalsExpected",		m_SignalsExpected		);
	persist.Xfer(		"m_bIsOneShot",				m_bIsOneShot			);
	persist.Xfer(		"m_bNoTrigBits",			m_bNoTrigBits			);
	persist.Xfer(		"m_bOneShotHasTriggered",	m_bOneShotHasTriggered	);
	persist.Xfer(		"m_bCanSelfDestruct",		m_bCanSelfDestruct		);

	persist.Xfer(		"m_ResetDuration",			m_ResetDuration			);
	persist.Xfer(		"m_bIsFlipFlop",			m_bIsFlipFlop			);
	persist.Xfer(		"m_bClientTrigger",			m_bClientTrigger		);
	persist.Xfer(		"m_bIsMulti",				m_bIsMulti				);
	persist.Xfer(		"m_bIsSingle",				m_bIsSingle				);
	persist.Xfer(		"m_bAllActionsExecuted",	m_bAllActionsExecuted	);
	persist.Xfer(		"m_LastParamIDAssigned",	m_LastParamIDAssigned	);

	persist.Xfer( "m_GroupName", m_GroupName);

	if ( persist.IsRestoring() )
	{
		m_pOwnerGO = NULL;
		m_SubGroupStates.clear();
		m_Conditions.clear();

		// Make sure that there are no stale signals in the file
		SignalIter sigit = m_SignalsExpected.begin();
		while (sigit!=m_SignalsExpected.end())
		{
			if ((*sigit).m_Time < gWorldTime.GetTime())
			{
				gpdevreportf( &gTriggerSysContext,(
					"TRIGGER_SYS: Attempted to load stale ExpectedSignal entry! The entry will be ignored\n"
					"\tSaved games from builds prior to 02.02.1201 may generate contain many stale signals\n"
					"\tStale signal data: wt:%f st:%f [trigger goid:%08x num:%04x cond:%04x] [mover g:%08x %s num:%04x]\n",
					gWorldTime.GetTime(),
					(*sigit).m_Time,
					(*sigit).m_TriggerID.GetOwnGoid(),			
					(*sigit).m_TriggerID.GetTrigNum(),			
					(*sigit).m_ConditionID,
					(*sigit).m_Data,
					ToString((*sigit).m_CrossType),
					(*sigit).m_SequenceNum
					));
				sigit = m_SignalsExpected.erase(sigit);
			}
			else
			{
				++sigit;
			}
		}
		
	}
	else
	{
		// Make sure that there are no stale signals going out to the file
		SignalIter sigit = m_SignalsExpected.begin();
		for (;sigit!=m_SignalsExpected.end();++sigit)
		{
			if ((*sigit).m_Time < gWorldTime.GetTime())
			{
				gpdevreportf( &gTriggerSysContext,(
					"TRIGGER_SYS: Attempting to save a stale signal! Will be ignored on reload\n"
					"\twt:%f st:%f [trigger goid:%08x num:%04x cond:%04x] [mover g:%08x %s num:%04x]\n",
					gWorldTime.GetTime(),
					(*sigit).m_Time,
					(*sigit).m_TriggerID.GetOwnGoid(),			
					(*sigit).m_TriggerID.GetTrigNum(),			
					(*sigit).m_ConditionID,
					(*sigit).m_Data,
					ToString((*sigit).m_CrossType),
					(*sigit).m_SequenceNum
					));
			}
		}
	}

	persist.XferMap( FuBi::XFER_HEX,"m_SubGroupStates", m_SubGroupStates );

	if ( persist.IsRestoring() && m_SubGroupStates.empty())
	{
		// Try and recover the old map format
		stdx::linear_map< int, bool > lss;
		persist.XferMap( FuBi::XFER_HEX,"m_SubGroupStates", lss );
		stdx::linear_map< int, bool >::iterator it = lss.begin();
		stdx::linear_map< int, bool >::iterator end = lss.end();
		for (;it!=end;++it)
		{
			m_SubGroupStates.unsorted_push_back(std::make_pair((*it).first,std::make_pair((*it).second,false)));
		}
		m_SubGroupStates.sort();
	}
	
	persist.XferMap( FuBi::XFER_HEX,"m_Conditions", m_Conditions );

	persist.EnterColl( "m_Actions" );
	if ( persist.IsSaving() )
	{
		ActionDb::iterator i, begin = m_Actions.begin();
		for ( i = begin ; i != m_Actions.end() ; ++i )
		{
			persist.AdvanceCollIter();	
			gpstring val = (*i).second.second->GetName();
			persist.Xfer( "_val", val );
			i->second.first.Xfer( persist );
		}
	}
	else
	{
		gpassert( m_Actions.empty() );

		while ( persist.AdvanceCollIter() )
		{
			gpstring val;
			Params newparams;
			persist.Xfer( "_val", val );
			newparams.Xfer( persist );
			if (newparams.ParamID == 0) 
			{ 
				++m_LastParamIDAssigned;
				newparams.ParamID = m_LastParamIDAssigned;
			}
			Action* pAction = gTriggerSys.NewAction(val);

			AddAction( newparams.ParamID, pAction, newparams );
		}
	}
	persist.LeaveColl();

	if ( persist.IsRestoring() )
	{
		UpdateTriggerFlags();
	}

	return ( true );
}

bool
Trigger::RestoreFromTrigBits()
{
	gpassert(m_pOwnerGO);

	if (m_pOwnerGO->IsSpawned() || IsNoTrigBits())
	{
		return false;
	}

	TrigBits tb; 
	if (gTriggerSys.FetchTrigBits(Scuid(m_pOwnerGO->GetScid(),m_Truid.GetTrigNum()),tb))
	{
		m_bCurrentActiveState = false;

		m_bOneShotHasTriggered =  ((tb & ONESHOT_HAS_TRIGGERED_TRIGBIT) != 0);
		m_bMarkedForDeletion =    ((tb & MARKED_FOR_DELETION_TRIGBIT)   != 0) && !Services::IsEditor();
		m_bRequestedActiveState = ((tb & CURRENT_ACTIVE_STATE_TRIGBIT)  != 0);
		
		// Restore the states of all the sub groups 
		DWORD bit = SUBGROUP0_IS_TRUE_TRIGBIT;
		DWORD count = 0;
		SubGroupIter sgit = m_SubGroupStates.begin();
		for (;sgit != m_SubGroupStates.end(); ++sgit)
		{
			count++;
			if ((count < 7) && (*sgit).second.first)
			{
				(*sgit).second.first = (tb & bit) != 0;
			}
			bit <<= 1;

			// Delayed status it note preserver
			(*sgit).second.second = false;
		}

		return true;
	}
	return false;
}

bool
Trigger::PreserveInTrigBits()
{
	gpassert(m_pOwnerGO);

	if (m_pOwnerGO->IsSpawned() || IsNoTrigBits())
	{
		return false;
	}

	TrigBits tb = 0; 
	tb |= (m_bRequestedActiveState ? CURRENT_ACTIVE_STATE_TRIGBIT  : 0);
	tb |= (m_bOneShotHasTriggered  ? ONESHOT_HAS_TRIGGERED_TRIGBIT : 0);
	tb |= (m_bMarkedForDeletion    ? MARKED_FOR_DELETION_TRIGBIT   : 0);

	// Preserver the states of all the sub groups 

	DWORD bit = SUBGROUP0_IS_TRUE_TRIGBIT;
	DWORD count = 0;

	SubGroupIter sgit = m_SubGroupStates.begin();
	for (;sgit != m_SubGroupStates.end() && count<8; ++sgit)
	{
		count++;
		if ((*sgit).second.first)
		{
			tb |= bit;
		}
		bit <<= 1;
	}

	return gTriggerSys.StoreTrigBits(Scuid(m_pOwnerGO->GetScid(),m_Truid.GetTrigNum()),tb);
}

bool Trigger::RemoveStaleExpectedSignals()
{
	SignalIter sigit = m_SignalsExpected.begin();

	bool ok = true;

	while (sigit!=m_SignalsExpected.end())
	{
		if ( PreciseAdd((*sigit).m_Time,3) < gWorldTime.GetTime())
		{
			gpdevreportf( &gTriggerSysContext,(
				"TRIGGER_SYS: You have a stale ExpectedSignal entry that is 3 seconds overdue! The entry will be removed!\n"
				"\tStale signal data: wt:%f st:%f [trigger goid:%08x num:%04x cond:%04x] [mover g:%08x %s num:%04x]\n",
				gWorldTime.GetTime(),
				(*sigit).m_Time,
				(*sigit).m_TriggerID.GetOwnGoid(),			
				(*sigit).m_TriggerID.GetTrigNum(),			
				(*sigit).m_ConditionID,
				(*sigit).m_Data,
				ToString((*sigit).m_CrossType),
				(*sigit).m_SequenceNum
				));

			// Do we want to erase them or keep them???
			sigit = m_SignalsExpected.erase(sigit);
			// We don't return false because the stale signal has been removed

			//	++sigit;
			//	ok = false;
		}
		else if ((*sigit).m_Time < gWorldTime.GetTime())
		{
			gpdevreportf( &gTriggerSysContext,(
				"TRIGGER_SYS: Warning! You have a stale ExpectedSignal, entry will be removed if not processed within 3 seconds!\n"
				"\tStale signal data: wt:%f st:%f [trigger goid:%08x num:%04x cond:%04x] [mover g:%08x %s num:%04x]\n",
				gWorldTime.GetTime(),
				(*sigit).m_Time,
				(*sigit).m_TriggerID.GetOwnGoid(),			
				(*sigit).m_TriggerID.GetTrigNum(),			
				(*sigit).m_ConditionID,
				(*sigit).m_Data,
				ToString((*sigit).m_CrossType),
				(*sigit).m_SequenceNum
				));
			++sigit;
			ok = false;
		}
		else
		{
			++sigit;
		}
	}

	return ok;
}

#if !GP_RETAIL

bool
Trigger::Validate(gpstring& errmsg)
{
	bool ret = true;

	GPPROFILERSAMPLE( "0Trigger :: Validate", SP_TRIGGER_MISC );
	{
		ConditionDb::iterator i, ibegin = m_Conditions.begin(), iend = m_Conditions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			ret &= i->second->Validate( *this, errmsg );		
		}
	}

	{
		ActionDb::iterator i, ibegin = m_Actions.begin(), iend = m_Actions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			ret &= i->second.second->Validate( *this, (*i).second.first, errmsg );		
		}
	}

	return ret;
}

#endif // !GP_RETAIL

void Trigger::EnterTheWorld(bool forceactive)
{
	// Now that the owner of the trigger is in the world we set the owner 
	// of each trigger and determine if the trigger has any
	// 'receive we_constructed' conditions
	
	ConditionDb::iterator i, begin = m_Conditions.begin();
	for ( i = begin ; i != m_Conditions.end() ; ++i )
	{
		(*i).second->SetTrigger(this);
	}
	
	if (m_bSpatialTrigger)
	{
		gTriggerSys.AddSpatialTrigger( this );
	}

	if (!m_bCurrentActiveState && m_bRequestedActiveState)
	{
		// Did we miss a construction event because we weren't active?
		if (forceactive)
		{
			OnActivation();
		}

		if (m_bWaitsForConstruction)
		{
			ConditionDb::iterator i, begin = m_Conditions.begin();
			for ( i = begin ; i != m_Conditions.end() ; ++i )
			{
				(*i).second->HandleMessage(*this, WE_CONSTRUCTED);
			}
			if (forceactive)
			{
				Update(gWorldTime.GetTime());
			}
		}
	}	
}

void Trigger::LeaveTheWorld( )
{
	if ( gTriggerSys.IsSpatialTrigger( this ) )
	{
		gTriggerSys.RemoveSpatialTrigger( this );
	}
}

void Trigger::HandleMessage( const WorldMessage& message )
{
	GPPROFILERSAMPLE( "0Trigger :: HandleMessage", SP_TRIGGER_MISC );

	if( TestWorldEventTraits( message.GetEvent(), WMT_RPC ) )
	{
		if( gServer.IsRemote() && !IsClientTrigger() )
		{
			gperrorf(( "You can't RPC WorldEvent '%s' to trigger owned by scid 0x%08x because it's not a client trigger.",
						ToString( message.GetEvent() ), m_pOwnerGO->GetScid() ));
			return;
		}
	}

	gpassertm(GetTruid().IsValid(),("TRIGGER_SYS::HandleMessage() The trigger you are handling messages for does not have a valid TRUID"));

	eWorldEvent evt = message.GetEvent();

	//*** 10392 -- we must preserve the fact that we have destroyed the trigger
	if( m_bMarkedForDeletion && (evt != WE_DESTRUCTED))
	{
		return;
	}

	if ( evt == WE_TRIGGER_ACTIVATE )
	{

		if (gServer.IsLocal())
		{
			m_bRequestedActiveState = true;
			gpdevreportf( &gTriggerSysContext,("Trigger SERVER-SIDE ACTIVATE [%s:0x%08x:%04x:0x%08x]\n"
				,GetOwner() ? GetOwner()->GetTemplateName() : "owner-STILL-not-known"
				,GetOwner() ? MakeInt(GetOwner()->GetGoid()) : 0
				,GetOwner() ? GetTruid().GetTrigNum() : 0
				,GetOwner() ? MakeInt(GetOwner()->GetScid()) : 0
				));
		}
		else
		{
			m_bRequestedActiveState = true;
			m_bCurrentActiveState = true;

			gpdevreportf( &gTriggerSysContext,("Trigger CLIENT-SIDE ACTIVATE [%s:0x%08x:%04x:0x%08x]\n"
				,GetOwner() ? GetOwner()->GetTemplateName() : "owner-STILL-not-known"
				,GetOwner() ? MakeInt(GetOwner()->GetGoid()) : 0
				,GetOwner() ? GetTruid().GetTrigNum() : 0
				,GetOwner() ? MakeInt(GetOwner()->GetScid()) : 0
				));
		}

	}
	else if ( evt == WE_ENTERED_WORLD )
	{
		EnterTheWorld(m_bPureMessageTrigger);
	}
	else if ( evt == WE_CONSTRUCTED )
	{

		RestoreFromTrigBits();

		//******* #10392
		// This trigger was destroyed earlier
		if( m_bMarkedForDeletion )
		{
			return;
		}
		//*******

		m_ResetTimeout = 0;
		m_SignalsExpected.clear();

		if (m_pOwnerGO->IsOmni())
		{
			EnterTheWorld(true);
		}
	}

	if ( m_bMessageTrigger && (m_bCurrentActiveState || (evt == WE_TRIGGER_ACTIVATE)))
	{
		ConditionDb::const_iterator i, ibegin = m_Conditions.begin(), iend = m_Conditions.end();
		for ( i = ibegin ; i != iend ; ++i )
		{

			gpassert(i->second->GetTrigger() == this);

			if (i->second->IsMessageHandler())
			{
				i->second->HandleMessage( *this, message );		
			}
		}
		if (m_bPureMessageTrigger || (evt == WE_KILLED) )
		{
			Update(gWorldTime.GetTime());
		}
	}

	if ( evt == WE_LEFT_WORLD)
	{
		if (!m_bPureMessageTrigger)
		{
			// Make sure we update one last time before we leave the world
			// ...just to be safe
			Update(gWorldTime.GetTime());
		}
		LeaveTheWorld();
	}
	else if ( evt == WE_TRIGGER_DEACTIVATE )
	{
		gpdevreportf( &gTriggerSysContext,("Trigger DEACTIVATE [%s:0x%08x:%04x:0x%08x]\n",
			GetOwner() ? GetOwner()->GetTemplateName() : "owner-STILL-not-known",
			GetOwner() ? MakeInt(GetOwner()->GetGoid()) : 0,
			GetOwner() ? GetTruid().GetTrigNum() : 0,
			GetOwner() ? MakeInt(GetOwner()->GetScid()) : 0));

		if (!m_bPureMessageTrigger)
		{
			// Make sure we update one last time before we go inactive			// We may have condtions that were waiting for this
			Update(gWorldTime.GetTime());
		}
		if (m_bCurrentActiveState)
		{
			if (gServer.IsLocal())
			{
				gTriggerSys.SDeactivateTrigger(m_Truid.GetOwnGoid(),m_Truid.GetTrigNum());
			}
		}
	} 
	else if ( evt == WE_DESTRUCTED )
	{
		PreserveInTrigBits();
	}
}

void
Trigger::Update( const double& CurrentTime )
{
	GPPROFILERSAMPLE( "0Trigger :: Update", SP_TRIGGER_MISC );

	gpassertm(GetTruid().IsValid(),("TRIGGER_SYS::Update() The trigger you are updating does not have a valid TRUID"));

	if( m_bMarkedForDeletion )
	{
		return;
	}

/*	This assert fires after teleports
#if !GP_RETAIL
	if (gServer.IsLocal())
	{
		gpassertf((m_SignalsExpected.size()<200) , (
				"Trigger [%s:0x%08x:%04x:%08x] is waiting for %d signals, that seems like too many",
				GetOwner() ? GetOwner()->GetTemplateName() : "owner-STILL-not-known",
				GetOwner() ? MakeInt(GetOwner()->GetGoid()) : 0,
				GetOwner() ? GetTruid().GetTrigNum() : 0,
				GetOwner() ? MakeInt(GetOwner()->GetScid()) : 0,
				m_SignalsExpected.size()
				));

		double check_time = gWorldTime.GetTime()-2.0;
		std::list<BoundaryCrossing>::iterator it = m_SignalsExpected.begin();
		for (;it!=m_SignalsExpected.end();++it)
		{
			gpassertf( ((*it).m_Time > check_time) , (
					"Trigger [%s:%08x:%04x:%08x] has a stale signal\n"
					"\twt:%f tt:%f [%s g:%08x s:%08x] %s #%d [%04x:%04x]\n",
						GetOwner() ? GetOwner()->GetTemplateName() : "owner-STILL-not-known",
						GetOwner() ? MakeInt(GetOwner()->GetGoid()) : 0,
						GetOwner() ? GetTruid().GetTrigNum() : 0,
						GetOwner() ? MakeInt(GetOwner()->GetScid()) : 0,
						gWorldTime.GetTime(),
						(*it).m_Time,
						GetGo((*it).m_Data) ? GetGo((*it).m_Data)->GetTemplateName() :  "<UNKNOWN GO>",
						(*it).m_Data,
						GetGo((*it).m_Data) ? MakeInt(GetGo((*it).m_Data)->GetScid()) : -1,
						ToString((*it).m_CrossType),
						(*it).m_SequenceNum,
						(*it).m_TriggerID.GetTrigNum(),
						(*it).m_ConditionID
					));
		}

	}
#endif
*/
	// Process the change in the active state
	if ( m_bRequestedActiveState != m_bCurrentActiveState )
	{
		if (m_bRequestedActiveState)
		{
			// Initialize all the occupants of the conditions & triggers!
			OnActivation();
			m_ResetTimeout = 0;
		}
		else
		{
			m_SignalsExpected.clear();
			if (gServer.IsLocal())
			{
				gTriggerSys.SDeactivateTrigger(m_Truid.GetOwnGoid(),m_Truid.GetTrigNum());
			}
		}
	}

	if ( !m_bCurrentActiveState )
	{
		return;
	}

	gpassert(!m_bMarkedForDeletion);
	
	ActionDb::iterator iAction;

	SubGroupIter sgit;

	if (m_bAllActionsExecuted)
	{
		if ( (m_ResetTimeout>0) && IsPositive(m_ResetDuration) )
		{
			if ( m_ResetTimeout < CurrentTime )
			{
				if (!m_SignalsExpected.empty())
				{
					// We still have signals expected, we have to stay in 
					// a reset
					m_ResetTimeout = CurrentTime;
				}
				else
				{
					Reset(CurrentTime);
				}
			}
		}
	}
	else
	{
		// Try to execute all the actions that have had their conditions met, but 
		// have not yet been executed due to a delay

		m_bAllActionsExecuted = true;

		// Run through all the sub groups, looking for actions that still need to be executed

		sgit = m_SubGroupStates.begin();
		for (;sgit != m_SubGroupStates.end(); ++sgit)
		{
			// Assume the sub group is NOT delayed
			(*sgit).second.second = false;

			for( iAction = m_Actions.begin() ; iAction != m_Actions.end(); ++iAction )
			{
				Params& parms = iAction->second.first;

				if (parms.SubGroup == (*sgit).first)
				{
					if (parms.CountdownTimeout > 0)
					{
						if (parms.CountdownTimeout <= CurrentTime)
						{
							Action* pAction = iAction->second.second;
							if ( gServer.IsLocal() || pAction->GetAllowClientExecution() )
							{
								GPPROFILERSAMPLE( "0Trigger :: Execute", SP_TRIGGER_EXEC );
								FuBi::AutoRpcNestDetectCancel autoCancel;
								pAction->Execute( *this, parms );
							}
							parms.CountdownTimeout = 0;
						}
						else
						{
							m_bAllActionsExecuted = false;
							(*sgit).second.second = true;	// This sub group IS delayed
						}
					}
					else
					{
						gpassert(0.0 == parms.CountdownTimeout);
					}
				}
			}
		}

		if ( !m_bIsFlipFlop && !m_bIsOneShot && m_bAllActionsExecuted && !IsZero(m_ResetDuration) )
		{
			m_ResetTimeout = PreciseAdd( CurrentTime, m_ResetDuration) ;
		}
		
		ClearUndelayedSubGroups();

		m_bOneShotHasTriggered |= m_bAllActionsExecuted && m_bIsOneShot;
		m_bMarkedForDeletion |=  m_bOneShotHasTriggered && !Services::IsEditor();

	}


	// Don't evaluate any conditions unless all m_bAllActionsExecuted is clear
	// Triggers with ON_UNIQUE_ENTER conditions are always evaluated

	if (  m_bAllActionsExecuted  &&
		 !m_bOneShotHasTriggered &&
		 !m_bMarkedForDeletion &&
		(m_bWaitsForConstruction || m_bUniqueEnterTrigger || (m_ResetTimeout < CurrentTime) || (CurrentTime ==0.0)))
	{
		bool all_groups_satisfied = !m_bIsFlipFlop;
		bool process_actions = false;

		// Run through all the sub groups, evaluating conditions for each subgroup
		sgit = m_SubGroupStates.begin();
		for (;sgit != m_SubGroupStates.end(); ++sgit)
		{
			if( m_bIsFlipFlop )
			{
				bool current_condition_state = EvaluateConditions( (*sgit).first );

				// IsFlipped gets XORed with current condition state
				bool flipflop = ((*sgit).second.first  || current_condition_state) && !((*sgit).second.first  && current_condition_state);

				// Are we flipping or flopping?
				if ( flipflop )
				{
					(*sgit).second.first = !(*sgit).second.first;
					process_actions = true;
				}
			}
			else
			{
				// If the condition is false
				if ( !(*sgit).second.first )
				{
					(*sgit).second.first = EvaluateConditions( (*sgit).first );
					process_actions |= (*sgit).second.first;
				}
				all_groups_satisfied &= (*sgit).second.first;
			}
		}

		if (process_actions)
		{
			m_bAllActionsExecuted = true;

			// Again we run through all the sub groups, executing all actions that are now true
			sgit = m_SubGroupStates.begin();
			for (;sgit != m_SubGroupStates.end(); ++sgit)
			{

				// Assume the sub group is NOT delayed
				(*sgit).second.second = false;

				iAction = m_Actions.begin();
				while( iAction != m_Actions.end() )
				{
					Params& parms = iAction->second.first;
					Action* pAction = iAction->second.second;

					// Let's be safe and increment the iterator NOW, just in case 
					// process of Executing(), it is somehow invalidated and removed
					++iAction;

					if (parms.SubGroup == (*sgit).first)
					{
						// Filter out the actions that don't match the
						// state of the condition evaluation 
						if (!(*sgit).second.first == parms.bWhenFalse)
						{
							float effdelay = pAction->EffectiveDelay(parms,m_InitialDelay);
							if ( effdelay > 0 )
							{
								// Save the occupants that have satisfied this condition
								parms.CountdownTimeout = PreciseAdd(CurrentTime, effdelay);
								m_bAllActionsExecuted = false;
								(*sgit).second.second = true;	// The sub group IS delayed
							}
							else
							{
								if ( gServer.IsLocal() || pAction->GetAllowClientExecution() )
								{
									GPPROFILERSAMPLE( "0Trigger :: Execute", SP_TRIGGER_EXEC );
									FuBi::AutoRpcNestDetectCancel autoCancel;
									pAction->Execute( *this, parms );
								}
								parms.CountdownTimeout = 0;
							}
						}
					}
					else if (parms.CountdownTimeout > 0) 
					{
						// We are still waiting for some earlier delayed actions
						// or sub groups that have not yet been triggered
						m_bAllActionsExecuted = false;
					}
				}			
			}
			
			if ( !m_bIsFlipFlop && !m_bIsOneShot && m_bAllActionsExecuted && !IsZero(m_ResetDuration) )
			{
				m_ResetTimeout = PreciseAdd( CurrentTime, m_ResetDuration);
			}

			ClearUndelayedSubGroups();

		}

		m_bOneShotHasTriggered |= all_groups_satisfied && m_bAllActionsExecuted && m_bIsOneShot;
		m_bMarkedForDeletion |=  m_bOneShotHasTriggered && !Services::IsEditor();
	}
	
}

bool Trigger::AddCondition( WORD id, Condition *pc )
{
	GPPROFILERSAMPLE( "0Trigger :: AddCondition", SP_TRIGGER_MISC );

	if (pc) 
	{
		bool valid_ordering = (!pc->IsMessageHandler() || (m_bMessageTrigger || m_Conditions.empty()));

		if (!valid_ordering)
		{
			gperrorf(("CONTENT_ERROR: Adding a message handling condition to a one-shot trigger that already has a non-message handling condition."
					  "This can be fixed by re-ordering the conditions so that any 'receive_world_message' appears before all other conditions"
					  ));
		}
		else
		{
			// Single shot triggers may not have any WHILE or EVERY condition checks

			// Convert conditions of one-shot triggers when either:
			// A) It is a message handling condition
			// B) There was no message handling condition, and this is not a message handler
			
			// In order for this to be parsed correctly, message handling conditions MUST preceed
			// non-message handlers.
			
			if (m_bIsOneShot)
			{
				if (pc->IsMessageHandler())
				{
					if (!pc->HasOneShotTest())
					{
						pc->ConvertToOneShotTest();
						//pc->AppendDocs("[*converted to one-shot*]");
					}
				}
				else if (!m_bMessageTrigger)
				{
					if (!pc->HasOneShotTest() && !pc->HasWhileInsideTest())
					{
						pc->ConvertToOneShotTest();
						//pc->AppendDocs("[*converted to one-shot*]");
					}
				}
			}
		}

		m_bMessageTrigger |= pc->IsMessageHandler();
		m_bSpatialTrigger |= pc->IsSpatial();

		gpassert((pc->GetID() & ~(0x8000|0x4000)) == 0);
		gpassert(m_Conditions.find(id) == m_Conditions.end());
		pc->SetID(id);
		id = pc->GetID();		// Flags will be added for certain triggers
		m_Conditions[id] = pc;
		return true;
	}
	else
	{
		return false;
	}
}

bool Trigger::RemoveCondition( WORD id )
{
	GPPROFILERSAMPLE( "0Trigger :: RemoveCondition", SP_TRIGGER_MISC );
	ConditionIter it = m_Conditions.find(id);
	if (it != m_Conditions.end())
	{
		if ((*it).second->IsGroupWatcher())
		{	
			gTriggerSys.RemoveGroupWatcher(	(*it).second->GetGroupName(), Cuid(m_Truid,(*it).first) );
		}
		delete (*it).second;
		m_Conditions.erase(it);
		return true;
	}
	else
	{
		return false;
	}
}

bool Trigger::AddAction( WORD id, Action* pa, const Params& parms )
{
	GPPROFILERSAMPLE( "0Trigger :: AddAction", SP_TRIGGER_MISC );

	if (pa) 
	{
		gpassert(id == parms.ParamID);
		gpassert(m_Actions.find(id) == m_Actions.end());
		m_Actions[id] = std::make_pair(parms,pa);
		return true;
	}
	else
	{
		return false;
	}
}

bool Trigger::RemoveAction( WORD id )
{
	GPPROFILERSAMPLE( "0Trigger :: RemoveAction", SP_TRIGGER_MISC );
	ActionIter it = m_Actions.find(id);
	if (it != m_Actions.end())
	{
		// The action does NOT own the pointer to the action definition
		m_Actions.erase(it);
		return true;
	}
	else
	{
		return false;
	}
}

void Trigger::LabelActions(  )
{
	GPPROFILERSAMPLE( "0Trigger :: LabelsActions", SP_TRIGGER_MISC );
	ActionIter it = m_Actions.begin();
	for (;it != m_Actions.end();++it)
	{
		(*it).second.second->Label(*this,(*it).second.first);
	}
}

bool Trigger::EvaluateConditions( int subgroup )
{
	GPPROFILERSAMPLE( "0Trigger :: EvaluateConditions", SP_TRIGGER_EVAL );
	// Passing in a negative value for a sub group will evaluate all conditions

	bool triggered = !m_Conditions.empty();

	// The conditions list is sorted so that "message handling conditions" are processed 
	// BEFORE any other conditions. One-Shot spatial triggers will not be processed until
	// the message is received

	ConditionDb::iterator iCondition = m_Conditions.begin();
	for(; triggered && iCondition != m_Conditions.end(); ++iCondition )
	{
		if( (subgroup < 0) || (subgroup == (*iCondition).second->GetSubGroup()) )
		{
			triggered = (*iCondition).second->Evaluate();
		}
	}

	if (!triggered)
	{
		// Wipe out any partial results
		ConditionDb::iterator i = m_Conditions.begin();
		for(; i != iCondition; ++i )
		{
			if( (subgroup < 0) || (subgroup == (*i).second->GetSubGroup()) )
			{
				if ( (*i).second->IsMessageHandler() )
				{
					(*i).second->ClearSatisfyingMessages();
				}
				else
				{
					(*i).second->ClearSatisfyingOccupants();
				}
			}
		}
	}

	return triggered;
}

void Trigger::ConvertConditionsToOneShot()
{
	GPPROFILERSAMPLE( "0Trigger :: ConvertConditionsToOneShot", SP_TRIGGER_EVAL );

	ConditionDb::iterator iCondition = m_Conditions.begin();
	for( ; iCondition != m_Conditions.end(); ++iCondition )
	{
		if ((*iCondition).second->HasOneShotTest())
		{
			(*iCondition).second->ConvertToOneShotTest();
			//(*iCondition).second->AppendDocs("[*converted to one-shot*]");
		}
	}
}



#if !GP_RETAIL
void
Trigger::Draw()
{
	GPPROFILERSAMPLE( "0Trigger :: Draw", SP_TRIGGER_MISC );

	if( m_bMarkedForDeletion )
	{
		return;
	}

	bool draw_solid = false;

	ConditionDb::iterator iCondition = m_Conditions.begin();

	for(; iCondition != m_Conditions.end(); ++iCondition )
	{
		draw_solid |= (*iCondition).second->HasBoundaryOccupants();

		 if ((*iCondition).second->IsGroupWatcher())
		 {
			SiegePos p1 = GetOwner()->GetPlacement()->GetPosition();
			p1.pos.y += 0.85f;

			bool group_draw_solid = false;
			SubGroupIter sgit = m_SubGroupStates.begin();
			for (;sgit != m_SubGroupStates.end(); ++sgit)
			{
				group_draw_solid |= (*sgit).second.first;
			}
		
			gWorld.DrawDebugWedge(p1, 0.10f , vector_3(0,0,1), 0 , m_bCurrentActiveState ? 0xFF66FF66 : 0xFFFF6666, group_draw_solid );
		 }

		(*iCondition).second->Draw(m_Conditions);
	}

	if (GetIsSpatial() && GetOwner() && GetOwner()->HasPlacement())
	{
		SiegePos p1 = GetOwner()->GetPlacement()->GetPosition();
		p1.pos.y += 0.85f;	
		gWorld.DrawDebugWedge(p1, 0.10f , vector_3(0,0,1), 0 , m_bCurrentActiveState ? 0xFF66FF66 : 0xFFFF6666, draw_solid );
	}

	ActionDb::iterator iAction = m_Actions.begin();

	for(; iAction != m_Actions.end(); ++iAction )
	{
		(*iAction).second.second->Draw( *this, (*iAction).second.first );
	}
}
#endif

void
Trigger::Reset(const double& CurrentTime)
{
	if (!m_SignalsExpected.empty())
	{
		m_ResetTimeout = CurrentTime;
		return;
	}

	if (m_bIsFlipFlop || m_bIsOneShot )
	{
		return;
	}

	m_ResetTimeout = 0;

	SubGroupIter sgit = m_SubGroupStates.begin();
	for (;sgit != m_SubGroupStates.end(); ++sgit)
	{
		(*sgit).second.first = false;
	}

	ConditionDb::iterator iCondition = m_Conditions.begin();
	for(; iCondition != m_Conditions.end(); ++iCondition )
	{
		(*iCondition).second->Reset();
	}

	m_bAllActionsExecuted = true;
	
}

void Trigger::IncrementExpectedSignals(const BoundaryCrossing& bc)
{
	m_SignalsExpected.insert(bc);
}


void Trigger::DecrementExpectedSignals(const BoundaryCrossing& bc, const double& current_time)
{

#if !GP_RETAIL
	bool found = !m_SignalsExpected.empty();
#endif

	SignalIter sigit = m_SignalsExpected.begin();
	for (;sigit!=m_SignalsExpected.end();++sigit)
	{
		if ((*sigit) == bc)
		{
			m_SignalsExpected.erase(sigit);
#if !GP_RETAIL
			found = true;
#endif
			break;
		}
	}
	
#if !GP_RETAIL
	if (!found)
	{
		gpstring errtxt;
		errtxt.assignf(
			"TRIGGER_SYS: UNEXPECTED SIGNAL [%s g:0x%08x s:0x%08x] %f %s #%d\n"
			"\t Trigger: [%s:%08x:%04x:%04x]\n",
			GetGo(MakeGoid(bc.m_Data)) ? GetGo(MakeGoid(bc.m_Data))->GetTemplateName() :  "<UNKNOWN GO>",
			bc.m_Data,
			GetGo(MakeGoid(bc.m_Data)) ? MakeInt(GetGo(MakeGoid(bc.m_Data))->GetScid()) : -1,
			bc.m_Time,
			ToString(bc.m_CrossType),
			bc.m_SequenceNum,
			GetOwner()->GetTemplateName(),
			GetOwner()->GetScid(),
			bc.m_TriggerID.GetTrigNum(),
			bc.m_ConditionID
		);
		if (m_SignalsExpected.empty())
		{
			errtxt.appendf("\t\tThere are NO expected signals\n");
		}
		else
		{
			errtxt.appendf("\t\tExpected signals are:\n");
			SignalIter i = m_SignalsExpected.begin();
			for (;i!=m_SignalsExpected.end();++i)
			{
				errtxt.appendf("\t\t\t%f: [%s g:0x%08x s:0x%08x] %s #%d [%04x:%04x]\n",
					(*i).m_Time,
					GetGo(MakeGoid((*i).m_Data)) ? GetGo(MakeGoid((*i).m_Data))->GetTemplateName() :  "<UNKNOWN GO>",
					(*i).m_Data,
					GetGo(MakeGoid((*i).m_Data)) ? MakeInt(GetGo(MakeGoid((*i).m_Data))->GetScid()) : -1,
					ToString((*i).m_CrossType),
					(*i).m_SequenceNum,
					(*i).m_TriggerID.GetTrigNum(),
					(*i).m_ConditionID
					);
			}
		}
		gpdevreportf( &gTriggerSysContext, (errtxt.c_str()) );
	}
#endif

	if ( (m_SignalsExpected.empty()) && (current_time>=0) )
	{
		if (m_bAllActionsExecuted && (m_ResetTimeout>0))
		{
			if (current_time > m_ResetTimeout)
			{
				Reset(current_time);
			}
		}
	}
}

int	Trigger::GetNumExpectedSignals(WORD condid)
{
	int i = 0;
	SignalConstIter sigit = m_SignalsExpected.begin();
	for (;sigit!=m_SignalsExpected.end();++sigit)
	{
		if ((*sigit).m_ConditionID == condid)
		{
			i++;
		}
	}
	return i;
}


Goid
Trigger::GetOwnerGoid() const
{
	GPPROFILERSAMPLE( "0Trigger :: GetOwnerGoid", SP_TRIGGER_MISC );

	return (m_pOwnerGO && m_pOwnerGO->HasValidGoid()) ? m_pOwnerGO->GetGoid() : GOID_INVALID;
}



bool
Trigger::GetConditionInfo( WORD id, gpstring &sName, Parameter& pFormat, Params& pParams )
{
	GPPROFILERSAMPLE( "0Trigger :: GetConditionInfo", SP_TRIGGER_MISC );

	ConditionIter it = m_Conditions.find(id);

	if (it == m_Conditions.end())
	{
		return false;
	}

	sName = (*it).second->GetName();
	(*it).second->FetchParameterList(pFormat);
	(*it).second->FetchParameterValues(pParams);

	return true;
}

bool
Trigger::SetConditionInfo( WORD id, Params& pParams )
{
	GPPROFILERSAMPLE( "0Trigger :: GetConditionInfo", SP_TRIGGER_MISC );

	ConditionIter it = m_Conditions.find(id);

	if (it == m_Conditions.end())
	{
		return false;
	}

	(*it).second->StoreParameterValues(pParams);

	UpdateTriggerFlags();

	return true;
}



bool
Trigger::GetActionInfo( WORD id, gpstring &sName, Parameter const * &pFormat, Params const * &pParams )
{
	GPPROFILERSAMPLE( "0Trigger :: GetActionInfo", SP_TRIGGER_MISC );

	ActionIter it = m_Actions.find(id);

	if (it == m_Actions.end())
	{
		return false;
	}

	sName = (*it).second.second->GetName();
	pFormat = (*it).second.second->GetFormat();
	pParams = &(*it).second.first;

	return true;
}

bool
Trigger::GetMustCreateOnClients() const
{
	GPPROFILERSAMPLE( "0Trigger :: GetMustCreateOnClients", SP_TRIGGER_MISC );

	return IsClientTrigger();
}

bool
Trigger::ExecuteActions( float SecondsSinceLastCall )
{
	UNREFERENCED_PARAMETER(SecondsSinceLastCall);
/*
// DISABLED!!!! This was a "SiegeEditor Only" function
// that was broken at some point before I started to 
// make changes (according to Chad)
//
// There are some nasty logic/interface issues that
// must be resolved in order for SE to directy execute
// trigger actions --biddle

	// Execute any delayed actions
	if( !m_bAllActionsExecuted )
	{
		m_bAllActionsExecuted = true;
		ActionDb::iterator iAction = m_Actions.begin();
		for(; iAction != m_Actions.end(); ++iAction )
		{
			Params parms = (*iAction).second.first;
			if (parms.CountdownTimeout > 0)
			{
				parms.CountdownTimeout -= SecondsSinceLastCall;
				if ( parms.CountdownTimeout > 0 )
				{
					m_bAllActionsExecuted = false;
				}
				else
				{
					(*iAction).second.second->Execute( *this, parms, NULL );
					parms.CountdownTimeout = 0;
				}
			}
		}
		if ( !m_bAllActionsExecuted )
		{
			return false;
		}
	}

	if( !m_bIsFlipFlop )
	{

		if( m_bHasTriggered && m_bIsOneShot )
		{
			return true;
		}

		if( m_bHasTriggered )
		{
			if( m_Elapsed >= m_ResetDuration )
			{
				Reset();
			}
			else
			{
				return true;
			}
		}

		m_Elapsed = 0;
		m_bHasTriggered = true;
		m_bAllActionsExecuted = true;

		ActionDb::iterator iAction = m_Actions.begin();
		for(; iAction != m_Actions.end(); ++iAction )
		{
			Params parms = (*iAction).second.first;
			if (( parms.strings.size() >= 2 ) &&
				( parms.strings[1].same_no_case( "result_of_condition" ) ) )
			{
				parms.strings[1] = "default";
			}

			parms.CountdownTimeout = parms.Delay;
			if ( parms.Delay > 0 )
			{
				m_bAllActionsExecuted = false;
				parms.bReadyToExecute = true;
			}
			else
			{
				(*iAction).second.second->Execute( *this, parms, NULL );
				parms.bReadyToExecute = false;
			}
		}

		return m_bAllActionsExecuted;
	}
	// Handle flip-flop trigger
	else
	{
		// Are we flipping or flopping?
		if( m_bHasTriggered  == false )
		{
			// Flip
			m_bHasTriggered = true;

			// Trigger only true state actions
			ActionDb::iterator iAction = m_Actions.begin();
			for(; iAction != m_Actions.end(); ++iAction )
			{
				// Filter out when_false actions
				Params parms = (*iAction).second.first;
				if( parms.bWhenFalse == false )
				{
					(*iAction).second.second->Execute( *this, parms, NULL );
				}
			}

		}
	}
*/
	return true;;
}


// Trigger Groups

bool Trigger::AddWatchedByCondition(Condition* watchcond)
{
	GPPROFILERSAMPLE( "0Trigger :: AddWatchedByCondition", SP_TRIGGER_MISC );

	bool ret = false;

	ConditionSetIter it = m_WatchedByConditions.find(watchcond);

	if (it == m_WatchedByConditions.end())
	{
		m_WatchedByConditions.insert(watchcond);
		ret = true;
	}
	else
	{
		ret = gGoDb.IsEditMode();
	}

	return ret;
}

bool Trigger::RemoveWatchedByCondition(Condition* watchcond)
{
	GPPROFILERSAMPLE( "0Trigger :: RemoveWatchedByCondition", SP_TRIGGER_MISC );

	bool ret = false;

	ConditionSetIter it = m_WatchedByConditions.find(watchcond);
	gpassert(gGoDb.IsEditMode() || (it != m_WatchedByConditions.end()));

	if (it != m_WatchedByConditions.end())
	{
		m_WatchedByConditions.erase(it);
		ret = true;
	}
	else
	{
		ret = gGoDb.IsEditMode();
	}

	return ret;
}

// Spatial Collisions & Signal Messaging 

void Trigger::OnActivation( )
{
	GPPROFILERSAMPLE( "0Trigger :: OnActivation", SP_TRIGGER_EVAL );
	
	if (gServer.IsLocal())
	{
		double wt = gWorldTime.GetTime();
		SignalSet insiders;
		 
		for(ConditionIter cit=m_Conditions.begin(); cit!=m_Conditions.end() ; ++cit )
		{
			GopColl newbies;
			if ((*cit).second->CollectInsideBoundary( newbies ))
			{
				for (GopColl::iterator git = newbies.begin(); git!=newbies.end(); ++git)
				{
					Goid ingoid = (*git)->GetGoid();
					BoundaryCrossing bc(wt,m_Truid,MakeInt(ingoid), BCROSS_ENTER_FORCED, (*cit).first, gTriggerSys.NextSeqNum());
					insiders.insert(bc);
					IncrementExpectedSignals(bc);
					CollectWatchersOfCollision((*git),wt,true,BCROSS_ENTER_FORCED,insiders);
					gMCP.AppendTriggerSignals(ingoid,insiders);
					insiders.clear();
				}
			}
		}
	}

	m_bRequestedActiveState = true;
	m_bCurrentActiveState = true;

}

void Trigger::OnDeactivation( )
{
	GPPROFILERSAMPLE( "0Trigger :: OnDeactivation", SP_TRIGGER_EVAL );

	for(ConditionIter cit=m_Conditions.begin(); cit!=m_Conditions.end() ; ++cit )
	{
		GoOccupantMapIter git = (*cit).second->GetBoundaryOccupants()->begin();
		while ( git != (*cit).second->GetBoundaryOccupants()->end() )
		{
			Goid stalegoid = (*git).first;	
	
			for(ConditionSetIter sit=m_WatchedByConditions.begin(); sit!=m_WatchedByConditions.end() ; ++sit )
			{
				GoOccupantMapIter wgit;
				(*sit)->RemoveOccupantFromBoundary(stalegoid,true,wgit);
			}

			(*cit).second->RemoveOccupantFromBoundary(stalegoid,true,git);
			
		}
	}

	m_bCurrentActiveState = false;
	m_bRequestedActiveState = false;
}

void Trigger::NewGoEntering(const double& in_Time, const Go* in_Newbie, DWORD in_NewFrustum, SignalSet& inout_hitlist)
{
	GPPROFILERSAMPLE( "0Trigger :: NewGoEntering", SP_TRIGGER_EVAL );

	if (in_Newbie->HasPlacement())
	{

		Goid newbiegoid = in_Newbie->GetGoid();
		const SiegePos& init_pos = in_Newbie->GetPlacement()->GetPosition();

		ConditionDb::iterator it = m_Conditions.begin();
		
		bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & in_NewFrustum) != 0;
		for(; (it!=m_Conditions.end()) ; ++it )
		{
			Condition* c = (*it).second; 
			if (c->IsSpatial() &&
				(match || !c->IsFrustumSpecific())  &&
			 	c->CheckForInsideBoundary( in_Newbie, init_pos ))
			{
				BoundaryCrossing bc(in_Time,m_Truid,MakeInt(newbiegoid), BCROSS_ENTER_FORCED, (*it).first,gTriggerSys.NextSeqNum()); 
				inout_hitlist.insert(bc); 
				IncrementExpectedSignals(bc);
				CollectWatchersOfCollision(in_Newbie,in_Time,true,BCROSS_ENTER_FORCED,inout_hitlist);
			}
		}
	}
}

void Trigger::StaleGoLeaving(const double& in_Time, const Go* in_Stale, SignalSet& inout_hitlist)
{
	GPPROFILERSAMPLE( "0Trigger :: StaleGoLeaving", SP_TRIGGER_EVAL );

	Goid stalegoid = in_Stale->GetGoid();

	bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & (DWORD)in_Stale->GetWorldFrustumMembership()) != 0;

	ConditionDb::iterator it = m_Conditions.begin();
	for(; (it!=m_Conditions.end()) ; ++it )
	{
		Condition* c = (*it).second; 
		if (c->IsSpatial() && 
			(match || !c->IsFrustumSpecific())  &&
			c->IsBoundaryOccupant( stalegoid ))
		{
			BoundaryCrossing bc(in_Time,m_Truid,MakeInt(stalegoid), BCROSS_LEAVE_FORCED, (*it).first,gTriggerSys.NextSeqNum()); 
			inout_hitlist.insert(bc); 
			IncrementExpectedSignals(bc);
			CollectWatchersOfCollision(in_Stale,in_Time,false,BCROSS_LEAVE_FORCED,inout_hitlist);
		}
	}
}

void Trigger::DestroyedGoRemoval(Goid deadgoid)
{
	GPPROFILERSAMPLE( "0Trigger :: DestroyedGoRemoval", SP_TRIGGER_EVAL );

	ConditionDb::iterator it = m_Conditions.begin();
	for(; (it!=m_Conditions.end()) ; ++it )
	{
		Condition* c = (*it).second; 
		if (c->IsSpatial())
		{
			if (c->IsBoundaryOccupant(deadgoid) || c->IsSatisfyingOccupant(deadgoid))
			{
				if (c->RemoveOccupant( deadgoid ))
				{
					m_SubGroupStates[c->GetSubGroup()] = SubGroupInfo( false, false );
				}
			}
		}
	}
}

void Trigger::TeleportingGo(const double& in_Time, const Go* in_Jumper, const SiegePos& in_JumpPos, SignalSet& inout_hitlist)
{
	GPPROFILERSAMPLE( "0Trigger :: TeleportingGo", SP_TRIGGER_EVAL );

	Goid jumpergoid = in_Jumper->GetGoid();

	ConditionDb::iterator it = m_Conditions.begin();

	bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & (DWORD)in_Jumper->GetWorldFrustumMembership()) != 0;
	for(; (it!=m_Conditions.end()) ; ++it )
	{
		Condition* c = (*it).second; 
		if ((*it).second->IsSpatial() && (match || !c->IsFrustumSpecific()))
		{
			if ((*it).second->IsBoundaryOccupant( jumpergoid ))
			{
				if (!(*it).second->CheckForInsideBoundary( in_Jumper, in_JumpPos ))
				{
					// We teleporting out of this condition
					BoundaryCrossing bc(in_Time,m_Truid,MakeInt(jumpergoid), BCROSS_LEAVE_FORCED, (*it).first, gTriggerSys.NextSeqNum()); 
					inout_hitlist.insert(bc); 
					IncrementExpectedSignals(bc);
					CollectWatchersOfCollision(in_Jumper,in_Time,false,BCROSS_LEAVE_FORCED,inout_hitlist);
				}
			}
			else
			{
				if ((*it).second->CheckForInsideBoundary( in_Jumper, in_JumpPos ))
				{
					// We teleporting into this condition
					BoundaryCrossing bc(in_Time,m_Truid,MakeInt(jumpergoid), BCROSS_ENTER_FORCED, (*it).first, gTriggerSys.NextSeqNum()); 
					inout_hitlist.insert(bc); 
					IncrementExpectedSignals(bc);
					CollectWatchersOfCollision(in_Jumper,in_Time,true,BCROSS_ENTER_FORCED,inout_hitlist);
				}
			}
		}
	}
}

void Trigger::GoJoinedWithParty(const double& in_Time, const Go* in_Buddy, SignalSet& inout_hitlist)
{
	GPPROFILERSAMPLE( "0Trigger :: GoJoinedWithParty", SP_TRIGGER_EVAL );

	Goid buddygoid = in_Buddy->GetGoid();
	const SiegePos& buddy_pos = in_Buddy->GetPlacement()->GetPosition();

	bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & (DWORD)in_Buddy->GetWorldFrustumMembership()) != 0;

	ConditionDb::iterator it = m_Conditions.begin();
	for(; (it!=m_Conditions.end()) ; ++it )
	{
		Condition* c = (*it).second; 
		if (c->IsPartySnooper() && (match || !c->IsFrustumSpecific()))
		{
			// Make sure we don't count this one twice!
			if (!c->IsBoundaryOccupant( buddygoid ))
			{
				if (c->CheckForInsideBoundary( in_Buddy, buddy_pos ))
				{
					BoundaryCrossing bc(in_Time,m_Truid,MakeInt(buddygoid), BCROSS_ENTER_FORCED, (*it).first, gTriggerSys.NextSeqNum()); 
					inout_hitlist.insert(bc);
					IncrementExpectedSignals(bc);
					CollectWatchersOfCollision(in_Buddy,in_Time,true,BCROSS_ENTER_FORCED,inout_hitlist);
				}
			}
		}
	}
}

void Trigger::GoRemovedFromParty(const double& in_Time, const Go* in_Estranged, SignalSet& inout_hitlist)
{
	GPPROFILERSAMPLE( "0Trigger :: GoRemovedFromParty", SP_TRIGGER_EVAL );

	Goid estrangedgoid = in_Estranged->GetGoid();

	bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & (DWORD)in_Estranged->GetWorldFrustumMembership()) != 0;

	ConditionDb::iterator it = m_Conditions.begin();
	for(; (it!=m_Conditions.end()) ; ++it )
	{
		Condition* c = (*it).second; 
		if (c->IsPartySnooper() && (match || !c->IsFrustumSpecific()))
		{
			if (c->IsBoundaryOccupant( estrangedgoid ))
			{
				BoundaryCrossing bc(in_Time,m_Truid,MakeInt(estrangedgoid), BCROSS_LEAVE_FORCED, (*it).first, gTriggerSys.NextSeqNum()); 
				inout_hitlist.insert(bc);
				IncrementExpectedSignals(bc);
				CollectWatchersOfCollision(in_Estranged,in_Time,false,BCROSS_LEAVE_FORCED,inout_hitlist);
			}
		}
	}
}

void Trigger::EnterWatchersOfTrigger( Goid in_Newbie )
{
	GPPROFILERSAMPLE( "0Trigger :: EnterWatchersOfOccupants", SP_TRIGGER_EVAL );
	for(ConditionSetIter sit=m_WatchedByConditions.begin(); sit!=m_WatchedByConditions.end() ; ++sit )
	{
		(*sit)->GetTrigger()->OnGoEntersBoundary(in_Newbie, 0, (*sit)->GetID());
	}
}

void Trigger::LeaveWatchersOfTrigger( Goid in_Stale )
{
	GPPROFILERSAMPLE( "0Trigger :: LeaveWatchersOfOccupants", SP_TRIGGER_EVAL );
	for(ConditionSetIter sit=m_WatchedByConditions.begin(); sit!=m_WatchedByConditions.end() ; ++sit )
	{
		(*sit)->GetTrigger()->OnGoLeavesBoundary(in_Stale, 0, (*sit)->GetID());
	}
}

#if !GP_RETAIL
int Trigger::NumInsideAfterAllPendingCrossings(Goid in_MoverGoid,
											   WORD in_CondID,
											   int  in_StartCount,
											   bool in_UniqueTest,
											   bool	in_GroupWatcher
											   ) const
#else
int Trigger::NumInsideAfterAllPendingCrossings(Goid in_MoverGoid,
											   WORD in_CondID,
											   int  in_StartCount
											   ) const
#endif
{
#if !GP_RETAIL
	if (in_StartCount == 0)
	{
		in_UniqueTest = false;
	}
#endif
	if (!m_SignalsExpected.empty())
	{
		SignalConstIter sigit = m_SignalsExpected.begin();
		for (;sigit != m_SignalsExpected.end();++sigit)
		{
			if ((MakeGoid((*sigit).m_Data) == in_MoverGoid) && ((*sigit).m_ConditionID == in_CondID))
			{
				if (IsEnteringAtCrossing((*sigit).m_CrossType))
				{
					in_StartCount++;
				}
				else
				{
					in_StartCount--;
				}
			}
		}
	}
#if !GP_RETAIL
	if (in_UniqueTest)
	{
		if (!(in_StartCount>=0))
		{
			gpdevreportf( &gTriggerSysContext,(
									"TriggerSys: NumInsideAfterAllPendingCrossings (unique) counted %d (<0)\n"
									"\t\t[%s:g:%08x:s:%08x:%04x] [%s g:%08x s:%08x]\n",		
										in_StartCount,
										GetOwner()->GetTemplateName(),
										GetTruid().GetOwnGoid(),	
										GetOwner()->GetScid(),		
										GetTruid().GetTrigNum(),	
										GetGo(in_MoverGoid)->GetTemplateName(),
										in_MoverGoid,					
										GetGo(in_MoverGoid)->GetScid())
										);
		}
		if ( ! (in_GroupWatcher || (in_StartCount<3) ) )
		{
			gpdevreportf( &gTriggerSysContext,(
									"TriggerSys: NumInsideAfterAllPendingCrossings (unique) counted %d (>2)\n"
									"\t\t[%s:g:%08x:s:%08x:%04x] [%s g:%08x s:%08x]\n",		
										in_StartCount,
										GetOwner()->GetTemplateName(),
										GetTruid().GetOwnGoid(),	
										GetOwner()->GetScid(),		
										GetTruid().GetTrigNum(),	
										GetGo(in_MoverGoid)->GetTemplateName(),
										in_MoverGoid,					
										GetGo(in_MoverGoid)->GetScid())
										);
		}
	}
	else
	{
		if (!(in_StartCount>=0))
		{
			gpdevreportf( &gTriggerSysContext,(
									"TriggerSys: NumInsideAfterAllPendingCrossings (plain) counted %d (<0)\n"
									"\t\t[%s:g:%08x:s:%08x:%04x] [%s g:%08x s:%08x]\n",		
										in_StartCount,
										GetOwner()->GetTemplateName(),
										GetTruid().GetOwnGoid(),	
										GetOwner()->GetScid(),		
										GetTruid().GetTrigNum(),	
										GetGo(in_MoverGoid)->GetTemplateName(),
										in_MoverGoid,					
										GetGo(in_MoverGoid)->GetScid())
										);
		}

		if (!(in_GroupWatcher || in_StartCount<2))
		{
			gpdevreportf( &gTriggerSysContext,(
									"TriggerSys: NumInsideAfterAllPendingCrossings (plain) counted %d (>1)\n"
									"\t\t[%s:g:%08x:s:%08x:%04x] [%s g:%08x s:%08x]\n",		
										in_StartCount,
										GetOwner()->GetTemplateName(),
										GetTruid().GetOwnGoid(),	
										GetOwner()->GetScid(),		
										GetTruid().GetTrigNum(),	
										GetGo(in_MoverGoid)->GetTemplateName(),
										in_MoverGoid,					
										GetGo(in_MoverGoid)->GetScid())
										);
		}
	}
#endif
	return in_StartCount;
}

void Trigger::CollectTriggerCollisions(const Go*       in_Mover,
									   const SiegePos& in_PosA,
									   const double&   in_TimeA,
									   const SiegePos& in_PosB, 
									   const double&   in_TimeB,
									   SignalSet&      inout_hitlist )
{
	GPPROFILERSAMPLE( "0Trigger :: CollectTriggerCollisions", SP_TRIGGER_EVAL );

	double time_diff = PreciseSubtract(in_TimeB, in_TimeA);

	if (IsZero((float)time_diff))
	{
		if (!IsZero(gSiegeEngine.GetDifferenceVector(in_PosA,in_PosB)))
		{

			// The go is teleporting!
			Goid movergoid = in_Mover->GetGoid();

			ConditionDb::iterator it = m_Conditions.begin();
			
			// Need to run through the conditions twice so that we can be sure to insert
			// exits AFTER the 

			bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & (DWORD)in_Mover->GetWorldFrustumMembership()) != 0;
			for(; (it!=m_Conditions.end()) ; ++it )
			{
				Condition* c = (*it).second; 
				if (c->IsSpatial() && (match || !c->IsFrustumSpecific()))
				{

					bool leaves = c->IsBoundaryOccupant( movergoid );				
					bool enters = c->CheckForInsideBoundary( in_Mover, in_PosB );
					if (enters != leaves)
					{
						WORD conid = (*it).first;
						if (leaves)
						{
							// We teleporting out of this condition
							BoundaryCrossing bc(in_TimeB,m_Truid,MakeInt(movergoid), BCROSS_LEAVE_AT_TIME, conid, gTriggerSys.NextSeqNum()); 
							inout_hitlist.insert(bc); 
							IncrementExpectedSignals(bc);
							CollectWatchersOfCollision(in_Mover,in_TimeA,false,BCROSS_LEAVE_AT_TIME,inout_hitlist);
						}
						if (enters)
						{
							// We teleporting into this condition
							BoundaryCrossing bc(in_TimeB,m_Truid,MakeInt(movergoid), BCROSS_ENTER_AT_TIME, conid, gTriggerSys.NextSeqNum()); 
							inout_hitlist.insert(bc); 
							IncrementExpectedSignals(bc);
							CollectWatchersOfCollision(in_Mover,in_TimeA,true,BCROSS_ENTER_AT_TIME,inout_hitlist);
						}
					}
				}
			}

		}
	}
	else
	{
		Goid movergoid = in_Mover->GetGoid();
		
		bool match = ((DWORD)GetOwner()->GetWorldFrustumMembership() & (DWORD)in_Mover->GetWorldFrustumMembership()) != 0;
		ConditionDb::iterator it = m_Conditions.begin();
		for(; (it!=m_Conditions.end()) ; ++it )
		{
			Condition* c = (*it).second; 
			if (c->IsSpatial() && (match || !c->IsFrustumSpecific()))
			{
				WORD conid = (*it).first;
				float tEnter,tLeave;
				bool bAlreadyInside = false;
				bool bOutsideAfterwards = false;
				
				bool hit = c->CheckForBoundaryCollision( 
													in_Mover,
													in_PosA,
													in_PosB,
													tEnter,
													tLeave,
													bAlreadyInside,
													bOutsideAfterwards
													);

				bool currently_dormant = c->IsDormant();

				bool currently_inside = NumInsideAfterAllPendingCrossings(
												movergoid,
												conid,
												c->GetBoundaryOccupantCount(movergoid)
#if !GP_RETAIL
												,c->HasUniqueEnterTest()
												,c->IsGroupWatcher()
#endif
												) > 0;
				

				if (bAlreadyInside && !bOutsideAfterwards)
				{
					// CASE1: We stay inside
					// Verify that we are in the occlist
					// Must not see any hits
					// ** MAY want to check for a LEAVE then ENTER???

#define BLAHBLAHBLAH																	\
							"\t[%s:g:%08x:s:%08x:%04x:%04x] [%s g:%08x s:%08x]\n",		\
							c->GetName(),												\
							c->GetTrigger()->GetOwner()->GetTemplateName(),				\
							c->GetTrigger()->GetTruid().GetOwnGoid(),					\
							c->GetTrigger()->GetOwner()->GetScid(),						\
							c->GetTrigger()->GetTruid().GetTrigNum(),					\
							c->GetID(),													\
							in_Mover->GetTemplateName(),								\
							movergoid,													\
							in_Mover->GetScid()																		


					if (hit)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE1 %s FALSE HIT DETECTED WHILE INSIDE!\n"
							BLAHBLAHBLAH
							));
						hit = false;
						tEnter = -2.0;
						tLeave =  2.0;
					}

					if (!currently_inside && !currently_dormant)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE1 %s WE HAVE SKIPPED INSIDE A TRIGGER BOUNDARY!\n"
							BLAHBLAHBLAH
							));
						hit = true;
						tEnter = 0.0f;
					}

				}
				else if (bAlreadyInside && bOutsideAfterwards)
				{
					// CASE2: We move from in to out
					// Verify that we are in the occlist
					// Must see a LEAVE hit

					if (hit)
					{
						if (tLeave>1)
						{
							tLeave = 1.0f;
						}					
						tEnter = -2.0f; 
					}
					else if (!currently_dormant)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE2 %s LEAVING HIT MISSED WHILE INSIDE!\n"
							BLAHBLAHBLAH
							));
						hit = true;
						tLeave = 1.0f;
					}

					if (!currently_inside && !currently_dormant)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE2 %s WE HAVE SKIPPED INSIDE A TRIGGER BOUNDARY!\n"
							BLAHBLAHBLAH
							));
						hit = true;
						tEnter = 0.0f;
					}
					
				}
				else if (!bAlreadyInside && !bOutsideAfterwards)
				{
					// CASE3: We move from out to in
					// Verify that we are NOT in the occlist
					// Must see an ENTER hit

					if (hit)
					{
						if (tEnter<0)
						{
							tEnter = 0.0f;
						}					
						tLeave = 2.0f; 
					}
					else if (!currently_dormant)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE3 %s ENTERING HIT MISSED WHILE OUTSIDE!\n"
							BLAHBLAHBLAH
							));
						hit = true;
						tEnter = 0.0f;
					}

					if (currently_inside && !c->HasUniqueEnterTest() && !currently_dormant)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE3 %s WE HAVE SKIPPED INSIDE A TRIGGER BOUNDARY!\n"
							BLAHBLAHBLAH
							));
						hit = false;
						tEnter = -2.0f;
					}
				}
				else // (!bAlreadyInside && bOutsideAfterwards)
				{
					// CASE4: We stay outside
					// Verify that we are NOT in the occlist
					// Must see NO hits, or BOTH an enter and leave

					if (hit)
					{
						if (tEnter<0)
						{
							tEnter = 0.0f;
						}					
						if (tLeave>1)
						{
							tLeave = 1.0f;
						}					
					}
					else
					{
						tEnter = -2.0f;
						tLeave =  2.0f;
					}

					if (currently_inside && !c->HasUniqueEnterTest() && !currently_dormant)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: HIT_CASE4 %s WE HAVE SKIPPED OUTSIDE A TRIGGER BOUNDARY!\n"
							BLAHBLAHBLAH
							));

						if (hit)
						{
							tEnter = -2.0;
						}
						else
						{
							hit = true;
							tLeave = 1.0f;
						}
					}
				}

				if (hit)
				{
					if (tEnter>=0.0f)
					{
						eBoundaryCrossType cross_type = c->GetEnterCrossType();
						double enter_time = PreciseAdd(in_TimeA, PreciseMultiply(time_diff, tEnter));
						BoundaryCrossing bc(enter_time,m_Truid,MakeInt(movergoid), cross_type, conid, gTriggerSys.NextSeqNum()); 
						inout_hitlist.insert(bc); 
						IncrementExpectedSignals(bc);
						CollectWatchersOfCollision(in_Mover,enter_time,true,cross_type,inout_hitlist);
					}
					if (tLeave<=1.0f)
					{
						eBoundaryCrossType cross_type = c->GetLeaveCrossType();
						double leave_time = PreciseAdd(in_TimeA, PreciseMultiply(time_diff, tLeave));
						BoundaryCrossing bc(leave_time,m_Truid,MakeInt(movergoid), cross_type, conid, gTriggerSys.NextSeqNum()); 
						inout_hitlist.insert(bc);
						IncrementExpectedSignals(bc);
						CollectWatchersOfCollision(in_Mover,leave_time,false,cross_type,inout_hitlist);
					}
				}
			}
		}
	}
}

void Trigger::CollectWatchersOfCollision(const Go*			in_Mover,
										 const double&		in_HitTime,
										 bool				in_IsEntering,
										 eBoundaryCrossType	in_CrossType,
										 SignalSet&			inout_hitlist)
{
	GPPROFILERSAMPLE( "0Trigger :: CollectWatchersOfCollision", SP_TRIGGER_EVAL );

	if (!m_WatchedByConditions.empty())
	{
		ConditionSetIter wcit = m_WatchedByConditions.begin();
		Goid movergoid = in_Mover->GetGoid();
		for (; wcit !=m_WatchedByConditions.end(); ++wcit)
		{
			// Is the watcher active or about to become active?
			if ( (*wcit)->GetTrigger()->GetCurrentActive() || (*wcit)->GetTrigger()->GetRequestedActive() )
			{
				// This boundary test will make sure that the in_Mover is an object that
				// the watcher is interested in
				float d0,d1;

				// Use the entering/exiting parameters to signal whether or not
				// what type of collision we have
				if (in_IsEntering)
				{
					d0 = 1.0f;
				}
				else
				{
					d0 = 0.0f;
				}

				Condition* c = (*wcit);

				bool dummy1;
				bool dummy2;
				bool hit = (c->CheckForBoundaryCollision( 
												in_Mover, 
												SiegePos::INVALID,
												SiegePos::INVALID,
												d0,
												d1,
												dummy1,
												dummy2));


				int curr_inside_count = c->GetTrigger()->NumInsideAfterAllPendingCrossings(
												movergoid,
												c->GetID(),
												c->GetBoundaryOccupantCount(movergoid)
#if !GP_RETAIL
												,c->HasUniqueEnterTest()
												,c->IsGroupWatcher()
#endif
												);

				if (hit)
				{

					if ((curr_inside_count==0) && !in_IsEntering)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: WATCHER_HIT_CASE1 %s WE ARE LEAVING A WATCHER THAT WE AREN'T INSIDE!\n"
							BLAHBLAHBLAH
							));
					}
					else if ( ( (curr_inside_count+1) > c->GetGroupSize()) && in_IsEntering)
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: WATCHER_HIT_CASE2 %s WE ARE ENTERING A WATCHER TOO MANY TIMES!\n"
							BLAHBLAHBLAH
							));
					}
					else
					{
						if ( curr_inside_count<0 )
						{
							gpdevreportf( &gTriggerSysContext,(
								"TRIGGER_SYS: WATCHER_HIT_CASE3 %s THE WATCHER'S CURRENTLY_INSIDE COUNT IS NEGATIVE!\n"
								BLAHBLAHBLAH
								));
						}
					
						WORD wcondid = c->GetID();
						Truid wtruid = c->GetTrigger()->GetTruid();
						BoundaryCrossing bc(in_HitTime,wtruid, MakeInt(movergoid) , in_CrossType, wcondid, gTriggerSys.NextSeqNum()); 
						inout_hitlist.insert(bc); 
						c->GetTrigger()->IncrementExpectedSignals(bc);
					}
				}
			}
		}
	}
}

void Trigger::NotifyWatchersOfCrossingError(const Goid		in_BadGoid,
										    const double&	in_BadTime)
{
	ConditionSetIter wit = m_WatchedByConditions.begin();
	for (; wit!=m_WatchedByConditions.end(); ++wit)
	{
		(*wit)->AddIllegalOccupantCrossing(in_BadTime,in_BadGoid);
	}
}

bool Trigger::OnGoEntersBoundary(Goid goid_entering, double time, WORD condID)
{

	UNREFERENCED_PARAMETER(time);

	ConditionDb::iterator it = m_Conditions.find(condID);
	if (it == m_Conditions.end())
	{
		return false;
	}

	if ((*it).second->IsIllegalOccupantCrossing(time,goid_entering))
	{
		gpdevreportf( &gTriggerSysContext,(
			"[TRIGGERSYS] [%s g:%08x s:%08x] Avoided an illegal boundary ENTER. "
			"Condition [%s:0x%08x:0x%04x]\n",
			GetGo(goid_entering) ? GetGo(goid_entering)->GetTemplateName() : "<UNKNOWN>",
			goid_entering,
			GetGo(goid_entering) ? MakeInt(GetGo(goid_entering)->GetScid()) : -1,
			(*it).second->GetName(),
			(GetOwner()) ? MakeInt(GetOwner()->GetGoid()) : -1,
			(*it).second->GetID(),
			(GetOwner()) ? GetOwner()->GetTemplateName() : "<UNKNOWN>",
			(GetOwner()) ? MakeInt(GetOwner()->GetScid()) : -1
			));
		return false;
	}
	
	bool ret = (*it).second->AddOccupantIntoBoundary(goid_entering);

#if !GP_RETAIL
	if (gWorldOptions.GetShowTriggerSysHits() || gWorldOptions.GetShowTriggerSys())
	{
		SiegePos s0 = GetGo(goid_entering)->GetPlacement()->GetPosition();
		float display_duration = 3.0f;

		static float angle = 0;
		angle+=(PI/8.0f);
		gWorld.DrawDebugBoxStack(s0, 0.11f, 0xff00ff00 , display_duration, YRotationColumns(angle));
	}
#endif

	if (ret)
	{
		// Set the owner's bucket dirty so we can be sure that the 
		// clients all get the updated occupants when the trigger is refreshed
		GetOwner()->SetBucketDirty(true);
	}
	
	return ret;
}

bool Trigger::OnGoLeavesBoundary(Goid goid_exiting, double time, WORD condID)
{
	UNREFERENCED_PARAMETER(time);

	ConditionDb::iterator it = m_Conditions.find(condID);
	if (it == m_Conditions.end())
	{
		return false;
	}

	if ((*it).second->IsIllegalOccupantCrossing(time,goid_exiting))
	{
		gpdevreportf( &gTriggerSysContext,(
			"[TRIGGERSYS] [%s g:%08x s:%08x] Avoided an illegal boundary LEAVE. "
			"Condition [%s:0x%08x:0x%04x]\n",
			GetGo(goid_exiting) ? GetGo(goid_exiting)->GetTemplateName() : "<UNKNOWN>",
			goid_exiting,
			GetGo(goid_exiting) ? MakeInt(GetGo(goid_exiting)->GetScid()) : -1,
			(*it).second->GetName(),
			(GetOwner()) ? MakeInt(GetOwner()->GetGoid()) : -1,
			(*it).second->GetID(),
			(GetOwner()) ? GetOwner()->GetTemplateName() : "unknown",
			(GetOwner()) ? MakeInt(GetOwner()->GetScid()) : -1
			));
		return false;
	}

	GoOccupantMapIter git;
	bool ret = (*it).second->RemoveOccupantFromBoundary(goid_exiting,false,git);

#if !GP_RETAIL		
	if (gWorldOptions.GetShowTriggerSysHits() || gWorldOptions.GetShowTriggerSys())
	{
		SiegePos s0 = GetGo(goid_exiting)->GetPlacement()->GetPosition();
		float display_duration = 3.0f;

		static float angle = 0;
		angle+=(PI/8.0f);
		gWorld.DrawDebugBoxStack(s0, 0.11f, 0xffff0000 , display_duration, YRotationColumns(angle));
	}
#endif

	return ret;
}

bool Trigger::IsInsideBoundary(Goid goid, WORD condID) const
{
	ConditionDb::const_iterator it = m_Conditions.find(condID);
	if (it == m_Conditions.end())
	{
		return false;
	}
	return ((*it).second->IsBoundaryOccupant(goid));
}

bool Trigger::GetSubGroupBoundaryOccupants(int sg, const GoOccupantMap* &occs)
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{
		if (((*cit).second->GetSubGroup() == sg) && !(*cit).second->IsMessageHandler())
		{
			occs = (*cit).second->GetBoundaryOccupants();
			return !occs->empty();
		}
	}
	gpassertf(0,("GetSubGroupBoundaryOccupants(int sg...), cannot find subgroup SG!"));
	return false;
}

bool Trigger::HasSubGroupBoundaryOccupants(int sg) const
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{
		if (((*cit).second->GetSubGroup() == sg) && !(*cit).second->IsMessageHandler() )
		{
			return (*cit).second->HasBoundaryOccupants();
		}
	}
	gpassertf(0,("HasSubGroupBoundaryOccupants(int sg), cannot find subgroup SG!"));
	return false;
}

bool Trigger::GetSubGroupSatisfyingOccupants(int sg, const GoOccupantMap* &occs)
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{	
		if ( ((*cit).second->GetSubGroup() == sg) && !(*cit).second->IsMessageHandler() )
		{
			occs = (*cit).second->GetSatisfyingOccupants();
			if (!occs->empty())
			{
				return true;
			}
		}
	}
	return false;
}

bool Trigger::HasSubGroupSatisfyingOccupants(int sg) const
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{
		if (((*cit).second->GetSubGroup() == sg) && !(*cit).second->IsMessageHandler() )
		{
			if ((*cit).second->HasSatisfyingOccupants())
			{
				return true;
			}
		}
	}
	return false;
}

bool Trigger::GetSubGroupSatisfyingMessages(int sg, const MessageList* &mess)
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{
		if (((*cit).second->GetSubGroup() == sg) && (*cit).second->IsMessageHandler())
		{
			mess = (*cit).second->GetSatisfyingMessages();
			if (!mess->empty())
			{
				return true;
			}
		}
	}
	return false;
}

bool Trigger::HasSubGroupSatisfyingMessages(int sg) const
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{
		if (((*cit).second->GetSubGroup() == sg) && (*cit).second->IsMessageHandler())
		{
			if ((*cit).second->HasSatisfyingMessages())
			{
				return true;
			}
		}
	}
	return false;
}

bool Trigger::ClearSubGroupSatisfyingMessages(int sg)
{
	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
	{
		if (((*cit).second->GetSubGroup() == sg) && (*cit).second->IsMessageHandler())
		{
			(*cit).second->ClearSatisfyingMessages();
		}
	}
	return true;
}

void Trigger::ClearUndelayedSubGroups(void)
{
	SubGroupIter sgit = m_SubGroupStates.begin();
	for (;sgit != m_SubGroupStates.end(); ++sgit)
	{
		bool state_after_clear = true;

		ConditionConstIter cit, begin = m_Conditions.begin();
		for ( cit = begin ; cit != m_Conditions.end() ; ++cit )
		{
			if ((*cit).second->GetSubGroup() == (*sgit).first)
			{
				if ( (*cit).second->IsMessageHandler() )
				{
					// Message handlers are ALWAYS cleared
					(*cit).second->ClearSatisfyingMessages();
					state_after_clear = false;
				}
				else
				{
					// Is the sub group NOT true (in which case we need to clear
					// any partially satisfied occupants). If it is true,
					// is the action associated with the sub group NOT delayed?			
					if (!(*sgit).second.first || !(*sgit).second.second)
					{
						(*cit).second->ClearSatisfyingOccupants();
						state_after_clear &= (*cit).second->HasSatisfyingOccupants();
					}
				}
			}
		}		
		(*sgit).second.first = state_after_clear;
	}
}

void Trigger::GetNonMessageConditions(gpstring& txt) const
{
	txt.clear();

	ConditionConstIter cit, begin = m_Conditions.begin();
	for ( cit = begin ; (cit != m_Conditions.end()) ; ++cit )
	{
		if (!(*cit).second->IsMessageHandler())
		{
			if (!txt.empty())
			{
				txt.append(",");
			}
			txt.append((*cit).second->GetName());
		}
	}
}

void Trigger::XferForSync( FuBi::BitPacker& packer )
{
	int num_entries = 0;

	ConditionIter cit = m_Conditions.begin();
	ConditionIter cend = m_Conditions.end();

	while (cit != cend)
	{
		if ( !(*cit).second->IsMessageHandler() )
		{
			if ( packer.IsSaving() )
			{
				++num_entries;
			}
			else
			{
				(*cit).second->GetBoundaryOccupants()->clear();
				(*cit).second->GetSatisfyingOccupants()->clear();
			}
		}
		++cit;
	}

	packer.XferCount( num_entries );

	if (!num_entries)
	{
		// No occupants to deal with
		return;
	}

	cit = m_Conditions.begin();
	while (cit != cend)
	{
		if ( !(*cit).second->IsMessageHandler() )
		{
			--num_entries;
			GoOccupantMap* boccs = (*cit).second->GetBoundaryOccupants();
			DWORD num_boccs = boccs->size();
			packer.XferCount( num_boccs );

			GoOccupantMap* soccs = (*cit).second->GetSatisfyingOccupants();
			DWORD num_soccs = soccs->size();
			packer.XferCount( num_soccs );

			if (packer.IsSaving())
			{
				GoOccupantMapIter git = boccs->begin();
				while ( git != boccs->end() )
				{
					DWORD g = (DWORD)(*git).first;
					DWORD c = (DWORD)(*git).second;
					packer.XferRaw( g );
					packer.XferCount( c );
					++git;
				}
				git = soccs->begin();
				while ( git != soccs->end() )
				{
					DWORD g = (DWORD)(*git).first;
					DWORD c = (DWORD)(*git).second;
					packer.XferRaw( g );
					packer.XferCount( c );
					++git;
				}
			}
			else
			{
				while (num_boccs>0)
				{
					DWORD g,c;
					packer.XferRaw( g );
					packer.XferCount( c );
					(*boccs)[(Goid)g] = c;
					num_boccs--;
				}
				while (num_soccs>0)
				{
					DWORD g,c;
					packer.XferRaw( g );
					packer.XferCount( c );
					(*soccs)[(Goid)g] = c;
					num_soccs--;
				}
			}
		}
		++cit;
	}

	gpassert(!num_entries);

	return;
}

//////////////////////////////////////////////////////////////////////////////
//	TriggerSys

TriggerSys::TriggerSys()
{
	m_CurrentSeqNum = 0;

	// Create the list of condition names (for SE) 
	m_ConditionNames.push_back( "receive_world_message"					);
	m_ConditionNames.push_back( "actor_within_sphere"					);
	m_ConditionNames.push_back( "actor_within_bounding_box"				);
	m_ConditionNames.push_back( "go_within_sphere"						);
	m_ConditionNames.push_back( "go_within_bounding_box"				);
	m_ConditionNames.push_back( "has_go_in_inventory"					);
	m_ConditionNames.push_back( "party_member_within_bounding_box"		);
	m_ConditionNames.push_back( "party_member_within_sphere"			);
	m_ConditionNames.push_back( "party_member_within_node"				);
	m_ConditionNames.push_back( "party_member_entered_trigger_group"	);
	m_ConditionNames.push_back( "party_member_left_trigger_group"		);
	m_ConditionNames.push_back( "party_member_within_trigger_group"		);
	
	
	// Action instantiation																		client execution?
	New_Action_Def	( "send_world_message",					new send_world_message,				true  );
	New_Action_Def	( "call_sfx_script",					new call_sfx_script,				true  );
	New_Action_Def	( "start_camera_fx",					new start_camera_fx,				true  );
	New_Action_Def	( "play_sound",							new play_sound,						true  );
	New_Action_Def	( "mood_change",						new mood_change,					true  );
	New_Action_Def	( "fade_nodes_outer_local_party",		new fade_nodes_outer_local_party,	true  );
	New_Action_Def	( "fade_nodes",							new fade_nodes,						true  );
	New_Action_Def	( "set_interest_radius",				new set_interest_radius,			true  );
	New_Action_Def	( "set_camera_fade_node",				new set_camera_fade_node,			false );
	New_Action_Def	( "set_occludes_camera_node",			new set_occludes_camera_node,		false );
	New_Action_Def	( "set_bounds_camera_node",				new set_bounds_camera_node,			false );
	New_Action_Def	( "fade_node",							new fade_node,						false );
	New_Action_Def	( "fade_nodes_global",					new fade_nodes_global,				false );
	New_Action_Def	( "change_actor_life",					new change_actor_life,				false );
	New_Action_Def	( "change_actor_mana",					new change_actor_mana,				false );
	New_Action_Def	( "change_quest_state",					new change_quest_state,				false );
	New_Action_Def	( "victory_condition_met",				new victory_condition_met,			false );
	New_Action_Def	( "set_player_world_location",			new set_player_world_location,		false );
}

TriggerSys::~TriggerSys()
{
	// Delete all of the actions
	stdx::for_each( m_ActionDefs, stdx::delete_second_by_ptr() );
	m_ActionDefs.clear();

	// Delete all of the group links
	stdx::for_each( m_Groups, stdx::delete_second_by_ptr() );
	m_Groups.clear();

}

void TriggerSys::Update()
{
	Critical::Lock locker( m_TrigSysCritical );

	tActionInfo_multimap::iterator i = m_ActionInfos.begin();
	tActionInfo_multimap::iterator iend = m_ActionInfos.upper_bound( gWorldTime.GetTime() );
	for( ; i != iend; ++i )
	{
		ExecuteActionInfo( (*i).second );
	}
	m_ActionInfos.erase( m_ActionInfos.begin(), iend );

}

void TriggerSys::RetireScid( Scid scid )
{
	Critical::Lock locker( m_TrigSysCritical );

	Scuid sc(scid,0);

	TrigBitsDb::iterator found = m_TrigBitsDb.upper_bound( sc );

	while (found != m_TrigBitsDb.end() && ((*found).first.GetOwnScid() == scid))
	{
		found = m_TrigBitsDb.erase( found );
	}
}

bool TriggerSys::FetchTrigBits(const Scuid& sc, TrigBits& tb)
{
	Critical::Lock locker( m_TrigSysCritical );

	TrigBitsDb::iterator found = m_TrigBitsDb.find( sc );
	if ( found != m_TrigBitsDb.end() )
	{
		tb = (*found).second;
		m_TrigBitsDb.erase( found );
		return true;
	}
	return false;
}

bool TriggerSys::StoreTrigBits(const Scuid& sc, const TrigBits& tb)
{
	Critical::Lock locker( m_TrigSysCritical );
	
	m_TrigBitsDb[sc] = tb;
	return true;
}

void TriggerSys::AddActionInfo( double time, ActionInfo * actionInfo )
{
	if( time <= gWorldTime.GetTime() )
	{
		ExecuteActionInfo( actionInfo );
	}
	else
	{
		Critical::Lock locker( m_TrigSysCritical );
		m_ActionInfos.insert( std::make_pair( time, actionInfo ) );
	}
}

void TriggerSys::ExecuteActionInfo( ActionInfo * actionInfo )
{
	switch( actionInfo->m_CallType )
	{
		case ActionInfo::CT_NODE_FADE:
		{
			GoidColl::const_iterator i, ibegin = actionInfo->m_Members.begin(), iend = actionInfo->m_Members.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( GetGo(*i) )
				{
					FrustumId fId = gGoDb.GetGoFrustum( (*i) );
					if( fId != FRUSTUMID_INVALID )
					{
						siege::SiegeFrustum* pFrustum = gSiegeEngine.GetFrustum( MakeInt(fId) );
						gpassert( pFrustum );
						if (pFrustum)
						{
							pFrustum->NodeFade( actionInfo->m_Region,
												actionInfo->m_Section,
												actionInfo->m_Level,
												actionInfo->m_Object,
												actionInfo->m_FT);
						}
						else
						{
							gpdevreportf( &gTriggerSysContext,(
								"TRIGGER_SYS: Attempted to execute a FADE, but the frustum doesn't seem to exist for \n"
								"\t\t[%s:g:%08x:s:%08x]\n"
									, GetGo(*i) ? GetGo(*i)->GetTemplateName() : "<UNRESOLVED>"
									, *i
									, GetGo(*i) ? MakeInt(GetGo(*i)->GetScid()) : -1
								));
						}
					}
					else
					{
						gpdevreportf( &gTriggerSysContext,(
							"TRIGGER_SYS: Attempted to execute a FADE, but the frustum is INVALID \n"
							"\t\t[%s:g:%08x:s:%08x]\n"
								, GetGo(*i) ? GetGo(*i)->GetTemplateName() : "<UNRESOLVED>"
								, *i 
								, GetGo(*i) ? MakeInt(GetGo(*i)->GetScid()) : -1
							));
					}
				}
			}
			break;
		}

		case ActionInfo::CT_SNODE_FADE:
		{
			// Node GUID, Fade type
			gWorldTerrain.SNodeFade( siege::database_guid( actionInfo->m_Region ), actionInfo->m_FT );
			break;
		}

		case ActionInfo::CT_SGLOBAL_NODE_FADE:
		{
			gWorldTerrain.SGlobalNodeFade(	actionInfo->m_Region,
											actionInfo->m_Section,
											actionInfo->m_Level,
											actionInfo->m_Object,
											actionInfo->m_FT);
			break;
		}

		case ActionInfo::CT_MOOD_CHANGE:
		{

			GoidColl::const_iterator i, ibegin = actionInfo->m_Members.begin(), iend = actionInfo->m_Members.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( GetGo(*i) )
				{
					gMood.RegisterMoodRequest( *i , actionInfo->m_MoodName );
				}
			}
			break;
		}

		default:
		{
			gpassertm( 0, "Invalid range" );
		}
	}

	delete actionInfo;
}

Parameter *
TriggerSys::GetActionFormat( const gpstring& sActionName )
{
	tActionDef_map_i iAction = m_ActionDefs.find( sActionName.c_str() );
	if( iAction != m_ActionDefs.end() )
	{
		return (*iAction).second->GetFormat();
	}

	return 0;
}

bool
TriggerSys::ActionAllowsClientExecution( const gpstring& sActionName )
{

	tActionDef_map_i iAction = m_ActionDefs.find( sActionName.c_str() );
	if( iAction != m_ActionDefs.end() )
	{
		return (*iAction).second->GetAllowClientExecution();
	}

	return false;
}

void TriggerSys::AddSpatialTrigger( Trigger* pTrigger )
{
	Critical::Lock locker( m_TrigSysCritical );

	gpassert(m_SpatialTriggers.find(pTrigger->GetTruid()) == m_SpatialTriggers.end());
	m_SpatialTriggers.insert( std::make_pair(pTrigger->GetTruid(),pTrigger) );
}

void TriggerSys::RemoveSpatialTrigger( Trigger* pTrigger )
{
	Critical::Lock locker( m_TrigSysCritical );

	gpassert(m_SpatialTriggers.find(pTrigger->GetTruid()) != m_SpatialTriggers.end());
	m_SpatialTriggers.erase( pTrigger->GetTruid() );
}

bool TriggerSys::IsSpatialTrigger( Trigger* pTrigger )
{
	Critical::Lock locker( m_TrigSysCritical );

	return pTrigger && 
		pTrigger->GetTruid() != Truid() && 
		(m_SpatialTriggers.find(pTrigger->GetTruid()) != m_SpatialTriggers.end());
}

void TriggerSys::AddValidTrigger( Trigger* pTrigger )
{
	Critical::Lock locker( m_TrigSysCritical );

	gpassert(m_ValidTriggers.find(pTrigger->GetTruid()) == m_ValidTriggers.end());
	m_ValidTriggers.insert( std::make_pair(pTrigger->GetTruid(),pTrigger) );
}

void TriggerSys::RemoveValidTrigger( Trigger* pTrigger )
{
	Critical::Lock locker( m_TrigSysCritical );

	gpassert(m_ValidTriggers.find(pTrigger->GetTruid()) != m_ValidTriggers.end());
	m_ValidTriggers.erase( pTrigger->GetTruid() );
}

bool TriggerSys::IsValidTrigger( Trigger* pTrigger )
{
	Critical::Lock locker( m_TrigSysCritical );

	return pTrigger && 
		pTrigger->GetTruid() != Truid() && 
		(m_ValidTriggers.find(pTrigger->GetTruid()) != m_ValidTriggers.end());
}

void TriggerSys::UpdateTriggersWhenGoEntersWorld( const Go* in_Newbie, DWORD in_NewFrustum )
{
	Critical::Lock locker( m_TrigSysCritical );
	
	GPPROFILERSAMPLE( "0Triggers :: UpdateTriggersWhenGoEntersWorld", SP_TRIGGER_MISC );
	if (!gServer.IsLocal())
	{
		return;
	}
	
	TriggerDb::iterator spit  = m_SpatialTriggers.begin();

	SignalSet hitlist;
	double time = gWorldTime.GetTime();
	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		if ( (*spit).second->GetCurrentActive() )
		{
			(*spit).second->NewGoEntering( time, in_Newbie, in_NewFrustum, hitlist );
		}
	}
	if (!hitlist.empty())
	{
		gMCP.AppendTriggerSignals(in_Newbie->GetGoid(),hitlist);
	}
}

void TriggerSys::UpdateTriggersWhenGoLeavesWorld( const Go* in_Stale )
{
	Critical::Lock locker( m_TrigSysCritical );
	
	GPPROFILERSAMPLE( "0Triggers :: UpdateTriggersWhenGoLeavesWorld", SP_TRIGGER_MISC );
	if (!gServer.IsLocal())
	{
		return;
	}
	
	TriggerDb::iterator spit  = m_SpatialTriggers.begin();

	SignalSet hitlist;
	double time = gWorldTime.GetTime();
	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		if ( (*spit).second->GetCurrentActive() )
		{
			(*spit).second->StaleGoLeaving( time, in_Stale, hitlist );
		}
	}
	if(!hitlist.empty())
	{
		gMCP.AppendTriggerSignals(in_Stale->GetGoid(),hitlist);
	}
}

void TriggerSys::UpdateTriggersWhenGoIsDestroyed( const Go* in_Dead )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Triggers :: UpdateTriggersWhenGoLeavesWorld", SP_TRIGGER_MISC );
	if (!gServer.IsLocal())
	{
		return;
	}
	
	Goid deadgoid = in_Dead->GetGoid();

	TriggerDb::iterator spit  = m_SpatialTriggers.begin();
	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		(*spit).second->DestroyedGoRemoval( deadgoid );
	}
}

void TriggerSys::UpdateTriggersAfterTerrainMoves( const Go* in_Jumper )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Triggers :: UpdateTriggersAfterTerrainMoves", SP_TRIGGER_MISC );
	if (!gServer.IsLocal())
	{
		return;
	}

	SignalSet hitlist;

	SiegePos newpos;
	bool ok=false;
	double jumptime = gWorldTime.GetTime();
	if (in_Jumper->HasFollower())
	{	
		MCP::Plan* jplan = gMCP.FindPlan(in_Jumper->GetGoid(),false);
		if (jplan)
		{
			newpos = jplan->GetPosition(jumptime,true,true);
			ok = true;
		}
	}
	if (!ok)
	{
		newpos = in_Jumper->GetPlacement()->GetPosition();
	}

	TriggerDb::iterator spit  = m_SpatialTriggers.begin();
	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		if ( (*spit).second->GetCurrentActive() )
		{
			(*spit).second->TeleportingGo( jumptime , in_Jumper, newpos, hitlist );
		}
	}
	if (!hitlist.empty())
	{
		gMCP.AppendTriggerSignals(in_Jumper->GetGoid(),hitlist);
	}
}

void TriggerSys::UpdateTriggersWhenGoJoinsParty ( const Go* in_NewBuddy )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Triggers :: UpdateTriggersWhenGoJoinsParty", SP_TRIGGER_MISC );

	if (!gServer.IsLocal())
	{
		return; // !!!
	}

	SignalSet hitlist;
	double time = gWorldTime.GetTime();
	TriggerDb::iterator spit  = m_SpatialTriggers.begin();
	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		if ( (*spit).second->GetCurrentActive() )
		{
			(*spit).second->GoJoinedWithParty( time, in_NewBuddy, hitlist );
		}
	}
	if(!hitlist.empty())
	{
		gMCP.AppendTriggerSignals(in_NewBuddy->GetGoid(),hitlist);
	}
}

void TriggerSys::UpdateTriggersWhenGoLeavesParty( const Go* in_Estranged )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Triggers :: UpdateTriggersWhenGoLeavesParty", SP_TRIGGER_MISC );

	if (!gServer.IsLocal())
	{
		return; // !!!
	}

	SignalSet hitlist;
	double time = gWorldTime.GetTime();
	TriggerDb::iterator spit  = m_SpatialTriggers.begin();
	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		if ( (*spit).second->GetCurrentActive() )
		{
			(*spit).second->GoRemovedFromParty( time, in_Estranged, hitlist );
		}
	}
	if (!hitlist.empty())
	{
		gMCP.AppendTriggerSignals(in_Estranged->GetGoid(),hitlist);
	}
}

bool TriggerSys::CollectTriggerCollisions(const Go*			in_Mover,
										  const SiegePos&	in_PosA,
										  const double&		in_TimeA,
										  const SiegePos&	in_PosB,
										  const double&		in_TimeB,
										  SignalSet&		out_HitList )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: CollectTriggerCollisions", SP_TRIGGER_MISC );
	if (!gServer.IsLocal())
	{
		return false; // !!!
	}
	
	TriggerDb::iterator spit  = m_SpatialTriggers.begin();

	// Need to test EVERY spatial trigger... 
	// Prime place for a spatial search tree!!!! -- biddle

	out_HitList.clear();

	bool ispartymember = in_Mover->IsAnyHumanPartyMember();

	for(; spit != m_SpatialTriggers.end(); ++spit )
	{
		if ( ispartymember || !(*spit).second->GetIsPartySpecific() )
		{
			// Check to make sure its active and not either marked for deletion or a one-shot 
			// that has been already triggered for delayed execution
			Trigger* trig = (*spit).second;
			if ( trig->m_bCurrentActiveState && !trig->m_bMarkedForDeletion && !trig->m_bOneShotHasTriggered)
			{
				trig->CollectTriggerCollisions( in_Mover, in_PosA, in_TimeA, in_PosB, in_TimeB, out_HitList );
			}
		}
	}

	return (!out_HitList.empty());
}
	
bool TriggerSys::SignalTrigger(const BoundaryCrossing& bc)
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: SignalTrigger", SP_TRIGGER_MISC );

	TriggerDb::iterator it  = m_ValidTriggers.find(bc.m_TriggerID);

	if (it != m_ValidTriggers.end())
	{
		m_Signals.insert( bc );
	}
	else
	{
		// Trigger can get delete but they have pending signals coming back from the 
		//gpassertm(it != m_ValidTriggers.end(),"Signalling an unknown trigger");
	}
	
	return true;
};

bool TriggerSys::ProcessSignals( double current_time )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: ProcessSignals", SP_TRIGGER_MISC );

	SignalIter sigit = m_Signals.begin();

	for ( ; (sigit != m_Signals.end()) && (*sigit).m_Time<=current_time ; ++sigit )
	{
		TriggerDb::iterator spit = m_ValidTriggers.find((*sigit).m_TriggerID);
		if (spit != m_ValidTriggers.end())
		{
			// Check to make sure its active and not either marked for deletion or a one-shot 
			// that has been already triggered for delayed execution
			Trigger* trig = (*spit).second;

			if (!trig->m_bCurrentActiveState)
			{
				if (!gServer.IsLocal())
				{
					// Inactive triggers on clients are activated whenever a signal arrives
					trig->m_bRequestedActiveState = true;
					trig->m_bCurrentActiveState = true;
				}
				else
				{
					gpdevreportf( &gTriggerSysContext,("TRIGGER_SYS: Server has signalled an inactive trigger [%s:0x%08x:%04x:0x%08x]\n"
						,trig->GetOwner() ? trig->GetOwner()->GetTemplateName() : "owner-STILL-not-known"
						,trig->GetOwner() ? MakeInt(trig->GetOwner()->GetGoid()) : 0
						,trig->GetOwner() ? trig->GetTruid().GetTrigNum() : 0
						,trig->GetOwner() ? MakeInt(trig->GetOwner()->GetScid()) : 0
						));
				}
			}

			if ( trig->m_bCurrentActiveState && !trig->m_bMarkedForDeletion && !trig->m_bOneShotHasTriggered)
			{

				bool ok;
				if (IsEnteringAtCrossing((*sigit).m_CrossType))
				{
					ok = trig->OnGoEntersBoundary(MakeGoid((*sigit).m_Data),(*sigit).m_Time,(*sigit).m_ConditionID);
				}
				else
				{
					ok = trig->OnGoLeavesBoundary(MakeGoid((*sigit).m_Data),(*sigit).m_Time,(*sigit).m_ConditionID);
				}
				if (ok)
				{
					trig->Update(current_time);
				}
				else
				{
					trig->NotifyWatchersOfCrossingError(MakeGoid((*sigit).m_Data),(*sigit).m_Time);
				}
			}

			if (gServer.IsLocal())
			{
				trig->DecrementExpectedSignals(*sigit,current_time);
			}

		}
		else
		{
			gpwarning("SIGNAL ERROR:: Something has removed a spatial trigger!");
		}
	}

	m_Signals.erase(m_Signals.begin(),sigit);

	TriggerDb::iterator vit = m_ValidTriggers.begin();
	while (vit != m_ValidTriggers.end())
	{
		if (!(*vit).second->GetIsPureMessageHandler())
		{
			// If its not a pure message handler, then test for stale signals
			(*vit).second->RemoveStaleExpectedSignals();
		}
		++vit;
	}

	// Reset the sequence number after process the signals
	m_CurrentSeqNum = 0;

	return true;
}

bool TriggerSys::CancelSignal(const BoundaryCrossing& bc)
{
	TriggerDb::iterator it = m_ValidTriggers.find(bc.m_TriggerID);
	if (it != m_ValidTriggers.end())
	{
		(*it).second->DecrementExpectedSignals(bc,bc.m_Time);
		return true;
	}
	return false;
}


Condition*
TriggerSys::NewCondition( const gpstring& sConditionName )
{
	// Create a new condition of the correct type

	Condition* pCondition = 0;

	     if (sConditionName.same_no_case("party_member_within_bounding_box"))	pCondition = new party_member_within_bounding_box;
	else if (sConditionName.same_no_case("party_member_within_sphere"))			pCondition = new party_member_within_sphere;
	else if (sConditionName.same_no_case("party_member_within_node"))			pCondition = new party_member_within_node;

	else if (sConditionName.same_no_case("party_member_entered_trigger_group"))	pCondition = new party_member_entered_trigger_group;
	else if (sConditionName.same_no_case("party_member_left_trigger_group"))	pCondition = new party_member_left_trigger_group;
	else if (sConditionName.same_no_case("party_member_within_trigger_group"))	pCondition = new party_member_within_trigger_group;

	else if (sConditionName.same_no_case("receive_world_message"))				pCondition = new receive_world_message;
	else if (sConditionName.same_no_case("actor_within_sphere"))				pCondition = new actor_within_sphere;
	else if (sConditionName.same_no_case("actor_within_bounding_box"))			pCondition = new actor_within_bounding_box;
	else if (sConditionName.same_no_case("go_within_sphere"))					pCondition = new go_within_sphere;
	else if (sConditionName.same_no_case("go_within_bounding_box"))				pCondition = new go_within_bounding_box;
	else if (sConditionName.same_no_case("has_go_in_inventory"))				pCondition = new has_go_in_inventory;

	return pCondition;
}

Action*
TriggerSys::NewAction( const gpstring& sActionName )
{

	tActionDef_map_i iAction = m_ActionDefs.find(sActionName.c_str());

	if (iAction != m_ActionDefs.end())
	{
		return (*iAction).second;
	}
	else
	{
		return NULL;
	}
}

// Private section

void
TriggerSys::New_Action_Def( const gpstring &sActionName, Action *pAction, bool bAllowClientExecution )
{
	Critical::Lock locker( m_TrigSysCritical );

	TriggerSys::tActionDef_pair action_entry( sActionName, pAction );
	pAction->SetAllowClientExecution( bAllowClientExecution );
	m_ActionDefs.insert( action_entry );
	m_ActionNames.push_back( sActionName );
}

void
TriggerSys::AddGroupWatcher( const gpstring& sGroupName, const Cuid& watching_condition)
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: AddGroupWatcher", SP_TRIGGER_MISC );

	GroupLink* gl;
	GroupIter it;

	if (!Services::IsEditor())
	{
		gpstring nm = sGroupName;

		it = m_Groups.find(nm);

		if (it == m_Groups.end())
		{
			gl = new GroupLink();
			m_Groups[nm] = gl;
		}
		else
		{
			gl = (*it).second;
		}

		if (gl->IsWatcher(watching_condition))
		{
			gl->RemoveWatcher(watching_condition);
		}
		
		gl->AddWatcher(watching_condition);
	}
	else // This is the editor
	{
		// Should I tack on an MP?
		Trigger* trig;
		if (FetchTriggerByTruid(watching_condition.GetOwnTrig(), trig))
		{
			if (trig->GetIsMulti())
			{
				gpstring mpname(sGroupName);
				mpname.append(":MP");
				
				it = m_Groups.find(mpname);

				if (it == m_Groups.end())
				{
					gl = new GroupLink();
					m_Groups[mpname] = gl;
				}
				else
				{
					gl = (*it).second;
				}

				if (gl->IsWatcher(watching_condition))
				{
					gl->RemoveWatcher(watching_condition);
				}
				
				gl->AddWatcher(watching_condition);
			}
			if (trig->GetIsSingle())
			{
				gpstring spname(sGroupName);
				
				it = m_Groups.find(spname);

				if (it == m_Groups.end())
				{
					gl = new GroupLink();
					m_Groups[spname] = gl;
				}
				else
				{
					gl = (*it).second;
				}

				if (gl->IsWatcher(watching_condition))
				{
					gl->RemoveWatcher(watching_condition);
				}
				
				gl->AddWatcher(watching_condition);
			}
		}
	}


}


void
TriggerSys::AddGroupMember( const gpstring &sGroupName, const Truid&  member_trigger)
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: AddGroupMember", SP_TRIGGER_MISC );
	
	GroupLink* gl;
	GroupIter it;

	it = m_Groups.find(sGroupName);

	if (it == m_Groups.end())
	{
		gl = new GroupLink();
		m_Groups[sGroupName] = gl;
	}
	else
	{
		gl = (*it).second;
	}

	if (gl->IsMember(member_trigger))
	{
		gl->RemoveMember(member_trigger);
	}
					
	gl->AddMember(member_trigger);

	if (Services::IsEditor())
	{
		gpstring mpname(sGroupName);
		mpname.append(":MP");

		it = m_Groups.find(mpname);
		
		if (it == m_Groups.end())
		{
			gl = new GroupLink();
			m_Groups[mpname] = gl;
		}
		else
		{
			gl = (*it).second;
		}

		if (gl->IsMember(member_trigger))
		{
			gl->RemoveMember(member_trigger);
		}
		
		gl->AddMember(member_trigger);
	}
}

void
TriggerSys::RemoveGroupWatcher( const gpstring& sGroupName, const Cuid& watching_condition  )
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: RemoveGroupWatcher", SP_TRIGGER_MISC );

	GroupIter it;
	
	if (!Services::IsEditor())
	{
		gpstring nm = sGroupName;
		it = m_Groups.find(nm);
		if (it != m_Groups.end())
		{
			if ((*it).second->IsWatcher(watching_condition))
			{
				(*it).second->RemoveWatcher(watching_condition);
			}

			if ((*it).second->GetMembers()->empty() && (*it).second->GetWatchers()->empty())
			{
				delete (*it).second;
				m_Groups.erase(it);
			}
		}
	}
	else // (Services::IsEditor())
	{
		// Make sure we track both SP & MP groups
		Trigger* trig;
		if (FetchTriggerByTruid(watching_condition.GetOwnTrig(), trig))
		{
			if (trig->GetIsMulti())
			{
				gpstring mpname(sGroupName);
				mpname.append(":MP");
				it = m_Groups.find(mpname);
				
				if (it != m_Groups.end())
				{
					if ((*it).second->IsWatcher(watching_condition))
					{
						(*it).second->RemoveWatcher(watching_condition);
					}

					if ((*it).second->GetMembers()->empty() && (*it).second->GetWatchers()->empty())
					{
						delete (*it).second;
						m_Groups.erase(it);
					}
				}
			}
			if (trig->GetIsSingle())
			{
				gpstring spname(sGroupName);
				it = m_Groups.find(spname);
				
				if (it != m_Groups.end())
				{
					if ((*it).second->IsWatcher(watching_condition))
					{
						(*it).second->RemoveWatcher(watching_condition);
					}

					if ((*it).second->GetMembers()->empty() && (*it).second->GetWatchers()->empty())
					{
						delete (*it).second;
						m_Groups.erase(it);
					}
				}
			}
		}
	}
}


void
TriggerSys::RemoveGroupMember( const gpstring &sGroupName, const Truid&  member_trigger )
{
	Critical::Lock locker( m_TrigSysCritical );
	
	GPPROFILERSAMPLE( "0Trigger :: RemoveGroupMember", SP_TRIGGER_MISC );

	GroupIter it = m_Groups.find(sGroupName);
	
	if (it != m_Groups.end())
	{
		if ((*it).second->IsMember(member_trigger))
		{
			(*it).second->RemoveMember(member_trigger);
		}

		if ((*it).second->GetMembers()->empty() && (*it).second->GetWatchers()->empty())
		{
			delete (*it).second;
			m_Groups.erase(it);
		}
	}

	if (Services::IsEditor())
	{
		gpstring nm = sGroupName;
		nm.append(":MP");

		it = m_Groups.find(nm);
		
		if (it != m_Groups.end())
		{
			if ((*it).second->IsMember(member_trigger))
			{
				(*it).second->RemoveMember(member_trigger);
			}

			if ((*it).second->GetMembers()->empty() && (*it).second->GetWatchers()->empty())
			{
				delete (*it).second;
				m_Groups.erase(it);
			}
		}
	}

}

bool
TriggerSys::GetGroupMembers(const gpstring &sGroupName, GroupLink::MemberSet* &members)
{
	Critical::Lock locker( m_TrigSysCritical );
	
	GroupIter it = m_Groups.find(sGroupName);
	
	if (it != m_Groups.end())
	{
		members = (*it).second->GetMembers();
		return true;
	}

	return false;
}

	// Trigger and condition lookup
///////////////////////////////////////////////////////////////////////////////////
bool TriggerSys::FetchTriggerByTruid(const Truid& truid, Trigger*& out_trig)
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: FetchTriggerByTruid", SP_TRIGGER_MISC );
	bool found = true;
	TriggerIter t;
	t = m_ValidTriggers.find(truid) ;
	if (t == m_ValidTriggers.end())
	{
		found = false;
	}
	if (found)
	{
		out_trig = (*t).second;
	}
	else
	{
		out_trig = NULL;
	}
	gpassert(found);
	return found;
}

///////////////////////////////////////////////////////////////////////////////////
bool TriggerSys::FetchConditionByCuid(const Cuid& cuid,  Condition*& out_cond)
{
	Critical::Lock locker( m_TrigSysCritical );

	GPPROFILERSAMPLE( "0Trigger :: FetchConditionByCuid", SP_TRIGGER_MISC );
	
	bool found = false;
	Trigger* trig;
	if (FetchTriggerByTruid(cuid.GetOwnTrig(), trig))
	{
		ConditionIter c = trig->m_Conditions.find(cuid.GetCondNum());
		gpassert(c != trig->m_Conditions.end());
		if (c != trig->m_Conditions.end())
		{
			out_cond = (*c).second;
			found = true;
		}
	}
	else
	{
		out_cond = NULL;
	}
	gpassert(found);
	return found;
}

///////////////////////////////////////////////////////////////////////////////////
void TriggerSys::SDeactivateTrigger( Goid owngoid, WORD trignum )
{
	CHECK_SERVER_ONLY;

	Critical::Lock locker( m_TrigSysCritical );
	
	GoHandle go( owngoid );
	if_gpassert( go )
	{
		go->RCDeactivateTrigger( trignum );
	}
}


void TriggerSys::DeactivateTrigger( Scid ownscid, Goid owngoid, WORD trignum )
{
	Critical::Lock locker( m_TrigSysCritical );
	
	Truid tr(owngoid,trignum);

	TriggerIter it = m_ValidTriggers.find(tr);
	if ( it != m_ValidTriggers.end() )
	{
		(*it).second->OnDeactivation();
	}
	else
	{
		// Wipe out any trigbits we are storing for this trigger
		Scuid sc(ownscid,trignum);
		TrigBits tb; 
		gTriggerSys.FetchTrigBits(sc,tb);
		tb = 0;
		gTriggerSys.StoreTrigBits(sc,tb);
	}
	
}

bool TriggerSys::SSendActionToPartyMembersRemainingInNode(SiegeId node_siegeid, const gpstring& actionreq )
{
	if (actionreq.empty())
	{
		// Early out: No string to parse
		return false;
	}

	siege::database_guid node_id;

	node_id.SetValue((DWORD)node_siegeid);

	GoidColl remInNode;
	DWORD remInNodeMask = 0;

	// Determine the Frustums of the Go affected by this action
	
	const Server::PlayerColl& playerColl = gServer.GetPlayers();
	Server::PlayerColl::const_iterator i, ibegin = playerColl.begin(), iend = playerColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Player* player = *i;

		if ( IsValid(player) && !player->IsComputerPlayer() )
		{
			const GopColl& parties = player->GetPartyColl();
			GopColl::const_iterator j, jbegin = parties.begin(), jend = parties.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				Go* party = *j;

				const GopColl& members = party->GetChildren();
				GopColl::const_iterator k, kbegin = members.begin(), kend = members.end();
				for ( k = kbegin ; k != kend ; ++k )
				{
					Go* member = *k;
					MCP::Plan* p = gMCP.FindPlan(member->GetGoid(),false);
					if (p)
					{
						if (p->GetFinalDestination().node == node_id)
						{
							remInNode.push_back(member->GetGoid());
							FrustumId fId	= gGoDb.GetGoFrustum( member->GetGoid() );
							if( fId != FRUSTUMID_INVALID )
							{
								remInNodeMask |= MakeInt(fId);
							}
						}
					}
				}
			}
		}
	}

	if ( remInNode.empty() )
	{
		// Early out: no one is in the node
		return false;
	}

	stdx::fast_vector<ActionInfo*> actioninfos;
	bool parsed_ok = true;

	stringtool::Extractor actionreqex( ";", actionreq.c_str(), true );
			
	gpstring actionparms;

	while (parsed_ok && actionreqex.GetNextString(actionparms) )
	{
		stringtool::Extractor fadeparmex( ",", actionparms.c_str() );


		// look for a mood name
		gpstring moodname;
		
		if (!fadeparmex.GetNextString(moodname))
		{
			parsed_ok = false;
			gperrorf(("CONTENT ERROR: (Elevators) Failed to parse an ActionInfo string [%s] "
				    "This problem affects region [%s] node [0x%08x]",
					actionreq.c_str(),
					gWorldMap.GetRegionNameForNode(node_id).c_str(),
					node_id
					));
			continue;
		}

		ActionInfo::eCallType calltype = ActionInfo::CT_INVALID;
		FADETYPE ft	= FT_NONE;

		if ((moodname[0] == '0') && (moodname[1] == 'x' || moodname[1] == 'X'))
		{
			// This is a fade, not a mood change!
			calltype =  ActionInfo::CT_NODE_FADE;

			stdx::fast_vector<int> parms(4);

			parms[0] = stringtool::strtol(moodname.c_str());

			if ( !gGoDb.IsEditMode() && !gWorldMap.ContainsRegion(MakeRegionId(parms[0])) )
			{
				gperrorf((
					"BAD CONTENT: Please tell Dave T that the ELEVATOR that moves node [0x%08x] has a FADE_NODES action with an invalid REGION ID [0x%08x]\n",
					(DWORD)node_siegeid,
					parms[0]
					));
				parsed_ok = false;
			}

			int i = 1;
			while ( (i < 4) && fadeparmex.GetNextInt(parms[i]) )
			{
				++i;
			}
			if (i == 4)
			{
				gpstring ft_str; 
				if (fadeparmex.GetNextString(ft_str))
				{	

					if( ft_str.same_no_case( "out:alpha" ) )
					{
						ft	= FT_ALPHA;
					}
					else if( ft_str.same_no_case( "out:black" ) )
					{
						ft	= FT_BLACK;
					}
					else if( ft_str.same_no_case( "out:instant" ) )
					{
						ft	= FT_INSTANT;
					}
					else if( ft_str.same_no_case( "in" ) )
					{
						ft	= FT_IN;
					}
					else if( ft_str.same_no_case( "in:instant" ) )
					{
						ft	= FT_IN_INSTANT;
					}
					else
					{
						parsed_ok = false;
					}

				}
			}
			if (parsed_ok)
			{
				float dly;
				if (!fadeparmex.GetNextFloat(dly))
				{	
					dly = 0.0f;
				}
				actioninfos.push_back(new ActionInfo(PreciseAdd(gWorldTime.GetTime(),dly),
													calltype,
													parms[0],
													parms[1],
													parms[2],
													parms[3],
													ft,
													remInNode));
			}
		}
		else
		{
			calltype = ActionInfo::CT_MOOD_CHANGE;

			float dly;
			if (!fadeparmex.GetNextFloat(dly))
			{	
				dly = 0.0f;
			}

			actioninfos.push_back(new ActionInfo(PreciseAdd(gWorldTime.GetTime(),dly),
												calltype,
												moodname,
												remInNode));
		}

	}

	DWORD sz = actioninfos.size();
	if (sz == 0)
	{
		// Early out: no valid fade
		return false;
	}

	if (!parsed_ok)
	{
		// We failed, but we need to remove the ptrs that did parse OK
		for (DWORD j = 0; j < sz; ++j)
		{
			delete actioninfos[j];
		}
		return false;
	}

	if (gServer.IsMultiPlayer())
	{
		FuBi::BitPacker packer;

		packer.XferCount( sz );
		for (DWORD k = 0; k < sz; ++k)
		{
			actioninfos[k]->XferForSync(packer);
		}

		FuBi::AddressColl addresses;
		gGoDb.FillAddressesForFrustum( (FrustumId)remInNodeMask, addresses);
		
		FuBi::AddressColl::iterator it;
		FuBi::AddressColl::iterator ibegin = addresses.begin();
		FuBi::AddressColl::iterator iend = addresses.end();
		
		for ( it = ibegin ; (it != iend) && (*it != NULL) ; ++it )
		{
			FUBI_SET_RPC_PEEKADDRESS( (*it) );
			RCSendActionToPartyMembersRemainingInNode( packer.GetSavedBits() );
		}
	}

	// Perform the action on all on the server

	for (DWORD j = 0; j < sz; ++j)
	{
		AddActionInfo(actioninfos[j]->m_Time,actioninfos[j]);
	}

	return true;
	
}

void TriggerSys::RCSendActionToPartyMembersRemainingInNode(  const_mem_ptr ptr )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCSendActionToPartyMembersRemainingInNode );
	FuBi::BitPacker packer( ptr );
	SendActionToPartyMembersRemainingInNode( packer );
}

void TriggerSys::SendActionToPartyMembersRemainingInNode( FuBi::BitPacker& packer )
{

	DWORD count;

	packer.XferCount( count );

	for (DWORD i = 0; i < count; ++i)
	{
		ActionInfo* fi = new ActionInfo;
		fi->XferForSync(packer);
		AddActionInfo(fi->m_Time,fi);
	}

}


///////////////////////////////////////////////////////////////////////////////////

bool TriggerSys::Xfer(FuBi::PersistContext& persist)
{
	if ( persist.IsRestoring() )
	{
		// Blow them out, just to be on the safe side...
		m_Signals.clear();
		m_TrigBitsDb.clear();
		m_ActionInfos.clear();
	}

	persist.Xfer(    "m_CurrentSeqNum",					m_CurrentSeqNum	);
	persist.XferSet( FuBi::XFER_HEX, "m_Signals",		m_Signals		);
	persist.XferMap( FuBi::XFER_HEX, "m_TrigBitsDb",	m_TrigBitsDb	);
	persist.XferMap( "m_ActionInfos",						m_ActionInfos		);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////
bool TriggerSys::Shutdown(void)
{
	gpgeneric( "TriggerSys :: Shutdown\n" );

	stdx::for_each( m_Groups, stdx::delete_second_by_ptr() );
	m_Groups.clear();

	TriggerIter it = m_ValidTriggers.begin();
	for (;it != m_ValidTriggers.end(); ++it)
	{
		(*it).second->Shutdown();
	}

	m_ValidTriggers.clear();
	m_SpatialTriggers.clear();

	m_Signals.clear();
	m_TrigBitsDb.clear();
	m_ActionInfos.clear();
	
	return true;
}

#if !GP_RETAIL
///////////////////////////////////////////////////////////////////////////////////
void TriggerSys::GroupDrawHelper(const SiegePos gpos, const gpstring& gname, const gpstring& lbl )
{
	Critical::Lock locker( m_TrigSysCritical );
	
	GroupIter findGroup = m_Groups.find( gname );
	
	if ( findGroup != m_Groups.end() )
	{
		if (!findGroup->second->IsValidGroup())
		{
			return;
		}

		GroupLink::WatcherSet* watchers = findGroup->second->GetWatchers();
		GroupLink::MemberSet* members = findGroup->second->GetMembers();
		
		if (gGoDb.IsEditMode())
		{
			gpstring errmsg;
			if (watchers->empty())
			{
				errmsg.appendf("Warning: Redundant Trigger Group [%s]\nNo conditions exist that watch over it\n\n",gname.c_str());
				if (!members->empty())
				{
					errmsg.appendf("Redundant member triggers:\n");
					for ( GroupLink::MemberIter i = members->begin(); i != members->end(); ++i )
					{
						Trigger* trig;
						if ( FetchTriggerByTruid(*i,trig) )
						{
							errmsg.appendf("\t%s:[%s:0x%08x]\n",
								trig->GetCurrentActive() ? "A":"D",
								trig->GetOwner() ? trig->GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
								trig->GetOwner() ? MakeInt(trig->GetOwner()->GetScid()) : -1
								);
						}
					}
				}
			}
			if (members->empty())
			{
				errmsg.appendf("Warning: Redundant Trigger Group [%s]\nIt has no member triggers\n\n",gname.c_str());
				if (!watchers->empty())
				{
					errmsg.appendf("Redundant watching conditions:\n");
					for ( GroupLink::WatcherIter i = watchers->begin(); i != watchers->end(); ++i )
					{
						Condition *watchcond;
						if ( FetchConditionByCuid( (*i),watchcond)  )
						{
							errmsg.appendf("\t%s[%s:0x%08x:0x%04x]\n",
								watchcond->IsDormant()?"X:":"",
								(watchcond->GetTrigger() && watchcond->GetTrigger()->GetOwner()) ? watchcond->GetTrigger()->GetOwner()->GetTemplateName() : "UNKNOWN_OWNER",
								(watchcond->GetTrigger() && watchcond->GetTrigger()->GetOwner()) ? MakeInt( watchcond->GetTrigger()->GetOwner()->GetScid()) : -1,
								watchcond->GetID()
								);
						}
					}
				}
			}
			if (!errmsg.empty())
			{
				MessageBoxEx( 0, errmsg.c_str(), "Trigger Performance Issue", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			}
		}

		if (members->empty() || watchers->empty())
		{
			findGroup->second->SetValidGroup(false);
			return;
		}

		// Are any watchers/members occupied?
		bool watchers_occupied = false;

		for (GroupLink::WatcherIter wit = watchers->begin(); wit != watchers->end(); ++wit)
		{
			Condition *watchcond;
			if ( FetchConditionByCuid( (*wit),watchcond)  )
			{
				watchers_occupied |= watchcond->HasBoundaryOccupants();	
			}
		}

		SiegePos p0 = gpos;
		if (gWorldOptions.TestDebugHudOptions( DHO_LABELS ))
		{
			SiegePos p0 = gpos;
			p0.pos.y += 2.15f;
			gWorld.DrawDebugScreenLabel( p0, lbl );	
			p0 = gpos;
		}
	
		p0.pos.y += 0.75f;

		gWorld.DrawDebugWedge(p0, 0.25f , vector_3(0,0,1), 0 ,0xFFFF00FF, watchers_occupied );

		if ( members->size() > 0 )
		{
			for ( GroupLink::MemberIter i = members->begin(); i != members->end(); ++i )
			{
				Trigger* trig;
				if ( FetchTriggerByTruid(*i,trig) )
				{
					if ( trig->GetOwner() && trig->GetOwner()->HasPlacement() )
					{
						ConditionSetIter wcit = trig->m_WatchedByConditions.begin();
						bool found_a_watcher = false;
						for (; !found_a_watcher && (wcit != trig->m_WatchedByConditions.end()); ++wcit)
						{
							Cuid wcid((*wcit)->GetTrigger()->GetTruid(),(*wcit)->GetID());
							found_a_watcher = (watchers->find(wcid) != watchers->end());
						}
						if (!found_a_watcher)
						{
							gperrorf(("ERROR: Trigger Group [%s] corrupt: WatchedByCondition is missing",trig->GetGroupName()));
						}
						
						bool members_occupied = false;
						for (ConditionConstIter mcit = trig->GetConditions().begin(); mcit != trig->GetConditions().end(); ++mcit)
						{
							members_occupied = (*mcit).second->HasBoundaryOccupants();
						}

						SiegePos p1 = trig->GetOwner()->GetPlacement()->GetPosition();
						p1.pos.y += 0.8f;
						gWorld.DrawDebugWedge(p1, 0.15f , vector_3(0,0,1), 0 ,0xFFFF99FF, members_occupied );
						gWorld.DrawDebugArc( p1, p0 , 0xFFFF00FF, "", members_occupied );
					}
				}
			}
		}

	}
}

#endif


}  // end namespace trigger

