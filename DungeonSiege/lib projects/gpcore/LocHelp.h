//////////////////////////////////////////////////////////////////////////////
//
// File     :  LocHelp.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes and methods to aid in localized coding.
//             Messing with DBCS, Unicode, etc...
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __LOCHELP_H
#define __LOCHELP_H

//////////////////////////////////////////////////////////////////////////////

#include "GpCore.h"
#include <map>

class DllBinder;

//////////////////////////////////////////////////////////////////////////////
// class LocMgr declaration

class LocMgr : public Singleton <LocMgr>
{
public:
	SET_NO_INHERITED( LocMgr );

// Setup.

	// ctor/dtor
	LocMgr( void );
   ~LocMgr( void );

	// setup
	bool Init( void );

	// translation map construction
	bool LoadExeTranslations( void );
	bool LoadGasTranslations( void );

// API.

	// processing
	bool ConvertMessageAW( HWND hwnd, UINT message, WPARAM& wparam, LPARAM& lparam );

	// get localized parameters
	UINT     GetCodePage    ( void ) const		{  return ( m_CodePage     );  }
	DWORD    GetCharSet     ( void ) const		{  return ( m_CharSet      );  }
	LANGID   GetLanguage    ( void ) const		{  return ( m_Language     );  }
	gpstring GetLanguageName( void ) const		{  return ( m_LanguageName );  }
	LCID     GetLocale      ( void ) const		{  return ( m_Locale       );  }

	// helpers
	bool IsAnsi( void ) const					{  return ( m_CodePage == 1252 );  }

	// ui query
	bool TranslateUiText   ( gpwstring& out, const char* intrface, const char* window ) const;
	bool TranslateUiToolTip( gpwstring& out, const char* intrface, const char* window ) const;

	// special helpers
	static LANGID GetDefaultLanguage( void );
	static LCID   GetDefaultLocale  ( void );

private:
	struct UiEntry
	{
		gpwstring m_Text;
		gpwstring m_ToolTip;
	};

	typedef std::map <gpstring, UiEntry, string_less> UiWindowDb;
	typedef std::map <gpstring, UiWindowDb, string_less> UiInterfaceDb;

	const UiEntry* FindUiEntry( const char* intrface, const char* window ) const;

	// spec
	DllBinder* m_Dll;				// LANGUAGE.DLL
	UINT       m_CodePage;			// app code page to use for MBCS where needed
	DWORD      m_CharSet;			// character set to use for GDI fonts
	LANGID     m_Language;			// language we're using
	gpstring   m_LanguageName;		// text version of our language
	LCID       m_Locale;			// locale we're in

	// databases
	UiInterfaceDb m_UiInterfaceDb;

	SET_NO_COPYING( LocMgr );
};

#define gLocMgr LocMgr::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class LocStringExtractor declaration

class LocStringExtractor
{
public:
	SET_NO_INHERITED( LocStringExtractor );

// Types.

	struct Entry
	{
		LANGID    m_Language;
		gpstring  m_From;
		gpwstring m_To;

		Entry( void ) : m_Language( 0 )  {  }
	};

	#pragma pack ( push, 1 )

	struct BinString
	{
		WORD m_ID;
		char m_String[ 1 ];
	};

	#pragma pack ( pop )

	typedef std::map <UINT16, Entry> StringColl;		// map ID to strings
	typedef std::map <LANGID, StringColl> LangColl;		// map language ID to map of strings

// Setup.

	// ctor/dtor
	LocStringExtractor( void )  {  }
   ~LocStringExtractor( void )  {  }

	// setup
	bool Init( HMODULE module = NULL, bool bUseFrom = true );			// $ defaults to LocMgr's dll, then to gpstring's instances

// API.

	// new entries
	void UpdateEntry( UINT16 id, const gpstring& from );
	void UpdateEntry( LANGID langid, UINT16 id, const gpwstring& to );

	// access
	const LangColl& GetStrings( void ) const  {  return ( m_Strings );  }

private:
	LangColl m_Strings;

	SET_NO_COPYING( LocStringExtractor );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __LOCHELP_H

//////////////////////////////////////////////////////////////////////////////
