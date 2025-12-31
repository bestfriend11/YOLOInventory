#include "precomp_world.h"

#include "Enchantment.h"
#include "Flamethrower.h"
#include "Flamethrower_FuBi_Support.h"
#include "FormulaHelper.h"
#include "FuBiPersist.h"
#include "FuBiTraits.h"
#include "Go.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoDb.h"
#include "GoDefend.h"
#include "GoMind.h"
#include "GoSupport.h"
#include "GoMagic.h"
#include "Rules.h"
#include "Server.h"
#include "Stdhelp.h"
#include "WorldFx.h"
#include "WorldMessage.h"
#include "WorldTime.h"


FUBI_EXPORT_ENUM( eAlteration, ALTER_BEGIN, ALTER_COUNT );

static const char* s_AlterationStrings[] =
{
#	define   ENCHANTMENT_IS_STRING 1
#	include "Enchantment.inc"
};

static bool s_AlterationActorOnly[] =
{
#	define   ENCHANTMENT_IS_BITS 1
#	include "Enchantment.inc"
};


COMPILER_ASSERT( ELEMENT_COUNT( s_AlterationStrings ) == ALTER_COUNT );

static stringtool::EnumStringConverter s_AlterationConverter( s_AlterationStrings, ALTER_BEGIN, ALTER_END, ALTER_INVALID );

const char* ToString( eAlteration e )
	{  return ( s_AlterationConverter.ToString( e ) );  }
bool FromString( const char* str, eAlteration& e )
	{  return ( s_AlterationConverter.FromString( str, rcast <int&> ( e ) ) );  }







EnchantmentStorage::EnchantmentStorage( const EnchantmentStorage& other )
{
	*this = other;
}


EnchantmentStorage::~EnchantmentStorage( void )
{
	stdx::for_each( m_Enchantments,   stdx::delete_by_ptr() );
}


EnchantmentStorage& EnchantmentStorage::operator = ( const EnchantmentStorage& other )
{
	gpassert( !other.m_bCurrentlyUpdating );

	m_bCurrentlyUpdating = false;
	stdx::for_each( m_Enchantments,   stdx::delete_by_ptr() );

	m_Enchantments  .clear();

	AddEnchantments( other );

	return ( *this );
}


void
EnchantmentStorage::DuplicateEnchantment( const Enchantment &enchantment, Goid newgo )
{
	GoHandle hNewGO( newgo );
	if ( hNewGO )
	{
		m_Enchantments.push_back( new Enchantment( enchantment, hNewGO ) );
	}
}


bool
EnchantmentStorage::Xfer( FuBi::PersistContext& persist )
{
	gpassert( !m_bCurrentlyUpdating );

	m_Enchantments.Xfer( persist, "m_Enchantments"   );
	
	return ( true );
}


bool
EnchantmentStorage::Load( FastFuelHandle fuel )
{
	bool success = true;

	FastFuelHandleColl enchantments;
	fuel.ListChildren( enchantments );
	FastFuelHandleColl::const_iterator i, ibegin = enchantments.begin(), iend = enchantments.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		// if it's named, then check for override
		Enchantment* base = NULL;
		if ( !same_no_case( (*i).GetName(), "*" ) )
		{
			EnchantmentVector::iterator j, jbegin = m_Enchantments.begin(), jend = m_Enchantments.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( (*j)->GetName().same_no_case( (*i).GetName() ) )
				{
					base = *j;
					break;
				}
			}
		}

		// override?
		if ( base )
		{
			// existing one, override it
			if ( !base->Load( *i ) )
			{
				success = false;
			}
		}
		else
		{
			// new one, load it
			std::auto_ptr <Enchantment> enchantment( new Enchantment );
			if ( enchantment->Load( *i ) )
			{
				m_Enchantments.push_back( enchantment.release() );
			}
			else
			{
				success = false;
			}
		}
	}
	return ( success );
}


void
EnchantmentStorage::AddEnchantments( const EnchantmentStorage& other )
{
	EnchantmentVector::const_iterator i, begin = other.m_Enchantments.begin(), end = other.m_Enchantments.end();
	for ( i = begin ; i != end ; ++i )
	{
		m_Enchantments.push_back( new Enchantment( **i ) );
	}
}


bool
EnchantmentStorage::HasEnchantment( const gpstring &sDescription ) const
{
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), iend = m_Enchantments.end();

	for( ; i != iend; ++i )
	{
		if( sDescription.same_no_case( (*i)->GetDescription() ) )
		{
			return true;
		}
	}

	return false;
}


bool
EnchantmentStorage::HasEnchantmentNamed( const gpstring &sName ) const
{
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), iend = m_Enchantments.end();

	for( ; i != iend; ++i )
	{
		if( sName.same_no_case( (*i)->GetName() ) )
		{
			return true;
		}
	}

	return false;
}


bool
EnchantmentStorage::HasSingleInstanceEnchantment( const EnchantmentStorage &enchantments ) const
{
	eAlteration this_alteration = ALTER_INVALID;
	gpstring this_description;

	// First check to see if there are any single instance enchantments here
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), i_end = m_Enchantments.end();

	for(; i != i_end; ++i )
	{
		if( (*i)->IsSingleInstanceOnly() )
		{
			this_alteration = (*i)->GetAlterationType();
			this_description = (*i)->GetDescription();
			break;
		}
	}

	if( this_alteration == ALTER_INVALID )
	{
		return false;
	}

	// If arrived here then there was a single instance enchantment found loacally - now see if there is a matching one
	// in the storage referencedd passed in

	i = enchantments.GetEnchantments().begin();
	i_end = enchantments.GetEnchantments().end();

	for(; i != i_end; ++i )
	{
		if( (*i)->IsSingleInstanceOnly() )
		{
			if( (this_alteration == (*i)->GetAlterationType()) && (this_description = (*i)->GetDescription()) )
			{
				return true;
			}
		}
	}
	return false;
}


float
EnchantmentStorage::GetLongestDuration( Goid target, Goid source, Goid item, bool bEvaluate ) const
{
	GoHandle hTarget( target );
	GoHandle hItem	( item  );

	float duration = 0, dur = 0;

	if( hTarget && hItem )
	{
		EnchantmentVectorCI iEnchantment = m_Enchantments.begin(), i_end = m_Enchantments.end();

		for(; iEnchantment != i_end; ++iEnchantment )
		{
			if( bEvaluate )
			{
				FormulaHelper::EvaluateFormula( (*iEnchantment)->GetDurationString(), NULL, dur, target, source, item );
			}
			else
			{
				if( item == (*iEnchantment)->GetItemGoid() )
				{
					dur = (*iEnchantment)->GetDuration() - (*iEnchantment)->GetElapsed();
				}
			}

			duration = max_t( dur, duration );
		}
	}
	return duration;
}


bool
EnchantmentStorage::GetEnchantment( const gpstring &sDescription, Enchantment **pEnchantment )
{
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), iend = m_Enchantments.end();

	for( ; i != iend; ++i )
	{
		if( sDescription.same_no_case( (*i)->GetDescription() ) )
		{
			*pEnchantment = *i;
			return true;
		}
	}
	return false;
}


float
EnchantmentStorage::GetEnchantmentTotal( const gpstring &sName )
{
	float total = 0.0f;
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), iend = m_Enchantments.end();

	for( ; i != iend; ++i )
	{
		if( sName.same_no_case( (*i)->GetName() ) )
		{
			total += (*i)->GetValue();			
		}
	}
	
	return total;
}


bool
EnchantmentStorage::GetFirstHitEffectInfo( gpstring & sScriptName, gpstring & sScriptParams ) const
{
	EnchantmentVectorCI i = m_Enchantments.begin(), i_end = m_Enchantments.end();

	for(; i != i_end; ++i )
	{
		if( (*i)->GetEffectScriptHit().empty() )
		{
			continue;
		}

		sScriptName = (*i)->GetEffectScriptHit();
		sScriptParams = (*i)->GetEffectScriptHitParams();

		return true;
	}

	return false;
}


