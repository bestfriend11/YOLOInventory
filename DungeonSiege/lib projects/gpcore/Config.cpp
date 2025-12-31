//////////////////////////////////////////////////////////////////////////////
//
// File     :  Config.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "Config.h"

#include "DllBinder.h"
#include "FileSysUtils.h"
#include "FileSysXfer.h"
#include "FuBiSchemaImpl.h"
#include "KernelTool.h"
#include "ReportSys.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// class ShFolderDll declaration and implementation

#include <shlobj.h>

class ShFolderDll : public DllBinder
{
public:
	SET_INHERITED( ShFolderDll, DllBinder );

	ShFolderDll( void )
		: Inherited( "SHFOLDER.DLL" )
	{
		AddProc( &SHGetFolderPathA, "SHGetFolderPathA" );
		AddProc( &SHGetFolderPathW, "SHGetFolderPathW", true );

		Load( false );
	}

	DllProc5 <HRESULT,						// SHGetFolderPathA(
			HWND,								// hwndOwner
			int,								// nFolder
			HANDLE,								// hToken
			DWORD,								// dwFlags
			LPSTR>								// pszPath
		SHGetFolderPathA;

	DllProc5 <HRESULT,						// SHGetFolderPathW(
			HWND,								// hwndOwner
			int,								// nFolder
			HANDLE,								// hToken
			DWORD,								// dwFlags
			LPWSTR>								// pszPath
		SHGetFolderPathW;

	SET_NO_COPYING( ShFolderDll );
};

//////////////////////////////////////////////////////////////////////////////
// class IniFile implementation

void IniFile :: Reset( void )
{
	m_Sections.clear();
	m_CurSection = m_Sections.insert( m_Sections.end(), Section() );
	m_Dirty = false;
}

bool IniFile :: LoadFile( const char* iniFileName, gpstring* outFileName )
{
	gpstring localName;
	if ( outFileName == NULL )
	{
		outFileName = &localName;
	}
	ResolveFileName( iniFileName, outFileName );

	Reset();

	FileSys::File file;
	if ( !file.OpenExisting( iniFileName ) )
	{
		return ( false );
	}

	FileSys::FileReader reader( file );
	return ( Load( reader ) );
}

bool IniFile :: Load( FileSys::Reader& reader )
{
	// clear out any existing data in case we are reloading
	Reset();

	// read lines in until there are no more or we fail
	for ( gpstring line ; reader.ReadLine( line ) ; line.clear() )
	{
		const char* p = line.c_str();

		// ignore leading spaces
		while ( ::isspace( *p ) )
		{
			++p;
		}

		// blank line?
		if ( *p == '\0' )
		{
			continue;
		}

		// is it a new section?
		if ( *p == '[' )
		{
			// skip [
			++p;

			// find end
			const char* end = ::strchr( p, ']' );
			if ( end != NULL )
			{
				// normally add, but set if empty
				gpstring name( p, end );
				if ( name.empty() )
				{
					SetSection( name );
				}
				else
				{
					AddSection( name );
				}
			}
			else
			{
				gperrorf(( "Error in INI section header: '%s'\n",
						   stringtool::EscapeCharactersToTokens( line ).c_str() ));
			}
			continue;
		}

		// is it a comment?
		if ( (*p == ';') || ((p[ 0 ] == '/') && (p[ 1 ] == '/')) )
		{
			AddComment( line );
			continue;
		}

		// is it a key = value combination? note that lines starting with an
		//  equal can be used to treat the whole line (after the '=') as a key
		const char* end = ::strchr( p, '=' );
		if ( end != NULL )
		{
			if ( end == p )
			{
				// skip leading '=' and let fallthrough catch rest of line as a
				//  keyless value
				++p;
			}
			else
			{
				AddKey( gpstring( p, end ), gpstring( end + 1, line.end() ) );
				continue;
			}
		}

		// treat the whole line as a key
		AddValue( line );
		continue;
	}

	// curSection must point to first section after loading
	m_CurSection = m_Sections.begin();

	// not dirty
	m_Dirty = false;

	// done
	return ( true );
}

bool IniFile :: SaveFile( const char* iniFileName, gpstring* outFileName )
{
	gpstring localName;
	if ( outFileName == NULL )
	{
		outFileName = &localName;
	}
	ResolveFileName( iniFileName, outFileName );

	if ( IsEmpty() )
	{
		return ( true );
	}

	FileSys::File file;
	if ( !file.CreateAlways( iniFileName ) )
	{
		return ( false );
	}

	FileSys::FileWriter writer( file );
	return ( Save( writer ) );
}

bool IniFile :: Save( FileSys::Writer& writer ) const
{
	if ( IsEmpty() )
	{
		return ( true );
	}

	SectionColl::const_iterator i, begin = m_Sections.begin(), end = m_Sections.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->Save( writer ) )  return ( false );
	}

	m_Dirty = false;

	return ( true );
}

void IniFile :: ResolveFileName( const char*& iniFileName, gpstring* outFileName )
{
	gpassert( outFileName != NULL );

	if ( iniFileName && *iniFileName )
	{
		*outFileName = iniFileName;

		if ( !FileSys::HasExtension( *outFileName ) )
		{
			*outFileName += ".ini";
		}

		if ( !FileSys::HasPathInfo( *outFileName ) )
		{
			outFileName->insert( 0, FileSys::GetLocalWritableDir() );
		}
	}
	else
	{
		*outFileName = FileSys::GetModuleFileName();
		stringtool::ReplaceExtension( *outFileName, ".ini" );
	}

	iniFileName = outFileName->c_str();
}

