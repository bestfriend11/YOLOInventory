//////////////////////////////////////////////////////////////////////////////
//
// File     :  PContentDb.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "PContentDb.h"

#include "AiQuery.h"
#include "ContentDb.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoBody.h"
#include "GoData.h"
#include "GoDefend.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GpConsole.h"
#include "GuiHelper.h"
#include "LocHelp.h"
#include "ReportSys.h"
#include "SkritEngine.h"
#include "Siege_Engine.h"
#include "Siege_Loader.h"
#include "Siege_Mouse_Shadow.h"
#include "Ui_Shell.h"

//////////////////////////////////////////////////////////////////////////////
// macros

// $ i keep forgetting to use the gocreate rng, so this will keep it real! -sb

#define Random GetGoCreateRng().Random
#define GetSeed GetGoCreateRng().GetSeed
#define SetSeed GetGoCreateRng().SetSeed

//////////////////////////////////////////////////////////////////////////////
// class GpcAdd declaration

#if !GP_RETAIL

class GpcAdd : public GpConsole_command, public Singleton <GpcAdd>
{
public:
	SET_INHERITED( GpcAdd, GpConsole_command );

	enum eAddType
	{
		ADD_TO_GROUND,
		ADD_TO_PARTY,
		ADD_TO_SELECTED,
		ADD_TO_MOUSEHOVER,
		EQUIP_TO_SELECTED,
		EQUIP_TO_MOUSEHOVER,
	};

	GpcAdd( void );
	virtual ~GpcAdd( void )					{  }

	UINT32       GetFlags       ( void )	{  return ( 0 );  }
	gpstring     GetLocation    ( void )	{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )	{  return ( 2 );  }
	unsigned int GetMaxArguments( void )	{  return ( 4 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& output );

	bool Add( const gpstring& pattern, eAddType addType, bool useFirst, Goid* dstGoid = NULL );

private:
	typedef stdx::fast_vector <gpstring> StringColl;

	bool       m_Buffering;
	StringColl m_BufferedAdds;

	SET_NO_COPYING( GpcAdd );
};

GpcAdd :: GpcAdd( void )
{
	m_Buffering = false;

	{
		gpstring help;
		std::vector< gpstring > usage;
		help += "add - load template content at cursor world location\n";
		help += "add [p] <template_name>\n";
		help += "	  p = make member of current screen player's party.\n";
		help += "add <begin|end> [type] [param] bracket adds with these to scatter new items\n";
		help += "	  type can be spew or array\n";
		help += "	  if array, then the param is the distance between items\n";

		gGpConsole.RegisterCommand( *this, "add", help, usage );
	}

	{
		gpstring help;
		std::vector< gpstring > usage;
		help += "addf - load first matching template content at cursor world location\n";
		help += "addf [p] <template_name>\n";
		help += "	  p = make member of current screen player's party.\n";

		gGpConsole.RegisterCommand( *this, "addf", help, usage );
	}
}

bool GpcAdd :: Execute( GpConsole_command_arguments& args, gpstring& /*output*/ )
{
	// get params
	bool useFirst = args.GetAsString( 0 ).same_no_case( "addf" );
	bool addToParty = args.GetAsString( 1 ).same_no_case( "p" );
	gpstring pattern = args.GetAsString( addToParty ? 2 : 1 );
	gpstring spewType = args.GetAsString( addToParty ? 3 : 2 );
	gpstring spewParam = args.GetAsString( addToParty ? 4 : 3 );
	bool success = true;

	if ( pattern.same_no_case( "begin" ) )
	{
		m_Buffering = true;
	}
	else if ( pattern.same_no_case( "end" ) )
	{
		m_Buffering = false;

		GoidColl goids;

	// Create them.

		if ( IsServerLocal() )
		{
			if ( gSiegeEngine.GetMouseShadow().IsTerrainHit() )
			{
				StringColl::const_iterator i, ibegin = m_BufferedAdds.begin(), iend = m_BufferedAdds.end();
				for ( i = ibegin ; i != iend ; ++i )
				{
					Goid goid = GOID_INVALID;
					if ( Add( *i, addToParty ? ADD_TO_PARTY : ADD_TO_GROUND, useFirst, &goid ) )
					{
						gpassert( goid != GOID_INVALID );
						goids.push_back( goid );
					}
				}
			}
			else
			{
				gperrorf(( "Load failed - mouse cursor is not over valid floor.\n" ));
				success = false;
			}
		}
		else
		{
			gperrorf(( "This command only available from the server!\n" ));
		}

		m_BufferedAdds.clear();

		// where to spread 'em
		SiegePos source = gSiegeEngine.GetMouseShadow().GetFloorHitPos();

	// Now arrange them.

		if ( spewType.empty() || spewType.same_no_case( "spew" ) )
		{
			PContentDb::PlaceObjectsInSpew( source, goids );
		}
		else
		{
			float dist;
			::FromString( spewParam, dist );

			PContentDb::PlaceObjectsInArray( source, goids, dist );
		}
	}
	else
	{
		success = Add( pattern, addToParty ? ADD_TO_PARTY : ADD_TO_GROUND, useFirst );
	}

	return ( success );
}

bool GpcAdd :: Add( const gpstring& pattern, eAddType addType, bool useFirst, Goid* dstGoid )
{
	// get some vars
	TemplateColl matches;
	int gold = 0;
	gpstring pcontent;
	bool success = true;

	// check for bad things
	bool isGiving = (addType == ADD_TO_SELECTED) || (addType == ADD_TO_MOUSEHOVER);
	bool isEquipping = (addType == EQUIP_TO_SELECTED) || (addType == EQUIP_TO_MOUSEHOVER);
	bool isBuffering = m_Buffering && !isGiving && !isEquipping;

	// special: look for gold
	if ( pattern.same_no_case( "gold:", 5 ) )
	{
		if ( isBuffering )
		{
			m_BufferedAdds.push_back( pattern );
			return ( true );
		}

		const char* goldStr = pattern.c_str() + 5;
		if ( stringtool::Get( goldStr, gold ) && (gold > 0) )
		{
			const GoDataTemplate* data = gContentDb.FindTemplateByName( gContentDb.GetDefaultGoldTemplate() );
			if ( data != NULL )
			{
				matches.push_back( data );
			}
		}
		else
		{
			gperrorf(( "Value '%s' is not a valid gold amount\n", goldStr ));
			return ( false );
		}
	}
	else if ( pattern[ 0 ] == '#' )
	{
		if ( isBuffering )
		{
			m_BufferedAdds.push_back( pattern );
			return ( true );
		}

		pcontent = pattern;
	}
	else
	{
		if ( pattern[ 0 ] == '\"' )
		{
			gpstring localPattern( pattern, 1, gpstring::npos );
			if ( !localPattern.empty() && (*localPattern.rbegin() == '\"') )
			{
				localPattern.erase( localPattern.length() - 1 );
			}

			// retrieve templates that match the pattern
			gContentDb.FindTemplatesByDocWildcard( matches, localPattern );
		}
		else
		{
			// retrieve templates that match the pattern
			gContentDb.FindTemplatesByWildcard( matches, pattern );
		}

		if ( isBuffering )
		{
			TemplateColl::const_iterator i, ibegin = matches.begin(), iend = matches.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				m_BufferedAdds.push_back( (*i)->GetName() );
			}

			return ( true );
		}
	}

	// if more than one and we want to use the first, just take it
	if ( useFirst && (matches.size() > 1) )
	{
		matches.erase( matches.begin() + 1, matches.end() );
	}

	// more than one match?
	if ( matches.size() > 200 )
	{
		gpgenericf((
				"%d matches found for pattern '%s'. That's too many to print, narrow down the search...\n",
				matches.size(), pattern.c_str() ));
		success = false;
	}
	else if ( matches.size() > 1 )
	{
		gpgenericf(( "%d matches found for pattern '%s':\n\n",
					 matches.size(), pattern.c_str() ));
		gpstring str;

		// dump objects into sink
		ReportSys::TextTableSink tableSink;
		ReportSys::LocalContext tableCtx( &tableSink, false );
		tableCtx.SetSchema( new ReportSys::Schema( 4, "template_name", "screen_name", "category_name", "doc" ), true );

		// fill table
		TemplateColl::const_iterator i, begin = matches.begin(), end = matches.end();
		for ( i = begin ; i != end ; ++i )
		{
			ReportSys::AutoReport autoReport( tableCtx );
			AutoRefTemplate autoRef( *i );

			tableCtx.OutputField( (*i)->GetName() );

			const GoDataComponent* common = (*i)->FindComponentByName( "common" );
			if ( common != NULL )
			{
				common->GetRecord()->Get( "screen_name", str );
				tableCtx.OutputField( str );
			}
			else
			{
				tableCtx.OutputField( "<?>" );
			}

			tableCtx.OutputField( (*i)->GetCategoryName() );
			tableCtx.OutputField( (*i)->GetDocs() );
		}

		// out header
		gGenericContext.Output( "\n" );
		gGenericContext.OutputRepeatWidth( tableSink.CalcDividerWidth(), "-==- " );
		gGenericContext.Output( "\n\nTemplates:\n\n" );

		// out table
		tableSink.Sort();
		tableSink.OutputReport( gGenericContext );
	}
	else if ( !matches.empty() || !pcontent.empty() )
	{
		gpstring match = matches.empty() ? pcontent : matches.front()->GetName();

		GopColl giveToColl;

		if ( (addType == ADD_TO_SELECTED) || (addType == EQUIP_TO_SELECTED) )
		{
			GopColl::const_iterator i, ibegin = gGoDb.GetSelectionBegin(), iend = gGoDb.GetSelectionEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( (*i)->IsScreenPartyMember() )
				{
					giveToColl.push_back( *i );
				}
			}

			success = !giveToColl.empty();
			if ( !success )
			{
				gpgenericf(( "%s failed - no party members selected.\n", isGiving ? "Give" : "Equip" ));
			}
		}
		else if ( (addType == ADD_TO_MOUSEHOVER) || (addType == EQUIP_TO_MOUSEHOVER) )
		{
			GopColl::const_iterator i, ibegin = gGoDb.GetMouseShadowedGosBegin(), iend = gGoDb.GetMouseShadowedGosEnd();
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( (*i)->HasInventory() )
				{
					giveToColl.push_back( *i );
				}
			}

			success = !giveToColl.empty();
			if ( !success )
			{
				gpgenericf(( "%s failed - no go's with inventories selected.\n", isGiving ? "Give" : "Equip" ));
			}
		}
		else
		{
			success = gSiegeEngine.GetMouseShadow().IsTerrainHit();
			if ( !success )
			{
				gpgenericf(( "Add failed - mouse cursor is not over valid floor.\n" ));
			}
		}

		if ( success )
		{
			// set hit pos
			GoCloneReq cloneReq;
			cloneReq.m_FadeIn = true;
			if ( !isGiving && !isEquipping )
			{
				cloneReq.SetStartingPos( gSiegeEngine.GetMouseShadow().GetFloorHitPos() );
			}
			else if ( !giveToColl.empty() )
			{
				cloneReq.SetStartingPos( (*giveToColl.begin())->GetPlacement()->GetPosition() );
			}

			// figure out parent and player
			Go* party = NULL;
			if ( (addType == ADD_TO_PARTY) && ((party = gServer.GetScreenParty()) != NULL) )
			{
				cloneReq.m_Parent = party->GetGoid();
				cloneReq.m_AllClients = true;
			}
			else
			{
				cloneReq.m_PlayerId = PLAYERID_COMPUTER;
			}

			Go* newGo = NULL;

			// special: creation of gold
			if ( gold > 0 )
			{
				if ( isGiving || isEquipping )
				{
					GopColl::iterator i, ibegin = giveToColl.begin(), iend = giveToColl.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						(*i)->GetInventory()->RSAddGold( gold );
					}
				}
				else if ( IsServerLocal() )
				{
					// add it (must succeed)
					GoHandle goldGo( gGoDb.SCloneGo( cloneReq, match ) );
					if ( goldGo )
					{
						goldGo->GetAspect()->RSSetGoldValue( gold );
						newGo = goldGo;
						gpgenericf(( "Added '%s' x%d\n", match.c_str(), gold ));
					}

					// update caller
					if ( dstGoid != NULL )
					{
						*dstGoid = goldGo->GetGoid();
					}
				}
				else
				{
					gpwarning( "Sorry, only the server may create and add gold currently.\n" );
				}
			}
			else
			{
				if ( IsServerLocal() && (isGiving || isEquipping) )
				{
					GopColl::iterator i, ibegin = giveToColl.begin(), iend = giveToColl.end();
					for ( i = ibegin ; i != iend ; ++i )
					{
						cloneReq.m_AllClients = (*i)->IsAllClients();
						
						Goid goid = gGoDb.SCloneGo( cloneReq, match );
						GoHandle go( goid );
						if ( go )
						{
							newGo = go;

							if ( go->HasGui() )
							{
								if ( isEquipping )
								{
									(*i)->GetInventory()->RSAutoEquip( ES_ANY, goid, AO_USER, true );
								}
								else
								{
									(*i)->GetInventory()->SAutoGet( goid, AO_REFLEX );
								}
							}
							else
							{
								gpwarningf(( "Requested go does not have a [gui] component and cannot be stored in inventory. Dropping on ground instead...\n" ));
							}
						}
					}
				}
				else
				{
					// add it (this is only a request, it may not succeed)
					if ( (dstGoid != NULL) && IsServerLocal() )
					{
						*dstGoid = gGoDb.SCloneGo( cloneReq, match );
						newGo = GoHandle( *dstGoid );
					}
					else if ( IsServerLocal() )
					{
						Goid newGoid = gGoDb.SCloneGo( cloneReq, match );
						newGo = GoHandle( newGoid );
					}
					else
					{
						gGoDb.RSCloneGo( cloneReq, match );
					}
				}

				// notify user
				gpgenericf(( "Requested to %s '%s'\n", isGiving ? "give" : (isEquipping ? "equip" : "add"), match.c_str() ));
			}

			// commit any textures
			GoAspect* newAspect = newGo ? newGo->QueryAspect() : NULL;
			if ( newAspect != NULL )
			{
				newAspect->GetAspectPtr()->Reconstitute( 0 );
				gSiegeLoadMgr.Commit( siege::LOADER_TEXTURE_TYPE );
			}
		}
	}
	else
	{
		gpgenericf(( "No matches for pattern '%s' in template database.\n", pattern.c_str() ));
		success = false;
	}

	return ( success );
}

#define gGpcAdd GpcAdd::GetSingleton()

class GpcGive : public GpConsole_command
{
public:
	SET_INHERITED( GpcGive, GpConsole_command );

	GpcGive( void )
	{
		{
			gpstring help;
			std::vector< gpstring > usage;
			help += "give - load template content at cursor world location and give to all selected party members\n";
			help += "give [h] <template_name>\n";
			help += "	h = add to the current mouse hover selection (rather than selected party members)\n";

			gGpConsole.RegisterCommand( *this, "give", help, usage );
		}

		{
			gpstring help;
			std::vector< gpstring > usage;
			help += "givef - load first matching template content at cursor world location and give to all selected party members\n";
			help += "givef [h] <template_name>\n";
			help += "	h = add to the current mouse hover selection (rather than selected party members)\n";

			gGpConsole.RegisterCommand( *this, "givef", help, usage );
		}

		{
			gpstring help;
			std::vector< gpstring > usage;
			help += "equip - load template content at cursor world location and equip on all selected party members\n";
			help += "equip [h] <template_name>\n";
			help += "	h = equip to the current mouse hover selection (rather than selected party members)\n";

			gGpConsole.RegisterCommand( *this, "equip", help, usage );
		}

		{
			gpstring help;
			std::vector< gpstring > usage;
			help += "equipf - load first matching template content at cursor world location and equip on all selected party members\n";
			help += "equipf [h] <template_name>\n";
			help += "	h = equip to the current mouse hover selection (rather than selected party members)\n";

			gGpConsole.RegisterCommand( *this, "equipf", help, usage );
		}
	}

	virtual ~GpcGive( void )				{  }

	UINT32       GetFlags       ( void )	{  return ( 0 );  }
	gpstring     GetLocation    ( void )	{  return ( __FILE__ );  }
	unsigned int GetMinArguments( void )	{  return ( 2 );  }
	unsigned int GetMaxArguments( void )	{  return ( 3 );  }

	bool Execute( GpConsole_command_arguments& args, gpstring& /*output*/ )
	{
		CHECK_SERVER_ONLY;

		bool addToHover = args.GetAsString( 1 ).same_no_case( "h" );
		gpstring pattern = args.GetAsString( addToHover ? 2 : 1 );

		GpcAdd::eAddType addType;
		bool useFirst = false;
		if ( args.GetAsString( 0 ).same_no_case( "give" ) )
		{
			addType = addToHover ? GpcAdd::ADD_TO_MOUSEHOVER : GpcAdd::ADD_TO_SELECTED;
		}
		else if ( args.GetAsString( 0 ).same_no_case( "givef" ) )
		{
			addType = addToHover ? GpcAdd::ADD_TO_MOUSEHOVER : GpcAdd::ADD_TO_SELECTED;
			useFirst = true;
		}
		else if ( args.GetAsString( 0 ).same_no_case( "equip" ) )
		{
			addType = addToHover ? GpcAdd::EQUIP_TO_MOUSEHOVER : GpcAdd::EQUIP_TO_SELECTED;
		}
		else
		{
			addType = addToHover ? GpcAdd::EQUIP_TO_MOUSEHOVER : GpcAdd::EQUIP_TO_SELECTED;
			useFirst = true;
		}

		return ( gGpcAdd.Add( pattern, addType, useFirst ) );
	}

private:
	SET_NO_COPYING( GpcGive );
};

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// private helper methods

enum eFoundType
{
	FOUND_FAILURE,			// explicit failure
	FOUND_ILLEGAL,			// illegal query
	FOUND_EMPTY,			// empty selection set
	FOUND_OK,				// found something ok
	FOUND_LOW,				// found something but it was below the low end
	FOUND_HIGH,				// found something but it was above the high end
	FOUND_TOO_LOW,			// found something ok but the chooser said the query was too low for it
	FOUND_TOO_HIGH,			// found something ok but the chooser said the query was too high for it
};

#if !GP_RETAIL

static const char* s_FoundStrings[] =
{
	"FOUND_FAILURE",
	"FOUND_ILLEGAL",
	"FOUND_EMPTY",
	"FOUND_OK",
	"FOUND_LOW",
	"FOUND_HIGH",
	"FOUND_TOO_LOW",
	"FOUND_TOO_HIGH",
};

const char* ToString( eFoundType x )
{
	return ( s_FoundStrings[ x ] );
}

#endif // !GP_RETAIL

inline bool IsFound( eFoundType found )
{
	return ( (found != FOUND_ILLEGAL) && (found != FOUND_EMPTY) );
}

struct RandomFilter
{
	template <typename ITER>
	eFoundType Choose( ITER& out, ITER upper )
	{
		std::advance( out, Random( std::distance( out, upper ) - 1 ) );
		return ( FOUND_OK );
	}
};

