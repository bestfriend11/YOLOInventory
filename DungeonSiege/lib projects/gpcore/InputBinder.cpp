/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	InputBinder.cpp

	Author(s)	: 	Bartosz Kijanka, Scott Bilas

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/

#include "Precomp_GpCore.h"
#include "InputBinder.h"

#include "FuBiSchema.h"
#include "Fuel.h"
#include "GpProfiler.h"
#include "ReportSys.h"
#include "StdHelp.h"
#include "StringTool.h"

//-----------------------------------------------------------------------------
// class QualifiedInput implementation

void QualifiedInput :: Save( FuelHandle fuel ) const
{
	if ( m_Qualifiers != Keys::Q_NONE )
	{
		fuel->Set( "qualifiers", Keys::ToFullString( m_Qualifiers ) );
	}
}

gpstring ToString( const QualifiedInput& input )
{
	return ( Keys::ToFullString( input.GetQualifiers() ) );
}

//-----------------------------------------------------------------------------
// class KeyInput implementation

void KeyInput :: Save( FuelHandle fuel ) const
{
	if ( IsValid() )
	{
		fuel->Set( "input", Keys::ToString( m_Key ) );
	}
	QualifiedInput::Save( fuel );
}

bool operator < ( const KeyInput& l, const KeyInput& r )
{
	if ( l.GetKey() < r.GetKey() )
	{
		return ( true );
	}
	if ( l.GetKey() != r.GetKey() )
	{
		return ( false );
	}

	Keys::QUALIFIER mask = l.GetIgnoreMask() & r.GetIgnoreMask();
	return ( (l.GetQualifierFlags() & mask) < (r.GetQualifierFlags() & mask) );
}

bool operator == ( const KeyInput& l, const KeyInput& r )
{
	if ( l.GetKey() != r.GetKey() )
	{
		return ( false );
	}
	Keys::QUALIFIER mask = l.GetIgnoreMask() & r.GetIgnoreMask();
	return ( (l.GetQualifierFlags() & mask) == (r.GetQualifierFlags() & mask) );
}

bool operator != ( const KeyInput& l, const KeyInput& r )
{
	return ( !(l == r) );
}

gpstring ToString( const KeyInput& input )
{
	gpstring str;

	if ( input.IsValid() )
	{
		if ( input.GetQualifiers() != Keys::Q_NONE )
		{
			str = ToString( scast <const QualifiedInput&> ( input ) );
			str += " + ";
		}

		str += Keys::ToString( input.GetKey() );
	}

	return ( str );
}

//-----------------------------------------------------------------------------
// class MouseInput implementation

static const char* s_MouseInputNames[] =
{
	"dx",
	"dy",
	"dz",
	"left_up",
	"left_down",
	"left_dbl_click",
	"middle_up",
	"middle_down",
	"middle_dbl_click",
	"right_up",
	"right_down",
	"right_dbl_click",
	"x1_up",
	"x1_down",
	"x1_dbl_click",
	"x2_up",
	"x2_down",
	"x2_dbl_click",
	"wheel_up",
	"wheel_down",
};

MouseInput :: MouseInput( const gpstring& name, Keys::QUALIFIER qualifiers )
	: QualifiedInput( qualifiers )
	, m_Name ( name )
	, m_Value( 0 )
{
	gpassert( name.empty() || IsValidMouseInput( name ) );
}

void MouseInput :: Save( FuelHandle fuel ) const
{
	if ( IsValid() )
	{
		fuel->Set( "input", m_Name );
	}
	QualifiedInput::Save( fuel );
}

bool MouseInput :: IsValidMouseInput( const char* name )
{
	// sort the array if it hasn't been sorted yet - do this once
	static bool s_Sorted = false;
	if( !s_Sorted )
	{
		s_Sorted = true;
		std::sort( s_MouseInputNames, ARRAY_END( s_MouseInputNames ), istring_less() );
	}

	// search the array
	bool success = false;
	const char** found = stdx::binary_search( s_MouseInputNames, ARRAY_END( s_MouseInputNames ),
											  name, istring_less() );
	if ( found != ARRAY_END( s_MouseInputNames ) )
	{
		success = true;
	}

	// done
	return ( success );
}

