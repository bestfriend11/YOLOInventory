//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_FuBiPacker.h"

#include "AppModule.h"
#include "FileSysUtils.h"
#include "FuBi.h"
#include "GpZLib.h"
#include "ReportSys.h"
#include "StringTool.h"

//////////////////////////////////////////////////////////////////////////////
// helper methods

typedef stdx::fast_vector <BYTE> Buffer;

template <typename T>
void store( Buffer& buffer, const T& obj )
{
	buffer.insert( buffer.end(), (BYTE*)&obj, (BYTE*)&obj + sizeof( T ) );
}

void store( Buffer& buffer, const gpstring& obj )
{
	buffer.insert( buffer.end(), (BYTE*)obj.c_str(), (BYTE*)obj.c_str() + obj.length() + 1 );
}

//////////////////////////////////////////////////////////////////////////////
// class App declaration and implementation

class App : public AppModule
{
public:
	SET_INHERITED( App, AppModule );

	App( void )
	{
		// this space intentionally left blank...
	}

	virtual ~App( void )
	{
		// this space intentionally left blank...
	}

private:
	bool ImportFunction( const char* mangledName, const char* unmangledName, DWORD address, void* )
	{
		gpgenericf(( "%s = %s @ 0x%08X\n", mangledName, unmangledName, address ));
		return ( true );
	}

