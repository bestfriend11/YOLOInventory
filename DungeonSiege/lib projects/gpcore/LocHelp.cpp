//////////////////////////////////////////////////////////////////////////////
//
// File     :  LocHelp.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "LocHelp.h"

#include "DllBinder.h"
#include "Fuel.h"
#include "ReportSys.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// enum helpers

namespace LocMgrCb
{
	static BOOL CALLBACK EnumResLangProc( HMODULE /*module*/, LPCTSTR /*type*/, LPCTSTR /*name*/, LANGID lang, LONG_PTR param )
	{
		*((LANGID*)param) = lang;
		return ( FALSE );
	}

	static BOOL CALLBACK EnumResNameProc( HMODULE module, LPCTSTR type, LPTSTR name, LONG_PTR param )
	{
		return ( ::EnumResourceLanguages( module, type, name, EnumResLangProc, param ) );
	}

	static BOOL CALLBACK EnumResTypeProc( HMODULE module, LPTSTR type, LONG_PTR param )
	{
		return ( ::EnumResourceNames( module, type, EnumResNameProc, param ) );
	}
}

//////////////////////////////////////////////////////////////////////////////
// class LocMgr implementation

LocMgr :: LocMgr( void )
{
	m_Dll      = NULL;
	m_Language = ::GetUserDefaultLangID();
	m_CodePage = ::GetACP();
	m_CharSet  = DEFAULT_CHARSET;
	m_Locale   = ::GetUserDefaultLCID();
}

LocMgr :: ~LocMgr( void )
{
	delete ( m_Dll );
}

bool LocMgr :: Init( void )
{
	gpassert( m_Dll == NULL );

// Gather info.

	// attempt load
	std::auto_ptr <DllBinder> dllBinder( new DllBinder );
	if ( !dllBinder->Load( "LANGUAGE.DLL", false, true ) && !dllBinder->Load( "..\\LANGUAGE.DLL", false, true ) )
	{
		return ( false );
	}

	// get language from the first resource we find
	::EnumResourceTypes( *dllBinder, LocMgrCb::EnumResTypeProc, (LONG_PTR)&m_Language );

	// update national settings
	if ( m_Language != 0 )
	{
		// take the locale
		m_Locale = MAKELCID( m_Language, SORT_DEFAULT );

		// figure out our code page
		char buffer[ 10 ];
		if ( ::GetLocaleInfo( m_Locale, LOCALE_IDEFAULTANSICODEPAGE, buffer, ELEMENT_COUNT( buffer ) ) != 0 )
		{
			// take it
			stringtool::Get( buffer, m_CodePage );
		}
		else
		{
			// just go with default
			m_CodePage = ::GetACP();
		}
	}
	else
	{
		// adjust language if independent
		m_Language = ::GetUserDefaultLangID();
	}

	// get the text name of this language
	char buffer[ 50 ];
	::VerLanguageName( m_Language, buffer, ELEMENT_COUNT( buffer ) );
	m_LanguageName = buffer;

// Configure system.

	// set up strings for them
	gpstring::SetCodePage         ( m_CodePage );
	gpstring::SetLocalizedInstance( *dllBinder );
	gpstring::SetLanguage         ( m_Language );

	// autodetect char set
	switch ( m_CodePage )
	{
		case ( 1252 ):  m_CharSet = ANSI_CHARSET;         break;
		case ( 932  ):  m_CharSet = SHIFTJIS_CHARSET;     break;
		case ( 949  ):  m_CharSet = HANGUL_CHARSET;       break;
		case ( 950  ):  m_CharSet = CHINESEBIG5_CHARSET;  break;
		default      :  m_CharSet = DEFAULT_CHARSET;
	}

	// setup c runtime codepage
	::setlocale( LC_CTYPE, gpstringf( ".%d", m_CodePage ) );

	// take the binder
	m_Dll = dllBinder.release();

	// success
	return ( true );
}

bool LocMgr :: LoadExeTranslations( void )
{
	bool success = false;

	if ( m_Dll != NULL )
	{
		// load resources
		LocStringExtractor extractor;
		if ( extractor.Init( *m_Dll ) )
		{
			using LocStringExtractor::LangColl;
			using LocStringExtractor::StringColl;

			LangColl::const_iterator found = extractor.GetStrings().find( m_Language );
			if ( found != extractor.GetStrings().end() )
			{
				StringColl::const_iterator
						i,
						ibegin = found->second.begin(),
						iend   = found->second.end  ();
				for ( i = ibegin ; i != iend ; ++i )
				{
					gReportSysMgr.GetTranslateMap().insert( std::make_pair( i->second.m_From, ::ToAnsi( i->second.m_To ) ) );
				}
			}

			success = true;
		}
	}

	return ( success );
}

