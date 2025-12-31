//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoData.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_World.h"
#include "GoData.h"

#include "ContentDb.h"
#include "FuBi.h"
#include "FuBiSchemaImpl.h"
#include "Fuel.h"
#include "GoDb.h"
#include "SkritEngine.h"
#include "StdHelp.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// class GoDataComponent implementation

const gpstring& StringCollectorCb( const gpstring& str )
{
	return ( gContentDb.AddString( str ) );
}

GoDataComponent :: GoDataComponent( void )
{
	m_Impl = new Impl;
}

GoDataComponent :: GoDataComponent( const GoDataComponent& other )
{
	gpassert( other.m_Impl != NULL );
	m_Impl = other.m_Impl;
	++m_Impl->m_RefCount;
}

GoDataComponent :: ~GoDataComponent( void )
{
	gpassert( m_Impl != NULL );
	if ( --m_Impl->m_RefCount == 0 )
	{
		delete ( m_Impl );
	}
}

bool GoDataComponent :: InitSchema( FastFuelHandle fuel, bool serverOnly, bool canServerExistOnly, FuBi::StoreHeaderSpec* storeHeaderSpec, FlagColl* flagColl )
{
	gpassert( !IsInitialized() );
	Impl* impl = GetImplForWrite();
	impl->Init( storeHeaderSpec, flagColl );
	impl->m_ServerOnly = serverOnly;
	impl->m_CanServerExistOnly = canServerExistOnly;

// Extract header data.

	// set name
	storeHeaderSpec->SetName( gContentDb.AddString( fuel.GetName() ) );

#	if ( !GP_RETAIL )
	{
		// extract doco (not optional!)
		if ( fuel.Get( "doc", impl->m_Docs, false ) )
		{
			storeHeaderSpec->SetDocs( impl->m_Docs );

			// override internal c++ docs to use schema's instead
			FuBi::eVarType type = gContentDb.FindFuBiTypeByName( GetName() );
			if ( type != FuBi::VAR_UNKNOWN )
			{
				FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( type );
				gpassert( spec != NULL );
				spec->m_Docs = impl->m_Docs;
			}
		}

		// extract required component set
		FastFuelFindHandle fc( fuel );

		if ( fc.FindFirstValue( "required_component*" ) )
		{
			gpstring required;
			while ( fc.GetNextValue( required ) )
			{
				impl->m_RequiredColl.push_back( gContentDb.AddString( required ) );
			}
		}

		// extract internal set
		if ( fc.FindFirstValue( "internal*" ) )
		{
			gpstring internal;
			while ( fc.GetNextValue( internal ) )
			{
				AddInternal( internal );
			}
		}
	}
#	endif // !GP_RETAIL

// Extract schema.

	// loop over all non-typed blocks

	FastFuelHandleColl params;
	fuel.ListChildrenTyped( params, "" );

	FastFuelHandleColl::iterator i, begin = params.begin(), end = params.end();
	for ( i = begin ; i != end ; ++i )
	{

	// Verify name.

		const gpstring& paramName = gContentDb.AddString( (*i).GetName() );
		if ( storeHeaderSpec->HasColumn( paramName ) )
		{
			gperrorf(( "Duplicate field name '%s' found in component '%s'\n",
					   paramName.c_str(), GetName().c_str() ));
			continue;
		}

	// Extract type.

		// get the type

		gpstring typeName = gContentDb.AddString( (*i).GetString( "type" ) );
		if ( typeName.empty() )
		{
			gperrorf(( "Type not specified for field '%s' in component '%s', setting to internal type\n",
					   paramName.c_str(), GetName().c_str() ));
			GPDEV_ONLY( AddInternal( paramName ) );
			continue;
		}

		// get special type flags
		eFlags typeFlags = FLAG_NONE;
		if ( typeName.same_no_case( "lstring" ) )
		{
			typeFlags = FLAG_LOCALIZE | FLAG_QUOTED;
			typeName.erase( 0, 1 );
		}
		else if ( typeName.same_no_case( "qstring" ) )
		{
			typeFlags = FLAG_QUOTED;
			typeName.erase( 0, 1 );
		}

		// convert to fubi type
		FuBi::eVarType fubiType = gFuBiSysExports.FindTypeInternalOk( typeName );
		if ( fubiType == FuBi::VAR_UNKNOWN )
		{
			gperrorf(( "Found unrecognized type '%s' for field '%s' in component '%s', setting to internal type\n",
					   typeName.c_str(), paramName.c_str(), GetName().c_str() ));
			GPDEV_ONLY( AddInternal( paramName ) );
			continue;
		}

		// add to the store
		FuBi::Store::eColl coll;
		int index = impl->m_Store->AddGeneric( fubiType, &coll );

		// special: it may be a pointerclass, so convert it
		FuBi::TypeSpec fubiTypeSpec( fubiType );
		if (   FuBi::IsUser( fubiType )
			&& (gFuBiSysExports.FindClass( fubiType )->m_Flags & FuBi::ClassSpec::FLAG_POINTERCLASS) )
		{
			fubiTypeSpec.m_Flags = FuBi::TypeSpec::FLAG_POINTER;
		}

	// Extract constraint.

#		if ( !GP_RETAIL )

		// this is our constraint
		FuBi::ConstraintSpec* constraintSpec = NULL;

		// get the text
		gpstring constraint;
		if ( (*i).Get( "constrain", constraint ) )
		{
			stringtool::RemoveBorderingWhiteSpace( constraint );

			bool isInt = fubiTypeSpec.IsSimpleInteger() && (fubiTypeSpec.m_Type != FuBi::VAR_BOOL);
			bool isFloat = fubiTypeSpec.IsSimpleFloat();

			if ( isInt || isFloat )
			{
				if ( constraint.same_no_case( "positive" ) )
				{
					// 0 <= x

					if ( isInt )
					{
						FuBi::IntConstrainRange range;
						range.m_Flags = FuBi::IntConstrainRange::FLAG_LEFT_INCLUSIVE
									  | FuBi::IntConstrainRange::FLAG_RIGHT_OPEN;
						constraintSpec = new FuBi::IntConstrainRange( range );
					}
					else
					{
						FuBi::FloatConstrainRange range;
						range.m_Flags = FuBi::FloatConstrainRange::FLAG_LEFT_INCLUSIVE
									  | FuBi::FloatConstrainRange::FLAG_RIGHT_OPEN;
						constraintSpec = new FuBi::FloatConstrainRange( range );
					}
				}
				else if ( constraint.same_no_case( "positive_nonzero" ) )
				{
					// 0 < x

					if ( isInt )
					{
						FuBi::IntConstrainRange range;
						range.m_Flags = FuBi::IntConstrainRange::FLAG_RIGHT_OPEN;
						constraintSpec = new FuBi::IntConstrainRange( range );
					}
					else
					{
						FuBi::FloatConstrainRange range;
						range.m_Flags = FuBi::FloatConstrainRange::FLAG_RIGHT_OPEN;
						constraintSpec = new FuBi::FloatConstrainRange( range );
					}
				}
				else if ( constraint.same_no_case( "negative" ) )
				{
					// x < 0

					if ( isInt )
					{
						FuBi::IntConstrainRange range;
						range.m_Flags = FuBi::IntConstrainRange::FLAG_LEFT_OPEN;
						constraintSpec = new FuBi::IntConstrainRange( range );
					}
					else
					{
						FuBi::FloatConstrainRange range;
						range.m_Flags = FuBi::FloatConstrainRange::FLAG_LEFT_OPEN;
						constraintSpec = new FuBi::FloatConstrainRange( range );
					}
				}
				else if ( constraint.same_no_case( "negative_nonzero" ) )
				{
					// x <= 0

					if ( isInt )
					{
						FuBi::IntConstrainRange range;
						range.m_Flags = FuBi::IntConstrainRange::FLAG_LEFT_OPEN
									  | FuBi::IntConstrainRange::FLAG_RIGHT_INCLUSIVE;
						constraintSpec = new FuBi::IntConstrainRange( range );
					}
					else
					{
						FuBi::FloatConstrainRange range;
						range.m_Flags = FuBi::FloatConstrainRange::FLAG_LEFT_OPEN
									  | FuBi::FloatConstrainRange::FLAG_RIGHT_INCLUSIVE;
						constraintSpec = new FuBi::FloatConstrainRange( range );
					}
				}
				else if ( constraint.same_no_case( "range", 5 ) == 0 )
				{
					// $$$ do this
				}
			}

			if ( constraintSpec == NULL )
			{
				if ( constraint.same_no_case( "choose_file", 11 ) )
				{
					// $$$ do this
				}
				else if ( constraint.same_no_case( "choose", 6 ) )
				{
					const char* begin = constraint.c_str() + 6;

					// choose block name or series?
					if ( constraint.find( "," ) == gpstring::npos )
					{
						// extract the block name
						while ( isspace( *begin ) )  ++begin;
						if ( *begin++ != '(' )  goto badParse;
						while ( isspace( *begin ) )  ++begin;
						const char* end = begin;
						while ( *end && !isspace( *end ) )  ++end;
						const char* past = end;
						while ( isspace( *past ) )  ++past;
						if ( *past++ != ')' )  goto badParse;
						while ( isspace( *past ) )  ++past;
						if ( *past != '\0' )  goto badParse;

						// take the name
						gpstring name( begin, end );

						// use it
						constraintSpec = gContentDb.CreateConstraint( name );
						if ( constraintSpec == NULL )
						{
							gperrorf((	"Bad 'choose' block name '%s' at fuel '%s'\n",
										name.c_str(), (*i).GetAddress().c_str() ));
						}
					}
					else
					{
						// $$$ do this
					}
				}
				else if ( constraint.same_no_case( "verify", 6 ) )
				{
					// $$$ do this
				}

				// skip bad parse code
				goto done;

			badParse:
				gperrorf((	"Failure to parse constraint '%s' at fuel '%s'\n",
							constraint.c_str(), (*i).GetAddress().c_str() ));

			done:
				// $ just a label
				;
			}
		}

		// unconstrained regular enum?
		if (   (constraintSpec == NULL)
			&& FuBi::IsEnum( fubiTypeSpec.m_Type )
			&& gFuBiSysExports.FindEnum( fubiTypeSpec.m_Type )->m_Exporter.IsContinuous() )
		{
			constraintSpec = new FuBi::ConstrainEnumStrings( fubiTypeSpec.m_Type );
		}

#		endif // !GP_RETAIL

	// Extract remaining data.

#		if ( !GP_RETAIL )
		gpstring docs;
		{
			// extract doco (not optional!)
			if ( (*i).Get( "doc", docs, false ) && docs.empty() )
			{
				gpwarningf(( "Field '%s' in component '%s' must have documentation!\n",
							 paramName.c_str(), GetName().c_str() ));
			}
		}
#		endif // !GP_RETAIL

		// add the column
		int column = storeHeaderSpec->AddColumn( fubiTypeSpec, paramName, coll, index  GPDEV_PARAM( docs ) );

		// add constraint
#		if ( !GP_RETAIL )
		storeHeaderSpec->SetConstraint( column, constraintSpec );
#		endif // !GP_RETAIL

		// extract default
		gpstring defValue;
		if ( (*i).Get( "default", defValue ) )
		{
			defValue = gContentDb.AddString( defValue );
			storeHeaderSpec->SetDefaultValue( column, defValue );
			if ( !impl->m_Record->SetAsString( column, defValue ) )
			{
				gperrorf(( "Unable to convert default value '%s' for field '%s' in component '%s'\n",
						   defValue.c_str(), paramName.c_str(), GetName().c_str() ));
			}
		}

		eFlags flags = FLAG_NONE;
		gpstring flagsText;

		// extract and convert flags
		if ( (*i).Get( "flags", flagsText ) && !FromString( flagsText, flags ) )
		{
			gperrorf(( "Field '%s' in component '%s' has illegal flags '%s' specified\n",
					   paramName.c_str(), GetName().c_str(), flagsText.c_str() ));
		}

		// take type flags
		flags |= typeFlags;

		// check sanity on localization
		if ( (flags & FLAG_LOCALIZE) && (fubiType != FuBi::VAR_STRING) )
		{
			gperrorf(( "Field '%s' in component '%s' is tagged as LOCALIZE, but it is not a string type!\n",
					   paramName.c_str(), GetName().c_str() ));
		}

		// check sanity on quoted-ness
		if ( (flags & FLAG_QUOTED) && (fubiType != FuBi::VAR_STRING) )
		{
			gperrorf(( "Field '%s' in component '%s' is tagged as QUOTED, but it is not a string type!\n",
					   paramName.c_str(), GetName().c_str() ));
		}

		// take the flags
		flagColl->push_back( flags );
	}

// Validation.

	// verify that we match up
	gpassert( storeHeaderSpec->GetColumnCount() == scast <int> ( flagColl->size() ) );

	// init override collection
	GPDEV_ONLY( impl->m_OverrideColl.resize( impl->m_StoreHeaderSpec->GetColumnCount(), false ); )

	// all ok
	impl->m_Type = TYPE_GENERIC;
	return ( true );
}

