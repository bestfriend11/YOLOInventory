//////////////////////////////////////////////////////////////////////////////
//
// File     :  ContentDb.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains the database that manages the data for the Game
//             Object system.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __CONTENTDB_H
#define __CONTENTDB_H

//////////////////////////////////////////////////////////////////////////////

#include "BlindCallback.h"
#include "BoneTranslator.h"
#include "Choreographer.h"
#include "Enchantment.h"
#include "Fuel.h"
#include "GoDefs.h"
#include "GoSupport.h"
#include "SkritDefs.h"

#include <map>
#include <set>
#include <list>

//////////////////////////////////////////////////////////////////////////////
// internal type declarations

struct GoGoldRange
{
	int      m_MinGold;
	GoString m_Model;
	GoString m_Texture;

	bool Load( const gpstring& key, const gpstring& value );

	bool operator < ( const GoGoldRange& other ) const
		{  return ( m_MinGold < other.m_MinGold );  }
};

struct GoGoldRangeColl : std::vector <GoGoldRange>
{
	bool Load( FastFuelHandle fuel );
};

struct GoPotionRange
{
	float    m_MaxRatio;
	GoString m_Texture;

	bool Load( const gpstring& key, const gpstring& value );

	bool operator < ( const GoPotionRange& other ) const
		{  return ( m_MaxRatio < other.m_MaxRatio );  }
};

struct GoPotionRangeColl : std::vector <GoPotionRange>
{
	bool Load( FastFuelHandle fuel );
};

struct GoInvRange
{
	float    m_MaxRatio;
	GoString m_Model;
	GoString m_Texture;

	bool Load( const gpstring& key, const gpstring& value );

	bool operator < ( const GoInvRange& other ) const
		{  return ( m_MaxRatio < other.m_MaxRatio );  }
};

struct GoInvRangeColl : std::vector <GoInvRange>
{
	bool Load( FastFuelHandle fuel );
};

//////////////////////////////////////////////////////////////////////////////
// class ContentDb declaration

// $ note that go's are not ref-counted, though the option is left open for
//   that if needed later (for debugging etc).

class ContentDb : public Singleton <ContentDb>
{
public:
	SET_NO_INHERITED( ContentDb );

// Types.

	// restrict who can create a go
	struct GoCreateReqRestrict
	{
	private:
		const GoCreateReq& m_CreateReq;

		GoCreateReqRestrict( const GoCreateReq& createReq ) : m_CreateReq( createReq )  {  }

		const GoCreateReq* operator -> ( void ) const  {  return ( &m_CreateReq );  }

		// this is the list of possible classes that may create go's
		friend ContentDb;
		friend GoDb;
	};

	typedef std::list <GoDataTemplate*> DataTemplateList;
	typedef std::set <gpstring, string_less> StringDb;
	typedef std::set <gpstring, istring_less> IStringDb;
	typedef std::set <gpwstring, string_less> WStringDb;
	typedef DataTemplateList::const_iterator DataTemplateIter;

	struct ExpireEntry
	{
		float m_DelaySeconds;		// time to expire in seconds
		float m_RangeSeconds;		// +/- modifier for delay

		ExpireEntry( void )
		{
			m_DelaySeconds   = 0;
			m_RangeSeconds   = 0;
		}
	};

	enum eCreateType
	{
		CREATE_NONE,				// do not create
		CREATE_GLOBAL,				// create a global go if server
		CREATE_LOCAL_PLAIN,			// create a non-lodfi local go always
		CREATE_LOCAL_LODFI,			// create a lodfi local go always
	};

	struct SyncSpec
	{
		SyncSpec( void )
		{
			::ZeroObject( *this );
		}

		DWORD m_ContentDbLqdCrc;	// crc of contentdb lqd
		DWORD m_ContentDbSkritCrc;	// crc of contentdb skrit components
		DWORD m_FormulasSkritCrc;	// crc of formulas.skrit
		DWORD m_PContentSkritCrc;	// crc of pcontent.skrit
	};

	static const char* SYNC_DIRS[];	// array of folder names that are involved in sync checking

// Setup.

	// ctor/dtor
	ContentDb( void );
   ~ContentDb( void );

