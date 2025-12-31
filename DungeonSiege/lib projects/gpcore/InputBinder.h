/*
  ============================================================================
  ----------------------------------------------------------------------------

	File		: 	InputBinder.h

	Author(s)	: 	Bartosz Kijanka, Scott Bilas

	Purpose		:	This file contains the classes used for remapping any
					Windows key, mouse input to any output.

					The main classes are:

					InputBinderDispatch - this is the master class for
					dispatching input.  The input makes it into here from
					Windows, and is then sent to each input binder.

					InputBinder - A black box mapping any Windows input to any
					output.  Output is simply a blind callback.  Multiple
					binders can be registered with the dispatcher.  Messages are
					sent by the dispatcher to the binders, according to a
					priority scheme.  A message/input is passed from one binder
					to the next, unless a binder stops the propegation.

	(C)opyright 2000 Gas Powered Games, Inc.

  ----------------------------------------------------------------------------
  ============================================================================
*/

#pragma once
#ifndef __INPUTBINDER_H
#define __INPUTBINDER_H

//-----------------------------------------------------------------------------

#include "BlindCallback.h"
#include "FuBiDefs.h"
#include "GpColl.h"
#include "Keys.h"

#include <map>

//-----------------------------------------------------------------------------

class FuelHandle;
class FastFuelHandle;


/*=============================================================================

  QualifiedInput

  purpose:	One of several classes used by InputBinder.  QualifiedInput is used
			for wrapping up qualifiers.

  author:	Scott Bilas

-----------------------------------------------------------------------------*/

class QualifiedInput
{
public:
	QualifiedInput( Keys::QUALIFIER q = Keys::Q_NONE )	: m_Qualifiers( q )  {  }

	void SetQualifiers( Keys::QUALIFIER q )				{  m_Qualifiers = q;  }
	void AddQualifiers( Keys::QUALIFIER q )				{  m_Qualifiers |= q;  }

	Keys::QUALIFIER GetQualifiers    ( void ) const		{  return ( m_Qualifiers );  }
	Keys::QUALIFIER GetQualifierFlags( void ) const		{  return ( m_Qualifiers & Keys::Q_QUALIFIER_MASK );  }
	Keys::QUALIFIER GetIgnoreFlags   ( void ) const		{  return ( m_Qualifiers & Keys::Q_IGNORE_MASK    );  }
	Keys::QUALIFIER GetIgnoreMask    ( void ) const		{  return ( (Keys::QUALIFIER)( 0xFFFFFFFF - (GetIgnoreFlags() >> Keys::Q_IGNORE_BITSHIFT) ) ); }

	bool GetControlKey( void ) const					{  return ( (m_Qualifiers & Keys::Q_CONTROL) != 0 );  }
	bool GetAltKey    ( void ) const					{  return ( (m_Qualifiers & Keys::Q_ALT    ) != 0 );  }
	bool GetShiftKey  ( void ) const					{  return ( (m_Qualifiers & Keys::Q_SHIFT  ) != 0 );  }
	bool GetSpecialKey( void ) const					{  return ( (m_Qualifiers & Keys::Q_SPECIAL) != 0 );  }

	bool GetLButton ( void ) const						{  return ( (m_Qualifiers & Keys::Q_LBUTTON ) != 0 );  }
	bool GetMButton ( void ) const						{  return ( (m_Qualifiers & Keys::Q_MBUTTON ) != 0 );  }
	bool GetRButton ( void ) const						{  return ( (m_Qualifiers & Keys::Q_RBUTTON ) != 0 );  }
	bool GetXButton1( void ) const						{  return ( (m_Qualifiers & Keys::Q_XBUTTON1) != 0 );  }
	bool GetXButton2( void ) const						{  return ( (m_Qualifiers & Keys::Q_XBUTTON2) != 0 );  }

	bool GetIgnoreControlKey( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_CONTROL) != 0 );  }
	bool GetIgnoreAltKey    ( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_ALT    ) != 0 );  }
	bool GetIgnoreShiftKey  ( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_SHIFT  ) != 0 );  }
	bool GetIgnoreSpecialKey( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_SPECIAL) != 0 );  }

	bool GetIgnoreLButton ( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_LBUTTON ) != 0 );  }
	bool GetIgnoreMButton ( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_MBUTTON ) != 0 );  }
	bool GetIgnoreRButton ( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_RBUTTON ) != 0 );  }
	bool GetIgnoreXButton1( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_XBUTTON1) != 0 );  }
	bool GetIgnoreXButton2( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_XBUTTON2) != 0 );  }

	bool GetIgnoreQualifiers( void ) const				{  return ( (m_Qualifiers & Keys::Q_IGNORE_QUALIFIERS) != 0 );  }

	void Save( FuelHandle fuel ) const;

