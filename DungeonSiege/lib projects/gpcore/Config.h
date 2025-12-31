//////////////////////////////////////////////////////////////////////////////
//
// File     :  Config.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes for configuring the game via command-line,
//             INI file, and registry.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __CONFIG_H
#define __CONFIG_H

//////////////////////////////////////////////////////////////////////////////

#include "FileSysDefs.h"
#include "FuBiTraits.h"
#include "GpColl.h"

#include <list>
#include <map>

//////////////////////////////////////////////////////////////////////////////
// class IniFile declaration

class IniFile : public FuBi::XferHelper <IniFile>
{
public:
	SET_INHERITED( IniFile, FuBi::XferHelper <IniFile> );

// Setup.

	// ctor/dtor
	IniFile( void )				{  Reset();  }
	virtual ~IniFile( void )	{  }

	// clear out everything
	void Reset( void );

	// anything changed?
	bool IsDirty( void ) const	{  return ( m_Dirty );  }

	// persistence
	bool LoadFile( const char* iniFileName = NULL, gpstring* outFileName = NULL );
	bool Load( FileSys::Reader& reader );
	bool SaveFile( const char* iniFileName = NULL, gpstring* outFileName = NULL );
	bool Save( FileSys::Writer& writer ) const;

	// helper
	static void ResolveFileName( const char*& iniFileName, gpstring* outFileName );

// Data setup.

	// add new section
	void AddSection( const gpstring& name )							{  m_Dirty = true;  m_CurSection = PrivateAddSection( name );  }

	// add new entry to current section
	void AddComment( const gpstring& comment )					 	{  m_Dirty = true;  m_CurSection->AddComment( comment );  }
	void AddKey    ( const gpstring& key, const gpstring& value )	{  m_Dirty = true;  m_CurSection->AddKey( key, value );  }
	void AddValue  ( const gpstring& value )						{  m_Dirty = true;  m_CurSection->AddValue( value );  }

	// set current section (create if not exist)
	void SetSection( const gpstring& name );
	const gpstring& GetSectionName( void ) const					{  return ( m_CurSection->m_Name );  }

	// set a key in the current section
	bool SetKeyValue( const gpstring& key, const gpstring& value )	{  m_Dirty = true;  m_CurSection->SetKeyValue( key, value );  return ( true );  }

	// helpers
	template <typename T>
	void Set( const gpstring& key, const T& obj )					{  SetKeyValue( key, ToString( obj ) );  }

// Data query.

	bool ReadKeyValue( const char* key, gpstring& out ) const		{  return ( AutoSection( this, key ).ReadString( out ) );  }
	bool SetKeyValue ( const char* key, const gpstring& out )		{  m_Dirty = true;  AutoSection( this, key ).SetString ( out );  return ( true );  }

	bool IsEmpty( void ) const;

private:
	struct Section
	{
		struct Key
		{
			gpstring m_Key;
			gpstring m_Value;
			bool     m_IsComment;

			bool Save( FileSys::Writer& writer ) const;

			Key& SetComment ( const gpstring& str );
			Key& SetKeyValue( const gpstring& key, const gpstring& value );
			Key& SetValue   ( const gpstring& value );
		};

		typedef std::list <Key> KeyColl;

		gpstring m_Name;
		KeyColl  m_Keys;

		bool Save( FileSys::Writer& writer ) const;

		void AddComment( const gpstring& str )
						 {  m_Keys.push_back( Key().SetComment( str ) );  }
		void AddKey    ( const gpstring& key, const gpstring& value )
						 {  m_Keys.push_back( Key().SetKeyValue( key, value ) );  }
		void AddValue  ( const gpstring& value )
						 {  m_Keys.push_back( Key().SetValue( value ) );  }

		bool ReadKeyValue( const char* key, gpstring& out ) const;
		void SetKeyValue( const gpstring& key, const gpstring& value );
	};

	typedef std::list <Section> SectionColl;
	typedef SectionColl::iterator SectionIter;

	SectionIter PrivateAddSection( const gpstring& name );
	SectionIter PrivateSetSection( const gpstring& name );

	class AutoSection
	{
	public:
		AutoSection( const IniFile* ini, const char* key )			{  Init( ccast <IniFile*> ( ini ), key );  }
		AutoSection( IniFile* ini, const char* key )				{  Init( ini, key );  }

		bool ReadString( gpstring& out ) const						{  return ( m_Section->ReadKeyValue( m_KeyName, out ) );  }
		void SetString( const gpstring& out )						{  m_Section->SetKeyValue ( m_KeyName, out );  }

	private:
		void Init( IniFile* ini, const char* key );

		SectionIter m_Section;
		gpstring    m_KeyName;
	};

	friend AutoSection;