void
EnchantmentStorage::DoAlterations( Goid target_object, Goid source_object, Goid item_object, bool bApplyInnate ) const
{
	GoHandle hTarget( target_object );
	GoHandle hSource( source_object );
	GoHandle hItem( item_object );

	// There always has to be a target object and a spell, if there isn't then abort...
	if( hTarget && hItem )
	{
		EnchantmentVectorCI iEnchantment = m_Enchantments.begin();
		for (; iEnchantment != m_Enchantments.end(); ++iEnchantment )
		{
			// Check to see if this enchantment can combine with another
			Enchantment *pHostEnchantment = NULL;

			if( gGoDb.GetEnchantment( target_object, (*iEnchantment)->GetDescription(), &pHostEnchantment ) &&
				( pHostEnchantment->GetAlterationType() == (*iEnchantment)->GetAlterationType() ) &&
				  (((*iEnchantment)->IsInnateEnchantment() && bApplyInnate) || !(*iEnchantment)->IsInnateEnchantment()) )
			{
				// Only enchantments that originate from items that are spells can combine
				if( hItem->IsSpell() )
				{
					// There is already an enchantment on the target with the same alteration type and
					// description so just extend the current duration based on a simple formula

					float new_dur = 0, new_value = 0;

					if ( !FormulaHelper::EvaluateFormula( (*iEnchantment)->GetDurationString(), NULL, new_dur, target_object, source_object, item_object ) )
					{
						gpwarningf(( "%s - invalid enchantment expression, duration = %s\n", (*iEnchantment)->GetDescription().c_str(), (*iEnchantment)->GetDurationString().c_str() ));
						return;
					}
					if ( !FormulaHelper::EvaluateFormula( (*iEnchantment)->GetValueString(), NULL, new_value, target_object, source_object, item_object ) )
					{
						gpwarningf(( "%s - invalid enchantment expression, value = %s\n", (*iEnchantment)->GetDescription().c_str(), (*iEnchantment)->GetValueString().c_str() ));
						return;
					}

					float new_power = new_dur * new_value;
					float old_power = pHostEnchantment->GetDuration() * pHostEnchantment->GetValue();

					float divisor = max_t( new_value, pHostEnchantment->GetValue() );

					float new_duration = ( new_power + old_power ) / divisor;

					pHostEnchantment->SetDuration( new_duration );
					pHostEnchantment->SetValue( divisor );
					pHostEnchantment->SetItemGoid( item_object );
				}
				else
				{
					(*iEnchantment)->DoAlterations( hTarget, hSource, hItem );
				}
			}
			else if ( ((*iEnchantment)->IsInnateEnchantment() && bApplyInnate) || !(*iEnchantment)->IsInnateEnchantment() )
			{
				// Start the equip effect script if one is specified
				if( !(*iEnchantment)->GetEffectScriptEquip().empty() )
				{
					SFxSID id = gWorldFx.RunScript( (*iEnchantment)->GetEffectScriptEquip(), item_object, target_object, "", item_object, WE_UNKNOWN  );
					(*iEnchantment)->SetEffectScriptEquipID( id );
				}

				(*iEnchantment)->DoAlterations( hTarget, hSource, hItem );
			}
		}
	}
}


bool
EnchantmentStorage::DoAlterationsByName( Goid target_object, Goid source_object, Goid item_object, const char *sName ) const
{
	GoHandle hTarget( target_object );
	GoHandle hSource( source_object );
	GoHandle hItem( item_object );

	if( hTarget && hItem )
	{
		bool bEnchantmentApplied = false;

		EnchantmentVectorCI iEnchantment = m_Enchantments.begin(), iend = m_Enchantments.end();
		for (; iEnchantment != iend; ++iEnchantment )
		{
			// Specified enchantment found so apply or combine
			if( (*iEnchantment)->GetName().same_no_case( sName ) )
			{
				// Check to see if there is already an enchantment running with the alteration type or description
				Enchantment *pHostEnchantment = NULL;
				if( gGoDb.GetEnchantment( target_object, (*iEnchantment)->GetDescription(), &pHostEnchantment ) )
				{
					if( pHostEnchantment->GetAlterationType() == (*iEnchantment)->GetAlterationType() )
					{
						// There is already an enchantment on the target with the same alteration type and
						// description so just extend the current duration based on a simple formula

						float new_dur = 0, new_value = 0;

						if ( !FormulaHelper::EvaluateFormula( (*iEnchantment)->GetDurationString(), NULL, new_dur, target_object, source_object, item_object ) )
						{
							gpwarningf(( "%s - invalid enchantment expression, duration = %s\n", (*iEnchantment)->GetDescription().c_str(), (*iEnchantment)->GetDurationString().c_str() ));
							return false;
						}
						if ( !FormulaHelper::EvaluateFormula( (*iEnchantment)->GetValueString(), NULL, new_value, target_object, source_object, item_object ) )
						{
							gpwarningf(( "%s - invalid enchantment expression, value = %s\n", (*iEnchantment)->GetDescription().c_str(), (*iEnchantment)->GetValueString().c_str() ));
							return false;
						}

						float new_power = new_dur * new_value;
						float old_power = pHostEnchantment->GetDuration() * pHostEnchantment->GetValue();

						float divisor = max_t( new_value, pHostEnchantment->GetValue() );

						float new_duration = ( new_power + old_power ) / divisor;

						pHostEnchantment->SetDuration( new_duration );
						pHostEnchantment->SetItemGoid( item_object );

						bEnchantmentApplied = true;
					}
				}
				else
				{
					(*iEnchantment)->DoAlterations( hTarget, hSource, hItem );
					bEnchantmentApplied = true;
				}
			}
		}
		return bEnchantmentApplied;
	}
	return false;
}


bool
EnchantmentStorage::RemoveEnchantments( Goid item )
{
	bool bRemoveDone = false;

	// For each enchantment in the comparision list check against this list and remove if it matches
	EnchantmentVector::iterator iEnchantment = m_Enchantments.begin();
	while( iEnchantment != m_Enchantments.end() )
	{
		Enchantment* enchantment = *iEnchantment;
		bool removeIt = enchantment->GetItemGoid() == item;

		// If no item then assume we're removing all "external" enchantments
		// (i.e. where item is not self and not in inventory).
		if ( !removeIt && (item == GOID_INVALID) )
		{
			// Cannot be self
			if ( enchantment->GetItemGoid() != enchantment->GetTargetGoid() )
			{
				GoHandle hTarget( enchantment->GetTargetGoid() );
				GoHandle hItem  ( enchantment->GetItemGoid  () );
				if ( hTarget && hItem )
				{
					// Cannot be inventory of our target (it may be an equipped
					// Sword Of Doom).
					if ( !hTarget->IsRelativeOf( hItem ) || !hItem->IsInsideInventory() )
					{
						// Must be ok to remove then
						removeIt = true;
					}
				}
				else
				{
					// No target/item, must be external or deleted
					removeIt = true;
				}
			}
		}

		// Do it
		if ( removeIt )
		{
			// can't delete an enchantment that is currently being updated so mark it for deletion ASAP
			if( IsCurrentlyUpdating() )
			{
				enchantment->MarkForDeletion();
				++iEnchantment;
			}
			else
			{
				RemoveEnchantment( iEnchantment );
				bRemoveDone = true;
			}
		}
		else
		{
			++iEnchantment;
		}
	}
	return bRemoveDone;
}


void EnchantmentStorage::RemoveEnchantment( EnchantmentVector::iterator &iEnchantment )
{
	// Cause re-evaluation of modifiers now that this enchantment is over with
	GoHandle hTarget( (*iEnchantment)->GetTargetGoid() );
	if ( hTarget && hTarget->HasActor() && hTarget->IsScreenPartyMember() )
	{
		hTarget->GetActor()->SetEnchantmentRemoved( true );
	}
	hTarget->SetModifiersDirty();

	// Remove its alterations
	(*iEnchantment)->UndoAlteration();

	// Delete it
	delete ( *iEnchantment );
	iEnchantment = m_Enchantments.erase( iEnchantment );
}


void
EnchantmentStorage::SetAbortScriptID( const gpstring &sName, SFxSID id )
{
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), iend = m_Enchantments.end();

	for( ; i != iend; ++i )
	{
		if( sName.same_no_case( (*i)->GetName() ) )
		{
			(*i)->SetAbortScriptID( id );
		}
	}
}


void
EnchantmentStorage::SetEnchantmentDoneMessage( const gpstring &sName, Goid send_to, eWorldEvent event )
{
	EnchantmentVector::const_iterator i = m_Enchantments.begin(), iend = m_Enchantments.end();

	for( ; i != iend; ++i )
	{
		if( sName.same_no_case( (*i)->GetName() ) )
		{
			(*i)->SetEnchantmentDoneMessage( send_to, event );
		}
	}
}


gpwstring
EnchantmentStorage::MakeToolTipString( bool bPotion ) const
{
	// $$$ note:  the old uiinventorymanager auto-enchantment tooltip maker code
	//            is down below in case it's needed. seems like it would be
	//            better for jake to specifically tag each modifier with a
	//            tooltip rather than attempting to guess at what it should say,
	//            but if we can make it work, then the code is still there as
	//            a basis to go from.

	gpwstring out;
	gpstring temp;

	EnchantmentVector::const_iterator i, ibegin = m_Enchantments.begin(), iend = m_Enchantments.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		temp = (*i)->GetDescription();
		if ( !temp.empty() )
		{
			if ( !(*i)->IsInnateEnchantment() )
			{
				out += L"<magic>";
			}
			out += ReportSys::TranslateW( temp );
			out += L'\n';
		}

		switch ( (*i)->GetAlterationType() )
		{
		case ALTER_LIFE:
			if ( bPotion )
			{
				out += L"<magic>";
				if ( (*i)->GetValue() > 0 )
				{
					out.appendf( gpwtranslate( $MSG$ "Restore Health: %0.0f" ), (*i)->GetValue() );
				}
				else if ( (*i)->GetValue() == 0 )
				{
					out.append( gpwtranslate( $MSG$ "Restore Health: EMPTY" ) );
				}
				out += L'\n';
			}
			break;
		case ALTER_MANA:
			if ( bPotion )
			{
				out += L"<magic>";
				if ( (*i)->GetValue() > 0 )
				{
					out.appendf( gpwtranslate( $MSG$ "Restore Mana: %0.0f" ), (*i)->GetValue() );
				}
				else if ( (*i)->GetValue() == 0 )
				{
					out.append( gpwtranslate( $MSG$ "Restore Mana: EMPTY" ) );
				}
				else
				{
					out.appendf( gpwtranslate( $MSG$ "Take Away Mana: %0.0f" ), (*i)->GetValue() );
				}
				out += L'\n';
			}
			break;
		}
	}
	return ( out );
}