bool FilterReservedNames( const char* name )
{
	// skip properties that are reserved names
	return ( *name != '_' );
}

bool GoDataComponent :: InitSchema( Skrit::HObject skrit, bool& serverOnly, FuBi::StoreHeaderSpec* storeHeaderSpec, FlagColl* flagColl )
{
	gpassert( !IsInitialized() );
	Impl* impl = GetImplForWrite();
	serverOnly = true;
	impl->Init( storeHeaderSpec, flagColl );

	// take the skrit
	impl->m_SkritObject = skrit;
	gSkritEngine.AddRefObject( impl->m_SkritObject );

	// get the schema
	const FuBi::StoreHeaderSpec* skritSpec = impl->m_SkritObject->GetImpl()->GetStoreHeaderSpec();

	// copy it
	impl->m_SkritObject->GetImpl()->BuildPropertiesAndStore( *storeHeaderSpec, *impl->m_Store, makeFunctor( FilterReservedNames ) );

	// extract reserved fields
	if ( skritSpec != NULL )
	{
		FuBi::HeaderSpec::ConstIter i, begin = skritSpec->GetColumnBegin(), end = skritSpec->GetColumnEnd();
		for ( i = begin ; i != end ; ++i )
		{
			const gpstring& name = i->GetName();
			if ( !FilterReservedNames( name ) )
			{
				if ( name.same_no_case( "_server_only" ) )
				{
					impl->m_SkritObject->GetRecord()->Get( i - begin, serverOnly );
				}
#				if !GP_RETAIL
				else if ( name.same_no_case( "_doc" ) )
				{
					impl->m_SkritObject->GetRecord()->Get( i - begin, impl->m_Docs );
				}
				else if ( name.same_no_case( "_required_component", sizeof( "_required_component" ) - 1 ) )
				{
					gpstring reqName;
					impl->m_SkritObject->GetRecord()->Get( i - begin, reqName );
					impl->m_RequiredColl.push_back( reqName );
				}
				else
				{
					gperrorf(( "Unrecognized reserved property '%s' in Skrit component '%s'\n",
							   name.c_str(), GetName().c_str() ));
				}
#				endif // !GP_RETAIL

				// don't persist this var!
				impl->m_SkritObject->GetImpl()->GetStoreHeaderSpec()->GetColumn( name ).m_Extra->m_NoXfer = true;
			}
		}
	}

	// make sure all strings get quoted, flags are none for the rest
	FuBi::HeaderSpec::ConstIter i, ibegin = storeHeaderSpec->GetColumnBegin(), iend = storeHeaderSpec->GetColumnEnd();
	for ( i = ibegin ; i != iend ; ++i )
	{
		eFlags flags = FLAG_NONE;
		if ( i->m_Type.m_Type == FuBi::VAR_STRING )
		{
			flags = FLAG_QUOTED;
		}
		flagColl->push_back( flags );
	}

	// init override collection
	GPDEV_ONLY( impl->m_OverrideColl.resize( storeHeaderSpec->GetColumnCount(), false ); )

	// get at the impl for the skrit
	const Skrit::ObjectImpl* simpl = impl->m_SkritObject->GetImpl();

	// get some serial id's for query
	static UINT s_UpdateSerialId = gFuBiSysExports.FindEventSerialByName( "OnGoUpdate" );
	static UINT s_DrawSerialId   = gFuBiSysExports.FindEventSerialByName( "OnGoDraw"   );

	// see if we want updates so they can poll
	impl->m_SkritWantsUpdates = simpl->DoesWantPolls();
	if ( !impl->m_SkritWantsUpdates )
	{
		// check for our own method
		impl->m_SkritWantsUpdates = simpl->DoesUseEvent( s_UpdateSerialId );
	}

	// see if we want draws
	impl->m_SkritWantsDraws = simpl->DoesUseEvent( s_DrawSerialId );

	// all ok
	impl->m_Type = TYPE_SKRIT;
	impl->m_ServerOnly = serverOnly;
	impl->m_CanServerExistOnly = serverOnly;

	return ( true );
}