	SectionColl  m_Sections;
	SectionIter  m_CurSection;
	mutable bool m_Dirty;

	SET_NO_COPYING( IniFile );
};

//////////////////////////////////////////////////////////////////////////////
// class CommandLine declaration

class CommandLine : public FuBi::XferHelper <CommandLine>
{
public:
	SET_INHERITED( CommandLine, FuBi::XferHelper <CommandLine> );

// Setup.

	// ctor/dtor
	CommandLine( const char* commandLine = NULL )	{  Init( commandLine );  }
	virtual ~CommandLine( void )					{  }

	// clear out everything
	void Reset( void )								{  m_Keys.clear();  }

	// persistence
	void Init( const char* commandLine = NULL );
	void ResolveMacros( const IniFile& iniFile );

// Data query.

	// get a key
	bool ReadKeyValue( const char* key, gpstring& out ) const;

private:
	struct Key
	{
		gpstring m_Key;
		gpstring m_Value;

		Key& SetKeyValue( const gpstring& key, const gpstring& value );
		Key& SetValue   ( const gpstring& value );
		Key& SetKey     ( const gpstring& key );
	};

	typedef std::list <Key> KeyColl;

	KeyColl m_Keys;

	SET_NO_COPYING( CommandLine );
};

//////////////////////////////////////////////////////////////////////////////
// class EnvironmentVars declaration

class EnvironmentVars : public FuBi::XferHelper <EnvironmentVars>
{
public:
	SET_INHERITED( EnvironmentVars, FuBi::XferHelper <EnvironmentVars> );

// Setup.

	// ctor/dtor
	EnvironmentVars( const char* prefix = "" )		: m_Prefix( prefix )  {  }
	virtual ~EnvironmentVars( void )				{  }

// Data query.

	// get a key
	bool ReadKeyValue( const char* key, gpstring& out ) const;

private:
	gpstring m_Prefix;

	SET_NO_COPYING( EnvironmentVars );
};

//////////////////////////////////////////////////////////////////////////////
// class Registry declaration

class Registry : public FuBi::XferHelper <Registry>
{
public:
	SET_INHERITED( Registry, FuBi::XferHelper <Registry> );
	typedef stdx::fast_vector <gpstring> Strings;

// Setup.

	// ctor/dtor
	Registry( const char* companyName, const char* productName, bool localUser );
	virtual ~Registry( void );

	const gpstring& GetRootPath( void ) const					{  return ( m_Root );  }

// Data setup.

	void SetSubKey( const gpstring& subKey );

	bool SetBinary ( const char* key, const_mem_ptr out )		{  return ( AutoKey( this, key ).SetBinary ( out ) );  }
	bool SetDword  ( const char* key, DWORD out )				{  return ( AutoKey( this, key ).SetDword  ( out ) );  }
	bool SetQword  ( const char* key, QWORD out )				{  return ( AutoKey( this, key ).SetQword  ( out ) );  }
	bool SetString ( const char* key, const gpstring& out )		{  return ( AutoKey( this, key ).SetString ( out ) );  }
	bool SetString ( const char* key, const char* out )			{  return ( AutoKey( this, key ).SetString ( out ) );  }
	bool SetWString( const char* key, const gpwstring& out )	{  return ( AutoKey( this, key ).SetWString( out ) );  }
	bool SetWString( const char* key, const wchar_t* out )		{  return ( AutoKey( this, key ).SetWString( out ) );  }
	bool SetStrings( const char* key, const Strings& out )		{  return ( AutoKey( this, key ).SetStrings( out ) );  }

// Data query.

	bool ReadBinary ( const char* key, mem_ptr out ) const		{  return ( AutoKey( this, key ).ReadBinary ( out ) );  }
	bool ReadDword  ( const char* key, DWORD& out ) const		{  return ( AutoKey( this, key ).ReadDword  ( out ) );  }
	bool ReadQword  ( const char* key, QWORD& out ) const		{  return ( AutoKey( this, key ).ReadQword  ( out ) );  }
	bool ReadString ( const char* key, gpstring& out ) const	{  return ( AutoKey( this, key ).ReadString ( out ) );  }
	bool ReadWString( const char* key, gpwstring& out ) const	{  return ( AutoKey( this, key ).ReadWString( out ) );  }
	bool ReadStrings( const char* key, Strings& out ) const		{  return ( AutoKey( this, key ).ReadStrings( out ) );  }

// XferHelper "overrides".

	bool  GetBool ( const char* key, bool def = false ) const;
	int   GetInt  ( const char* key, int def = 0 ) const;
	float GetFloat( const char* key, float def = 0 ) const;

	bool SetBool ( const char* key, bool  value )				{  return ( SetDword( key, (DWORD)value ) );  }
	bool SetInt  ( const char* key, int   value )				{  return ( SetDword( key, force_cast <DWORD> ( value ) ) );  }
	bool SetFloat( const char* key, float value )				{  return ( SetDword( key, force_cast <DWORD> ( value ) ) );  }