void EnchantmentStorage::Update( float deltaTime, bool forActor )
{
	m_bCurrentlyUpdating = true;

	// Process active enchantments
	EnchantmentVectorI iActive = m_Enchantments.begin();
	while ( iActive != m_Enchantments.end() )
	{
		bool actorOnly = s_AlterationActorOnly[ (*iActive)->GetAlterationType() ];
		if ( actorOnly == forActor )
		{
			if ( (*iActive)->Update( deltaTime ) )
			{
				++iActive;
			}
			else
			{
				// Cause re-evaluation of modifiers now that this enchantment is over with
				(*iActive)->SignalModifiersDirty();

				GoHandle hTarget( (*iActive)->GetTargetGoid() );
				if ( hTarget && hTarget->HasActor() && hTarget->IsScreenPartyMember() )
				{
					hTarget->GetActor()->SetEnchantmentRemoved( true );
				}

				delete ( *iActive );
				iActive = m_Enchantments.erase( iActive );
			}
		}
		else
		{
			++iActive;
		}
	}

	// Process any delete requests
	ProcessDeleteRequests();

	m_bCurrentlyUpdating = false;
}


void EnchantmentStorage::ProcessDeleteRequests( void )
{
	EnchantmentVectorI iActive = m_Enchantments.begin();
	while ( iActive != m_Enchantments.end() )
	{
		if( (*iActive)->IsMarkedForDeletion() )
		{
			RemoveEnchantment( iActive );
		}
		else
		{
			++iActive;
		}
	}
}



//
// --- Enchantment
//
Enchantment::Enchantment( Go* target )
	: m_sMaxValue          ( "0" )
	, m_sDuration          ( "0" )
	, m_sFrequency         ( "0" )
	, m_sTransferEfficiency( "0" )
	, m_sInitialDelay      ( "0" )
{
	m_Alteration			= ALTER_INVALID;
	m_EffectScriptID		= 0;
	m_EffectScriptEquipID	= 0;
	m_AbortScriptID			= 0;

	m_Value					= 0;
	m_MaxValue				= 0;
	m_bMultiplyValue		= false;
	m_Duration				= 0;
	m_Frequency				= 0;
	m_TransferEfficiency	= 0;

	m_InitialDelay			= 0;

	m_bEnhancement			= true;
	m_bPermanent			= false;
	m_bTransmissible		= false;
	m_bSingleInstanceOnly	= false;
	m_bIsValueLimited		= false;
	m_bIsTargetTransfer		= false;
	m_bIsSourceTransfer		= false;
	m_bIsOffensiveTransfer	= false;

	m_bUITimerVisible		= false;
	m_bAbortOnMin			= false;
	m_bAbortOnMax			= false;
	m_bActiveOnParty		= false;
	m_bInnateEnchantment	= false;

	m_bMarkedForDeletion	= false;

	m_UndoValue				= 0;
	m_Elapsed				= 0;
	m_Interval				= 0;

	m_InitialDelay_reset	= 0;

	m_DoneWorldMsgSendTo	= GOID_INVALID;
	m_DoneWorldMsgEvent		= WE_INVALID;

	m_Target				= target?target->GetGoid():GOID_INVALID;
	m_Source				= GOID_INVALID;
	m_Item					= GOID_INVALID;
}


Enchantment::Enchantment( const Enchantment &enchantment, Go* target )
	: m_sName					( enchantment.m_sName                  )
	, m_sEffectScript			( enchantment.m_sEffectScript          )
	, m_sEffectScriptEquip		( enchantment.m_sEffectScriptEquip     )
	, m_sEffectScriptHit		( enchantment.m_sEffectScriptHit	   )
	, m_sEffectScriptHitParams	( enchantment.m_sEffectScriptHitParams )
	, m_sValue					( enchantment.m_sValue				   )
	, m_sMaxValue				( enchantment.m_sMaxValue			   )
	, m_sDuration				( enchantment.m_sDuration              )
	, m_sFrequency				( enchantment.m_sFrequency             )
	, m_sTransferEfficiency		( enchantment.m_sTransferEfficiency    )
	, m_sInitialDelay			( enchantment.m_sInitialDelay          )
	, m_sDescription			( enchantment.m_sDescription           )
	, m_sMonsterType			( enchantment.m_sMonsterType		   )
	, m_sCustomDamage			( enchantment.m_sCustomDamage          )
	, m_sCustomDamageSkrit		( enchantment.m_sCustomDamageSkrit     )
{
	gpassert( !enchantment.m_bMarkedForDeletion );

	m_Alteration			= enchantment.m_Alteration;
	m_EffectScriptID		= enchantment.m_EffectScriptID;
	m_EffectScriptEquipID	= enchantment.m_EffectScriptEquipID;

	m_AbortScriptID			= enchantment.m_AbortScriptID;

	m_Value					= enchantment.m_Value;
	m_MaxValue				= enchantment.m_MaxValue;
	m_bMultiplyValue		= enchantment.m_bMultiplyValue;
	m_Duration				= enchantment.m_Duration;
	m_Frequency				= enchantment.m_Frequency;
	m_TransferEfficiency	= enchantment.m_TransferEfficiency;

	m_InitialDelay			= enchantment.m_InitialDelay_reset;

	m_bEnhancement			= enchantment.m_bEnhancement;
	m_bPermanent			= enchantment.m_bPermanent;
	m_bTransmissible		= enchantment.m_bTransmissible;
	m_bSingleInstanceOnly	= enchantment.m_bSingleInstanceOnly;
	m_bIsValueLimited		= enchantment.m_bIsValueLimited;
	m_bIsTargetTransfer		= enchantment.m_bIsTargetTransfer;
	m_bIsSourceTransfer		= enchantment.m_bIsSourceTransfer;
	m_bIsOffensiveTransfer	= enchantment.m_bIsOffensiveTransfer;

	m_bUITimerVisible		= enchantment.m_bUITimerVisible;
	m_bAbortOnMin			= enchantment.m_bAbortOnMin;
	m_bAbortOnMax			= enchantment.m_bAbortOnMax;
	m_bActiveOnParty		= enchantment.m_bActiveOnParty;
	m_bInnateEnchantment	= enchantment.m_bInnateEnchantment;

	m_sDependName			= enchantment.m_sDependName;
	m_sDependObject			= enchantment.m_sDependObject;

	m_bMarkedForDeletion	= false;

	m_UndoValue				= enchantment.m_UndoValue;

	m_Target				= target?target->GetGoid():GOID_INVALID;
	m_Source				= enchantment.m_Source;
	m_Item					= enchantment.m_Item;

	m_InitialDelay_reset	= enchantment.m_InitialDelay_reset;

	// These values aren't copied so the enchantment gets reset
	m_Elapsed				= 0;
	m_Interval				= 0;

	m_DoneWorldMsgSendTo	= enchantment.m_DoneWorldMsgSendTo;
	m_DoneWorldMsgEvent		= enchantment.m_DoneWorldMsgEvent;
}