void IniFile :: SetSection( const gpstring& name )
{
	if ( !m_CurSection->m_Name.same_no_case( name ) )
	{
		m_CurSection = PrivateSetSection( name );
	}
}

bool IniFile :: IsEmpty( void ) const
{
	SectionColl::const_iterator i, begin = m_Sections.begin(), end = m_Sections.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->m_Keys.empty() )
		{
			return ( false );
		}
	}

	return ( true );
}

IniFile::SectionIter IniFile :: PrivateAddSection( const gpstring& name )
{
	m_Dirty = true;  
	SectionIter sec = m_Sections.insert( m_Sections.end(), Section() );
	sec->m_Name = name;
	stringtool::RemoveBorderingWhiteSpace( sec->m_Name );
	return ( sec );
}

IniFile::SectionIter IniFile :: PrivateSetSection( const gpstring& name )
{
	// look for existing section
	SectionColl::iterator i, begin = m_Sections.begin(), end = m_Sections.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Name.same_no_case( name ) )
		{
			return ( i );
		}
	}

	// not found, add it
	return ( PrivateAddSection( name ) );
}

//////////////////////////////////////////////////////////////////////////////
// class IniFile::Section implementation

bool IniFile::Section :: Save( FileSys::Writer& writer ) const
{
	if ( !m_Name.empty() )
	{
		if ( !writer.WriteF( "[%s]\n", m_Name.c_str() ) )  return ( false );
	}

	KeyColl::const_iterator i, begin = m_Keys.begin(), end = m_Keys.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->Save( writer ) )  return ( false );
	}

	return ( writer.WriteEol() );
}

bool IniFile::Section :: ReadKeyValue( const char* key, gpstring& out ) const
{
	KeyColl::const_iterator i, begin = m_Keys.begin(), end = m_Keys.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->m_IsComment && i->m_Key.same_no_case( key ) )
		{
			out = i->m_Value;
			return ( true );
		}
	}
	return ( false );
}

void IniFile::Section :: SetKeyValue( const gpstring& key, const gpstring& value )
{
	KeyColl::iterator i, begin = m_Keys.begin(), end = m_Keys.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( !i->m_IsComment && i->m_Key.same_no_case( key ) )
		{
			i->m_Value = value;
			stringtool::RemoveBorderingWhiteSpace( i->m_Value );
			return;
		}
	}

	AddKey( key, value );
}

//////////////////////////////////////////////////////////////////////////////
// class IniFile::Section::Key implementation

bool IniFile::Section::Key :: Save( FileSys::Writer& writer ) const
{
	if ( !m_Value.empty() )
	{
		if ( !m_Key.empty() )
		{
			if ( !writer.WriteF( "%s = %s\n", m_Key.c_str(), m_Value.c_str() ) )  return ( false );
		}
		else
		{
			if ( !writer.WriteText( m_Value, m_Value.length() ) )  return ( false );
			if ( !writer.WriteEol() )  return ( false );
		}
	}
	return ( true );
}

IniFile::Section::Key& IniFile::Section::Key :: SetComment( const gpstring& str )
{
	m_Key.clear();
	m_Value = str;
	stringtool::RemoveBorderingWhiteSpace( m_Value );
	m_IsComment = true;
	return ( *this );
}

IniFile::Section::Key& IniFile::Section::Key :: SetKeyValue( const gpstring& key, const gpstring& value )
{
	m_Key = key;
	stringtool::RemoveBorderingWhiteSpace( m_Key );
	m_Value = value;
	stringtool::RemoveBorderingWhiteSpace( m_Value );
	m_IsComment = false;
	return ( *this );
}