	// $$$ check traits TRAIT_INTEGER etc.
	// $$$ put these set/get into another helper class that config can derive from
	// $$$ what about binary??

	bool ReadKeyValue( const char* key, gpstring& out ) const;
	bool SetKeyValue ( const char* key, const gpstring& out )	{  return ( SetString( key, out ) );  }

private:
	class AutoKey
	{
	public:
		AutoKey( const Registry* reg, const char* key,
				 REGSAM access = KEY_READ )						{  Init( reg, key, access );  }
		AutoKey( Registry* reg, const char* key,
				 REGSAM access = KEY_WRITE )					{  Init( reg, key, access );  }
	   ~AutoKey( void );

		bool SetBinary ( const_mem_ptr out );
		bool SetDword  ( DWORD out );
		bool SetQword  ( QWORD out );
		bool SetString ( const gpstring& out );
		bool SetWString( const gpwstring& out );
		bool SetStrings( const Strings& out );

		bool ReadBinary ( mem_ptr out ) const;
		bool ReadDword  ( DWORD& out ) const;
		bool ReadQword  ( QWORD& out ) const;
		bool ReadString ( gpstring& out ) const;
		bool ReadWString( gpwstring& out ) const;
		bool ReadStrings( Strings& out ) const;

	private:
		void Init      ( const Registry* reg, const char* key, REGSAM access = KEY_READ );
		bool SetValue  ( DWORD type, const_mem_ptr out );
		bool QueryValue( DWORD type, mem_ptr out ) const;
		bool QueryValue( mem_ptr out ) const;
		bool QueryInfo ( DWORD& type, DWORD& size ) const;

		HKEY m_Key;
		gpstring m_KeyName;

		SET_NO_COPYING( AutoKey );
	};

	friend AutoKey;

	gpstring m_Root;
	gpstring m_SubKey;
	bool     m_LocalUser;

	SET_NO_COPYING( Registry );
};

//////////////////////////////////////////////////////////////////////////////
// struct ConfigInit declaration

	// note: sample paths returned from the shell folder query stuff:
	//
	// win2k:
	//
	// CSIDL_PERSONAL = C:\Documents and Settings\Scott Bilas\My Documents
	//
	// win98:
	//
	// CSIDL_PERSONAL = C:\WINDOWS\My Documents (i think...)

struct ConfigInit
{
	gpstring m_CompanyName;				// used in registry
	gpstring m_IniFileName;				// filename for ini file
	gpstring m_RegistryName;			// keyname for registry
	int      m_DocsFolder;				// docs folder to use - defaults to "my documents", set to -1 for none
	gpstring m_DocsSubDir;				// dir under docs folder to use
	gpstring m_EnvVarPrefix;			// prefix used for gpg env vars
	gpstring m_LogSubDir;				// subdir for log_path (optional)
	bool     m_DisableDevMode;			// disable dev mode by default

	ConfigInit( void );

	void SetProductName( gpstring productName );
	void SetMyDocuments( void );
};

//////////////////////////////////////////////////////////////////////////////
// class Config declaration

class Config : public FuBi::XferHelper <Config>, public Singleton <Config>
{
public:
	SET_INHERITED( Config, FuBi::XferHelper <Config> );

	// ctor/dtor
	Config( const ConfigInit& init );
	virtual ~Config( void );

	// setup paths
	bool Init( bool errorOnFail = true );

	// lazy writes
	void FlushChanges( bool forceWrite = false );

	// dir creation
	bool CreateDocsDir( void );

	// setup
	void SetOutRegistryIni   ( void )							{  m_OutRegistry = NULL;  }
	void SetOutRegistryLocal ( void )							{  m_OutRegistry = &m_LocalRegistry;  }
	void SetOutRegistryGlobal( void )							{  m_OutRegistry = &m_GlobalRegistry;  }

	// $$$ setsubkey...
	// $$$ env var, cl should ignore path prefixes and convert spaces to underscores

// Query.

	bool            IsDevMode          ( void ) const			{  return ( m_DevMode );  }
	const gpstring& GetIniFileName     ( void ) const			{  return ( m_IniFileName );  }
	gpstring        GetRegistryRootPath( void ) const			{  return ( m_GlobalRegistry.GetRootPath() );  }

// XferHelper "overrides".

	bool ReadKeyValue( const char* key, gpstring& out ) const;
	bool SetKeyValue ( const char* key, const gpstring& out );
	bool HasKey      ( const char* key ) const					{  gpstring temp;  return ( ReadKeyValue( key, temp ) );  }