bool GoDataComponent :: SpecializeSchema( FastFuelHandle fuel )
{
	// $$$ use "unused param report" and dump set of extra fuel entries

	bool success = true;

	// check for field name
	int column = GetImplForRead()->m_StoreHeaderSpec->FindColumn( fuel.GetName() );
	if ( column != -1 )
	{
		// specialize the schema
		// $$$ remember to use gContentDb.AddString()
		GP_Unimplemented$$$();
	}
	else
	{
		// try as internal
		success = SpecializeInternal( fuel );
	}

	return ( success );
}

bool GoDataComponent :: SpecializeInternal( FastFuelHandle fuel, bool ignoreSchemaChanges )
{
	bool success = true;
	FastFuelHandle* internal = FindInternal( fuel.GetName() );

	// specialization?
	if ( internal != NULL )
	{
		// specialize (completely override) the internal field
		*internal = fuel;
	}
	else
	{
		// check for schema if we're ignoring it
		if ( ignoreSchemaChanges && GetImplForRead()->m_StoreHeaderSpec->HasColumn( fuel.GetName() ) )
		{
			// just ignore it
			return ( true );
		}

#		if ( GP_RETAIL )
		{
			// just add it
			GetImplForWrite()->m_InternalColl.push_back( fuel );
		}
#		else // GP_RETAIL
		{
			// not a valid specialization
			gperrorf(( "Attempted to specialize a nonexistent field '%s' in component '%s' at fuel '%s'\n",
					   fuel.GetName(), GetName().c_str(), fuel.GetAddress().c_str() ));
			success = false;
		}
#		endif // GP_RETAIL
	}

	return ( success );
}

