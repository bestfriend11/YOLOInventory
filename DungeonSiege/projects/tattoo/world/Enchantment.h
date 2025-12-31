#pragma once
/*=======================================================================================

  EnchantmentStorage

  purpose:	This class defined a "dynamic property" which modifies the properties of a
			Game Object.

  author:	Rick Saenz

  (C)opyright Gas Powered Games 2000

---------------------------------------------------------------------------------------*/
#ifndef __ENCHANTMENT_STORAGE_H
#define __ENCHANTMENT_STORAGE_H


class WorldMessage;
class Enchantment;
class Skill;
enum eAlteration;


#include "GoDefs.h"
#include "flamethrower_types.h"



struct EnchantmentVector : std::vector <my Enchantment*>
{
	bool Xfer( FuBi::PersistContext& persist, const char* key );
};
typedef EnchantmentVector::iterator			EnchantmentVectorI;
typedef EnchantmentVector::const_iterator	EnchantmentVectorCI;



class EnchantmentStorage
{
public:

	// --- Existence

	EnchantmentStorage				( void ) : m_bCurrentlyUpdating( false ) {}
	EnchantmentStorage				( const EnchantmentStorage& other );
	~EnchantmentStorage				( void );

	EnchantmentStorage& operator =	( const EnchantmentStorage& other );

	void		DuplicateEnchantment( const Enchantment &enchantment, Goid newgo );



	// --- Persist

	bool		Xfer				( FuBi::PersistContext& persist );



	// --- Initialization

	// Loads a set of enchantments from a fuel address
	bool		Load				( FastFuelHandle fuel );

	// Adds enchantments from another storage into this one
	void		AddEnchantments		( const EnchantmentStorage& other );



	// --- Query

	// True if exist
	bool		HasEnchantments				( void ) const			{ return !m_Enchantments.empty(); }
	bool		HasEnchantment				( const gpstring &sDescription ) const;
	bool		HasEnchantmentNamed			( const gpstring &sName ) const;
	bool		HasSingleInstanceEnchantment( const EnchantmentStorage &enchantments ) const;

	// Iterate through the enchantments and find the longest occuring duration based on the Gos involved
	float		GetLongestDuration			( Goid target, Goid source, Goid item, bool bEvaluate ) const;

	// Returns a reference to all enchantments - be careful, this is only good for one frame.
	EnchantmentVector&			GetEnchantments		( void )				{ return m_Enchantments; }
	const EnchantmentVector&	GetEnchantments		( void ) const			{ return m_Enchantments; }

	// Get an individual enchantment matching description field
	bool		GetEnchantment				( const gpstring &sDescription, Enchantment **pEnchantment );
	float		GetEnchantmentTotal			( const gpstring &sName );

	// Get the first instance of a hit effects for any enchantments stored in this storage class
	bool		GetFirstHitEffectInfo		( gpstring & sScriptName, gpstring & sScriptParams ) const;


	// --- Application

	// Applies or unapplies alterations to an target object and/or source object
	// There must always be a target object, source object is optional unless using transfer alteration
	void		DoAlterations				( Goid target_object, Goid source_object, Goid item_object, bool bApplyInnate = true) const;

	// Special version of DoAlterations that will only apply enchantments of the specified name
	bool		DoAlterationsByName			( Goid target_object, Goid source_object, Goid item_object, const char *sName ) const;



	// --- Removal

	// Removes enchantments owned by a particular item
	bool		RemoveEnchantments			( Goid item );

private:
	void		RemoveEnchantment			( EnchantmentVector::iterator &iEnchantment );
public:


	// --- Misc

	// Sets the abort script id for an enchantment in storage to message when the enchantment ends or aborts
	void		SetAbortScriptID			( const gpstring &sName, SFxSID id );

	// Sets what worldmessage to send out when an enchantment finishes
	void		SetEnchantmentDoneMessage	( const gpstring &sName, Goid send_to, eWorldEvent event );

	// Builds a tooltip from current enchantments
	gpwstring	MakeToolTipString			( bool bPotion ) const;

	// This bool set to true at beginning of update and false at the end
	bool		IsCurrentlyUpdating			( void ) const			{ return m_bCurrentlyUpdating; }


	// --- Update

	// Applies each enchantments alteration in the collection according to application properties
	void		Update						( float deltaTime, bool forActor );


private:
	void		ProcessDeleteRequests		( void );


	EnchantmentVector	m_Enchantments;

	bool				m_bCurrentlyUpdating;	// for bug #12371
};






class Enchantment
{

public:

	// --- Existence
	Enchantment		( Go* target = NULL );
	Enchantment		( const Enchantment &enchantment, Go* target = NULL );
	~Enchantment	( void )	{}