	virtual bool OnExecute( void )
	{
		stringtool::CommandLineExtractor cl;

	// Fetch imports.

		// test?
		if ( cl.HasTag( "-test" ) )
		{
			FuBi::SysExports sysExports;
			bool rc = sysExports.ImportBindings();
			gpgenericf(( "%s\n", rc ? "Success!" : "Failure!" ));
			return ( rc );
		}

		// get the file to process
		gpstring fileName = cl.FindPostTag( "-file" );
		if ( fileName.empty() )
		{
			gpgenericf(( "Usage: FUBIPACKER.EXE [-test] -file <filename>\n" ));
			return ( false );
		}
		FileSys::MemoryFile file;
		gpgenericf(( "Opening file '%s' as image...\n", fileName.c_str() ));
		if ( !file.Create(
				fileName,
				FileSys::MemoryFile::ACCESS_READ_ONLY,
				FileSys::MemoryFile::SHARE_READ_ONLY,
				FILE_ATTRIBUTE_NORMAL,
				NULL,
				SEC_IMAGE ) )
		{
			gperrorf(( "Unable to open file '%s' for reading, error is '%s'\n",
					   fileName.c_str(), stringtool::GetLastErrorText().c_str() ));
			return ( false );
		}

		// fetch imports
		gpgeneric( "Importing bindings...\n" );
		FuBi::SysExports::ExportColl exportColl;
		{
			FuBi::SysExports sysExports;
			if ( !sysExports.ImportBindings( (HMODULE)file.GetData(), &exportColl ) )
			{
				gperrorf(( "Error importing bindings from '%s'\n", fileName.c_str() ));
				return ( false );
			}
			gpgenericf(( "...%d found!\n", exportColl.size() ));
		}

		// reopen file as read/write
		gpgenericf(( "Reopening file '%s' for writes...\n", fileName.c_str() ));
		if ( !file.Close() || !file.Create( fileName, FileSys::MemoryFile::ACCESS_READ_WRITE ) )
		{
			gperrorf(( "Unable to open file '%s' for writing, error is '%s'\n",
					   fileName.c_str(), stringtool::GetLastErrorText().c_str() ));
			return ( false );
		}

	// Serialize.

		// pack into a single stream
		Buffer exportBuffer;

		// first store the element count
		store( exportBuffer, (WORD)exportColl.size() );

		// now serialize all members
		gpgeneric( "Serializing...\n" );
		FuBi::SysExports::ExportColl::const_iterator i,
				ibegin = exportColl.begin(), iend = exportColl.end();
		for ( i = ibegin ; i != iend ; ++i )
		{
			store( exportBuffer, i->m_MangledName );
			store( exportBuffer, i->m_UnmangledName );
			store( exportBuffer, i->m_Address );
		}

	// Process.

		// compress
		gpgeneric( "Compressing...\n" );
		Buffer compressBuffer;
		compressBuffer.resize( zlib::GetMaxDeflateSize( exportBuffer.size() ) );
		int compressedSize = zlib::Deflate(
				mem_ptr( compressBuffer.begin(), compressBuffer.size() ),
				const_mem_ptr( exportBuffer.begin(), exportBuffer.size() ),
				9 /* max compression */ );
		if ( compressedSize <= 0 )
		{
			gperrorf(( "Error compressing import table!\n" ));
			return ( false );
		}

		// zero-extend to qword
		compressBuffer.resize( compressedSize );
		store( compressBuffer, (QWORD)0 );
		QwordAlignUp( compressedSize );
		compressBuffer.resize( compressedSize );

		// encrypt in place
		gpgeneric( "Encrypting...\n" );
		FuBi::Encrypt( compressBuffer.begin(), compressBuffer.size() );

		// build header
		FuBi::ExportTableHeader header;
		header.m_Signature = FuBi::FUBI_SIGNATURE;
		header.m_CompressedSize = compressedSize;
		header.m_UncompressedSize = exportBuffer.size();

	// Patch EXE.

		gpgeneric( "Repacking EXE...\n" );

		// get headers
		BYTE* imageBase = rcast <BYTE*> ( file.GetData() );
		IMAGE_DOS_HEADER* dosHeader = rcast <IMAGE_DOS_HEADER*> ( imageBase );
		IMAGE_NT_HEADERS* winHeader = rcast <IMAGE_NT_HEADERS*> ( imageBase + dosHeader->e_lfanew );

		// find the export data directory
		IMAGE_DATA_DIRECTORY& exportDataDir = winHeader->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
		DWORD exportRva  = exportDataDir.VirtualAddress;
		DWORD exportSize = exportDataDir.Size;

		// have to fail if compression didn't make it so we'll fit
		DWORD neededSize = compressedSize + sizeof( FuBi::ExportTableHeader );
		if ( exportSize < neededSize )
		{
			gperrorf(( "Error: compressed import table (%d bytes) is too small to fit in existing export table (%d bytes)\n",
					   neededSize, exportSize ));
			return ( false );
		}

		// write the export header
		FuBi::ExportTableHeader* fubiHeader = rcast <FuBi::ExportTableHeader*> ( imageBase + exportRva );
		::CopyObject( *fubiHeader, header );

		// write the compressed encrypted data
		::memcpy( fubiHeader + 1, compressBuffer.begin(), compressedSize );

		// fill the rest with random data
		std::generate( (BYTE*)fubiHeader + neededSize, (BYTE*)fubiHeader + exportSize, RandomByte );

		// ok now checksum everything after the crc and write it back
		void* crcBase = (BYTE*)&winHeader->OptionalHeader.CheckSum + sizeof( winHeader->OptionalHeader.CheckSum );
		UINT32 crc = ::GetCRC32( crcBase, file.GetSize() - ((BYTE*)crcBase - (BYTE*)file.GetData()) );
		winHeader->OptionalHeader.CheckSum = crc;

		// all done!
		gpgenericf(( "Done! Repacked %d export bytes into %d preprocessed fubi bytes, checksum is 0x%08X\n",
					 exportSize, neededSize, crc ));
		return ( true );
	}

private:
	SET_NO_COPYING( App );
};

//////////////////////////////////////////////////////////////////////////////
// function main() implementation

int main( void )
{
	ReportSys::FileSink consoleSink( ::GetStdHandle( STD_OUTPUT_HANDLE ) );
	gGlobalSink.AddSink( &consoleSink, false );

	return ( App().AutoRun() );
}

//////////////////////////////////////////////////////////////////////////////