bool
Enchantment::Xfer( FuBi::PersistContext& persist )
{
	gpassert( !m_bMarkedForDeletion );

	persist.Xfer      ( "m_Alteration",          m_Alteration          );
	persist.Xfer      ( "m_sName",               m_sName               );
	persist.Xfer      ( "m_sEffectScript",       m_sEffectScript       );
	persist.Xfer      ( "m_sEffectScriptEquip",  m_sEffectScriptEquip  );

	persist.Xfer      ( "m_sEffectScriptHit",		m_sEffectScriptHit			);
	persist.Xfer      ( "m_sEffectScriptHitParams", m_sEffectScriptHitParams    );

	persist.Xfer      ( "m_EffectScriptID",      m_EffectScriptID      );
	persist.Xfer      ( "m_EffectScriptEquipID", m_EffectScriptEquipID );
	persist.Xfer      ( "m_AbortScriptID",       m_AbortScriptID       );

	persist.Xfer      ( "m_sValue",              m_sValue              );
	persist.Xfer      ( "m_sMaxValue",           m_sMaxValue           );
	persist.Xfer      ( "m_sDuration",           m_sDuration           );
	persist.Xfer      ( "m_sFrequency",          m_sFrequency          );
	persist.Xfer      ( "m_sTransferEfficiency", m_sTransferEfficiency );
	persist.Xfer      ( "m_sInitialDelay",       m_sInitialDelay       );

	persist.Xfer      ( "m_Value",               m_Value               );
	persist.Xfer      ( "m_MaxValue",            m_MaxValue            );
	persist.Xfer	  ( "m_bMultiplyValue",		 m_bMultiplyValue	   );
	persist.Xfer      ( "m_Duration",            m_Duration            );
	persist.Xfer      ( "m_Frequency",           m_Frequency           );
	persist.Xfer      ( "m_TransferEfficiency",  m_TransferEfficiency  );
	persist.Xfer	  ( "m_bActiveOnParty",		 m_bActiveOnParty	   );
	persist.Xfer	  ( "m_bInnateEnchantment",  m_bInnateEnchantment  );

	persist.Xfer      ( "m_InitialDelay",        m_InitialDelay        );
	persist.XferQuoted( "m_sDescription",        m_sDescription        );

	persist.Xfer      ( "m_bEnhancement",        m_bEnhancement        );
	persist.Xfer      ( "m_bPermanent",          m_bPermanent          );
	persist.Xfer      ( "m_bTransmissible",      m_bTransmissible      );
	persist.Xfer      ( "m_bSingleInstanceOnly", m_bSingleInstanceOnly );
	persist.Xfer      ( "m_bIsValueLimited",     m_bIsValueLimited     );
	persist.Xfer      ( "m_bIsTargetTransfer",   m_bIsTargetTransfer   );
	persist.Xfer      ( "m_bIsSourceTransfer",   m_bIsSourceTransfer   );
	persist.Xfer	  ( "m_bIsOffensiveTransfer",m_bIsOffensiveTransfer);
	persist.Xfer      ( "m_bUITimerVisible",     m_bUITimerVisible     );
	persist.Xfer      ( "m_bAbortOnMin",		 m_bAbortOnMin         );
	persist.Xfer      ( "m_bAbortOnMax",		 m_bAbortOnMax         );

	persist.Xfer      ( "m_sDependName",         m_sDependName         );
	persist.Xfer      ( "m_sDependObject",       m_sDependObject       );

	persist.Xfer	  ( "m_sMonsterType",		 m_sMonsterType		   );

	persist.Xfer	  ( "m_sCustomDamage",		 m_sCustomDamage       );
	persist.Xfer	  ( "m_sCustomDamageSkrit",	 m_sCustomDamageSkrit  );

	persist.Xfer      ( "m_DoneWorldMsgSendTo",  m_DoneWorldMsgSendTo  );
	persist.Xfer      ( "m_DoneWOrldMsgEvent",   m_DoneWorldMsgEvent   );

	persist.Xfer      ( "m_UndoValue",           m_UndoValue           );
	persist.Xfer      ( "m_Elapsed",             m_Elapsed             );
	persist.Xfer      ( "m_Interval",            m_Interval            );

	persist.Xfer      ( "m_Target",              m_Target              );
	persist.Xfer      ( "m_Source",              m_Source              );
	persist.Xfer	  ( "m_Item",				 m_Item				   );

	persist.Xfer      ( "m_InitialDelay_reset",  m_InitialDelay_reset  );

	return ( true );
}


bool
Enchantment::Load( FastFuelHandle fuel )
{
	bool success = true;
	gpstring temp;

	// Take name
	if ( !same_no_case( fuel.GetName(), "*" ) )
	{
		m_sName = fuel.GetName();
	}

	// First handle loading in the required fields
	fuel.Get( "alteration", temp );
	if ( !temp.empty() )
	{
		FromString( temp, m_Alteration );
	}
	if ( m_Alteration == ALTER_INVALID )
	{
		gperrorf(( "Enchantment must specify valid alteration type, location = '%s'", fuel.GetAddress().c_str() ));
		success = false;
	}

	fuel.Get( "value", m_sValue );
	if ( m_sValue.empty() )
	{
		gperrorf(( "Enchantment must specify a value, location = '%s'", fuel.GetAddress().c_str() ));
		success = false;
	}

	fuel.Get( "description", m_sDescription );

	m_sMaxValue = m_sValue;
	fuel.Get( "max_value",				m_sMaxValue );
	fuel.Get( "is_value_limited",		m_bIsValueLimited );

	// Check to see if value and max value are non-formulaic
	if ( m_bIsValueLimited )
	{
		if (   (m_sValue   .find_first_not_of( ".-0123456789", 0 ) != gpstring::npos)
			|| (m_sMaxValue.find_first_not_of( ".-0123456789", 0 ) != gpstring::npos) )
		{
			gperrorf(( "Enchantment is using a formula for a value-limited amount - must be numeric, location = '%s'", fuel.GetAddress().c_str() ));
			success = false;
		}
	}

	fuel.Get( "multiply_value",			m_bMultiplyValue		);
	fuel.Get( "duration",				m_sDuration				);
	fuel.Get( "frequency",				m_sFrequency			);
	fuel.Get( "effect_script",			m_sEffectScript			);
	fuel.Get( "effect_script_equip",	m_sEffectScriptEquip	);

	fuel.Get( "effect_script_hit",			m_sEffectScriptHit		);
	fuel.Get( "effect_script_hit_params",	m_sEffectScriptHitParams);

	fuel.Get( "transfer_efficiency",	m_sTransferEfficiency );
	fuel.Get( "initial_delay",			m_sInitialDelay       );
	fuel.Get( "is_enhancement",			m_bEnhancement        );
	fuel.Get( "is_permanent",			m_bPermanent          );
	fuel.Get( "is_transmissible",		m_bTransmissible      );
	fuel.Get( "is_single_instance",		m_bSingleInstanceOnly );
	fuel.Get( "is_target_transfer",		m_bIsTargetTransfer   );
	fuel.Get( "is_source_transfer",		m_bIsSourceTransfer   );
	fuel.Get( "is_offensive_transfer",	m_bIsOffensiveTransfer);
	fuel.Get( "is_ui_timer_visible",	m_bUITimerVisible     );
	fuel.Get( "abort_on_min",			m_bAbortOnMin         );
	fuel.Get( "abort_on_max",			m_bAbortOnMax         );
	fuel.Get( "depend_name",			m_sDependName         );
	fuel.Get( "depend_object",			m_sDependObject       );
	fuel.Get( "monster_type",			m_sMonsterType		  );
	fuel.Get( "party_active",			m_bActiveOnParty	  );
	fuel.Get( "custom_damage",			m_sCustomDamage       );
	fuel.Get( "custom_damage_skrit",	m_sCustomDamageSkrit  );
	fuel.Get( "innate_enchantment",		m_bInnateEnchantment  );

	// If there is no macro present then convert the string to a number
	if ( m_sValue.find( "#" ) == gpstring::npos )
	{
		stringtool::Get( m_sValue, m_Value );
	}
	if ( m_sMaxValue.find( "#" ) == gpstring::npos )
	{
		stringtool::Get( m_sMaxValue, m_MaxValue );
	}
	if ( m_sDuration.find( "#" ) == gpstring::npos )
	{
		stringtool::Get( m_sDuration, m_Duration );
	}
	if ( m_sFrequency.find( "#" ) == gpstring::npos )
	{
		stringtool::Get( m_sFrequency, m_Frequency );
	}
	if ( m_sTransferEfficiency.find( "#" ) == gpstring::npos )
	{
		stringtool::Get( m_sTransferEfficiency, m_TransferEfficiency );
	}
	if ( m_sInitialDelay.find( "#" ) == gpstring::npos )
	{
		stringtool::Get( m_sInitialDelay, m_InitialDelay );
		m_InitialDelay_reset = m_InitialDelay;
	}

	return ( success );
}


bool
Enchantment::Update( float deltaTime )
{
	bool stillAlive = true;

	// Don't start the effects of an enchantment until the initial use delay has elapsed
	if ( m_InitialDelay > 0 )
	{
		m_InitialDelay -= deltaTime;
	}
	else
	{
		// Increment elapsed time
		m_Elapsed += deltaTime;

		// Do the alteration if the amount of time has elapsed specified by the frequency
		if ( m_Interval <= 0 )
		{
			m_Interval = m_Frequency;
			stillAlive = !DoAlteration();
		}
		m_Interval -= deltaTime;

		// Remove any expired enchantments ( -1 is infinite )
		if ( (m_Duration != -1.0f) && (m_Elapsed >= m_Duration) )
		{
			if ( !IsPermanent() )
			{
				UndoAlteration();
			}

			stillAlive = false;
		}
	}

	// The enchantment has expired so do expiration stuff
	if( !stillAlive )
	{
		gWorldFx.FinishScript( m_AbortScriptID );
		if( m_DoneWorldMsgSendTo != GOID_INVALID )
		{
			WorldMessage( m_DoneWorldMsgEvent, m_DoneWorldMsgSendTo ).Send();
		}
	}

	return ( stillAlive );
}


bool
EnchantmentVector::Xfer( FuBi::PersistContext& persist, const char* key )
{
	persist.EnterColl( key );

	if ( persist.IsSaving() )
	{
		for ( iterator i = begin() ; i != end() ; ++i )
		{
			persist.AdvanceCollIter();
			(*i)->Xfer( persist );
		}
	}
	else
	{
		while ( persist.AdvanceCollIter() )
		{
			push_back( new Enchantment );
			back()->Xfer( persist );
		}
	}

	persist.LeaveColl();

	return ( true );
}


