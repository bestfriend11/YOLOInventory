//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoCore.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoCore.h"

#include "FileSysUtils.h"
#include "FormulaHelper.h"
#include "FuBiBitPacker.h"
#include "FuBiPersist.h"
#include "FuBiTraitsImpl.h"
#include "ContentDb.h"
#include "Enchantment.h"
#include "FormulaHelper.h"
#include "GoActor.h"
#include "GoAspect.h"
#include "GoAttack.h"
#include "GoData.h"
#include "GoDefend.h"
#include "GoDb.h"
#include "GoInventory.h"
#include "GoMagic.h"
#include "GoShmo.h"
#include "GoStash.h"
#include "GoStore.h"
#include "GoSupport.h"
#include "GuiHelper.h"
#include "LocHelp.h"
#include "nema_aspect.h"
#include "nema_blender.h"
#include "Siege_Engine.h"
#include "Siege_Frustum.h"
#include "Trigger_Sys.h"
#include "Ui_Shell.h"
#include "WorldMap.h"
#include "WorldState.h"
#include "WorldTerrain.h"
#include "WorldTime.h"

DECLARE_GO_SERVER_ONLY_COMPONENT( GoCommon, "common" );
DECLARE_GO_SERVER_ONLY_COMPONENT( GoPlacement, "placement" );
DECLARE_GO_COMPONENT( GoGui, "gui" );

//////////////////////////////////////////////////////////////////////////////

const WORD INITIAL_TRIG_NUM_TEMPLATE = 0x0000;
const WORD INITIAL_TRIG_NUM_INSTANCE = 0x8000;

//////////////////////////////////////////////////////////////////////////////
// class Membership implementation

const DWORD MembershipElementBitLength = 32;

Membership::MemberMap Membership::m_MemberMap;
static kerneltool::RwCritical s_MemberMapCritical;

Membership::Membership()
{
	m_Data.clear();
}

Membership::Membership( Membership const & membership )
	: m_Data( membership.m_Data )
{
}

Membership::~Membership()
{
}

void Membership::Add( gpstring const & memberList )
{
	int imax = stringtool::GetNumDelimitedStrings( memberList, ',' );

	for( int i = 0; i != imax; ++i )
	{
		gpstring substring;
		stringtool::GetDelimitedValue( memberList, ',', i, substring );
		PrivateAdd( substring );
	}
}

void Membership::PrivateAdd( gpstring const & member )
{
	// find member in master list, if not found, add it

	DWORD memberId = 0;

	{
		kerneltool::RwCritical::WriteLock writeLock( s_MemberMapCritical );

		MemberMap::iterator i = m_MemberMap.find( member );

		if( i == m_MemberMap.end() )
		{
			memberId = m_MemberMap.size();
			m_MemberMap.insert( MemberMap::value_type( member, memberId ) );
		}
		else
		{
			memberId = (*i).second;
		}
	}

	// find element and bit to set in data for this Set

	DWORD dataElement = memberId/MembershipElementBitLength;

	if( m_Data.size() < dataElement  + 1 )
	{
		while( m_Data.size() < dataElement  + 1 )
		{
			m_Data.push_back( 0 );
		}
	}

	m_Data[dataElement ] = m_Data[dataElement ] | ( 1 << ( memberId % MembershipElementBitLength ) );
}

bool Membership::Contains( gpstring const & member ) const
{
	kerneltool::RwCritical::ReadLock readLock( s_MemberMapCritical );

	MemberMap::iterator i = m_MemberMap.find( member );

	if( i == m_MemberMap.end() )
	{
		return false;
	}
	else
	{
		DWORD memberId = (*i).second;

		DWORD dataElement = memberId/MembershipElementBitLength;

		if( m_Data.size() < dataElement  + 1 )
		{
			return false;
		}
		else
		{
			return ( m_Data[dataElement] & ( 1 << ( memberId % MembershipElementBitLength ) ) ) != 0;
		}
	}
}

bool Membership::ContainsAny( Membership const & membership ) const
{
	for (  SetData::const_iterator a = m_Data.begin(), b = membership.m_Data.begin()
		 ; (a != m_Data.end()) && (b != membership.m_Data.end())
		 ; ++a, ++b )
	{
		if( *a & *b )
		{
			return true;
		}
	}

	return false;
}

bool Membership::ContainsAll( Membership const & membership ) const
{
	if( m_Data.size() < membership.m_Data.size() )
	{
		return false;
	}
	else
	{
		unsigned int imax = min( membership.m_Data.size(), m_Data.size() );

		for( unsigned int i = 0; i != imax; ++i )
		{
			if( ( m_Data[i] & membership.m_Data[i] ) != membership.m_Data[i] )
			{
				return false;
			}
		}
		return true;
	}
}

bool Membership::Xfer( FuBi::PersistContext& persist, const char* key )
{
	return ( persist.XferVector( FuBi::XFER_HEX, key, m_Data ) );
}

void Membership::Xfer( FuBi::BitPacker& packer )
{
	int size = m_Data.size();
	packer.XferCount( size, 2 );
	if ( size > 0 )
	{
		if ( packer.IsSaving() )
		{
			packer.WriteBytes( &*m_Data.begin(), size * sizeof( DWORD ) );
		}
		else
		{
			m_Data.resize( size );
			packer.ReadBytes( &*m_Data.begin(), size * sizeof( DWORD ) );
		}
	}
}

void Membership::StaticInit( void )
{
	m_MemberMap.clear();

	FastFuelHandle fuel( "world:contentdb:membership_seed" );
	if ( fuel )
	{
		FastFuelFindHandle finder( fuel );
		finder.FindFirstValue( "*" );
		gpstring value;
		Membership dummy;
		while ( finder.GetNextValue( value ) )
		{
			// just adding stuff to this dummy membership will seed it fine
			dummy.Add( value );
		}
	}
}

void Membership::StaticXfer( FuBi::PersistContext& persist )
{
	if ( persist.IsRestoring() )
	{
		m_MemberMap.clear();
	}
	persist.XferMap( FuBi::XFER_HEX, "MembershipMap", m_MemberMap );
}

//////////////////////////////////////////////////////////////////////////////
// class GoCommon implementation

GoCommon :: GoCommon( Go* go )
	: Inherited( go )
{
	m_TemplateTriggers = NULL;
	m_InstanceTriggers = NULL;
	m_OkToSelfDestruct = false;
}

GoCommon :: GoCommon( const GoCommon& source, Go* newGo )
	: Inherited( source, newGo )
	, m_ScreenName( source.m_ScreenName )
	, m_Description( source.m_Description )
	, m_Membership( source.GetMembership() )
{
	m_TemplateTriggers = NULL;
	m_InstanceTriggers = NULL;
	m_OkToSelfDestruct = false;

	if ( source.m_TemplateTriggers != NULL )
	{
		m_TemplateTriggers = new trigger::Storage( *source.m_TemplateTriggers, INITIAL_TRIG_NUM_TEMPLATE, GetGo() );
	}

	if ( source.m_InstanceTriggers != NULL )
	{
		m_InstanceTriggers = new trigger::Storage( *source.m_InstanceTriggers, INITIAL_TRIG_NUM_INSTANCE, GetGo() );
	}
}

GoCommon :: ~GoCommon( void )
{
	delete ( m_TemplateTriggers );
	delete ( m_InstanceTriggers );
}