struct ItemFilter
{
	const PContentReq& m_Req;
	float              m_TotalModifierPower;
	bool               m_MatchArmor;
	bool               m_MatchFinish;

	ItemFilter( const PContentReq& req, float totalModifierPower )
		: m_Req( req )
		, m_TotalModifierPower( totalModifierPower )
	{
		m_MatchArmor  = false;
		m_MatchFinish = !req.GetFinish().empty();

		if ( req.GetBasicType() == PT_ARMOR )
		{
			if ( !m_Req.GetArmorMaterial().empty() || !m_Req.GetArmorSkillType().empty() )
			{
				m_MatchArmor = true;
			}
		}
	}

	template <typename ITER>
	bool IsMatch( const ITER& i, eFoundType& found )
	{
		const PContentDb::ItemEntry& itemEntry = i->second;

		if ( !PContentDb::IsCompatible( itemEntry.m_DbType, m_Req.GetDbType() ) )
		{
			return ( false );
		}

		if ( m_TotalModifierPower >= 0 )
		{
			if ( m_TotalModifierPower < itemEntry.m_ModifierMin )
			{
				found = FOUND_TOO_LOW;
				return ( false );
			}

			if ( m_TotalModifierPower > itemEntry.m_ModifierMax )
			{
				found = FOUND_TOO_HIGH;
				return ( false );
			}

			if ( m_Req.GetReqModifierCount() < itemEntry.m_ModifierCountMin )
			{
				return ( false );
			}

			if ( m_Req.GetReqModifierCount() > itemEntry.m_ModifierCountMax )
			{
				return ( false );
			}
		}

		if ( m_MatchArmor )
		{
			gpstring material, skill;
			if (   !PContentDb::GetArmorTraitsFromTemplateName( material, skill, itemEntry.m_Template->GetName() )
				|| !(m_Req.GetArmorMaterial ().empty() || m_Req.GetArmorMaterial ().same_no_case( material ) )
				|| !(m_Req.GetArmorSkillType().empty() || m_Req.GetArmorSkillType().same_no_case( skill    ) ) )
			{
				return ( false );
			}
		}

		if ( m_MatchFinish )
		{
			if ( !m_Req.GetFinish().same_no_case( itemEntry.m_Variation ) )
			{
				return ( false );
			}
		}

		return ( true );
	}

	template <typename ITER>
	eFoundType Choose( ITER& out, ITER upper )
	{
		eFoundType found = FOUND_EMPTY;

		stdx::fast_vector <ITER> choices;

		for ( ITER i = out ; i != upper ; ++i )
		{
			if ( IsMatch( i, found ) )
			{
				choices.push_back( i );
			}
		}

		if ( !choices.empty() )
		{
			out = choices[ Random( choices.size() - 1 ) ];
			found = FOUND_OK;
		}

		return ( found );
	}
};

struct ModifierFilter
{
	const PContentReq&               m_Req;
	const PContentDb::ModifierEntry* m_ModifierEntry;
	eAttackClass                     m_AttackClass;

	ModifierFilter( const PContentReq& req, const PContentDb::ModifierEntry* modifierEntry )
		: m_Req( req ), m_ModifierEntry( modifierEntry )
	{
		m_AttackClass = AC_BEASTFU;

		if ( (m_Req.m_BasicType == PT_WEAPON) && (m_Req.m_Template != NULL) )
		{
			PContentDb::ItemTraits itemTraits;
			if ( gPContentDb.QueryItemTraits( itemTraits, m_Req.m_Template, PT_WEAPON, true ) )
			{
				m_AttackClass = itemTraits.m_AttackClass;
			}
		}
	}

	template <typename ITER>
	eFoundType Choose( ITER& out, ITER upper )
	{
		eFoundType found = FOUND_EMPTY;

		stdx::fast_vector <ITER> choices;

		for ( ITER i = out ; i != upper ; ++i )
		{
			const PContentDb::ModifierEntry& modifierEntry = i->second->second;

			// first check traits
			if ( !PContentDb::IsCompatible( modifierEntry.GetDbType(), m_Req.GetDbType() ) )
			{
				continue;
			}

			// last check modifier-modifier exclusions
			if ( (m_ModifierEntry == NULL) || m_ModifierEntry->CanCoexistWith( &modifierEntry ) )
			{
				// then check against template or type exclusions
				if ( i->second->second.CanModify( m_Req.m_Template, m_Req.m_SpecificType, m_AttackClass ) )
				{
					choices.push_back( i );
				}
			}
		}

		if ( !choices.empty() )
		{
			out = choices[ Random( choices.size() - 1 ) ];
			found = FOUND_OK;
		}

		return ( found );
	}
};

template <typename ITER, typename COLL, typename PRED>
eFoundType FindNearest( ITER& out, COLL& coll, float low, float high, float fuzz, PRED pred )
{
	// $ bail early if empty
	if ( coll.empty() )
	{
		return ( FOUND_EMPTY );
	}

	// get ranges
	if ( low == FLOAT_MAX )
	{
		low = coll.rbegin()->first;
	}
	if ( high == FLOAT_MAX )
	{
		high = coll.rbegin()->first;
	}

	// find nearest set within range
	out = coll.lower_bound( low - fuzz );
	ITER upper = coll.upper_bound( high + fuzz );

	// got a valid range?
	eFoundType found = FOUND_OK;
	if ( out != coll.end() )
	{
		// choose one at random
		if ( out != upper )
		{
			found = pred.Choose( out, upper );
		}
		else if ( out != coll.begin() )
		{
			// make sure we've got the *nearest* - lower_bound and upper_bound
			// will both be the same (and above the possible target) if we asked
			// for a range in between two points.
			--out;
			float mid = (low + high) * 0.5f;
			if ( fabsf( out->first - mid ) > fabsf( upper->first - mid ) )
			{
				// it was right the first time, restore it
				++out;
			}

			// now even though we've got the right power level, it may not
			// validate properly (because it can't support the modifier level
			// we're after) so find again using this as the new range within
			// given fuzz level.
			found = FindNearest( out, coll, out->first, out->first, fuzz, pred );
		}
		else
		{
			// ok the requested range was before all of our points, choose
			// random from bottom within fuzz level
			found = FindNearest( out, coll, out->first, out->first, fuzz, pred );
			if ( (found == FOUND_OK) || (found == FOUND_TOO_LOW) || (found == FOUND_TOO_HIGH) )
			{
				found = FOUND_LOW;
			}
		}
	}
	else
	{
		// choose random from top within fuzz level
		--out;
		found = FindNearest( out, coll, out->first, out->first, fuzz, pred );
		if ( (found == FOUND_OK) || (found == FOUND_TOO_LOW) || (found == FOUND_TOO_HIGH) )
		{
			found = FOUND_HIGH;
		}
	}

	return ( found );
}

template <typename ITER, typename COLL>
eFoundType FindNearest( ITER& out, COLL& coll, float low, float high, float fuzz )
{
	return ( FindNearest( out, coll, low, high, fuzz, RandomFilter() ) );
}

template <typename ITER, typename COLL>
eFoundType FindNearest( ITER& out, COLL& coll, float low, float high, float fuzz, const PContentReq& req, float totalModifierPower )
{
	return ( FindNearest( out, coll, low, high, fuzz, ItemFilter( req, totalModifierPower ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// report streams

#if !GP_RETAIL

ReportSys::Context& GetPContentContext( void )
{
	static ReportSys::Context* gPContentContextPtr = NULL;
	if ( gPContentContextPtr == NULL )
	{
		static ReportSys::Context s_PContentContext( "PContent" );
		s_PContentContext.Disable();
		gPContentContextPtr = &s_PContentContext;

		s_PContentContext.AddSink( new ReportSys::LogFileSink <> ( "pcontent.log" ), true );
		s_PContentContext.SetType( "Log" );
	}
	return ( *gPContentContextPtr );
}

ReportSys::Context& GetPContentErrorContext( void )
{
	static ReportSys::Context* gPContentErrorContextPtr = NULL;
	if ( gPContentErrorContextPtr == NULL )
	{
		static ReportSys::Context s_PContentErrorContext( "PContentErrors" );
		s_PContentErrorContext.Disable   ();
		s_PContentErrorContext.SetOptions( ReportSys::Context::OPTION_AUTOREPORT );
		s_PContentErrorContext.SetTraits ( ReportSys::Context::TRAIT_ERROR );
		s_PContentErrorContext.SetType   ( "Log" );
		s_PContentErrorContext.AddSink   ( new ReportSys::CallbackSink( ReportSys::CallbackSink::MakeAssertCallback() ), true );

		gPContentErrorContextPtr = &s_PContentErrorContext;
	}
	return ( *gPContentErrorContextPtr );
}

#endif // !GP_RETAIL

//////////////////////////////////////////////////////////////////////////////
// class PContentDb implementation

static const stringtool::BitEnumStringConverter::Entry s_DbTypeStrings[] =
{
	{  "normal", 0,                      },
	{  "normal", PContentDb::DB_NORMAL,  },
	{  "rare",   PContentDb::DB_RARE,    },
	{  "unique", PContentDb::DB_UNIQUE,  },
};

static stringtool::BitEnumStringConverter
	s_DbTypeConverter( s_DbTypeStrings, ELEMENT_COUNT( s_DbTypeStrings ), PContentDb::DB_NORMAL );

gpstring PContentDb :: ToCommaString( eDbType e )
{
	return ( s_DbTypeConverter.ToFullString( e, ',' ) );
}

bool PContentDb :: FromCommaString( const char* str, eDbType& e )
{
	return ( s_DbTypeConverter.FromFullString( str, rcast <DWORD&> ( e ), ',' ) );
}

PContentDb :: PContentDb( void )
{
	// force construction
	GPDEV_ONLY( GetPContentContext() );

	m_MainFormulas = NULL;
	m_LoaderFormulas = NULL;

#	if !GP_RETAIL
	m_GpcAdd   = new GpcAdd;
	m_GpcGive  = new GpcGive;
#	endif // !GP_RETAIL

	Shutdown();
}

PContentDb :: ~PContentDb( void )
{
	Shutdown();

#	if !GP_RETAIL
	Delete( m_GpcAdd   );
	Delete( m_GpcGive  );
#	endif // !GP_RETAIL
}

bool PContentDb :: Init( void )
{
	// $$$ use "unused param report" and dump set of extra fuel entries

	// clear out old data
	Shutdown();

	gpassert( !m_Initializing );
	GPDEV_ONLY( m_Initializing = true );

// Initialize formulas.

	FastFuelHandle contentDb( "world:contentdb" );
	if ( !contentDb )
	{
		return ( false );
	}

	// get the skrit code
	{
		Skrit::CreateReq createReq( "world/contentdb/pcontent" );
		m_MainFormulas = gSkritEngine.CreateNewObject( createReq );
		if ( m_MainFormulas != NULL )
		{
			FuBi::Record* record = m_MainFormulas->GetRecord();
			if ( record != NULL )
			{
				record->SetXferMode( GP_DEBUG ? FuBi::XMODE_STRICT : FuBi::XMODE_QUIET );
			}

			m_LoaderFormulas = new Skrit::Object( m_MainFormulas->GetImpl(), NULL );
			record = m_LoaderFormulas->GetRecord();
			if ( record != NULL )
			{
				record->SetXferMode( GP_DEBUG ? FuBi::XMODE_STRICT : FuBi::XMODE_QUIET );
			}
		}
	}


	// get skrit formulas
	FastFuelHandle formulas = contentDb.GetChildNamed( "formulas" );
	if ( formulas )
	{
		// get infinite weapons
		FastFuelHandle melee = formulas.GetChildNamed( "infinite_melee_weapons" );
		if ( melee )
		{
			FastFuelFindHandle finder( melee );
			finder.FindFirstValue( "*" );
			gpstring str;
			while ( finder.GetNextValue( str ) )
			{
				m_InfiniteMeleeWeapons.insert( str );
			}
		}

		FastFuelHandle ranged = formulas.GetChildNamed( "infinite_ranged_weapons" );
		if ( ranged )
		{
			FastFuelFindHandle finder( ranged );
			finder.FindFirstValue( "*" );
			gpstring str;
			while ( finder.GetNextValue( str ) )
			{
				m_InfiniteRangedWeapons.insert( str );
			}
		}
	}

	// error on any kind of failure
	if ( (m_MainFormulas == NULL) || (m_LoaderFormulas == NULL) )
	{
		gperror( "Unable to find/compile formulas, pcontent will be disabled!\n" );
	}

// Initialize macros.

	// get macros
	FastFuelHandle macros = contentDb.GetChildNamed( "macros" );
	if ( macros )
	{
		FastFuelFindHandle fh( macros );
		if( fh.FindFirstKeyAndValue() )
		{
			gpstring from, to;
			while ( fh.GetNextKeyAndValue( from, to ) )
			{
				m_MacroDb[ from ] = to;
			}
		}
	}

// Add Loader type.

	struct Loader
	{
		static void Load( PContentDb::TraitTemplateDb& traitDb, ePContentType type )
		{
			const char* baseName = ::ToQueryString( type );

			// get templates $ early bailout if none
			TemplateColl items;
			if ( !gContentDb.FindTemplatesAndChildren( items, baseName ) )
			{
				return;
			}

			// iter templates and add to traits db
			TemplateColl::iterator i, ibegin = items.begin(), iend = items.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				PContentDb::ItemTraits itemTraits;
				if ( gPContentDb.QueryItemTraits( itemTraits, *i, type, false, true ) )
				{
					// grab a new ref
					AutoRefTemplate autoRef( *i );

					// then release the one we got automatically (this is so
					// we don't lose a ref after QIT is done only to need to
					// reacquire it right after on a 'true' condition)
					gContentDb.ReleaseTemplate( *i );

					// get traits name by class
					const char* traits = (type == PT_WEAPON)
									   ? ToQueryString( itemTraits.m_AttackClass )
									   : ToQueryString( itemTraits.m_SpecificType );

					// get db by traits name
					PContentDb::PowerTemplateDb& db = traitDb[ traits ];
					std::pair <float, PContentDb::ItemEntry> baseEntry( itemTraits.m_ItemPower, *i );

					// detect modifiers allowed flag
					if ( itemTraits.m_NoModifiers )
					{
						baseEntry.second.m_ModifierCountMax = 0;
					}

					// detect traits
					baseEntry.second.m_DbType = itemTraits.m_DbType;

					// add variations to db if any
					FastFuelHandle pcontent = (*i)->GetFuelHandle().GetChildNamed( "pcontent" );
					if ( pcontent )
					{
						// get all variations
						FastFuelHandleColl variations;
						pcontent.ListChildren( variations );

						FastFuelHandleColl::iterator j, jbegin = variations.begin(), jend = variations.end();
						for ( j = jbegin ; j != jend ; ++j )
						{
							// variation entry
							std::pair <float, PContentDb::ItemEntry> varEntry( baseEntry );

							// get modifier range
							bool hasModifiers = false;
							if (   (*j).Get( "modifier_min", varEntry.second.m_ModifierMin )
								&& (*j).Get( "modifier_max", varEntry.second.m_ModifierMax ) )
							{
								hasModifiers = true;

								// required modifiers?
								(*j).Get( "modifier_count_min", varEntry.second.m_ModifierCountMin );
								(*j).Get( "modifier_count_max", varEntry.second.m_ModifierCountMax );
							}

							// get forced power
							bool hasForcedPower = false;
							float forcedPower = 0;
							if ( (*j).Get( "force_item_power", forcedPower ) )
							{
								hasForcedPower = true;
							}

							// get special type
							gpstring specialType;
							if ( (*j).Get( "pcontent_special_type", specialType ) && !specialType.empty() )
							{
								if ( !PContentDb::FromCommaString( specialType, varEntry.second.m_DbType ) )
								{
									gperrorf(( "Illegal pcontent_special_type '%s' found at '%s'\n", specialType.c_str(), (*j).GetAddress().c_str() ));
								}
							}

							// is this actually the base type?
							if ( (hasModifiers || hasForcedPower) && same_no_case( (*j).GetName(), "base" ) )
							{
								// update it
								if ( hasModifiers )
								{
									baseEntry.second.m_ModifierMin      = varEntry.second.m_ModifierMin;
									baseEntry.second.m_ModifierMax      = varEntry.second.m_ModifierMax;
									baseEntry.second.m_ModifierCountMin = varEntry.second.m_ModifierCountMin;
									baseEntry.second.m_ModifierCountMax = varEntry.second.m_ModifierCountMax;
									baseEntry.second.m_DbType           = varEntry.second.m_DbType;
								}

								// update forced power if any
								if ( hasForcedPower )
								{
									baseEntry.first = forcedPower;
								}
								else if ( (type == PT_AMULET) || (type == PT_RING) )
								{
									// amulets and rings have no inherent power
									baseEntry.first = 0;
								}
							}
							else if ( hasModifiers )
							{
								float powerMin = 0, powerMax = 0;
								bool calc = true;

								if ( type == PT_WEAPON )
								{
									// get damage
									if (   !(*j).Get( "damage_min", powerMin )
										|| !(*j).Get( "damage_max", powerMax ) )
									{
										powerMin = itemTraits.m_ItemPower;
										calc = false;
									}
								}
								if ( (type == PT_AMULET) || (type == PT_RING) )
								{
									// $ do nothing, amulets and rings have no inherent power
									calc = false;
								}
								else if ( type == PT_ARMOR )
								{
									// get defense
									if ( !(*j).Get( "defense", powerMin ) )
									{
										gperrorf(( "PContent error: item '%s' variation '%s' has missing/invalid defense at '%s'\n",
												   (*i)->GetName(), (*j).GetName(), (*j).GetAddress().c_str() ));
									}
									powerMax = powerMin;
								}

								// update power
								if ( hasForcedPower )
								{
									varEntry.first = forcedPower;
								}
								else
								{
									// calc damage from min/max
									gpassert( type == ToBasicPContentType( itemTraits.m_SpecificType ) );
									if ( calc )
									{
										varEntry.first = gPContentDb.CalcItemPower(
												type,
												itemTraits.m_SpecificType,
												powerMin,
												powerMax,
												itemTraits.m_IsTwoHanded,
												itemTraits.m_ItemSubType,
												itemTraits.m_ItemLength );
									}
									else
									{
										varEntry.first = powerMin;
									}
								}
								varEntry.second.m_Variation = (*j).GetName();

								// add it
								db.insert( varEntry );
							}
							else
							{
								gperrorf(( "PContent error: item '%s' variation '%s' has missing/invalid modifier_min/max at '%s'\n",
										   (*i)->GetName(), (*j).GetName(), (*j).GetAddress().c_str() ));
							}
						}
					}

					// add base type to db
					db.insert( baseEntry );
				}
			}
		}
	};

// Initialize items.

	// do weapon tree
	{
		Loader::Load( m_TraitTemplateDb, PT_WEAPON );

		// postprocess to find all our types
		TraitTemplateDb::const_iterator i, ibegin = m_TraitTemplateDb.begin(), iend = m_TraitTemplateDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			eAttackClass ac;
			if ( ::FromQueryString( i->first, ac ) )
			{
				ePContentType pt = ToPContentType( ac );
				if ( pt == PT_MELEE )
				{
					if ( ac != AC_STAFF )
					{
						m_MeleeTypesNoStaff.push_back( i->first );
					}
				}
				else if ( pt == PT_RANGED )
				{
					m_RangedTypes.push_back( i->first );
				}
			}
		}
	}

	// do armor tree
	{
		Loader::Load( m_TraitTemplateDb, PT_ARMOR );
	}

	// do amulet tree
	{
		Loader::Load( m_TraitTemplateDb, PT_AMULET );
	}

	// do ring tree
	{
		Loader::Load( m_TraitTemplateDb, PT_RING );
	}

	// do spell tree (includes scrolls)
	{
		Loader::Load( m_TraitTemplateDb, PT_SPELL );
	}

	// do potion tree
	{
		Loader::Load( m_TraitTemplateDb, PT_POTION );
	}

	// do spell book tree
	{
		Loader::Load( m_TraitTemplateDb, PT_SPELLBOOK );
	}

// Add modifiers.

	// get modifiers
	FastFuelHandle modifiers = contentDb.GetChildNamed( "modifiers" );
	if ( modifiers )
	{
		// create modifiers
		FastFuelHandleColl modifierFuels;
		modifiers.ListChildren( modifierFuels );
		FastFuelHandleColl::iterator i, ibegin = modifierFuels.begin(), iend = modifierFuels.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			std::pair <ModifierStorageDb::iterator, bool> rc = m_ModifierStorageDb.insert(
					ModifierStorageDb::value_type( (*i).GetName(), ModifierEntry( *i ) ) );
			if ( !rc.second )
			{
				gperrorf(( "Duplicate modifier '%s' detected at fuel '%s'\n",
						   (*i).GetName(), (*i).GetAddress().c_str() ));
			}
		}

		// find specializations
		gpstring base;
		ModifierStorageDb::iterator j, jbegin = m_ModifierStorageDb.begin(), jend = m_ModifierStorageDb.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			base = j->second.m_Fuel.GetString( "specializes" );
			if ( !base.empty() )
			{
				ModifierStorageDb::iterator found = m_ModifierStorageDb.find( base );
				if ( found != m_ModifierStorageDb.end() )
				{
					j->second.m_Base = &found->second;
				}
				else
				{
					gperrorf(( "Specialization of nonexistent modifier '%s' by modifier '%s' found at fuel '%s'\n",
							   base.c_str(), j->second.m_Name.c_str(), j->second.m_Fuel.GetAddress().c_str() ));
				}
			}
		}
	}

// Add builder type.

	// this will perform the recursion and track dependencies
	struct Builder
	{
		typedef std::set <ModifierEntry*> ModSet;

		ModSet m_Modifiers;					// these are modifiers to be processed
		ModSet m_FailedModifiers;			// these are modifiers that failed to specialize
		ModSet m_Recursion;					// watch for cyclic specialization

		bool Init( ModifierEntry* mod )
		{
			// update recursion
			std::pair <ModSet::iterator, bool> rc = m_Recursion.insert( mod );
			if ( !rc.second )
			{
				ReportSys::AutoReport autoReport( &gErrorContext );
				gperror( "Cyclic modifier specialization detected in the set: " );

				ModSet::const_iterator i, begin = m_Recursion.begin(), end = m_Recursion.end();
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
			PContentDb::ModifierEntry* base = mod->GetBase();
			if ( (base != NULL) && (m_Modifiers.find( base ) != m_Modifiers.end()) )
			{
				// init that one - if failed, then mod fails because we
				// can't specialize on a failed modifier.
				success = Init( base );
			}

			// initialize local
			if ( !success || !mod->Init() )
			{
				// add to failed set
				m_FailedModifiers.insert( mod );
				success = false;
			}

			// remove from working set
			m_Modifiers.erase( m_Modifiers.find( mod ) );

			// remove recursion
			m_Recursion.erase( rc.first );

			// done
			return ( success );
		}

		void Init( void )
		{
			while ( !m_Modifiers.empty() )
			{
				Init( *m_Modifiers.begin() );
				gpassert( m_Recursion.empty() );
			}
		}
	};

// Specialize modifiers.

	// build builder
	Builder builder;

	// process actual modifier data (now that specialization is possible)
	{
		// add each to builder set
		ModifierStorageDb::iterator i, begin = m_ModifierStorageDb.begin(), end = m_ModifierStorageDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			builder.m_Modifiers.insert( &i->second );
		}

		// now do it
		builder.Init();
	}

	// process errors
	{
		// add each to builder set
		Builder::ModSet::const_iterator i,
										ibegin = builder.m_FailedModifiers.begin(),
										iend   = builder.m_FailedModifiers.end  ();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// print error
			gperrorf(( "Failure to add modifier '%s' to pcontent database\n",
					   (*i)->m_Name.c_str() ));

			// remove it from set
			ModifierStorageDb::iterator j, jbegin = m_ModifierStorageDb.begin(), jend = m_ModifierStorageDb.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( *i == &j->second )
				{
					m_ModifierStorageDb.erase( j );
					break;
				}
			}

			// make sure we found it
			gpassert( j != jend );
		}
	}

	// store in proper indexes
	{
		ModifierStorageDb::iterator i, begin = m_ModifierStorageDb.begin(), end = m_ModifierStorageDb.end();
		for ( i = begin ; i != end ; ++i )
		{
			// add to index
			switch ( i->second.m_Type )
			{
				case ( ModifierEntry::TYPE_PREFIX ):
				{
					m_ModifierPrefixDb.insert( std::make_pair( i->second.m_Power, i ) );
				}
				break;

				case ( ModifierEntry::TYPE_SUFFIX ):
				{
					m_ModifierSuffixDb.insert( std::make_pair( i->second.m_Power, i ) );
				}
				break;

				default:
				{
					gpassert( 0 );	// $ should never get here
				}
			}

			// update max
			maximize( m_MaxModifierPower, i->second.m_Power );
		}
	}

	// validate exclusions vs. names