bool GoDataComponent :: SpecializeData( FastFuelHandle fuel, bool ignoreSchemaChanges )
{
	// $$$ use "unused param report" and dump set of extra fuel entries

	bool success = true;

	// specialize fields
	gpstring key, value;
	FastFuelFindHandle fh( fuel );

	for ( fh.FindFirstKeyAndValue() ; fh.GetNextKeyAndValue( key, value ) ; )
	{
		int column = GetImplForRead()->m_StoreHeaderSpec->FindColumn( key );
		if ( column != FuBi::INVALID_COLUMN_INDEX )
		{
			Impl* impl = GetImplForWrite();

			// optionally translate the string
			const char* str = value;
			if ( (*impl->m_FlagColl)[ column ] & FLAG_LOCALIZE )
			{
				str = ReportSys::TranslateA( str );
			}

			// commit the value
			if ( !impl->m_Record->SetAsString( column, str ) )
			{
				gperrorf(( "Unable to convert value '%s' for field '%s' at fuel '%s'\n",
						   stringtool::EscapeCharactersToTokens( value ).c_str(), key.c_str(), fuel.GetAddress().c_str() ));
				success = false;
			}

			// set override flag
#			if ( !GP_RETAIL && 0 )
			{
				// $ note: i disabled this code because we aren't using it
				//         anyway, but want to leave the option here in case
				//         the mod community wants something like this for
				//         after we ship. -sb

				// $ special: to remove forced overrides and only allow
				//            overrides that are detected by comparing template
				//            data with instance data, set this bit.

				if ( GoDb::DoesSingletonExist() && !gGoDb.GetOverrideDetectOnly() )
				{
					gpassert( (column >= 0) && (column < scast <int> ( impl->m_OverrideColl.size()) ) );
					GPDEV_ONLY( impl->m_OverrideColl[ column ] = true; )
				}
			}
#			endif // !GP_RETAIL
		}
		else
		{
			gperrorf(( "Cannot specialize nonexistent field '%s' at fuel '%s'\n", key.c_str(), fuel.GetAddress().c_str() ));
			success = false;
		}
	}

	// now specialize internals
	FastFuelHandleColl internals;
	if ( fuel.ListChildren( internals ) )
	{
		FastFuelHandleColl::const_iterator i, begin = internals.begin(), end = internals.end();
		for ( i = begin ; i != end ; ++i )
		{
			// specialize it as internal data
			if ( !SpecializeInternal( *i, ignoreSchemaChanges ) )
			{
				success = false;
			}
		}
	}

	// done
	return ( success );
}

