//////////////////////////////////////////////////////////////////////////////
//
// File     :  ContentDb.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "ContentDb.h"

#include "Config.h"
#include "FileSys.h"
#include "FileSysUtils.h"
#include "FuBi.h"
#include "FuBiPersistImpl.h"
#include "FuBiTraitsImpl.h"
#include "Fuel.h"
#include "FuelDb.h"
#include "MessageDispatch.h"
#include "NamingKey.h"
#include "Nema_AspectMgr.h"
#include "Go.h"
#include "GoCore.h"
#include "GoData.h"
#include "GoDb.h"
#include "GoSupport.h"
#include "PContentDb.h"
#include "ReportSys.h"
#include "Server.h"
#include "Services.h"
#include "SkritEngine.h"
#include "SkritObject.h"
#include "SkritStructure.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "Trigger_Sys.h"
#include "WorldMap.h"

#include <set>

// $ includes required for runtime
#include "GoConversation.h"
#include "GoStore.h"

//////////////////////////////////////////////////////////////////////////////
// class ConstrainTemplates declaration

#if !GP_RETAIL

class ConstrainTemplates : public FuBi::ConstrainStrings
{
public:
	SET_INHERITED( ConstrainTemplates, FuBi::ConstrainStrings );

	virtual FuBi::ConstraintSpec* Clone( void )
		{  return ( new ThisType( *this ) );  }

	virtual int OnGetStrings( StringColl& strings )
	{
		TemplateColl coll;
		int size = gContentDb.FindTemplatesByWildcard( coll, "*" );

		TemplateColl::const_iterator i, ibegin = coll.begin(), iend = coll.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			strings.push_back( Entry( (*i)->GetName(), (*i)->GetDocs() ) );
		}

		return ( size );
	}
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// internal type implementations

bool GoGoldRange :: Load( const gpstring& key, const gpstring& value )
{
	bool success = true;

	if ( !::FromString( key, m_MinGold ) || (m_MinGold < 0) )
	{
		gperrorf(( "Illegal int value '%s'\n", key.c_str() ));
		success = false;
	}

	int count = stringtool::GetNumDelimitedStrings( value, ',' );
	if ( count == 1 )
	{
		m_Model = value;
	}
	else if ( count == 2 )
	{
		gpstring tmp;
		stringtool::GetDelimitedValue( value, ',', 0, tmp );
		m_Model = tmp;
		stringtool::GetDelimitedValue( value, ',', 1, tmp );
		m_Texture = tmp;
	}
	else
	{
		gperrorf(( "Bad format '%s'\n", value.c_str() ));
		success = false;
	}

	return ( success );
}

bool GoGoldRangeColl :: Load( FastFuelHandle fuel )
{
	bool success = true;

	if( fuel )
	{
		FastFuelFindHandle fh( fuel );
		if( fh.FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while( fh.GetNextKeyAndValue( key, value ) )
			{
				GoGoldRange range;
				if ( range.Load( key, value ) )
				{
					push_back( range );
				}
				else
				{
					gperrorf(( "Unable to add gold range '%s = '%s' at '%s\n",
							   key.c_str(),
							   value.c_str(),
							   fuel.GetAddress().c_str() ));
					success = false;
				}
			}
			stdx::sort( *this );
		}
	}

	return ( success );
}

bool GoPotionRange :: Load( const gpstring& key, const gpstring& value )
{
	bool success = true;

	if ( !::FromString( key, m_MaxRatio ) || (m_MaxRatio < 0) || (m_MaxRatio > 1) )
	{
		gperrorf(( "Illegal float value '%s'\n", key.c_str() ));
		success = false;
	}

	m_Texture = value;

	return ( success );
}

bool GoPotionRangeColl :: Load( FastFuelHandle fuel )
{
	bool success = true;

	if ( fuel )
	{
		FastFuelFindHandle fh( fuel );
		if ( fh.FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				GoPotionRange range;
				if ( range.Load( key, value ) )
				{
					push_back( range );
				}
				else
				{
					gperrorf(( "Unable to add potion range '%s = '%s' at '%s\n",
							   key.c_str(), value.c_str(), fuel.GetAddress().c_str() ));
					success = false;
				}
			}
			stdx::sort( *this );
		}
	}
	return ( success );
}

bool GoInvRange :: Load( const gpstring& key, const gpstring& value )
{
	bool success = true;

	if ( !::FromString( key, m_MaxRatio ) || (m_MaxRatio < 0) || (m_MaxRatio > 1) )
	{
		gperrorf(( "Illegal float value '%s'\n", key.c_str() ));
		success = false;
	}

	int count = stringtool::GetNumDelimitedStrings( value, ',' );
	if ( count == 1 )
	{
		m_Model = value;
	}
	else if ( count == 2 )
	{
		gpstring tmp;
		stringtool::GetDelimitedValue( value, ',', 0, tmp );
		m_Model = tmp;
		stringtool::GetDelimitedValue( value, ',', 1, tmp );
		m_Texture = tmp;
	}
	else
	{
		gperrorf(( "Bad format '%s'\n", value.c_str() ));
		success = false;
	}

	return ( success );
}

bool GoInvRangeColl :: Load( FastFuelHandle fuel )
{
	bool success = true;

	if ( fuel )
	{
		FastFuelFindHandle fh( fuel );
		if ( fh.FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				GoInvRange range;
				if ( range.Load( key, value ) )
				{
					push_back( range );
				}
				else
				{
					gperrorf(( "Unable to add inventory range '%s = '%s' at '%s\n",
							   key.c_str(), value.c_str(), fuel.GetAddress().c_str() ));
					success = false;
				}
			}

			stdx::sort( *this );
		}
	}

	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class ContentDb implementation

const char* ContentDb::SYNC_DIRS[] =
{
	"art/animations/skrits",
	"world/contentdb",
	"world/global/formula",
	"world/global/effects",
	"world/ai",
	"ui/interfaces",
	NULL,
};

ContentDb :: ContentDb( void )
{
	m_PContentDb = new PContentDb;
	m_Formulas = NULL;
	m_MaxPartyGold = 0;

#	if !GP_RETAIL
	m_JitCompile = gConfig.GetBool( "contentdb_jit", false ) && !Services::IsEditor();
#	endif // !GP_RETAIL

	gMessageDispatch.RegisterBroadcastCallback( "contentdb", makeFunctor( *this, &ContentDb::HandleBroadcast ) );
}

ContentDb :: ~ContentDb( void )
{
	Shutdown();

	Delete( m_PContentDb );

	if( MessageDispatch::DoesSingletonExist() )
	{
		gMessageDispatch.UnregisterBroadcastCallback( "contentdb" );
	}
}

bool ContentDb :: Init( void )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	// $$$ use "unused param report" and dump set of extra fuel entries

// Init external systems that are Go-related.

	// must seed the member map!!
	Membership::StaticInit();

// Prep database.

	// clear out old data
	Shutdown();

	// fill in preregistered factories
	{
		for ( GocExporter* i = GocExporter::ms_Root ; i != NULL ; i = i->m_Next )
		{
			// only add non-dev components if we're forcing retail content
			if ( !i->m_DevOnly || !gWorldOptions.GetForceRetailContent() )
			{
				// add factory
				RegisterGoComponentFactory( i->m_Name, i->m_FactoryCb, i->m_ServerOnly, i->m_CanServerExistOnly );

				// add fubi type
				gpassert( m_FuBiComponentDb.find( i->m_Name ) == m_FuBiComponentDb.end() );
				m_FuBiComponentDb[ i->m_Name ] = gFuBiSysExports.FindType( i->m_InternalName );
			}
		}
	}

	// refresh data
	FastFuelHandle contentDb( "world:contentdb" );
	if ( !contentDb )
	{
		gperrorf(( "Unable to find content database, game will likely fail to run!\n" ));
		return ( false );
	}

	// find components
	FastFuelHandle components = contentDb.GetChildNamed( "components" );
	if ( !components )
	{
		gperrorf(( "Unable to find content database components, game will likely fail to run!\n" ));
		return ( false );
	}