IniFile::Section::Key& IniFile::Section::Key :: SetValue( const gpstring& value )
{
	m_Key.clear();
	m_Value = value;
	stringtool::RemoveBorderingWhiteSpace( m_Value );
	m_IsComment = false;
	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// class IniFile::AutoSection implementation

void IniFile::AutoSection :: Init( IniFile* ini, const char* key )
{
	gpassert( (key != NULL) && (*key != '\0') );

	// $ note: the INI format only allows nesting of one level. so for a path
	//         type key we'll use the entire subkey name as the section name.
	//         also by convention forward slashes become backslashes.

	// skip initial backslash if any
	if ( (*key == '/') || (*key == '\\') )
	{
		++key;
	}

	// setup subkey
	gpstring subKey = ini->m_CurSection->m_Name;
	size_t pathPos = ::strrcspn( key, "\\/" );
	if ( pathPos != -1 )
	{
		// tack on subkey path
		if ( !subKey.empty() )
		{
			subKey += '\\';
		}
		subKey.append( key, pathPos );

		// replace any slashes we find
		std::replace( subKey.begin_split(), subKey.end_split(), '/', '\\' );

		// skip past the path and slash
		key += pathPos + 1;
	}

	// take em
	m_Section = ini->PrivateSetSection( subKey );
	m_KeyName = key;
}

//////////////////////////////////////////////////////////////////////////////
// class CommandLine implementation

static gpstring NextToken( const char*& iter )
{
	gpstring out;

	if ( *iter != '\0' )
	{
		const char* begin = iter;
		const char* end = stringtool::NextCommandLineToken( iter );
		out.assign( begin, end - begin );
	}

	return ( out );
}

static gpstring RemoveQuotes( gpstring str )
{
	if ( !str.empty() && (*str.begin() == '\"') && (*str.rbegin() == '\"') )
	{
		str.erase( str.size() - 1 );
		str.erase( 0, 1 );
	}
	return ( str );
}

void CommandLine :: Init( const char* commandLine )
{
	if ( commandLine == NULL )
	{
		commandLine = ::GetCommandLine();
	}

	for ( const char* p = commandLine ; *p != '\0' ;  )
	{
		if ( *p == '/' )
		{
			++p;

			// c++ comment?
			if ( *p == '/' )
			{
				break;
			}

			// c comment?
			if ( *p == '*' )
			{
				for ( ++p ; *p != '\0' ; ++p )
				{
					if ( (p[ 0 ] == '*') && (p[ 1 ] == '/') )
					{
						break;
					}
				}
				continue;
			}

			// it's a switch
			gpstring sw = RemoveQuotes( NextToken( p ) );
			if ( !sw.empty() )
			{
				bool on = true;
				if ( *sw.rbegin() == '+' )
				{
					sw.erase( sw.size() - 1 );
				}
				else if ( *sw.rbegin() == '-' )
				{
					on = false;
					sw.erase( sw.size() - 1 );
				}
				m_Keys.push_back( Key().SetKeyValue( sw, on ? "1" : "0" ) );
			}
		}
		else
		{
			// try name-value pair
			gpstring val = NextToken( p );
			if ( !val.empty() )
			{
				size_t eq = val.find( '=' );
				if ( eq != gpstring::npos )
				{
					m_Keys.push_back( Key().SetKeyValue( val.left( eq ), val.substr( eq + 1 ) ) );
				}
				else
				{
					m_Keys.push_back( Key().SetKey( val ) );
				}
			}
		}
	}
}

void CommandLine :: ResolveMacros( const IniFile& iniFile )
{
	// search for #-prefixed entries
	KeyColl::iterator i, ibegin = m_Keys.begin(), iend = m_Keys.end();
	for ( i = ibegin ; i != iend ; )
	{
		if ( (i->m_Key[ 0 ] == '#') && i->m_Value.empty() )
		{
			// fetch it
			gpstring macro;
			if ( iniFile.ReadKeyValue( gpstring( "cl_macros/" ) + (i->m_Key.c_str() + 1), macro ) )
			{
				// parse into new command line
				CommandLine cl( macro );

				// merge into my own
				m_Keys.splice( m_Keys.end(), cl.m_Keys );

				// not needed now
				i = m_Keys.erase( i );
			}
			else
			{
				++i;
			}
		}
		else
		{
			++i;
		}
	}
}

bool CommandLine :: ReadKeyValue( const char* key, gpstring& out ) const
{
	KeyColl::const_iterator i, begin = m_Keys.begin(), end = m_Keys.end();
	for ( i = begin ; i != end ; ++i )
	{
		if ( i->m_Key.same_no_case( key ) )
		{
			out = i->m_Value;
			return ( true );
		}
	}
	return ( false );
}

//////////////////////////////////////////////////////////////////////////////
// class CommandLine::Key implementation

CommandLine::Key& CommandLine::Key :: SetKeyValue( const gpstring& key, const gpstring& value )
{
	m_Key = RemoveQuotes( key );
	m_Value = RemoveQuotes( value );
	return ( *this );
}

CommandLine::Key& CommandLine::Key :: SetValue( const gpstring& value )
{
	m_Key.clear();
	m_Value = RemoveQuotes( value );
	return ( *this );
}

CommandLine::Key& CommandLine::Key :: SetKey( const gpstring& key )
{
	m_Key = RemoveQuotes( key );
	m_Value.clear();
	return ( *this );
}

//////////////////////////////////////////////////////////////////////////////
// class EnvironmentVars implementation

bool EnvironmentVars :: ReadKeyValue( const char* key, gpstring& out ) const
{
	return ( stringtool::GetEnvironmentVariable( m_Prefix + key, out ) );
}

//////////////////////////////////////////////////////////////////////////////
// class Registry implementation

Registry :: Registry( const char* companyName, const char* productName, bool localUser )
{
	gpassert( (companyName != NULL) && (*companyName != '\0') );
	gpassert( (productName != NULL) && (*productName != '\0') );

	m_Root = "Software\\";
	m_Root += companyName;
	m_Root += '\\';
	m_Root += productName;
	stringtool::RemoveTrailingBackslash( m_Root );

	m_LocalUser = localUser;
}

Registry :: ~Registry( void )
{
	// this space intentionally left blank...
}

void Registry :: SetSubKey( const gpstring& subKey )
{
	gpassert( subKey[ 0 ] != '\\' );
	gpassert( subKey.find( '.' ) == gpstring::npos );

	m_SubKey = subKey;
	stringtool::RemoveTrailingBackslash( m_SubKey );
	std::replace( m_SubKey.begin_split(), m_SubKey.end_split(), '/', '\\' );
}

bool Registry :: GetBool( const char* key, bool def ) const
{
	bool out;
	DWORD dout;
	if ( ReadDword( key, dout ) )
	{
		out = !!dout;
	}
	else
	{
		// try it as a string
		gpstring str;
		if ( !ReadString( key, str ) || str.empty() || !::FromString( str, out ) )
		{
			out = def;
		}
	}
	return ( out );
}

int Registry :: GetInt( const char* key, int def ) const
{
	int out;
	if ( !ReadDword( key, force_cast <DWORD> ( out ) ) )
	{
		// try it as a string
		gpstring str;
		if ( !ReadString( key, str ) || str.empty() || !::FromString( str, out ) )
		{
			out = def;
		}
	}
	return ( out );
}

float Registry :: GetFloat( const char* key, float def ) const
{
	float out;
	if ( !ReadDword( key, force_cast <DWORD> ( out ) ) )
	{
		// try it as a string
		gpstring str;
		if ( !ReadString( key, str ) || str.empty() || !::FromString( str, out ) )
		{
			out = def;
		}
	}
	return ( out );
}

bool Registry :: ReadKeyValue( const char* key, gpstring& out ) const
{
	return ( ReadString( key, out ) );
}

//////////////////////////////////////////////////////////////////////////////
// class Registry::AutoKey implementation

Registry::AutoKey :: ~AutoKey( void )
{
	if ( m_Key != NULL )
	{
		::RegCloseKey( m_Key );
	}
}

void Registry::AutoKey :: Init( const Registry* reg, const char* key, REGSAM access )
{
	gpassert( (key != NULL) && (*key != '\0') );

	// $ note: the registry allows forward slashes to be used in the names of
	//         keys and value names. to avoid confusion, we're going to disallow
	//         this. slashes are slashes, just like in the file system.

	// skip initial backslash if any
	if ( (*key == '/') || (*key == '\\') )
	{
		++key;
	}

	// setup subkey
	gpstring subKey( reg->m_Root );
	if ( !reg->m_SubKey.empty() )
	{
		subKey += '\\';
		subKey += reg->m_SubKey;
	}
	size_t pathPos = ::strrcspn( key, "\\/" );
	if ( pathPos != -1 )
	{
		// tack on subkey path
		subKey += '\\';
		subKey.append( key, pathPos );
		std::replace( subKey.begin_split(), subKey.end_split(), '/', '\\' );

		// skip past the path and slash
		key += pathPos + 1;
	}

	// take the key
	m_KeyName = key;

	// open the key
	HKEY hkey = reg->m_LocalUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
	if ( ::RegOpenKeyEx( hkey, subKey, 0, access, &m_Key ) != ERROR_SUCCESS )
	{
		m_Key = NULL;

		if ( access & KEY_CREATE_SUB_KEY )
		{
			const char* begin = subKey.c_str();
			const char* iter = begin;
			for ( ; *begin != '\0' ; begin = iter )
			{
				const char* end = stringtool::NextPathComponent( iter );
				if ( ::RegCreateKeyEx( hkey, gpstring( begin, end ), 0, NULL, 0,
									   KEY_ALL_ACCESS, NULL, &hkey, NULL ) != ERROR_SUCCESS )
				{
					return;
				}
			}
			m_Key = hkey;
		}
	}
}

bool Registry::AutoKey :: SetBinary( const_mem_ptr out )
{
	return ( SetValue( REG_BINARY, out ) );
}

bool Registry::AutoKey :: SetDword( DWORD out )
{
	return ( SetValue( REG_DWORD, make_const_mem_ptr( out ) ) );
}

bool Registry::AutoKey :: SetQword( QWORD out )
{
	return ( SetValue( REG_BINARY, make_const_mem_ptr( out ) ) );
}

bool Registry::AutoKey :: SetString( const gpstring& out )
{
	return ( SetValue( REG_SZ, const_mem_ptr( out.c_str(), out.length() + 1 ) ) );
}

bool Registry::AutoKey :: SetWString( const gpwstring& out )
{
	return ( SetString( ::UnicodeToUtf8( out ) ) );
}

bool Registry::AutoKey :: SetStrings( const Strings& out )
{
	std::vector <char> buffer;

	Strings::const_iterator i, begin = out.begin(), end = out.end();
	for ( i = begin ; i != end ; ++i )
	{
		gpassert( !out.empty() );		// empty strings not allowed
		if ( !out.empty() )
		{
			buffer.insert( buffer.end(), i->begin(), i->end() + 1 );
		}
	}
	buffer.push_back( 0 );
	if ( buffer.size() < 2 )
	{
		buffer.push_back( 0 );
	}
	return ( SetValue( REG_MULTI_SZ, const_mem_ptr( buffer.begin(), buffer.size() ) ) );
}

bool Registry::AutoKey :: ReadBinary( mem_ptr out ) const
{
	return ( QueryValue( REG_BINARY, out ) );
}

bool Registry::AutoKey :: ReadDword( DWORD& out ) const
{
	return ( QueryValue( REG_DWORD, make_mem_ptr( out ) ) );
}

bool Registry::AutoKey :: ReadQword( QWORD& out ) const
{
	return ( QueryValue( REG_BINARY, make_mem_ptr( out ) ) );
}

bool Registry::AutoKey :: ReadString( gpstring& out ) const
{
	DWORD type, size;
	if ( !QueryInfo( type, size ) || (type != REG_SZ) )
	{
		return ( false );
	}
	if ( size <= 1 )
	{
		out.clear();
		return ( true );
	}
	out.resize( size - 1 );
	return ( QueryValue( mem_ptr( out.begin_split(), size ) ) );
}

bool Registry::AutoKey :: ReadWString( gpwstring& out ) const
{
	gpstring temp;
	bool rc = ReadString( temp );
	if ( rc )
	{
		out = ::Utf8ToUnicode( temp );
	}
	return ( rc );
}

bool Registry::AutoKey :: ReadStrings( Strings& out ) const
{
	DWORD type, size;
	if ( !QueryInfo( type, size ) || (type != REG_MULTI_SZ) )
	{
		return ( false );
	}
	if ( size <= 2 )
	{
		return ( true );
	}

	std::vector <char> buffer( size );
	if ( !QueryValue( mem_ptr( buffer.begin(), size ) ) )
	{
		return ( false );
	}

	gpstring str;
	for ( const char* p = buffer.begin() ; *p != '\0' ; p += str.length() + 1 )
	{
		out.push_back( str = p );
	}

	return ( true );
}

bool Registry::AutoKey :: SetValue( DWORD type, const_mem_ptr out )
{
	if ( m_Key == NULL )
	{
		return ( false );
	}

	return ( ::RegSetValueEx( m_Key, m_KeyName, 0, type, rcast <const BYTE*> ( out.mem ), out.size )
			 == ERROR_SUCCESS  );
}

bool Registry::AutoKey :: QueryValue( DWORD type, mem_ptr out ) const
{
	// bail on no key
	if ( m_Key == NULL )
	{
		return ( false );
	}

	// verify type
	DWORD realType, realSize;
	if ( !QueryInfo( realType, realSize ) || (realType != type) || (realSize != out.size) )
	{
		return ( false );
	}

	// get data
	return ( QueryValue( out ) );
}

bool Registry::AutoKey :: QueryValue( mem_ptr out ) const
{
	if ( m_Key == NULL )
	{
		return ( false );
	}

	DWORD size = out.size;
	return ( ::RegQueryValueEx( m_Key, m_KeyName, NULL, NULL, rcast <BYTE*> ( out.mem ), &size )
			 == ERROR_SUCCESS );
}

bool Registry::AutoKey :: QueryInfo( DWORD& type, DWORD& size ) const
{
	if ( m_Key == NULL )
	{
		return ( false );
	}

	return ( ::RegQueryValueEx( m_Key, m_KeyName, NULL, &type, NULL, &size ) == ERROR_SUCCESS );
}

//////////////////////////////////////////////////////////////////////////////
// struct ConfigInit implementation

ConfigInit :: ConfigInit( void )
{
	m_CompanyName = COMPANY_NAME;
	SetProductName( FileSys::GetFileNameOnly( FileSys::GetModuleFileName() ) );
	m_DocsFolder = -1;
	m_EnvVarPrefix = "GPG_";
	m_DisableDevMode = GP_SIEGEMAX || GP_RETAIL;
}

void ConfigInit :: SetProductName( gpstring productName )
{
	m_IniFileName  = productName;
	m_RegistryName = productName;
	m_DocsSubDir   = productName;
}

void ConfigInit :: SetMyDocuments( void )
{
	m_DocsFolder = CSIDL_PERSONAL;
}

//////////////////////////////////////////////////////////////////////////////
// class Config implementation

static kerneltool::Critical s_Critical;

Config :: Config( const ConfigInit& init )
	: m_Init           ( init ),
	  m_LocalRegistry  ( init.m_CompanyName, init.m_RegistryName, true ),
	  m_GlobalRegistry ( init.m_CompanyName, init.m_RegistryName, false ),
	  m_EnvironmentVars( init.m_EnvVarPrefix )
{
	m_OutRegistry = &m_LocalRegistry;

// Init dev-mode flag.

	// default is off
	m_DevMode = false;

	// and only auto-detect if it wasn't disabled
	if ( !init.m_DisableDevMode )
	{
		m_DevMode = stringtool::GetEnvironmentVariable( "USERDOMAIN" ).same_no_case( "GASPOWERED" );
	}

	// try this var
	m_EnvironmentVars.Read( "dev", m_DevMode );

	// special: permit an INI that exists in EXE dir to override (only do this
	// in dev modes to avoid cpu cost)
#	if !GP_RETAIL
	IniFile iniFile;
	if ( iniFile.LoadFile( m_Init.m_IniFileName ) )
	{
		iniFile.Read( m_Init.m_EnvVarPrefix + "dev", m_DevMode );
	}
#	endif // !GP_RETAIL

	// ...but command line has final say
	m_CommandLine.Read( m_Init.m_EnvVarPrefix + "dev", m_DevMode );
}

Config :: ~Config( void )
{
	FlushChanges();
}

bool Config :: Init( bool errorOnFail )
{
	ePathVarMode errorMode = errorOnFail ? PVM_ERROR_ON_FAIL : PVM_NONE;

	// install path is where the EXE is located
	SetPathVar( "exe_path", FileSys::GetModuleDirectory() );

	// this is a place guaranteed to be ok to write to, but not necessarily easy
	// to find. that's ok though, because windows knows %temp% and can erase
	// files from it on the cleanup wizard. if we put our big files there then
	// no worries about space usage.
	SetPathVar( "temp_path", FileSys::GetTempWritableDir( true, true ), PVM_USE_CONFIG | PVM_WRITABLE | PVM_ERROR_ON_FAIL );

	// log output defaults to first writable dir so it's easier to find
	SetPathVar( "log_path", FileSys::GetLocalWritableDir(), PVM_USE_CONFIG | PVM_WRITABLE | PVM_ERROR_ON_FAIL );

	// try config version
	if ( !SetPathVar( "user_path", (const char*)NULL, PVM_USE_CONFIG | PVM_WRITABLE | errorMode ) )
	{
		bool userSet = false;

		// if not dev mode, try using "my documents" folder
		if ( !IsDevMode() && (m_Init.m_DocsFolder >= 0) )
		{
			// if our shfolder version is too low to handle our query or we
			// don't know what it is, just force dev mode
			ShFolderDll shfolder;
			char userPathBuf[ MAX_PATH ];
			if ( shfolder && SUCCEEDED( shfolder.SHGetFolderPath( NULL, m_Init.m_DocsFolder, NULL, SHGFP_TYPE_CURRENT, userPathBuf ) ) )
			{
				gpwstring userPath( ::ToUnicode( userPathBuf ) );

				// make sure that the base path exists. some users delete their
				// user folders and get pissed if i re-create any of them.
				if ( FileSys::DoesPathExist( userPath ) )
				{
					userPath += L'\\';
					userPath += ::ToUnicode( m_Init.m_DocsSubDir );
					if ( SetPathVar( "user_path", userPath, PVM_CREATE | errorMode ) )
					{
						userSet = true;
					}
				}
			}
			else
			{
				// installation problem, tell the user
				gperrorbox( $MSG$ "Unable to access SHGetFolderPath(). Your SHFOLDER.DLL may need updating. Try reinstalling to fix.\n" );
			}
		}

		// user path still not set? ok default to EXE dir
		if ( !userSet )
		{
			SetPathVar( "user_path", "%exe_path%", PVM_WRITABLE | errorMode );
		}
	}

	// successful if we have a user_path out of this
	bool success = IsPathVarSet( "user_path" );
	if ( success )
	{
		// try to set log path off user path so it's super easy to find for PSS
		gpstring logPath = "%user_path%";
		if ( !m_Init.m_LogSubDir.empty() )
		{
			logPath += '/';
			logPath += m_Init.m_LogSubDir;
		}

		// set it
		SetPathVar( "log_path", logPath, Config::PVM_CREATE );
	}

	// figure out INI filename
	gpstring iniFileName = m_Init.m_IniFileName;
	if ( !FileSys::HasPathInfo( iniFileName ) )
	{
		// it goes in user path dir
		iniFileName = ResolvePathVars( "%user_path%/" + m_Init.m_IniFileName );
	}

	// load INI file in
	if ( m_IniFile.LoadFile( iniFileName, &m_IniFileName ) )
	{
		m_CommandLine.ResolveMacros( m_IniFile );
	}

	// done
	return ( success );
}

void Config :: FlushChanges( bool forceWrite )
{
	kerneltool::Critical::Lock locker( s_Critical );

	if ( forceWrite || m_IniFile.IsDirty() )
	{
		m_IniFile.SaveFile( m_IniFileName );
	}
}

bool Config :: ReadKeyValue( const char* key, gpstring& out ) const
{
	kerneltool::Critical::Lock locker( s_Critical );

	gpassert( key != NULL );

	// ignore cl and env for pathed keys
	bool hasPath = key[ ::strcspn( key, "\\/" ) ] != '\0';

	// try them...
	return (   (!hasPath && m_CommandLine    .ReadKeyValue( key, out ))
			||              m_IniFile        .ReadKeyValue( key, out )
			||              m_LocalRegistry  .ReadKeyValue( key, out )
			||              m_GlobalRegistry .ReadKeyValue( key, out )
			|| (!hasPath && m_EnvironmentVars.ReadKeyValue( key, out )) );
}

bool Config :: SetKeyValue( const char* key, const gpstring& out )
{
	kerneltool::Critical::Lock locker( s_Critical );

	if ( m_OutRegistry != NULL )
	{
		return ( m_OutRegistry->SetKeyValue( key, out ) );
	}
	else
	{
		return ( m_IniFile.SetKeyValue( key, out ) );
	}
}

const gpstring& Config :: FUBI_RENAME( GetString )( const char* key, const char* def ) const
{
	static gpstring s_String;
	s_String = GetString( key, def );
	return ( s_String );
}

bool Config :: ResolvePathVars( gpstring& out, const char* in, ePathVarMode mode )
{
	gpwstring outw;
	bool rc = ResolvePathVars( outw, ::ToUnicode( in ), mode );
	out = ::ToAnsi( outw );
	return ( rc );
}

bool Config :: ResolvePathVars( gpwstring& out, const wchar_t* in, ePathVarMode mode )
{
	// just copy it over first
	out = in;

	// loop until everything replaced
	for ( ; ; )
	{
		// find first
		wchar_t* beg = ::wcschr( out, L'%' );
		if ( beg == NULL )
		{
			break;
		}

		// find next
		wchar_t* end = ::wcschr( beg + 1, L'%' );
		if ( end == NULL )
		{
			break;
		}

		// build a key
		gpstring key = ::ToAnsi( gpwstring( beg + 1, end - beg - 1 ) );

		// look it up
		gpwstring var;
		std::pair <StringStringDb::const_iterator, StringStringDb::const_iterator> rc
				= m_PathVarDb.equal_range( key );
		if ( rc.first != rc.second )
		{
			for ( StringStringDb::const_iterator i = rc.first ; i != rc.second ; ++i )
			{
				gpassert( !i->second.empty() );

				if ( i != rc.first )
				{
					var += L';';
				}
				var += i->second;

				if ( !(mode & PVM_APPEND) )
				{
					break;
				}
			}
		}
		else if ( mode & PVM_USE_CONFIG )
		{
			var = gConfig.GetWString( key );
		}

		if ( var.empty() )
		{
			if ( mode & (PVM_ERROR_ON_FAIL | PVM_FATAL_ON_FAIL) )
			{
				ReportSys::ContextRef ctx = (mode & PVM_ERROR_ON_FAIL) ? &gErrorBoxContext : &gFatalContext;
				ctx->OutputF( $MSG$ "Unable to resolve a critical path that is "
									"required to operate. This may be caused by incorrect "
									"INI file, shortcut, or registry settings. Try "
									"reinstalling to resolve this problem. This condition "
									"can also result from this user account not having "
									"permission to access this directory.\n"
									"\n"
									"Internal info:\n"
									"  in var: '%S'\n"
									"  key: '%s'\n",
									in,
									key.c_str() );
			}

			// abort!
			out.clear();
			return ( false );
		}

		// replace it
		out.replace( beg, end + 1, var );
	}

	// clean it up if not appending (multi-paths will cleanup independently as they parse out)
	if ( !(mode & PVM_APPEND) )
	{
		FileSys::CleanupPath( out );
	}

	// done
	return ( true );
}

gpstring Config :: ResolvePathVars( const char* str, ePathVarMode mode )
{
	gpstring out;
	ResolvePathVars( out, str, mode );
	return ( out );
}

gpwstring Config :: ResolvePathVars( const wchar_t* str, ePathVarMode mode )
{
	gpwstring out;
	ResolvePathVars( out, str, mode );
	return ( out );
}

bool Config :: SetPathVar( const char* var, const char* value, ePathVarMode mode )
{
	if ( value != NULL )
	{
		return ( SetPathVar( var, ::ToUnicode( value ), mode ) );
	}
	else
	{
		return ( SetPathVar( var, (const wchar_t*)NULL, mode ) );
	}
}

bool Config :: SetPathVar( const char* var, const wchar_t* value, ePathVarMode mode )
{
	gpassert( var && *var );
	bool success = false;

	// try config first?
	if ( mode & PVM_USE_CONFIG )
	{
		gpwstring configValue = GetWString( var );
		if ( !configValue.empty() )
		{
			return ( SetPathVar( var, configValue, mode & NOT( PVM_USE_CONFIG ) ) );
		}
	}

	// try what we had now
	if ( value && *value )
	{
		std::list <gpwstring> paths;
		if ( mode & PVM_APPEND )
		{
			// extract paths
			FileSys::FindPathsFromSpec( value, paths );
		}
		else
		{
			// use entire thing
			paths.push_back( value );
		}

		std::list <gpwstring>::const_iterator i, ibegin = paths.begin(), iend = paths.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			if ( setPathVar( var, *i, mode ) )
			{
				success = true;
			}
			else if ( mode & (Config::PVM_ERROR_ON_FAIL | Config::PVM_FATAL_ON_FAIL) )
			{
				ReportSys::ContextRef ctx = (mode & Config::PVM_ERROR_ON_FAIL) ? &gErrorBoxContext : &gFatalContext;

				DWORD lastError = ::GetLastError();
				if ( lastError == ERROR_ACCESS_DENIED )
				{
					ctx->OutputF( $MSG$ "Unable to access or create a critical "
										"path that is required to operate. The "
										"system returned an 'access denied' "
										"error which most likely means that "
										"you do not have permission to write "
										"to or create the '%S' directory.\n"
										"\n"
										"Internal info:\n"
										"  path var: '%s'\n"
										"  value: '%S'\n"
										"  resolved: '%S'\n"
										"  error: '%s' (0x%08X)\n",
										ResolvePathVars( *i, PVM_USE_CONFIG ).c_str(),
										var,
										i->c_str(), ResolvePathVars( *i, PVM_USE_CONFIG ).c_str(),
										stringtool::GetLastErrorText( lastError ).c_str(), lastError );
				}
				else
				{
					ctx->OutputF( $MSG$ "Unable to access or create a critical "
										"path that is required to operate. "
										"This may be caused by incorrect INI "
										"file, shortcut, or registry settings. "
										"Try reinstalling to resolve this "
										"problem.\n"
										"\n"
										"Internal info:\n"
										"  path var: '%s'\n"
										"  value: '%S'\n"
										"  resolved: '%S'\n"
										"  error: '%s' (0x%08X)\n",
										var,
										i->c_str(), ResolvePathVars( *i, PVM_USE_CONFIG ).c_str(),
										stringtool::GetLastErrorText( lastError ).c_str(), lastError );
				}
			}
		}
	}

	// done
	return ( success );
}