bool operator < ( const MouseInput& l, const MouseInput& r )
{
	if ( l.GetName().compare_no_case( r.GetName() ) < 0 )
	{
		return ( true );
	}
	if ( !l.GetName().same_no_case( r.GetName() ) )
	{
		return ( false );
	}

	Keys::QUALIFIER mask = l.GetIgnoreMask() & r.GetIgnoreMask();
	return ( (l.GetQualifierFlags() & mask) < (r.GetQualifierFlags() & mask) );
}

bool operator == ( const MouseInput& l, const MouseInput& r )
{
	if ( !l.GetName().same_no_case( r.GetName() ) )
	{
		return ( false );
	}
	Keys::QUALIFIER mask = l.GetIgnoreMask() & r.GetIgnoreMask();
	return ( (l.GetQualifierFlags() & mask) == (r.GetQualifierFlags() & mask) );
}

bool operator != ( const MouseInput& l, const MouseInput& r )
{
	return ( !(l == r) );
}

gpstring ToString( const MouseInput& input )
{
	gpstring str;

	if ( input.IsValid() )
	{
		if ( input.GetQualifiers() != Keys::Q_NONE )
		{
			str = ToString( scast <const QualifiedInput&> ( input ) );
			str += " + ";
		}

		str += input.GetName();
	}

	return ( str );
}

//-----------------------------------------------------------------------------
// class Input implementation

void Input :: Save( FuelHandle fuel ) const
{
	if ( IsKey() )
	{
		m_Key.Save( fuel );
	}
	else
	{
		m_Mouse.Save( fuel );
	}
}

bool operator < ( const Input& l, const Input& r )
{
	if ( l.IsMouse() && !r.IsMouse() )
	{
		return ( false );
	}
	else if ( !l.IsMouse() && r.IsMouse() )
	{
		return ( true );
	}
	else if ( l.IsMouse() && r.IsMouse() )
	{
		return ( l.GetMouse() < r.GetMouse() );
	}
	else
	{
		return ( l.GetKey() < r.GetKey() );
	}
}


bool operator == ( const Input& l, const Input& r )
{
	return ( (l.GetKey() == r.GetKey()) && (l.GetMouse() == r.GetMouse()) );
}

bool operator != ( const Input& l, const Input& r )
{
	return ( !(l == r) );
}

gpstring ToString( const Input& input )
{
	gpstring str = ToString( input.GetKey() );
	gpstring mouse = ToString( input.GetMouse() );

	if ( !mouse.empty() )
	{
		if ( !str.empty() )
		{
			str += " + ";
		}
		str += mouse;
	}

	return ( str );
}

//-----------------------------------------------------------------------------
// class InputBinder implementation

InputBinder::BinderDb InputBinder::ms_BinderDb;
bool InputBinder::ms_AllowDevOnly = !GP_RETAIL;

InputBinder :: InputBinder( const gpstring& name )
	: m_Name( name )
{
	// each binder should have a name!
	gpassert( !name.empty() );

	m_Active				= false;
	m_InputRepeatEnabled	= true;

	gpassert( ms_BinderDb.find( name ) == ms_BinderDb.end() );
	ms_BinderDb[ name ] = this;
}

InputBinder :: ~InputBinder( void )
{
	BinderDb::iterator found = ms_BinderDb.find( GetName() );
	gpassert( found != ms_BinderDb.end() );
	gpassert( found->second == this );
	ms_BinderDb.erase( found );
}

void InputBinder :: ClearBindings( void )
{
	PublishedDb::iterator i, ibegin = m_PublishedDb.begin(), iend = m_PublishedDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->second.ClearBindings();
	}

	m_BindingDb.clear();
}

void InputBinder :: ClearBindings( const gpstring& name )
{
	PublishedDb::iterator found = m_PublishedDb.find( name );
	if ( found != m_PublishedDb.end() )
	{
		Binding::BindingColl::const_iterator i,
									   ibegin = found->second.GetBindingColl().begin(),
									   iend   = found->second.GetBindingColl().end  ();
		for ( i = ibegin ; i != iend ; ++i )
		{
			m_BindingDb.erase( *i );
		}

		found->second.ClearBindings();
	}
}

void InputBinder :: SaveBindings( FuelHandle block ) const
{
	// store bindings
	BindingDb::const_iterator i, ibegin = m_BindingDb.begin(), iend = m_BindingDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->first.Save( block->CreateChildBlock( i->second->GetName() ) );
	}
}