private:
	Keys::QUALIFIER m_Qualifiers;
};

gpstring ToString( const QualifiedInput& input );


/*=============================================================================

  KeyInput

  purpose:	One of several classes used by InputBinder.  KeyInput is used for
			wrapping a keystroke.

  author:	Bartosz Kijanka

-----------------------------------------------------------------------------*/

class KeyInput : public QualifiedInput
{
public:
	KeyInput( Keys::KEY key = Keys::KEY_INVALID, Keys::QUALIFIER q = Keys::Q_NONE )
		: QualifiedInput( q ), m_Key( key )  {  }

	Keys::KEY	GetKey ( void ) const				{  return ( m_Key );  }
	bool		IsValid( void ) const				{  return ( m_Key != Keys::KEY_INVALID );  }
	bool		HasKey ( void ) const				{  return ( IsValid() && (m_Key != Keys::KEY_NONE) );  }
	void		Save   ( FuelHandle fuel ) const;

private:
	Keys::KEY m_Key;
};

bool operator <  ( const KeyInput& l, const KeyInput& r );
bool operator == ( const KeyInput& l, const KeyInput& r );
bool operator != ( const KeyInput& l, const KeyInput& r );

gpstring ToString( const KeyInput& input );


/*=============================================================================

  MouseInput

  purpose:	One of several classes used by InputBinder.  This is used for
			wrapping a mouse input.

  author:	Bartosz Kijanka

-----------------------------------------------------------------------------*/

class MouseInput : public QualifiedInput
{
public:
	MouseInput( const gpstring& name = gpstring::EMPTY, Keys::QUALIFIER qualifiers = Keys::Q_NONE );

	const gpstring& GetName ( void ) const			{  return ( m_Name );  }
	int				GetDelta( void ) const			{  return ( m_Value );  }
	void			SetDelta( int delta )			{  m_Value = delta;  }
	bool			IsValid ( void ) const			{  return ( !m_Name.empty() );  }
	void			Save    ( FuelHandle fuel ) const;

	static bool IsValidMouseInput( const char* name );

private:
	gpstring m_Name;
	int      m_Value;
};

bool operator <  ( const MouseInput& l, const MouseInput& r );
bool operator == ( const MouseInput& l, const MouseInput& r );
bool operator != ( const MouseInput& l, const MouseInput& r );

gpstring ToString( const MouseInput& input );


/*=============================================================================

  Input

  purpose:	Wraps any sort of input; mouse or key.

  author:	Bartosz Kijanka

-----------------------------------------------------------------------------*/

class Input
{
public:
	Input( const KeyInput& key, const MouseInput& mouse )	: m_Key( key ), m_Mouse( mouse )  {  }
	Input( const KeyInput& key )							: m_Key( key )  {  }
	Input( const MouseInput& mouse )						: m_Mouse( mouse )  {  }
	Input( void )											{  }

	bool IsValid( void ) const								{  return ( m_Key.IsValid() || m_Mouse.IsValid() );  }
	bool IsKey  ( void ) const								{  return ( m_Key  .IsValid() );  }
	bool IsMouse( void ) const								{  return ( m_Mouse.IsValid() );  }
	bool HasKey ( void ) const								{  return ( m_Key.HasKey()  );  }
	bool HasKeyOrMouse( void ) const						{  return ( m_Key.HasKey() || m_Mouse.IsValid() );  }

	const KeyInput&   GetKey  ( void ) const				{  return ( m_Key   );  }
	KeyInput&         GetKey  ( void )						{  return ( m_Key   );  }
	const MouseInput& GetMouse( void ) const				{  return ( m_Mouse );  }
	MouseInput&       GetMouse( void )						{  return ( m_Mouse );  }

	void Save( FuelHandle fuel ) const;

private:
	KeyInput   m_Key;
	MouseInput m_Mouse;
};

bool operator <  ( const Input& l, const Input& r );
bool operator == ( const Input& l, const Input& r );
bool operator != ( const Input& l, const Input& r );