#	if !GP_RETAIL
	{
		ModifierStorageDb::const_iterator i, ibegin = m_ModifierStorageDb.begin(), iend = m_ModifierStorageDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			ModifierEntry::StringSet::const_iterator j, jbegin = i->second.m_Exclusions.begin(), jend = i->second.m_Exclusions.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( m_ModifierStorageDb.find( *j ) == m_ModifierStorageDb.end() )
				{
					gperrorf(( "PContent modifier at '%s' excludes modifier '%s' which does not exist!\n",
							   i->second.m_Fuel.GetAddress().c_str(), j->c_str() ));
				}
			}
		}
	}
#	endif // !GP_RETAIL

// Load in the multiplayer loot scaling rules.

	// get modifiers
	FastFuelHandle loot( "world:global:mp_loot_scaling" );
	if ( loot )
	{
		gpstring value, traits, trait;

		loot.Get( "all_chance_multiplier", value );
		stringtool::Extractor( ",", value ).GetFloats( m_MpLootScaling.m_AllChanceMultiplier );
		loot.Get( "oneof_chance_multiplier", value );
		stringtool::Extractor( ",", value ).GetFloats( m_MpLootScaling.m_OneOfChanceMultiplier );

		FastFuelHandleColl minmaxes;
		loot.ListChildrenNamed( minmaxes, "minmax*" );
		FastFuelHandleColl::iterator i, ibegin = minmaxes.begin(), iend = minmaxes.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			i->Get( "traits", traits );
			stringtool::Extractor extractor( ",", traits );
			while ( extractor.GetNextString( trait ) )
			{
				MinMax* minmax = NULL;

				if ( trait.same_no_case( "gold" ) )
				{
					minmax = &m_MpLootScaling.m_MinMaxGold;
				}
				else
				{
					minmax = &*m_MpLootScaling.m_MinMaxColl.push_back();

					if ( !::FromQueryString( trait, minmax->m_AttackClass ) )
					{
						if ( !::FromQueryString( trait, minmax->m_PContentType ) )
						{
							gperrorf(( "Illegal trait name '%s' at fuel '%s'\n", trait.c_str(), loot.GetAddress().c_str() ));
							m_MpLootScaling.m_MinMaxColl.pop_back();
							continue;
						}
					}
				}

				i->Get( "min_multiplier", value );
				stringtool::Extractor( ",", value ).GetFloats( minmax->m_MinCountMultiplier );
				i->Get( "max_multiplier", value );
				stringtool::Extractor( ",", value ).GetFloats( minmax->m_MaxCountMultiplier );
			}
		}
	}

// Load in pcontent name composition order.

	m_bPrefixFirst = true;
	FastFuelHandle hSettings( "config:global_settings:locale" );
	if ( hSettings.IsValid() )
	{
		hSettings.Get( "name_prefix_first", m_bPrefixFirst );
	}

// Done.

	GPDEV_ONLY( m_Initializing = false );

	// done
	return ( true );
}

bool PContentDb :: IsInitialized( void )
{
	return ( !m_TraitTemplateDb.empty() );
}

UINT32 PContentDb :: AddCrc32( UINT32 seed ) const
{
	return ( m_MainFormulas ? m_MainFormulas->GetImpl()->AddCrc32( seed ) : seed );
}

void PContentDb :: Shutdown( void )
{
	m_MacroDb              .clear();
	m_TraitTemplateDb      .clear();
	m_ModifierStorageDb    .clear();
	m_ModifierPrefixDb     .clear();
	m_ModifierSuffixDb     .clear();
	m_MeleeTypesNoStaff    .clear();
	m_RangedTypes          .clear();
	m_InfiniteMeleeWeapons .clear();
	m_InfiniteRangedWeapons.clear();
	m_MpLootScaling        .clear();

	m_MaxModifierPower	= 0;
	m_bPrefixFirst		= true;

	Delete ( m_MainFormulas );
	Delete ( m_LoaderFormulas );

#	if !GP_RETAIL
	m_Initializing				= false;
	m_CallDepth					= 0;
	m_NoPContentAllowedItems	= 0;
	m_StatRequests				= 0;
	m_StatRequestIterations		= 0;
	m_StatLowFinds				= 0;
	m_StatHighFinds				= 0;
	m_StatHighSubstitutions		= 0;
	m_StatModifiersTooLow		= 0;
	m_StatModifiersTooHigh		= 0;
	m_StatOverflowIntoMods		= 0;
	m_StatModifiersNotFound		= 0;
	m_StatTotalTime				= 0;
#	endif // !GP_RETAIL
}

SKRIT_IMPORT float CallFloatFormula( Skrit::Object* skrit, const char* funcName, ePContentType /*type*/ )
	{  SKRIT_CALL( float, CallFloatFormula, skrit, funcName );  }
SKRIT_IMPORT int CallIntFormula( Skrit::Object* skrit, const char* funcName, ePContentType /*type*/ )
	{  SKRIT_CALL( int, CallIntFormula, skrit, funcName );  }
SKRIT_IMPORT ePContentType CallPContentFormula( Skrit::Object* skrit, const char* funcName )
	{  SKRIT_CALL( ePContentType, CallPContentFormula, skrit, funcName );  }
SKRIT_IMPORT ePContentType CallPContentTypeFormula( Skrit::Object* skrit, const char* funcName, ePContentType /*type*/ )
	{  SKRIT_CALL( ePContentType, CallPContentTypeFormula, skrit, funcName );  }

ePContentType PContentDb :: CalcGeneralType( void )
{
	ePContentType type = PT_INVALID;

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		type = CallPContentFormula( formulas, "calc_general_type$" );
	}

	return ( type );
}

float PContentDb :: CalcItemPower(
		ePContentType type, ePContentType specificType, float minPower,
		float maxPower, bool isTwoHanded, const char* subType,
		const char* length, const char* material )
{
	float power = 0;

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		if ( minPower == maxPower )
		{
			power = minPower;
		}

		if ( (minPower != FLOAT_MAX) && (maxPower != FLOAT_MAX) )
		{
			formulas->GetRecord()->Set( "item_specific_type", specificType );
			formulas->GetRecord()->Set( "item_power_min",     minPower     );
			formulas->GetRecord()->Set( "item_power_max",     maxPower     );
			formulas->GetRecord()->Set( "item_subtype",       subType      );
			formulas->GetRecord()->Set( "item_length",        length       );
			formulas->GetRecord()->Set( "item_material",      material     );
			formulas->GetRecord()->Set( "item_two_handed",    isTwoHanded  );
			power = CallFloatFormula( formulas, "calc_item_power$", type );
		}
		else
		{
			power = FLOAT_MAX;
		}
	}

	return ( power );
}

float PContentDb :: CalcItemPower( ePContentType type, float minPower, float maxPower )
{
	// $ this special version of item power calc does not artificially modify
	//   the power levels based on type. only used to get average.

	return ( CalcItemPower( type, type, minPower, maxPower ) );
}

int PContentDb :: CalcModifierCount( ePContentType type, float totalPower )
{
	int count = 0;

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		if ( totalPower != FLOAT_MAX )
		{
			formulas->GetRecord()->Set( "total_power", totalPower );
			count = CallIntFormula( formulas, "calc_modifier_count$", type );
			clamp_min_max( count, -1, MAX_MODIFIERS );
		}
	}

	return ( count );
}

float PContentDb :: CalcModifierPower( ePContentType type, float totalPower, int modifierCount, eDbType dbType )
{
	float power = 0;

	if ( totalPower != FLOAT_MAX )
	{
		Skrit::Object* formulas = GetFormulas();
		if ( formulas != NULL )
		{
			formulas->GetRecord()->Set( "total_power", totalPower );
			formulas->GetRecord()->Set( "modifier_count", modifierCount );
			formulas->GetRecord()->Set( "item_is_normal", (bool)((dbType & DB_NORMAL) || (dbType == 0)) );
			formulas->GetRecord()->Set( "item_is_rare",   (bool)((dbType & DB_RARE  ) != 0) );
			formulas->GetRecord()->Set( "item_is_unique", (bool)((dbType & DB_UNIQUE) != 0) );
			power = CallFloatFormula( formulas, "calc_modifier_power$", type );
		}
	}
	else
	{
		gperrorf(( "Error in calc_modifier_power$: unable to calculate modifier power from an infinite request!\n" ));
	}

	return ( power );
}

float PContentDb :: CalcModifierBiasedPower( ePContentType type, ePContentType specificType, float modifierPower, const char* subType, const char* material )
{
	float power = modifierPower;

	gpassert( modifierPower != FLOAT_MAX );

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		formulas->GetRecord()->Set( "modifier_power",     modifierPower );
		formulas->GetRecord()->Set( "item_specific_type", specificType  );
		formulas->GetRecord()->Set( "item_subtype",       subType       );
		formulas->GetRecord()->Set( "item_material",      material      );
		power = CallFloatFormula( formulas, "calc_modifier_biased_power$", type );
	}

	return ( power );
}

int PContentDb :: CalcGoldValue(
		ePContentType type, ePContentType specificType, float minPower, float maxPower,
		float itemPower, float modifierPower0, float modifierPower1,
		bool isTwoHanded, const char* subType, const char* length, const char* material )
{
	int value = 0;

	gpassert( itemPower != FLOAT_MAX );

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		formulas->GetRecord()->Set( "item_specific_type",  specificType      );
		formulas->GetRecord()->Set( "item_power_min",      minPower          );
		formulas->GetRecord()->Set( "item_power_max",      maxPower          );
		formulas->GetRecord()->Set( "item_power",          itemPower         );
		formulas->GetRecord()->Set( "modifier_power_0",    modifierPower0    );
		formulas->GetRecord()->Set( "modifier_power_1",    modifierPower1    );
		formulas->GetRecord()->Set( "item_subtype",        subType           );
		formulas->GetRecord()->Set( "item_length",         length            );
		formulas->GetRecord()->Set( "item_material",       material          );
		formulas->GetRecord()->Set( "item_two_handed",     isTwoHanded       );
		value = CallIntFormula( formulas, "calc_gold_value$", type );
	}

	return ( value );
}

float PContentDb :: CalcItemPowerFuzziness( ePContentType type, float itemPower, eDbType dbType )
{
	float fuzz = 0;

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		if ( itemPower != FLOAT_MAX )
		{
			formulas->GetRecord()->Set( "item_power",     itemPower );
			formulas->GetRecord()->Set( "item_is_normal", (bool)((dbType & DB_NORMAL) || (dbType == 0)) );
			formulas->GetRecord()->Set( "item_is_rare",   (bool)((dbType & DB_RARE  ) != 0) );
			formulas->GetRecord()->Set( "item_is_unique", (bool)((dbType & DB_UNIQUE) != 0) );
			fuzz = CallFloatFormula( formulas, "calc_item_power_fuzziness$", type );
		}
	}

	return ( fuzz );
}

float PContentDb :: CalcModifierPowerFuzziness( ePContentType type, float modifierPower, eDbType dbType )
{
	float fuzz = 0;

	gpassert( modifierPower != FLOAT_MAX );

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		formulas->GetRecord()->Set( "modifier_power", modifierPower );
		formulas->GetRecord()->Set( "item_is_normal", (bool)((dbType & DB_NORMAL) || (dbType == 0)) );
		formulas->GetRecord()->Set( "item_is_rare",   (bool)((dbType & DB_RARE  ) != 0) );
		formulas->GetRecord()->Set( "item_is_unique", (bool)((dbType & DB_UNIQUE) != 0) );
		fuzz = CallFloatFormula( formulas, "calc_modifier_power_fuzziness$", type );
	}

	return ( fuzz );
}

ePContentType PContentDb :: CalcArmorCoverage( void )
{
	ePContentType type = PT_INVALID;

	Skrit::Object* formulas = GetFormulas();
	if ( formulas != NULL )
	{
		type = CallPContentTypeFormula( formulas, "calc_armor_coverage$", PT_ARMOR );
	}

	return ( type );
}

#if !GP_RETAIL

float PContentDb :: GetModifierPower( const char* modifierName ) const
{
	ModifierStorageDb::const_iterator found = m_ModifierStorageDb.find( modifierName );
	if ( found != m_ModifierStorageDb.end() )
	{
		return ( found->second.GetPower() );
	}
	else
	{
		gperrorf(( "PContentDb::GetModifierPower - illegal modifier name: %s\n", modifierName ));
		return ( 0 );
	}
}

const PContentDb::ItemEntry* PContentDb :: FindItemEntry( const char* templateName, const char* variationName ) const
{
	// $ note: this is really SLOW, and could be easily sped up with an index.
	//   however as this is only needed for automation this isn't necessary yet.

	gpassert( templateName != NULL );
	gpassert( variationName != NULL );

	if ( ::same_no_case( variationName, "base" ) )
	{
		variationName = "";
	}

	TraitTemplateDb::const_iterator i, ibegin = m_TraitTemplateDb.begin(), iend = m_TraitTemplateDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		PowerTemplateDb::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
		for ( j = jbegin ; j != jend ; ++j )
		{
			const ItemEntry* entry = &j->second;
			if ( ::same_no_case( entry->m_Template->GetName(), templateName ) )
			{
				if ( ::same_no_case( entry->m_Variation.c_str(), variationName ) )
				{
					return ( entry );
				}
			}
		}
	}

	gperrorf(( "PContentDb::FindItemEntry - illegal template/variation name: %s:%s\n", templateName, variationName ));
	return ( NULL );
}