const gpstring& GoDataComponent :: GetName( void ) const
{
	return ( GetImplForRead()->m_StoreHeaderSpec->GetName() );
}

const FuBi::HeaderSpec* GoDataComponent :: GetHeaderSpec( void ) const
{
	gpassert( GetImplForRead()->m_StoreHeaderSpec != NULL );
	return ( GetImplForRead()->m_StoreHeaderSpec );
}

Skrit::HObject GoDataComponent :: GetSkritObject( void ) const
{
	gpassert( (GetType() == TYPE_SKRIT) && GetImplForRead()->m_SkritObject );
	return ( GetImplForRead()->m_SkritObject.GetHandle() );
}

void GoDataComponent :: CopyForWrite( void )
{
	gpassert( m_Impl != NULL );
	if ( m_Impl->m_RefCount > 1 )
	{
		Impl* oldImpl = m_Impl;
		m_Impl = new Impl( *oldImpl );
		--oldImpl->m_RefCount;
	}
}

GoDataComponent::eFlags GoDataComponent :: GetFlags( const char* fieldName ) const
{
	return ( GetFlags( GetImplForRead()->m_StoreHeaderSpec->FindColumn( fieldName ) ) );
}

#if !GP_RETAIL

bool GoDataComponent :: AddInternal( const gpstring& name )
{
	if ( name.empty() )
	{
		gperrorf(( "Empty internal field name found in component '%s'\n",
				   GetName().c_str() ));
	}

	std::pair <InternalColl::iterator, bool> rc
			= GetImplForWrite()->m_InternalColl.insert( std::make_pair( gContentDb.AddString( name ), FastFuelHandle() ) );
	if ( !rc.second )
	{
		gperrorf(( "Duplicate internal field name '%s' found in component '%s'\n",
				   name.c_str(), GetName().c_str() ));
	}
	return ( rc.second );
}

#endif // !GP_RETAIL

FastFuelHandle* GoDataComponent :: FindInternal( const char* name )
{
	Impl* impl = GetImplForWrite();

#	if ( GP_RETAIL )
	{
		InternalColl::iterator i, begin = impl->m_InternalColl.begin(), end = impl->m_InternalColl.end();
		for ( i = begin ; i != end ; ++i )
		{
			if ( ::stricmp( i->GetName(), name ) == 0 )
			{
				return ( &*i );
			}
		}
		return ( NULL );
	}
#	else // GP_RETAIL
	{
		InternalColl::iterator found = impl->m_InternalColl.find( name );
		return ( (found == impl->m_InternalColl.end()) ? NULL : &found->second );
	}
#	endif // GP_RETAIL
}

//////////////////////////////////////////////////////////////////////////////
// struct GoDataComponent::Impl implementation