gpstring ToString( const Input& input );


/*=============================================================================

  InputBinder

  purpose:	The InputBinder binds any sort of input any sort of interface
			(callback). One first publishes an interface to the InputBinder then
			one tells it what sort of input to bind to what output ( to what
			published interface ).

  author:	Bartosz Kijanka

-----------------------------------------------------------------------------*/

class InputBinder
{
public:
	SET_NO_INHERITED( InputBinder );

// Types.

	// $ note: the 'char' interface is special - it is sent whenever the WM_CHAR
	//   message is received, no matter what the character is.

	typedef CBFunctor0wRet <bool>					VoidCb;
	typedef CBFunctor1wRet <int, bool>				IntCb;
	typedef CBFunctor1wRet <const KeyInput&, bool>	KeyCb;
	typedef CBFunctor1wRet <wchar_t, bool>			CharCb;
	typedef CBFunctor1wRet <const Input&, bool>		InputCb;

	// meant for configuration from a standard two-column GUI
	struct BindingEntry
	{
		gpstring  m_Name;		// name of this binding in code
		gpwstring m_ScreenName;	// human-readable name of this binding
		Input     m_Input0;		// left column
		Input     m_Input1;		// right column

		bool operator < ( const BindingEntry& other ) const
		{
			return ( m_ScreenName.compare_no_case( other.m_ScreenName ) < 0 );
		}
	};

	// used for xfer to/from gui configuration interface
	typedef stdx::fast_vector <BindingEntry> BindingColl;

	// for gui organization
	struct BindingEntryGroup
	{
		BindingEntryGroup( gpwstring screenName = L"" )	{  m_ScreenName = screenName;  }

		gpwstring	m_ScreenName;
		BindingColl	m_Bindings;
	};

	typedef std::map< int, BindingEntryGroup > BindingGroupMap;

// Setup.

	// ctor/dtor
	InputBinder( const gpstring& name );
   ~InputBinder( void );

	// activation state
	bool IsActive( void ) const					{  return ( m_Active );  }
	void SetIsActive( bool flag )				{  m_Active = flag;  }

	void SetInputRepeatEnabled( bool flag )		{  m_InputRepeatEnabled = flag; }

	// query
	const gpstring& GetName( void ) const		{  return ( m_Name ); }

	// remove all non-global bindings
	void ClearBindings( void );
	void ClearBindings( const gpstring& name );

	// save bindings to a fuel address
	void SaveBindings( FuelHandle block ) const;

// Processing.

	// returns true if handled
	bool ProcessInput( const Input& input ) const;
	bool ProcessCharInput( wchar_t c ) const;

	// interfaces for client to publish its interface with optional default
	// binding to apply
	void PublishVoidInterface ( const gpstring& interfaceName, VoidCb  cb );
	void PublishVoidInterface ( const gpstring& interfaceName, VoidCb  cb, const Input& defInput );
	void PublishIntInterface  ( const gpstring& interfaceName, IntCb   cb );
	void PublishIntInterface  ( const gpstring& interfaceName, IntCb   cb, const Input& defInput );
	void PublishKeyInterface  ( const gpstring& interfaceName, KeyCb   cb );
	void PublishKeyInterface  ( const gpstring& interfaceName, KeyCb   cb, const Input& defInput );
	void PublishInputInterface( const gpstring& interfaceName, InputCb cb );
	void PublishInputInterface( const gpstring& interfaceName, InputCb cb, const Input& defInput );

	// publish auto-binding all-key/character interfaces
	void PublishAllKeysInterface ( KeyCb cb );
	void PublishAllCharsInterface( CharCb cb );

	// bind arbitrary input to published interface
	void BindInputToPublishedInterface( const Input& input, const char* interfaceName );
	void BindInputToPublishedInterface( const char* inputText, const char* qualifiersText, const char* interfaceName );
	void BindInputsToPublishedInterfaces( FuelHandle block );
	void BindInputsToPublishedInterfaces( FastFuelHandle block );
	void BindDefaultInputsToPublishedInterfaces( void );

	// figure out where our default bindings are
	FastFuelHandle FindDefaultBindings( void ) const;

	// lookup
	static InputBinder* FindInputBinder( const char* name );

// Binding.