// Gather constraints.

	#if !GP_RETAIL

	// add built-in choose constraints
	m_ConstraintDb.insert( ConstraintDb::value_type( "template", new ConstrainTemplates ) );

	// find all our constraints
	{
		// $ note we go 2 deep to catch constraints inside components (purely
		//   a convenience thing)
		FastFuelHandleColl constraintList;
		components.ListChildrenTyped( constraintList, "constraint", 2 );

		FastFuelHandleColl::const_iterator i, begin = constraintList.begin(), end = constraintList.end();
		for ( i = begin ; i != end ; ++i )
		{
			AddConstraint( *i );
		}
	}

	#endif // !GP_RETAIL

// Fill simple databases.

	// look up sound databases
	m_SoundDb = FastFuelHandle( "world:global:sounds:sounddb" );
	m_VoiceDb = FastFuelHandle( "world:global:sounds:global_voice" );

	// load the armor types db
	FastFuelHandle armorTypes( "world:global:armor_subtype_lookup" );
	if ( armorTypes )
	{
		FastFuelFindHandle fh( armorTypes );
		if ( fh.FindFirstKeyAndValue() )
		{
			gpstring key, value;
			while ( fh.GetNextKeyAndValue( key, value ) )
			{
				int count = stringtool::GetNumDelimitedStrings( value, ',' );
				if ( count != 2 )
				{
					gperrorf(( "Failed to parse armor type lookup '%s = '%s' at '%s\n",
							   key.c_str(), value.c_str(), armorTypes.GetAddress().c_str() ));
				}
				else
				{
					gpstring boot, gauntlet;
					stringtool::GetDelimitedValue( value, ',', 0, boot );
					stringtool::GetDelimitedValue( value, ',', 1, gauntlet );

					m_ArmorSubTypeDb[ key ] = std::make_pair( boot, gauntlet );
				}
			}
		}
	}

	// load the expiration db $$$ TEMPORARILY hard coded until constraints are in
	FastFuelHandle expireTypes( "world:contentdb:components:common:expiration_class:choose" );
	if ( expireTypes )
	{
		FastFuelHandleColl expireList;
		expireTypes.ListChildren( expireList );

		FastFuelHandleColl::iterator i, ibegin = expireList.begin(), iend = expireList.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			ExpireEntry entry;
			if ( (*i).Get( "delay_seconds", entry.m_DelaySeconds ) )
			{
				// optional range
				(*i).Get( "range_seconds", entry.m_RangeSeconds );

				// store it
				m_ExpireDb[ (*i).GetName() ] = entry;
			}
			else
			{
				gperrorf(( "Failed to parse expiration class entry '%s' at fuel '%s'\n",
						   (*i).GetName(), (*i).GetAddress().c_str() ));
			}
		}
	}

	// load the gui speed weapon ranges db
	FastFuelHandle speedRangeNames( "world:global:speed_gui_lookup" );
	if ( speedRangeNames )
	{
		FastFuelHandleColl speedChildren;
		FastFuelHandleColl::iterator i;
		speedRangeNames.ListChildren( speedChildren );
		for ( i = speedChildren.begin(); i != speedChildren.end(); ++i )
		{
			SpeedRangeNameEntry sre;
			sre.minRange = 0.0;
			sre.maxRange = 0.0;
			(*i).Get( "min_range", sre.minRange );
			(*i).Get( "max_range", sre.maxRange );
			(*i).Get( "screen_name", sre.sScreenName );
			m_SpeedRangeNameDb.push_back( sre );
		}
	}

// Construct templates.

	// find templates
	FastFuelHandle templates = contentDb.GetChildNamed( "templates" );
	if ( !templates )
	{
		gperrorf(( "Unable to find content database templates, game will likely fail to run!\n" ));
		return ( false );
	}

	// find all templates cursively from base of content db
	{
		// first insert templates by name
		FastFuelHandleColl templateList;
		templates.ListChildrenTyped( templateList, "template", -1 );
		FastFuelHandleColl::const_iterator i, begin = templateList.begin(), end = templateList.end();
		for ( i = begin ; i != end ; ++i )
		{
			AddTemplate( *i );
		}
	}

	// set up specialization tree
	{
		DataTemplateDb::iterator i = m_DataTemplateDb.begin();
		while ( i != m_DataTemplateDb.end() )
		{
			if ( i->second->InitBase() )
			{
				// root?
#				if ( !GP_RETAIL )
				{
					if ( i->second->IsRootTemplate() )
					{
						m_DataTemplateRoots.push_back( i->second );
					}
				}
#				endif // !GP_RETAIL

				// advance
				++i;
			}
			else
			{
				// failure - report and remove, reset iters
				gperrorf(( "Failure to add template '%s' to content database\n",
						   i->second->GetName() ));
				i->second->PrepFailureDeletion();
				delete ( i->second );
				i = m_DataTemplateDb.erase( i );
			}
		}
	}

// Read components.

	// find all our component specs
	{
		FastFuelHandleColl componentList;
		components.ListChildrenTyped( componentList, "component" );
		FastFuelHandleColl::const_iterator i, begin = componentList.begin(), end = componentList.end();
		for ( i = begin ; i != end ; ++i )
		{
			AddComponent( *i );
		}
	}

	// find all our skrit components
	{
		FileSys::RecursiveFinder finder( "world/contentdb/components/*.skrit" );
		gpstring name;
		while ( finder.GetNext( name, true ) )
		{
			// build the name
			const gpstring& componentName = AddString( FileSys::GetFileNameOnly( name ) );

			// is it used? if not, don't bother compiling it
			if ( m_ComponentNameDb.find( componentName ) != m_ComponentNameDb.end() )
			{
				// try to load up the skrit to get at properties and schema - don't
				// actually construct it though (ever). also note that we should not
				// persist this, as it's effectively "const".
				Skrit::HAutoObject skrit;
				Skrit::GetReq getReq( name );
				getReq.SetOwner( GoSkritComponent::GetFuBiType() );
				getReq.m_DeferConstruction = true;
				getReq.SetPersist( false );
				if ( skrit.CreateSkrit( getReq ) )
				{
					AddComponent( skrit, componentName );
				}
				else
				{
					gperrorf(( "Failure to compile Skrit component '%s'\n", name.c_str() ));
				}
			}
			else
			{
				gpwarningf(( "Unused skrit component '%s' detected!\n", name.c_str() ));
			}
		}

		// now that we have them all, we no longer need the db
		m_ComponentNameDb.clear();
	}

#	if ( !GP_RETAIL )
	{
		// verify that we have a 1:1 match between the component factory and
		// the data component db
		GocFactoryDb   ::const_iterator fi = m_GocFactoryDb   .begin(), fend = m_GocFactoryDb   .end();
		DataComponentDb::const_iterator ci = m_DataComponentDb.begin(), cend = m_DataComponentDb.end();
		for ( ; ; )
		{
			int compare;
			if ( fi == fend )
			{
				if ( ci == cend )
				{
					break;
				}
				compare = 1;
			}
			else if ( ci == cend )
			{
				compare = -1;
			}
			else
			{
				compare = fi->first.compare_no_case( ci->first );
			}

			if ( compare == 0 )
			{
				++fi;
				++ci;
			}
			else if ( compare < 0 )
			{
				gperrorf(( "Component '%s' found in factory but no equivalent schema exists\n", fi->first.c_str() ));
				++fi;
			}
			else
			{
				if ( ci->second.m_DataComponent != NULL )
				{
					gperrorf(( "Component '%s' found in schema but no equivalent factory exists\n", ci->first.c_str() ));
				}
				++ci;
			}
		}
	}
#	endif // !GP_RETAIL

// Add builder type.

	// this will perform the recursion and track dependencies
	struct Builder
	{
		typedef std::set <GoDataTemplate*> GodtSet;

		GodtSet m_Templates;				// these are templates to be processed
		GodtSet m_FailedTemplates;			// these are templates that failed to specialize
		GodtSet m_Recursion;				// watch for cyclic specialization

		bool Init( GoDataTemplate* godt )
		{
			// update recursion
			std::pair <GodtSet::iterator, bool> rc = m_Recursion.insert( godt );
			if ( !rc.second )
			{
				ReportSys::AutoReport autoReport( &gErrorContext );
				gperror( "Cyclic template specialization detected in the set: " );

				GodtSet::const_iterator i, begin = m_Recursion.begin(), end = m_Recursion.end();
				for ( i = begin ; i != end ; ++i )
				{
					if ( i != begin )
					{
						gperror( ", " );
					}
					gperror( (*i)->GetName() );
				}
				gperror( "\n" );
				return ( false );
			}

			bool success = true;

			// make sure specialized base is initialized
			GoDataTemplate* base = ccast <GoDataTemplate*> ( godt->GetBaseDataTemplate() );
			if ( (base != NULL) && (m_Templates.find( base ) != m_Templates.end()) )
			{
				// init that one - if failed, then godt fails because we
				// can't specialize on a failed template.
				success = Init( base );
			}

			// initialize local
			if ( !success || !godt->Init() )
			{
				// add to failed set
				m_FailedTemplates.insert( godt );
				success = false;
			}

			// remove from working set
			m_Templates.erase( m_Templates.find( godt ) );

			// remove recursion
			m_Recursion.erase( rc.first );

			// done
			return ( success );
		}

		void Init( void )
		{
			while ( !m_Templates.empty() )
			{
				Init( *m_Templates.begin() );
				gpassert( m_Recursion.empty() );
			}
		}
	};

