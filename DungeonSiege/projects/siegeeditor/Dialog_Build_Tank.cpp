// Dialog_Build_Tank.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Build_Tank.h"
#include "Dialog_Save_As_DSMod.h"
#include "Dialog_Extract_Tank.h"
#include "FileSysXfer.h"
#include "KernelTool.h"
#include "RatioStack.h"
#include "RapiImage.h"
#include "TankBuilder.h"
#include "TankFileMgr.h"
#include "TankMgr.h"
#include "ReportSysDialogs.h"
#include "GpZlib.h"
#include "GpLzo.h"
#include "Config.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////
// class ImgFilter declaration

class ImgFilter : public Tank::Builder::Filter
{
public:
	SET_INHERITED( ImgFilter, Tank::Builder::Filter );

	enum /*FOURCC*/
	{
		FILTER_RAW_MIPMAPS_STRIPTOP = 'RawS',
		FILTER_RAW_MIPMAPS          = 'RawM',
		FILTER_RAW_PLAIN            = 'Raw0',
	};

	ImgFilter( DWORD type )			{  m_Type = type;  }
	virtual ~ImgFilter( void )		{  }

	static void Register( void )
	{
		Tank::Builder::RegisterFilter( FILTER_RAW_MIPMAPS_STRIPTOP, new ImgFilter( FILTER_RAW_MIPMAPS_STRIPTOP ) );
		Tank::Builder::RegisterFilter( FILTER_RAW_MIPMAPS,          new ImgFilter( FILTER_RAW_MIPMAPS          ) );
		Tank::Builder::RegisterFilter( FILTER_RAW_PLAIN,            new ImgFilter( FILTER_RAW_PLAIN            ) );
	}

private:
	typedef Tank::Builder::Buffer Buffer;

	virtual bool OnFilter( const gpstring& fileName, gpstring& newName, Buffer& buffer )
	{
		bool success = false;
		bool useMipMaps = (m_Type == FILTER_RAW_MIPMAPS_STRIPTOP) || (m_Type == FILTER_RAW_MIPMAPS);
		bool stripTop   = m_Type == FILTER_RAW_MIPMAPS_STRIPTOP;

		FileSys::BufferWriter writer( m_TempBuffer );
		m_TempBuffer.clear();

		std::auto_ptr <RapiImageReader> reader( RapiImageReader::CreateReader( fileName, const_mem_ptr( &*buffer.begin(), buffer.size() ), useMipMaps, false ) );
		if ( reader.get() )
		{

			// strip off mip 0 if (a) it's been requested and (b) it's large enough
			// to be worth worrying about
			if ( useMipMaps && stripTop && (reader->GetWidth() >= 16) && (reader->GetHeight() >= 16) )
			{
				reader->SkipNextSurface();
			}

			if ( WriteImage( writer, IMAGE_RAW, reader.get(), false, useMipMaps ) )
			{
				m_TempBuffer.swap( buffer );
				stringtool::ReplaceExtension( newName, gpstring( "." ) + ::GetExtension( IMAGE_RAW ) );
				success = true;
			}
		}

		return ( success );
	}

	// local data
	DWORD  m_Type;
	Buffer m_TempBuffer;

	SET_NO_COPYING( ImgFilter );
};

/////////////////////////////////////////////////////////////////////////////
// WorkerThread

class WorkerThread : public kerneltool::Thread, private RatioStack
{
private:

	const char*        m_Name;

protected:

	Dialog_Build_Tank* m_Output;
	kerneltool::Event  m_RequestQuit;
	gpstring           m_LastStatus;
	Tank::Builder      m_Builder;
	bool               m_WereAssertsEnabled;
	ReportSys::Sink*   m_EditSink;

public:

	WorkerThread( const char* name, Dialog_Build_Tank* output )
	{
		m_Name               = name;
		m_Output             = output;
		m_WereAssertsEnabled = ReportSys::IsErrorAssertEnabled();
		m_EditSink           = NULL;

		// wait for thread to finish starting before caller continues
		SetOptions( kerneltool::Thread::OPTION_BEGIN_THREAD_WAIT );
	}

	virtual ~WorkerThread( void )
	{
		if ( m_EditSink != NULL )
		{
			ReportSys::EnableErrorAssert( m_WereAssertsEnabled );
			gGlobalSink.RemoveSink( m_EditSink );
		}
	}

	void RequestQuit( void )
	{
		m_RequestQuit.Set();
	}

	bool IsQuitRequested( void )
	{
		return ( m_RequestQuit.IsSignaled() );
	}

	bool Start( void )
	{
		return ( BeginThread( m_Name ) );
	}