GoDataComponent::Impl :: Impl( void )
{
	// build objects
	m_Type               = TYPE_NONE;
	m_SkritWantsUpdates  = false;
	m_SkritWantsDraws    = false;
	m_ServerOnly         = false;
	m_CanServerExistOnly = false;
	m_FlagColl           = NULL;
	m_StoreHeaderSpec    = NULL;
	m_Store              = new FuBi::Store;
	m_Record             = NULL;

	// start with ref of 1
	m_RefCount = 1;
}

GoDataComponent::Impl :: Impl( Impl& other )
	: m_InternalColl( other.m_InternalColl )
#	  if !GP_RETAIL
	, m_Docs        ( other.m_Docs )
	, m_RequiredColl( other.m_RequiredColl )
	, m_OverrideColl( other.m_OverrideColl.size(), false )
#	  endif // !GP_RETAIL
{
	// copy objects
	m_Type               = other.m_Type;
	m_SkritWantsUpdates  = other.m_SkritWantsUpdates;
	m_SkritWantsDraws    = other.m_SkritWantsDraws;
	m_ServerOnly         = other.m_ServerOnly;
	m_CanServerExistOnly = other.m_CanServerExistOnly;
	m_FlagColl           = other.m_FlagColl;
	m_StoreHeaderSpec    = other.m_StoreHeaderSpec;
	m_Store              = new FuBi::Store( *other.m_Store );
	m_Record             = new FuBi::StoreHeaderSpec::Record( m_Store, m_StoreHeaderSpec );

	// copy the skrit
	m_SkritObject = other.m_SkritObject.GetHandle();
	if ( m_SkritObject )
	{
		gSkritEngine.AddRefObject( m_SkritObject );
	}

	// start with ref of 1
	m_RefCount = 1;
}

GoDataComponent::Impl :: ~Impl( void )
{
	// only do this when ref count is zero
	gpassert( m_RefCount == 0 );

	// reset internal c++ docs now that we're going away
#	if ( !GP_RETAIL )
	{
		if ( m_Type == TYPE_GENERIC )
		{
			FuBi::eVarType type = gContentDb.FindFuBiTypeByName( m_StoreHeaderSpec->GetName() );
			if ( type != FuBi::VAR_UNKNOWN )
			{
				FuBi::ClassSpec* spec = gFuBiSysExports.FindClass( type );
				gpassert( spec != NULL );
				spec->m_Docs = NULL;
			}
		}
	}
#	endif // !GP_RETAIL

	// delete remaining components
	Delete( m_Store  );
	Delete( m_Record );
}

void GoDataComponent::Impl :: Init( FuBi::StoreHeaderSpec* storeHeaderSpec, FlagColl* flagColl )
{
	storeHeaderSpec->SetStringCollectorCb( makeFunctor( StringCollectorCb ) );

	m_FlagColl        = flagColl;
	m_StoreHeaderSpec = storeHeaderSpec;
	m_Record          = new FuBi::StoreHeaderSpec::Record( m_Store, m_StoreHeaderSpec );
}

//////////////////////////////////////////////////////////////////////////////
// enum GoDataComponent::eFlags implementation

static const stringtool::BitEnumStringConverter::Entry s_FlagEntries[] =
{
	{  "none",     GoDataComponent::FLAG_NONE      },
	{  "required", GoDataComponent::FLAG_REQUIRED  },
	{  "localize", GoDataComponent::FLAG_LOCALIZE  },
	{  "quoted",   GoDataComponent::FLAG_QUOTED    },
	{  "advanced", GoDataComponent::FLAG_ADVANCED  },
	{  "hidden",   GoDataComponent::FLAG_HIDDEN    },
	{  "dev",      GoDataComponent::FLAG_DEV       },
};

static stringtool::BitEnumStringConverter s_FlagConverter(
		s_FlagEntries, ELEMENT_COUNT( s_FlagEntries ), GoDataComponent::FLAG_NONE );

void ToString( gpstring& out, GoDataComponent::eFlags flags )
{
	out = s_FlagConverter.ToFullString( flags );
}