float PContentDb :: GetModifierMin( const char* templateName, const char* variationName ) const
{
	const ItemEntry* itemEntry = FindItemEntry( templateName, variationName );
	if ( itemEntry != NULL )
	{
		return ( itemEntry->m_ModifierMin );
	}
	else
	{
		return ( 0 );
	}
}

float PContentDb :: GetModifierMax( const char* templateName, const char* variationName ) const
{
	const ItemEntry* itemEntry = FindItemEntry( templateName, variationName );
	if ( itemEntry != NULL )
	{
		return ( itemEntry->m_ModifierMax );
	}
	else
	{
		return ( FLOAT_MAX );
	}
}

#endif // !GP_RETAIL

bool PContentDb :: QueryItemTraits(
		ItemTraits&				itemTraits,
		const GoDataTemplate*	godt,
		ePContentType			expectedType,
		bool					ignorePContentAllowedFlag,
		bool					keepRef,
		bool					specialGetTypeOnly )
{
	// $ note the optimization - we query the base then the template's gas
	//   file directly to avoid the overhead of completely constructing the
	//   template only to find out that it's not really allowed into pcontent
	//   anyway.

	if ( !ignorePContentAllowedFlag && (expectedType != PT_INVALID) )
	{
		// first check to see if the base is allowed
		const GoDataTemplate* godtBase = godt->GetBaseDataTemplate();
		if ( godtBase != NULL )
		{
			AutoRefTemplate autoRef( godtBase );
			const GoDataComponent* data = godtBase->FindComponentByName( "common" );
			if ( data != NULL )
			{
				// get record
				const FuBi::Record* record = data->GetRecord();
				gpassert( record != NULL );

				// allowed to match?
				bool allowed;
				gpverify( record->Get( "is_pcontent_allowed", allowed ) );
				if ( !allowed )
				{
#					if !GP_RETAIL
					if ( m_Initializing )
					{
						++m_NoPContentAllowedItems;
					}
#					endif // !GP_RETAIL
					return ( false );
				}
			}
		}

		// now check the gas directly
		FastFuelHandle commonFuel = godt->GetFuelHandle().GetChildNamed( "common" );
		if ( commonFuel && (*commonFuel.GetType() == '\0') )
		{
			if ( !commonFuel.GetBool( "is_pcontent_allowed", true ) )
			{
#				if !GP_RETAIL
				if ( m_Initializing )
				{
					++m_NoPContentAllowedItems;
				}
#				endif // !GP_RETAIL
				return ( false );
			}
		}
	}

	// definitely need this thing, lock it down
	AutoRefTemplate autoRef( godt );

	// get at common data
	if ( !specialGetTypeOnly )
	{
		const GoDataComponent* commonData = godt->FindComponentByName( "common" );
		if ( commonData != NULL )
		{
			// get record
			const FuBi::Record* record = commonData->GetRecord();
			gpassert( record != NULL );

			// allowed to have modifiers?
			bool temp;
			gpverify( record->Get( "allow_modifiers", temp ) );
			itemTraits.m_NoModifiers = !temp;

			// what about other traits?
			gpstring specialType;
			gpverify( record->Get( "pcontent_special_type", specialType ) );
			if ( !FromCommaString( specialType, itemTraits.m_DbType ) )
			{
				gperrorf(( "Illegal pcontent_special_type '%s' in template %s\n", specialType.c_str(), godt->GetName() ));
			}
		}
	}

	// get any modifiers local to this template
	float itemModifierPower = 0;
	const GoDataComponent* magicData = godt->FindComponentByName( "magic" );
	if ( (magicData != NULL) && !specialGetTypeOnly )
	{
		FastFuelHandle ench = magicData->GetInternalField( "enchantments" );
		if ( ench )
		{
			itemModifierPower = ench.GetFloat( "power" );
		}
	}

	// group what we're expecting to gather initial magic stats
	bool expectedMagic
			=  (expectedType == PT_SPELL    )
			|| (expectedType == PT_SCROLL   )
			|| (expectedType == PT_POTION   )
			|| (expectedType == PT_SPELLBOOK);

	// first check for if it's a spell/scroll/potion to avoid falling into a
	// weapon/armor slot
	ePContentType magicType = PT_INVALID;
	if ( (expectedType == PT_INVALID) || expectedMagic )
	{
		// if it's got a potion component then it's a potion!
		const GoDataComponent* potionData = godt->FindComponentByName( "potion" );
		if ( potionData != NULL )
		{
			magicType = PT_POTION;
		}
		else
		{
			// if it's got gui and es_spellbook then it's a spellbook!
			const GoDataComponent* guiData = godt->FindComponentByName( "gui" );
			if ( guiData != NULL )
			{
				const FuBi::Record* guiRecord = guiData->GetRecord();
				gpassert( guiRecord != NULL );

				eEquipSlot equipSlot;
				gpverify( guiRecord->Get( "equip_slot", equipSlot ) );
				if ( equipSlot == ES_SPELLBOOK )
				{
					magicType = PT_SPELLBOOK;
				}
			}

			// no found? then try spell/scroll
			if ( (magicType == PT_INVALID) && (magicData != NULL) )
			{
				// get records
				const FuBi::Record* magicRecord = magicData->GetRecord();
				gpassert( magicRecord != NULL );

				// if it's one-use then it's a scroll
				bool oneUse;
				gpverify( magicRecord->Get( "one_use", oneUse ) );
				if ( oneUse )
				{
					magicType = PT_SCROLL;
				}
				else
				{
					// get the magic class
					eMagicClass magicClass;
					gpverify( magicRecord->Get( "magic_class", magicClass ) );

					if ( magicClass == MC_COMBAT_MAGIC )
					{
						magicType = PT_CMAGIC;
					}
					else if ( magicClass == MC_NATURE_MAGIC )
					{
						magicType = PT_NMAGIC;
					}
					else if ( magicClass == MC_POTION )
					{
						magicType = PT_POTION;
					}
					else if ( expectedType != PT_INVALID )
					{
						gperrorf(( "Template '%s' is a spell but its magic class is '%s'!\n",
								   godt->GetName(), ::ToString( magicClass ) ));
						return ( false );
					}
				}
			}
		}
	}

	// attempt weapon
	if ( (expectedType == PT_WEAPON) || ((magicType == PT_INVALID) && (expectedType == PT_INVALID)) )
	{
		// get at attack data
		const GoDataComponent* data = godt->FindComponentByName( "attack" );
		if ( data != NULL )
		{
			// get record
			const FuBi::Record* record = data->GetRecord();
			gpassert( record != NULL );

			// get its attack class
			gpverify( record->Get( "attack_class", itemTraits.m_AttackClass ) );

			// update type
			itemTraits.m_SpecificType = ::ToPContentType( itemTraits.m_AttackClass );

			// have all our type info now
			if ( !specialGetTypeOnly )
			{
				// skip ammo
				if ( IsAmmo( itemTraits.m_AttackClass ) )
				{
					return ( false );
				}

				// get subtype and length
				GetWeaponTraits(
						itemTraits.m_ItemSubType,
						itemTraits.m_ItemLength,
						itemTraits.m_AttackClass,
						godt->GetName() );

				// check two handed
				bool temp;
				gpverify( record->Get( "is_two_handed", temp ) );
				itemTraits.m_IsTwoHanded = temp;

				// get its average power
				float damageMin, damageMax;
				gpverify( record->Get( "damage_min", damageMin ) );
				gpverify( record->Get( "damage_max", damageMax ) );
				itemTraits.m_ItemPower = itemModifierPower + CalcItemPower(
						PT_WEAPON,
						itemTraits.m_SpecificType,
						damageMin,
						damageMax,
						itemTraits.m_IsTwoHanded,
						itemTraits.m_ItemSubType,
						itemTraits.m_ItemLength );

				// check melee
				gpverify( record->Get( "is_melee", temp ) );
				itemTraits.m_IsMelee = temp;
				gpverify( record->Get( "is_projectile", temp ) );
				itemTraits.m_IsRanged = temp;
			}

			// done
			if ( keepRef )
			{
				gContentDb.AddRefTemplate( godt );
			}
			return ( true );
		}
		else if ( expectedType != PT_INVALID )
		{
			gperrorf(( "Expected a weapon, found something else ('%s')!\n", godt->GetName() ));
			return ( false );
		}
	}

	// attempt armor
	if ( (expectedType == PT_ARMOR) || ((magicType == PT_INVALID) && (expectedType == PT_INVALID)) )
	{
		// get at component data
		const GoDataComponent* guiData    = godt->FindComponentByName( "gui"    );
		const GoDataComponent* defendData = godt->FindComponentByName( "defend" );
		if ( (guiData != NULL) && (defendData != NULL) )
		{
			// get records
			const FuBi::Record* guiRecord    = guiData   ->GetRecord();
			const FuBi::Record* defendRecord = defendData->GetRecord();
			gpassert( guiRecord    != NULL );
			gpassert( defendRecord != NULL );

			// get the equip slot
			eEquipSlot equipSlot;
			gpverify( guiRecord->Get( "equip_slot", equipSlot ) );
			itemTraits.m_SpecificType = ::ToPContentType( equipSlot );

			// get its power
			if ( !specialGetTypeOnly )
			{
				// check validity
				if ( !::IsArmorPContentType( itemTraits.m_SpecificType ) )
				{
					if ( expectedType != PT_INVALID )
					{
						gperrorf(( "Template '%s' is armor but uses invalid armor equip slot '%s'!\n",
								   godt->GetName(), ::ToString( equipSlot ) ));
					}
					return ( false );
				}

				// get its skill type
				gpstring material, skill;
				if ( GetArmorTraitsFromTemplateName( material, skill, godt->GetName() ) )
				{
					itemTraits.m_ItemSubType = skill;
				}

				// get power
				float defense;
				gpverify( defendRecord->Get( "defense", defense ) );
				itemTraits.m_ItemPower = itemModifierPower + CalcItemPower(
						PT_ARMOR,
						itemTraits.m_SpecificType,
						defense,
						defense,
						false,
						itemTraits.m_ItemSubType,
						itemTraits.m_ItemLength,
						material );
			}

			// done
			if ( keepRef )
			{
				gContentDb.AddRefTemplate( godt );
			}
			return ( true );
		}
		else if ( expectedType != PT_INVALID )
		{
			gperrorf(( "Expected armor, found something else ('%s')!\n", godt->GetName() ));
			return ( false );
		}
	}

	// attempt spell/scroll/potion/spellbook
	if ( expectedMagic || ((magicType != PT_INVALID) && (expectedType == PT_INVALID)) )
	{
		// get at component data
		if ( magicData != NULL )
		{
			// set our type from what we got before
			itemTraits.m_SpecificType = magicType;

			// we have type, may not need to query more
			if ( !specialGetTypeOnly )
			{
				// get records
				const FuBi::Record* magicRecord = magicData->GetRecord();
				gpassert( magicRecord != NULL );

				// now get the power level - use required level or pcontent level
				// for this one, whichever is greater
				float requiredLevel = 0, pcontentLevel = 0;
				gpverify( magicRecord->Get( "required_level", requiredLevel ) );
				gpverify( magicRecord->Get( "pcontent_level", pcontentLevel ) );
				itemTraits.m_ItemPower = itemModifierPower + max( requiredLevel, pcontentLevel );
			}

			// done
			if ( keepRef )
			{
				gContentDb.AddRefTemplate( godt );
			}
			return ( true );
		}
		else if ( magicType == PT_SPELLBOOK )
		{
			// set our type from what we got before
			itemTraits.m_SpecificType = magicType;

			// we have type, may not need to query more
			if ( !specialGetTypeOnly )
			{
				itemTraits.m_ItemPower = itemModifierPower;
			}

			// done
			if ( keepRef )
			{
				gContentDb.AddRefTemplate( godt );
			}
			return ( true );
		}
		else if ( expectedType != PT_INVALID )
		{
			gperrorf(( "Expected a %s, found something else ('%s')!\n", ::ToQueryString( expectedType ), godt->GetName() ));
			return ( false );
		}
	}

	// keep asking base templates until we hit a match (probably ring/amulet)
	if ( (expectedType == PT_AMULET) || (expectedType == PT_RING) || (expectedType == PT_INVALID) )
	{
		for (  const GoDataTemplate* base = godt
			 ; base != NULL
			 ; base = base->GetBaseDataTemplate() )
		{
			if ( ::FromQueryString( base->GetName(), itemTraits.m_SpecificType ) )
			{
				if ( keepRef )
				{
					gContentDb.AddRefTemplate( godt );
				}
				return ( true );
			}
		}
	}

	// if got here, didn't find anything
	return ( false );
}

const gpstring* PContentDb :: FindMacro( const char* name ) const
{
	const gpstring* macro = NULL;

	StringDb::const_iterator found = m_MacroDb.find( name );
	if ( found != m_MacroDb.end() )
	{
		macro = &found->second;
	}

	return ( macro );
}

const PContentDb::ModifierEntry* PContentDb :: FindModifier( const char* name ) const
{
	const ModifierEntry* entry = NULL;

	ModifierStorageDb::const_iterator found = m_ModifierStorageDb.find( name );
	if ( found != m_ModifierStorageDb.end() )
	{
		entry = &found->second;
	}

	return ( entry );
}

bool PContentDb :: GetArmorTraitsFromTemplateName( gpstring& material, gpstring& skill, const char* name )
{
	stringtool::Extractor extractor( "_", name );

	// get type
	const char* begin, * end;
	if ( !extractor.GetNextString( begin, end ) || (begin == end) )
	{
		return ( false );
	}

	// skip the temp prefix if any
	if ( same_no_case( "temp", begin, end - begin ) )
	{
		if ( !extractor.GetNextString( begin, end ) || (begin == end) )
		{
			return ( false );
		}
	}

	// skip the subtype if we're boots, gloves, or helms
	if (   same_no_case( "bo", begin, end - begin )
		|| same_no_case( "gl", begin, end - begin )
		|| same_no_case( "he", begin, end - begin ) )
	{
		if ( !extractor.GetNextString( begin, end ) || (begin == end) )
		{
			return ( false );
		}
	}

	// next is material
	if ( !extractor.GetNextString( material ) || material.empty() )
	{
		return ( false );
	}

	// if the material was "un" or "ra" then skip it (as per fug 4276 and later
	// emails from jake)
	if ( material.same_no_case( "un" ) || material.same_no_case( "ra" ) )
	{
		if ( !extractor.GetNextString( material ) || material.empty() )
		{
			return ( false );
		}
	}

	// next is skill
	if ( !extractor.GetNextString( skill ) || skill.empty() )
	{
		return ( false );
	}

	return ( true );
}

bool PContentDb :: GetProjectileTraitsFromTemplateName( gpstring& length, const char* name )
{
	stringtool::Extractor extractor( "_", name );

	// get type
	const char* begin, * end;
	if ( !extractor.GetNextString( begin, end ) || (begin == end) )
	{
		return ( false );
	}

	// skip the temp prefix if any
	if ( same_no_case( "temp", begin, end - begin ) )
	{
		if ( !extractor.GetNextString( begin, end ) || (begin == end) )
		{
			return ( false );
		}
	}

	// get iter count
	int iterCount = 0;
	if ( same_no_case( "bw", begin, end - begin ) )
	{
		iterCount = 2;
	}
	else if ( same_no_case( "cw", begin, end - begin ) )
	{
		iterCount = 1;
	}
	else
	{
		return ( false );
	}

	// get potential un/ra
	if ( !extractor.GetNextString( begin, end ) || (begin == end) )
	{
		return ( false );
	}

	// skip uniques and rares from this part
	if ( same_no_case( "un", begin, end - begin ) || same_no_case( "ra", begin, end - begin ) )
	{
		++iterCount;
	}

	// iterate till we find it
	for ( ; iterCount != 0 ; --iterCount )
	{
		if ( !extractor.GetNextString( begin, end ) || (begin == end) )
		{
			return ( false );
		}
	}

	// next is the length
	if ( !extractor.GetNextString( length ) || length.empty() )
	{
		return ( false );
	}

	return ( true );
}

void PContentDb :: GetWeaponTraits( gpstring& subType, gpstring& length, eAttackClass attackClass, const char* name )
{
	subType = ::ToQueryString( attackClass );
	if ( attackClass == AC_MINIGUN )
	{
		// could be a crossbow - checkit
		if ( same_no_case( name, "cw_", 3 ) )
		{
			subType = "crossbow";
			GetProjectileTraitsFromTemplateName( length, name );
		}
	}
	else if ( attackClass == AC_BOW )
	{
		GetProjectileTraitsFromTemplateName( length, name );
	}
}

