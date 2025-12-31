//////////////////////////////////////////////////////////////////////////////
//
// File     :  PContentDb.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the databases that manages and constructs
//             parameterized content for the Game Objects system.
//
//             This code is the very definition of the Heisenberg Uncertainty
//             Principle. I don't know how any of this works, and neither
//             should you.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __PCONTENTDB_H
#define __PCONTENTDB_H

//////////////////////////////////////////////////////////////////////////////

#include "GoDefs.h"

class GpConsole_command;
class PContentReq;
struct ModifierFilter;

ReportSys::Context& GetPContentContext( void );
ReportSys::Context& GetPContentErrorContext( void );

#define gppcontent( msg ) DEV_MACRO_REPORT( &GetPContentContext(), msg )
#define gppcontentf( msg ) DEV_MACRO_REPORTF( &GetPContentContext(), msg )

#define gppcontenterror( msg ) DEV_MACRO_REPORT( &GetPContentErrorContext(), msg )
#define gppcontenterrorf( msg ) DEV_MACRO_REPORTF( &GetPContentErrorContext(), msg )

//////////////////////////////////////////////////////////////////////////////
// class PContentDb declaration

// note: pcontent generation works in four phases
//
// 1. process text query into a PContentReq
// 2. process PContentReq into a specific template
// 3. create the game object by the template
// 4. apply PContentReq to the game object

class PContentDb : public Singleton <PContentDb>
{
public:
	SET_NO_INHERITED( PContentDb );

	enum  {  MAX_MODIFIERS = 2  };

	enum eDbType
	{
		DB_NORMAL = 1 << 0,
		DB_RARE   = 1 << 1,
		DB_UNIQUE = 1 << 2,
	};

	static gpstring ToCommaString  ( eDbType e );
	static bool     FromCommaString( const char* str, eDbType& e );

	static bool IsCompatible( eDbType l, eDbType r )
	{
		// special case - where all bits not set, it's considered normal-only
		if ( l == 0 )
		{
			l = DB_NORMAL;
		}
		if ( r == 0 )
		{
			r = DB_NORMAL;
		}

		// if any overlap then it's a match
		return ( (l & r) != 0 );
	}

	struct ItemEntry
	{
		const GoDataTemplate* m_Template;			// template for this item
		float                 m_ModifierMin;		// min modifier level this item can accept
		float                 m_ModifierMax;		// max modifier level this item can accept
		int                   m_ModifierCountMin;	// minimum modifiers this item must have to exist
		int                   m_ModifierCountMax;	// maximum modifiers this item must have to exist
		GoString              m_Variation;			// pcontent variation, empty if base
		eDbType               m_DbType;				// which db's is this in?

		ItemEntry( const GoDataTemplate* godt = NULL )
		{
			m_Template         = godt;
			m_ModifierMin      = 0;
			m_ModifierMax      = FLOAT_MAX;
			m_ModifierCountMin = 0;
			m_ModifierCountMax = INT_MAX;
			m_DbType           = DB_NORMAL;
		}
	};

	typedef std::multimap <float, ItemEntry>                        PowerTemplateDb;
	typedef std::map      <gpstring, PowerTemplateDb, istring_less> TraitTemplateDb;

	class ModifierEntry
	{
	public:
		typedef stdx::linear_set <gpstring, istring_less> StringSet;
		typedef stdx::fast_vector <ePContentType> PContentTypeColl;
		typedef stdx::fast_vector <eAttackClass> AttackClassColl;
		typedef std::map <gpwstring,gpwstring,istring_less> TagToScreenNameMap;

	private:
		friend PContentDb;

		enum eType
		{
			TYPE_PREFIX,
			TYPE_SUFFIX,
		};