void
Enchantment::DoAlterations( Go* target, Go* source, Go* item )
{
	gpassert( target != NULL );

	m_Target = (target) ? target->GetGoid() : GOID_INVALID;
	m_Source = (source) ? source->GetGoid() : GOID_INVALID;
	m_Item  = (item)  ? item->GetGoid() : GOID_INVALID;


	// Evaluate what the values should be for this enchantment depending
	// on how the formulas were defined in the gas file.

	if ( !FormulaHelper::EvaluateFormula( m_sValue, NULL, m_Value, m_Target, m_Source, m_Item ) )
	{
		gpwarningf(("%s - invalid enchantment expression, value = %s\n", GetDescription().c_str(), m_sValue.c_str()));
	}
	if ( !FormulaHelper::EvaluateFormula( m_sMaxValue, NULL, m_MaxValue, m_Target, m_Source, m_Item ) )
	{
		gpwarningf(("%s - invalid enchantment expression, max_value = %s\n",GetDescription().c_str(), m_sMaxValue.c_str()));
	}
	if ( !FormulaHelper::EvaluateFormula( m_sDuration, NULL, m_Duration, m_Target, m_Source, m_Item ) )
	{
		gpwarningf(("%s - invalid enchantment expression, duration = %s\n",GetDescription().c_str(), m_sDuration.c_str()));
	}
	if ( !FormulaHelper::EvaluateFormula( m_sFrequency, NULL, m_Frequency, m_Target, m_Source, m_Item ) )
	{
		gpwarningf(("%s - invalid enchantment expression, frequency = %s\n",GetDescription().c_str(),m_sFrequency.c_str()));
	}
	if ( !FormulaHelper::EvaluateFormula( m_sTransferEfficiency, NULL, m_TransferEfficiency, m_Target, m_Source, m_Item ) )
	{
		gpwarningf(("%s - invalid enchantment expression, transfer_efficiency = %s\n",GetDescription().c_str(), m_sTransferEfficiency.c_str()));
	}
	if ( !FormulaHelper::EvaluateFormula( m_sInitialDelay, NULL, m_InitialDelay, m_Target, m_Source, m_Item ) )
	{
		gpwarningf(("%s - invalid enchantment expression, initial_delay = %s\n",GetDescription().c_str(), m_sInitialDelay.c_str()));
	}
	else
	{
		m_InitialDelay_reset = m_InitialDelay;
	}

	if ( (GetDuration() == 0) && (GetFrequency() == 0) && (GetInitialDelay() == 0) )
	{
		// The effects are not going to take any time just make the
		// alterations immediately without copying the object over

		DoAlteration();
		m_Target = GOID_INVALID;
		m_Source = GOID_INVALID;
		m_Item	 = GOID_INVALID;
	}
	else
	{
		// The alterations will take time so duplicate the object and
		// have the object that we are modifying update the enchantment
		// through his update steps

		// Copy it to our host and activate it
		gGoDb.RegisterEnchantment( m_Target, *this );
	}
}