bool InputBinder :: ProcessInput( const Input& input ) const
{
	GPPROFILERSAMPLE( "InputBinder::ProcessInput", SP_MISC );
	
	if( ( input.GetKey().GetQualifiers() & Keys::Q_REPEAT ) && !m_InputRepeatEnabled )
	{
		return( true );
	}

	// check exact binding
	if ( PrivateProcessInput( input, input ) )
	{
		return ( true );
	}

	// special key cases
	if ( input.IsKey() )
	{
		// build local input event
		Input asyncInput( input );
		Keys::QUALIFIER qualifiers( asyncInput.GetKey().GetQualifiers() );

		// reset qualifiers, and go for both down and up at once (for async)
		asyncInput.GetKey().SetQualifiers( qualifiers | Keys::Q_KEY_DOWN | Keys::Q_KEY_UP );

		// check for an "async" state
		if ( PrivateProcessInput( asyncInput, input ) )
		{
			return ( true );
		}

		// call "all" cb if not handled
		if ( m_AllKeysInterface )
		{
			return ( m_AllKeysInterface( input.GetKey() ) );
		}
	}

	// nobody got it i guess
	return ( false );
}

bool InputBinder :: ProcessCharInput( wchar_t c ) const
{
	GPPROFILERSAMPLE( "InputBinder::ProcessCharInput()", SP_MISC );

	bool handled = false;

	// callback if set (filter out nonprintables and special key)
	if ( m_AllCharsInterface && !::iswcntrl( c ) && (c != Keys::ToChar( Keys::KEY_SPECIAL)) )
	{
		handled = m_AllCharsInterface( c );
	}

	return ( handled );
}

void InputBinder :: PublishVoidInterface( const gpstring& interfaceName, VoidCb cb )
{
	PublishInterface( interfaceName, Binding( cb ) );
}

void InputBinder :: PublishVoidInterface( const gpstring& interfaceName, VoidCb cb, const Input& defInput )
{
	PublishInterface( interfaceName, Binding( cb ), &defInput );
}

void InputBinder :: PublishIntInterface( const gpstring& interfaceName, IntCb cb )
{
	PublishInterface( interfaceName, Binding( cb ) );
}

void InputBinder :: PublishIntInterface( const gpstring& interfaceName, IntCb cb, const Input& defInput )
{
	PublishInterface( interfaceName, Binding( cb ), &defInput );
}

void InputBinder :: PublishKeyInterface( const gpstring& interfaceName, KeyCb cb )
{
	PublishInterface( interfaceName, Binding( cb ) );
}

void InputBinder :: PublishKeyInterface( const gpstring& interfaceName, KeyCb cb, const Input& defInput )
{
	PublishInterface( interfaceName, Binding( cb ), &defInput );
}

void InputBinder :: PublishInputInterface( const gpstring& interfaceName, InputCb cb )
{
	PublishInterface( interfaceName, Binding( cb ) );
}

void InputBinder :: PublishInputInterface( const gpstring& interfaceName, InputCb cb, const Input& defInput )
{
	PublishInterface( interfaceName, Binding( cb ), &defInput );
}

void InputBinder :: PublishAllKeysInterface( KeyCb cb )
{
	m_AllKeysInterface = cb;
}

void InputBinder :: PublishAllCharsInterface( CharCb cb )
{
	m_AllCharsInterface = cb;
}

void InputBinder :: BindInputToPublishedInterface( const Input& input, const char* interfaceName )
{
	gpassert( input.IsValid() );

	// find callback by name
	PublishedDb::iterator found = m_PublishedDb.find( interfaceName );
	if ( found != m_PublishedDb.end() )
	{
		// may not want to permit this in retail mode for dev-only bindings
		bool permit = true;
		if ( found->second.IsDevOnly() && !ms_AllowDevOnly )
		{
			permit = false;
		}

		// only do it if it's permitted!
		if ( permit )
		{
			BindingDb::iterator rc = m_BindingDb.insert( std::make_pair( input, &found->second ) );
			found->second.AddInputBinding( rc );
		}
	}
	else
	{
		gpwarningf(( "InputBinder: attempted to bind to interface '%s', "
					 "which is unknown to binder '%s'.\n",
					 interfaceName, GetName().c_str() ));
	}
}