	void SignalDone( void )
	{
		::PostMessage( *m_Output, WM_COMMAND, IDC_PROGRESS, (LPARAM)::GetDlgItem( *m_Output, IDC_PROGRESS ) );
	}

	void SetStatusText( const char* newStatus )
	{
		if ( m_LastStatus.same_with_case( newStatus ) )
		{
			return;
		}

		// edit ctl needs \r\n format
		m_LastStatus = newStatus;
		for ( size_t found = 0 ; ; )
		{
			found = m_LastStatus.find( '\n', found );
			if ( found != gpstring::npos )
			{
				m_LastStatus.insert( found, 1, '\r' );
				found += 2;
			}
			else
			{
				break;
			}
		}

		HWND statusText = ::GetDlgItem( *m_Output, IDC_STATUS );
		::SetWindowText( statusText, m_LastStatus );
	}

	virtual void OnRatioChanged( double newValue, const char* newStatus )
	{
		HWND progressBar  = ::GetDlgItem( *m_Output, IDC_PROGRESS );
		HWND progressText = ::GetDlgItem( *m_Output, IDC_PROGRESS_TEXT );

		int ipercent = (int)ceil(newValue * 100);

		::SendMessage( progressBar, PBM_SETPOS, ipercent, 0 );
		::SetWindowText( progressText, gpstringf( "%d%%", ipercent ) );

		SetStatusText( newStatus );
	}

	enum eRetCode
	{
		RC_SUCCESS,
		RC_FAILURE,
		RC_ABORTED_SUCCESS,
		RC_ABORTED_FAILURE,
	};

	virtual DWORD Execute( void )
	{
		// disable errors for a bit
		ReportSys::EnableErrorAssert( false );

		// set output to go to the log
		HWND logText = ::GetDlgItem( *m_Output, IDC_LOG );
		m_EditSink = new ReportSys::EditCtlSink( logText );
		gGlobalSink.AddSink( m_EditSink, true );

		// start timer
		double startTime = ::GetSystemSeconds();

		// do op
		eRetCode rc = OnExecute( startTime );

		// output status
		double endTime = ::GetSystemSeconds();
		int hours = 0, minutes = 0;
		float seconds = (float)(endTime - startTime);
		::ConvertTime( hours, minutes, seconds );
		gpgenericf(( "* Operation completed\n    CPU usage %d:%02d:%06.3f\n", hours, minutes, seconds ));

		// finish!
		Finish( "" );

		// update status
		bool success = (rc == RC_SUCCESS) || (rc == RC_ABORTED_SUCCESS);
		if ( (rc == RC_ABORTED_SUCCESS) || (rc == RC_ABORTED_FAILURE) )
		{
			gpwarning( "\n\n*** User aborted ***\n" );
			SetStatusText( "Aborted!" ); 
		}
		else
		{
			SetStatusText( success ? "Success!" : "Failure!" );
		}

		// done!
		SignalDone();
		return ( success );
	}

	virtual eRetCode OnExecute( double startTime ) = 0;
};

#define CHECK_EXIT() if ( IsQuitRequested() ) { return ( success ? RC_ABORTED_SUCCESS : RC_ABORTED_FAILURE ); }

/////////////////////////////////////////////////////////////////////////////
// class BuildWorkerThread

struct SrcFileSpec : Tank::Builder::FileSpec
{
	gpstring m_SrcPath;			// where on hard drive we pull file from
	DWORD    m_Size;			// raw size of file

	SrcFileSpec( const gpstring& fileName, const gpstring& folderName, const gpstring& outputPrefix )
		: m_SrcPath( fileName )
	{

		// remove leading path and prefix the output base dir
		m_FileName = outputPrefix + fileName.substr( folderName.length() );

		// get file infoz
		FileSys::FileFinder ffinder( fileName );
		if ( ffinder.Next() )
		{
			m_Size = ffinder.Get().nFileSizeLow;
			m_FileTime = ffinder.Get().ftLastWriteTime;
		}

		// get dir infoz
		FileSys::FileFinder pfinder( GetPath( false ) );
		if ( pfinder.Next() )
		{
			m_DirTime = pfinder.Get().ftLastWriteTime;
		}
	}

	void EnableCompression( bool enable )
	{
		m_DataFormat = enable ? Tank::DATAFORMAT_ZLIB : Tank::DATAFORMAT_RAW;
	}

	void AddFilter( DWORD filter )
	{
		m_Filters.push_back( filter );
	}

	gpstring GetPath( bool trailingSlash = true ) const
	{
		return ( FileSys::GetPathOnly( m_SrcPath.c_str(), trailingSlash ) );
	}
};

typedef std::list <SrcFileSpec> SrcFileList;