		ModifierEntry*       m_Base;				// specializing off this modifier
		FastFuelHandle		 m_Fuel;				// fuel where this spec is found
		eType                m_Type;				// where does modifier go?
		float                m_Power;				// what's the base power rating of this
		GoString             m_Name;				// name of modifier
		GowString            m_ScreenName;			// screen name to use when constructing object screen name / in localized builds, this is the default string to use if no gender tags are present
		TagToScreenNameMap	 m_TaggedScreenNames;	// screen names to use in localized builds that have a specific gender tag
		GoString             m_EquipRequirements;	// additional equip requirements for the modifier
		int                  m_GoldValue;			// value of this modifier when used to calc object value
		float                m_ToolTipWeight;		// weight of tooltip (used for relative comparisons)
		GoString             m_ToolTipColorName;	// name of color to use for tooltip (empty if none)
		RpEnchantmentStorage m_Enchantments;		// actual enchantments contained in the modifier
		PContentTypeColl     m_PContentTypeColl;	// pcontent types that can use this modifier (if empty defaults to all)
		AttackClassColl      m_AttackClassColl;		// attack classes that can use this modifier (if empty defaults to all)
		StringSet            m_Exclusions;			// modifiers and templates where we cannot use this modifier
		eDbType              m_DbType;				// which db's can this modifier be applied to?

	public:
		ModifierEntry( FastFuelHandle fuel )
		{
			m_Base          = NULL;
			m_Fuel          = fuel;
			m_Type          = TYPE_PREFIX;
			m_Name          = m_Fuel.GetName();
			m_Power         = 0;
			m_GoldValue     = 0;
			m_ToolTipWeight = 0;
			m_DbType        = DB_NORMAL;
		}

		ModifierEntry*          GetBase                     ( void ) const		{  return ( m_Base );  }
		float                   GetPower                    ( void ) const		{  return ( m_Power );  }
		const gpstring&         GetName                     ( void ) const		{  return ( m_Name );  }
		const gpwstring&        GetScreenName               ( void ) const		{  return ( m_ScreenName );  }
		const gpstring&         GetEquipRequirements        ( void ) const		{  return ( m_EquipRequirements );  }
		int                     GetGoldValue                ( void ) const		{  return ( m_GoldValue );  }
		float                   GetToolTipWeight            ( void ) const		{  return ( m_ToolTipWeight );  }
		DWORD                   GetToolTipColor             ( void ) const;
		const PContentTypeColl& GetIncludedPContentTypes    ( void ) const		{  return ( m_PContentTypeColl );  }
		const AttackClassColl&  GetIncludedAttackClasses    ( void ) const		{  return ( m_AttackClassColl );  }
		const StringSet&        GetExclusions               ( void ) const		{  return ( m_Exclusions );  }
		eDbType                 GetDbType                   ( void ) const		{  return ( m_DbType );  }
		int                     GetIndex                    ( void ) const		{  return ( (m_Type == TYPE_PREFIX) ? 0 : 1 );  }
		const char*             GetIndexName                ( void ) const		{  return ( (m_Type == TYPE_PREFIX) ? "prefix" : "suffix" );  }
		bool                    Init                        ( void );
		bool                    CanCoexistWith              ( const ModifierEntry* other ) const;
		bool                    CanModify                   ( const GoDataTemplate* godt, ePContentType pt, eAttackClass ac ) const;
	};

	struct Range
	{
		float m_Mid, m_Min, m_Max;

		Range( void )										{  m_Min = 0;  m_Mid = FLOAT_MAX;  m_Max = FLOAT_MAX;  }

		Range& operator = ( float f )						{  m_Mid = m_Min = m_Max = f;  return ( *this );  }
		Range& operator = ( const Range& r )				{  m_Mid = r.m_Mid;  m_Min = r.m_Min;  m_Max = r.m_Max;  return ( *this );  }

		Range& operator -= ( float f );

		operator float ( void ) const						{  return ( m_Mid );  }

		gpstring ToString( void ) const;
	};

	friend PContentReq;

	struct ItemTraits
	{
		ePContentType m_SpecificType;
		float         m_ItemPower;
		gpstring      m_ItemSubType;
		gpstring      m_ItemLength;
		eAttackClass  m_AttackClass;
		eDbType       m_DbType;
		bool          m_IsMelee     : 1;
		bool          m_IsRanged    : 1;
		bool          m_IsTwoHanded : 1;
		bool          m_NoModifiers : 1;