// Specialize templates.

	// build builder
	Builder builder;

	// process actual template data (now that specialization is possible)
	{
		// add each to builder set
		DataTemplateDb::iterator i, begin = m_DataTemplateDb.begin(), end = m_DataTemplateDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			builder.m_Templates.insert( i->second );
		}

		// now do it
		builder.Init();

		// build set of metagroup names
#		if ( !GP_RETAIL )
		{
			if ( !gContentDb.IsJitCompile() )
			{
				DataTemplateDb::const_iterator i, ibegin = m_DataTemplateDb.begin(), iend = m_DataTemplateDb.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					m_MetaGroupNames.insert( i->second->GetMetaGroupName() );
				}
			}
		}
#		endif // !GP_RETAIL
	}

	// process errors
	{
		// add each to builder set
		Builder::GodtSet::const_iterator i,
										 ibegin = builder.m_FailedTemplates.begin(),
										 iend   = builder.m_FailedTemplates.end  ();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// print error
			gperrorf(( "Failure to add template '%s' to content database\n",
					   (*i)->GetName() ));

			// remove it from set
			DataTemplateDb::iterator j, jbegin = m_DataTemplateDb.begin(), jend = m_DataTemplateDb.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( *i == j->second )
				{
					m_DataTemplateDb.erase( j );
					break;
				}
			}

			// make sure we found it
			gpassert( j != jend );
		}
	}

// Initialize owned objects.

	// parameterized content database
	if ( !m_PContentDb->Init() )
	{
		return ( false );
	}

	// get skrit formulas
	{
		FastFuelHandle formulas( "world:global:formula:general_formulas" );
		if ( formulas )
		{
			// get the skrit code
			gpstring skrit;
			if ( formulas.Get( "skrit", skrit ) )
			{
				// attempt to compile the skrit
				Skrit::CreateReq createReq( "formulas.gas:skrit" );
				m_Formulas = gSkritEngine.CreateNewObject( createReq, const_mem_ptr( skrit.begin(), skrit.size() ) );
				if ( m_Formulas != NULL )
				{
					FuBi::Record* record = m_Formulas->GetRecord();
					if ( record != NULL )
					{
						record->SetXferMode( GP_DEBUG ? FuBi::XMODE_STRICT : FuBi::XMODE_QUIET );
					}
				}
			}
		}

		// error on any kind of failure
		if ( m_Formulas == NULL )
		{
			gperror( "Unable to find/compile general formulas!\n" );
		}
	}

// Initialize defaults

	FastFuelHandle hDefaults( "world:contentdb:defaults" );
	m_MaxPartyGold = 9999999;
	if ( hDefaults.IsValid() )
	{
		// misc
		hDefaults.Get( "default_texture",         m_DefaultTextureName            );

		// templates
		hDefaults.Get( "default_gold",            m_DefaultGoldTemplate           );
		hDefaults.Get( "default_packmule",        m_DefaultPackmuleTemplate       );
		hDefaults.Get( "default_mule",            m_DefaultMuleTemplate           );
		hDefaults.Get( "default_spellbook",       m_DefaultSpellBookTemplate      );
		hDefaults.Get( "default_tombstone",       m_DefaultTombstoneTemplate      );
		hDefaults.Get( "default_fader_proxy",     m_DefaultFaderProxyTemplate     );
		hDefaults.Get( "default_waypoint_moving", m_DefaultWaypointMovingTemplate );
		hDefaults.Get( "default_point",           m_DefaultPointTemplate          );
		hDefaults.Get( "default_party_humanoid",  m_DefaultPartyHumanoidTemplate  );
		hDefaults.Get( "default_stash",			  m_DefaultStashTemplate		  );
		hDefaults.Get( "max_party_gold",		  m_MaxPartyGold				  );
	}

	const GoDataComponent* aspectGoc = FindComponentByName( "aspect" );
	if ( aspectGoc != NULL )
	{
		aspectGoc->GetRecord()->Get( "model", m_DefaultAspectName );
	}

// Build sync object.

	// calc easy stuff
	m_SyncSpec.m_FormulasSkritCrc = m_Formulas ? m_Formulas->GetImpl()->AddCrc32() : 0;
	m_SyncSpec.m_PContentSkritCrc = m_PContentDb->AddCrc32();

	// see if we can get a crc from fuel
	{
		m_SyncSpec.m_ContentDbLqdCrc = 0;

		for ( const char** i = SYNC_DIRS ; *i != NULL ; ++i )
		{
			FastFuelHandle fuel( FilePathToFastFuelHandle( *i ) );
			if ( fuel )
			{
				DWORD crc = fuel.GetRecursiveStaticChecksum();
				m_SyncSpec.m_ContentDbLqdCrc += crc;
#if				GP_DEBUG
				gpgenericf(( "recursive static Fuel CRC for %s = 0x%08x\n", *i, crc ));
#endif
			}
		}
	}

	// add in crc's from skrit
	{
		DataComponentDb::const_iterator i, ibegin = m_DataComponentDb.begin(), iend = m_DataComponentDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			const GoDataComponent* goc = i->second.m_DataComponent;
			if ( (goc != NULL) && goc->IsSkrit() )
			{
				m_SyncSpec.m_ContentDbSkritCrc = goc->GetSkritObject()->GetImpl()->AddCrc32( m_SyncSpec.m_ContentDbSkritCrc );
			}
		}
	}

// Done.

	// print status
	GPDEV_ONLY( DumpStatistics() );

	// we're ok
	return ( true );
}

void ContentDb :: HandleBroadcast( WorldMessage & msg )
{
	if( msg.GetEvent() == WE_POST_RESTORE_GAME )
	{
		for( ChoreDictionaryDb::iterator i = m_ChoreDictionaryDb.begin(); i != m_ChoreDictionaryDb.end(); ++i )
		{
			RpChoreDictionary cd = (*i).second;
			for( ChoreDictionary::iterator j = cd->begin(); j != cd->end(); ++j )
			{
				(*j).second.CommitCreation();
			}
		}
	}
}

bool ContentDb :: IsInitialized( void )
{
	return ( !m_GocFactoryDb.empty() );
}

void ContentDb :: Shutdown( void )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	gpassert( m_ComponentNameDb.empty() );

	// shutdown owned objects
	m_PContentDb->Shutdown();
	Delete ( m_Formulas );

	// get the templates first (they use the component data)
	stdx::for_each( m_DataTemplateDb, stdx::delete_second_by_ptr() );

	// now we can delete our components
	DataComponentDb::iterator i, ibegin = m_DataComponentDb.begin(), iend = m_DataComponentDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Delete( i->second.m_DataComponent   );
		Delete( i->second.m_StoreHeaderSpec );
	}

	// free data
	m_GocFactoryDb     .clear();
	m_DataComponentDb  .clear();
	m_FuBiComponentDb  .clear();
	m_DataTemplateDb   .clear();
	m_StringCollector  .clear();
	m_WStringCollector .clear();
	m_ArmorSubTypeDb   .clear();
	m_ExpireDb         .clear();
	m_SpeedRangeNameDb .clear();

	GPDEV_ONLY( m_DataTemplateRoots.clear() );

	// clear fuel handles
	m_SoundDb = FastFuelHandle();
	m_VoiceDb = FastFuelHandle();

	// clear collectors
	m_BoneTranslatorDb    .clear();
	m_NullBoneTranslator  .Free();
	m_ChoreDictionaryDb   .clear();
	m_EnchantmentStorageDb.clear();
	m_TriggerStorageDb    .clear();
	m_GoGoldRangeCollDb   .clear();
	m_GoPotionRangeCollDb .clear();
	m_GoInvRangeCollDb    .clear();

	// clear cache
	m_DefaultAspectName            .clear();
	m_DefaultTextureName           .clear();
	m_DefaultGoldTemplate          .clear();
	m_DefaultPackmuleTemplate      .clear();
	m_DefaultMuleTemplate          .clear();
	m_DefaultSpellBookTemplate     .clear();
	m_DefaultTombstoneTemplate     .clear();
	m_DefaultFaderProxyTemplate    .clear();
	m_DefaultWaypointMovingTemplate.clear();
	m_DefaultPointTemplate         .clear();
	m_DefaultPartyHumanoidTemplate .clear();
	m_DefaultStashTemplate		   .clear();
	m_MaxPartyGold = 0;
	m_SyncSpec = SyncSpec();

	// clear the various query caches
	AutoConstQueryBase::ResetColumnCaches();

	// clear siege db
	gSiegeEngine.MeshDatabase().FileNameMap().clear();

	// clear dev-only databases