void InputBinder :: BindInputToPublishedInterface( const char* inputText, const char* qualifiersText, const char* interfaceName )
{
	Keys::QUALIFIER qualifiers;
	Keys::KEY key;

	if ( Keys::FromFullString( qualifiersText, qualifiers ) )
	{
		if ( MouseInput::IsValidMouseInput( inputText ) )
		{
			// build mouse input and bind
			BindInputToPublishedInterface( MouseInput( inputText, qualifiers ), interfaceName );
		}
		else if ( Keys::FromString( inputText, key ) )
		{
			// special: ignore mouse buttons by default if no other ignore
			//          flags set and we haven't explicitly said "none".
			if (   !(qualifiers & Keys::Q_IGNORE_MASK)
				&& (qualifiersText[ ::strcspn( qualifiersText, "none" ) ] == '\0') )
			{
				qualifiers |= Keys::Q_IGNORE_BUTTONS;

				// special: ignore all qualifiers by default if key_up
				if ( (qualifiers & Keys::Q_QUALIFIER_MASK) == Keys::Q_KEY_UP )
				{
					qualifiers |= Keys::Q_IGNORE_QUALIFIERS;
				}
			}

			// special: if it's not key_up, then it's key_down
			if ( !(qualifiers & Keys::Q_KEY_UP) )
			{
				// must be a key down then
				qualifiers |= Keys::Q_KEY_DOWN;
			}

			// build key input and bind
			BindInputToPublishedInterface( KeyInput( key, qualifiers ), interfaceName );
		}
		else
		{
			gpwarningf(( "InputBinder: unable to convert input "
						 "('%s') for interface '%s' from text.\n",
						 inputText, interfaceName ));
		}
	}
	else
	{
		gpwarningf(( "InputBinder: unable to convert qualifiers "
					 "('%s') for interface '%s' from text.\n",
					 qualifiersText, interfaceName ));
	}
}

void InputBinder :: BindInputsToPublishedInterfaces( FuelHandle block )
{
	gpstring inputName;

	FuelHandleList bindings = block->ListChildBlocks( 2 );
	for ( FuelHandleList::iterator i = bindings.begin() ; i != bindings.end() ; ++i )
	{
		if ( (*i)->Get( "input", inputName ) )
		{
			BindInputToPublishedInterface( inputName, (*i)->GetString( "qualifiers" ), (*i)->GetName() );
		}
	}
}

void InputBinder :: BindInputsToPublishedInterfaces( FastFuelHandle block )
{
	gpstring inputName;

	FastFuelHandleColl bindings;
	block.ListChildren( bindings, 2 );
	for ( FastFuelHandleColl::iterator i = bindings.begin() ; i != bindings.end() ; ++i )
	{
		if ( !i->IsEmpty() && i->Get( "input", inputName ) )
		{
			BindInputToPublishedInterface( inputName, i->GetString( "qualifiers" ), i->GetName() );
		}
	}
}

void InputBinder :: BindDefaultInputsToPublishedInterfaces( void )
{
	FastFuelHandle bindings = FindDefaultBindings();
	if ( bindings )
	{
		BindInputsToPublishedInterfaces( bindings );

		// now go and mark anything that doesn't have a default binding as dev-only
		PublishedDb::iterator i, ibegin = m_PublishedDb.begin(), iend = m_PublishedDb.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			i->second.Commit();
		}
	}
}

FastFuelHandle InputBinder :: FindDefaultBindings( void ) const
{
	return ( FastFuelHandle( "config:input_bindings:" + GetName() ) );
}

InputBinder* InputBinder :: FindInputBinder( const char* name )
{
	BinderDb::iterator found = ms_BinderDb.find( name );
	return ( (found == ms_BinderDb.end()) ? NULL : found->second );
}