bool LocMgr :: LoadGasTranslations( void )
{
	bool success = false;

	// access fuel translations
	FuelHandle lang( "language" );
	if ( lang )
	{
		// build language
		gpstring type;
		type.assignf( "0x%04X", m_Language );

		// get text
		FuelHandleList blocks;
		if ( lang->ListChildBlocks( 1, type, "text", blocks ) )
		{
			gpstring from;
			gpwstring to;

			// iterate over languages
			FuelHandleList::iterator i, ibegin = blocks.begin(), iend = blocks.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// iterate over translations
				FuelHandleList entries = (*i)->ListChildBlocks();
				FuelHandleList::iterator j, jbegin = entries.begin(), jend = entries.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					if ( (*j)->Get( "from", from ) && (*j)->Get( "to", to ) )
					{
						gReportSysMgr.GetTranslateMap().insert( std::make_pair( from, ::ToAnsi( to ) ) );
					}
				}
			}
		}

		// get ui
		if ( lang->ListChildBlocks( 1, type, "ui", blocks ) )
		{
			// iterate over languages
			FuelHandleList::iterator i, ibegin = blocks.begin(), iend = blocks.end();
			for ( i = ibegin ; i != iend ; ++i )
			{
				// iterate over interfaces
				FuelHandleList interfaces = (*i)->ListChildBlocks();
				FuelHandleList::iterator j, jbegin = interfaces.begin(), jend = interfaces.end();
				for ( j = jbegin ; j != jend ; ++j )
				{
					UiWindowDb& db = m_UiInterfaceDb[ (*j)->GetName() ];

					// iterate over windows
					FuelHandleList windows = (*j)->ListChildBlocks();
					FuelHandleList::iterator k, kbegin = windows.begin(), kend = windows.end();
					for ( k = kbegin ; k != kend ; ++k )
					{
						UiEntry& entry = db[ (*k)->GetName() ];
						(*k)->Get( "text", entry.m_Text );
						(*k)->Get( "tooltip", entry.m_ToolTip );
					}
				}
			}
		}

		// free memory
		lang.Unload();
	}

	return ( success );
}

// $ this code adapted from MSJ April 1999 "Design a Single Unicode App that
//   Runs on Both Windows 98 and Windows 2000" by F. Avery Bishop

bool LocMgr :: ConvertMessageAW( HWND /*hwnd*/, UINT message, WPARAM& wparam, LPARAM& /*lparam*/ )
{
	static char s_AnsiChar[ 3 ] = "";

	bool success = TRUE;
	switch ( message )
	{
		case ( WM_CHAR ):
		{
			// we have to go through all this malarky because DBCS characters
			// arrive one byte at a time. in this sample application, most of
			// this code is never used because DBCS chars are handled by
			// WM_IME_CHAR below. you can comment out that case (and the
			// corresponding one in WinProc) to test this code.

			if ( s_AnsiChar[ 0 ] == '\0' )
			{
				// no lead byte already waiting for trail byte
				s_AnsiChar[0] = (char)wparam;

				// check for lead byte
				if( ::IsDBCSLeadByteEx( m_CodePage, (BYTE)wparam ) )
				{
					// save it and wait for trail byte
					return ( false );
				}

				// not a DBCS character, convert to unicode
				::MultiByteToWideChar( m_CodePage, 0, s_AnsiChar, 1, (LPWSTR)&wparam, 1 );
			}
			else
			{
				// have lead byte, wparam should contain the trail byte
				s_AnsiChar[ 1 ] = (char)wparam;

				// convert both bytes into one Unicode character
				::MultiByteToWideChar( m_CodePage, 0, s_AnsiChar, 2, (LPWSTR)&wparam, 1 );
			}

			// reset to non-waiting state
			s_AnsiChar[ 0 ] = '\0';
		}
		break;

		case ( WM_IME_CHAR ):
		{
			// the next 3 lines replace all but one line in the WM_CHAR case
			// above. this is why it's best to use WM_IME_CHAR rather than
			// WM_CHAR to get DBCS chars in ANSI mode.

			s_AnsiChar[ 1 ] = LOBYTE( (WORD)wparam );
			s_AnsiChar[ 0 ] = HIBYTE( (WORD)wparam );

			if ( ::MultiByteToWideChar( m_CodePage, 0, s_AnsiChar, 2, (LPWSTR)&wparam, 1 ) == 0 )
			{
				success = false;
			}
		}
	}

	return ( success );
}