#	if !GP_RETAIL
	m_DataTemplateRoots.clear();
	m_MetaGroupNames   .clear();
	m_ConstraintDb     .clear();
#	endif // !GP_RETAIL
}

void ContentDb :: RegisterGoComponentFactory( const gpstring& name, GocFactoryCb cb, bool serverOnly, bool canServerExistOnly )
{
	gpassert( m_GocFactoryDb.find( name ) == m_GocFactoryDb.end() );
	m_GocFactoryDb.insert( std::make_pair( name, FactoryEntry( cb, serverOnly, canServerExistOnly ) ) );
}

void ContentDb :: UnregisterGoComponentFactory( const gpstring& name )
{
	GocFactoryDb::iterator found = m_GocFactoryDb.find( name );
	gpassert( found != m_GocFactoryDb.end() );
	m_GocFactoryDb.erase( found );
}

template <typename T>
void DerefTemplateInstanceField(
	OUT T&						out,				// output field value here
	OUT const GoDataTemplate**	godt,				// if non-null, set this to found template
	OUT const GoDataComponent**	godc,				// if non-null, set this to found component
	OUT FastFuelHandle*			componentFuel,		// if non-null, set this to component fuel
	IN  FastFuelHandle			instance,			// incoming object fuel
	IN  const char*				componentName,		// name of component to look in
	IN  const char*				fieldName )			// name of field to query
{
	// verify params
	gpassert( instance );
	gpassert( (componentName != NULL) && (*componentName != '\0') );
	gpassert( (fieldName != NULL) && (*fieldName != '\0') );

	// look up instance
	FastFuelHandle localComponentFuel;
	if ( componentFuel == NULL )
	{
		componentFuel = &localComponentFuel;
	}
	if ( !*componentFuel )
	{
		*componentFuel = instance.GetChildNamed( componentName );
	}

	// try to extract the override
	if ( !*componentFuel || !componentFuel->Get( fieldName, out ) )
	{
		// look up the data component
		const GoDataComponent* localGodc = NULL;
		if ( godc == NULL )
		{
			godc = &localGodc;
		}
		if ( *godc == NULL )
		{
			// look up the template
			const GoDataTemplate* localGodt = NULL;
			if ( godt == NULL )
			{
				godt = &localGodt;
			}
			if ( *godt == NULL )
			{
				// grab a ref so we can use its data - this will need to be
				// released later on by our caller...
				*godt = gContentDb.AcquireTemplateByName( instance.GetType() );
			}

			// look up the data component
			if ( *godt != NULL )
			{
				*godc = (*godt)->FindComponentByName( componentName );
			}
		}

		// extract the field
		if ( *godc != NULL )
		{
			(*godc)->GetRecord()->Get( fieldName, out );
		}
	}
}

ContentDb::eCreateType ContentDb :: GetCreateTypeForScid( Scid scid, RegionId regionId ) const
{
	// in SE we always create it globally
#	if !GP_RETAIL
	if ( gGoDb.IsEditMode() )
	{
		return ( CREATE_GLOBAL );
	}
#	endif // !GP_RETAIL

	// make default true, so any lookup errors will still get reported properly
	// later on in the creation function
	eCreateType createType = CREATE_GLOBAL;

	// find the block
	FastFuelHandle fuel( gWorldMap.MakeObjectFuel( scid, regionId ) );
	if ( fuel )
	{
		// check disable by sp/mp only flag
		bool should = true;
		const GoDataTemplate* godt = NULL;
		DerefTemplateInstanceField(
				should, &godt, NULL, NULL, fuel, "common",
				(::IsSinglePlayer() && !gWorldOptions.GetForceMpContent()) ? "is_single_player" : "is_multi_player" );

		// still should? check lodfi
		if ( should )
		{
			// cache vars
			float lowerLodfi = -1, upperLodfi = -1;
			const GoDataComponent* godc = NULL;
			FastFuelHandle componentFuel;
			float lodfi = gWorldOptions.GetObjectDetailLevel();

			// get upper bound
			DerefTemplateInstanceField( upperLodfi, &godt, &godc, &componentFuel, fuel, "aspect", "lodfi_upper" );

			// lodfi less than zero means it's definitely global
			if ( !::IsNegative( upperLodfi ) )
			{
				// probably creating a local one unless out of range
				createType = CREATE_LOCAL_LODFI;

				// only continue checks if we aren't definitely using it
				if ( lodfi < upperLodfi )
				{
					// get upper bound
					DerefTemplateInstanceField( lowerLodfi, &godt, &godc, &componentFuel, fuel, "aspect", "lodfi_lower" );

					// if out of range, definitely not, otherwise choose random
					// number in range for comparison
					if ( (lodfi < lowerLodfi) || (lodfi < Random( lowerLodfi, upperLodfi )) )
					{
						createType = CREATE_NONE;
					}
				}

				// check for blocking if lodfi 0/0 (but only bother if we're the
				// server in an MP game, no point otherwise).
				if ( (upperLodfi == 0) && ::IsMultiPlayer() && ::IsServerLocal() )
				{
					bool doesBlockPath = false;
					DerefTemplateInstanceField( doesBlockPath, &godt, &godc, &componentFuel, fuel, "aspect", "does_block_path" );
					if ( doesBlockPath )
					{
						// $ the whole point of this setting is to force the
						//   server to load blocking objects for all client
						//   frustums. we keep them lodfi as an optimization for
						//   network and save game, but lodfi does not normally
						//   load on host's screen frustum so making them pure
						//   local instead will bypass this problem without
						//   forcing them global.

						createType = CREATE_LOCAL_PLAIN;
					}
				}
			}
		}
		else
		{
			// not supposed to create due to sp/mp requirements
			createType = CREATE_NONE;
		}

		// finally, release the template if we found it
		if ( godt != NULL )
		{
			gContentDb.ReleaseTemplate( godt );
		}
	}

	// done
	return ( createType );
}

Go* ContentDb :: CreateGo( const GoCreateReqRestrict& createReq ) const
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	// parameter validation
	gpassert( createReq->AssertValid() );

	// get the player
	Player* player = gServer.GetPlayer( createReq->m_PlayerId );
	if ( player == NULL )
	{
		gperrorf(( "Illegal player attempted to create a game object (id = 0x%08X)\n",
				   createReq->m_PlayerId ));
		return ( NULL );
	}

	// find the block
	FastFuelHandle fuel( gWorldMap.MakeObjectFuel( createReq->m_Scid, createReq->m_RegionId, createReq->m_StartingPos.node, true ) );
	if ( !fuel )
	{
		return ( NULL );
	}

	// create the Go
	std::auto_ptr <Go> newGo( new Go( player ) );

	// set its region id
	gpassert( createReq->m_RegionId != REGIONID_INVALID );
	newGo->m_RegionSource = createReq->m_RegionId;

	// load it up - note the persistence flag is based on lodfi status
	if ( !newGo->LoadFromInstance( fuel, SCID_INVALID, false, !createReq->m_LodfiGo ) )
	{
		gperrorf(( "Failure to load game object (Scid = 0x%08X)\n", createReq->m_Scid ));
		return ( NULL );
	}

	// all done!
	return ( newGo.release() );
}