	// initialization/reload (auto-shutdown)
	bool Init( void );
	void HandleBroadcast( WorldMessage & msg );

	// spec building
	SyncSpec MakeSynchronizeSpec( void ) const							{  return ( m_SyncSpec );  }

	// got data?
	bool IsInitialized( void );
#	if !GP_RETAIL
	bool IsJitCompile( void ) const										{  return ( m_JitCompile );  }
#	endif // !GP_RETAIL

	// free all resources
	void Shutdown( void );

	// add entry to string collectors
	const gpstring&  AddString( const gpstring& str )					{  return ( *m_StringCollector .insert( str ).first );  }
	const gpwstring& AddString( const gpwstring& str )					{  return ( *m_WStringCollector.insert( str ).first );  }

	// factory manipulation
	void RegisterGoComponentFactory  ( const gpstring& name, GocFactoryCb cb, bool serverOnly = false, bool canServerExistOnly = false );
	void UnregisterGoComponentFactory( const gpstring& name );

// Content construction.

	// query to see if this scid can be created given world state constraints
	eCreateType GetCreateTypeForScid( Scid scid, RegionId regionId ) const;

	// create a go, returns null on failure - note the restricted access!
	Go* CreateGo( const GoCreateReqRestrict& createReq ) const;

	// create a component given the prototype and specialization
	GoComponent* CreateGoComponent( Go* parent, const GoDataComponent& godc, FastFuelHandle gocOverride, bool& serverComponent, bool autoXfer = true, bool shouldPersist = true ) const;

	// create a raw component - don't call this unless you know what you're doing!
	GoComponent* CreateRawGoComponent( Go* parent, const gpstring& type, bool& serverComponent ) const;

	// validation/reporting
#	if !GP_RETAIL
	int  GetStringCollectedCount  ( void ) const;
	int  GetStringAllocsSavedCount( void ) const;
	int  GetStringMemSavedBytes   ( void ) const;
	void DumpStatistics           ( ReportSys::ContextRef ctx ) const;
FEX	void DumpStatistics           ( void ) const						{  DumpStatistics( NULL );  }
#	endif // !GP_RETAIL

// Query.

	// database query
	const GoDataComponent* FindComponentByName       ( const gpstring& name, bool* wasInternal = NULL ) const;
	FuBi::eVarType         FindFuBiTypeByName        ( const char* name ) const;
	const GoDataTemplate*  FindTemplateByName        ( const char* name ) const;
	int                    FindTemplatesByWildcard   ( TemplateColl& coll, const char* pattern, bool leavesOnly = true ) const;
#	if !GP_RETAIL
	int                    FindTemplatesByDocWildcard( TemplateColl& coll, const char* pattern, bool leavesOnly = true ) const;
#	endif // !GP_RETAIL
	int                    FindTemplatesAndChildren  ( TemplateColl& coll, const char* rootName, bool leavesOnly = true ) const;
	gpstring               FindMaterialSound         ( const char* event, gpstring& pri, const char* srcMaterial = NULL, const char* dstMaterial = NULL, bool recurse = true );
	gpstring               FindGlobalVoiceSound      ( const char* event, gpstring& pri );
	const ExpireEntry*     FindExpirationClass       ( const char* name ) const;

	// ref-counted template access
	const GoDataTemplate* AcquireTemplateByName( const char* name );
	int                   AddRefTemplate       ( const GoDataTemplate* godt );
	int                   ReleaseTemplate      ( const GoDataTemplate* godt );

	// dev-only database query
#	if !GP_RETAIL
	DataTemplateIter GetDataTemplateRootsBegin( void ) const			{  return ( m_DataTemplateRoots.begin() );  }
	DataTemplateIter GetDataTemplateRootsEnd  ( void ) const			{  return ( m_DataTemplateRoots.end  () );  }
	void             GetAllMetaGroupNames     ( StringDb& db ) const	{  db = m_MetaGroupNames;  }
#	endif // !GP_RETAIL

// Other databases.