bool Config :: AddPathVar( const char* var, const char* value, ePathVarMode mode )
{
	return ( SetPathVar( var, value, mode | PVM_APPEND ) );
}

bool Config :: AddPathVar( const char* var, const wchar_t* value, ePathVarMode mode )
{
	return ( SetPathVar( var, value, mode | PVM_APPEND ) );
}

bool Config :: IsPathVarSet( const char* var ) const
{
	return ( m_PathVarDb.find( var ) != m_PathVarDb.end() );
}

bool Config :: DeletePathVar( const char* var )
{
	std::pair <StringStringDb::iterator, StringStringDb::iterator> rc
			= m_PathVarDb.equal_range( var );
	m_PathVarDb.erase( rc.first, rc.second );
	return ( rc.first != rc.second );
}

#if !GP_RETAIL

void Config :: DumpPathVars( ReportSys::ContextRef ctx )
{
	// dump objects into sink
	ReportSys::TextTableSink sink;
	ReportSys::LocalContext lctx( &sink, false );
	lctx.SetSchema( new ReportSys::Schema( 3, "Key", "Value", "Expanded" ), true );

	// path vars
	StringStringDb::const_iterator i, ibegin = m_PathVarDb.begin(), iend = m_PathVarDb.end();
	for ( i = ibegin ; i != iend ; ++i )
	{
		ReportSys::AutoReport autoReport( lctx );

		gpstring name( i->first );

		// if this also exists in config, special tag
		gpwstring value( i->second );
		if ( !GetWString( name ).empty() )
		{
			value.insert( 0U, 1U, L'[' );
			value += L']';
		}

		// check for continuation
		if ( i != ibegin )
		{
			StringStringDb::const_iterator last = i;
			--last;
			if ( last->first.same_no_case( i->first ) )
			{
				name.clear();
			}
		}

		lctx.OutputFieldObj( name );
		lctx.OutputFieldObj( value );
		lctx.OutputFieldObj( ResolvePathVars( i->second ) );
	}

	// out header
	gpgeneric( "\n" );
	gGenericContext.OutputRepeatWidth( sink.CalcDividerWidth(), "-==- " );
	gpgeneric( "\n\nPath variables:\n\n" );

	// out table
	sink.OutputReport( ctx );
}