void InputBinder :: GetBindingsGroupForGui( BindingColl& bindings, FastFuelHandle bindingsFuel ) const
{
	FastFuelHandleColl groupBindings;
	bindingsFuel.ListChildren( groupBindings );
	for ( FastFuelHandleColl::iterator i = groupBindings.begin() ; i != groupBindings.end() ; ++i )
	{
		if ( !same_no_case( "group", i->GetType() ) )
		{
			PublishedDb::const_iterator findBinding = m_PublishedDb.find( i->GetName() );
			if ( findBinding == m_PublishedDb.end() )
			{
				continue;
			}

			bool foundBinding = false;
			BindingColl::iterator iBinding = bindings.begin();
			for ( ; iBinding != bindings.end(); ++iBinding )
			{
				if ( iBinding->m_Name.same_no_case( i->GetName() ) )
				{
					foundBinding = true;
					break;
				}
			}
			if ( foundBinding )
			{
				continue;
			}

			BindingEntry entry;
			entry.m_Name = i->GetName();
			entry.m_ScreenName = ::ToUnicode( i->GetName() );

			// dev only binding?
			bool devOnly = i->GetBool( "dev_only", false );

			// check screen name
			gpstring screenName = i->GetString( "screen_name" );
			if ( !screenName.empty() )
			{
				ReportSys::Translate( entry.m_ScreenName, screenName );
			}

			// now only do this if we're not dev-only or if that's ok
			if ( !devOnly || ms_AllowDevOnly )
			{
				// set its inputs
				const Binding::BindingColl& currentBindings = findBinding->second.GetBindingColl();
				if ( currentBindings.size() > 0 )
				{
					entry.m_Input0 = currentBindings[ 0 ]->first;

					if ( currentBindings.size() > 1 )
					{
						entry.m_Input1 = currentBindings[ 1 ]->first;
					}
				}

				// add to collection
				bindings.push_back( entry );
			}
		}
	}

	// sort by screen name by default
	bindings.sort();
}

void InputBinder :: GetBindingsForGui( BindingGroupMap& bindings ) const
{
	FastFuelHandle bindingsFuel = FindDefaultBindings();

	bindings.clear();
	bindings.insert( BindingGroupMap::value_type( -1, BindingEntryGroup() ) );
	GetBindingsGroupForGui( bindings[ -1 ].m_Bindings, bindingsFuel );

	FastFuelHandleColl groups;
	bindingsFuel.ListChildrenTyped( groups, "group" );
	for ( FastFuelHandleColl::iterator i = groups.begin() ; i != groups.end() ; ++i )
	{
		int groupId = i->GetInt( "id" );

		BindingGroupMap::iterator findGroup = bindings.find( groupId );
		if ( findGroup == bindings.end() )
		{
			BindingEntryGroup group;

			gpstring screenName = i->GetString( "screen_name" );
			if ( !screenName.empty() )
			{
				ReportSys::Translate( group.m_ScreenName, screenName );
			}

			findGroup = bindings.insert( BindingGroupMap::value_type( groupId, group ) ).first;
		}

		GetBindingsGroupForGui( findGroup->second.m_Bindings, *i );
	}
}

void InputBinder :: SetBindingsForGui( const BindingGroupMap& bindings )
{
	BindingGroupMap::const_iterator iGroup = bindings.begin();
	for ( ; iGroup != bindings.end() ; ++iGroup )
	{
		BindingColl::const_iterator i = iGroup->second.m_Bindings.begin();
		for ( ; i != iGroup->second.m_Bindings.end(); ++i )
		{
			ClearBindings( i->m_Name );

			if ( i->m_Input0.IsValid() )
			{
				BindInputToPublishedInterface( i->m_Input0, i->m_Name );
			}

			if ( i->m_Input1.IsValid() )
			{
				BindInputToPublishedInterface( i->m_Input1, i->m_Name );
			}
		}
	}
}

bool InputBinder :: GetInputBindingForGui( const char * name, Input & input )
{
	BindingDb::iterator i;
	for ( i = m_BindingDb.begin(); i != m_BindingDb.end(); ++i )
	{
		if ( (*i).second->GetName().same_no_case( name ) )
		{
			input = (*i).first;
			return true;
		}
	}

	return false;
}

#if !GP_RETAIL