	// --- Persist
	bool			Xfer				( FuBi::PersistContext& persist );

	// Load from a fuel address
	bool			Load				( FastFuelHandle fuel );



	// --- Alter

	// Apply all alterations
	void			DoAlterations		( Go* target, Go* source, Go* item );
	// Apply single alteration returns false if alteration caused expiration
	bool			DoAlteration		( void );

	// helpers for skill alterations
	bool			AlterSkillLevel( Skill *pSkill, bool bMultiply, float level_delta );
	bool			TransferSkillLevel( Skill *pSkillFrom, Skill *pSkillTo, float transfer_amount );

	// Undo previously applied alteration
	void			UndoAlteration		( void );

	void			SetEnchantmentDoneMessage( Goid send_to, eWorldEvent event );


	// --- Access

	eAlteration		GetAlterationType		( void ) const			{ return m_Alteration; }
	const gpstring&	GetName					( void ) const			{ return m_sName; }

	const gpstring&	GetEffectScript			( void ) const			{ return m_sEffectScript; }
	const gpstring& GetEffectScriptEquip	( void ) const			{ return m_sEffectScriptEquip; }

	const gpstring& GetEffectScriptHit		( void ) const			{ return m_sEffectScriptHit; }
	const gpstring& GetEffectScriptHitParams( void ) const			{ return m_sEffectScriptHitParams; }

	SFxSID			GetEffectScriptID		( void ) const			{ return m_EffectScriptID; }
	void			SetEffectScriptID		( SFxSID id )			{ m_EffectScriptID = id; }

	SFxSID			GetEffectScriptEquipID	( void ) const			{ return m_EffectScriptEquipID; }
	void			SetEffectScriptEquipID	( SFxSID id )			{ m_EffectScriptEquipID = id; }

	SFxSID			GetAbortScriptID		( void ) const			{ return m_AbortScriptID; }
	void			SetAbortScriptID		( SFxSID id )			{ m_AbortScriptID = id; }

	float			GetValue				( void ) const			{ return m_Value; }
	gpstring		GetValueString			( void ) const			{ return m_sValue; }
	bool			SetValue				( const gpstring &sVal );
	void			SetValue				( float value )			{ m_Value = value; }

	float			GetMaxValue				( void ) const			{ return m_MaxValue; }
	gpstring		GetMaxValueString		( void ) const			{ return m_sMaxValue; }
	void			SetMaxValue				( float value )			{ m_MaxValue = value; }

	float			GetTransferEfficiency		( void )			{ return m_TransferEfficiency; }
	gpstring		GetTransferEfficiencyString	( void ) const		{ return m_sTransferEfficiency; }
	void			SetTransferEfficiency		( float value )		{ m_TransferEfficiency = value; }

	float			GetDuration			( void ) const				{ return m_Duration; }
	gpstring		GetDurationString	( void ) const				{ return m_sDuration; }
	void			SetDuration			( float value )				{ m_Duration = value; }

	float			GetElapsed			( void ) const				{ return m_Elapsed; }

	float			GetFrequency			( void ) const			{ return m_Frequency; }
	gpstring		GetFrequencyString		( void ) const			{ return m_sFrequency; }
	void			SetFrequency			( float value )			{ m_Frequency = value; }

	float			GetInitialDelay			( void ) const			{ return m_InitialDelay; }
	gpstring		GetInitialDelayString	( void ) const			{ return m_sInitialDelay; }
	const gpstring&	GetDescription			( void ) const			{ return m_sDescription; }

	bool			IsEnhancement		( void ) const				{ return m_bEnhancement; }
	bool			IsValueLimited		( void ) const				{ return m_bIsValueLimited; }
	bool			IsPermanent			( void ) const				{ return m_bPermanent; }
	bool			IsTransmissible		( void ) const				{ return m_bTransmissible; }
	bool			IsSingleInstanceOnly( void ) const				{ return m_bSingleInstanceOnly; }
	bool			IsSourceTransfer	( void ) const				{ return m_bIsSourceTransfer; }
	bool			IsTargetTransfer	( void ) const				{ return m_bIsTargetTransfer; }
	bool			IsOffensiveTransfer	( void ) const				{ return m_bIsOffensiveTransfer; }
	bool			IsUITimerVisible	( void ) const				{ return m_bUITimerVisible; }
	bool			IsMultiplicative	( void ) const				{ return m_bMultiplyValue; }
	bool			IsInnateEnchantment	( void ) const				{ return m_bInnateEnchantment; }

	void			MarkForDeletion		( void )					{ m_bMarkedForDeletion = true; }
	bool			IsMarkedForDeletion	( void ) const				{ return m_bMarkedForDeletion; }