	// caches
	RpBoneTranslator     AddOrFindBoneTranslator    ( FastFuelHandle fuel, nema::HAspect aspect );
	RpBoneTranslator     AddOrFindNullBoneTranslator( void );
	RpChoreDictionary    AddOrFindChoreDictionary   ( FastFuelHandle fuel );
	RpEnchantmentStorage AddOrFindEnchantmentStorage( FastFuelHandle fuel, const EnchantmentStorage* base = NULL );
	RpTriggerStorage     AddOrFindTriggerStorage    ( FastFuelHandle fuel, bool IsTemplateStorage );
	RpGoGoldRangeColl    AddOrFindGoldRangeColl     ( FastFuelHandle fuel );
	RpGoPotionRangeColl  AddOrFindPotionRangeColl   ( FastFuelHandle fuel );
	RpGoInvRangeColl     AddOrFindInvRangeColl      ( FastFuelHandle fuel );

	// constraints
#	if !GP_RETAIL
	FuBi::ConstraintSpec* CreateConstraint( const char* name ) const;
#	endif // !GP_RETAIL

	// misc query
	int                   GetAllHeroes						( TemplateColl& coll );
	int                   GetAllHeroes						( GoidColl& coll );
	const GoDataTemplate* GetRandomHero						( void );
	const char*           GetRandomHeroName					( void );
	const gpstring&       GetDefaultAspectName				( void ) const  {  return ( m_DefaultAspectName );  }
	const gpstring&       GetDefaultTextureName				( void ) const  {  return ( m_DefaultTextureName );  }
	const gpstring&		  GetDefaultGoldTemplate			( void ) const  {  return ( m_DefaultGoldTemplate );  }
	const gpstring&		  GetDefaultSpellBookTemplate		( void ) const  {  return ( m_DefaultSpellBookTemplate );  }
	const gpstring&		  GetDefaultPackmuleTemplate		( void ) const  {  return ( m_DefaultPackmuleTemplate );  }
	const gpstring&		  GetDefaultMuleTemplate			( void ) const  {  return ( m_DefaultMuleTemplate );  }
	const gpstring&		  GetDefaultTombstoneTemplate		( void ) const  {  return ( m_DefaultTombstoneTemplate );  }
	const gpstring&		  GetDefaultFaderProxyTemplate		( void ) const  {  return ( m_DefaultFaderProxyTemplate );  }
	const gpstring&		  GetDefaultWaypointMovingTemplate	( void ) const  {  return ( m_DefaultWaypointMovingTemplate );  }
	const gpstring&		  GetDefaultPointTemplate			( void ) const  {  return ( m_DefaultPointTemplate );  }
	const gpstring&		  GetDefaultPartyHumanoidTemplate	( void ) const  {  return ( m_DefaultPartyHumanoidTemplate );  }
	const gpstring&		  GetDefaultStashTemplate			( void ) const	{  return ( m_DefaultStashTemplate ); }
FEX	int					  GetMaxPartyGold					( void ) const	{  return ( m_MaxPartyGold ); }

	// armor query
	const gpstring& GetBootSubTypeForArmorBodyType	  ( const gpstring& abt ) const;
	const gpstring& GetGauntletSubTypeForArmorBodyType( const gpstring& abt ) const;

	// weapon range speed screen name query
	const gpstring&	GetGuiWeaponSpeedString			  ( const float speed	) const;

// Formulas.

	// $ these formulas support the standard URL form ?key1=val1&key2=val2

	// formula query
FEX float EvalFloatFormula( const char* formula, float defValue );
FEX float EvalFloatFormula( const char* formula )						{  return ( EvalFloatFormula( formula, 0.0f ) );  }
FEX int   EvalIntFormula  ( const char* formula, int defValue );
FEX int   EvalIntFormula  ( const char* formula )						{  return ( EvalIntFormula( formula, 0 ) );  }
FEX bool  EvalBoolFormula ( const char* formula, bool defValue );
FEX bool  EvalBoolFormula ( const char* formula )						{  return ( EvalBoolFormula( formula, false ) );  }

// Template component property access.

FEX int             GetTemplateInt   ( const char* templatePath, int defInt );
FEX int             GetTemplateInt   ( const char* templatePath )
										{  return ( GetTemplateInt( templatePath, 0 ) );  }
FEX bool            GetTemplateBool  ( const char* templatePath, bool defBool );
FEX bool            GetTemplateBool  ( const char* templatePath )
										{  return ( GetTemplateBool( templatePath, false ) );  }
FEX float           GetTemplateFloat ( const char* templatePath, float defFloat );
FEX float           GetTemplateFloat ( const char* templatePath )
										{  return ( GetTemplateFloat( templatePath, 0 ) );  }
FEX const gpstring& GetTemplateString( const char* templatePath, const gpstring& defString );
FEX const gpstring& GetTemplateString( const char* templatePath )
										{  return ( GetTemplateString( templatePath, gpstring::EMPTY ) );  }
FEX Scid            GetTemplateScid  ( const char* templatePath, Scid defScid );
FEX Scid            GetTemplateScid  ( const char* templatePath )
										{  return ( GetTemplateScid( templatePath, SCID_INVALID ) );  }

private:

#	if !GP_RETAIL
	bool AddConstraint( FastFuelHandle fuel );
#	endif // !GP_RETAIL
	bool AddComponent ( FastFuelHandle fuel );
	bool AddComponent ( Skrit::HObject skrit, const gpstring& name );
	bool AddTemplate  ( FastFuelHandle fuel );