bool PContentDb :: ParseQuery( PContentReq& req, const char* query, bool processMacros )
{
	bool success = true;

	gpassert( query != NULL );
	GPDEV_ONLY( req.m_QueryText = query );

	gppcontentf(( "\nParsing PContent query '%s' (macros %s)\n\n", query, processMacros ? "ok" : "not ok" ));
	GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

	// ok god DAMMIT i'm just going to @$&&%@# set the @&*$&*!# FPU mode 
	// every GOD DAMN QUERY and maybe THEN my !*$&*&$ hell @$%*&&% bugs
	// will never come back. see 11777, 11933, 12111, 11726. -sb
	SetFpuChopMode();

// Process traits.

	/*
		request
			: <NULL>
			| seed '/' full_trait '/' params
			| full_trait '/' params
			;

		seed
			: '[' random_seed ']'   // seed for random number generator before query
			;

		full_trait
			: <NULL>
			| trait
			| trait ',' armor_trait
			| trait ',' armor_trait ',' armor_trait
			| trait ':' finish
			;

		finish
			: <fuel_block_name>
			;

		armor_trait
			: armor_material
			| armor_skill_type
			;

		armor_material
			: "le", "sl", "bl", "br", "sc", "ch", "ba", "pl", "fp", "bp"
			;

		armor_skill_type
			: "f", "r", "m"
			;
	*/

	const char* i = query;
	if ( i[ 0 ] == '[' )
	{
		// seed
		const char* begin = i;
		++i;
		bool rc = ::FromString( i, req.m_RandomSeed );

		// skip the rest
		for ( ++i ; (*i != '\0') && (*i != '/') ; ++i )
		{
			// $ just advance
		}

		// seed, or possible error
		if ( rc )
		{
			req.m_ForceSeed = true;
			SetSeed( req.m_RandomSeed );
			gppcontentf(( "Option: use seed 0x%08X\n", req.m_RandomSeed ));
		}
		else
		{
			gperrorf(( "Illegal random seed value '%s' found in query\n", gpstring( begin, i ).c_str() ));
		}

		// skip delim
		if ( *i == '/' )
		{
			++i;
		}
	}

	gpstring trait;
	for ( ; (*i != '\0') && (*i != '/') ; ++i )
	{
		if ( *i == ',' )
		{
			for ( ++i ; (*i != '\0') && (*i != '/') ; ++i )
			{
				if ( *i == ',' )
				{
					for ( ++i ; (*i != '\0') && (*i != '/') ; ++i )
					{
						req.m_ArmorSkillType += *i;
					}
					break;
				}
				else
				{
					req.m_ArmorMaterial += *i;
				}
			}

			if ( req.m_ArmorMaterial.length() == 1 )
			{
				req.m_ArmorMaterial.swap( req.m_ArmorSkillType );
			}

#			if !GP_RETAIL
			if ( !req.m_ArmorSkillType.empty() )
			{
				gppcontentf(( "Option: query for armor skill type '%s'\n", req.m_ArmorSkillType.c_str() ));
			}
			if ( !req.m_ArmorMaterial.empty() )
			{
				gppcontentf(( "Option: query for armor material '%s'\n", req.m_ArmorMaterial.c_str() ));
			}
#			endif // !GP_RETAIL

			break;
		}
		else if ( *i == ':' )
		{
			for ( ++i ; (*i != '\0') && (*i != '/') ; ++i )
			{
				req.m_Finish += *i;
			}

			gppcontentf(( "Option: use variation '%s'\n", req.m_Finish.c_str() ));

			break;
		}
		else
		{
			trait += *i;
		}
	}

// Check for macros.

	/*
		trait
			: <NULL>
			| '*'
			| pcontent_type
			| attack_class
			| template_name
			| macro_name
			;

		pcontent_type
			: "armor" | "weapon" | "amulet" | "ring" | "body" | "helm"
			| "gloves" | "boots" | "shield" | "melee" | "ranged" | "spell"
			| "nmagic" | "cmagic" | "scroll" | "potion" | "spellbook"
			;

		attack_class
			: "beastfu" | "axe" | "club" | "dagger" | "hammer" | "longsword"
			| "mace" | "staff" | "sword" | "bow" | "minigun" | "arrow" | "bolt"
			| "combat_magic" | "nature_magic"
			;
	*/

	// check for macro substitution
	if ( processMacros )
	{
		const gpstring* macro = FindMacro( trait );
		if ( macro != NULL )
		{
			gppcontent( "Macro found\n" );

			// $ early bailout
			return ( ParseQuery( req, *macro + i, false ) );
		}
	}

// Parse params.

	/*
		params
			: param
			| params '/' param
			;

		param
			: power_range
			| modifier
			| option
			| seed
			;

		power_range
			: bound
			| '-' bound
			| bound '-'
			| bound '-' bound
			;

		bound
			: '*'
			| <float literal>
			;

		modifier
			: '+' modifier_name     // name of fuel block for modifier - up to MAX_MODIFIERS allowed per query
			;

		option
			: "-no_mod"             // no modifiers allowed
			| "-no_orn"             // no ornate finish allowed
			;
	*/

	// parse
	bool powerNeedsCalc = false;
	while ( *i != '\0' )
	{
		gpassert( *i == '/' );
		++i;

		if ( (i[ 0 ] == '-') && ::isalpha( i[ 1 ] ) )
		{
			// option
			++i;
			const char* end = ::strchr( i, '/' );
			int len = end ? (end - i) : ::strlen( i );

			// find it
			bool success = false;
			if ( same_no_case( i, "no_mod", len ) )
			{
				gppcontent( "Option: no modifiers allowed\n" );
				req.m_ReqModifierCount = 0;
				success = true;
			}
			else if ( same_no_case( i, "no_orn", len ) )
			{
				gppcontent( "Option: no ornate items allowed\n" );
				req.m_NoOrnate = true;
				success = true;
			}
			else if ( same_no_case( i, "mod(", 4 ) )
			{
				success = ::FromString( i + 4, req.m_ReqModifierCount );
			}
			else if ( same_no_case( i, "rare", 4 ) )
			{
				if ( i[ 4 ] == '(' )
				{
					if ( ::FromString( i + 5, req.m_ReqModifierCount ) )
					{
						gppcontentf(( "Option: rare type with %d modifiers\n", req.m_ReqModifierCount ));
						req.m_DbType = DB_RARE;
						success = true;
					}
				}
				else if ( len == 4 )
				{
					gppcontent( "Option: rare type\n" );
					req.m_DbType = DB_RARE;
					success = true;
				}
			}
			else if ( same_no_case( i, "unique", 6 ) )
			{
				if ( i[ 6 ] == '(' )
				{
					if ( ::FromString( i + 7, req.m_ReqModifierCount ) )
					{
						gppcontentf(( "Option: unique type with %d modifiers\n", req.m_ReqModifierCount ));
						req.m_DbType = DB_UNIQUE;
						success = true;
					}
				}
				else if ( len != 6 )
				{
					gppcontent( "Option: unique type\n" );
					req.m_DbType = DB_UNIQUE;
					success = true;
				}
			}

			if ( !success )
			{
				gperrorf(( "Invalid pcontent query option '-%.*s'\n", len, i ));
			}
			if ( ::clamp_max( req.m_ReqModifierCount, MAX_MODIFIERS ) )
			{
				gperrorf(( "Too many modifiers requested in option '-%.*s'\n", len, i ));
			}

			// advance
			i += len;
		}
		else if ( (i[ 0 ] == '+') && ::isalpha( i[ 1 ] ) )
		{
			// modifier
			gpstring name;
			for ( ++i ; (*i != '\0') && (*i != '/') ; ++i )
			{
				name += *i;
			}

			// use it
			const ModifierEntry* modifier = FindModifier( name );
			if ( modifier != NULL )
			{
				// assign to a slot
				const ModifierEntry*& out = req.m_Modifiers[ modifier->GetIndex() ];
				if ( out == NULL )
				{
					out = modifier;
					gppcontentf(( "Option: use modifier '%s' (%s)\n", modifier->GetName().c_str(), (modifier->GetIndex() == 0) ? "prefix" : "suffix" ));
				}
				else
				{
					gperrorf(( "Invalid %s modifier '%s' - only one modifier allowed per slot!\n",
							   modifier->GetIndexName(), name.c_str() ));
				}
			}
			else
			{
				gperrorf(( "Illegal modifier name '%s' found in query\n", name.c_str() ));
			}
		}
		else
		{
			// must be a power range
			const char* begin = i;
			const char* div = NULL;
			float minPower = 0;
			float maxPower = FLOAT_MAX;

			// find end of segment
			while ( (*i != '\0') && (*i != '/') )
			{
				if ( *i == '-' )
				{
					div = i;
				}
				++i;
			}

			// go to end if div not found
			if ( div == NULL )
			{
				div = i;
			}

			// get the min
			if ( (begin != div) && !((*begin == '*') && (begin == (div - 1))) )
			{
				if ( !::FromString( begin, div, minPower ) )
				{
					gperrorf(( "Invalid pcontent query power '%.*s'\n", div - begin, begin ));
				}
			}

			// get the max
			if ( div != i )
			{
				++div;

				// if not pattern 'x-' or 'x-*'...
				if ( (div != i) && !((*div == '*') && (div == (i - 1))) )
				{
					if ( !::FromString( div, i, maxPower ) )
					{
						gperrorf(( "Invalid pcontent query power '%.*s'\n", i - div, div ));
					}
					else if ( maxPower < minPower )
					{
						gperrorf(( "Invalid pcontent range '%.*s'\n", i - begin, begin ));
						maxPower = minPower;
					}
				}
			}
			else if ( !((*begin == '*') && (begin == (div - 1))) )
			{
				maxPower = minPower;
			}

			req.m_TotalPower.m_Min = minPower;
			req.m_TotalPower.m_Max = maxPower;
			powerNeedsCalc = true;
		}
	}

// Postprocess traits.

	// if it's the "all" case, randomly choose a trait
	if ( (trait[ 0 ] == '\0') || ((trait[ 0 ] == '*') && (trait[ 1 ] == '\0')) )
	{
		gppcontent( "'All' case detected, randomly choosing a trait\n" );
		ePContentType type = CalcGeneralType();
		if ( type != PT_INVALID )
		{
			trait = ::ToQueryString( type );
			gppcontentf(( "Randomly chose trait '%s'\n", trait.c_str() ));
		}
	}

	// attempt to extract type directly if possible
	bool hadTrait = ::FromQueryString( trait, req.m_SpecificType );
	if ( !hadTrait )
	{
		// try weapon class
		eAttackClass ac;
		if ( ::FromQueryString( trait, ac ) )
		{
			req.m_SpecificType = ::ToPContentType( ac );

			gppcontentf(( "Extracted type '%s' from trait field\n", ::ToString( req.m_SpecificType ) ));
		}
	}

	// calc the requested power
	if ( powerNeedsCalc )
	{
		req.m_TotalPower.m_Mid = CalcItemPower( ::ToBasicPContentType( req.m_SpecificType ), req.m_TotalPower.m_Min, req.m_TotalPower.m_Max );
		gppcontentf(( "Extracted power range %s\n", req.m_TotalPower.ToString().c_str() ));
	}

	// reprocess weapon
	if ( hadTrait )
	{
		gppcontentf(( "Extracted type '%s' from trait field\n", ::ToString( req.m_SpecificType ) ));
		GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

		// special handling
		switch ( req.m_SpecificType )
		{
			// if it's generic weapon, choose melee/ranged/staff then subdivide
			case ( PT_WEAPON ):
			{
				gppcontent( "Requested generic weapon, choosing random melee/ranged/staff then subdividing\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				int i = Random( 2 );
				if ( i == 0 )
				{
					// 1/3 chance of staff
					req.m_SpecificType = PT_MELEE;
					trait = "staff";
				}
				else
				{
					// 1/3 chance of melee, 1/3 chance of ranged
					req.m_SpecificType = (i == 1) ? PT_MELEE : PT_RANGED;
					trait = ChooseRandomTrait( req.m_TotalPower.m_Mid, req.m_SpecificType );
				}

				gppcontentf(( "Randomly chose %s, weapon type is '%s'\n", ::ToString( req.m_SpecificType ), trait.c_str() ));
			}
			break;

			// if it's melee, choose any melee weapon (including staves)
			case ( PT_MELEE ):
			{
				gppcontent( "Requested melee weapon, choosing random type\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				// choose
				int count = m_MeleeTypesNoStaff.size();
				int choice = Random( count );
				if ( choice == count )
				{
					// special: last (virtual entry) is staff
					trait = "staff";
				}
				else
				{
					trait = ChooseRandomTrait( req.m_TotalPower.m_Mid, req.m_SpecificType );
				}

				gppcontentf(( "Randomly chose weapon type '%s'\n", trait.c_str() ));
			}
			break;

			// if it's ranged, choose any ranged weapon
			case ( PT_RANGED ):
			{
				gppcontent( "Requested ranged weapon, choosing random type\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				trait = ChooseRandomTrait( req.m_TotalPower.m_Mid, req.m_SpecificType );

				gppcontentf(( "Randomly chose weapon type '%s'\n", trait.c_str() ));
			}
			break;

			// if it's generic armor, randomly choose the body coverage area
			case ( PT_ARMOR ):
			{
				gppcontent( "Requested armor, choosing random type\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				req.m_SpecificType = CalcArmorCoverage();
				if ( !IsArmorPContentType( req.m_SpecificType ) )
				{
					// if bad, default to body
					gperrorf(( "Skrit returned non-armor type '%s' when requesting armor coverage, defaulting to body armor\n", ::ToString( req.m_SpecificType ) ));
					req.m_SpecificType = PT_BODY;
				}
				trait = ::ToQueryString( req.m_SpecificType );

				gppcontentf(( "Randomly chose armor type '%s' from pcontent.gas:calc_armor_coverage$\n", trait.c_str() ));
			}
			break;

			// if it's generic spell, randomly choose specific type
			case ( PT_SPELL ):
			{
				gppcontent( "Requested spell, choosing random type\n" );
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				req.m_SpecificType = (ePContentType)Random( (int)PT_SPELL_BEGIN, (int)PT_SPELL_END - 1 );
				trait = ::ToQueryString( req.m_SpecificType );

				gppcontentf(( "Randomly chose type '%s'\n", trait.c_str() ));
			}
			break;
		}
	}

	// unknown?
	if ( req.m_SpecificType == PT_INVALID )
	{
		gppcontentf(( "Extracted specific template name '%s' from trait field\n", trait.c_str() ));
		GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

		// it must be a template - find it
		req.m_Template = gContentDb.FindTemplateByName( trait );
		if ( req.m_Template == NULL )
		{
			// invalid query
			gperrorf(( "Could not find template by name '%s' from pcontent query\n", trait.c_str() ));
			return ( false );
		}

		// gather traits
		ItemTraits itemTraits;
		if ( QueryItemTraits( itemTraits, req.m_Template ) )
		{
			req.m_SpecificType = itemTraits.m_SpecificType;
			req.m_ItemPower    = itemTraits.m_ItemPower;

			if ( itemTraits.m_NoModifiers )
			{
				req.m_ReqModifierCount = 0;
			}

			gppcontentf(( "Traits of requested template: type = '%s', power = %s, no modifiers = %s\n",
						  ::ToString( itemTraits.m_SpecificType ),
						  req.m_ItemPower.ToString().c_str(),
						  itemTraits.m_NoModifiers ? "yes" : "no" ));
		}
	}

	// update basic
	req.m_BasicType = ::ToBasicPContentType( req.m_SpecificType );
	gppcontentf(( "Basic type of pcontent chosen: '%s'\n", ::ToString( req.m_BasicType ) ));

	// process armor traits if available
	if ( (req.m_BasicType == PT_ARMOR) && (req.m_Template != NULL) )
	{
		GetArmorTraitsFromTemplateName( req.m_ArmorMaterial, req.m_ArmorSkillType, req.m_Template->GetName() );

		gppcontentf(( "Armor traits of requested template: material = '%s', skill type = '%s'\n",
					  req.m_ArmorMaterial.c_str(), req.m_ArmorSkillType.c_str() ));
	}

	// find trait db if haven't chosen specific template
	if ( req.m_Template == NULL )
	{
		req.m_TemplateDbIter = m_TraitTemplateDb.find( trait );
		if ( req.m_TemplateDbIter == m_TraitTemplateDb.end() )
		{
			// invalid db
			gperrorf(( "Could not find trait database '%s'\n", trait.c_str() ));
			return ( false );
		}
	}

	/*
		Now we've parsed the query into a context structure. Here's what we may
		have learned:

			m_QueryText
				always set to original query text (dev only)

			m_SpecificType
				this will be learned from query or we return false. either this
				was the type asked for in query, or we deduced it from the
				template asked for by QueryItemTraits().

			m_BasicType
				basic type of m_SpecificType.

			m_Template
				this will be NULL unless the query was for a specific template
				by name (and it was a valid template). if the template has been
				chosen, then lots of rules change. we will definitely get an
				object of this type.

			m_TemplateDbIter
				if we haven't chosen a specific template, then the traits will
				be used to look up the correct db where all our potential item
				hits live. this points to it. it may point to "end" in which
				case we'll never find anything.

			m_TotalPower
				this is the query's power request. defaults to 0-infinite, which
				will include everything in the potential set. note that in cases
				where the upper bound is infinite, we will never get modifiers
				by random choice. they must be requested instead.

			m_ReqModifierCount
				0 if -no_mod set in query
				0, 1, or 2 from -rare(1) style query

			m_RandomSeed
				set if used [123] style query where the random seed is dictated
				by the requester. generally used for debugging.

			m_ArmorMaterial
				this is set if query asked for a specific armor material

			m_ArmorSkillType
				this is set if query asked for a specific armor skill type

			m_Finish
				this is set if query asked for a specific "finish" to be placed
				on the item.

			m_Modifiers
				set if asked for specific modifier(s) to be applied to the item.

			m_NoOrnate
				true if -no_orn set in query
	*/

	return ( success );
}

bool PContentDb :: ProcessRequest( PContentReq& req )
{
	gpassert( req.m_SpecificType != PT_INVALID );
	gpassert( ::IsBasicPContentType( req.m_BasicType ) );

	gppcontentf(( "\nProcessing PContent query (attempt %d)\n\n", req.m_AttemptCount ));
	GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

	// making another attempt...
	++req.m_AttemptCount;

#	if !GP_RETAIL
	::InterlockedIncrement( (LONG*)&m_StatRequestIterations );
	::InterlockedIncrement( (LONG*)&m_CallDepth );
	double startTime = ::GetSystemSeconds();
#	endif // !GP_RETAIL

// Get modifier traits.

	// get some vars
	float modifierPower[ MAX_MODIFIERS ];
	::ZeroObject( modifierPower );
	bool chooseModifiers = false;

	// calc item power based on requested power minus any requested modifiers
	if ( req.m_Template == NULL )
	{
		req.m_ItemPower = req.m_TotalPower;

		const ModifierEntry** i, ** ibegin = req.m_Modifiers, ** iend = ARRAY_END( req.m_Modifiers );
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( *i != NULL )
			{
				req.m_ItemPower -= (*i)->m_Power;

				gppcontentf(( "Subtracting modifier power %.2f from item\n", (*i)->m_Power ));
			}
		}
	}
	gppcontentf(( "Item power = %s\n", req.m_ItemPower.ToString().c_str() ));

	// only bother if we haven't already chosen modifiers
	if ( !req.HasModifiers() )
	{
		// make sure we haven't requested that modifiers be excluded
		if ( req.m_ReqModifierCount != 0 )
		{
			// get number of modifiers
			if ( req.m_ReqModifierCount == -1 )
			{
				req.m_ReqModifierCount = CalcModifierCount( req.m_BasicType, req.m_TotalPower );
			}
			gppcontentf(( "Randomly chose %d modifiers from power request level of %s\n", req.m_ReqModifierCount, req.m_TotalPower.ToString().c_str() ));
			GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

			// normal -  just choose at will
			if ( req.m_Template == NULL )
			{
				// get modifier power levels, and adjust power for item
				float* i, * ibegin = modifierPower, * iend = ibegin + req.m_ReqModifierCount;
				for ( i = ibegin ; i != iend ; ++i )
				{
					// calc the modifier power
					*i = CalcModifierPower( req.m_BasicType, req.m_TotalPower, req.m_ReqModifierCount, req.GetDbType() );

					// make sure to clamp it
					if ( *i > m_MaxModifierPower )
					{
						gppcontenterrorf((
								"TELL JAKE: I just had to clamp a modifier power (%.2f) because it was too high (from query '%s')\n",
								*i, req.m_QueryText.c_str() ));
						*i = m_MaxModifierPower;
					}
					gppcontentf(( "Randomly chose modifier power level of %.2f\n", *i ));

					// subtract from base
					req.m_ItemPower -= *i;

					// bias power by type for tweakiness
					*i = CalcModifierBiasedPower( req.m_BasicType, req.m_SpecificType, *i, req.m_ArmorSkillType, req.m_ArmorMaterial );
					GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );
					gppcontentf(( "Biased modifier power level to %.2f\n", *i ));
				}
			}
			else
			{
				// template already chosen - the difference between its
				// power and what we requested is what all the modifiers
				// will get (spread out evenly)
				float delta = (req.m_TotalPower - req.m_ItemPower) / req.m_ReqModifierCount;
				gppcontentf(( "Template already chosen - modifiers limited to power level of %.2f each\n", delta ));
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );
				if ( delta > 0 )
				{
					// get modifier power levels, and adjust power for item
					float* i, * ibegin = modifierPower, * iend = ibegin + req.m_ReqModifierCount;
					for ( i = ibegin ; i != iend ; ++i )
					{
						// bias delta by type for tweakiness
						*i = CalcModifierBiasedPower( req.m_BasicType, req.m_SpecificType, delta, req.m_ArmorSkillType, req.m_ArmorMaterial );

						// make sure to clamp modifier powers
						if ( *i > m_MaxModifierPower )
						{
							gppcontenterrorf((
									"TELL JAKE: I just had to clamp a modifier power (%.2f) because it was too high (from query '%s')\n",
									*i, req.m_QueryText.c_str() ));
							*i = m_MaxModifierPower;
						}
						gppcontentf(( "Biased modifier power level to %.2f\n", *i ));
					}
				}
				else
				{
					gppcontentf(( "Delta not >0, killing all modifiers\n", delta ));
					req.m_ReqModifierCount = 0;
				}
			}

			// ok, got the count, should we choose modifiers?
			chooseModifiers = req.m_ReqModifierCount > 0;
		}
	}
	else
	{
		// we already have modifiers - store the power settings in our local vars
		const ModifierEntry** i, ** ibegin = req.m_Modifiers, ** iend = ARRAY_END( req.m_Modifiers );
		float* out = modifierPower;
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( *i != NULL )
			{
				*out = (*i)->m_Power;
				++out;
				++req.m_ReqModifierCount;

				gppcontentf(( "Requested modifier '%s' has power level of %.2f\n", (*i)->GetName().c_str(), *out ));
			}
		}
	}

// Query for a base item.

	// only bother if we haven't chosen already, and we have a traits db
	eFoundType found = FOUND_ILLEGAL;
	float fuzziness = 0;
	gpstring originalMaterial, originalSkillType;
	if ( req.m_TemplateDbIter != m_TraitTemplateDb.end() )
	{
		gppcontentf(( "Querying for a base item...\n" ));
		GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

		gpassert( req.m_Template == NULL );

		const PowerTemplateDb* db = &req.m_TemplateDbIter->second;

		// get some vars
		fuzziness = CalcItemPowerFuzziness( req.m_BasicType, req.m_ItemPower, req.GetDbType() );
		PowerTemplateDb::const_iterator nearest;
		gppcontentf(( "Calculated item power fuzziness of %.2f\n", fuzziness ));

		// get total modifier power
		float totalModifierPower = 0;
		float* i, * ibegin = modifierPower, * iend = ibegin + req.m_ReqModifierCount;
		for ( i = ibegin ; i != iend ; ++i )
		{
			totalModifierPower += *i;
		}

		// special boots/gloves handling
		if ( (req.m_SpecificType == PT_BOOTS) || (req.m_SpecificType == PT_GLOVES) )
		{
			// only try to match material or skill to body if not fixed
			if ( req.m_ArmorMaterial.empty() || req.m_ArmorSkillType.empty() )
			{
				gppcontentf(( "Special: %s requested, must match material/skill to body, executing sub-query\n", ::ToString( req.m_SpecificType ) ));
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				// build sub request to get material/skill from equiv body
				PContentReq subReq( req );
				subReq.m_SpecificType = PT_BODY;

				// find it
				if ( ProcessRequest( subReq ) )
				{
					req.m_ArmorMaterial  = subReq.m_ArmorMaterial;
					req.m_ArmorSkillType = subReq.m_ArmorSkillType;

					gppcontentf(( "\nSub-query succeeded, chose material '%s', skill type '%s'\n", req.m_ArmorMaterial.c_str(), req.m_ArmorSkillType.c_str() ));
				}
			}
		}
		else if ( (req.m_SpecificType == PT_AMULET) || (req.m_SpecificType == PT_RING) )
		{
			gppcontentf(( "Ring/amulet detected, resetting item power to 0\n" ));
			req.m_ItemPower = 0;
		}

		// find a nice item of whatever type we're looking for
		found = FindNearest( nearest,
							 *db,
							 req.m_ItemPower.m_Min,
							 req.m_ItemPower.m_Max,
							 fuzziness,
							 req,
							 totalModifierPower );
		gppcontentf(( "Finding nearest match for item power %s and total modifier power %.2f, query returned %s\n",
					  req.m_ItemPower.ToString().c_str(), totalModifierPower, ToString( found ) ));

		// update stats
#		if ( !GP_RETAIL )
		{
			if ( found == FOUND_LOW )
			{
				::InterlockedIncrement( (LONG*)&m_StatLowFinds );
			}
			if ( found == FOUND_HIGH )
			{
				::InterlockedIncrement( (LONG*)&m_StatHighFinds );
			}
		}
#		endif // !GP_RETAIL

		// special weapon support for maxed out hits
		if ( (found == FOUND_HIGH) && (req.m_BasicType == PT_WEAPON) )
		{
			gppcontentf(( "Maxed out the weapon - no weapon is high enough in power to match request\n" ));
			GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

			const StringSet* infinite = NULL;

			// not an infinite weapon?
			if (   (req.m_SpecificType == PT_MELEE)
				&& (m_InfiniteMeleeWeapons.find( req.m_TemplateDbIter->first ) == m_InfiniteMeleeWeapons.end()) )
			{
				infinite = &m_InfiniteMeleeWeapons;
			}
			else if (   (req.m_SpecificType == PT_RANGED)
					 && (m_InfiniteRangedWeapons.find( req.m_TemplateDbIter->first ) == m_InfiniteRangedWeapons.end()) )
			{
				infinite = &m_InfiniteRangedWeapons;
			}

			// ok not infinite, then choose one
			if ( infinite != NULL )
			{
				if ( !infinite->empty() )
				{
					gppcontentf(( "Weapon type '%s' is not infinite-capable and will be substituted\n", req.m_TemplateDbIter->first.c_str() ));
					GPDEV_ONLY( ::InterlockedIncrement( (LONG*)&m_StatHighSubstitutions ) );

					// pick random
					int index = Random( m_InfiniteMeleeWeapons.size() - 1 );
					const gpstring& type = m_InfiniteMeleeWeapons.at( index ).GetString();
					gppcontentf(( "Picked new weapon type '%s' at random from set of available infinite weapons\n", type.c_str() ));

					// re-execute query with this weapon type
					req.m_TemplateDbIter = m_TraitTemplateDb.find( type );
					if ( req.m_TemplateDbIter != m_TraitTemplateDb.end() )
					{
						db = &req.m_TemplateDbIter->second;
						found = FindNearest( nearest,
											 *db,
											 req.m_ItemPower.m_Min,
											 req.m_ItemPower.m_Max,
											 fuzziness,
											 req,
											 totalModifierPower );
						gppcontentf(( "Finding nearest match for new item power %s and total modifier power %.2f, query returned %s\n",
									  req.m_ItemPower.ToString().c_str(), totalModifierPower, ToString( found ) ));
					}
					else
					{
						gperrorf(( "Invalid trait name '%s' - not in pcontent database! Found in infinite %s weapons block\n",
								   type.c_str(), (infinite == &m_InfiniteMeleeWeapons) ? "melee" : "ranged" ));
						found = FOUND_ILLEGAL;
					}
				}
				else
				{
					gperrorf(( "No available traits found in infinite %s weapons block!\n",
							   (infinite == &m_InfiniteMeleeWeapons) ? "melee" : "ranged" ));
					found = FOUND_ILLEGAL;
				}
			}
			else
			{
				gppcontentf(( "Weapon type '%s' is infinite-capable\n", req.m_TemplateDbIter->first.c_str() ));
			}
		}

		// if the item couldn't go that high, feed back the delta
		if ( (found == FOUND_HIGH) && (req.m_ReqModifierCount > 0) )
		{
			gppcontentf(( "Maxed out the item (modifiers are available to transfer power into)\n" ));
			GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

			// get the per-modifier delta
			float delta = req.m_ItemPower - nearest->first;
			req.m_ItemPower = nearest->first;
			gpassert( delta >= 0 );
			float each = delta / req.m_ReqModifierCount;

			// and put it back into the modifiers
			float* i, * ibegin = modifierPower, * iend = ibegin + req.m_ReqModifierCount;
			for ( i = ibegin ; i != iend ; ++i )
			{
				*i += each;

				// make sure to clamp it
				if ( *i > m_MaxModifierPower )
				{
					gppcontenterrorf((
							"TELL JAKE: I just had to clamp a modifier power (%.2f) because it was too high (from query '%s')\n",
							*i, req.m_QueryText.c_str() ));
					*i = m_MaxModifierPower;
				}
				gppcontentf(( "Changed modifier value by %.2f to %.2f\n", each, *i ));
			}

			// update total and find again with new modifier stats
			totalModifierPower += delta;
			found = FindNearest( nearest,
								 *db,
								 req.m_ItemPower.m_Min,
								 req.m_ItemPower.m_Max,
								 fuzziness,
								 req,
								 totalModifierPower );
			gppcontentf(( "Finding nearest match with new item power %s and total modifier power %.2f, query returned %s\n",
						  req.m_ItemPower.ToString().c_str(), ToString( found ) ));
		}

		// check for if the item couldn't accept a modifier that low
		if ( ((found == FOUND_EMPTY) || (found == FOUND_TOO_LOW)) && (req.m_ReqModifierCount > 0) )
		{
			gppcontent( (found == FOUND_EMPTY) ? "No items found at all!\n" : "Lowballed the item\n" );
			GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

			GPDEV_ONLY( ::InterlockedIncrement( (LONG*)&m_StatModifiersTooLow ) );

			// try again
			for ( int i = 0 ; i < 6 ; ++i )
			{
				gppcontentf(( "Attempt %d\n", i ));
				GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

				float delta = 1.0f;
				float each = delta / req.m_ReqModifierCount;
				totalModifierPower += delta;

				// and put it back into the modifiers
				float* i, * ibegin = modifierPower, * iend = ibegin + req.m_ReqModifierCount;
				for ( i = ibegin ; i != iend ; ++i )
				{
					*i += each;
					gppcontentf(( "Changed modifier value by %.2f to %.2f\n", each, *i ));
				}

				// see if that works...
				found = FindNearest( nearest,
									 *db,
									 req.m_ItemPower.m_Min,
									 req.m_ItemPower.m_Max,
									 fuzziness,
									 req,
									 totalModifierPower );
				gppcontentf(( "Finding nearest match with new item power %s and total modifier power %.2f, query returned %s\n",
							  req.m_ItemPower.ToString().c_str(), totalModifierPower, ToString( found ) ));
				if ( found == FOUND_OK )
				{
					break;
				}
			}

			if ( found != FOUND_OK )
			{
				// fuck!
				gppcontenterrorf((
						"TELL JAKE: I tried 6 freakin' times to find a modifier "
						"that fits from query '%s' but it's always too low to "
						"be accepted by the items I find! Cutting request in "
						"half and trying again...\n",
						req.m_QueryText.c_str() ));

				// can't be absolute zero or too many attempts!
				if ( req.m_AttemptCount <= 10 )
				{
					if (   ::IsZero( req.m_TotalPower.m_Min )
						&& ::IsZero( req.m_TotalPower.m_Mid )
						&& ::IsZero( req.m_TotalPower.m_Max ) )
					{
						// cut the power request down and try again
						req.m_TotalPower.m_Min /= 2;
						req.m_TotalPower.m_Mid /= 2;
						req.m_TotalPower.m_Max /= 2;
						return ( ProcessRequest( req ) );
					}
					else
					{
						gppcontenterrorf((
							"TELL JAKE: Query '%s' has been reduced to ZERO, and "
							"further searching will cause an infinite loop! Looks "
							"like object traits prevented anything showing up in "
							"the freakin' query (which I did 10 times)...\n",
							req.m_QueryText.c_str() ));
					}
				}
				else
				{
					gppcontenterrorf((
						"TELL JAKE: I divided the request '%s' in half 10 "
						"freakin' times and still found nothing! Aborting!\n",
						req.m_QueryText.c_str() ));
				}
			}
		}

		// check for if the item couldn't accept a modifier that high
		if ( found == FOUND_TOO_HIGH )
		{
			gppcontentf(( "Maxed out the item, attempting another random query from the top\n" ));
			GPDEV_ONLY( ::InterlockedIncrement( (LONG*)&m_StatModifiersTooHigh ) );

			// try again
			if ( req.m_AttemptCount <= 10 )
			{
				return ( ProcessRequest( req ) );
			}
			else
			{
				// fuck!
				gppcontenterrorf((
						"TELL JAKE: I tried 10 freakin' times to find a modifier "
						"that fits from query '%s' but it's always too high to "
						"be accepted by the items I find!\n",
						req.m_QueryText.c_str() ));

				// ok, try again, accepting anything that will fit for the modifier
				found = FindNearest( nearest,
									 *db,
									 req.m_ItemPower.m_Min,
									 req.m_ItemPower.m_Max,
									 fuzziness,
									 req,
									 -1 );
				gppcontentf(( "I give up - finding nearest match with new item power %s and total modifier power %.2f, query returned %s\n",
							  req.m_ItemPower.ToString().c_str(), ToString( found ) ));
			}
		}

		// extract params if we found something
		if ( found != FOUND_ILLEGAL )
		{
			if ( (found != FOUND_EMPTY) && (found != FOUND_TOO_LOW) )
			{
				// take item params
				req.m_Template  = nearest->second.m_Template;
				req.m_ItemPower = nearest->first;
				req.m_Finish    = nearest->second.m_Variation;
				gppcontentf(( "Found pcontent: template = '%s', power = %s, variation = '%s'\n",
							  req.m_Template->GetName(), req.m_ItemPower.ToString().c_str(), req.m_Finish.c_str() ));

				// take armor params
				if ( req.m_BasicType == PT_ARMOR )
				{
					// remember old ones first
					originalMaterial  = req.m_ArmorMaterial;
					originalSkillType = req.m_ArmorSkillType;

					// now get traits
					GetArmorTraitsFromTemplateName( req.m_ArmorMaterial, req.m_ArmorSkillType, req.m_Template->GetName() );
					gppcontentf(( "Armor traits: material = '%s', skill type = '%s'\n",
								  req.m_ArmorMaterial.c_str(), req.m_ArmorSkillType.c_str() ));
				}
			}
			else
			{
				// fuck!
				found = FOUND_FAILURE;
			}
		}
	}

// Choose modifiers.

	// create and attach modifiers
	if ( chooseModifiers )
	{
		gppcontentf(( "Choosing modifiers\n" ));
		GPDEV_ONLY( ReportSys::AutoIndent autoIndent( GetPContentContext() ) );

		// choose prefix, suffix, or both
		float* prefixPower = NULL, * suffixPower = NULL;
		if ( req.m_ReqModifierCount == 1 )
		{
			if ( Random( 1L ) && !m_ModifierPrefixDb.empty() )
			{
				prefixPower = &modifierPower[ 0 ];
			}
			else
			{
				suffixPower = &modifierPower[ 0 ];
			}
		}
		else
		{
			gpassert( req.m_ReqModifierCount == MAX_MODIFIERS );
			prefixPower = &modifierPower[ 0 ];
			suffixPower = &modifierPower[ 1 ];
		}

		bool success = true;

		// do prefix
		if ( prefixPower != NULL )
		{
			const ModifierEntry* modifier = ChooseModifier( true, *prefixPower, req );
			req.m_Modifiers[ 0 ] = modifier;

			if ( modifier == NULL )
			{
				success = false;
			}
		}

		// do suffix
		if ( suffixPower != NULL )
		{
			const ModifierEntry* modifier = ChooseModifier( false, *suffixPower, req, req.m_Modifiers[ 0 ] );
			req.m_Modifiers[ 1 ] = modifier;

			if ( modifier == NULL )
			{
				success = false;
			}
		}

		// on failure, have to retry...
		if ( !success )
		{
			gppcontentf(( "At least one modifier could not be found, attempting another random query from the top\n" ));
			GPDEV_ONLY( ::InterlockedIncrement( (LONG*)&m_StatModifiersNotFound ) );

			// clean out old ones
			req.m_Template = NULL;
			::ZeroObject( req.m_Modifiers );

			// restore original material/skill types
			req.m_ArmorSkillType = originalSkillType;
			req.m_ArmorMaterial  = originalMaterial;

			// try again
			if ( req.m_AttemptCount <= 5 )
			{
				return ( ProcessRequest( req ) );
			}
			else
			{
				// fuck!
				gppcontenterrorf((
						"TELL JAKE: I tried 5 freakin' times to find a modifier "
						"that fits from query '%s' but it never matches for "
						"some reason!\n",
						req.m_QueryText.c_str() ));
				found = FOUND_FAILURE;
			}
		}
	}

// Done

	// report failure
	if ( found == FOUND_FAILURE )
	{
		gpdevreportf( (ReportSys::IsContextEnabled( &GetPContentErrorContext() ) ? GetPContentErrorContext() : ReportSys::GetWarningContext()),
				 ( "TELL ERIK TO TELL GREG TO TELL JAKE: I just "
				   "couldn't find anything given the query '%s'. "
				   "Somehow this ended up with the following query "
				   "parameters:\n"
				   "\n"
				   "\ttraitDb        = %s\n"
				   "\titemPowerMin   = %.1f\n"
				   "\titemPowerMax   = %s\n"
				   "\tfuzziness      = %.1f\n"
				   "\tspecificType   = %s\n"
				   "\tbasicType      = %s\n"
				   "\tmodifierCount  = %d\n"
				   "\tarmorMaterial  = %s\n"
				   "\tarmorSkillType = %s\n"
				   "\tfinish         = %s\n"
				   "\ttype           = %s\n"
				 , req.m_QueryText.c_str()
				 , req.m_TemplateDbIter->first.c_str()
				 , req.m_ItemPower.m_Min
				 , (req.m_ItemPower.m_Max == FLOAT_MAX) ? "(inf)" : gpstringf( "%.1f", req.m_ItemPower.m_Max ).c_str()
				 , fuzziness
				 , ::ToString( req.m_SpecificType )
				 , ::ToString( req.m_BasicType )
				 , req.m_ReqModifierCount
				 , req.m_ArmorMaterial.c_str()
				 , req.m_ArmorSkillType.c_str()
				 , req.m_Finish.c_str()
				 , ToCommaString( req.m_DbType ).c_str()
				 ));
	}

	// update stats
#	if !GP_RETAIL
	gpassert( m_CallDepth > 0 );
	double offset = PreciseSubtract(::GetSystemSeconds(), startTime);
	if ( --m_CallDepth == 0 )
	{
		++m_StatRequests;
		m_StatTotalTime += offset;
	}
#	endif // !GP_RETAIL

	// done
	gppcontentf(( "\nProcessing completed in %.3f msec\n", offset * 1000.0f ));

	// done
	return ( req.m_Template != NULL );
}