bool FromString( const char* str, GoDataComponent::eFlags& obj )
{
	return ( s_FlagConverter.FromFullString( str, rcast <DWORD&> ( obj ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// class GoDataTemplate declaration

static kerneltool::Critical s_TemplateReadCritical;
static kerneltool::Critical s_TemplateCritical;

GoDataTemplate :: GoDataTemplate( FastFuelHandle fuel )
	: m_Fuel( fuel )
{
	gpassert( m_Fuel );

	m_BaseDataTemplate = NULL;
	m_RefCount = 0;

#	if !GP_RETAIL
	m_Group     = NULL;
	m_MetaGroup = NULL;
#	endif // !GP_RETAIL
}

GoDataTemplate :: ~GoDataTemplate( void )
{
	// this space intentionally left blank...
}

bool GoDataTemplate :: InitBase( void )
{
	bool success = true;

	// extract specialization
	gpstring special;
	if ( m_Fuel.Get( "specializes", special ) )
	{
		// find it
		m_BaseDataTemplate = gContentDb.FindTemplateByName( special );

		// register with parent (ccast ok)
		if ( m_BaseDataTemplate != NULL )
		{
			ccast <GoDataTemplate*> ( m_BaseDataTemplate )->m_ChildColl.push_back( this );
		}
		else
		{
			gperrorf(( "Specialization of nonexistent template '%s' by template '%s' found at fuel '%s'\n",
					   special.c_str(), GetName(), m_Fuel.GetAddress().c_str() ));
			success = false;
		}
	}

	// all ok
	return ( success );
}

bool GoDataTemplate :: Init( void )
{
	CHECK_PRIMARY_THREAD_ONLY;

	// paranoia check to make sure we aren't initting twice
	gpassert( m_ComponentColl.empty() );
	gpassert( m_RefCount == 0 );

	// add a reference for each existing child
	for ( int i = m_ChildColl.size() ; i != 0 ; --i )
	{
		AddRef( false );
	}

	// no jit-compiling? always have a ref on self too to force compile
#	if !GP_RETAIL
	if ( !gContentDb.IsJitCompile() )
	{
		AddRef( false );
	}
#	endif // !GP_RETAIL

	// success if a ref was added or have no children (and hence not need init yet)
	return ( m_ChildColl.empty() || IsReferenced() );
}

#if !GP_RETAIL

void GoDataTemplate :: PrepMetaGroupDeletion( const GoDataTemplate* oldGodt, const GoDataTemplate* newGodt )
{
	if ( m_MetaGroup == oldGodt )
	{
		m_MetaGroup = newGodt;
	}

	ChildColl::iterator i, ibegin = m_ChildColl.begin(), iend = m_ChildColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		(*i)->PrepMetaGroupDeletion( oldGodt, newGodt );
	}
}

#endif // !GP_RETAIL

const GoDataComponent* GoDataTemplate :: FindComponentByName( const char* name ) const
{
	ComponentColl::const_iterator found = ccast <ThisType*> ( this )->findComponentByName( name );
	return ( (found != m_ComponentColl.end()) ? &*found : NULL );
}

int GoDataTemplate :: AddRef( bool serialize )
{
	// acquire a lock to addref
	kerneltool::Critical::OptionalLock readLocker( serialize ? &s_TemplateReadCritical : NULL );

	// if already initted, just go
	if ( m_RefCount > 0 )
	{
		return ( ++m_RefCount );
	}

	// acquire a template creation lock to compile this thing
	kerneltool::Critical::OptionalLock locker( serialize ? &s_TemplateCritical : NULL );

	// if not initted then should be no components!
	gpassert( m_ComponentColl.empty() );

// Extract header.

	// $$$ use "unused param report" and dump set of extra fuel entries

#	if !GP_RETAIL

	// extract doco
	m_Fuel.Get( "doc",       m_Docs     );
	m_Fuel.Get( "extra_doc", m_ExtraDocs );

	// only bother with group/metagroup if not a root template
	if ( !IsRootTemplate() )
	{
		// set group
		if ( !m_Fuel.GetBool( "is_group", false ) )
		{
			if ( m_BaseDataTemplate->m_Group != NULL )
			{
				m_Group = m_BaseDataTemplate->m_Group;
			}
			else
			{
				m_Group = m_BaseDataTemplate;
			}
		}

		// set metagroup
		if ( !m_Fuel.GetBool( "is_meta_group", false ) )
		{
			if ( m_BaseDataTemplate->m_MetaGroup != NULL )
			{
				m_MetaGroup = m_BaseDataTemplate->m_MetaGroup;
			}
			else
			{
				m_MetaGroup = m_BaseDataTemplate;
			}
		}
	}

	// set category name
	m_CategoryName = m_Fuel.GetString(  "category_name", IsRootTemplate()
									   ? gpstring::EMPTY
									   : m_BaseDataTemplate->m_CategoryName );

#	endif // !GP_RETAIL

// Construct specialization.

	// only specialize when we have a base
	if ( m_BaseDataTemplate != NULL )
	{
		m_ComponentColl.assign_new( m_BaseDataTemplate->m_ComponentColl.begin(), m_BaseDataTemplate->m_ComponentColl.end() );
	}

// Extract specializations.

	// loop through all subcomponents (non-typed)
	{
		FastFuelHandleColl components;
		m_Fuel.ListChildrenTyped( components, "" );

		FastFuelHandleColl::iterator i, ibegin = components.begin(), iend = components.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			// take string
			const gpstring& name = gContentDb.AddString( (*i).GetName() );

			// new component or specialized from base?
			ComponentColl::iterator found = findComponentByName( name );
			if ( found == m_ComponentColl.end() )
			{
				// new component, ask from contentdb
				bool wasInternal = false;
				const GoDataComponent* gocomponent = gContentDb.FindComponentByName( name, &wasInternal );
				if ( gocomponent == NULL )
				{
					// error if not internal or we don't care
					if ( !wasInternal && !gWorldOptions.GetForceRetailContent() )
					{
						gperrorf(( "Template '%s' uses nonexistent component '%s' at fuel '%s'\n",
								   GetName(), name.c_str(), (*i).GetAddress().c_str() ));
					}

					// skip rest
					continue;
				}

				// add it
				found = m_ComponentColl.push_back( *gocomponent );
			}

			// get ptr to component
			GoDataComponent& component = *found;

			// first check for specialized or internal params in schema
			FastFuelHandleColl params;
			(*i).ListChildrenTyped( params, "" );

			FastFuelHandleColl::const_iterator j, jbegin = params.begin(), jend = params.end();
			for ( j = jbegin ; j != jend ; ++j )
			{
				// specialize the schema
				if ( !component.SpecializeSchema( *j ) )
				{
					gperrorf(( "Failure to specialize schema/internal field '%s' for component '%s' in template '%s' at fuel '%s'\n",
							   (*j).GetName(), component.GetName().c_str(), GetName(), (*i).GetAddress().c_str() ));
				}
			}

			// specialize the values - ignore schema overrides btw
			if ( !component.SpecializeData( *i, true ) )
			{
				gperrorf(( "Failure to specialize component '%s' in template '%s' at fuel '%s'\n",
						   component.GetName().c_str(), GetName(), (*i).GetAddress().c_str() ));
			}
		}
	}

// Validate.

	// check to see if all required components are available
#	if ( !GP_RETAIL )
	{
		ComponentColl::const_iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			bool first = true;

			GoDataComponent::StringColl::const_iterator j,
					jbegin = i->GetRequiredColl().begin(),
					jend   = i->GetRequiredColl().end  ();
			for ( j = jbegin ; j != jend ; ++j )
			{
				if ( findComponentByName( *j ) == m_ComponentColl.end() )
				{
					if ( first )
					{
						first = false;
						ReportSys::BeginReport( &gErrorContext );
						gperrorf(( "Required components missing within template '%s': ", GetName() ));
					}
					else
					{
						gperror( ", " );
					}

					gperrorf(( "%s (req by %s)", j->c_str(), i->GetName().c_str() ));
				}
			}

			if ( !first )
			{
				gperror( "\n" );
				ReportSys::EndReport( &gErrorContext );
			}
		}
	}
#	endif // !GP_RETAIL

// Done.

	// check for doco usage
#	if !GP_RETAIL
	{
		if ( !m_Docs.empty() && (*m_Docs.begin() == '[') && (*m_Docs.rbegin() == ']') )
		{
			// use that component's doc instead
			const GoDataComponent* found = FindComponentByName( m_Docs.substr( 1, m_Docs.length() - 2 ) );
			if ( found != NULL )
			{
				m_Docs = found->GetDocs();
			}
		}

		if ( !m_ExtraDocs.empty() && (*m_ExtraDocs.begin() == '[') && (*m_ExtraDocs.rbegin() == ']') )
		{
			// use that component's doc instead
			const GoDataComponent* found = FindComponentByName( m_ExtraDocs.substr( 1, m_ExtraDocs.length() - 2 ) );
			if ( found != NULL )
			{
				m_ExtraDocs = found->GetDocs();
			}
		}
	}
#	endif // !GP_RETAIL

	// all ok, inc ref to indicate success
	return ( ++m_RefCount );
}

int GoDataTemplate :: Release( bool serialize )
{
	// acquire a lock to addref
	kerneltool::Critical::OptionalLock readLocker( serialize ? &s_TemplateReadCritical : NULL );

	// if more refs, just go
	if ( m_RefCount > 1 )
	{
		return ( --m_RefCount );
	}

	// acquire a template creation lock to decompile this thing
	kerneltool::Critical::OptionalLock locker( serialize ? &s_TemplateCritical : NULL );

	// clean out data
	m_ComponentColl.clear();
#	if !GP_RETAIL
	m_Docs.clear();
	m_ExtraDocs.clear();
	m_CategoryName.clear();
#	endif // !GP_RETAIL

	// done
	return ( --m_RefCount );
}

void GoDataTemplate :: PrepFailureDeletion( void )
{
	ChildColl::iterator i, ibegin = m_ChildColl.begin(), iend = m_ChildColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		gpassert( (*i)->m_BaseDataTemplate == this );
		(*i)->m_BaseDataTemplate = m_BaseDataTemplate;

#		if !GP_RETAIL
		(*i)->PrepMetaGroupDeletion( this, m_MetaGroup );
#		endif // !GP_RETAIL
	}
}

GoDataTemplate::ComponentColl::iterator GoDataTemplate :: findComponentByName( const char* name )
{
	ComponentColl::iterator i, ibegin = m_ComponentColl.begin(), iend = m_ComponentColl.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		if ( i->GetName().same_no_case( name ) )
		{
			break;
		}
	}

	return ( i );
}

//////////////////////////////////////////////////////////////////////////////