		ItemTraits( void )
		{
			m_SpecificType = PT_INVALID;
			m_ItemPower    = 0;
			m_AttackClass  = AC_BEASTFU;
			m_DbType       = DB_NORMAL;
			m_IsMelee      = false;
			m_IsRanged     = false;
			m_IsTwoHanded  = false;
			m_NoModifiers  = false;
		}
	};

// Setup.

	// ctor/dtor
	PContentDb( void );
   ~PContentDb( void );

	// initialization/reload (auto-shutdown)
	bool Init( void );

	// got data?
	bool IsInitialized( void );

	// sync work
	UINT32 AddCrc32( UINT32 seed = 0 ) const;

	// free all resources
	void Shutdown( void );

// Query.

	// skrit-based formulas
	ePContentType CalcGeneralType           ( void );
	float         CalcItemPower             ( ePContentType type, ePContentType specificType, float minPower, float maxPower, bool isTwoHanded = false, const char* subType = "", const char* length = "", const char* material = "" );
	float         CalcItemPower             ( ePContentType type, float minPower, float maxPower );
	int           CalcModifierCount         ( ePContentType type, float totalPower );
	float         CalcModifierPower         ( ePContentType type, float totalPower, int modifierCount, eDbType dbType );
	float         CalcModifierBiasedPower   ( ePContentType type, ePContentType specificType, float modifierPower, const char* subType, const char* material );
	int           CalcGoldValue             ( ePContentType type, ePContentType specificType, float minPower, float maxPower, float itemPower, float modifierPower0, float modifierPower1, bool isTwoHanded, const char* subType = "", const char* length = "", const char* material = "" );
	float         CalcItemPowerFuzziness    ( ePContentType type, float itemPower, eDbType dbType );
	float         CalcModifierPowerFuzziness( ePContentType type, float modifierPower, eDbType dbType );
	ePContentType CalcArmorCoverage         ( void );

	// helper functions for automation - these might be very slow, careful!
	// DO NOT CALL FROM PRODUCTION CODE
#	if !GP_RETAIL
	const ItemEntry* FindItemEntry   ( const char* templateName, const char* variationName ) const;
FEX	float            GetModifierPower( const char* modifierName ) const;
FEX	float            GetModifierMin  ( const char* templateName, const char* variationName ) const;
FEX	float            GetModifierMax  ( const char* templateName, const char* variationName ) const;
#	endif // !GP_RETAIL

	// classification
	bool QueryItemTraits(
			ItemTraits&				itemTraits,									// dest traits go here
			const GoDataTemplate*	godt,										// which one to query
			ePContentType			expectedType				= PT_INVALID,	// what we're expecting (needed for db tree init, leave default for general query)
			bool					ignorePContentAllowedFlag	= false,		// ignore the "is_pcontent_allowed" flag
			bool					keepRef						= false,		// keep the ref we add after finished with query
			bool					specialGetTypeOnly			= false );		// we're only after the pcontent type and attack class

	// db lookup
	const gpstring*      FindMacro   ( const char* name ) const;
	const ModifierEntry* FindModifier( const char* name ) const;

	// utility
	static bool GetArmorTraitsFromTemplateName     ( gpstring& material, gpstring& skill, const char* name );
	static bool GetProjectileTraitsFromTemplateName( gpstring& length, const char* name );
	static void GetWeaponTraits                    ( gpstring& subType, gpstring& length, eAttackClass attackClass, const char* name );

// Creation query.

	// parses a string query into a pcontent request
	bool ParseQuery( PContentReq& req, const char* query, bool processMacros = true );

	// returns the template it chooses (from the properly set up request), and sets context data in the request
	bool ProcessRequest( PContentReq& req );

	// all-in-one parse + find
	const GoDataTemplate* FindTemplateByPContent( PContentReq& req, const char* query, bool processMacros = true );

	// stores the templates it finds into coll
	int FindTemplatesByTraits( TemplateSet& coll, const char* traits, float minPower, float maxPower ) const;

// Creation postprocessing.

	// applies pcontent request to the fresh go
	bool ApplyPContentRequest( Go* go, const PContentReq& req );