	// call this to fill out a collection for gui configuration
	void GetBindingsGroupForGui( BindingColl& bindings, FastFuelHandle bindingsFuel ) const;
	void GetBindingsForGui( BindingGroupMap& bindings ) const;
	void SetBindingsForGui( const BindingGroupMap& bindings );
	bool GetInputBindingForGui( const char * name, Input & input );


// Debug.

#	if !GP_RETAIL

	static void DumpBinders( ReportSys::ContextRef ctx );
	FUBI_EXPORT static void DumpBinders( void )		{  DumpBinders( NULL );  }

	static void SetAllowDevOnly( bool set = true )	{  ms_AllowDevOnly = set;  }
	static bool GetAllowDevOnly( void )				{  return ( ms_AllowDevOnly );  }

#	endif // !GP_RETAIL

private:
	class Binding;

	bool PrivateProcessInput( const Input& input, const Input& callInput ) const;
	void PublishInterface( const gpstring& interfaceName, const Binding& binding, const Input* defInput = NULL );

// Types.

	typedef std::multimap <Input, Binding*> BindingDb;					// input to trigger a binding mapped to binding

	class Binding
	{
		enum eType
		{
			TYPE_UNKNOWN,
			TYPE_VOID,
			TYPE_INT,
			TYPE_KEY,
			TYPE_INPUT,
		};

	public:
		typedef stdx::fast_vector <BindingDb::iterator> BindingColl;

		Binding( VoidCb  cb ) : m_Cb( cb ), m_Type( TYPE_VOID   )	{  m_DevOnly = false;  }
		Binding( IntCb   cb ) : m_Type( TYPE_INT     )				{  m_DevOnly = false;  ::memcpy( &m_Cb, &cb, sizeof( m_Cb ) );  }
		Binding( KeyCb   cb ) : m_Type( TYPE_KEY     )				{  m_DevOnly = false;  ::memcpy( &m_Cb, &cb, sizeof( m_Cb ) );  }
		Binding( InputCb cb ) : m_Type( TYPE_INPUT   )				{  m_DevOnly = false;  ::memcpy( &m_Cb, &cb, sizeof( m_Cb ) );  }
		Binding( void )       : m_Type( TYPE_UNKNOWN )				{  m_DevOnly = false;  }

		bool IsValid        ( void ) const							{  return ( m_Type != TYPE_UNKNOWN );  }
		bool IsDevOnly      ( void ) const							{  return ( m_DevOnly );  }
		bool Callback       ( const Input& input );
		void AddInputBinding( BindingDb::iterator binding )			{  m_BindingColl.push_back( binding );  }
		void ClearBindings  ( void )								{  m_BindingColl.clear();  }

		void            SetName( const gpstring& name )				{  m_Name = name;  }
		const gpstring& GetName( void ) const						{  return ( m_Name );  }

		const BindingColl& GetBindingColl( void ) const				{  return ( m_BindingColl );  }

		const char* GetValidTypeString( void ) const
		{
			switch ( m_Type )
			{
				case ( TYPE_VOID  ):  return ( "void"  );
				case ( TYPE_INT   ):  return ( "int"   );
				case ( TYPE_KEY   ):  return ( "key"   );
				case ( TYPE_INPUT ):  return ( "input" );
			}
			return ( NULL );
		}

		void Commit( void )
		{
			// if we've already bound defaults and there are none, then this
			// must be considered dev-only, and unavailable in retail.
			m_DevOnly = m_BindingColl.empty();
		}

	private:
		gpstring    m_Name;
		eType       m_Type;
		bool        m_DevOnly;
		VoidCb      m_Cb;
		BindingColl m_BindingColl;
	};

// Collections.

	typedef std::map <gpstring, Binding, istring_less> PublishedDb;		// name of published binding mapped to callback
	typedef std::map <gpstring, ThisType*, istring_less> BinderDb;		// all binders

// Specification.

	gpstring    m_Name;							// name of this binder
	bool        m_Active;						// true if this binder can receive
	bool		m_InputRepeatEnabled;			// process repeating key input ?
	PublishedDb m_PublishedDb;					// here we keep all registered callbacks
	BindingDb   m_BindingDb;					// input to client callbacks bindings are defined here
	KeyCb       m_AllKeysInterface;				// interface for all key messages
	CharCb      m_AllCharsInterface;			// interface for all character messages

	static BinderDb ms_BinderDb;				// all binders
	static bool     ms_AllowDevOnly;			// are we permitting dev-only bindings?

	SET_NO_COPYING( InputBinder );
};


//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