GoComponent* ContentDb :: CreateGoComponent( Go* parent,
											 const GoDataComponent& godc,
											 FastFuelHandle gocOverride,
											 bool& serverComponent,
											 bool autoXfer,
											 bool shouldPersist ) const
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	// create the component
	std::auto_ptr <GoComponent> newGoc( CreateRawGoComponent( parent, godc.GetName(), serverComponent ) );
	if ( newGoc.get() == NULL )
	{
		gperrorf(( "Failure to construct component: '%s'\n", godc.GetName().c_str() ));
		return ( NULL );
	}

	// now optionally specialize
	FastFuelHandle child;
	if ( gocOverride && (child = gocOverride.GetChildNamed( godc.GetName() )).IsValid() )
	{
		// copy, specialize
		std::auto_ptr <GoDataComponent> newGodc( new GoDataComponent( godc ) );
		if ( !newGodc->SpecializeData( child ) )
		{
			gperrorf(( "Failure to specialize component: '%s'\n", godc.GetName().c_str() ));
			return ( NULL );
		}

		// assign and release
		newGoc->Init( newGodc.release() );
	}
	else
	{
		// not specialized - just point to data component
		newGoc->Init( &godc );
	}

	// should we be xfering?
	if ( autoXfer )
	{
		// xfer data in as partial (initializing) persist object
		FuBi::RecordReader reader( *newGoc->GetData()->GetRecord() );
		FuBi::PersistContext persist( &reader, false );

		// special skrit component handling - it needs that flag
		bool success = false;
		if ( !shouldPersist && godc.IsSkrit() )
		{
			success = ((GoSkritComponent*)newGoc.get())->SpecialXfer( persist, shouldPersist );
		}
		else
		{
			success = newGoc->Xfer( persist );
		}

		// handle error
		if ( !success )
		{
			gperrorf(( "Failure to xfer data into component '%s'\n", godc.GetName().c_str() ));
			return ( NULL );
		}
	}

	// all done
	return ( newGoc.release() );
}

GoComponent* ContentDb :: CreateRawGoComponent( Go* parent, const gpstring& type, bool& serverComponent ) const
{
	GPSTATS_SYSTEM( SP_LOAD_GO );

	gpassert( parent != NULL );
	serverComponent = false;

	// look up factory
	GocFactoryDb::const_iterator found = m_GocFactoryDb.find( type );
	if ( found == m_GocFactoryDb.end() )
	{
		gperrorf(( "Component '%s' not found in factory\n", type.c_str() ));
		return ( NULL );
	}

	// server only?
	serverComponent = found->second.m_ServerOnly;

	// create the component
	return ( (found->second.m_FactoryCb)( type, parent ) );
}

#if !GP_RETAIL

int ContentDb :: GetStringCollectedCount( void ) const
{
	return ( m_StringCollector.size() + m_WStringCollector.size() );
}

int ContentDb :: GetStringAllocsSavedCount( void ) const
{
	int saved = 0;

	StringDb::const_iterator i, ibegin = m_StringCollector.begin(), iend = m_StringCollector.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		saved += i->get_ref_count();
	}

	WStringDb::const_iterator j, jbegin = m_WStringCollector.begin(), jend = m_WStringCollector.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		saved += j->get_ref_count();
	}

	return ( saved );
}

int ContentDb :: GetStringMemSavedBytes( void ) const
{
	int saved = 0;

	StringDb::const_iterator i, ibegin = m_StringCollector.begin(), iend = m_StringCollector.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		saved += i->get_ref_count() * i->capacity();
	}

	WStringDb::const_iterator j, jbegin = m_WStringCollector.begin(), jend = m_WStringCollector.end();
	for ( j = jbegin ; j != jend ; ++j )
	{
		saved += j->get_ref_count() * j->capacity() * 2;
	}

	return ( saved );
}

void ContentDb :: DumpStatistics( ReportSys::ContextRef ctx = NULL ) const
{
	// get leaf count
	int leafCount = 0;
	DataTemplateDb::const_iterator i, ibegin = m_DataTemplateDb.begin(), iend = m_DataTemplateDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->second->IsLeafTemplate() )
		{
			++leafCount;
		}
	}

	// dump stats
	ReportSys::AutoReport autoReport( ctx );
	ctx->OutputF( "ContentDb initialization completed:\n" );
	{
		ReportSys::AutoIndent autoIndent( ctx );
		ctx->OutputF( "Factory count        : %d\n", m_GocFactoryDb.size() );
		ctx->OutputF( " - C++ components    : %d\n", m_FuBiComponentDb.size() );
		ctx->OutputF( " - Skrit components  : %d\n", m_GocFactoryDb.size() - m_FuBiComponentDb.size() );
		ctx->OutputF( "Data component count : %d\n", m_DataComponentDb.size() );
		ctx->OutputF( "Leaf template count  : %d (%d total)\n", leafCount, m_DataTemplateDb.size() );
		ctx->OutputF( "String collector     : count = %d, saved (refs) = %d, saved (bytes) = %d\n",
					  GetStringCollectedCount(), GetStringAllocsSavedCount(), GetStringMemSavedBytes() );
	}
}

#endif // !GP_RETAIL

const GoDataComponent* ContentDb :: FindComponentByName( const gpstring& name, bool* wasInternal ) const
{
	const GoDataComponent* component = NULL;

	DataComponentDb::const_iterator found = m_DataComponentDb.find( name );
	if ( found == m_DataComponentDb.end() )
	{
		if ( wasInternal != NULL )
		{
			*wasInternal = false;
		}
	}
	else
	{
		component = found->second.m_DataComponent;
		if ( wasInternal != NULL )
		{
			*wasInternal = (component == NULL);
		}
	}

	return ( component );
}

FuBi::eVarType ContentDb :: FindFuBiTypeByName( const char* name ) const
{
	FuBiComponentDb::const_iterator found = m_FuBiComponentDb.find( name );
	return ( (found == m_FuBiComponentDb.end()) ? FuBi::VAR_UNKNOWN : found->second );
}

const GoDataTemplate* ContentDb :: FindTemplateByName( const char* name ) const
{
	DataTemplateDb::const_iterator found = m_DataTemplateDb.find( name );
	return ( (found == m_DataTemplateDb.end()) ? NULL : found->second );
}

int ContentDb :: FindTemplatesByWildcard( TemplateColl& coll, const char* pattern, bool leavesOnly ) const
{
	int oldSize = coll.size();

	if ( (pattern == NULL) || (pattern[ ::strcspn( pattern, "*?" ) ] != '\0') )
	{
		DataTemplateDb::const_iterator i, begin = m_DataTemplateDb.begin(), end = m_DataTemplateDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			if (   ((pattern == NULL) || stringtool::IsDosWildcardMatch( pattern, i->second->GetName() ))
				&& (!leavesOnly || i->second->IsLeafTemplate()) )
			{
				coll.push_back( i->second );
			}
		}
	}
	else
	{
		const GoDataTemplate* found = FindTemplateByName( pattern );
		if ( (found != NULL) && (!leavesOnly || found->IsLeafTemplate()) )
		{
			coll.push_back( found );
		}
	}

	return ( coll.size() - oldSize );
}

#if !GP_RETAIL

int ContentDb :: FindTemplatesByDocWildcard( TemplateColl& coll, const char* pattern, bool leavesOnly ) const
{
	int oldSize = coll.size();

	DataTemplateDb::const_iterator i, begin = m_DataTemplateDb.begin(), end = m_DataTemplateDb.end();
	for ( i = begin ; i != end ; ++i )
	{
		AutoRefTemplate autoRef( i->second );

		if (   ((pattern == NULL) || stringtool::IsDosWildcardMatch( pattern, i->second->GetDocs() ))
			&& (!leavesOnly || i->second->IsLeafTemplate()) )
		{
			coll.push_back( i->second );
		}
	}

	return ( coll.size() - oldSize );
}

#endif // !GP_RETAIL

static void AddTemplatesAndChildren( TemplateColl& coll, const GoDataTemplate* root, bool leavesOnly )
{
	if ( !leavesOnly || root->IsLeafTemplate() )
	{
		coll.push_back( root );
	}

	GoDataTemplate::ChildIter i, ibegin = root->GetChildBegin(), iend = root->GetChildEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		AddTemplatesAndChildren( coll, *i, leavesOnly );
	}
}