	// calc's and applies pcontent rules to the fresh non-pcontent go (but using pcontent rules)
	bool CalcAndApplyPContentRules( Go* go );

#	if !GP_RETAIL
	void Dump     ( ReportSys::ContextRef ctx = NULL ) const;
	void DumpStats( ReportSys::ContextRef ctx = NULL ) const;
#	endif // !GP_RETAIL

// Inventory-pcontent query.

	// inventory multiplayer multipliers
	float GetInvMpAllChanceMultiplier  ( Go* forGo ) const;
	float GetInvMpOneOfChanceMultiplier( Go* forGo ) const;
	void  GetInvMpGoldMinMaxMultipliers( Go* forGo, float& minMult, float& maxMult ) const;
	bool  GetInvMpMinMaxMultipliers    ( Go* forGo, ePContentType pt, eAttackClass ac, float& minMult, float& maxMult ) const;

// Utility

	static void PlaceObjectsInArray( const SiegePos& origin, const GoidColl& goids, float distance = 0.0f );
	static void PlaceObjectsInSpew ( const SiegePos& origin, const GoidColl& goids );

private:

	const ModifierEntry* ChooseModifier   ( bool prefix, float modifierPower, const PContentReq& req, const ModifierEntry* other = NULL );
	const gpstring&      ChooseRandomTrait( float reqPower, ePContentType specificType ) const;
	Skrit::Object*       GetFormulas      ( void )  {  return ( ::IsPrimaryThread() ? m_MainFormulas : m_LoaderFormulas );  }

	struct GeneralTrait
	{
		GoString m_TraitName;			// name of trait - ring, sword, etc.
		float    m_RandomOffset;		// offset in 0.0 to 1.0 continuum where randomly choose this one
	};

	typedef stdx::fast_vector <float> FloatColl;

	struct MinMax
	{
		ePContentType m_PContentType;			// general pcontent types and armor
		eAttackClass  m_AttackClass;			// weapons
		FloatColl     m_MinCountMultiplier;		// multiplier to apply to min count in any block
		FloatColl     m_MaxCountMultiplier;		// multiplier to apply to max count in any block

		MinMax( void )
		{
			clear();
		}

		void clear( void )
		{
			m_PContentType = PT_INVALID;
			m_AttackClass  = AC_BEASTFU;
			m_MinCountMultiplier.clear();
			m_MaxCountMultiplier.clear();
		}
	};

	typedef stdx::fast_vector <MinMax> MinMaxColl;

	struct MpLootScaling
	{
		FloatColl  m_AllChanceMultiplier;		// multiplier to apply to the chance on the contents of [all*] blocks
		FloatColl  m_OneOfChanceMultiplier;		// multiplier to apply to the chance on the contents of [oneof*] blocks
		MinMax     m_MinMaxGold;				// minmax changes for gold only
		MinMaxColl m_MinMaxColl;				// multipliers to apply after choosing a block and trying to decide count

		MpLootScaling( void )
		{
			clear();
		}

		void clear( void )
		{
			m_AllChanceMultiplier.clear();
			m_OneOfChanceMultiplier.clear();
			m_MinMaxGold.clear();
			m_MinMaxColl.clear();
		}
	};

	typedef std::map          <gpstring, gpstring, istring_less>        StringDb;
	typedef std::map          <gpstring, ModifierEntry,   istring_less> ModifierStorageDb;
	typedef std::multimap     <float,    ModifierStorageDb::iterator>   ModifierPowerDb;
	typedef stdx::fast_vector <GoString>                                StringColl;
	typedef stdx::linear_set  <GoString, istring_less>                  StringSet;