bool
Enchantment::DoAlteration( void )
{
	// Verify that there is a target
	if ( m_Target == GOID_INVALID )
	{
		return false;
	}

	GoHandle	hTarget( m_Target );
	GoActor		*pActorTarget	= NULL;
	GoAspect	*pAspectTarget	= NULL;
	GoAttack	*pAttackTarget	= NULL;
	GoBody		*pBodyTarget	= NULL;
	GoDefend	*pDefendTarget	= NULL;

	if( hTarget )
	{
		const bool bActor = hTarget->HasActor();

		if( bActor )
		{
			pActorTarget = hTarget->GetActor();

			if( hTarget->HasAspect() )
			{
				pAspectTarget = hTarget->GetAspect();
			}

			if( hTarget->HasAttack() )
			{
				pAttackTarget = hTarget->GetAttack();
			}

			if( hTarget->HasBody() )
			{
				pBodyTarget = hTarget->GetBody();
			}

			if ( hTarget->HasDefend() )
			{
				pDefendTarget = hTarget->GetDefend();
			}
		}
		else
		{
			Go *pGo = hTarget->GetParent();

			if( pGo )
			{
				pActorTarget = pGo->QueryActor();

				if( pActorTarget )
				{
					if( pGo->HasAspect() )
					{
						pAspectTarget = pGo->GetAspect();
					}

					if( pGo->HasAttack() )
					{
						pAttackTarget = pGo->GetAttack();
					}

					if( pGo->HasBody() )
					{
						pBodyTarget = pGo->GetBody();
					}

					if( pGo->HasDefend() )
					{
						pDefendTarget = pGo->GetDefend();
					}
				}
			}
		}
	}
	else
	{
		return false;
	}

	GoActor		*pActorSource	= NULL;
	GoAspect	*pAspectSource	= NULL;
	GoAttack	*pAttackSource	= NULL;


	// This alteration might involve doing a transfer from game object to another so determine that
	// now and then get a pointer to that objects stats
	const bool	bIsTransferring = IsSourceTransfer() || IsTargetTransfer();

	if ( bIsTransferring )
	{
		if ( m_Source == GOID_INVALID )
		{
			gpwarningf(("Enchantment %s, not initialized properly for transferring - missing second target", GetDescription().c_str()) );
			return false;
		}
		GoHandle hSource( m_Source );

		if( hSource )
		{
			if( hSource->HasActor() )
			{
				pActorSource = hSource->GetActor();
			}

			if( hSource->HasAspect() )
			{
				pAspectSource = hSource->GetAspect();
			}

			if( hSource->HasAttack() )
			{
				pAttackSource = hSource->GetAttack();
			}
		}
	}

	// This variable is set by each alteration depending on whether or not it makes sense to play the script
	// that goes along with the alteration - if the alteration couldn't be made then script shouldn't be
	// played.
	bool bModificationMade = false;

	// This is set depending on if the flags for abortion on minimization or maximization are set and the condition was met
	// When this is the case the enchentment expires
	bool bAbort = false;

	// Do the actual alterations
	switch( GetAlterationType() )
	{
		case ALTER_LIFE:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				if ( bIsTransferring )
				{
					gpassert( pAspectSource );

					if( pAspectSource )
					{
						GoAspect	*pFrom = 0, *pTo = 0;

						// Set transfer direction
						if ( IsSourceTransfer() )
						{
							pFrom	= pAspectTarget;
							pTo		= pAspectSource;
						}
						else if ( IsTargetTransfer() )
						{
							pFrom	= pAspectSource;
							pTo		= pAspectTarget;
						}

						const eLifeState life_state = pFrom->GetLifeState();

						if( (life_state == LS_ALIVE_CONSCIOUS) || (life_state == LS_ALIVE_UNCONSCIOUS) )
						{
							float life_supply = pFrom->GetCurrentLife();

							const float damage = GetTransferEfficiency() * GetValue();

							// only let the server do the experience rewarding and damaging
							if( ::IsServerLocal() )
							{
								gRules.AwardExperience( m_Source, m_Item, gRules.CalculateExperience( m_Source, m_Item, pFrom->GetGoid(), damage ) );
								gRules.DamageGo( pFrom->GetGoid(), m_Source, m_Item, damage, false );
							}

							// change the life locally on the client as well as server so the client will see smooth life changes
							// this is done on the server too and that is where it all matters
							const float life_transferred = ( m_bIsOffensiveTransfer )?damage:Minimum( life_supply, damage );
							gRules.ChangeLifeLocal( pTo->GetGoid(), life_transferred );

							bModificationMade = true;
						}
					}
				}
				else
				{
					gpassert( pAspectTarget );

					if( pAspectTarget )
					{
						eLifeState life_state = pAspectTarget->GetLifeState();
						const bool bTargetAlive = ( (life_state == LS_ALIVE_CONSCIOUS) || (life_state == LS_ALIVE_UNCONSCIOUS) );

						if ( GetValue() > 0.0f )
						{
							// Adding life
							if ( bTargetAlive )
							{
								float amount_available		= GetValue();
								const float target_life		= pAspectTarget->GetCurrentLife();
								const float target_maxlife	= pAspectTarget->GetMaxLife();

								float new_amount =0.0f;

								if ( IsValueLimited() )
								{
									if ( (target_life + amount_available) > target_maxlife )
									{
										amount_available =  target_maxlife - target_life;
										new_amount = GetValue() - amount_available;
									}
									else
									{
										new_amount=0.0f;
									}

									GoHandle hItem(m_Item);
									SetValue( new_amount );
									if (hItem.IsValid() && hItem->IsPotion() && ::IsServerLocal())
									{
										hItem->GetMagic()->RCSetPotionAmount(new_amount);
									}
									stringtool::Set( new_amount, m_sValue );
								}


								float change_amount = amount_available;

								if( (target_life + amount_available) > target_maxlife )
								{
									change_amount = target_maxlife - target_life;
									if ( change_amount == 0 )
									{
										break;
									}
								}

								gRules.ChangeLife( m_Target, change_amount, RPC_TO_LOCAL );
								bModificationMade = true;
							}
						}
						else
						{
							// Subtracting life
							if ( bTargetAlive )
							{
								gRules.ChangeLife( m_Target, GetValue(), RPC_TO_LOCAL );
								bModificationMade = true;
							}
						}
					}
				}

				// Check to see if this alteration should cause the enchantment to abort
				const float cur = pAspectTarget->GetCurrentLife();
				const float max = pAspectTarget->GetMaxLife();
				bAbort = ( ( m_bAbortOnMin && (GetValue()+cur <=0) ) || ( m_bAbortOnMax && (cur+GetValue() >= max) ) );
#if !GP_RETAIL
			if( cur <= 0 )
			{
				GoHandle hItem(m_Item);

				if( strncmp( hItem->GetTemplateName(), "spell_summon", 10) == 0 )
				{
					gperrorf((" WARNING:  %s has just enchanted %s to death!!!! "
							"if you got this while someone in the game was using a summoned creature then "
							"do not ignore this error and show jessica!!!!!",
							hItem->GetTemplateName(),
							hTarget->GetTemplateName()

							));
				}
			}


#endif
			}
		
		} break;

		case ALTER_MANA:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float	amount_needed	= pAspectTarget->GetMaxMana() - pAspectTarget->GetCurrentMana();
				float	amount_available= 0;

				if ( bIsTransferring )
				{
					gpassert( pAspectSource );

					if( pAspectSource )
					{
						GoAspect *pFrom = 0, *pTo = 0;

						if ( IsSourceTransfer() )
						{
							pFrom	= pAspectTarget;
							pTo		= pAspectSource;
						}
						else if ( IsTargetTransfer() )
						{
							pFrom	= pAspectSource;
							pTo		= pAspectTarget;
						}

						// Mana flows from SOURCE -> TARGET
						float mana_supply = pFrom->GetCurrentMana();

						if ( m_bIsOffensiveTransfer || ((mana_supply >= 1) && (pTo->GetCurrentMana() < pTo->GetMaxMana() )) )
						{
							// ok to transfer mana

							// Make sure we send only the amount that can be stored
							const float max_receivable = pTo->GetMaxMana() - pTo->GetCurrentMana();
							if( !m_bIsOffensiveTransfer && mana_supply > max_receivable )
							{
								mana_supply = max_receivable;
							}

							amount_available = Minimum( mana_supply, GetValue() );

							// Subtract the mana from the sender
							gRules.ChangeMana( pFrom->GetGoid(), -amount_available, RPC_TO_LOCAL );

							// Adjust for how much is lost during the transfer
							const float amount_received = amount_available * GetTransferEfficiency();

							// Receive the mana
							gRules.ChangeMana( pTo->GetGoid(), amount_received, RPC_TO_LOCAL );

							if( amount_received > FLOAT_TOLERANCE )
							{
								bModificationMade = true;
							}
						}
					}
				}
				else
				{
					// Mana is coming from a potion or thin air, etc...
					amount_available = GetValue();

					// Adding mana
					if ( (amount_needed > 0) && (amount_available > 0) )
					{
						float amount_used = amount_needed;

						if ( amount_needed > amount_available )
						{
							amount_used = amount_available;
						}

						float amount_left = amount_available - amount_used;

						if ( IsValueLimited() )
						{
							SetValue( amount_left );
							stringtool::Set( amount_left, m_sValue );
							GoHandle hItem(m_Item);
							if (hItem.IsValid() && hItem->IsPotion() && ::IsServerLocal())
							{
								hItem->GetMagic()->RCSetPotionAmount(amount_left );
							}
						}

						// Alter by the amount that was actually used
						gRules.ChangeMana( pAspectTarget->GetGoid(), amount_used, RPC_TO_LOCAL );

						bModificationMade = true;
					}
					else
					{
					// Subtracting mana
						if( amount_available < 0 )
						{
							if( (pAspectTarget->GetCurrentMana() + amount_available) >= 0 )
							{
								gRules.ChangeMana( pActorTarget->GetGoid(), amount_available, RPC_TO_LOCAL );
								bModificationMade = true;
							}
						}
					}
				}

				// Check to see if this alteration should cause the enchantment to abort
				const float cur = pAspectTarget->GetCurrentMana();
				const float max = pAspectTarget->GetMaxMana();

				bAbort = ( ( m_bAbortOnMin && (cur+amount_available) <= 0 ) || ( m_bAbortOnMax && ((cur+amount_available) >= max) ) );
			}
		} break;

		case ALTER_MAX_LIFE:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float new_life = pAspectTarget->GetMaxLife();

				if( m_bMultiplyValue )
				{
					new_life *= GetValue();
				}
				else
				{
					new_life += GetValue();
				}

				pAspectTarget->SetMaxLife( new_life );
				bModificationMade = true;
			}
		} break;

		case ALTER_LIFE_RECOVERY_UNIT:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float new_lru = pAspectTarget->GetLifeRecoveryUnit();

				if( m_bMultiplyValue )
				{
					new_lru *= GetValue();
				}
				else
				{
					new_lru += GetValue();
				}

				pAspectTarget->SetLifeRecoveryUnit( new_lru );
				bModificationMade = true;
			}
		} break;

		case ALTER_LIFE_RECOVERY_PERIOD:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float new_lrp = pAspectTarget->GetLifeRecoveryPeriod();

				if( m_bMultiplyValue )
				{
					new_lrp *= GetValue();
				}
				else
				{
					new_lrp += GetValue();
				}

				pAspectTarget->SetLifeRecoveryPeriod(  new_lrp );
				bModificationMade = true;
			}
		} break;

		case ALTER_MAX_MANA:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float new_maxmana = pAspectTarget->GetMaxMana();

				if( m_bMultiplyValue )
				{
					new_maxmana *= GetValue();
				}
				else
				{
					new_maxmana += GetValue();
				}

				pAspectTarget->SetMaxMana( new_maxmana );
				bModificationMade = true;
			}
		} break;

		case ALTER_MANA_RECOVERY_UNIT:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float new_mru = pAspectTarget->GetManaRecoveryUnit();

				if( m_bMultiplyValue )
				{
					new_mru *= GetValue();
				}
				else
				{
					new_mru += GetValue();
				}

				pAspectTarget->SetManaRecoveryUnit( new_mru );
				bModificationMade = true;
			}
		} break;

		case ALTER_MANA_RECOVERY_PERIOD:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				float new_mrp = pAspectTarget->GetManaRecoveryPeriod();

				if( m_bMultiplyValue )
				{
					new_mrp *= GetValue();
				}
				else
				{
					new_mrp += GetValue();
				}

				pAspectTarget->SetManaRecoveryPeriod( new_mrp );
				bModificationMade = true;
			}

		} break;

		case ALTER_MOVE_SPEED:
		{
			gpassert( pBodyTarget );

			if( pAspectTarget )
			{
				float new_speed = pBodyTarget->GetAvgMoveVelocity();
	
				if( m_bMultiplyValue )
				{
					new_speed *= GetValue();
				}
				else
				{
					new_speed += GetValue();
				}

				pBodyTarget->SetAvgMoveVelocity( new_speed );
				bModificationMade = true;
			}
		} break;

		case ALTER_STRENGTH:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "strength", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_INTELLIGENCE:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "intelligence", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_DEXTERITY:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "dexterity", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_MELEE:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "melee", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_RANGED:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "ranged", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_NATURE_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "nature magic", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_COMBAT_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pSkill = NULL;
				if ( pActorTarget->GetSkill( "combat magic", &pSkill ) )
				{
					bModificationMade = AlterSkillLevel( pSkill, m_bMultiplyValue, GetValue() );
				}
			}
		} break;

		case ALTER_STRENGTH_TO_INTELLIGENCE:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pStrength = NULL, *pIntelligence = NULL;

				if( pActorTarget->GetSkill( "strength", &pStrength ) && 
					pActorTarget->GetSkill( "intelligence", &pIntelligence ) )
				{
					bModificationMade = TransferSkillLevel( pStrength, pIntelligence, GetValue() );
				}
			}
		} break;


		case ALTER_STRENGTH_TO_DEXTERITY:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pStrength = NULL, *pDexterity = NULL;

				if( pActorTarget->GetSkill( "strength", &pStrength ) &&
					pActorTarget->GetSkill( "dexterity", &pDexterity ) )
				{
					bModificationMade = TransferSkillLevel( pStrength, pDexterity, GetValue() );
				}
			}
		} break;

		case ALTER_INTELLIGENCE_TO_DEXTERITY:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pIntelligence = NULL, *pDexterity = NULL;

				if( pActorTarget->GetSkill( "intelligence", &pIntelligence ) &&
					pActorTarget->GetSkill( "dexterity", &pDexterity ) )
				{
					bModificationMade = TransferSkillLevel( pIntelligence, pDexterity, GetValue() );
				}
			}
		} break;

		case ALTER_MELEE_TO_RANGED:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pMelee = NULL, *pRanged = NULL;

				if( pActorTarget->GetSkill( "melee", &pMelee ) &&
					pActorTarget->GetSkill( "ranged", &pRanged ) )
				{
					bModificationMade = TransferSkillLevel( pMelee, pRanged, GetValue() );
				}
			}
		} break;

		case ALTER_MELEE_TO_NATURE_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pMelee = NULL, *pNatureMagic = NULL;

				if( pActorTarget->GetSkill( "melee", &pMelee ) &&
					pActorTarget->GetSkill( "nature magic", &pNatureMagic ) )
				{
					bModificationMade = TransferSkillLevel( pMelee, pNatureMagic, GetValue() );
				}
			}
		} break;

		case ALTER_MELEE_TO_COMBAT_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pMelee = NULL, *pCombatMagic = NULL;

				if( pActorTarget->GetSkill( "melee", &pMelee ) &&
					pActorTarget->GetSkill( "combat magic", &pCombatMagic ) )
				{
					bModificationMade = TransferSkillLevel( pMelee, pCombatMagic, GetValue() );
				}
			}
		} break;

		case ALTER_RANGED_TO_NATURE_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pRanged = NULL, *pNatureMagic = NULL;

				if( pActorTarget->GetSkill( "ranged", &pRanged ) &&
					pActorTarget->GetSkill( "nature magic", &pNatureMagic ) )
				{
					bModificationMade = TransferSkillLevel( pRanged, pNatureMagic, GetValue() );
				}
			}
		} break;

		case ALTER_RANGED_TO_COMBAT_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pRanged = NULL, *pCombatMagic = NULL;

				if( pActorTarget->GetSkill( "ranged", &pRanged ) &&
					pActorTarget->GetSkill( "combat magic", &pCombatMagic ) )
				{
					bModificationMade = TransferSkillLevel( pRanged, pCombatMagic, GetValue() );
				}
			}
		} break;

		case ALTER_NATURE_MAGIC_TO_COMBAT_MAGIC:
		{
			gpassert( pActorTarget );

			if( pActorTarget )
			{
				Skill *pNatureMagic = NULL, *pCombatMagic = NULL;

				if( pActorTarget->GetSkill( "nature magic", &pNatureMagic ) &&
					pActorTarget->GetSkill( "combat magic", &pCombatMagic ) )
				{
					bModificationMade = TransferSkillLevel( pNatureMagic, pCombatMagic, GetValue() );
				}
			}
		} break;


		case ALTER_MELEE_DAMAGE_MIN:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMinMelee();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMinMelee( new_value );
			}
		} break;
		case ALTER_MELEE_DAMAGE_MAX:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMaxMelee();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMaxMelee( new_value );
			}
		} break;

		case ALTER_RANGED_DAMAGE_MIN:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMinRanged();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMinRanged( new_value );
			}
		} break;
		case ALTER_RANGED_DAMAGE_MAX:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMaxRanged();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMaxRanged( new_value );
			}
		} break;

		case ALTER_CMAGIC_DAMAGE_MIN:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMinCMagic();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMinCMagic( new_value );
			}
		} break;
		case ALTER_CMAGIC_DAMAGE_MAX:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMaxCMagic();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMaxCMagic( new_value );
			}
		} break;

		case ALTER_NMAGIC_DAMAGE_MIN:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMinNMagic();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMinNMagic( new_value );
			}
		} break;
		case ALTER_NMAGIC_DAMAGE_MAX:
		{
			gpassert( pAttackTarget );

			if( pAttackTarget )
			{
				float new_value = pAttackTarget->GetDamageBonusMaxNMagic();

				if( m_bMultiplyValue )
				{
					new_value *= GetValue();
				}
				else
				{
					new_value += GetValue();
				}

				pAttackTarget->SetDamageBonusMaxNMagic( new_value );
			}
		} break;

		case ALTER_CUSTOM_DAMAGE:
		{
			if_gpassert( pAttackTarget )
			{
				if ( !m_sCustomDamage.empty() )
				{
					pAttackTarget->SetCustomDamage( m_sCustomDamage, gpstring::EMPTY, GetValue(), GetMaxValue(), m_sCustomDamageSkrit );
				}
			}
		} break;

		case ALTER_CUSTOM_DAMAGE_MELEE:
		{
			if_gpassert( pAttackTarget )
			{
				if ( !m_sCustomDamage.empty() )
				{
					pAttackTarget->SetCustomDamage( m_sCustomDamage, "melee", GetValue(), GetMaxValue(), m_sCustomDamageSkrit );
				}
			}
		} break;

		case ALTER_CUSTOM_DAMAGE_RANGED:
		{
			if_gpassert( pAttackTarget )
			{
				if ( !m_sCustomDamage.empty() )
				{
					pAttackTarget->SetCustomDamage( m_sCustomDamage, "ranged", GetValue(), GetMaxValue(), m_sCustomDamageSkrit );
				}
			}
		} break;

		case ALTER_CUSTOM_DAMAGE_CHANCE:
		{
			if_gpassert( pAttackTarget )
			{
				if ( !m_sCustomDamage.empty() )
				{
					pAttackTarget->SetCustomDamageChance( m_sCustomDamage, GetValue() );
				}
			}
		} break;

		case ALTER_CUSTOM_DAMAGE_CHANCE_MELEE:
		{
			if_gpassert( pAttackTarget )
			{
				if ( !m_sCustomDamage.empty() )
				{
					pAttackTarget->SetCustomDamageChance( gpstring( m_sCustomDamage ).append( "_melee" ), GetValue() );
				}
			}
		} break;

		case ALTER_CUSTOM_DAMAGE_CHANCE_RANGED:
		{
			if_gpassert( pAttackTarget )
			{
				if ( !m_sCustomDamage.empty() )
				{
					pAttackTarget->SetCustomDamageChance( gpstring( m_sCustomDamage ).append( "_ranged" ), GetValue() );
				}
			}
		} break;

		case ALTER_LIFE_STEAL:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetLifeStealAmount( pAttackTarget->GetLifeStealAmount() + GetValue() );
			}
		} break;

		case ALTER_MANA_STEAL:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetManaStealAmount( pAttackTarget->GetManaStealAmount() + GetValue() );
			}
		} break;

		case ALTER_LIFE_BONUS:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetLifeBonusAmount( pAttackTarget->GetLifeBonusAmount() + GetValue() );
			}
		} break;

		case ALTER_MANA_BONUS:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetManaBonusAmount( pAttackTarget->GetManaBonusAmount() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_CHANCE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageChance( pAttackTarget->GetPiercingDamageChance() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_CHANCE_MELEE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageChanceMelee( pAttackTarget->GetPiercingDamageChanceMelee() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_CHANCE_RANGED:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageChanceRanged( pAttackTarget->GetPiercingDamageChanceRanged() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_CHANCE_AMOUNT:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageChanceAmount( pAttackTarget->GetPiercingDamageChanceAmount() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_CHANCE_AMOUNT_MELEE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageChanceAmountMelee( pAttackTarget->GetPiercingDamageChanceAmountMelee() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_CHANCE_AMOUNT_RANGED:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageChanceAmountRanged( pAttackTarget->GetPiercingDamageChanceAmountRanged() + GetValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_RANGE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageMin( pAttackTarget->GetPiercingDamageMin() + GetValue() );
				pAttackTarget->SetPiercingDamageMax( pAttackTarget->GetPiercingDamageMax() + GetMaxValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_RANGE_MELEE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageMeleeMin( pAttackTarget->GetPiercingDamageMeleeMin() + GetValue() );
				pAttackTarget->SetPiercingDamageMeleeMax( pAttackTarget->GetPiercingDamageMeleeMax() + GetMaxValue() );
			}
		} break;

		case ALTER_PIERCING_DAMAGE_RANGE_RANGED:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetPiercingDamageRangedMin( pAttackTarget->GetPiercingDamageRangedMin() + GetValue() );
				pAttackTarget->SetPiercingDamageRangedMax( pAttackTarget->GetPiercingDamageRangedMax() + GetMaxValue() );
			}
		} break;

		case ALTER_DAMAGE_BONUS_TO_TYPE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetAmountDamageToType( GetValue() );
				pAttackTarget->SetDamageToType( m_sMonsterType );
			}
		} break;

		case ALTER_CHANCE_TO_HIT:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetChanceToHitBonus( pAttackTarget->GetChanceToHitBonus() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_HIT_MELEE:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetChanceToHitBonusMelee( pAttackTarget->GetChanceToHitBonusMelee() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_HIT_RANGED:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetChanceToHitBonusRanged( pAttackTarget->GetChanceToHitBonusRanged() + GetValue() );
			}
		} break;

		case ALTER_EXPERIENCE_GAINED:
		{
			gpassert( pAttackTarget );

			if ( pAttackTarget )
			{
				pAttackTarget->SetExperienceBonus( pAttackTarget->GetExperienceBonus() + GetValue() );
			}
		} break;

		case ALTER_ARMOR:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				float new_defense = pDefendTarget->GetDefense();
				if( m_bMultiplyValue )
				{
					new_defense *= GetValue();
				}
				else
				{
					new_defense += GetValue();
				}
				pDefendTarget->SetDefense( new_defense );
				bModificationMade = true;
			}
		} break;

		case ALTER_INVINCIBILITY:
		{
			gpassert( pAspectTarget );

			if( pAspectTarget )
			{
				pAspectTarget->SetIsInvincible( (GetValue() == 1) );
				bModificationMade = true;
			}
		} break;


		case ALTER_BLOCK_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockDamage( pDefendTarget->GetBlockDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_MELEE_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockMeleeDamage( pDefendTarget->GetBlockMeleeDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_RANGED_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockRangedDamage( pDefendTarget->GetBlockRangedDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_COMBAT_MAGIC_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockCombatMagicDamage( pDefendTarget->GetBlockCombatMagicDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_NATURE_MAGIC_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockNatureMagicDamage( pDefendTarget->GetBlockNatureMagicDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PIERCING_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPiercingDamage( pDefendTarget->GetBlockPiercingDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_CUSTOM_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockCustomDamage( m_sCustomDamage, GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartDamage( pDefendTarget->GetBlockPartDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_MELEE_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartMeleeDamage( pDefendTarget->GetBlockPartMeleeDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_RANGED_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartRangedDamage( pDefendTarget->GetBlockPartRangedDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_COMBAT_MAGIC_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartCombatMagicDamage( pDefendTarget->GetBlockPartCombatMagicDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_NATURE_MAGIC_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartNatureMagicDamage( pDefendTarget->GetBlockPartNatureMagicDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_PIERCING_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartPiercingDamage( pDefendTarget->GetBlockPartPiercingDamage() + GetValue() );
			}
		} break;

		case ALTER_BLOCK_PART_CUSTOM_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockPartCustomDamage( m_sCustomDamage, GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_MELEE_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockMeleeDamageChance( pDefendTarget->GetBlockMeleeDamageChance() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_RANGED_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockRangedDamageChance( pDefendTarget->GetBlockRangedDamageChance() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_CMAGIC_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockCombatMagicChance( pDefendTarget->GetBlockCombatMagicChance() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_NMAGIC_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockNatureMagicChance( pDefendTarget->GetBlockNatureMagicChance() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_MELEE_DAMAGE_AMOUNT:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockMeleeDamageChanceAmount( pDefendTarget->GetBlockMeleeDamageChanceAmount() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_RANGED_DAMAGE_AMOUNT:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockRangedDamageChanceAmount( pDefendTarget->GetBlockRangedDamageChanceAmount() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_CMAGIC_DAMAGE_AMOUNT:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockCombatMagicChanceAmount( pDefendTarget->GetBlockCombatMagicChanceAmount() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_BLOCK_NMAGIC_DAMAGE_AMOUNT:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetBlockNatureMagicChanceAmount( pDefendTarget->GetBlockNatureMagicChanceAmount() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_DODGE_HIT:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetChanceToDodgeHit( pDefendTarget->GetChanceToDodgeHit() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_DODGE_HIT_MELEE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetChanceToDodgeHitMelee( pDefendTarget->GetChanceToDodgeHitMelee() + GetValue() );
			}
		} break;

		case ALTER_CHANCE_TO_DODGE_HIT_RANGED:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetChanceToDodgeHitRanged( pDefendTarget->GetChanceToDodgeHitRanged() + GetValue() );
			}
		} break;

		case ALTER_REFLECT_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetReflectDamageAmount( pDefendTarget->GetReflectDamageAmount() + GetValue() );
			}
		} break;

		case ALTER_REFLECT_DAMAGE_CHANCE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetReflectDamageChance( pDefendTarget->GetReflectDamageChance() + GetValue() );
			}
		} break;

		case ALTER_REFLECT_PIERCING_DAMAGE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetReflectPiercingDamageAmount( pDefendTarget->GetReflectPiercingDamageAmount() + GetValue() );
			}
		} break;

		case ALTER_REFLECT_PIERCING_DAMAGE_CHANCE:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetReflectPiercingDamageChance( pDefendTarget->GetReflectPiercingDamageChance() + GetValue() );
			}
		} break;

		case ALTER_MANA_SHIELD:
		{
			gpassert( pDefendTarget );

			if ( pDefendTarget )
			{
				pDefendTarget->SetManaShield( pDefendTarget->GetManaShield() + GetValue() );
			}
		} break;
	}

	// If a modification to a stat was made then signal for an update
	if ( bModificationMade )
	{
		SignalModifiersDirty();

		// If there is an effect script that is to accompany this enchantment then go ahead and start
		// playing that script now.
		if ( !GetEffectScript().empty() )
		{
			def_tracker tracker = WorldFx::MakeTracker();

			if( bIsTransferring )
			{
				if( IsSourceTransfer() )
				{
					tracker->AddGoTarget( m_Source );
					tracker->AddGoTarget( m_Target );

				}
				else if( IsTargetTransfer() )
				{
					tracker->AddGoTarget( m_Target );
					tracker->AddGoTarget( m_Source );
				}
			}
			else
			{
				// Set the target target
				tracker->AddGoTarget( m_Target );
				// Set the source target
				if( m_Source == GOID_INVALID )
				{
					tracker->AddGoTarget( m_Target );
				}
				else
				{
					tracker->AddGoTarget( m_Source );
				}
			}

			GoHandle hItem( m_Item );

			Goid script_owner = hItem?m_Item:m_Target;

			gWorldFx.SRunScript( GetEffectScript(), tracker, "", script_owner, WE_UNKNOWN );
			SFxSID id = gWorldFx.GetLastScriptID();
			SetEffectScriptID( id );
		}
	}

	// Check dependencies
	if( !m_sDependName.empty() )
	{
		Goid dep_goid = m_Target;

		if( m_sDependObject.same_no_case( "source" ) )
		{
			if( m_Source == GOID_INVALID )
			{
				gpwarningf(("Enchantment (desc = %s) specifies source for dependent object - source unspecified.\n", GetDescription().c_str() ));
			}
			else
			{
				dep_goid = m_Source;
			}
		}

		// Check the enchantment db
		bAbort |= !gGoDb.HasEnchantmentNamed( dep_goid, m_sDependName );
	}

	return bAbort;
}


bool
Enchantment::TransferSkillLevel( Skill *pSkillFrom, Skill *pSkillTo, float transfer_amount )
{
	// level bias cannot change
	// natural level cannot cange

	// indicated level must be at least 1
	// modified level can go negative

	const float from_supply	= pSkillFrom->GetLevel();	// Modified + Level Bias

	if( transfer_amount > from_supply )
	{
		transfer_amount = from_supply - 1;
	}

	bool bSkillAltered = false;

	bSkillAltered |= AlterSkillLevel( pSkillFrom, false, -transfer_amount );
	bSkillAltered |= AlterSkillLevel( pSkillTo,	  false,  transfer_amount );

	return bSkillAltered;
}


bool
Enchantment::AlterSkillLevel( Skill *pSkill, bool bMultiply, float level_delta )
{
	bool bSkillAltered = false;
	float current_level		= pSkill->GetLevelNoBias();
	const float level_bias	= pSkill->GetLevelBias();

	if( bMultiply )
	{
		if( pSkill->GetLevel() < (pSkill->GetMaxLevel() - level_bias) )
		{
			const float calced_level = current_level + (current_level * level_delta);

			if( calced_level > pSkill->GetMaxLevel() )
			{
				current_level = pSkill->GetMaxLevel();
			}
			else if( current_level < 1.0f )
			{
				current_level = 1.0f;
			}
			bSkillAltered = true;
		}
	}
	else
	{
		const float max_level	 = pSkill->GetMaxLevel() - level_bias;
		const float calced_level = current_level + level_delta;

		if( level_delta > 0 )
		{
			if( calced_level > max_level )
			{
				current_level = max_level;
			}
			else
			{
				current_level += level_delta;
			}
		}
		else if( level_delta < 0 )
		{
			if( calced_level < (-level_bias) )
			{
				current_level = -level_bias;
			}
			else
			{
				current_level += level_delta;
			}
		}

		bSkillAltered = (level_delta != 0);
	}

	if( bSkillAltered )
	{
		pSkill->SetLevel( current_level );
	}

	return bSkillAltered;
}


void
Enchantment::UndoAlteration( void )
{
	if ( !GetEffectScript().empty() )
	{
		gWorldFx.FinishScript( GetEffectScriptID() );
	}

	if ( !GetEffectScriptEquip().empty() )
	{
		gWorldFx.StopScript( GetEffectScriptEquipID() );
	}
}


void
Enchantment::SetEnchantmentDoneMessage( Goid send_to, eWorldEvent event )
{
	m_DoneWorldMsgSendTo	= send_to;
	m_DoneWorldMsgEvent		= event;
}


bool
Enchantment::SetValue( const gpstring &sVal )
{
	if ( m_bIsValueLimited )
	{
		stringtool::Get( sVal, m_Value );
		m_Value = min_t( m_Value, m_MaxValue );
		m_sValue.assignf( "%g", m_Value );
		return true;
	}
	return false;
}


void
Enchantment::SignalModifiersDirty( void ) const
{
	GoHandle hTarget( m_Target );
	GoHandle hSource( m_Source );

	if( hTarget )
	{
		hTarget->SetModifiersDirty();
	}

	if( hSource )
	{
		hSource->SetModifiersDirty();
	}
}