void InputBinder :: DumpBinders( ReportSys::ContextRef ctx )
{
	ReportSys::AutoReport autoReport( ctx );

	BinderDb::const_iterator i, ibegin = ms_BinderDb.begin(), iend = ms_BinderDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		const InputBinder* binder = i->second;

		ctx->OutputF( "Binder '%s':\n\n", binder->GetName().c_str() );
		ReportSys::AutoIndent autoIndent( ctx );
		{
			ctx->OutputF( "Active             : %s\n", binder->IsActive() ? "true" : "false" );
			ctx->OutputF( "All keys interface : %s\n", (binder->m_AllKeysInterface != NULL) ? "bound" : "unbound" );
			ctx->OutputF( "All chars interface: %s\n", (binder->m_AllCharsInterface != NULL) ? "bound" : "unbound" );

			if ( !binder->m_PublishedDb.empty() )
			{
				ctx->OutputEol();
				ctx->Output( "Published bindings:\n\n" );

				// setup output
				ReportSys::TextTableSink sink;
				ReportSys::LocalContext loc( &sink, false );
				loc.SetSchema( new ReportSys::Schema( 3, "Name", "Type", "BoundTo" ), true );

				// dump elements
				PublishedDb::const_iterator j,
											jbegin = binder->m_PublishedDb.begin(),
											jend   = binder->m_PublishedDb.end  ();
				for ( j = jbegin ; j != jend ; ++j )
				{
					// for each bound input
					Binding::BindingColl::const_iterator k,
														 kbegin = j->second.GetBindingColl().begin(),
														 kend   = j->second.GetBindingColl().end();
					bool first = true;
					for ( k = kbegin ; first || (k != kend) ; ++k, first = false )
					{
						// out the name
						ReportSys::AutoReport autoReport( loc );
						loc.OutputField( j->first );

						// out the type
						const char* type = j->second.GetValidTypeString();
						loc.OutputField( (type != NULL) ? type : "" );

						// use the binding if any
						gpstring binding;
						if ( !first || (kbegin != kend) )
						{
							loc.OutputField( ToString( (*k)->first ) );
						}
						else
						{
							loc.OutputField( "<none>" );
							break;
						}
					}
				}

				// out table
				sink.OutputReport( ctx );
				ctx->OutputEol();
			}
			else
			{
				ctx->Output( "Published bindings : none!\n\n" );
			}
		}
	}
}

#endif // !GP_RETAIL

bool InputBinder :: PrivateProcessInput( const Input& input, const Input& callInput ) const
{
	bool handled = false;

	// lookup input
	BindingDb::const_iterator lowerBound = m_BindingDb.lower_bound( input );
	BindingDb::const_iterator upperBound = m_BindingDb.upper_bound( input );

	// call all appropriate callbacks
	for ( BindingDb::const_iterator i = lowerBound ; i != upperBound ; ++i )
	{
		if ( i->first == input )
		{
			if ( i->second->Callback( callInput ) )
			{
				handled = true;
				break;
			}
		}
	}

	// done
	return ( handled );
}

void InputBinder :: PublishInterface( const gpstring& interfaceName, const Binding& binding, const Input* defInput )
{
	gpassertf( m_PublishedDb.find( interfaceName ) == m_PublishedDb.end(),
			 ( "Attempted to insert duplicate interface '%s' in binder '%s'.",
			 interfaceName.c_str(), GetName().c_str() ));

	// insert it
	std::pair <PublishedDb::iterator, bool> rc = m_PublishedDb.insert(
			std::make_pair( interfaceName, binding ) );
	rc.first->second.SetName( interfaceName );

	// optionally insert default binding
	if ( defInput != NULL )
	{
		BindInputToPublishedInterface( *defInput, interfaceName );
	}
}

//-----------------------------------------------------------------------------
// class InputBinder::Binding implementation

bool InputBinder::Binding :: Callback( const Input& input )
{
	GPPROFILERSAMPLE( "InputBinder::Binding::Callback()", SP_MISC );

	bool handled = false;

	if ( m_Type == TYPE_VOID )
	{
		handled = m_Cb();
	}
	else if ( m_Type == TYPE_INT )
	{
		handled = (*rcast <IntCb*> ( &m_Cb ))( input.GetMouse().GetDelta() );
	}
	else if ( m_Type == TYPE_KEY )
	{
		handled = (*rcast <KeyCb*> ( &m_Cb ))( input.GetKey() );
	}
	else if ( m_Type == TYPE_INPUT )
	{
		handled = (*rcast <InputCb*> ( &m_Cb ))( input );
	}
	else
	{
		gpassert( 0 );		// should never get here
	}

	return ( handled );
}

//-----------------------------------------------------------------------------