int ContentDb :: FindTemplatesAndChildren( TemplateColl& coll, const char* rootName, bool leavesOnly ) const
{
	int oldSize = coll.size();

	const GoDataTemplate* root = FindTemplateByName( rootName );
	if ( root != NULL )
	{
		::AddTemplatesAndChildren( coll, root, leavesOnly );
	}

	return ( coll.size() - oldSize );
}

gpstring ContentDb :: FindMaterialSound( const char* event, gpstring& pri, const char* srcMaterial, const char* dstMaterial, bool recurse )
{
	if ( (srcMaterial == NULL) || (*srcMaterial == '\0') )
	{
		srcMaterial = "generic";
	}
	if ( (dstMaterial == NULL) || (*dstMaterial == '\0') )
	{
		dstMaterial = "generic";
	}

	gpassert( (event != NULL) && (*event != '\0') );

	FastFuelHandle src, dst, evt;

	if ( m_SoundDb )
	{
		src = m_SoundDb.GetChildNamed( srcMaterial );
		if ( src )
		{
			dst = src.GetChildNamed( dstMaterial );
			if ( dst )
			{
				evt = dst.GetChildNamed( event );
			}
		}
	}

	gpstring choice;

	if ( evt )
	{
		int key_sub	= 1;
		FastFuelHandle fevent( evt );
		if( fevent.IsValid() && fevent.HasKey( "priority" ) )
		{
			pri		= fevent.GetString( "priority" );
			key_sub	= 2;
		}

		FastFuelFindHandle fh( evt );
		if( fh.FindFirstValue( "*" ) )
		{
			// $$$ optimise -bk
			int index = Random( fh.GetKeyAndValueCount() - key_sub );
			while ( index-- >= 0 )
			{
				fh.GetNextValue( choice );
			}
		}
	}
	else if ( recurse )
	{
		bool srcGeneric = same_no_case( srcMaterial, "generic" );
		bool dstGeneric = same_no_case( dstMaterial, "generic" );

		if ( !srcGeneric && !dstGeneric )
		{
			choice = FindMaterialSound( event, pri, "generic", dstMaterial, false );
			if ( choice.empty() )
			{
				choice = FindMaterialSound( event, pri, srcMaterial, "generic", false );
				if ( choice.empty() )
				{
					choice = FindMaterialSound( event, pri, "generic", "generic", false );
				}
			}
		}
		else if ( !srcGeneric || !dstGeneric )
		{
			choice = FindMaterialSound( event, pri, "generic", "generic", false );
		}
	}

	return ( choice );
}

gpstring ContentDb :: FindGlobalVoiceSound( const char* event, gpstring& pri )
{
	gpassert( (event != NULL) && (*event != '\0') );

	FastFuelHandle evt = m_VoiceDb ? m_VoiceDb.GetChildNamed( event ) : FastFuelHandle();
	if ( !evt )
	{
		return ( gpstring::EMPTY );
	}

	int key_sub	= 1;
	FastFuelHandle fevent( evt );
	if( fevent.IsValid() && fevent.HasKey( "priority" ) )
	{
		pri		= fevent.GetString( "priority" );
		key_sub	= 2;
	}

	FastFuelFindHandle fh( evt );
	if( !fh.FindFirstValue( "*" ) )
	{
		return ( gpstring::EMPTY );
	}

	gpstring choice;
	int index = Random( fh.GetKeyAndValueCount() - key_sub );
	while ( index-- >= 0 )	// $$$ optimize -bk
	{
		fh.GetNextValue( choice );
	}

	return ( choice );
}

const ContentDb::ExpireEntry* ContentDb :: FindExpirationClass( const char* name ) const
{
	gpassert( name != NULL );
	ExpireDb::const_iterator found = m_ExpireDb.find( name );
	return ( (found == m_ExpireDb.end()) ? NULL : &found->second );
}

const GoDataTemplate* ContentDb :: AcquireTemplateByName( const char* name )
{
	const GoDataTemplate* godt = FindTemplateByName( name );
	if ( godt != NULL )
	{
		ccast <GoDataTemplate*> ( godt )->AddRef();
	}
	return ( godt );
}

int ContentDb :: AddRefTemplate( const GoDataTemplate* godt )
{
	gpassert( godt != NULL );
	return ( ccast <GoDataTemplate*> ( godt )->AddRef() );
}

int ContentDb :: ReleaseTemplate( const GoDataTemplate* godt )
{
	gpassert( godt != NULL );
	return ( ccast <GoDataTemplate*> ( godt )->Release() );
}

RpBoneTranslator ContentDb :: AddOrFindBoneTranslator( FastFuelHandle fuel, nema::HAspect aspect )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	gpassert( aspect );

	RpBoneTranslator ptr;

	// must have valid fuel
	if ( fuel )
	{
		// lookup/add
		std::pair <BoneTranslatorDb::iterator, bool> rc = m_BoneTranslatorDb.insert( std::make_pair( fuel, RpBoneTranslator() ) );
		RpBoneTranslator& entry = rc.first->second;

		if ( rc.second )
		{
			// have to create new one, start iter
			if ( fuel.GetNumKeys() > 0 )
			{
				// new entry
				entry.Assign();

				// load and take it
				if ( entry->Load( aspect, fuel ) )
				{
					ptr = entry;
				}
			}

			// undo
			if ( ptr.IsNull() )
			{
				m_BoneTranslatorDb.erase( rc.first );
			}
		}
		else
		{
			ptr = entry;
		}
	}

	// done
	return ( ptr );
}

RpBoneTranslator ContentDb :: AddOrFindNullBoneTranslator( void )
{
	if ( !m_NullBoneTranslator )
	{
		m_NullBoneTranslator.Assign();
	}
	return ( m_NullBoneTranslator );
}

template <typename PTR, typename COLL>
bool AddOrFindGeneric( FastFuelHandle fuel, PTR& ptr, COLL& coll, const PTR::DataType* base = NULL )
{
	GPSTATS_SYSTEM( SP_STATIC_CONTENT );

	// must have valid fuel
	if ( !fuel )
	{
		return ( false );
	}

	// lookup/add
	std::pair <COLL::iterator, bool> rc = coll.insert( std::make_pair( fuel, PTR() ) );
	PTR& entry = rc.first->second;
	if ( rc.second )
	{
		// new entry
		if ( base != NULL )
		{
			entry.Assign( *base );
		}
		else
		{
			entry.Assign();
		}

		// load and take it
		if ( entry->Load( fuel ) )
		{
			ptr = entry;
		}
		else
		{
			// undo
			coll.erase( rc.first );
		}
	}
	else
	{
		// take existing
		ptr = entry;
	}

	// done
	return ( true );
}

RpChoreDictionary ContentDb :: AddOrFindChoreDictionary( FastFuelHandle fuel )
{
	RpChoreDictionary ptr;
	AddOrFindGeneric( fuel, ptr, m_ChoreDictionaryDb );
	return ( ptr );
}

RpEnchantmentStorage ContentDb :: AddOrFindEnchantmentStorage( FastFuelHandle fuel, const EnchantmentStorage* base )
{
	RpEnchantmentStorage ptr;
	if ( !AddOrFindGeneric( fuel, ptr, m_EnchantmentStorageDb, base ) )
	{
		gperror( "No enchantment data found!\n" );
	}

	// done
	return ( ptr );
}

RpTriggerStorage ContentDb :: AddOrFindTriggerStorage( FastFuelHandle fuel, bool IsTemplateStorage )
{
	RpTriggerStorage ptr;

	// must have valid fuel
	if ( !fuel )
	{
		gperror( "No trigger data found!\n" );
		return ( ptr );
	}

	// lookup/add
	std::pair <TriggerStorageDb::iterator, bool> rc = m_TriggerStorageDb.insert( std::make_pair( fuel, RpTriggerStorage() ) );
	RpTriggerStorage& entry = rc.first->second;
	if ( rc.second )
	{
		// new entry
		entry.Assign();

		// load and take it
		if ( entry->Load( fuel, IsTemplateStorage ) )
		{
			ptr = entry;
		}
		else
		{
			// undo
			m_TriggerStorageDb.erase( rc.first );
		}
	}
	else
	{
		// take existing
		ptr = entry;
	}

	// done
	return ( ptr );
}