const GoDataTemplate* PContentDb :: FindTemplateByPContent( PContentReq& req, const char* query, bool processMacros )
{
	const GoDataTemplate* tmpl = NULL;

	// $ note here that if the type is invalid, it may be that this pcontent is
	//   being used to generate a simple item with a fixed random seed or
	//   something. in that case, just use the template that is there. it will
	//   be null if it had failed anyway.

	if ( ParseQuery( req, query, processMacros ) && ((req.m_SpecificType == PT_INVALID) || ProcessRequest( req )) )
	{
		tmpl = req.m_Template;
	}

	return ( tmpl );
}

int PContentDb :: FindTemplatesByTraits( TemplateSet& coll, const char* traits, float minPower, float maxPower ) const
{
	int oldSize = coll.size();

	// change "*" to empty to match all case
	gpassert( traits != NULL );
	if ( (traits[ 0 ] == '*') && (traits[ 1 ] == '\0') )
	{
		traits = "";
	}

	// find this trait
	TraitTemplateDb::const_iterator traitDb = m_TraitTemplateDb.find( traits );
	if ( traitDb != m_TraitTemplateDb.end() )
	{
		// zoom in
		PowerTemplateDb::const_iterator i,
										ibegin = traitDb->second.lower_bound( minPower ),
										iend   = traitDb->second.upper_bound( maxPower );
		for ( i = ibegin ; i != iend ; ++i )
		{
			coll.insert( i->second.m_Template );
		}
	}
	else
	{
		// assume it's a template name and go from there (ignore power if asked for specific)
		const GoDataTemplate* found = gContentDb.FindTemplateByName( traits );
		if ( found != NULL )
		{
			coll.insert( found );
		}
	}

	return ( coll.size() - oldSize );
}