bool GoCommon :: GetIsPContentAllowed( void ) const
{
	static AutoConstQuery <bool> s_Query( "is_pcontent_allowed" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoCommon :: GetAllowModifiers( void ) const
{
	static AutoConstQuery <bool> s_Query( "allow_modifiers" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoCommon :: GetDisplayRollover( void ) const
{
	static AutoConstQuery <bool> s_Query( "rollover_display" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoCommon :: GetDisplayRolloverLife( void ) const
{
	static AutoConstQuery <bool> s_Query( "rollover_life" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoCommon :: GetRolloverHelpKey( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "rollover_help_key" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

#if !GP_RETAIL

const gpstring& GoCommon :: GetDevInstanceText( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "dev_instance_text" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

#endif // !GP_RETAIL

void GoCommon :: SSetScreenName( gpwstring const & name )
{
	CHECK_SERVER_ONLY;

	RCSetScreenName( name );
}

FuBiCookie GoCommon :: RCSetScreenName( const gpwstring& name )
{
	FUBI_RPC_THIS_CALL_RETRY( RCSetScreenName, RPC_TO_ALL );

	SetScreenName( name );

	return ( RPC_SUCCESS );
}

gpwstring GoCommon :: GetTemplateScreenName( void )
{
	static AutoConstQuery <gpstring> s_Query( "screen_name" );
	gpwstring sScreenName = ::ToUnicode( s_Query.Get( GetData()->GetRecord() ) );
	
	if ( LocMgr::DoesSingletonExist() && !GetGo()->IsCloneSourceGo() )
	{		
		ExtractFirstTagFromString( sScreenName, m_NameGender );		
	}

	return ( sScreenName );
}

const gpstring& GoCommon :: GetForcedExpirationClass( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "forced_expiration_class" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoCommon :: GetAutoExpirationClass( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "auto_expiration_class" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoCommon :: CanExpireTriggers( void )
{
	if (m_InstanceTriggers)
	{
		if (!m_InstanceTriggers->GetCanExpire())
		{
			return false;
		}
	}
	if (m_TemplateTriggers)
	{
		if (!m_TemplateTriggers->GetCanExpire())
		{
			return false;
		}
	}
	return true;
}

trigger::Storage& GoCommon :: GetValidInstanceTriggers( void )
{
	if ( m_InstanceTriggers == NULL )
	{
		m_InstanceTriggers = new trigger::Storage( INITIAL_TRIG_NUM_INSTANCE, GetGo() );
	}
    return ( *m_InstanceTriggers );
}

void GoCommon :: RefreshTriggers( void )
{
	if (m_InstanceTriggers)
	{
		m_InstanceTriggers->RefreshOwner();
	}
	if (m_TemplateTriggers)
	{
		m_TemplateTriggers->RefreshOwner();
	}
}

void GoCommon :: SCopyMembership( Goid go )
{
	CHECK_SERVER_ONLY;

	RCCopyMembership( go );
}

void GoCommon :: RCCopyMembership( Goid go )
{
	FUBI_RPC_THIS_CALL( RCCopyMembership, RPC_TO_ALL );
	CopyMembership( go );
}

void GoCommon :: CopyMembership( const Membership& membership )
{
	m_Membership_last = m_Membership;
	m_Membership = membership;
}

void GoCommon :: CopyMembership( Goid go )
{
	GoHandle hGo( go );

	if( hGo )
	{
		m_Membership_last = m_Membership;
		m_Membership = hGo->GetCommon()->GetMembership();
	}
}

void GoCommon :: SRestoreLastMembership( void )
{
	CHECK_SERVER_ONLY;

	RCRestoreLastMembership();
}

void GoCommon :: RCRestoreLastMembership( void )
{
	FUBI_RPC_THIS_CALL( RCRestoreLastMembership, RPC_TO_ALL );
	RestoreLastMembership();
}

GoComponent* GoCommon :: Clone( Go* newGo )
{
	return ( new GoCommon( *this, newGo ) );
}

bool GoCommon :: Xfer( FuBi::PersistContext& persist )
{
	bool success = true;

	persist.XferQuoted( "screen_name", m_ScreenName );
	persist.XferQuoted( "description", m_Description );
	persist.XferQuoted( "custom_name", m_CustomName );

	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_NameGender",       m_NameGender       );
		persist.Xfer( "m_OkToSelfDestruct", m_OkToSelfDestruct );

		if ( persist.IsSaving() )
		{
			if ( m_TemplateTriggers != NULL )
			{
				persist.EnterBlock( "m_TemplateTriggers" );
				m_TemplateTriggers->Xfer( persist );
				persist.LeaveBlock();
			}

			if ( m_InstanceTriggers != NULL )
			{
				persist.EnterBlock( "m_InstanceTriggers" );
				m_InstanceTriggers->Xfer( persist );
				persist.LeaveBlock();
			}
		}
		else
		{
			gpassert( m_TemplateTriggers == NULL );
			gpassert( m_InstanceTriggers == NULL );

			if ( persist.EnterBlock( "m_TemplateTriggers" ) )
			{
				m_TemplateTriggers = new trigger::Storage( INITIAL_TRIG_NUM_TEMPLATE, GetGo() );
				m_TemplateTriggers->Xfer( persist );
			}
			persist.LeaveBlock();

			if ( persist.EnterBlock( "m_InstanceTriggers" ) )
			{
				m_InstanceTriggers = new trigger::Storage( INITIAL_TRIG_NUM_INSTANCE, GetGo() );
				m_InstanceTriggers->Xfer( persist );
			}
			persist.LeaveBlock();
		}

		m_Membership.Xfer( persist, "m_Membership" );
		m_Membership_last.Xfer( persist, "m_Membership_last" );
	}
	else
	{
		// only build storage if this is initial creation
		if ( persist.IsRestoring() && !GetGo()->HasValidGoid() )
		{
			gpassert( m_TemplateTriggers == NULL );
			gpassert( m_InstanceTriggers == NULL );

			FastFuelHandle fuel = GetData()->GetInternalField( "template_triggers" );
			if ( fuel )
			{
				trigger::Storage* triggers = gContentDb.AddOrFindTriggerStorage( fuel, true ).Get();
				if ( triggers != NULL )
				{
					m_TemplateTriggers = new trigger::Storage( *triggers, INITIAL_TRIG_NUM_TEMPLATE, GetGo() );
				}
			}

			fuel = GetData()->GetInternalField( "instance_triggers" );
			if ( fuel )
			{
				std::auto_ptr <trigger::Storage> newTriggers( new trigger::Storage( INITIAL_TRIG_NUM_INSTANCE, GetGo() ) );
				if ( newTriggers->Load( fuel, false ) )
				{
					m_InstanceTriggers = newTriggers.release();
				}

			}

			gpstring sMembership;
			persist.Xfer( "membership", sMembership );
			if( !sMembership.empty() )
			{
				m_Membership.Add( sMembership );
			}
		}
	}

#	if !GP_RETAIL
	if ( gContentDb.FindExpirationClass( GetForcedExpirationClass() ) == NULL )
	{
		gperrorf(( "Invalid forced expiration class '%s' on template '%s'\n", GetForcedExpirationClass().c_str(), GetGo()->GetTemplateName() ));
	}
	if ( gContentDb.FindExpirationClass( GetAutoExpirationClass() ) == NULL )
	{
		gperrorf(( "Invalid auto expiration class '%s' on template '%s'\n", GetAutoExpirationClass().c_str(), GetGo()->GetTemplateName() ));
	}
#	endif // !GP_RETAIL

	return ( success );
}

bool GoCommon :: XferCharacter( FuBi::PersistContext& persist )
{
	persist.Xfer( "screen_name", m_ScreenName );

	return ( true );
}

void GoCommon :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	if ( msg.GetSendFrom() != GOID_INVALID && 
		 msg.GetSendFrom() == GetGoid() && 
		 msg.GetEvent() != WE_REQ_USE &&
		 msg.GetEvent() != WE_REQ_ACTIVATE &&
		 msg.GetEvent() != WE_REQ_DEACTIVATE &&
		 msg.GetEvent() != WE_DESTRUCTED &&
		 !IsMCPMessage( msg.GetEvent() ) )
	{
		return;
	}

	if ( msg.HasEvent( WE_POST_RESTORE_GAME) )
	{
		if ( m_TemplateTriggers )
		{
			m_TemplateTriggers->XferPost();
		}
		if ( m_InstanceTriggers )
		{
			m_InstanceTriggers->XferPost();
		}
	}
	else if ( GetGo()->IsInAnyWorldFrustum() || GetGo()->IsOmni()
				|| msg.HasEvent( WE_ENTERED_WORLD )
				|| msg.HasEvent( WE_LEFT_WORLD )
				|| msg.HasEvent( WE_CONSTRUCTED )
				|| msg.HasEvent( WE_EQUIPPED )
				|| msg.HasEvent( WE_DESTRUCTED ) )
	{
		// only pass along messages to triggers if we're in the world!
		if ( m_TemplateTriggers != NULL )
		{
			m_TemplateTriggers->HandleMessage( msg );
		}
		if ( m_InstanceTriggers != NULL )
		{
			m_InstanceTriggers->HandleMessage( msg );
		}
	}
}

void GoCommon :: Update( float deltaTime )
{
	UpdateOrDetect( &deltaTime );
}

void GoCommon :: DetectWantsBits( void )
{
	UpdateOrDetect();
}

bool GoCommon :: CanServerExistOnly( void )
{
	if ( !Inherited::CanServerExistOnly() )
	{
		return ( false );
	}

	// check triggers
	if ( m_TemplateTriggers != NULL )
	{
		if ( m_TemplateTriggers->GetMustCreateOnClients() )
		{
			return ( false );
		}
	}
	if ( m_InstanceTriggers != NULL )
	{
		if ( m_InstanceTriggers->GetMustCreateOnClients() )
		{
			return ( false );
		}
	}

	// guess it's ok
	return ( true );
}

bool GoCommon :: CommitCreation( void )
{
	// If this is a localized version, then there might be "gender tags" applied to
	// the translated screen name.  Let's extract these ( so it doesn't show up in the name )
	// and use it for lookups when constructing pcontent.
	if ( LocMgr::DoesSingletonExist() && !GetGo()->IsCloneSourceGo() )
	{
		gpwstring sScreenName = m_ScreenName.GetString();
		ExtractFirstTagFromString( sScreenName, m_NameGender );
		SetScreenName( sScreenName );
	}

	if ( !GetGo()->IsCloneSourceGo() )
	{
		// Commit triggers & assign Tuids --biddle
		if ( m_TemplateTriggers != NULL )
		{
			m_TemplateTriggers->CommitCreation();
		}
		if ( m_InstanceTriggers != NULL )
		{
			m_InstanceTriggers->CommitCreation();
		}
	}

	// make sure that we have triggers, no self-destruct without those
	if ( (m_TemplateTriggers != NULL) || (m_InstanceTriggers != NULL) )
	{
		// need one that can self-destructo
		if (   (m_TemplateTriggers && m_TemplateTriggers->CanSelfDestruct())
			|| (m_InstanceTriggers && m_InstanceTriggers->CanSelfDestruct()) )
		{
			// if we're global, we gotta be the server
			if ( ::IsServerLocal() || !GetGo()->IsGlobalGo() )
			{
				// anything visible and parties are out of the question
				if ( !GetGo()->HasAspect() && !GetGo()->HasParty() )
				{
					// as are skrit components
					Go::ComponentColl::const_iterator i, ibegin = GetGo()->GetComponentBegin(), iend = GetGo()->GetComponentEnd();
					for ( i = ibegin ; i != iend ; ++i )
					{
						if ( (*i)->GetData()->IsSkrit() )
						{
							// damn!
							break;
						}
					}

					// good?
					if ( i == iend )
					{
						// yeah!
						m_OkToSelfDestruct = true;
					}
				}
			}
		}
	}

	return true;
}

void GoCommon :: XferForSync( FuBi::BitPacker& packer )
{
	// We need to refresh the occupants of any spatial triggers when
	// a go re-enters a client's frustum 

	// Need to take care to detect when the the host and client
	// have mis-matched sets of triggers, something that can happen when
	// one-shots are involved.
	
	DWORD num_trigs = (m_TemplateTriggers == NULL) ? 0 : m_TemplateTriggers->NumClientTriggers();

	if (num_trigs != 0)
	{
		if (packer.IsSaving())
		{
			packer.XferCount( num_trigs );
		}

		m_TemplateTriggers->XferForSync( packer );
	}
	else
	{
		if (packer.IsSaving())
		{
			packer.XferCount( num_trigs ); // will be 0
		}
		else
		{
			packer.XferCount( num_trigs );
			while (num_trigs)
			{
				// We have to skip over all the data
				trigger::Storage::SkipTriggerXfer( packer );
				--num_trigs;
			}
		}
	}

	num_trigs = (m_InstanceTriggers == NULL) ? 0 : m_InstanceTriggers->NumClientTriggers();
	
	if (num_trigs != 0)
	{
		if (packer.IsSaving())
		{
			packer.XferCount( num_trigs );
		}

		m_InstanceTriggers->XferForSync( packer );
	}
	else
	{
		if (packer.IsSaving())
		{
			packer.XferCount( num_trigs ); // will be 0
		}
		else
		{
			packer.XferCount( num_trigs );
			while (num_trigs)
			{
				// We have to skip over all the data
				trigger::Storage::SkipTriggerXfer( packer );
				--num_trigs;
			}
		}
	}
	
}

#if !GP_RETAIL

bool GoCommon :: ShouldSaveInternals( void )
{
	return ( (m_InstanceTriggers != NULL) && (m_InstanceTriggers->Size() > 0) );
}

void GoCommon :: SaveInternals( FuelHandle fuel )
{
	m_InstanceTriggers->Save( fuel->CreateChildBlock( "instance_triggers" ) );
}

void GoCommon :: DrawDebugHud( void )
{
	DrawDebugTriggers();
}

void GoCommon :: DrawDebugTriggers( void )
{
	if ( m_TemplateTriggers != NULL )
	{
		m_TemplateTriggers->Draw();
	}

	if ( m_InstanceTriggers != NULL )
	{
		m_InstanceTriggers->Draw();
	}
}


void GoCommon :: PadDebugLabelHelper(const trigger::Trigger* in_trig, const char* in_padding, gpstring& out_lbl1, gpstring& out_lbl2) const
{
	// Attempt to stack labels of triggers so they don't overlap. WHAT A HASSLE!

	gpstring* lbl = &out_lbl1;

	if (m_InstanceTriggers)
	{
		trigger::Storage::TriggerCollConstIter iTrigger = m_InstanceTriggers->Begin();
		for ( ;iTrigger != m_InstanceTriggers->End(); ++iTrigger )
		{
			if ((*iTrigger) == in_trig)
			{
				lbl = &out_lbl2;
			}
			for (DWORD k = 0; k < (*iTrigger)->GetConditions().size(); k++)
			{
				lbl->append(in_padding);
			}
		}
	}
	if (m_TemplateTriggers)
	{
		trigger::Storage::TriggerCollConstIter iTrigger = m_TemplateTriggers->Begin();
		for ( ;iTrigger != m_TemplateTriggers->End(); ++iTrigger )
		{
			if ((*iTrigger) == in_trig)
			{
				lbl = &out_lbl2;
			}
			for (DWORD k = 0; k < (*iTrigger)->GetConditions().size(); k++)
			{
				lbl->append(in_padding);
			}
		}
	}
}


#endif // !GP_RETAIL

void GoCommon :: UpdateOrDetect( float* deltaTime )
{
	GPSTATS_GROUP( GROUP_TRIGGERS );

	bool wantsUpdates = false;

	double wt = gWorldTime.GetTime();

	if ( m_TemplateTriggers != NULL )
	{
		if ( deltaTime != NULL )
		{
			m_TemplateTriggers->Update( wt );
		}

		if ( m_TemplateTriggers->CommitDeleteRequests() )
		{
			wantsUpdates = true;
		}
		else
		{
			Delete( m_TemplateTriggers );
		}
	}

	if ( m_InstanceTriggers != NULL )
	{
		if ( deltaTime != NULL )
		{
			m_InstanceTriggers->Update( wt );
		}

		if ( m_InstanceTriggers->CommitDeleteRequests() )
		{
			wantsUpdates = true;
		}
		else
		{
			Delete( m_InstanceTriggers );
		}
	}

#	if !GP_RETAIL
	// siege editor always wants updates because it can change the triggers
	// but it won't tell the go to start updating
	if ( gGoDb.IsEditMode() )
	{
		wantsUpdates = true;
	}
#	endif // !GP_RETAIL

	SetWantsUpdates( wantsUpdates );

	// if we don't want updates any more, then our triggers are gone. if that's
	// the case, and you can't see us anyways, then delete myself entirely!
	if ( !wantsUpdates && m_OkToSelfDestruct )
	{
		// Set so that we mark for deletion next frame -- biddle
		PostWorldMessage(WE_REQ_DELETE, GetGoid(), GetGoid(), 60.0f);
	}
}

DWORD GoCommon :: GoCommonToNet( GoCommon* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoCommon* GoCommon :: NetToGoCommon( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetCommon() : NULL );
}


//////////////////////////////////////////////////////////////////////////////
// class GoPlacement implementation

#if !GP_RETAIL
bool GoPlacement::ms_IsRendering = false;
#endif // !GP_RETAIL

GoPlacement :: GoPlacement( Go* go )
	: Inherited( go )
{
	m_RenderIndexCache[ 0 ] = 0;
	m_RenderIndexCache[ 1 ] = 0;
	m_RenderIndexCache[ 2 ] = 0;
	m_LiquidHeight          = 0;
	m_LiquidFlags           = siege::LF_NONE;
	m_PlacementDirty        = true;
}

GoPlacement :: GoPlacement( const GoPlacement& source, Go* newGo )
	: Inherited    ( source, newGo )
	, m_Position   ( source.m_Position    )
	, m_Orientation( source.m_Orientation )
{
	m_RenderIndexCache[ 0 ] = source.m_RenderIndexCache[ 0 ];
	m_RenderIndexCache[ 1 ] = source.m_RenderIndexCache[ 1 ];
	m_RenderIndexCache[ 2 ] = source.m_RenderIndexCache[ 2 ];
	m_LiquidHeight          = source.m_LiquidHeight;
	m_LiquidFlags           = source.m_LiquidFlags;
	m_PlacementDirty        = source.m_PlacementDirty;
}

GoPlacement :: ~GoPlacement( void )
{
	// this space intentionally left blank...
}

bool GoPlacement :: IsGuiGo( void ) const
{
	static AutoConstQuery <bool> s_Query( "is_gui_go" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

void GoPlacement :: RCSetPosition( const SiegePos& pos )
{
	FUBI_RPC_THIS_CALL( RCSetPosition, RPC_TO_ALL );

	PrivateSetPlacement( &pos, NULL );
}

void GoPlacement :: SSetPosition( const SiegePos& pos, bool snapToTerrain )
{
	CHECK_SERVER_ONLY;

	SiegePos localPos( pos );
	PrivateCalcPosition( localPos, snapToTerrain );
	RCSetPosition( localPos );
}

void GoPlacement :: SetPosition( const SiegePos& pos, bool snapToTerrain )
{
	SiegePos localPos( pos );
	PrivateCalcPosition( localPos, snapToTerrain );
	PrivateSetPlacement( &localPos, NULL );
}

const SiegePos& GoPlacement :: GetOriginalPosition( void ) const
{
	static AutoConstQuery <SiegePos> s_Query( "position" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPlacement :: HasMovedFromOriginalPosition( void ) const
{
	return ( GetPosition() != GetOriginalPosition() );
}

bool GoPlacement :: DoesInheritPlacement( void ) const
{
	// don't mess with SE
#	if !GP_RETAIL
	if ( ::IsWorldEditMode() )
	{
		return ( false );
	}
#	endif // !GP_RETAIL

	if ( GetGo()->IsOmni() )
	{
		return ( false );
	}

	// inventory inherits
	return ( GetGo()->IsInsideInventory() );
}

void GoPlacement :: ForceSetPosition( const SiegePos& pos )
{
	PrivateSetPlacement( &pos, NULL );
}

const SiegePos& GoPlacement :: GetPosition( void ) const
{	
	const SiegePos& pos = DoesInheritPlacement() ? GetGo()->GetParent()->GetPlacement()->GetPosition() : m_Position;

#	if !GP_RETAIL
	if ( (pos.node != m_Position.node) && !GetGo()->IsPlacementChanging() )
	{
		gperrorf(( "TELL SCOTT: a Go is in the wrong node!!!\n"
				   "\n"
				   "This Go: 0x%08X ('%s')\n"
				   "This node: %s\n"
				   "Inherited: %s\n"
				   "Parent Go: 0x%08X ('%s')\n"
				   "Parent node: %s\n",
				   GetGoid(), GetGo()->GetTemplateName(),
				   m_Position.node.ToString().c_str(),
				   DoesInheritPlacement() ? "yes" : "no",
				   GetGo()->HasParent() ? GetGo()->GetParent()->GetGoid() : 0,
				   GetGo()->HasParent() ? GetGo()->GetParent()->GetTemplateName() : "<none>",
				   GetGo()->HasParent() ? GetGo()->GetParent()->GetPlacement()->GetPosition().node.ToString().c_str() : "<none>" ));
	}
#	endif // !GP_RETAIL

	return ( pos );
}

const Quat& GoPlacement :: GetOrientation( void ) const
{	
	return ( DoesInheritPlacement() ? GetGo()->GetParent()->GetPlacement()->GetOrientation() : m_Orientation );
}

#if !GP_RETAIL

bool GoPlacement :: VerifyPosition( void ) const
{
	bool valid = true;

	// must be siege based to have terrain context
	if ( !GetGo()->IsOmni() )
	{
		// make sure we're in the node we think we are
		if_not_gpassert( !GetGo()->IsInAnyWorldFrustum() || gGoDb.IsNodeOccupant( GetGo(), m_Position.node ) )
		{
			valid = false;
		}

		if ( IsPrimaryThread() && !gSiegeEngine.IsNodeInAnyFrustum( m_Position.node ) )
		{
			if_not_gpassertm(
					!GetGo()->IsInAnyWorldFrustum() || GetGo()->IsPlacementChanging() /*#10335*/ || GetGo()->HasFrustum() /*#12317*/,
					"I'm in the world frustum but my node is not! Wtf??" )
			{
				valid = false;
			}
		}
	}

	return ( valid );
}

#endif // !GP_RETAIL

void GoPlacement :: RCSetOrientation( const Quat& quat )
{
	FUBI_RPC_THIS_CALL( RCSetOrientation, RPC_TO_ALL );

	PrivateSetPlacement( NULL, &quat );
}

void GoPlacement :: SSetOrientation( const Quat& quat )
{
	CHECK_SERVER_ONLY;

	RCSetOrientation( quat );
}

void GoPlacement :: SetOrientation( const Quat& quat )
{
	PrivateSetPlacement( NULL, &quat );
}

void GoPlacement :: RCSetPlacement( DWORD machineId, const SiegePos& pos, const Quat& quat )
{
	FUBI_SET_RPC_PEEKADDRESS( machineId );
	RCSetPlacementPeek( pos, quat );
}

void GoPlacement :: RCSetPlacementPeek( SiegePos pos, Quat quat )
{
	FUBI_RPC_THIS_CALL_PEEKADDRESS( RCSetPlacementPeek );
	PrivateSetPlacement( &pos, &quat );
}

void GoPlacement :: SSetPlacement( const SiegePos& pos, const Quat& quat, bool snapToTerrain )
{
	CHECK_SERVER_ONLY;

	SiegePos localPos( pos );
	PrivateCalcPosition( localPos, snapToTerrain );
	RCSetPlacement( RPC_TO_ALL, localPos, quat );
}

void GoPlacement :: SetPlacement( const SiegePos& pos, const Quat& quat, bool snapToTerrain )
{
	SiegePos localPos( pos );
	PrivateCalcPosition( localPos, snapToTerrain );
	PrivateSetPlacement( &localPos, &quat );
}

void GoPlacement :: ForceSetPlacement( const SiegePos& pos, const Quat& quat )
{
	PrivateSetPlacement( &pos, &quat );
}

void GoPlacement :: ForceSetPlacement( const SiegePos* pos, const Quat* quat )
{
	PrivateSetPlacement( pos, quat );
}

bool GoPlacement :: GetIsInVisibleNode( void ) const
{
	siege::SiegeNode* node = gSiegeEngine.IsNodeValid( m_Position.node );
	return ( (node != NULL) && node->GetVisibleThisFrame() );
}

bool GoPlacement :: GetIsNodeInAnyWorldFrustum( void ) const
{
	siege::SiegeNode* node = gSiegeEngine.IsNodeValid( m_Position.node );
	return ( (node != NULL) && (node->GetFrustumOwnedBitfield() != 0) );
}

const gpstring& GoPlacement :: GetUsePoints( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "use_point_scids" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPlacement :: HasUsePoints( void ) const
{
	bool ret = false;

	const gpstring& s =GetUsePoints();
	if ( !s.empty() )
	{
		// Make sure that the object hasn't moved
		ret = !HasMovedFromOriginalPosition();
	}

	return ret;
}

int GoPlacement :: GetUsePoints( UsePointColl& pts ) const
{
	pts.clear();

	const gpstring& s = GetUsePoints();

	DWORD count = stringtool::GetNumDelimitedStrings(s,',');

	if (count > 0)
	{
		// Make sure that the object hasn't moved
		if ( !HasMovedFromOriginalPosition() )
		{
			for (DWORD i = 0; i < count; i++)
			{
				DWORD usep_scid;
				if ( stringtool::GetDelimitedValue(s,',',i,usep_scid) )
				{
					Goid usep_goid = gGoDb.FindGoidByScid((Scid)usep_scid);
					GoHandle usep_go(usep_goid);
					if (usep_go && usep_go->HasPlacement() && usep_go->IsInAnyWorldFrustum() )
					{
						const SiegePos &usep_pos = usep_go->GetPlacement()->GetPosition();
						pts.push_back(usep_pos);
					}
				}
			}
		}
	}

	return pts.size();
}

const gpstring& GoPlacement :: GetDropPoints( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "drop_point_scids" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoPlacement :: HasDropPoints( void ) const
{
	bool ret;

	const gpstring& dpts = GetDropPoints();
	ret = !dpts.empty() && !HasMovedFromOriginalPosition();

	return ret;
}

int GoPlacement :: GetDropPoints( UsePointColl& pts ) const
{
	pts.clear();

	const gpstring& s = GetDropPoints();

	DWORD count = stringtool::GetNumDelimitedStrings(s,',');

	if (count > 0)
	{
		// Make sure that the object hasn't moved
		if ( !HasMovedFromOriginalPosition() )
		{
			for (DWORD i = 0; i < count; i++)
			{
				DWORD usep_scid;
				if ( stringtool::GetDelimitedValue(s,',',i,usep_scid) )
				{
					Goid usep_goid = gGoDb.FindGoidByScid((Scid)usep_scid);
					GoHandle usep_go(usep_goid);
					if (usep_go && usep_go->HasPlacement() && usep_go->IsInAnyWorldFrustum() )
					{
						const SiegePos &usep_pos = usep_go->GetPlacement()->GetPosition();
						pts.push_back(usep_pos);
					}
				}
			}
		}
	}
	else
	{
		// If there are no drop points defined, use the list of USE_POINTS instead
		GetUsePoints(pts);
	}


	return pts.size();
}

void GoPlacement :: UpdateLightingCache( void )
{
	bool success = false;

	// only bother with this work if we're not an inventory item, are invisible,
	// or are ammo. the inventory can't be seen, nor can invisible items, and
	// ammo is not worth lighting via siege. ammo moving through the world is
	// very expensive.
	if (   !GetGo()->IsInsideInventory()
		&& !GetGo()->IsAmmo()
		&& GetGo()->HasAspect()
		&& GetGo()->GetAspect()->GetIsVisible() )
	{
		// get render indices
		SiegePos position = GetPosition();

		// see if this node has a floor - can't use lighting info from another node
		if( GetGo()->HasBody() )
		{
			success = gSiegeEngine.AdjustPointToTerrainWithIndex( position, m_RenderIndexCache[ 0 ], m_RenderIndexCache[ 1 ], m_RenderIndexCache[ 2 ], siege::LF_IS_FLOOR );
		}
		else if ( GetGo()->HasAspect() || GetGo()->HasFader() )
		{
			success = gSiegeEngine.AdjustPointToTerrainWithIndex( position, m_RenderIndexCache[ 0 ], m_RenderIndexCache[ 1 ], m_RenderIndexCache[ 2 ], siege::LF_IS_ANY );
		}
		else
		{
			success = gSiegeEngine.AdjustPointToTerrain( position, siege::LF_IS_ANY );
		}

		// see if it's the same
		if ( success )
		{
			success = position.node == m_Position.node;
		}
	}

	// clear cache if invalid
	if ( !success )
	{
		m_RenderIndexCache[ 0 ] = 0;
		m_RenderIndexCache[ 1 ] = 0;
		m_RenderIndexCache[ 2 ] = 0;
	}
}

void GoPlacement :: UpdateLiquidInfo( void )
{
	if (   GetGo()->IsInsideInventory()
		|| !gSiegeEngine.GetLiquidInfo( GetPosition(), m_LiquidHeight, m_LiquidFlags ) )
	{
		// clear info if invalid
		m_LiquidHeight = 0.0f;
		m_LiquidFlags  = siege::LF_NONE;
	}
}

GoComponent* GoPlacement :: Clone( Go* newGo )
{
	return ( new GoPlacement( *this, newGo ) );
}

bool GoPlacement :: Xfer( FuBi::PersistContext& persist )
{
	// special: if we're xfering an existing placement, setpos normally to keep
	// membership lists in sync.
	if ( persist.IsRestoring() && GetGo()->HasValidGoid() && !persist.IsFullXfer() )
	{
		SiegePos position;
		Quat orientation;
		persist.Xfer( "position",    position );
		persist.Xfer( "orientation", orientation );

		PrivateSetPlacement( &position, &orientation, true );
	}
	else
	{
		bool rc1 = persist.Xfer( "position",    m_Position    );
		bool rc2 = persist.Xfer( "orientation", m_Orientation );

		gpassert( !persist.IsFullXfer() || (rc1 && rc2) );

		if ( persist.IsFullXfer() )
		{
			// always set placement dirty, then no need to persist it (skrooit!)
			m_PlacementDirty = true;
		}
	}

	// verify that our bits match up!!!
#	if !GP_RETAIL
	if ( persist.IsFullXfer() && !GetGo()->IsOmni() )
	{
		if ( (DWORD)GetGo()->GetWorldFrustumMembership() != 0 && !gSiegeEngine.IsNodeValid( m_Position.node ) )
		{
			gperrorf(( "Frustum membership mismatch!! A Go (0x%08X) was saved "
					   "out with membership bits 0x%08X, but Siege is telling "
					   "me that its node %s isn't even loaded!\n",
					   GetGoid(), GetGo()->GetWorldFrustumMembership(), m_Position.node.ToString().c_str() ));
		}
	}
#	endif // !GP_RETAIL

	return ( true );
}

bool GoPlacement :: CommitCreation( void )
{
	// if we've got a scid, start out with clean placement
	if ( !GetGo()->IsSpawned() )
	{
		m_PlacementDirty = false;
	}

	return ( true );
}

bool GoPlacement :: CommitLoad( void )
{
	if ( !CommitCreation() )
	{
		return ( false );
	}

	if ( gSiegeEngine.IsNodeInAnyFrustum( m_Position.node ) )
	{
		UpdateLightingCache();
		UpdateLiquidInfo();
	}

	return ( true );
}

void GoPlacement :: HandleMessage( const WorldMessage& msg )
{
	gpassert( !msg.IsCC() );

	if ( msg.GetEvent() == WE_CONSTRUCTED )
	{
		// force an update of our position to pick up kids
		ResetPosition();
	}
	else if ( msg.GetEvent() == WE_ENTERED_WORLD )
	{
		if ( gSiegeEngine.IsNodeInAnyFrustum( m_Position.node ) )
		{
			UpdateLightingCache();
			UpdateLiquidInfo();
		}
	}
}

#if GP_DEBUG

bool GoPlacement :: AssertValid( void ) const
{
	// must be siege based to have terrain context
	if ( !GetGo()->IsOmni() )
	{
		// make sure we're in the node we think we are
		gpassert( !GetGo()->IsInAnyWorldFrustum() || gGoDb.IsNodeOccupant( GetGo(), m_Position.node ) );

		// can only do this stuff from the main thread or siege will no like
		if ( GetGo()->IsInAnyWorldFrustum() && ::IsPrimaryThread() )
		{
			// get our node
			SiegeNode* node = gSiegeEngine.IsNodeValid( m_Position.node );
			if ( node != NULL )
			{
				// validate indexes
				if ( (m_RenderIndexCache[ 0 ] != 0) || (m_RenderIndexCache[ 1 ] != 0) || (m_RenderIndexCache[ 2 ] != 0) )
				{
					// should all be different
					gpassert( m_RenderIndexCache[ 0 ] != m_RenderIndexCache[ 1 ] );
					gpassert( m_RenderIndexCache[ 0 ] != m_RenderIndexCache[ 2 ] );
					gpassert( m_RenderIndexCache[ 1 ] != m_RenderIndexCache[ 2 ] );

					// should all be valid indices
					gpassert( (m_RenderIndexCache[ 0 ] >= 0) && (m_RenderIndexCache[ 0 ] < (short)node->GetNumLitVertices()) );
					gpassert( (m_RenderIndexCache[ 1 ] >= 0) && (m_RenderIndexCache[ 1 ] < (short)node->GetNumLitVertices()) );
					gpassert( (m_RenderIndexCache[ 2 ] >= 0) && (m_RenderIndexCache[ 2 ] < (short)node->GetNumLitVertices()) );
				}
			}
			else
			{
				gpassertm( GetGo()->IsPlacementChanging() /*#10335*/ || GetGo()->HasFrustum() /*#12317*/,
						   "I'm in the world frustum but my node is not! Wtf??" );
			}
		}
	}

	// all-zeros quat is never allowed by order of mike!
	gpassert( m_Orientation != Quat::ZERO );
	gpassert( m_Orientation != Quat::INVALID );

	// simply calling GetPosition() will do a node check - only do it on non-omni owned items
	// because it is possible for a omni item to be transferred to a character and lose its
	// omni status BEFORE it is given a valid position, hence leading to MAJOR problems. 
	if ( !(GetGo()->HasParent() && GetGo()->GetParent()->IsOmni()) )
	{
		GetPosition();
	}

	return ( true );
}

#endif // GP_DEBUG

void GoPlacement :: PrivateCalcPosition( SiegePos& pos, bool snapToTerrain )
{
#	if GP_DEBUG
	if ( GetGoidClass( GetGoid() ) != GO_CLASS_LOCAL )
	{
		CHECK_SERVER_ONLY;
	}
#	endif // GP_DEBUG

	// must be siege based to have terrain context
	if ( !GetGo()->IsOmni() )
	{
		// fix up the siege pos
		if ( snapToTerrain )
		{
			// snap the position to the terrain - note that this automatically
			// adjusts the node if necessary (same as UpdateNodePosition).
			gSiegeEngine.AdjustPointToTerrain( pos );
		}
		else
		{
			// see if offset from node results in placement in a new node
			SiegePos correct( pos );
			gSiegeEngine.AdjustPointToTerrain( correct );

			pos.pos = pos.WorldPos();
			pos.node = correct.node;
			pos.FromWorldPos( pos.pos, pos.node );
		}
	}
}

void GoPlacement :: PrivateSetPlacement( const SiegePos* pos, const Quat* quat, bool alwaysUpdate )
{
#	if !GP_RETAIL

	// can't change positions during rendering, very bad!! note the world
	// frustum check to make sure it's been added to the world already. better
	// than checking current thread for primary because a secondary thread might
	// end up moving a go already in the world and queued for rendering (and
	// this is bad).
	if ( ms_IsRendering && GetGo()->IsInAnyWorldFrustum() )
	{
		gperrorf(( "Serious error! Go 0x%08X scid 0x%08X template '%s' is changing its position while RENDERING!\n",
				   GetGoid(), GetGo()->GetScid(), GetGo()->GetTemplateName() ));
	}

	// if we're setting the position, don't be setting it to NOTHING
	if ( (pos != NULL) && (pos->node == siege::UNDEFINED_GUID) && !GetGo()->IsOmni() )
	{
		if ( !::IsLoading( gWorldState.GetCurrentState() ) )
		{
			gperrorf(( "Serious error! Go 0x%08X scid 0x%08x template '%s' is getting its node set to UNDEFINED_GUID!\n",
					   GetGoid(), GetGo()->GetScid(), GetGo()->GetTemplateName() ));
		}
	}

	// special uber paranoia
	if ( (pos != NULL) && !pos->IsExactlyEqual( m_Position ) )
	{
		const SiegePos& thisPos = GetPosition();
		gpgouberf(( "MOVE: g:0x%08X s:0x%08X '%s' (%d) %g,%g,%g,%s [%g,%g,%g,%s] -> %g,%g,%g,%s\n",
					GetGoid(), GetScid(), GetGo()->GetTemplateName(), alwaysUpdate,
					thisPos.pos.x, thisPos.pos.y, thisPos.pos.z, thisPos.node.ToString().c_str(),
					m_Position.pos.x, m_Position.pos.y, m_Position.pos.z, m_Position.node.ToString().c_str(),
					pos->pos.x, pos->pos.y, pos->pos.z, pos->node.ToString().c_str() ));
	}

#	endif // !GP_RETAIL

	// minimum tolerance: 1 mm
	const float MIN_PLACEMENT_UPDATE_TOLERANCE = 0.001f;

	// because camera depends on the position of the party, we want that party
	// to always update to prevent minor pop-ola
	if ( GetGo()->HasParty() )
	{
		alwaysUpdate = true;
	}

	// calc dirtyness

	// the editor needs percise rotation increments, so let's make sure that very small changes in rotation are recognized.
	float floatTolerance = gGoDb.IsEditMode() ? FLOAT_TOLERANCE : 0.01f;

	bool posDirty  = (pos  != NULL) && !pos ->IsEqual( m_Position, MIN_PLACEMENT_UPDATE_TOLERANCE );
	bool quatDirty = (quat != NULL) && !quat->IsEqual( m_Orientation, floatTolerance );
	if ( posDirty || quatDirty )
	{
#		if !GP_RETAIL

		if( GetGo()->HasAspect() && GetGo()->GetAspect()->GetDoesBlockPath() &&
			!IsWorldEditMode() && !GetGo()->IsPendingEntry() )
		{
			gperror( "Cannot move path blocking object!" );
		}

#	endif // !GP_RETAIL

		m_PlacementDirty = true;
	}

	// update position if different
	bool updatedChildren = false;
	if ( (pos != NULL) && (alwaysUpdate || posDirty) )
	{
		// note that children only need to move on a node update
		bool updateChildren = alwaysUpdate || (pos->node != m_Position.node);

		// move
		siege::database_guid oldNode = m_Position.node;
		m_Position = *pos;
		GetGo()->SetSpaceDirty();

		GPDEV_ONLY( GetGo()->SetPlacementChanging() );

		// must be siege based to have terrain context
		if ( !GetGo()->IsOmni() )
		{
			// update frustum position
			if ( GetGo()->HasFrustum() && !gWorldOptions.GetFixedWorldFrustum() )
			{
				// get the frustum
				FrustumId frustumId = gGoDb.GetGoFrustum( GetGoid() );
				siege::SiegeFrustum* frustum = gSiegeEngine.GetFrustum( MakeInt( frustumId ) );
				if_gpassert( frustum != NULL )
				{
					// move it to where i am!
					frustum->SetPosition( GetPosition() );
				}
			}

			// only mess with the node when we are on the primary thread
			if ( !GetGo()->IsPendingEntry() )
			{
				if ( gSiegeEngine.IsNodeInAnyFrustum( m_Position.node ) )
				{
					// fix up lighting
					UpdateLightingCache();

					// update liquid info
					UpdateLiquidInfo();
				}

				// transfer occupancy if we're already in the world
				gGoDb.TransferNodeOccupant( GetGo(), oldNode, m_Position.node );
			}
		}

		GPDEV_ONLY( GetGo()->ClearPlacementChanging() );

		// move children if needed
		GetGo()->UpdateChildrenPositions( updateChildren );
		updatedChildren = true;

		// validate new position
		gpassert( VerifyPosition() );
	}

	// update orientation if different
	if ( (quat != NULL) && (alwaysUpdate || quatDirty) )
	{
		// take it
		m_Orientation = *quat;
		GetGo()->SetWobbDirty();

		// rotate children if we haven't already
		if ( !updatedChildren )
		{
			GetGo()->UpdateChildrenPositions( false );
		}
	}

#	if !GP_RETAIL
	if ( gGoDb.IsEditMode() )
	{
		if (GetGo()->HasCommon())
		{
			GetGo()->GetCommon()->RefreshTriggers();
		}
	}
#	endif

	GPDEBUG_ONLY( AssertValid() );
}

void GoPlacement :: PrivateCalcWorldPosition( void ) const
{
	m_WorldPosCache = GetPosition().WorldPos();
	GetGo()->SetSpaceDirty( false );
}

DWORD GoPlacement :: GoPlacementToNet( GoPlacement* x )
{
	return ( MakeInt( x->GetGoid() ) );
}

GoPlacement* GoPlacement :: NetToGoPlacement( DWORD d, FuBiCookie* cookie )
{
	Go* go = Go::NetToGo( d, cookie );
	return ( go ? go->GetPlacement() : NULL );
}

//////////////////////////////////////////////////////////////////////////////
// class GoGui implementation

GoGui :: GoGui( Go* parent )
	: Inherited( parent )
{
	m_InventoryWidth  = 0;
	m_InventoryHeight = 0;
	m_ItemPower       = 0;
	m_bIdentified	  = false;
	m_bCanSell		  = true;
}

GoGui :: GoGui( const GoGui& source, Go* parent )
	: Inherited( source, parent )
	, m_InventoryIcon    ( source.m_InventoryIcon     )
	, m_EquipRequirements( source.m_EquipRequirements )
	, m_Variation        ( source.m_Variation         )
{
	m_InventoryWidth  = source.m_InventoryWidth;
	m_InventoryHeight = source.m_InventoryHeight;
	m_ItemPower       = source.m_ItemPower;
	m_bIdentified	  = source.m_bIdentified;
	m_bCanSell		  = source.m_bCanSell;
}

GoGui :: ~GoGui( void )
{
	// this space intentionally left blank...
}

const gpstring& GoGui :: GetLastInventoryIcon( void ) const
{
	GoPotion* potion = GetGo()->QueryPotion();
	if ( (potion != NULL) && !potion->GetInventoryIcon2().empty() )
	{
		return ( potion->GetInventoryIcon2() );
	}
	else
	{
		return ( GetInventoryIcon() );
	}
}

const gpstring& GoGui :: GetActiveIcon( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "active_icon" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoGui :: IsSpellBook( void ) const
{
	return ( GetEquipSlot() == ES_SPELLBOOK );
}

int GoGui :: GetInventoryMaxStackable( void ) const
{
	static AutoConstQuery <int> s_Query( "inventory_max_stackable" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

eEquipSlot GoGui :: GetEquipSlot( void ) const
{
	static AutoConstQuery <eEquipSlot> s_Query( "equip_slot" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoGui :: IsDroppable( void ) const
{
	static AutoConstQuery <bool> s_Query( "is_droppable" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

bool GoGui :: IsLoreBook( void ) const
{
	static AutoConstQuery <bool> s_Query( "is_lorebook" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

const gpstring& GoGui :: GetLoreKey( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "lore_key" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

int GoGui :: GetEquipRequirements( EquipRequirementColl& coll ) const
{
	return ( coll.Load( m_EquipRequirements ) );
}

const gpstring& GoGui :: GetEquipRequirements( void ) const
{
	return ( m_EquipRequirements );
}

void GoGui :: SetEquipRequirements( const gpstring& requirements )
{
	m_EquipRequirements = requirements;
}

void GoGui :: AddEquipRequirements( const gpstring& requirements )
{
	if ( !requirements.empty() )
	{
		EquipRequirementColl req( m_EquipRequirements );
		req.Merge( requirements );
		m_EquipRequirements = req.ToString();
	}
}

void GoGui :: SetInventoryIcon( const gpstring& icon )
{
	m_InventoryIcon = icon;
}

void GoGui :: SetInventoryWidth ( int width )
{
	gpassert( (width >= 0) && (width < 20) );		// check sanity
	m_InventoryWidth = width;
}

void GoGui :: SetInventoryHeight( int height )
{
	gpassert( (height >= 0) && (height < 20) );		// check sanity
	m_InventoryHeight = height;
}

const gpstring& GoGui :: GetToolTipColorName( void ) const
{
	static AutoConstQuery <gpstring> s_Query( "tooltip_color" );
	return ( s_Query.Get( GetData()->GetRecord() ) );
}

DWORD GoGui :: GetToolTipColor( void ) const
{
	// The name in the template takes precedence
	DWORD dwTipColor = gUIShell.GetTipColor( GetToolTipColorName() );
	if ( dwTipColor != 0 && !GetToolTipColorName().empty() )
	{
		return dwTipColor;
	}
	else
	{
		dwTipColor = 0;
	}

	// Get the dominant modifier color and return that
	if ( GetGo()->HasMagic() )
	{
		GoMagic::Modifier prefixModifier;
		GoMagic::Modifier suffixModifier;
		bool bPrefix = GetGo()->GetMagic()->GetPrefixModifier( prefixModifier );
		bool bSuffix = GetGo()->GetMagic()->GetSuffixModifier( suffixModifier );

		if ( bPrefix && bSuffix )
		{
			if ( prefixModifier.m_ToolTipWeight >= suffixModifier.m_ToolTipWeight )
			{
				dwTipColor = prefixModifier.m_ToolTipColor;
			}
			else
			{
				dwTipColor = suffixModifier.m_ToolTipColor;
			}
		}
		else if ( bPrefix && !bSuffix )
		{
			dwTipColor = prefixModifier.m_ToolTipColor;
		}
		else if ( !bPrefix && bSuffix )
		{
			dwTipColor = suffixModifier.m_ToolTipColor;
		}
	}

	return dwTipColor;
}

gpwstring GoGui :: MakeToolTipString( void ) const
{
#	if !GP_RETAIL
	if ( gWorldOptions.GetDebugToolTips() )
	{
		nema::HAspect aspect = GetGo()->GetAspect()->GetAspectHandle();
		return ( gpwstringf(
					L"template = '%S%S'\n"
					L"screen_name = '%s'\n"
					L"mesh = '%S'\n"
					L"bitmap = '%S'\n"
					L"inventory_icon = '%S'\n"
					L"active_icon = '%S'",
					GetGo()->GetTemplateName(),
					GetVariation().empty() ? gpstring::EMPTY.c_str() : gpstringf( ":%s", GetVariation().c_str() ).c_str(),
					GetGo()->GetCommon()->GetScreenName().c_str(),
					aspect->GetDebugName(),
					FileSys::GetFileNameOnly( gDefaultRapi.GetTexturePathname( aspect->GetTexture( 0 ) ) ).c_str(),
					GetInventoryIcon().c_str(),
					GetActiveIcon().c_str()
				) );
	}
#	endif // !GP_RETAIL

	gpwstring text;

	// pre-detection of magical name
	GoMagic* magic = GetGo()->QueryMagic();

	gpstring sColor = GetToolTipColorName();

	if ( sColor.empty() )
	{
		if ( magic != NULL && magic->HasNonInnateEnchantments() && !GetGo()->IsPotion() && !GetGo()->IsSpell() )
		{
			DWORD dwTipColor = GetToolTipColor();
			if ( dwTipColor == 0 )
			{
				text += L"<magic>";
			}
			else
			{
				gpstring sTipColor = gUIShell.GetTooltipColorName( dwTipColor );
				if ( !sTipColor.empty() )
				{
					text.appendf( L"<%s>", ToUnicode( sTipColor.c_str() ) );
				}
				else
				{
					text += L"<magic>";
				}
			}
		}
		else if ( GetGo()->IsSpell() )
		{
			if ( magic->GetMagicClass() == MC_NATURE_MAGIC )
			{
				text += L"<nature_magic>";
			}
			else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC )
			{
				text += L"<combat_magic>";
			}
		}
	}
	else
	{
		text.appendf( L"<%s>", ToUnicode( sColor.c_str() ) );
	}

	// screen name
	text += GetGo()->GetCommon()->GetScreenName();
	text += L'\n';

	if ( !GetGo()->GetCommon()->GetCustomName().empty() )
	{
		text += GetGo()->GetCommon()->GetCustomName();
		text += L'\n';
	}

	// mana cost
	if ( (magic != NULL) && magic->IsSpell() )
	{
		if ( magic->GetMagicClass() == MC_NATURE_MAGIC )
		{
			text.appendf( L"<nature_magic>%s\n", gpwtranslate( $MSG$ "Nature Magic" ).c_str() );
		}
		else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC )
		{
			text.appendf( L"<combat_magic>%s\n", gpwtranslate( $MSG$ "Combat Magic" ).c_str() );
		}
	}

	// requirements
	int currentDamageMin	= 0;
	int currentDamageMax	= 0;
	float currentRange		= 0.0f;
	float mana_cost_min		= 0.0f;
	float mana_cost_max		= 0.0f;
	float currentDuration	= 0.0f;

	Go* pCaster = GetGo()->GetParent();
	if ( pCaster && pCaster->HasStash() )
	{
		GoHandle hMember( pCaster->GetStash()->GetStashMember() );
		if ( hMember )
		{
			pCaster = hMember.Get();
		}
	}

	GoHandle parentCaster;
	if ( GetGo()->HasParent() && GetGo()->GetParent()->HasStore() && GetGo()->GetParent()->GetStore()->IsItemStore() && !GetGo()->GetParent()->IsScreenPartyMember() )
	{
		parentCaster = GetGo()->GetParent()->GetStore()->GetShopper();
		pCaster = parentCaster;
	}

	Goid Caster = GOID_INVALID;
	float magic_skill_level = 0;
	float next_magic_skill_level = 0;

	bool bMeetMagicRequirements = false;
	if ( pCaster )
	{
		Caster = pCaster->GetGoid();

		if( !pCaster->HasActor() )
		{
			Caster = pCaster->GetParent()->GetGoid();
			pCaster = pCaster->GetParent();
		}

		if( magic )
		{
			magic_skill_level = pCaster->HasActor()?pCaster->GetActor()->GetSkillLevel( magic->GetSkillClass() ):0;
			next_magic_skill_level = floorf( magic_skill_level + 1 );

			if ( magic->GetRequiredCastLevel() <= magic_skill_level )
			{
				bMeetMagicRequirements = true;
			}
		}
	}


	// description
	if ( !GetGo()->GetCommon()->GetDescription().empty() )
	{
		gpwstring sTranslated = GetGo()->GetCommon()->GetDescription();

		unsigned int macroBegin = sTranslated.find( L'<' );
		unsigned int macroEnd	= sTranslated.find( L'>' );
		while ( macroBegin != gpwstring::npos && macroEnd != gpwstring::npos )
		{
			gpwstring sMacro = sTranslated.substr( macroBegin+1, (macroEnd-macroBegin)-1 );

			gpwstring sBegin	= sTranslated.substr( 0, macroBegin );
			gpwstring sEnd		= sTranslated.substr( macroEnd+1, sTranslated.size() );

			bool bFound = false;

			if ( magic )
			{
				EnchantmentVector::const_iterator iEnchants;
				for ( iEnchants = magic->GetEnchantmentStorage().GetEnchantments().begin(); iEnchants != magic->GetEnchantmentStorage().GetEnchantments().end(); ++iEnchants )
				{
					gpstring sEnchant;
					sEnchant = ::ToString( (*iEnchants)->GetAlterationType() );
					if ( sEnchant.same_no_case( ToAnsi( sMacro ) ) )
					{
						float value = 0.0f;
						bool bEval = false;

						if ( pCaster )
						{
							if( bMeetMagicRequirements )
							{
								bEval = FormulaHelper::EvaluateFormula( (*iEnchants)->GetValueString(), NULL, value, pCaster->GetGoid(), pCaster->GetGoid(), GetGo()->GetGoid() );
							}
							else
							{
								bEval = FormulaHelper::EvaluateFormula( (*iEnchants)->GetValueString(), "#magic = #spell_req_level", value, pCaster->GetGoid(), pCaster->GetGoid(), GetGo()->GetGoid() );
							}
						}						

						if( bEval )
						{
							const float freq = (*iEnchants)->GetFrequency();
							value *= (freq>0)?1.0f/freq:1.0f;
							sBegin.appendf( L"%.f", floorf( value ) );
						}
						else
						{
							sBegin.appendf( L"%.f", floorf((*iEnchants)->GetValue()) );
						}
						bFound = true;
					}
				}
			}

			if ( bFound )
			{
				sTranslated = sBegin;
				sTranslated += sEnd;

				macroBegin = sTranslated.find( L'<' );
				macroEnd	= sTranslated.find( L'>' );
			}
			else
			{
				gperrorf( ( "Macro %S not found in spell description for %S, please check the spell description for an incorrect enchantment tag.", sMacro.c_str(), GetGo()->GetCommon()->GetScreenName().c_str() ) );
				break;
			}
		}

		text += sTranslated.c_str();
		text += L'\n';

		if (	( macroBegin != gpwstring::npos && macroEnd == gpwstring::npos ) ||
				( macroBegin != gpwstring::npos && macroEnd == gpwstring::npos ) )
		{
			gperrorf(( "Mismatched bracket pair in object tooltip: %S", GetGo()->GetCommon()->GetScreenName().c_str() ));
		}
	}


	if ( magic )
	{
		if ( magic->GetIsOneUse() || magic->IsRejuvenationPotion() )
		{
			text.appendf( gpwtranslate( $MSG$ "Can only be used once\n" ) );
		}
		else if ( magic->GetIsOneShot() )
		{
			text.appendf( gpwtranslate( $MSG$ "Single Cast\n" ) );
		}
	}


	// attack
	GoAttack* attack = GetGo()->QueryAttack();
	if ( attack != NULL )
	{
		if ( GetGo()->HasParent() )
		{
			GoHandle hUser( GetGo()->GetParent()->GetGoid() );							
			if ( GetGo()->GetParent()->HasStash() )
			{
				hUser = GetGo()->GetParent()->GetStash()->GetStashMember();
			}
			else if ( GetGo()->HasParent() && GetGo()->GetParent()->HasStore() && GetGo()->GetParent()->GetStore()->IsItemStore() )
			{
				hUser = GetGo()->GetParent()->GetStore()->GetShopper();
			}

			// This is for bug 7779.  The pack mule displays different weapon speeds because the
			// game is asking for the animation speed of that weapon on the character.  So I'm 
			// grabbing the first valid actor I can ( I put the gridbox check in there so we don't select
			// a summoned creature! ) and then setting the user to be that party member.
			if ( hUser && hUser->HasInventory() && hUser->GetInventory()->IsPackOnly() )
			{
				if ( hUser->HasParent() )
				{
					GopColl partyColl = hUser->GetParent()->GetChildren();
					GopColl::iterator iMember;
					for ( iMember = partyColl.begin(); iMember != partyColl.end(); ++iMember )
					{						
						if ( (*iMember) && (*iMember)->HasInventory() && 
							 (*iMember)->GetInventory()->HasGridbox() && !(*iMember)->GetInventory()->IsPackOnly() )
						{
							hUser = (*iMember)->GetGoid();
							break;
						}
					}
				}
			}

			if ( hUser.IsValid() && !GetGo()->IsSpell() && hUser->GetAspect()->GetAspectHandle()->HasBlender() )
			{
				eAnimStance stance = AS_SINGLE_MELEE;
				if ( attack->GetAttackClass() == AC_BOW )
				{
					stance = AS_BOW_AND_ARROW;
				}
				else if ( attack->GetAttackClass() == AC_STAFF )
				{
					stance = AS_STAFF;
				}
				else if ( attack->GetAttackClass() == AC_MINIGUN )
				{
					stance = AS_MINIGUN;
				}
				else if ( attack->GetIsTwoHanded() )
				{
					if ( attack->GetAttackClass() == AC_SWORD )
					{
						stance = AS_TWO_HANDED_SWORD;
					}
					else
					{
						stance = AS_TWO_HANDED_MELEE;
					}
				}

				float attackLoopDuration = attack->GetReloadDelay();
				attackLoopDuration += hUser->GetAspect()->GetAspectHandle()->GetBlender()->GetBaseDuration( CHORE_ATTACK, stance );
				const gpwstring sWeaponSpeed = ReportSys::TranslateW( gContentDb.GetGuiWeaponSpeedString( attackLoopDuration ) );
				if ( !sWeaponSpeed.empty() )
				{
					text.appendf( L"%s\n", sWeaponSpeed.c_str() );
				}

				if ( attack->GetIsTwoHanded() )
				{
					text.append( gpwtranslate( $MSG$ "Two Handed Weapon\n" ) );
				}
			}
		}

		float damage_modifier_min = 0;
		float damage_modifier_max = 0;		

		if( magic && pCaster && pCaster->HasActor() )
		{
			if( !magic->GetAttackDamageModifierMin().empty() )
			{
				if( bMeetMagicRequirements )
				{
					FormulaHelper::EvaluateFormula( magic->GetAttackDamageModifierMin(), NULL, damage_modifier_min, Caster, Caster, GetGo()->GetGoid() );
				}
				else
				{
					FormulaHelper::EvaluateFormula( magic->GetAttackDamageModifierMin(), "#magic = #spell_req_level", damage_modifier_min, Caster, Caster, GetGo()->GetGoid() );
				}
			}
			if( !magic->GetAttackDamageModifierMax().empty() )
			{
				if( bMeetMagicRequirements )
				{
					FormulaHelper::EvaluateFormula( magic->GetAttackDamageModifierMax(), NULL, damage_modifier_max, Caster, Caster, GetGo()->GetGoid() );
				}
				else
				{
					FormulaHelper::EvaluateFormula( magic->GetAttackDamageModifierMax(), "#magic = #spell_req_level", damage_modifier_max, Caster, Caster, GetGo()->GetGoid() );
				}
			}
		}

		int damageMin = fix_precision( attack->GetBaseDamageMin() + damage_modifier_min );
		int damageMax = fix_precision( attack->GetBaseDamageMax() + damage_modifier_max );

		currentDamageMin = damageMin;
		currentDamageMax = damageMax;

		if ( GetGo()->IsMeleeWeapon() || GetGo()->IsRangedWeapon() )
		{
			float totalMin = 0.0f;
			float totalMax = 0.0f;
			attack->EvaluateTotalMinMaxDamage( totalMin, totalMax, false );

			int newDamageMin = fix_precision( totalMin );
			int newDamageMax = fix_precision( totalMax );

			DWORD dwMagicColor	= gUIShell.GetTipColor( "magic" );
			DWORD dwReqColor	= gUIShell.GetTipColor( "required" );

			DWORD color1 = 0;
			DWORD color2 = 0;

			if ( newDamageMin == damageMin )
			{
				color1 = 0xffffffff;
			}
			else if ( newDamageMin > damageMin )
			{
				color1 = dwMagicColor;
			}
			else if ( newDamageMin < damageMin )
			{
				color1 = dwReqColor;
			}

			if ( newDamageMax == damageMax )
			{
				color2 = 0xffffffff;
			}
			else if ( newDamageMax > damageMax )
			{
				color2 = dwMagicColor;
			}
			else if ( newDamageMax < damageMax )
			{
				color2 = dwReqColor;
			}

			text.appendf( gpwtranslate( $MSG$ "Damage: <c:0x%x>%d</c> to <c:0x%x>%d</c>\n" ), color1, newDamageMin, color2, newDamageMax );
		}
		else
		{
			if ( magic && magic->IsSpell() && magic->GetDoesDamagePerSecond() && pCaster )
			{
				text.appendf( gpwtranslate( $MSG$ "Damage Per Second: %d to %d\n" ), damageMin, damageMax );
			}
			else if ( (pCaster && magic) || !magic )
			{			
				text.appendf( gpwtranslate( $MSG$ "Damage: %d to %d\n" ), damageMin, damageMax );			
			}
		}

		// ranged
		if ( attack->GetIsProjectile() )
		{
			text.appendf( gpwtranslate( $MSG$ "Range: %.f Meters\n" ), attack->GetAttackRange() );

			if ( attack->GetAttackClass() == AC_MINIGUN && GetGo()->GetAspect()->GetMaxMana() != 0 )
			{
				text.appendf( gpwtranslate( $MSG$ "Ammo/Fuel: %0.0f/%0.0f\n" ), GetGo()->GetAspect()->GetCurrentMana(), GetGo()->GetAspect()->GetMaxMana() );
			}
		}
	}

	// defend
	GoDefend* defend = GetGo()->QueryDefend();
	if ( defend != NULL )
	{
		text.appendf( gpwtranslate( $MSG$ "Defense: %.f\n" ), defend->GetDefense() );
	}

	// mana cost
	if ( (magic != NULL) && magic->IsSpell() )
	{
		// Magic requirement
		if ( magic->GetMagicClass() == MC_NATURE_MAGIC && magic->GetRequiredCastLevel() != 0  )
		{
			if ( !bMeetMagicRequirements )
			{
				text += L"<required>";
			}
			text.appendf( gpwtranslate( $MSG$ "Requires Nature Magic: Level %.f\n" ), magic->GetRequiredCastLevel() );
		}
		else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC && magic->GetRequiredCastLevel() != 0 )
		{
			if ( !bMeetMagicRequirements )
			{
				text += L"<required>";
			}
			text.appendf( gpwtranslate( $MSG$ "Requires Combat Magic: Level %.f\n" ), magic->GetRequiredCastLevel() );
		}

		// --- Current level

		// Range
		if ( magic->GetUsageContextFlags() == UC_AGGRESSIVE || magic->GetUsageContextFlags() == UC_OFFENSIVE )
		{
			text.appendf( gpwtranslate( $MSG$ "Range: %.f\n" ), magic->GetCastRange() );

			currentRange = magic->GetCastRange();
		}

		// Duration
		float duration = 0;
		if ( pCaster )
		{
			if( bMeetMagicRequirements )
			{
				duration = magic->EvaluateEffectDuration( pCaster, pCaster, NULL );
			}
			else
			{
				duration = magic->EvaluateEffectDuration( pCaster, pCaster, "#magic = #spell_req_level" );
			}
		}

		if ( duration > 0 )
		{
			if ( magic->GetUsageContextFlags() != UC_AGGRESSIVE && magic->GetUsageContextFlags() != UC_OFFENSIVE )
			{
				text.appendf( gpwtranslate( $MSG$ "Duration Per Cast: %.f\n" ), duration );
			}
			else
			{
				text.appendf( gpwtranslate( $MSG$ "Duration: %.f\n" ), duration );
			}
			currentDuration = duration;
		}

		// Mana cost - caster may be invalid ( item is trading hands )
		if( pCaster )
		{
			if( bMeetMagicRequirements )
			{
				magic->GetManaCostRangeUI( mana_cost_min, mana_cost_max, pCaster );
			}
			else
			{
				magic->GetManaCostRangeUI( mana_cost_min, mana_cost_max, pCaster, "#magic = #spell_req_level" );
			}

			if( mana_cost_min == mana_cost_max )
			{
				text.appendf( gpwtranslate( $MSG$ "Mana Cost: %.f\n" ), mana_cost_min );
			}
			else
			{
				text.appendf( gpwtranslate( $MSG$ "Mana Cost: %.f - %.f\n" ), mana_cost_min, mana_cost_max );
			}
		}

		// --- Next level

		if( pCaster && magic->CanReachNextLevel( pCaster ) && (magic_skill_level >= magic->GetRequiredCastLevel()) )
		{
			bool bSkillDiff = false;

			// Damage
			if ( attack != NULL )
			{
				float damage_modifier_min = 0;
				float damage_modifier_max = 0;

				if( pCaster->HasActor() )
				{
					Goid Caster = pCaster->GetGoid();

					if( !magic->GetAttackDamageModifierMin().empty() )
					{
						FormulaHelper::EvaluateFormula( magic->GetAttackDamageModifierMin(), "#magic = (#magic+1)", damage_modifier_min, Caster, Caster, GetGo()->GetGoid() );
					}
					if( !magic->GetAttackDamageModifierMax().empty() )
					{
						FormulaHelper::EvaluateFormula( magic->GetAttackDamageModifierMax(), "#magic = (#magic+1)", damage_modifier_max, Caster, Caster, GetGo()->GetGoid() );
					}
				}				

				int damageMin = fix_precision( attack->GetBaseDamageMin() + damage_modifier_min );
				int damageMax = fix_precision( attack->GetBaseDamageMax() + damage_modifier_max );
				if ( !(damageMin == currentDamageMin && damageMax == currentDamageMax) )
				{
					if ( !bSkillDiff )
					{
						if ( magic->GetMagicClass() == MC_NATURE_MAGIC )
						{
							text += L"\n<nature_magic>";
						}
						else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC )
						{
							text += L"\n<combat_magic>";
						}

						text.appendf( gpwtranslate( $MSG$ "At Skill Level %.f:\n" ), next_magic_skill_level );
						bSkillDiff = true;
					}

					text.appendf( gpwtranslate( $MSG$ "Damage: %d to %d\n" ), damageMin, damageMax );
				}
			}

			// Range
			if ( magic->GetUsageContextFlags() == UC_AGGRESSIVE || magic->GetUsageContextFlags() == UC_OFFENSIVE )
			{
				if ( currentRange != magic->GetCastRange() )
				{
					if ( !bSkillDiff )
					{
						if ( magic->GetMagicClass() == MC_NATURE_MAGIC )
						{
							text += L"\n<nature_magic>";
						}
						else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC )
						{
							text += L"\n<combat_magic>";
						}

						text.appendf( gpwtranslate( $MSG$ "At Skill Level %.f:\n" ), next_magic_skill_level );
						bSkillDiff = true;
					}

					text.appendf( gpwtranslate( $MSG$ "Range: %.f\n" ), magic->GetCastRange() );
				}
			}

			// Duration
			duration = magic->EvaluateEffectDuration( pCaster, pCaster, "#magic = (#magic+1)" );
			if ( duration > 0 )
			{
				if ( currentDuration != duration )
				{
					if ( !bSkillDiff )
					{
						if ( magic->GetMagicClass() == MC_NATURE_MAGIC )
						{
							text += L"\n<nature_magic>";
						}
						else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC )
						{
							text += L"\n<combat_magic>";
						}

						text.appendf( gpwtranslate( $MSG$ "At Skill Level %.f:\n" ), next_magic_skill_level );
						bSkillDiff = true;
					}
					text.appendf( gpwtranslate( $MSG$ "Duration: %.f\n" ), duration );
				}
			}

			// Mana cost
			float next_mana_cost_min = mana_cost_min;
			float next_mana_cost_max = mana_cost_max;
			magic->GetManaCostRangeUI( next_mana_cost_min, next_mana_cost_max, pCaster, "#magic = (#magic+1)" );
			if ( (mana_cost_min != next_mana_cost_min) || (mana_cost_max != next_mana_cost_max) )
			{
				if ( !bSkillDiff )
				{
					if ( magic->GetMagicClass() == MC_NATURE_MAGIC )
					{
						text += L"\n<nature_magic>";
					}
					else if ( magic->GetMagicClass() == MC_COMBAT_MAGIC )
					{
						text += L"\n<combat_magic>";
					}


					text.appendf( gpwtranslate( $MSG$ "At Skill Level %.f:\n" ), next_magic_skill_level );
					bSkillDiff = true;
				}

				if( next_mana_cost_min == next_mana_cost_max )
				{
					text.appendf( gpwtranslate( $MSG$ "Mana Cost: %.f\n" ), next_mana_cost_min );
				}
				else
				{
					text.appendf( gpwtranslate( $MSG$ "Mana Cost: %.f - %.f\n" ), next_mana_cost_min, next_mana_cost_max );
				}
			}
		}
	}

	// equip requirements
	EquipRequirementColl requirements;
	if ( GetEquipRequirements( requirements ) )
	{
		text += requirements.MakeToolTipString( pCaster );
	}

	// gold
	if ( GetGo()->IsGold() )
	{
		text.appendf( gpwtranslate( $MSG$ "Value: %d\n" ), GetGo()->GetAspect()->GetGoldValue() );
	}

	// enchantments
	if ( (magic != NULL) && magic->HasEnchantments() && !GetGo()->IsSpell() )
	{
		text += magic->GetEnchantmentStorage().MakeToolTipString( GetGo()->IsPotion() );
	}

	// don't have a trailing newline
	if ( *text.rbegin() == '\n' )
	{
		text.erase( text.length() - 1 );
	}

	// done
	return ( text );
}

GoComponent* GoGui :: Clone( Go* newParent )
{
	return ( new GoGui( *this, newParent ) );
}

bool GoGui :: Xfer( FuBi::PersistContext& persist )
{
	persist.Xfer( "inventory_icon",     m_InventoryIcon     );
	persist.Xfer( "inventory_width",    m_InventoryWidth    );
	persist.Xfer( "inventory_height",   m_InventoryHeight   );
	persist.Xfer( "equip_requirements", m_EquipRequirements );
	persist.Xfer( "can_sell",			m_bCanSell			);

	if ( persist.IsFullXfer() )
	{
		persist.Xfer( "m_ItemPower",   m_ItemPower   );
		persist.Xfer( "m_bIdentified", m_bIdentified );
		persist.Xfer( "m_Variation", m_Variation );
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