RpGoGoldRangeColl ContentDb :: AddOrFindGoldRangeColl( FastFuelHandle fuel )
{
	RpGoGoldRangeColl ptr;
	AddOrFindGeneric( fuel, ptr, m_GoGoldRangeCollDb );
	return ( ptr );
}

RpGoPotionRangeColl ContentDb :: AddOrFindPotionRangeColl( FastFuelHandle fuel )
{
	RpGoPotionRangeColl ptr;
	AddOrFindGeneric( fuel, ptr, m_GoPotionRangeCollDb );
	return ( ptr );
}

RpGoInvRangeColl ContentDb :: AddOrFindInvRangeColl( FastFuelHandle fuel )
{
	RpGoInvRangeColl ptr;
	AddOrFindGeneric( fuel, ptr, m_GoInvRangeCollDb );
	return ( ptr );
}

#if !GP_RETAIL

FuBi::ConstraintSpec* ContentDb :: CreateConstraint( const char* name ) const
{
	gpassert( name != NULL );

	FuBi::ConstraintSpec* newSpec = NULL;

	ConstraintDb::const_iterator found = m_ConstraintDb.find( name );
	if ( found != m_ConstraintDb.end() )
	{
		newSpec = found->second.m_Constraint->Clone();
	}

	return ( newSpec );
}

#endif // !GP_RETAIL

static void RecurseGetAllHeroes( const GoDataTemplate* base, TemplateColl& coll )
{
	// $$ make this a generic iter-filter function instead

	gpassert( base != NULL );

	if ( base->IsLeafTemplate() )
	{
		gContentDb.AddRefTemplate( base );

		const GoDataComponent* actor = base->FindComponentByName( "actor" );
		if ( actor != NULL )
		{
			bool isHero;
			actor->GetRecord()->Get( "is_hero", isHero );
			if ( isHero )
			{
				coll.push_back( base );
			}
		}

		gContentDb.ReleaseTemplate( base );
	}
	else
	{
		GoDataTemplate::ChildIter i, begin = base->GetChildBegin(), end = base->GetChildEnd();
		for ( i = begin ; i != end ; ++i )
		{
			RecurseGetAllHeroes( *i, coll );
		}
	}
}

int ContentDb :: GetAllHeroes( TemplateColl& coll )
{
	size_t oldSize = coll.size();

	const GoDataTemplate* heroes = FindTemplateByName( "hero" );
	if ( heroes != NULL )
	{
		RecurseGetAllHeroes( heroes, coll );
	}

	return ( coll.size() - oldSize );
}

int ContentDb :: GetAllHeroes( GoidColl& coll )
{
	TemplateColl templates;
	int diff = GetAllHeroes( templates );

	TemplateColl::iterator i, ibegin = templates.begin(), iend = templates.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		coll.push_back( gGoDb.FindCloneSource( (*i)->GetName() ) );
	}

	return ( diff );
}

const GoDataTemplate* ContentDb :: GetRandomHero( void )
{
	const GoDataTemplate* godt = NULL;

	TemplateColl templates;
	if ( GetAllHeroes( templates ) )
	{
		TemplateColl::iterator i = templates.begin();
		std::advance( i, Random( templates.size() - 1 ) );
		godt = *i;
	}

	return ( godt );
}

const char* ContentDb :: GetRandomHeroName( void )
{
	const GoDataTemplate* godt = GetRandomHero();
	return ( godt ? godt->GetName() : NULL );
}

const gpstring& ContentDb :: GetBootSubTypeForArmorBodyType( const gpstring& abt ) const
{
	ArmorSubTypeDb::const_iterator found = m_ArmorSubTypeDb.find( abt );
	if ( found != m_ArmorSubTypeDb.end() )
	{
		return ( found->second.first );
	}

	return ( gpstring::EMPTY );
}

const gpstring& ContentDb :: GetGauntletSubTypeForArmorBodyType( const gpstring& abt ) const
{
	ArmorSubTypeDb::const_iterator found = m_ArmorSubTypeDb.find( abt );
	if ( found != m_ArmorSubTypeDb.end() )
	{
		return ( found->second.second );
	}

	return ( gpstring::EMPTY );
}

const gpstring& ContentDb :: GetGuiWeaponSpeedString( const float speed ) const
{
	SpeedRangeNameDb::const_iterator i;
	for ( i = m_SpeedRangeNameDb.begin(); i != m_SpeedRangeNameDb.end(); ++i )
	{
		if ( speed <= (*i).maxRange && speed >= (*i).minRange )
		{
			return (*i).sScreenName;
		}
	}

	return ( gpstring::EMPTY );
}

static kerneltool::Critical s_FormulaCritical;

template <typename T>
T EvalGenericFormula( Skrit::Object* object, const char* formula, T defValue )
{
	gpassert( formula != NULL );

	// $ early bailout if no skrit
	if ( object == NULL )
	{
		gperror( "Error: attempted to evaluate a formula but none found for ContentDb! Check formulas.gas.\n" );
		return ( defValue );
	}

	// $ early bailout on bad request
	if ( *formula == '\0' )
	{
		gperror( "Error executing formula request because request is empty!\n" );
		return ( defValue );
	}

	// have to lock access to the formula as multiple threads may access at once
	kerneltool::Critical::Lock locker( s_FormulaCritical );

	// attempt to set properties
	const char* paramStart = ::strchr( formula, '?' );
	if ( paramStart != NULL )
	{
		// $ early bailout if couldn't convert to properties
		if ( !object->ReadRecord( paramStart + 1 ) )
		{
			gperrorf(( "Error parsing or setting properties for formula request '%s'\n", formula ));
			return ( defValue );
		}

		// find end - there may be w/s in the way
		const char* nameEnd = paramStart;
		stringtool::RemoveTrailingWhiteSpace( formula, nameEnd );

		// take name
		int len = nameEnd - formula;
		char* localFormula = (char*)::_alloca( len + 1 );
		::memcpy( localFormula, formula, len );
		localFormula[ len ] = '\0';
		formula = localFormula;
	}

	// find the function
	int funcIndex = object->FindFunction( formula );
	if ( funcIndex == Skrit::INVALID_FUNCTION_INDEX )
	{
		// $ early bailout if invalid function
		gperrorf(( "Error running formula request '%s' - function does not exist in formulas.gas!\n", formula ));
		return ( defValue );
	}

	// check return type match
	FuBi::eVarType rtype = FuBi::Traits <T>::GetVarType();
	FuBi::eVarType ftype = object->GetImpl()->GetFunctionSpec( funcIndex )->m_ReturnType;
	if ( rtype != ftype )
	{
		// $ early bailout if no match
		gperrorf(( "Error running formula request '%s' - asking for object of type %s but function returns type %s\n",
				   formula, gFuBiSysExports.FindTypeName( rtype ), gFuBiSysExports.FindTypeName( ftype ) ));
		return ( defValue );
	}

	// execute it
	Skrit::Result rc = object->Call( funcIndex );
	if ( rc.IsError() )
	{
		// $ early bailout, failed!
		gperrorf(( "Error running formula request '%s' - function returned an error!\n", formula ));
		return ( defValue );
	}

	// cool, return result
	return ( *rcast <T*> ( &rc.m_Raw ) );
}

float ContentDb :: EvalFloatFormula( const char* formula, float defValue )
{
	return ( EvalGenericFormula( m_Formulas, formula, defValue ) );
}

int ContentDb :: EvalIntFormula( const char* formula, int defValue )
{
	return ( EvalGenericFormula( m_Formulas, formula, defValue ) );
}

bool ContentDb :: EvalBoolFormula( const char* formula, bool defValue )
{
	return ( EvalGenericFormula( m_Formulas, formula, defValue ) );
}

template <typename T>
T GetTemplateProperty( const char* templatePath, const T& defValue )
{
	gpassert( templatePath != NULL );

	T value = defValue;

	stringtool::Extractor extractor( ":", templatePath );
	gpstring next;
	const GoDataTemplate* godt = NULL;
	if ( extractor.GetNextString( next ) && ((godt = gContentDb.AcquireTemplateByName( next )) != NULL) )
	{
		const GoDataComponent* godc = NULL;
		if ( extractor.GetNextString( next ) && ((godc = godt->FindComponentByName( next )) != NULL) )
		{
			if ( !extractor.GetNextString( next ) || !godc->GetRecord()->Get( next, value ) )
			{
				gperrorf(( "Field '%s' not found or could not be converted in ContentDb template property query '%s'\n", next.c_str(), templatePath ));
			}
		}
		else
		{
			gperrorf(( "Component '%s' not found in ContentDb template property query '%s'\n", next.c_str(), templatePath ));
		}

		if ( godt != NULL )
		{
			gContentDb.ReleaseTemplate( godt );
		}
	}
	else
	{
		gperrorf(( "Template '%s' not found in ContentDb template property query '%s'\n", next.c_str(), templatePath ));
	}

	return ( value );
}