bool PContentDb :: ApplyPContentRequest( Go* go, const PContentReq& req )
{
	gpassert( go != NULL );
	bool success = true;

	go->SetIsPContentGenerated();

	// apply finish
	if ( !req.m_Finish.empty() )
	{
		FastFuelHandle pcontent = go->GetDataTemplate()->GetFuelHandle().GetChildNamed( "pcontent" );
		if ( pcontent )
		{
			FastFuelHandle finishBlock = pcontent.GetChildNamed( req.m_Finish );
			if ( finishBlock )
			{
				if ( go->HasGui() )
				{
					// override inv width!
					int invWidth;
					if ( finishBlock.Get( "inventory_width", invWidth ) )
					{
						go->GetGui()->SetInventoryWidth( invWidth );
					}

					// override inv height!
					int invHeight;
					if ( finishBlock.Get( "inventory_height", invHeight ) )
					{
						go->GetGui()->SetInventoryHeight( invHeight );
					}

					// override inv icon!
					gpstring invIcon;
					if ( finishBlock.Get( "inventory_icon", invIcon ) )
					{
						go->GetGui()->SetInventoryIcon( invIcon );
					}

					// override equip requirements!
					gpstring equipReq;
					if ( finishBlock.Get( "equip_requirements", equipReq ) )
					{
						go->GetGui()->SetEquipRequirements( equipReq );
					}

					// set variation
					go->GetGui()->SetVariation( finishBlock.GetName() );
				}

				if ( go->HasAspect() )
				{
					// override mesh!
					gpstring model;
					if ( finishBlock.Get( "model", model ) )
					{
						go->GetAspect()->SetCurrentAspectSimple( model );
					}

					// override texture!
					gpstring texture;
					if ( finishBlock.Get( "texture", texture ) )
					{
						go->GetAspect()->SetCurrentAspectTexture( 0, texture );
					}
				}

				GoAttack* attack = go->QueryAttack();
				if ( attack != NULL )
				{
					// override damage!
					float damage;
					if ( finishBlock.Get( "damage_min", damage ) )
					{
						attack->SetDamageMinNatural( damage );
					}
					if ( finishBlock.Get( "damage_max", damage ) )
					{
						attack->SetDamageMaxNatural( damage );
					}
				}

				GoDefend* defend = go->QueryDefend();
				if ( defend != NULL )
				{
					// override defense!
					float defense;
					if ( finishBlock.Get( "defense", defense ) )
					{
						defend->SetDefenseNatural( defense );
					}

					// override type!
					bool rebuild = false;
					gpstring type;
					if ( finishBlock.Get( "armor_type", type ) && !defend->GetArmorType().same_no_case( type ) )
					{
						defend->SetArmorType( type );
						rebuild = true;
					}

					// override style!
					gpstring style;
					if ( finishBlock.Get( "armor_style", style ) && !defend->GetArmorStyle().same_no_case( style ) )
					{
						defend->SetArmorStyle( style );
						rebuild = true;
					}

					// rebuild armor?
					if ( rebuild )
					{
						GoBody::InstantiateDeformableUnwornArmor( go );
					}
				}
			}
		}
	}

	// apply modifiers
	if ( req.HasModifiers() )
	{
		// make sure it has magic to store enchantments
		if ( go->AddNewComponent( "magic" ) )
		{
			const ModifierEntry* prefix = req.m_Modifiers[ 0 ];
			const ModifierEntry* suffix = req.m_Modifiers[ 1 ];

			// set it for the magic component
			if ( prefix != NULL )
			{
				go->GetMagic()->SetPrefixModifierName( prefix->m_Name );
			}
			if ( suffix != NULL )
			{
				go->GetMagic()->SetSuffixModifierName( suffix->m_Name );
			}

			// get screen name tags
			const wchar_t* strPre = prefix ? prefix->m_ScreenName.c_str() : NULL;
			const wchar_t* strMid = go->GetCommon()->GetScreenName();
			const wchar_t* strSuf = suffix ? suffix->m_ScreenName.c_str() : NULL;

			// handle any localization gender tag cases
			if ( LocMgr::DoesSingletonExist() && !go->GetCommon()->GetNameGender().empty() )
			{
				if ( strPre )
				{
					ModifierEntry::TagToScreenNameMap::const_iterator iTag = prefix->m_TaggedScreenNames.find( go->GetCommon()->GetNameGender() );
					if ( iTag != prefix->m_TaggedScreenNames.end() )
					{
						strPre = (*iTag).second;
					}
#					if !GP_RETAIL
					else
					{
						gperrorf(( "Object: %S, Gender tag: \"%S\" not found for prefix: %S, using default (first one found)",
								   go->GetCommon()->GetScreenName().c_str(),
								   go->GetCommon()->GetNameGender().c_str(),
								   prefix->m_ScreenName.c_str() ));
					}
#					endif
				}
				if ( strSuf )
				{
					ModifierEntry::TagToScreenNameMap::const_iterator iTag = suffix->m_TaggedScreenNames.find( go->GetCommon()->GetNameGender() );
					if ( iTag != suffix->m_TaggedScreenNames.end() )
					{
						strSuf = (*iTag).second;
					}
#					if !GP_RETAIL
					else
					{
						gperrorf(( "Object: %S, Gender tag: \"%S\" not found for suffix: %S, using default (first one found)",
								   go->GetCommon()->GetScreenName().c_str(),
								   go->GetCommon()->GetNameGender().c_str(),
								   suffix->m_ScreenName.c_str() ));
					}
#					endif
				}
			}

			// both?
			if ( strPre && strSuf )
			{
				go->GetCommon()->SetScreenName( gpwstringf( L"%s %s %s", m_bPrefixFirst ? strPre : strMid, m_bPrefixFirst ? strMid : strPre, strSuf ) );
			}
			else if ( strPre && !strSuf )
			{
				go->GetCommon()->SetScreenName( gpwstringf( L"%s %s", m_bPrefixFirst ? strPre : strMid, m_bPrefixFirst ? strMid : strPre ) );
			}
			else if ( !strPre && strSuf )
			{
				go->GetCommon()->SetScreenName( gpwstringf( L"%s %s", strMid, strSuf ) );
			}

			// add enchantments
			go->GetMagic()->MakeEnchantments();
			const ModifierEntry* const* i, * const* ibegin = req.m_Modifiers, * const* iend = ARRAY_END( req.m_Modifiers );
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( *i != NULL )
				{
					go->GetMagic()->GetEnchantmentStorage().AddEnchantments( *(*i)->m_Enchantments );
				}
			}

			// add requirements if any
			if ( go->HasGui() )
			{
				if ( prefix )
				{
					go->GetGui()->AddEquipRequirements( prefix->GetEquipRequirements() );
				}
				if ( suffix )
				{
					go->GetGui()->AddEquipRequirements( suffix->GetEquipRequirements() );
				}
			}
		}
		else
		{
			gperror( "Unable to create 'magic' Go component for modifiers\n" );
			success = false;
		}
	}

	// update gold value
	if ( go->HasAspect() && (req.m_BasicType != PT_INVALID) )
	{
		// calc item power from pure native values, not from modified item power
		float itemPowerForGold = req.m_ItemPower;
		float minPowerForGold = req.m_ItemPower;
		float maxPowerForGold = req.m_ItemPower;
		bool isTwoHanded = false;
		gpstring subType, length;
		if ( req.m_BasicType == PT_WEAPON )
		{
			itemPowerForGold = (go->GetAttack()->GetDamageMinNatural() + go->GetAttack()->GetDamageMaxNatural()) * 0.5f;
			if ( go->GetAttack()->GetIsTwoHanded() )
			{
				isTwoHanded = true;
			}

			minPowerForGold = go->GetAttack()->GetDamageMinNatural();
			maxPowerForGold = go->GetAttack()->GetDamageMaxNatural();

			// have to re-query our weapon-specific traits
			GetWeaponTraits( subType, length, go->GetAttack()->GetAttackClass(), go->GetTemplateName() );
		}
		else if ( req.m_BasicType == PT_ARMOR )
		{
			itemPowerForGold = go->GetDefend()->GetNaturalDefense();
			subType = req.m_ArmorSkillType;
		}

		// item
		int gold = CalcGoldValue(
				req.m_BasicType,
				req.m_SpecificType,
				minPowerForGold,
				maxPowerForGold,
				itemPowerForGold,
				req.m_Modifiers[ 0 ] ? req.m_Modifiers[ 0 ]->m_Power : 0,
				req.m_Modifiers[ 1 ] ? req.m_Modifiers[ 1 ]->m_Power : 0,
				isTwoHanded,
				subType,
				length,
				req.m_ArmorMaterial );
		go->GetAspect()->AddGoldValue( gold );

		// modifiers
		const ModifierEntry* const* i, * const* ibegin = req.m_Modifiers, * const* iend = ARRAY_END( req.m_Modifiers );
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( *i != NULL )
			{
				go->GetAspect()->AddGoldValue( (*i)->m_GoldValue );
			}
		}
	}

	// update power level
	if ( go->HasGui() )
	{
		// item power
		float power = req.m_ItemPower;

		// add in modifier power
		const ModifierEntry* const* i, * const* ibegin = req.m_Modifiers, * const* iend = ARRAY_END( req.m_Modifiers );
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( *i != NULL )
			{
				power += (*i)->m_Power;
			}
		}

		// accept
		go->GetGui()->SetItemPower( power );
	}

	// done
	return ( success );
}

bool PContentDb :: CalcAndApplyPContentRules( Go* go )
{
	bool success = true;

	if ( go->HasGui() && !::IsWorldEditMode() )
	{
		ItemTraits itemTraits;
		if ( QueryItemTraits( itemTraits, go->GetDataTemplate(), PT_INVALID, true ) )
		{
			PContentReq req;
			req.m_SpecificType = itemTraits.m_SpecificType;
			req.m_BasicType    = ::ToBasicPContentType( itemTraits.m_SpecificType );
			req.m_ItemPower    = itemTraits.m_ItemPower;

			if ( req.m_BasicType == PT_ARMOR )
			{
				if ( !GetArmorTraitsFromTemplateName( req.m_ArmorMaterial, req.m_ArmorSkillType, go->GetTemplateName() ) )
				{
					success = false;
				}
			}

			if ( success )
			{
				ApplyPContentRequest( go, req );
			}
		}
		else
		{
			success = false;
		}
	}

	return ( success );
}

#if !GP_RETAIL

void PContentDb :: Dump( ReportSys::ContextRef ctx ) const
{
	ReportSys::AutoReport autoReport( ctx );
	gpreportf( ctx, ( "*** CONTENTS OF PCONTENTDB ***\n" ));

	{
		ReportSys::AutoIndent autoIndent( ctx );
		gpreportf( ctx, ( "\n*** Statistics ***\n\n" ));
		DumpStats( ctx );
	}

	{
		ReportSys::AutoIndent autoIndent( ctx );
		gpreportf( ctx, ( "\n*** Macro Database ***\n\n" ));

		StringDb::const_iterator i, ibegin = m_MacroDb.begin(), iend = m_MacroDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpreportf( ctx, ( "%s -> %s\n", i->first.c_str(), i->second.c_str() ));
		}
	}

	{
		ReportSys::AutoIndent autoIndent( ctx );
		gpreportf( ctx, ( "\n*** Trait Power Template Databases ***\n\n" ));

		TraitTemplateDb::const_iterator i, ibegin = m_TraitTemplateDb.begin(), iend = m_TraitTemplateDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpreportf( ctx, ( "%s\n\n", i->first.c_str() ) );
			ReportSys::AutoIndent autoIndent( ctx );

			PowerTemplateDb::const_iterator j, jbegin = i->second.begin(), jend = i->second.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				const GoDataTemplate* godt = j->second.m_Template;
				const GoDataComponent* common = godt->FindComponentByName( "common" );
				gpstring screenName;
				if_gpassert( common != NULL )
				{
					gpverify( common->GetRecord()->Get( "screen_name", screenName ) );
				}

				gpreportf( ctx, ( "%.1f -> %s, screen_name = '%s', modifier_min/max = %.1f/%s, modifier_count_min/max = %d/%s",
								  j->first, godt->GetName(), screenName.c_str(),
								  j->second.m_ModifierMin,
								  (j->second.m_ModifierMax == FLOAT_MAX) ? "(inf)" : gpstringf( "%.1f", j->second.m_ModifierMax ).c_str(),
								  j->second.m_ModifierCountMin,
								  (j->second.m_ModifierCountMax == INT_MAX) ? "(inf)" : gpstringf( "%d", j->second.m_ModifierCountMax ).c_str() ));
				if ( !j->second.m_Variation.GetString().empty() )
				{
					gpreportf( ctx, ( ", variation = %s", j->second.m_Variation.c_str() ));
				}
				else
				{
					gpreport( ctx, ", variation = (base)" );
				}
				gpreport( ctx, "\n" );
			}
			gpreport( ctx, "\n" );
		}
	}

	{
		ReportSys::AutoIndent autoIndent( ctx );
		gpreportf( ctx, ( "\n*** Modifier Database ***\n\n" ));

		ModifierStorageDb::const_iterator i, ibegin = m_ModifierStorageDb.begin(), iend = m_ModifierStorageDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpreportf( ctx, ( "%s (%s): power = %.1f, screen_name = '%S', gold_value = %d\n",
							  i->second.GetName().c_str(), i->second.GetIndexName(),
							  i->second.GetPower(), i->second.GetScreenName().c_str(),
							  i->second.GetGoldValue() ));
			if (   !i->second.GetIncludedPContentTypes().empty()
				|| !i->second.GetIncludedAttackClasses().empty() )
			{
				ReportSys::AutoIndent autoIndent( ctx );
				gpreportf( ctx, ( "* Included Types *\n" ));

				{
					ModifierEntry::PContentTypeColl::const_iterator j,
							jbegin = i->second.GetIncludedPContentTypes().begin(),
							jend   = i->second.GetIncludedPContentTypes().end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						gpreportf( ctx, ( "%s\n", ::ToQueryString( *j ) ));
					}
				}

				{
					ModifierEntry::AttackClassColl::const_iterator j,
							jbegin = i->second.GetIncludedAttackClasses().begin(),
							jend   = i->second.GetIncludedAttackClasses().end();
					for ( j = jbegin ; j != jend ; ++j )
					{
						gpreportf( ctx, ( "%s\n", ::ToQueryString( *j ) ));
					}
				}
			}
			if ( !i->second.GetExclusions().empty() )
			{
				ReportSys::AutoIndent autoIndent( ctx );
				gpreportf( ctx, ( "* Excluded Modifiers and Templates *\n" ));

				ModifierEntry::StringSet::const_iterator j,
						jbegin = i->second.GetExclusions().begin(), jend = i->second.GetExclusions().end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					gpreportf( ctx, ( "%s\n", j->c_str() ));
				}
			}
		}
	}

	{
		ReportSys::AutoIndent autoIndent( ctx );
		gpreportf( ctx, ( "\n*** Melee types ***\n\n" ));

		StringColl::const_iterator i, ibegin = m_MeleeTypesNoStaff.begin(), iend = m_MeleeTypesNoStaff.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpreportf( ctx, ( "%s\n", i->c_str() ));
		}

		if ( m_TraitTemplateDb.find( "staff" ) != m_TraitTemplateDb.end() )
		{
			gpreport( ctx, "staff\n" );
		}
	}

	{
		ReportSys::AutoIndent autoIndent( ctx );

		gpreportf( ctx, ( "\n*** Ranged types ***\n\n" ));

		StringColl::const_iterator i, ibegin = m_RangedTypes.begin(), iend = m_RangedTypes.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			gpreportf( ctx, ( "%s\n", i->c_str() ));
		}
	}
}

inline double GetPercentFail( int x, int total )
{
	gpassert( total != 0 );
	return ( 100.0 * (((double)x / (double)total) - 1.0) );
}

inline double GetPercent( int x, int total )
{
	gpassert( total != 0 );
	return ( 100.0 * ((double)x / (double)total) );
}

void PContentDb :: DumpStats( ReportSys::ContextRef ctx ) const
{
	if ( m_StatRequests > 0 )
	{
		int templateCount = 0;
		TraitTemplateDb::const_iterator i, ibegin = m_TraitTemplateDb.begin(), iend = m_TraitTemplateDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			templateCount += i->second.size();
		}

		gpreportf( ctx, ( "Requests made             : %d\n"
						  "Request iterations made   : %d (%.1f%% failure rate)\n"
						  "Finds over the max        : %d (%.1f%% of total)\n"
						  "Finds below the min       : %d (%.1f%% of total)\n"
						  "Finds over max -> special : %d (%.1f%% of total)\n"
						  "Retries from mods too low : %d (%.1f%% of total)\n"
						  "Retries from mods too high: %d (%.1f%% of total)\n"
						  "Overflows into modifiers  : %d (%.1f%% of total)\n"
						  "Modifier(s) not found     : %d (%.1f%% of total)\n"
						  "Total time in requester   : %.2f sec\n"
						  "Average time per request  : %.2f msec\n"
						  "Total traits              : %d\n"
						  "Total entries in trait db : %d\n"
						  "Total modifiers           : %d\n"
						, m_StatRequests
						, m_StatRequestIterations, GetPercentFail( m_StatRequestIterations, m_StatRequests )
						, m_StatLowFinds,          GetPercent    ( m_StatLowFinds,          m_StatRequests )
						, m_StatHighFinds,         GetPercent    ( m_StatHighFinds,         m_StatRequests )
						, m_StatHighSubstitutions, GetPercent    ( m_StatHighSubstitutions, m_StatRequests )
						, m_StatModifiersTooLow,   GetPercent    ( m_StatModifiersTooLow,   m_StatRequests )
						, m_StatModifiersTooHigh,  GetPercent    ( m_StatModifiersTooHigh,  m_StatRequests )
						, m_StatOverflowIntoMods,  GetPercent    ( m_StatOverflowIntoMods,  m_StatRequests )
						, m_StatModifiersNotFound, GetPercent    ( m_StatModifiersNotFound, m_StatRequests )
						, m_StatTotalTime
						, 1000.0 * (m_StatTotalTime / m_StatRequests)
						, m_TraitTemplateDb.size()
						, templateCount
						, m_ModifierStorageDb.size()
						  ));
	}
	else
	{
		gpreportf( ctx, ( "No statistics gathered yet - no pcontent requests have been made\n" ));
	}
}