bool LocMgr :: TranslateUiText( gpwstring& out, const char* intrface, const char* window ) const
{
	bool success = false;

	const UiEntry* entry = FindUiEntry( intrface, window );
	if ( (entry != NULL) && !entry->m_Text.empty() )
	{
		out = entry->m_Text;
		success = true;
	}

	return ( success );
}

bool LocMgr :: TranslateUiToolTip( gpwstring& out, const char* intrface, const char* window ) const
{
	bool success = false;

	const UiEntry* entry = FindUiEntry( intrface, window );
	if ( (entry != NULL) && !entry->m_ToolTip.empty() )
	{
		out = entry->m_ToolTip;
		success = true;
	}

	return ( success );
}

LANGID LocMgr :: GetDefaultLanguage( void )
{
	// default to English if no LocMgr. this is required because so many
	// things assume that a missing LocMgr == English. fixes #13347.
	return ( DoesSingletonExist() ? gLocMgr.GetLanguage() : (LANGID)MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
}

LCID LocMgr :: GetDefaultLocale( void )
{
	return ( DoesSingletonExist() ? gLocMgr.GetLocale() : (LCID)MAKELCID( GetDefaultLanguage(), SORT_DEFAULT ) );
}

const LocMgr::UiEntry* LocMgr :: FindUiEntry( const char* intrface, const char* window ) const
{
	const UiEntry* entry = NULL;

	UiInterfaceDb::const_iterator ifound = m_UiInterfaceDb.find( intrface );
	if ( ifound != m_UiInterfaceDb.end() )
	{
		UiWindowDb::const_iterator wfound = ifound->second.find( window );
		if ( wfound != ifound->second.end() )
		{
			entry = &wfound->second;
		}
	}

	return ( entry );
}

//////////////////////////////////////////////////////////////////////////////
// enum helpers

namespace LocStringExtractorCb
{
	static BOOL CALLBACK EnumResLangProc( HMODULE module, LPCTSTR /*type*/, LPCTSTR name, LANGID lang, LONG_PTR param )
	{
		LocStringExtractor* extractor = (LocStringExtractor*)param;

		gpwstring str;
		if ( LoadStringW( str, module, (UINT)name, lang ) && !str.empty() )
		{
			extractor->UpdateEntry( lang, (UINT16)name, str );
			return ( true );
		}
		else
		{
			return ( false );
		}
	}

	static BOOL CALLBACK EnumResNameProc( HMODULE module, LPCTSTR type, LPTSTR name, LONG_PTR param )
	{
		return ( ::EnumResourceLanguages( module, type, name, EnumResLangHelperProc, EnumResLangHelperStruct( EnumResLangProc, param ) ) );
	}
}

//////////////////////////////////////////////////////////////////////////////
// class LocStringExtractor implementation

bool LocStringExtractor :: Init( HMODULE module, bool bUseFrom )
{
	bool success = false;

	// get proper dll
	if ( module == NULL )
	{
		module = gpstring::GetLocalizedInstance();
		if ( module == NULL )
		{
			module = gpstring::GetSystemInstance();
			if ( module == NULL )
			{
				module = ::GetModuleHandle( NULL );
			}
		}
	}

	// fill foreign from the string table
	if ( ::EnumResourceNames( module, RT_STRING, LocStringExtractorCb::EnumResNameProc, (LONG_PTR)this ) )
	{
		if ( !bUseFrom )
		{
			return ( true );
		}

		// fill english (preserved through locstudio) from the rcdata
		HRSRC rsrc = ::FindResource( module, MAKEINTRESOURCE( 1 ), RT_RCDATA );
		if ( rsrc != NULL )
		{
			// load it $ NOT a "real" HGLOBAL (see docs)
			HGLOBAL res = ::LoadResource( module, rsrc );
			if ( res != NULL )
			{
				// lock for copy $ no need to free this up later
				const BinString* data = rcast <const BinString*> ( ::LockResource( res ) );
				if ( (data != NULL) && (data->m_ID != 0) )
				{
					gpstring str;

					// now iterate
					do
					{
						str.assign( data->m_String );
						UpdateEntry( data->m_ID, str );
						data = (const BinString*)(((const BYTE*)(data + 1)) + str.length() );
					}
					while ( data->m_ID != 0 );

					success = true;
				}
			}
		}
	}

	return ( success );
}

void LocStringExtractor :: UpdateEntry( UINT16 id, const gpstring& from )
{
	LangColl::iterator i, ibegin = m_Strings.begin(), iend = m_Strings.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		i->second[ id ].m_From = from;
	}
}

void LocStringExtractor :: UpdateEntry( LANGID langid, UINT16 id, const gpwstring& to )
{
	m_Strings[ langid ][ id ].m_To = to;
}

//////////////////////////////////////////////////////////////////////////////