class BuildWorkerThread : public WorkerThread
{
	Dialog_Save_As_DSMod* m_BuildOptions;
	RatioSample*          m_VerifyRatioSample;

public:
	SET_INHERITED( BuildWorkerThread, WorkerThread );

	BuildWorkerThread( Dialog_Save_As_DSMod* buildOptions, Dialog_Build_Tank* output )
		: Inherited( "TankBuilder", output )
	{
		m_BuildOptions      = buildOptions;
		m_VerifyRatioSample = NULL;        
	}

	bool AdvanceVerify( void )
	{
		if ( m_VerifyRatioSample != NULL )
		{
			m_VerifyRatioSample->Advance();
		}
		return ( !IsQuitRequested() );
	}

	virtual eRetCode OnExecute( double startTime )
	{

	// Init output.

		// prep our source path
		gpstring sourceFolder = m_BuildOptions->m_SourceFolder;
		stringtool::AppendTrailingBackslash( sourceFolder );
		FileSys::CleanupPath( sourceFolder );

		// prep our output path
		gpstring outputPrefix = m_BuildOptions->m_OutputPrefix;
		stringtool::AppendTrailingBackslash( outputPrefix );
		FileSys::CleanupPath( outputPrefix );
		if ( outputPrefix.same_no_case( "\\" ) )
		{
			// root path like this is not necessary
			outputPrefix.clear();
		}

		// get filename
		gpstring tankFileName = m_BuildOptions->m_DstFileName;

		// get some other vars
		SrcFileList fileSpecs;
		DWORD totalSize = 0;
		bool success = true;

	// Init options.
	{
		RatioSample ratioSample( "Initializing", 0, 0.01 );
		gpgeneric( "* Initializing\n" );
		ReportSys::AutoIndent autoIndent( gGenericContext );
		CHECK_EXIT();

		// set tank builder options
		m_Builder.SetOptions			( Tank::Builder::OPTION_BACKUP_OLD, !!m_BuildOptions->m_UseBackup );
		m_Builder.SetOptions			( Tank::Builder::OPTION_USE_TEMP_FILE );
		m_Builder.SetCreatorId			( 'SE' );
		m_Builder.SetTitleText			( ::ToUnicode( m_BuildOptions->m_Title ) );
		m_Builder.SetAuthorText			( ::ToUnicode( m_BuildOptions->m_Author ) );
		m_Builder.SetCopyrightText		( ::ToUnicode( m_BuildOptions->m_Copyright ) );
		m_Builder.SetDescriptionText	( ::ToUnicode( m_BuildOptions->m_Description ) );

		// special override if we're GPG
		if ( HasAnyDungeonSiegeSpecialFile() )
		{
			m_Builder.SetCreatorId( CREATOR_GPG );
		}

		// print status
		if ( m_BuildOptions->m_UseBackup )  gpgeneric( "Option: use backup\n" );
		gpgenericf(( "OutputFolderPrefix: '%s'\n", outputPrefix.c_str() ));
		gpgenericf(( "Title: '%s'\n", (LPCTSTR)m_BuildOptions->m_Title ));
		gpgenericf(( "Author: '%s'\n", (LPCTSTR)m_BuildOptions->m_Author ));
		gpgenericf(( "Copyright: '%s'\n", (LPCTSTR)m_BuildOptions->m_Copyright ));
		gpgenericf(( "Description: '%s'\n", (LPCTSTR)m_BuildOptions->m_Description ));

		// configure version info
		m_Builder.SetTankProductVersion	( ::GetDungeonSiegeVersion() );

		// set version
		if ( !m_BuildOptions->m_MinVersion.IsEmpty() )
		{
			// see if it's straight version text or an EXE file
			gpversion minVersion;
			if ( !::FromString( m_BuildOptions->m_MinVersion, minVersion ) && !minVersion.InitFromResources( m_BuildOptions->m_MinVersion ) )
			{
				gperrorf(( "ERROR: invalid version format '%s', or file does not exist\n", (LPCTSTR)m_BuildOptions->m_MinVersion ));
				success = false;
			}

			// set it
			m_Builder.SetTankMinimumVersion( minVersion );
			gpgenericf(( "MinVersion: '%s'\n", ::ToString( minVersion, gpversion::MODE_COMPLETE ).c_str() ));
		}

		// add flags
		if ( m_BuildOptions->m_DevOnly )
		{
			m_Builder.AddTankFlags( Tank::TANKFLAG_NON_RETAIL );
			gpgeneric( "Option: non-retail\n" );
		}
		if ( m_BuildOptions->m_ProtectedContent )
		{
			m_Builder.AddTankFlags( Tank::TANKFLAG_PROTECTED_CONTENT );
			gpgeneric( "Option: protected content\n" );
		}

		// set priority
		TankConstants::ePriority priority;
		if ( TankConstants::FromString( m_BuildOptions->m_Priority, priority ) )
		{
			m_Builder.SetTankPriority( priority );
			gpgenericf(( "Priority: %s\n", TankConstants::ToString( priority ).c_str() ));
		}
		else
		{
			gperrorf(( "ERROR: invalid priority of '%s' specified.\n" ));
			success = false;
		}

		// check source path existence
		if ( FileSys::DoesPathExist( sourceFolder ) )
		{
			gpgenericf(( "SourceFolder: '%s'\n", sourceFolder.c_str() ));

			if ( m_BuildOptions->m_LqdInclude )
			{
				gpgeneric( "Option: include LQD files\n" );
			}
			if ( m_BuildOptions->m_LqdCompile )
			{
				gpgeneric( "Option: recompile LQD files\n" );
			}
			if ( m_BuildOptions->m_LqdDelete )
			{
				gpgeneric( "Option: delete existing LQD files in preprocessing\n" );
			}

			// check to make sure it's writable if generating LQD
			if ( (m_BuildOptions->m_LqdCompile || m_BuildOptions->m_LqdDelete) && !FileSys::IsDirWritable( sourceFolder ) )
			{
				gperrorf(( "ERROR: lqd rebuilding is enabled, yet unable to write to source folder '%s', error is '%s' (0x%08X)\n",
						   sourceFolder.c_str(),
						   stringtool::GetLastErrorText().c_str(),
						   ::GetLastError() ));
				success = false;
			}
		}
		else
		{
			gperrorf(( "ERROR: unable to access source folder '%s', error is '%s' (0x%08X)\n",
					   sourceFolder.c_str(),
					   stringtool::GetLastErrorText().c_str(),
					   ::GetLastError() ));
			success = false;
		}

		// try to start tank
		if ( m_Builder.CreateTankAlways( ::ToUnicode( tankFileName ) ) )
		{
			gpgenericf(( "DestFile: '%s' successfully created\n", tankFileName.c_str() ));
		}
		else
		{
			gperrorf(( "ERROR: unable to create destination file '%s', error is '%s' (0x%08X)\n",
					   tankFileName.c_str(),
					   stringtool::GetLastErrorText().c_str(),
					   ::GetLastError() ));
			success = false;
		}

		// check for failure
		if ( !success )
		{
			gperror( "\n*** Failure during initialization ***\n\nCheck parameters and try building again.\n" );
			return ( RC_FAILURE );
		}
	}

	// Prep.
	{
		RatioSample ratioSample( "Scanning", 0, 0.15 );
		CHECK_EXIT();

		// rebuilding lqd files?
		{
			RatioSample ratioSample( "Processing LQD", 0, 0.25 );
			gpgeneric( "* Processing LQD\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			if ( m_BuildOptions->m_LqdCompile || m_BuildOptions->m_LqdDelete )
			{
				BinaryFuelDB* bdb = gFuelSys.AddBinaryDb( "recompile_bdb" );
				bdb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE, sourceFolder, sourceFolder );
				bdb->SetOptions( FuelDB::OPTION_NO_READ_DEV_BLOCKS, !m_BuildOptions->m_DevOnly );

				if ( m_BuildOptions->m_LqdCompile )
				{
					bdb->RecompileAll( !!m_BuildOptions->m_LqdDelete );
				}
				else
				{
					bdb->DeleteAllLqdFiles();
				}

				gFuelSys.Remove( bdb );
			}

		}

		// scan paths
		std::list <gpwstring> foundFiles;
		{
			RatioSample ratioSample( "Scanning files", 0, 0.7 );
			gpgeneric( "* Scanning files\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			FileSys::FindFilesFromSpec( ::ToUnicode( sourceFolder + "*\\" ), foundFiles, false );
			gpgenericf(( "Found %d files\n", foundFiles.size() ));
		}

		// remove stuff we don't want to pack up
		{
			RatioSample ratioSample( "Spackling bits", foundFiles.size() );
			gpgeneric( "* Spackling bits\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			int skipCount = 0, compressCount = 0, imgFilterCount = 0;
			for ( std::list <gpwstring>::iterator i = foundFiles.begin() ; i != foundFiles.end() ; ++i )
			{
				CHECK_EXIT();

				SrcFileSpec fileSpec( ::ToAnsi( *i ), sourceFolder, outputPrefix );
				ratioSample.Advance( fileSpec.GetPath() );

				// compression enabled by default option
				fileSpec.EnableCompression( !!m_BuildOptions->m_CompressFiles );
				if ( m_BuildOptions->m_CompressFiles )
				{
					++compressCount;
				}

				// get extension
				const char* ext = FileSys::GetExtension( fileSpec.m_SrcPath );
				if ( ext == NULL )
				{
					ext = "";
				}

				// check files to skip
				if (   (!m_BuildOptions->m_LqdInclude && stringtool::IsDosWildcardMatch( "lqd*", ext ))
					|| ::same_no_case( ext, "bak", 3 )
					|| ::same_no_case( ext, "tmp", 3 ) )
				{
					++skipCount;
					continue;
				}

				// compression options
				if (   ::same_no_case( ext, "wav" )
					|| ::same_no_case( ext, "mp3" )
					|| ::same_no_case( ext, "bik" )
					|| ::same_no_case( ext, "avi" ) )
				{
					fileSpec.EnableCompression( false );
					--compressCount;
				}

				// filter type
				if ( m_BuildOptions->m_ReprocessFiles )
				{
					if ( FileSys::IFileMgr::IsAliasedFrom( fileSpec.m_SrcPath, ".%img%" ) )
					{
						fileSpec.AddFilter( ImgFilter::FILTER_RAW_MIPMAPS );
						++imgFilterCount;
					}
				}

				// take it
				totalSize += fileSpec.m_Size;
				fileSpecs.push_back( fileSpec );
			}

			gpgenericf((
					"Total found: %d\n"
					"Skipped: %d\n"
					"To be included: %d (%d bytes)\n"
					"Compressed: %d\n"
					"Mipped: %d\n",
					foundFiles.size(),
					skipCount,
					fileSpecs.size(), totalSize,
					compressCount,
					imgFilterCount ));
		}
	}

	// Build.
	{
		RatioSample ratioSample( "Building resource file", totalSize, m_BuildOptions->m_VerifyTank ? 0.7 : 0.83 );
		CHECK_EXIT();

		{
			RatioSample ratioSample( "Adding files", totalSize, 0.95 );
			gpgeneric( "* Adding files\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			DWORD bytesRead = 0, filesWritten = 0, bytesWritten = 0;
			for ( SrcFileList::const_iterator i = fileSpecs.begin() ; i != fileSpecs.end() ; ++i )
			{
				CHECK_EXIT();
				ratioSample.Advance( i->m_Size, i->GetPath( false ) + "\n      " + FileSys::GetFileName( i->m_SrcPath ) );

				FileSys::MemoryFile file;
				bool localSuccess = false;
				DWORD localSize = 0, oldTankSize = m_Builder.GetTankSize();

				try
				{
					bool wasEmpty = false;
					if ( file.Create( i->m_SrcPath, &wasEmpty ) )
					{
						localSuccess = m_Builder.AddFile( *i, const_mem_ptr( file.GetData(), file.GetSize() ) );
						localSize = file.GetSize();
					}
					else if ( wasEmpty )
					{
						// zero-length files are ok
						localSuccess = m_Builder.AddFile( *i, const_mem_ptr() );
					}
				}
				catch ( ... )
				{
					// must be read error
					::SetLastError( ERROR_READ_FAULT );
				}

				if ( localSuccess )
				{
					bytesRead += localSize;
					bytesWritten += m_Builder.GetTankSize() - oldTankSize;
					++filesWritten;
				}
				else
				{
					success = false;
					gperrorf(( "ERROR: unable to open/access file '%s', error is '%s' (0x%08X)\n",
							   i->m_SrcPath.c_str(),
							   stringtool::GetLastErrorText().c_str(),
							   ::GetLastError() ));
				}
			}

			double deltaTime = ::GetSystemSeconds() - startTime;
			int totalRate = 0;
			if ( deltaTime > 0 )
			{
				totalRate = int( (bytesWritten / 1024) / deltaTime );
			}

			gpgenericf((
					"Done!\n"
					"Bytes written: %d (includes padding)\n"
					"Bytes read: %d\n"
					"Files written: %d at %dK bytes/sec\n",
					bytesWritten,
					bytesRead,
					filesWritten, totalRate ));

			if ( (bytesRead != bytesWritten) && (bytesRead > 0) )
			{
				gpgenericf(( "Overall compression: %2d%%\n", 100 - (int)(bytesWritten * 100.0f / bytesRead) ));
			}

			const zlib::PerfCounter& zlibCounter = zlib::GetPerfCounters();
			if ( (zlibCounter.m_BytesDeflatedIn > 0) && (zlibCounter.m_SecondsDeflating > 0) )
			{
				int ratio = 100 - int( zlibCounter.m_BytesDeflatedOut * 100.0f / zlibCounter.m_BytesDeflatedIn );
				int rate = int( (zlibCounter.m_BytesDeflatedIn / 1024) / zlibCounter.m_SecondsDeflating );
				gpgenericf(( "Zlib avg compression: %2d%% at %dK bytes/sec\n", ratio, rate ));
			}

			const lzo::PerfCounter& lzoCounter = lzo::GetPerfCounters();
			if ( (lzoCounter.m_BytesDeflatedIn > 0) && (lzoCounter.m_SecondsDeflating > 0) )
			{
				int ratio = 100 - int( lzoCounter.m_BytesDeflatedOut * 100.0f / lzoCounter.m_BytesDeflatedIn );
				int rate = int( (lzoCounter.m_BytesDeflatedIn / 1024) / lzoCounter.m_SecondsDeflating );
				gpgenericf(( "Lzo avg compression = %2d%% at %dK bytes/sec\n", ratio, rate ));
			}
		}

		if ( success )
		{
			RatioSample ratioSample( "Committing" );
			gpgeneric( "* Committing\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			if ( !m_Builder.CommitTank() )
			{
				gperrorf(( "Unable to commit, error is '%s' (0x%08X)\n",
						   stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				success = false;
			}
		}
	}

	// Dump.
	{
		if ( success && m_BuildOptions->m_GenerateDump )
		{
			RatioSample ratioSample( "Generating dump file", 0, 0.01 );
			gpgeneric( "* Generating dump file\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			Tank::TankFile tankFile;
			if ( tankFile.Open( tankFileName ) )
			{
				gpstring outFile = tankFileName + ".txt";

				ReportSys::FileSink sink;
				if ( sink.Open( outFile, true ) )
				{
					ReportSys::LocalContext ctx( &sink, false );
					tankFile.Dump( ctx );

					gpgenericf(( "Dump file created at '%s'\n", outFile.c_str() ));
				}
				else
				{
					// $ don't set success=false, this is not a critical failure

					gpwarningf(( "Unable to create dump file '%s', error is '%s' (0x%08X)\n",
								 outFile.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				}
			}
			else
			{
				gperrorf(( "Unable to open resource file '%s', error is '%s' (0x%08X)\n",
						   tankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				success = false;
			}
		}
	}

	// Verify.
	{
		if ( success && m_BuildOptions->m_VerifyTank )
		{
			RatioSample ratioSample( "Verifying resource file", fileSpecs.size() );
			m_VerifyRatioSample = &ratioSample;
			gpgeneric( "* Verifying resource file\n" );
			ReportSys::AutoIndent autoIndent( gGenericContext );
			CHECK_EXIT();

			FileSys::TankFileMgr tankFileMgr;
			if ( tankFileMgr.OpenTank( tankFileName ) )
			{
				if ( tankFileMgr.VerifyAll( makeFunctor( *this, &BuildWorkerThread::AdvanceVerify ) ) )
				{
					gpgeneric( "Resource file verified ok!!\n" );
				}
				else
				{
					gpgeneric( "Resource file had CRC errors!!\n" );
				}
			}
			else
			{
				gperrorf(( "Unable to open resource file '%s', error is '%s' (0x%08X)\n",
						   tankFileName.c_str(), stringtool::GetLastErrorText().c_str(), ::GetLastError() ));
				success = false;
			}
			m_VerifyRatioSample = NULL;
		}
	}

		return ( success ? RC_SUCCESS : RC_FAILURE );
	}
};

/////////////////////////////////////////////////////////////////////////////
// class ExtractWorkerThread

class ExtractWorkerThread : public WorkerThread
{
	Dialog_Extract_Tank* m_ExtractOptions;

public:
	SET_INHERITED( ExtractWorkerThread, WorkerThread );

	ExtractWorkerThread( Dialog_Extract_Tank* extractOptions, Dialog_Build_Tank* output )
		: Inherited( "TankExtractor", output )
	{
		m_ExtractOptions = extractOptions;
	}

	virtual eRetCode OnExecute( double startTime )
	{
		// prep our dest path
		gpstring dstFolder = m_ExtractOptions->m_DefaultDstFolder
				? gConfig.ResolvePathVars( "%out%" )
				: gpstring( m_ExtractOptions->m_DstFolder );
		stringtool::AppendTrailingBackslash( dstFolder );
		FileSys::CleanupPath( dstFolder );

		// alloc some vars
		FileSys::TankFileMgr fileMgr;
		int fileCount = 0, byteCount = 0;
		bool success = true;

	// Init options.
	{
		gpgeneric( "* Initializing\n" );
		ReportSys::AutoIndent autoIndent( gGenericContext );

		// build a tank manager and try to open that particular tank file
		if ( !fileMgr.OpenTank( m_ExtractOptions->m_SourceFile ) )
		{
			gperrorf(( "ERROR: unable to access source file '%s', error is '%s' (0x%08X)\n",
					   (LPCTSTR)m_ExtractOptions->m_SourceFile,
					   stringtool::GetLastErrorText().c_str(),
					   ::GetLastError() ));
			return ( RC_FAILURE );
		}
		gpgenericf(( "Source file '%s' successfully opened\n", (LPCTSTR)m_ExtractOptions->m_SourceFile ));

		// must be writable if we're going to store files there
		if ( !FileSys::IsDirWritable( dstFolder ) )
		{
			gperrorf(( "ERROR: destination folder '%s' is not writable, error is '%s' (0x%08X)\n",
					   dstFolder.c_str(),
					   stringtool::GetLastErrorText().c_str(),
					   ::GetLastError() ));
			return ( RC_FAILURE );
		}

		// get total counts
		FileSys::RecursiveFinder finder( "", -1, &fileMgr );
		FileSys::FindData findData;
		while ( finder.GetNext( findData ) )
		{
			if ( !m_ExtractOptions->m_ExtractLqdFiles )
			{
				const char* ext = FileSys::GetExtension( findData.m_Name );
				if ( (ext != NULL) && stringtool::IsDosWildcardMatch( "lqd*", ext ) )
				{
					continue;
				}
			}

			++fileCount;
			byteCount += findData.m_Size;
		}
		gpgenericf(( "Found %d files for %d bytes total to be written\n", fileCount, byteCount ));
	}

	// Extract.
	{
		RatioSample ratioSample( "Extracting", byteCount );
		gpgeneric( "* Extracting files\n" );
		ReportSys::AutoIndent autoIndent( gGenericContext );
		CHECK_EXIT();

		// now do the extraction
		int filesWritten = 0, bytesWritten = 0;
		FileSys::RecursiveFinder finder( "", -1, &fileMgr );
		FileSys::FindData findData;
		while ( finder.GetNext( findData, true ) )
		{
			if ( !m_ExtractOptions->m_ExtractLqdFiles )
			{
				const char* ext = FileSys::GetExtension( findData.m_Name );
				if ( (ext != NULL) && stringtool::IsDosWildcardMatch( "lqd*", ext ) )
				{
					continue;
				}
			}

			CHECK_EXIT();
			ratioSample.Advance( findData.m_Size, FileSys::GetPathOnly( findData.m_Name ) + "\n      " + FileSys::GetFileName( findData.m_Name ) );

			// get some vars
			gpstring fileName = dstFolder + findData.m_Name;
			gpstring filePath = FileSys::GetPathOnly( fileName );

			// make sure path exists
			if ( !FileSys::CreateFullDirectory( filePath ) )
			{
				gperrorf(( "Unable to create output path '%s', error is '%s' (0x%08X)\n",
						   filePath.c_str(),
						   stringtool::GetLastErrorText().c_str(),
						   ::GetLastError() ));
			}

			// ok map source file
			FileSys::FileHandle inFile = fileMgr.Open( findData.m_Name );
			FileSys::MemHandle  inMem  = fileMgr.Map ( inFile );
			if ( !inFile || !inMem )
			{
				gperrorf(( "Weird...unable to open file '%s' from resource '%s', error is '%s' (0x%08X)\n",
						   findData.m_Name.c_str(),
						   (LPCTSTR)m_ExtractOptions->m_SourceFile,
						   stringtool::GetLastErrorText().c_str(),
						   ::GetLastError() ));
				if ( inFile )
				{
					fileMgr.Close( inFile );
				}
				continue;
			}

			// create a file
			FileSys::File file;
			bool localSuccess = false, skip = false;
			if ( m_ExtractOptions->m_OverwriteFiles )
			{
				localSuccess = file.CreateAlways( fileName );
			}
			else
			{
				localSuccess = file.CreateNew( fileName );
				if ( !success && (::GetLastError() == ERROR_FILE_EXISTS) )
				{
					gpwarningf(( "File already exists, skipping extraction: '%s'\n", fileName.c_str() ));
					skip = true;
				}
			}

			// create mappings
			FileSys::FileMap map;
			FileSys::MapView view;
			if ( localSuccess && findData.m_Size )
			{
				if (   !map .Create( file, FileSys::FileMap::PROTECT_READ_WRITE, findData.m_Size ) 
					|| !view.Create( map , FileSys::MapView::ACCESS_READ_WRITE ) )
				{
					localSuccess = false;
				}
			}

			// error?
			if ( localSuccess )
			{
				// do it
				if ( findData.m_Size > 0 )
				{
					::memcpy( view.GetData(), fileMgr.GetData( inMem ), findData.m_Size );
				}
			}
			else if ( !skip )
			{
				success = false;
				gperrorf(( "Unable to create/map output file '%s', error is '%s' (0x%08X)\n",
						   fileName.c_str(), 
						   stringtool::GetLastErrorText().c_str(),
						   ::GetLastError() ));
			}

			// cleanup
			fileMgr.Close( inMem );
			fileMgr.Close( inFile );

			// advance
			++filesWritten;
			bytesWritten += findData.m_Size;
		}

		double deltaTime = ::GetSystemSeconds() - startTime;
		int totalRate = 0;
		if ( deltaTime > 0 )
		{
			totalRate = int( (bytesWritten / 1024) / deltaTime );
		}

		gpgenericf((
				"Done!\n"
				"Bytes written: %d\n"
				"Files written: %d at %dK bytes/sec\n",
				bytesWritten,
				filesWritten, totalRate ));
	}

		return ( success ? RC_SUCCESS : RC_FAILURE );
	}
};

/////////////////////////////////////////////////////////////////////////////
// Dialog_Build_Tank dialog

Dialog_Build_Tank::Dialog_Build_Tank(Dialog_Save_As_DSMod* pParent /*=NULL*/)
	: CDialog(Dialog_Build_Tank::IDD, pParent)
{
	m_WorkerThread	= new BuildWorkerThread( pParent, this );
	m_State			= STATE_RUNNING;

	//{{AFX_DATA_INIT(Dialog_Build_Tank)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

Dialog_Build_Tank::Dialog_Build_Tank(Dialog_Extract_Tank* pParent /*=NULL*/)
	: CDialog(Dialog_Build_Tank::IDD, pParent)
{
	m_WorkerThread	= new ExtractWorkerThread( pParent, this );
	m_State			= STATE_RUNNING;

	//{{AFX_DATA_INIT(Dialog_Build_Tank)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

Dialog_Build_Tank::~Dialog_Build_Tank( void )
{
	Delete( m_WorkerThread );
}

void Dialog_Build_Tank::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Build_Tank)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Build_Tank, CDialog)
	//{{AFX_MSG_MAP(Dialog_Build_Tank)
	ON_COMMAND( IDC_PROGRESS, OnDone )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void Dialog_Build_Tank::SetState( eState state )
{
	switch( state )
	{
		case STATE_RUNNING:
		{
			GetDlgItem( IDOK )->ShowWindow( SW_HIDE );
			GetDlgItem( IDCANCEL )->ShowWindow( SW_SHOW );
		}
		break;

		case STATE_CANCELLING:
		{
			SetDlgItemText( IDCANCEL, "Cancelling..." );
			GetDlgItem( IDCANCEL )->EnableWindow( false );
		}
		break;

		case STATE_DONE:
		{
			if ( m_State == STATE_CANCELLING )
			{
				// make IDCANCEL pretend to be "close" so we get proper return code
				SetDlgItemText( IDCANCEL, "Cl&ose" );
				GetDlgItem( IDCANCEL )->EnableWindow();
			}
			else
			{
				GetDlgItem( IDOK )->ShowWindow( SW_SHOW );
				GetDlgItem( IDCANCEL )->ShowWindow( SW_HIDE );
			}
		}
		break;
	}

	m_State = state;
}

/////////////////////////////////////////////////////////////////////////////
// Dialog_Build_Tank message handlers

BOOL Dialog_Build_Tank::OnInitDialog()
{
	CDialog::OnInitDialog();

	// make sure filters are registered
	ImgFilter::Register();

	// reset stats
	zlib::ResetPerfCounters();
	lzo::ResetPerfCounters();

	// start the builder thread up
	m_WorkerThread->Start();

	// update buttons
	SetState( STATE_RUNNING );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Build_Tank::OnCancel()
{
	if ( m_State == STATE_RUNNING )
	{
		if ( MessageBox( "Are you sure you want to cancel?", "Cancel Request", MB_YESNO ) == IDYES )
		{
			if ( m_WorkerThread->IsThreadRunning() )
			{
				SetState( STATE_CANCELLING );
				m_WorkerThread->RequestQuit();
			}
			else
			{
				CDialog::OnCancel();
			}
		}
	}
	else if ( m_State == STATE_DONE )
	{
		CDialog::OnCancel();
	}
}

void Dialog_Build_Tank::OnDone()
{
	// wait for it to finish completely
	m_WorkerThread->WaitForExit();

	// treat failed return code as cancel
	if ( !m_WorkerThread->GetLastReturn() && !m_WorkerThread->IsQuitRequested() )
	{
		SetState( STATE_CANCELLING );
	}

	SetState( STATE_DONE );
}