FEX void DumpPContentDb( void )
{
	ReportSys::LocalContext localCtx( new ReportSys::RetryFileSink( "pcontent.log", true ), true );
	gPContentDb.Dump( localCtx );
}

FEX void DumpPContentDbStats( void )
{
	gPContentDb.DumpStats();
}

FEX void ProfilePContentDb( const char* query, int repeat )
{
	double time = ::GetSystemSeconds();

	for ( int i = 0 ; i < repeat ; ++i )
	{
		PContentReq pcontentReq;
		gPContentDb.FindTemplateByPContent( pcontentReq, query );
	}

	time = PreciseSubtract( ::GetSystemSeconds(), time );
	gpgenericf(( "Query '%s' for %d iterations took %f seconds, or %f ms per iteration\n",
				 query, repeat, time, (time * 1000.0 / repeat) ));
}

#endif // !GP_RETAIL

static float getMultiplier( Go* forGo, const stdx::fast_vector <float> & coll )
{
	float mult = 1.0f;

	if ( ::IsMultiPlayer() && !forGo->IsAllClients() )
	{
		int index = forGo->GetMpPlayerCount() - 1;
		if ( !coll.empty() )
		{
			::clamp_min_max( index, 0, (int)coll.size() - 1 );
			mult = coll[ index ];
			::clamp_min( mult, 0.0f );
		}
	}

	return ( mult );
}

float PContentDb :: GetInvMpAllChanceMultiplier( Go* forGo ) const
{
	return ( getMultiplier( forGo, m_MpLootScaling.m_AllChanceMultiplier ) );
}

float PContentDb :: GetInvMpOneOfChanceMultiplier( Go* forGo ) const
{
	return ( getMultiplier( forGo, m_MpLootScaling.m_OneOfChanceMultiplier ) );
}

void PContentDb :: GetInvMpGoldMinMaxMultipliers( Go* forGo, float& minMult, float& maxMult ) const
{
	minMult = getMultiplier( forGo, m_MpLootScaling.m_MinMaxGold.m_MinCountMultiplier );
	maxMult = getMultiplier( forGo, m_MpLootScaling.m_MinMaxGold.m_MaxCountMultiplier );
}

bool PContentDb :: GetInvMpMinMaxMultipliers( Go* forGo, ePContentType pt, eAttackClass ac, float& minMult, float& maxMult ) const
{
	if ( ::IsMultiPlayer() && !forGo->IsAllClients() )
	{
		MinMaxColl::const_iterator i, ibegin = m_MpLootScaling.m_MinMaxColl.begin(), iend = m_MpLootScaling.m_MinMaxColl.end();

		// try attack class first, it's the most specific
		if ( ac != AC_BEASTFU )
		{
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i->m_AttackClass == ac )
				{
					minMult = getMultiplier( forGo, i->m_MinCountMultiplier );
					maxMult = getMultiplier( forGo, i->m_MaxCountMultiplier );
					return ( true );
				}
			}
		}

		// try specific pcontent type next
		if ( pt != PT_INVALID )
		{
			// specific
			for ( i = ibegin ; i != iend ; ++i )
			{
				if ( i->m_PContentType == pt )
				{
					minMult = getMultiplier( forGo, i->m_MinCountMultiplier );
					maxMult = getMultiplier( forGo, i->m_MaxCountMultiplier );
					return ( true );
				}
			}

			// now check basic type
			ePContentType bpt = ::ToBasicPContentType( pt );
			if ( bpt != pt )
			{
				for ( i = ibegin ; i != iend ; ++i )
				{
					if ( i->m_PContentType == bpt )
					{
						minMult = getMultiplier( forGo, i->m_MinCountMultiplier );
						maxMult = getMultiplier( forGo, i->m_MaxCountMultiplier );
						return ( true );
					}
				}
			}
		}
	}

	minMult = maxMult = 1.0f;
	return ( false );
}

void PContentDb :: PlaceObjectsInArray( const SiegePos& origin, const GoidColl& goids, float distance )
{
	int dir = 0;
	int maxWidth = 1;
	int curWidth = 0;
	bool first = true;

	if ( ::IsZero( distance ) )
	{
		distance = 2.0f;
	}

	SiegePos source( origin );

	GoidColl::const_iterator i, ibegin = goids.begin(), iend = goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		SiegePos pos( source );

		if ( i != ibegin )
		{
			if ( ++curWidth == maxWidth )
			{
				curWidth = 0;

				if ( !first )
				{
					++maxWidth;
				}
				first = !first;

				if ( ++dir == 4 )
				{
					dir = 0;
				}
			}
		}

		switch ( dir )
		{
			case ( 0 ):  source.pos.x += distance;  break;
			case ( 1 ):  source.pos.z -= distance;  break;
			case ( 2 ):  source.pos.x -= distance;  break;
			case ( 3 ):  source.pos.z += distance;  break;
		}

		GoHandle go( *i );
		go->GetPlacement()->SetPosition( pos );
	}
}

void PContentDb :: PlaceObjectsInSpew( const SiegePos& origin, const GoidColl& goids )
{
	GoidColl::const_iterator i, ibegin = goids.begin(), iend = goids.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		Quat quat;
		SiegePos pos;

		if ( gAIQuery.FindSpotRelativeToSource( &origin,
												gWorldOptions.GetSpotWalkMaskRadius(),
												gWorldOptions.GetDropItemsDistanceMin(),
												gWorldOptions.GetDropItemsDistanceMax(),
												gWorldOptions.GetInventorySpewVerticalConstraint(),
												false,
												pos ) )
		{
			gAIQuery.GetRandomOrientation( quat );
			GoHandle go( *i );
			go->GetPlacement()->SetPlacement( pos, quat );
		}
	}
}

const PContentDb::ModifierEntry* PContentDb :: ChooseModifier(
		bool prefix, float modifierPower, const PContentReq& req, const ModifierEntry* other )
{
	const ModifierEntry* modifier = NULL;

	// choose proper index
	const ModifierPowerDb* db = prefix ? &m_ModifierPrefixDb : &m_ModifierSuffixDb;
	if ( !db->empty() )
	{
		gppcontentf(( "Chose a %s\n", prefix ? "prefix" : "suffix" ));

		// choose a modifier
		float fuzziness = CalcModifierPowerFuzziness( ::ToBasicPContentType( req.m_BasicType ), modifierPower, req.GetDbType() );
		gppcontentf(( "Calculated modifer power fuzziness of %.2f\n", fuzziness ));
		ModifierPowerDb::const_iterator nearest;
		eFoundType found = FindNearest( nearest, *db, modifierPower, modifierPower, fuzziness, ModifierFilter( req, other ) );
		gppcontentf(( "Finding nearest match with modifier power %.2f, query returned %s\n",
					  modifierPower, ToString( found ) ));

		// only accept if >= valid
		if ( IsFound( found ) && (found != FOUND_LOW) )
		{
			// take it
			modifier = &nearest->second->second;
		}
	}

	// done
	return ( modifier );
}

const gpstring& PContentDb :: ChooseRandomTrait( float reqPower, ePContentType specificType ) const
{
	// grab the right collection
	const StringColl& coll = (specificType == PT_MELEE) ? m_MeleeTypesNoStaff : m_RangedTypes;
	gpassert( (specificType == PT_MELEE) || (specificType == PT_RANGED) );

	// first, figure out how many are eligible
	stdx::fast_vector <int> eligible;
	StringColl::const_iterator i, ibegin = coll.begin(), iend = coll.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		bool ok = false;

		// first check minimum
		TraitTemplateDb::const_iterator found = m_TraitTemplateDb.find( *i );
		gpassert( found != m_TraitTemplateDb.end() );

		// now check lowest available of this type against the mid
		gpassert( !found->second.empty() );
		float minPower = found->second.begin()->first;
		if ( minPower <= reqPower )
		{
			ok = true;
		}
		else if ( specificType == PT_MELEE )
		{
			// see if it's infinite
			ok = m_InfiniteMeleeWeapons.find( i->GetString() ) != m_InfiniteMeleeWeapons.end();
		}
		else
		{
			// see if it's infinite
			ok = m_InfiniteRangedWeapons.find( i->GetString() ) != m_InfiniteRangedWeapons.end();
		}

		// now add it
		if ( ok )
		{
			eligible.push_back( i - ibegin );
		}
		else
		{
			gppcontentf(( "Rejected trait '%s' because requested avg power %s is lower than non-infinite trait min power %s\n",
						  i->c_str(),
						  (reqPower == FLOAT_MAX) ? "inf" : gpstringf( "%.2f", reqPower ).c_str(),
						  (minPower == FLOAT_MAX) ? "inf" : gpstringf( "%.2f", minPower ).c_str() ));
		}
	}

	// any remaining?
	if ( !eligible.empty() )
	{
		// pick one at random
		int random = Random( eligible.size() - 1 );
		return ( coll[ eligible[ random ] ] );
	}
	else
	{
		// none
		return ( gpstring::EMPTY );
	}
}

//////////////////////////////////////////////////////////////////////////////
// class PContentDb::ModifierEntry implementation

DWORD PContentDb::ModifierEntry :: GetToolTipColor( void ) const
{
	return ( gUIShell.GetTipColor( m_ToolTipColorName.c_str(), 0 ) );
}

bool PContentDb::ModifierEntry :: Init( void )
{
	bool success = true;

	// copy from base first for specialization
	if ( m_Base != NULL )
	{
		// preserve local values
		ModifierEntry* base = m_Base;
		FastFuelHandle fuel = m_Fuel;
		gpstring       name = m_Name;

		// copy
		*this = *m_Base;

		// restore local values
		m_Base = base;
		m_Fuel = fuel;
		m_Name = name;
	}

	// get type
	gpstring type;
	if ( m_Fuel.Get( "type", type, m_Base != NULL ) )
	{
		if ( type.same_no_case( "prefix" ) )
		{
			m_Type = TYPE_PREFIX;
		}
		else if ( type.same_no_case( "suffix" ) )
		{
			m_Type = TYPE_SUFFIX;
		}
		else
		{
			gperrorf(( "Invalid modifier type '%s' at '%s'\n", type.c_str(), m_Fuel.GetAddress().c_str() ));
			success = false;
		}
	}

	// get optional power
	m_Fuel.Get( "power", m_Power, m_Base != NULL );

	// get screen name
	gpstring screenName;
	if ( m_Fuel.Get( "screen_name", screenName, m_Base != NULL ) )
	{
		m_ScreenName = ReportSys::TranslateW( screenName );

		// if this is a loc version let's see if this string has any genders
		// For example, does the modifier string look like this:
		//		strPre = <fs>super,<fm>supor,<m>supir;
		if ( LocMgr::DoesSingletonExist() )
		{
			gpstring sModifier = ToAnsi( m_ScreenName );
			int numGender = stringtool::GetNumDelimitedStrings( sModifier, ',' );
			if ( numGender > 0 )
			{
				// it has multiple genders, let see what they are
				gpstring sValue;
				gpwstring swValue, swTag;
				for ( int iGender = 0; iGender != numGender; ++iGender )
				{
					stringtool::GetDelimitedValue( sModifier, ',', iGender, sValue, false );
					swValue = ToUnicode( sValue );

					// clean up the screen name of the tag ( if any ) and retrieve that tag
					if ( !ExtractFirstTagFromString( swValue, swTag ) )
					{
						// No tag, just set it to default, the loc guys will have to police themselves
						m_ScreenName = swValue;
					}
					else if ( iGender == 0 )
					{
						// default fallback
						m_ScreenName = swValue;
					}

					if ( !swTag.empty() )
					{
						// all tags get inserted into the list
						m_TaggedScreenNames.insert( std::make_pair( swTag, swValue ) );
					}
				}
			}
						
		}
	}

	// get equip requirements
	gpstring equipRequirements;
	if ( m_Fuel.Get( "equip_requirements", equipRequirements ) )
	{
		m_EquipRequirements = equipRequirements;
	}

	// get gold value
	m_Fuel.Get( "gold_value", m_GoldValue );

	// get tooltip weight
	m_Fuel.Get( "tooltip_weight", m_ToolTipWeight );

	// get tooltip color
	m_ToolTipColorName = m_Fuel.GetString( "tooltip_color" );

	// get enchantments
	m_Enchantments = gContentDb.AddOrFindEnchantmentStorage( m_Fuel, m_Enchantments.Get() );
	if ( !m_Enchantments )
	{
		success = false;
	}

	// get included types
	gpstring inclusions;
	if ( m_Fuel.Get( "object_types", inclusions ) && !inclusions.empty() )
	{
		m_PContentTypeColl.clear();
		m_AttackClassColl .clear();

		stringtool::Extractor extractor( ",", inclusions );
		gpstring temp;
		while ( extractor.GetNextString( temp ) )
		{
			ePContentType pt = PT_INVALID;
			eAttackClass  ac = AC_BEASTFU;

			if ( ::FromQueryString( temp, pt ) && (pt != PT_INVALID) )
			{
				m_PContentTypeColl.push_back( pt );
			}
			else if ( ::FromQueryString( temp, ac ) && (ac != AC_BEASTFU) )
			{
				m_AttackClassColl.push_back( ac );
			}
			else
			{
				gperrorf(( "Invalid object type '%s' at '%s'\n", temp.c_str(), m_Fuel.GetAddress().c_str() ));
			}
		}
	}

	// get excluded modifiers and templates
	gpstring exclusions;
	if ( m_Fuel.Get( "exclusions", exclusions ) && !exclusions.empty() )
	{
		m_Exclusions.clear();

		stringtool::Extractor extractor( ",", exclusions );
		gpstring temp;
		while ( extractor.GetNextString( temp ) )
		{
			m_Exclusions.insert( temp );
		}
	}

	// get traits
	gpstring specialType;
	m_Fuel.Get( "special_type", specialType );
	if ( !FromCommaString( specialType, m_DbType ) )
	{
		gperrorf(( "Illegal special_type '%s' at '%s'\n", specialType.c_str(), m_Fuel.GetAddress().c_str() ));
	}

	// done
	return ( success );
}

bool PContentDb::ModifierEntry :: CanCoexistWith( const ModifierEntry* other ) const
{
	gpassert( other != NULL );
	gpassert( m_Type != other->m_Type );

	if ( m_Exclusions.find( other->m_Name ) != m_Exclusions.end() )
	{
		return ( false );
	}

	if ( other->m_Exclusions.find( m_Name ) != other->m_Exclusions.end() )
	{
		return ( false );
	}

	return ( true );
}

bool PContentDb::ModifierEntry :: CanModify( const GoDataTemplate* godt, ePContentType pt, eAttackClass ac ) const
{
	// if we've got the template, check for exclusions based on name
	if ( godt != NULL )
	{
		if ( m_Exclusions.find( godt->GetName() ) != m_Exclusions.end() )
		{
			return ( false );
		}
	}

	// if we have specific pcontent type matches required
	if ( (pt != PT_INVALID) && !m_PContentTypeColl.empty() )
	{
		// test against non-basic type
		if ( IsBasicPContentType( pt ) || !stdx::has_any( m_PContentTypeColl, pt ) )
		{
			// test against basic type
			if ( !stdx::has_any( m_PContentTypeColl, ::ToBasicPContentType( pt ) ) )
			{
				return ( false );
			}
		}
	}

	// if we have an attack class match to worry about
	if ( !m_AttackClassColl.empty() )
	{
		if ( ac != AC_BEASTFU )
		{
			// test against attack class
			if ( !stdx::has_any( m_AttackClassColl, ac ) )
			{
				return ( false );
			}
		}
		else
		{
			// special case here - some things (such as spell books) do not have
			// attack class. so if we *only* have attack class matches, but no
			// pcontent matches, then we have to fail here because we only want
			// to match things that have attack classes.
			if ( m_PContentTypeColl.empty() )
			{
				return ( false );
			}
		}
	}

	// made it!
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
// class PContentDb::Range implementation

PContentDb::Range& PContentDb::Range :: operator -= ( float f )
{
	if ( m_Mid != FLOAT_MAX )
	{
		m_Mid -= f;
		::maximize( m_Mid, 0.0f );
	}
	if ( m_Min != FLOAT_MAX )
	{
		m_Min -= f;
		::maximize( m_Min, 0.0f );
	}
	if ( m_Max != FLOAT_MAX )
	{
		m_Max -= f;
		::maximize( m_Max, 0.0f );
	}
	return ( *this );
}

gpstring PContentDb::Range :: ToString( void ) const
{
	return ( gpstringf( "%s/%s/%s",
			 (m_Min == FLOAT_MAX) ? "inf" : gpstringf( "%.2f", m_Min ).c_str(),
			 (m_Mid == FLOAT_MAX) ? "inf" : gpstringf( "%.2f", m_Mid ).c_str(),
			 (m_Max == FLOAT_MAX) ? "inf" : gpstringf( "%.2f", m_Max ).c_str() ) );
}

//////////////////////////////////////////////////////////////////////////////
// class PContentReq implementation

int PContentReq :: GetModifierCount( void ) const
{
	int count = 0;

	const ModifierEntry* const* i, * const* ibegin = m_Modifiers, * const* iend = ARRAY_END( m_Modifiers );
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( *i != NULL )
		{
			++count;
		}
	}

	return ( count );
}

PContentReq :: PContentReq( void )
{
	m_SpecificType     = PT_INVALID;
	m_BasicType        = PT_INVALID;
	m_Template         = NULL;
	m_TemplateDbIter   = gPContentDb.m_TraitTemplateDb.end();
	m_ReqModifierCount = -1;
	m_RandomSeed       = 0;
	m_DbType           = PContentDb::DB_NORMAL;
	m_NoOrnate         = false;
	m_ForceSeed        = false;
	m_AttemptCount     = 0;

	::ZeroObject( m_Modifiers );
}

void PContentReq :: SetModifier( int index, const ModifierEntry* modifier )
{
	gpassert( (index >= 0) && (index < PContentDb::MAX_MODIFIERS) );
	m_Modifiers[ index ] = modifier;
}

//////////////////////////////////////////////////////////////////////////////