	StringDb          m_MacroDb;					// db of template query macros
	TraitTemplateDb   m_TraitTemplateDb;			// db of templates by trait
	ModifierStorageDb m_ModifierStorageDb;			// db of modifiers that can be applied to fresh pcontent
	ModifierPowerDb   m_ModifierPrefixDb;			// db of prefix modifiers by their power ratings
	ModifierPowerDb   m_ModifierSuffixDb;			// db of suffix modifiers by their power ratings
	float             m_MaxModifierPower;			// cache of highest available modifier
	Skrit::Object*    m_MainFormulas;				// skrit formula object for calculating pcontent-related values
	Skrit::Object*    m_LoaderFormulas;				// same as above but for loader thread
	StringColl        m_MeleeTypesNoStaff;			// all melee types minus staff
	StringColl        m_RangedTypes;				// all ranged types
	StringSet         m_InfiniteMeleeWeapons;		// infinite melee weapons
	StringSet         m_InfiniteRangedWeapons;		// infinite ranged weapons
	MpLootScaling     m_MpLootScaling;				// spec for multiplayer-only loot scaling
	bool			  m_bPrefixFirst;				// loc feature for name composition ordering

	// commands
#	if !GP_RETAIL
	GpConsole_command* m_GpcAdd;
	GpConsole_command* m_GpcGive;
#	endif // !GP_RETAIL

	// statistics
#	if !GP_RETAIL
	bool   m_Initializing;
	int    m_CallDepth;
	int    m_NoPContentAllowedItems;
	int    m_StatRequests;
	int    m_StatRequestIterations;
	int    m_StatLowFinds;
	int    m_StatHighFinds;
	int    m_StatHighSubstitutions;
	int    m_StatModifiersTooLow;
	int    m_StatModifiersTooHigh;
	int    m_StatOverflowIntoMods;
	int    m_StatModifiersNotFound;
	double m_StatTotalTime;
#	endif // !GP_RETAIL

	FUBI_SINGLETON_CLASS( PContentDb, "Parameterized Content Database for generated content." );
	SET_NO_COPYING( PContentDb );
};

#define gPContentDb PContentDb::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class PContentReq declaration

class PContentReq
{
	friend PContentDb;
	friend ModifierFilter;

	typedef PContentDb::TraitTemplateDb::const_iterator TraitTemplateDbCIter;
	typedef PContentDb::Range Range;
	typedef PContentDb::ModifierEntry ModifierEntry;

	// request
#	if !GP_RETAIL
	gpstring               m_QueryText;
#	endif // !GP_RETAIL
	ePContentType          m_SpecificType;
	ePContentType          m_BasicType;
	const GoDataTemplate*  m_Template;
	TraitTemplateDbCIter   m_TemplateDbIter;
	Range                  m_TotalPower;
	int                    m_ReqModifierCount;
	DWORD                  m_RandomSeed;

	// tuning
	gpstring               m_ArmorMaterial;
	gpstring               m_ArmorSkillType;
	gpstring               m_Finish;

	// modifiers
	const ModifierEntry*   m_Modifiers[ PContentDb::MAX_MODIFIERS ];

	// options
	PContentDb::eDbType    m_DbType;
	bool                   m_NoOrnate    : 1;
	bool                   m_ForceSeed   : 1;

	// result context
	Range                  m_ItemPower;
	int                    m_AttemptCount;

	int  GetModifierCount( void ) const;
	bool HasModifiers    ( void ) const					{  return ( GetModifierCount() != 0 );  }

public:

	PContentReq( void );

	ePContentType   GetBasicType       ( void ) const	{  return ( m_BasicType );  }
	int             GetReqModifierCount( void ) const	{  return ( m_ReqModifierCount );  }
	const gpstring& GetArmorMaterial   ( void ) const	{  return ( m_ArmorMaterial  );  }
	const gpstring& GetArmorSkillType  ( void ) const	{  return ( m_ArmorSkillType );  }
	const gpstring& GetFinish          ( void ) const	{  return ( m_Finish );  }

	bool  IsForcingRandomSeed( void ) const				{  return ( m_ForceSeed );  }
	DWORD GetForcedRandomSeed( void ) const				{  return ( m_RandomSeed );  }

	void SetFinish  ( const gpstring& finish )			{  m_Finish = finish;  }
	void SetModifier( int index, const ModifierEntry* modifier );

	PContentDb::eDbType GetDbType( void ) const			{  return ( m_DbType );  }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __PCONTENTDB_H

//////////////////////////////////////////////////////////////////////////////