	// type specific
FEX	bool            FUBI_RENAME( GetBool   )( const char* key, bool def ) const			{  return ( GetBool  ( key, def ) );  }
FEX	bool            FUBI_RENAME( GetBool   )( const char* key ) const					{  return ( GetBool  ( key      ) );  }
FEX	int             FUBI_RENAME( GetInt    )( const char* key, int def ) const			{  return ( GetInt   ( key, def ) );  }
FEX	int             FUBI_RENAME( GetInt    )( const char* key ) const					{  return ( GetInt   ( key      ) );  }
FEX	float           FUBI_RENAME( GetFloat  )( const char* key, float def ) const		{  return ( GetFloat ( key, def ) );  }
FEX	float           FUBI_RENAME( GetFloat  )( const char* key ) const					{  return ( GetFloat ( key      ) );  }
FEX	const gpstring& FUBI_RENAME( GetString )( const char* key, const char* def ) const;
FEX	const gpstring& FUBI_RENAME( GetString )( const char* key ) const					{  return ( FUBI_RENAME( GetString )( key, "" ) );  }

// Path var work

	enum ePathVarMode
	{
		PVM_NONE					=       0,									// do no special processing when adding pathvar
		PVM_APPEND					=  1 << 0,									// append new paths to existing pathvars
		PVM_APPEND_DUPS_OK			=  1 << 1,									// appended paths are ok to be dups of existing ones
		PVM_USE_CONFIG				=  1 << 2,									// check self for variable already existing in GetString() first and use that instead if exists, otherwise use passed-in var if non-null/empty.
		PVM_REQUIRED				=  1 << 3,									// fail if the path does not exist or is not creatable
		PVM_WRITABLE				= (1 << 4) | PVM_REQUIRED,					// fail if the path is not writable
		PVM_CREATE					= (1 << 5) | PVM_REQUIRED | PVM_WRITABLE,	// create path if does not exist
		PVM_ABSOLUTE_PATH			=  1 << 6,									// fail if the path is not absolute
		PVM_ERROR_ON_FAIL			=  1 << 7,									// print error if anything fails
		PVM_FATAL_ON_FAIL			=  1 << 8,									// fatal if anything fails
	};

	// resolution of paths
	bool      ResolvePathVars( gpstring & out, const char   * in, ePathVarMode mode = (ePathVarMode)(PVM_USE_CONFIG | PVM_ERROR_ON_FAIL) );
	bool      ResolvePathVars( gpwstring& out, const wchar_t* in, ePathVarMode mode = (ePathVarMode)(PVM_USE_CONFIG | PVM_ERROR_ON_FAIL) );
	gpstring  ResolvePathVars( const char   * str, ePathVarMode mode = (ePathVarMode)(PVM_USE_CONFIG | PVM_ERROR_ON_FAIL) );
	gpwstring ResolvePathVars( const wchar_t* str, ePathVarMode mode = (ePathVarMode)(PVM_USE_CONFIG | PVM_ERROR_ON_FAIL) );

	// direct variable access
	bool      SetPathVar     ( const char* var, const char   * value, ePathVarMode mode = PVM_NONE );
	bool      SetPathVar     ( const char* var, const wchar_t* value, ePathVarMode mode = PVM_NONE );
	bool      AddPathVar     ( const char* var, const char   * value, ePathVarMode mode = PVM_NONE );
	bool      AddPathVar     ( const char* var, const wchar_t* value, ePathVarMode mode = PVM_NONE );
	bool      IsPathVarSet   ( const char* var ) const;
	bool      DeletePathVar  ( const char* var );

#	if !GP_RETAIL
	void DumpPathVars( ReportSys::ContextRef ctx );
FEX	void DumpPathVars( void )  {  DumpPathVars( NULL );  }
#	endif // !GP_RETAIL

private:

	bool setPathVar( const char* var, const wchar_t* value, ePathVarMode mode );

	typedef std::multimap <gpstring, gpwstring, istring_less> StringStringDb;

	// state
	Registry*      m_OutRegistry;
	gpstring       m_IniFileName;
	bool           m_DevMode;
	ConfigInit     m_Init;
	StringStringDb m_PathVarDb;

	// config objects - these are in override order (highest first)
	CommandLine     m_CommandLine;
	IniFile         m_IniFile;
	Registry        m_LocalRegistry;
	Registry        m_GlobalRegistry;
	EnvironmentVars m_EnvironmentVars;

	FUBI_SINGLETON_CLASS( Config, "Configuration class for command line/INI/registry settings." );
	SET_NO_COPYING( Config );
};

MAKE_ENUM_BIT_OPERATORS( Config::ePathVarMode );

#define gConfig Config::GetSingleton()

//////////////////////////////////////////////////////////////////////////////

#endif  // __CONFIG_H

//////////////////////////////////////////////////////////////////////////////