#endif // !GP_RETAIL

bool Config :: setPathVar( const char* var, const wchar_t* value, ePathVarMode mode )
{
	gpassert( var && *var );
	gpassert( value );

	// just fail if empty and supposed to be a path
	if ( (*value == L'\0') && (mode & (PVM_REQUIRED | PVM_ABSOLUTE_PATH)) )
	{
		return ( false );
	}

	// first resolve it, may have embedded dirs
	gpwstring localValue = ResolvePathVars( value, mode );
	if ( localValue.empty() )
	{
		return ( false );
	}

	// split into parts, and assume a\b style path where 'b' is path not file
	FileSys::PathSpec pathSpec;
	pathSpec.Init( localValue, L"", false );

	// attempt to create?
	if ( (mode & PVM_CREATE) == PVM_CREATE )
	{
		// build it - note that this will do nothing and succeed if already exists
		if ( !FileSys::CreateFullDirectory( pathSpec.m_PathName ) )
		{
			return ( false );
		}

		// make sure it's writable
		if ( !FileSys::IsDirWritable( pathSpec.m_PathName ) )
		{
			return ( false );
		}
	}
	else if ( (mode & PVM_WRITABLE) == PVM_WRITABLE )
	{
		if ( !FileSys::IsDirWritable( pathSpec.m_PathName ) )
		{
			return ( false );
		}
	}
	else if ( mode & PVM_REQUIRED )
	{
		if ( !FileSys::DoesPathExist( pathSpec.m_PathName ) )
		{
			return ( false );
		}
	}

	// must be absolute?
	if ( mode & PVM_ABSOLUTE_PATH )
	{
		if ( !FileSys::IsAbsolute( pathSpec.m_PathName ) )
		{
			::SetLastError( ERROR_BAD_PATHNAME );
			return ( false );
		}
	}

	// restore to original to preserve embedded vars, and clean up for storage
	localValue = value;
	FileSys::CleanupPath( localValue );

	// register
	if ( mode & PVM_APPEND )
	{
		if ( mode & PVM_APPEND_DUPS_OK )
		{
			m_PathVarDb.insert( std::make_pair( var, localValue ) );
		}
		else
		{
			// check for dup already there
			std::pair <StringStringDb::iterator, StringStringDb::iterator> rc
					= m_PathVarDb.equal_range( var );
			for ( ; rc.first != rc.second ; ++rc.first )
			{
				// found!
				if ( localValue.same_no_case( rc.first->second ) )
				{
					break;
				}
			}

			// only if not found do we insert it
			if ( rc.first == rc.second )
			{
				m_PathVarDb.insert( std::make_pair( var, localValue ) );
			}
		}
	}
	else
	{
		std::pair <StringStringDb::iterator, StringStringDb::iterator> rc
				= m_PathVarDb.equal_range( var );
		m_PathVarDb.erase( rc.first, rc.second );
		m_PathVarDb.insert( std::make_pair( var, localValue ) );
	}

	// must be successful
	return ( true );
}

//////////////////////////////////////////////////////////////////////////////