	struct FactoryEntry
	{
		GocFactoryCb m_FactoryCb;
		bool         m_ServerOnly;
		bool         m_CanServerExistOnly;

		FactoryEntry( GocFactoryCb cb, bool serverOnly = false, bool canServerExistOnly = false )
			: m_FactoryCb( cb ), m_ServerOnly( serverOnly ), m_CanServerExistOnly( canServerExistOnly )  {  }
	};

#	if !GP_RETAIL
	struct ConstraintEntry
	{
		my FuBi::ConstraintSpec* m_Constraint;
		FastFuelHandle           m_Fuel;

		ConstraintEntry( FuBi::ConstraintSpec* spec = NULL )  {  m_Constraint = spec;  }
		ConstraintEntry( const ConstraintEntry& obj );
	   ~ConstraintEntry( void );

		ConstraintEntry& operator = ( const ConstraintEntry& obj );
	};
#	endif // !GP_RETAIL

	typedef stdx::fast_vector <DWORD> DwordColl;

	struct ComponentEntry
	{
		my GoDataComponent*       m_DataComponent;
		my FuBi::StoreHeaderSpec* m_StoreHeaderSpec;
		DwordColl                 m_DwordColl;		// (flags, optz for #include dependencies)

		ComponentEntry( void )
		{
			m_DataComponent   = NULL;
			m_StoreHeaderSpec = NULL;
		}
	};

	typedef std::map <gpstring, FactoryEntry,                   istring_less> GocFactoryDb;
	typedef std::map <gpstring, ComponentEntry,                 istring_less> DataComponentDb;
	typedef std::map <gpstring, FuBi::eVarType,                 istring_less> FuBiComponentDb;
	typedef std::map <gpstring, my GoDataTemplate*,             istring_less> DataTemplateDb;
	typedef std::map <gpstring, std::pair <gpstring, gpstring>, istring_less> ArmorSubTypeDb;
	typedef std::map <gpstring, ExpireEntry,                    istring_less> ExpireDb;

	struct SpeedRangeNameEntry
	{
		gpstring sScreenName;
		float minRange;
		float maxRange;
	};

	typedef std::vector< SpeedRangeNameEntry > SpeedRangeNameDb;

#	if !GP_RETAIL
	typedef std::map <gpstring, ConstraintEntry, istring_less> ConstraintDb;
#	endif // !GP_RETAIL

	typedef std::map <FastFuelHandle, RpBoneTranslator>     BoneTranslatorDb;
	typedef std::map <FastFuelHandle, RpChoreDictionary>    ChoreDictionaryDb;
	typedef std::map <FastFuelHandle, RpEnchantmentStorage> EnchantmentStorageDb;
	typedef std::map <FastFuelHandle, RpTriggerStorage>     TriggerStorageDb;
	typedef std::map <FastFuelHandle, RpGoGoldRangeColl>    GoGoldRangeCollDb;
	typedef std::map <FastFuelHandle, RpGoPotionRangeColl>  GoPotionRangeCollDb;
	typedef std::map <FastFuelHandle, RpGoInvRangeColl>     GoInvRangeCollDb;

	// owned objects
	PContentDb* m_PContentDb;						// db for parameterized content
	Skrit::Object* m_Formulas;						// skrit formula object for calculating general formulaic values