	void			SignalModifiersDirty( void ) const;

	Goid			GetTargetGoid		( void ) const				{ return m_Target; }
	Goid			GetSourceGoid		( void ) const				{ return m_Source; }
	Goid			GetItemGoid			( void ) const				{ return m_Item; }
	void			SetItemGoid			( Goid item )				{ m_Item = item; }

	// --- Update - returns false if expired
	bool			Update				( float deltaTime );


private:

	float			GetUndoValue		( void ) const				{ return m_UndoValue; }
	void			SetUndoValue		( float value )				{ m_UndoValue = value; }

	eAlteration		m_Alteration;						// What this enchantment alters
	gpstring		m_sName;							// Name of this enchantment (optional)
	gpstring		m_sEffectScript;					// Name of a Flamethrower script to execute when alteration is made
	gpstring		m_sEffectScriptEquip;				// Name of an effect script to run when the item is equipped
	gpstring		m_sEffectScriptHit;					// Name of an effect script tp run when the weapon/projectile successfully hits something
	gpstring		m_sEffectScriptHitParams;			// Params to go along with the effect script hit script

	SFxSID			m_EffectScriptID;
	SFxSID			m_EffectScriptEquipID;
	SFxSID			m_AbortScriptID;					// ID of Flamethrower script to end when effect aborts/ends

	// These values are evaluated right before the enchantment is actually used
	// so it can pay attention to any stat influences
	gpstring		m_sValue;							// Context sensitive value - like a scalar or a constant
	gpstring		m_sMaxValue;						// The maximum value that value can be in a given context
	gpstring		m_sDuration;						// Enchantment duration in seconds
	gpstring		m_sFrequency;						// How long between each alteration
	gpstring		m_sTransferEfficiency;				// What percent of value per iteration gets transferred.
	gpstring		m_sInitialDelay;					// Initial delay before the alteration is made

	// These are the numeric values that are derived from the expressions stored
	// in the gas file
	float			m_Value;
	float			m_MaxValue;
	bool			m_bMultiplyValue;					// Multiplys value instead of adding when the time comes
	float			m_Duration;
	float			m_Frequency;
	float			m_TransferEfficiency;
	bool			m_bActiveOnParty;					// enchantment is active on all party members.
	bool			m_bInnateEnchantment;

	float			m_InitialDelay;
	gpstring		m_sDescription;						// Unique description - "poisoned", "badly poisoned", "gangrenous", etc...

	bool			m_bEnhancement;						// Is this an enhancement or and affliction?
	bool			m_bPermanent;						// Are the effects of this enchantment permanent?
	bool			m_bTransmissible;					// Can this enchantment spread to another game object?
	bool			m_bSingleInstanceOnly;				// Dissallow accumulation of same alteration?
	bool			m_bIsValueLimited;					// For fractional potion support - subtracts amount used from m_Value
	bool			m_bIsTargetTransfer;				// Transfer direction - target is transferring
	bool			m_bIsSourceTransfer;				// Transfer direction - source is transferring
	bool			m_bIsOffensiveTransfer;				// If offensive will keep draining even if there is no supply
	bool			m_bUITimerVisible;
	bool			m_bAbortOnMin;						// Ends enchantment when value being modified reaches zero or less.
	bool			m_bAbortOnMax;						// Ends enchantment when value being modified gets maxed out

	bool			m_bMarkedForDeletion;				// fix for #12371

	gpstring		m_sDependName;						// If named enchantment isn't active then this enchantment 
	gpstring		m_sDependObject;					// Check "target" or "source" enchantment storage for enchantment m_sDependName

	gpstring		m_sMonsterType;						// specific monster template this enchantment is active against.

	gpstring		m_sCustomDamage;					// Type of custom damage
	gpstring		m_sCustomDamageSkrit;				// Name of the skrit to call when custom damage is applied

	Goid			m_DoneWorldMsgSendTo;				// Minimum worldmessage information for sending out when an enchantment finishes
	eWorldEvent		m_DoneWorldMsgEvent;

	float			m_UndoValue;
	float			m_Elapsed;
	float			m_Interval;

	Goid			m_Target;
	Goid			m_Source;
	Goid			m_Item;

	// For resetting when transmitting to another game object
	float			m_InitialDelay_reset;

};




enum eAlteration
{
	SET_BEGIN_ENUM( ALTER_, 0 ),
#	define   ENCHANTMENT_IS_ENUM 1
#	include "Enchantment.inc"
	SET_END_ENUM( ALTER_ ),
};



FUBI_EXPORT const char* ToString  ( eAlteration e );
FUBI_EXPORT bool        FromString( const char* str, eAlteration& e );



#endif