int ContentDb :: GetTemplateInt( const char* templatePath, int defInt )
{
	return ( GetTemplateProperty( templatePath, defInt ) );
}

bool ContentDb :: GetTemplateBool( const char* templatePath, bool defBool )
{
	return ( GetTemplateProperty( templatePath, defBool ) );
}

float ContentDb :: GetTemplateFloat( const char* templatePath, float defFloat )
{
	return ( GetTemplateProperty( templatePath, defFloat ) );
}

const gpstring& ContentDb :: GetTemplateString( const char* templatePath, const gpstring& defString )
{
	static gpstring s_String;
	s_String = GetTemplateProperty( templatePath, defString );
	return ( s_String );
}

Scid ContentDb :: GetTemplateScid( const char* templatePath, Scid defScid )
{
	return ( GetTemplateProperty( templatePath, defScid ) );
}

#if !GP_RETAIL

bool ContentDb :: AddConstraint( FastFuelHandle fuel )
{
	// take string
	const gpstring& name = AddString( fuel.GetName() );

	// build entry
	ConstraintEntry constraintEntry;
	constraintEntry.m_Fuel = fuel;

	// build string choice constraint
	FuBi::ConstrainStaticStrings* constraint = new FuBi::ConstrainStaticStrings;
	constraintEntry.m_Constraint = constraint;

	// choice type?
	FastFuelHandle choose = fuel.GetChildNamed( "choose" );
	bool success = false;
	if ( choose )
	{
		FastFuelFindHandle fh( choose );
		// fill with basic entries
		if ( fh.FindFirstValue( "*" ) )
		{
			gpstring entry;
			while ( fh.GetNextValue( entry ) )
			{
				constraint->AddStaticEntry( entry );
				success = true;
			}
		}

		// fill with advanced entries
		FastFuelHandleColl constraintList;
		choose.ListChildren( constraintList );
		FastFuelHandleColl::iterator i, ibegin = constraintList.begin(), iend = constraintList.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpstring docs;
			(*i).Get( "doc", docs );
			constraint->AddStaticEntry( FuBi::ConstrainStrings::Entry( (*i).GetName(), docs ) );
			success = true;
		}
	}

	// skrit type?
	// $$$ fill this in - load up

	// fail on no entries
	if ( success )
	{
		// check for dup
		std::pair <ConstraintDb::iterator, bool> rc = m_ConstraintDb.insert( std::make_pair( name, constraintEntry ) );
		if ( !rc.second )
		{
			gperrorf(( "Duplicate constraint '%s' detected! First found at '%s', second found at '%s'\n",
						   name.c_str(), rc.first->second.m_Fuel.GetAddress().c_str(), fuel.GetAddress().c_str() ));
			success = false;
		}
	}

	// done
	return ( success );
}

#endif // !GP_RETAIL

bool ContentDb :: AddComponent( FastFuelHandle fuel )
{
	// get some vars
	const gpstring& name = AddString( fuel.GetName() );
	bool internal = fuel.GetBool( "internal_component", false );

	// does a factory exist for it?
	GocFactoryDb::const_iterator found = m_GocFactoryDb.end();
	if ( !internal )
	{
		found = m_GocFactoryDb.find( name );
		if ( found == m_GocFactoryDb.end() )
		{
			// no? can't construct it then (probably dev-only)
			return ( false );
		}
	}

	// insert new component
	std::pair <DataComponentDb::iterator, bool> rc =
		m_DataComponentDb.insert( DataComponentDb::value_type( name, ComponentEntry() )  );

	// check for duplicate
	if ( !rc.second )
	{
		gperrorf(( "Duplicate component detected: '%s' is already in content database\n", name.c_str() ));
		return ( false );
	}

	// only add a real component if it's not internal
	if ( !internal )
	{
		ComponentEntry& entry = rc.first->second;

		// get the flag out
		bool serverOnly = found->second.m_ServerOnly;
		bool canServerExistOnly = found->second.m_CanServerExistOnly;

		// create header spec
		entry.m_StoreHeaderSpec = new FuBi::StoreHeaderSpec;

		// add it
		std::auto_ptr <GoDataComponent> newComponent( new GoDataComponent );
		if ( newComponent->InitSchema(
				fuel,
				serverOnly,
				canServerExistOnly,
				entry.m_StoreHeaderSpec,
				rcast <GoDataComponent::FlagColl*> ( &entry.m_DwordColl ) ) )
		{
			entry.m_DataComponent = newComponent.release();
		}
		else
		{
			// $ leave it as null - that's the "internal" marker

			gperrorf(( "Failure to add component '%s' to content database, marking as internal instead\n", name.c_str() ));
			return ( false );
		}
	}

	// done
	return ( true );
}

GoComponent* SkritComponentFactory( const char* /*name*/, Go* parent )
{
	return ( new GoSkritComponent( parent ) );
}

bool ContentDb :: AddComponent( Skrit::HObject skrit, const gpstring& name )
{
	// insert new component
	std::pair <DataComponentDb::iterator, bool> rc =
		m_DataComponentDb.insert( DataComponentDb::value_type( name, ComponentEntry() )  );

	// check for duplicate
	if ( !rc.second )
	{
		gperrorf(( "Duplicate component detected: '%s' is already in content database\n", name.c_str() ));
		return ( false );
	}

	ComponentEntry& entry = rc.first->second;

	// create header spec
	entry.m_StoreHeaderSpec = new FuBi::StoreHeaderSpec;

	// add it
	std::auto_ptr <GoDataComponent> newComponent( new GoDataComponent );
	bool serverOnly = false;
	if ( newComponent->InitSchema(
			skrit,
			serverOnly,
			entry.m_StoreHeaderSpec,
			rcast <GoDataComponent::FlagColl*> ( &entry.m_DwordColl ) ) )
	{
		// store in db
		entry.m_DataComponent = newComponent.release();

		// add a factory for it
		RegisterGoComponentFactory( name, makeFunctor( SkritComponentFactory ), serverOnly, serverOnly );
	}
	else
	{
		// $ leave it as null - that's the "internal" marker

		gperrorf(( "Failure to add component '%s' to content database, marking as internal instead\n", name.c_str() ));
		return ( false );
	}

	// done
	return ( true );
}

bool ContentDb :: AddTemplate( FastFuelHandle fuel )
{
	// take string
	const gpstring& name = AddString( fuel.GetName() );

	// insert new template
	std::pair <DataTemplateDb::iterator, bool> rc =
		m_DataTemplateDb.insert( DataTemplateDb::value_type( name, NULL )  );

	// check for duplicate
	if ( !rc.second )
	{
		gperrorf(( "Duplicate template detected: '%s' is already in content database\n", name.c_str() ));
		return ( false );
	}

	// add it
	rc.first->second = new GoDataTemplate( fuel );

	// add possible components to the set
	FastFuelHandleColl components;
	fuel.ListChildrenTyped( components, "" );
	FastFuelHandleColl::iterator i, ibegin = components.begin(), iend = components.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		m_ComponentNameDb.insert( i->GetName() );
	}

	// done
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class ContentDb::ConstraintEntry implementation

#if !GP_RETAIL

ContentDb::ConstraintEntry :: ConstraintEntry( const ConstraintEntry& obj )
	: m_Fuel( obj.m_Fuel )
{
	m_Constraint = obj.m_Constraint ? obj.m_Constraint->Clone() : NULL;
}

ContentDb::ConstraintEntry :: ~ConstraintEntry( void )
{
	delete ( m_Constraint );
}

ContentDb::ConstraintEntry& ContentDb::ConstraintEntry :: operator = ( const ConstraintEntry& obj )
{
	if ( this != &obj )
	{
		m_Fuel = obj.m_Fuel;
		delete ( m_Constraint );
		m_Constraint = obj.m_Constraint ? obj.m_Constraint->Clone() : NULL;
	}
	return ( *this );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