	// standard databases
	GocFactoryDb     m_GocFactoryDb;				// db of factories to create components
	DataComponentDb  m_DataComponentDb;				// db of base components to build from (note: a NULL component means it's internal)
	FuBiComponentDb  m_FuBiComponentDb;				// db of c++ components by name to fubi type
	DataTemplateDb   m_DataTemplateDb;				// db of available templates for instancing content
	StringDb         m_StringCollector;				// ref-counts strings for memory efficiency
	WStringDb        m_WStringCollector;			// ref-counts wide strings for memory efficiency
	IStringDb        m_ComponentNameDb;				// temporary db of requested component names used at startup to detect unused skrits
	FastFuelHandle   m_SoundDb;						// db of material sound mappings
	FastFuelHandle   m_VoiceDb;						// db of global voice mappings
	ArmorSubTypeDb   m_ArmorSubTypeDb;				// db of boot & gauntlet subtype by body armour type
	ExpireDb         m_ExpireDb;					// db of different expiration types
	SpeedRangeNameDb m_SpeedRangeNameDb;			// db of weapon speed ranges and their respective screen names.

	// collectors
	BoneTranslatorDb     m_BoneTranslatorDb;		// collects bone translators by fuel handle
	RpBoneTranslator     m_NullBoneTranslator;		// just a null bone translator
	ChoreDictionaryDb    m_ChoreDictionaryDb;		// collects chore dictionaries by fuel handle
	EnchantmentStorageDb m_EnchantmentStorageDb;	// collects enchantment storage by fuel handle
	TriggerStorageDb     m_TriggerStorageDb;		// collects trigger storage by fuel handle
	GoGoldRangeCollDb    m_GoGoldRangeCollDb; 		// collects gold range storage by fuel handle
	GoPotionRangeCollDb  m_GoPotionRangeCollDb;		// collects potion range storage by fuel handle
	GoInvRangeCollDb     m_GoInvRangeCollDb;		// collects inventory range storage by fuel handle

	// cache
	gpstring m_DefaultAspectName;
	gpstring m_DefaultTextureName;
	gpstring m_DefaultGoldTemplate;
	gpstring m_DefaultPackmuleTemplate;
	gpstring m_DefaultMuleTemplate;
	gpstring m_DefaultSpellBookTemplate;
	gpstring m_DefaultTombstoneTemplate;
	gpstring m_DefaultFaderProxyTemplate;
	gpstring m_DefaultWaypointMovingTemplate;
	gpstring m_DefaultPointTemplate;
	gpstring m_DefaultPartyHumanoidTemplate;
	gpstring m_DefaultStashTemplate;
	int		 m_MaxPartyGold;
	SyncSpec m_SyncSpec;

	// dev-only databases
#	if !GP_RETAIL
	bool             m_JitCompile;					// jit-compile the contentdb (defaults to false in non-retail)
	DataTemplateList m_DataTemplateRoots;			// list of data template root nodes
	StringDb         m_MetaGroupNames;				// db of all meta groups
	ConstraintDb     m_ConstraintDb;				// db of all available "choose" type constraints
#	endif // !GP_RETAIL

	FUBI_SINGLETON_CLASS( ContentDb, "Content database." );
	SET_NO_COPYING( ContentDb );
};

#define gContentDb ContentDb::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class AutoRefTemplate declaration

class AutoRefTemplate
{
public:
	AutoRefTemplate( const GoDataTemplate* godt )
	{
		m_DataTemplate = godt;
		if ( m_DataTemplate != NULL )
		{
			gContentDb.AddRefTemplate( m_DataTemplate );
		}
	}

	AutoRefTemplate( const char* templateName )
	{
		m_DataTemplate = gContentDb.AcquireTemplateByName( templateName );
	}

   ~AutoRefTemplate( void )
	{
		if ( m_DataTemplate != NULL )
		{
			gContentDb.ReleaseTemplate( m_DataTemplate );
		}
	}

	const GoDataTemplate* GetDataTemplate( void ) const
	{
		return ( m_DataTemplate );
	}

private:
	const GoDataTemplate* m_DataTemplate;
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __CONTENTDB_H

//////////////////////////////////////////////////////////////////////////////
