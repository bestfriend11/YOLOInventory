//////////////////////////////////////////////////////////////////////////////
//
// File     :  Main.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains classes and functions to support main().
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __MAIN_H
#define __MAIN_H

//////////////////////////////////////////////////////////////////////////////

#include "AppModule.h"
#include "FuelDb.h"
#include "MasterFileMgr.h"
#include "PathFileMgr.h"
#include "ReportSys.h"
#include "Spec.h"
#include "StringTool.h"
#include "TankFileMgr.h"

//////////////////////////////////////////////////////////////////////////////

/*
	The builder works like this:

		Read in fuel, compile the blocks into specs.

		Build selection set from the inclusion/exclusion list. This is the pool
		of files from which we are building. Fetch all filenames and size info,
		which we can use later for time estimates. Process the inclusion list,
		and as we go, remove files and directories by testing against the
		exclusion list.

		Run through all files, testing each against the modifier patterns to set
		up filters, scanners, compression, etc.

		Read in any journal file to set the order of the files for tank storage.

		Iterate through each file

			- Open, map data

			- Add to the tank

		NOTE: all filenames are always converted to lowercase when they come in
		so we can do fast case sensitive compares.

	Specs:

		Each spec can be a list of comma-delimited specs. It is treated as if
		each spec was on its own line.

		Specs are processed in the same order they appear in the gas file. This
		is essentially the priority/override system. Later specs will override
		earlier ones.

		Each spec is composed of a path pattern and a file pattern, each of
		which is optional. We detect the end of a path pattern by the last
		slash. The presence of a trailing slash but no file characters means
		that it is exclusively a path, and a * file pattern is implied.

		Forward and back slashes are considered the same in this system, except
		for \\computername\share style UNC paths for the root path. Multiple
		sequential slashes are compacted.

		Path and file patterns may contain wildcards. Path patterns must be
		relative from the root (and so cannot contain :). File patterns may not
		contain path characters like /, \, and :.

		Any leading \ will be ignored.

	Root:

		The root path must be a pathname, and may end in a + if it should be
		recursive. The scanner will automatically add all files if there is no
		include* in the block.
*/

//////////////////////////////////////////////////////////////////////////////
// class StatusSink declaration

class StatusSink : public ReportSys::FileSink, public Singleton <StatusSink>
{
public:
	SET_INHERITED( StatusSink, ReportSys::FileSink );

	StatusSink( void )
		: Inherited( ::GetStdHandle( STD_OUTPUT_HANDLE ) )
	{
		::ZeroObject( m_BeginInfo );
		::ZeroObject( m_EndInfo );
		m_Column = 0;
		m_RowCount = 0;
	}

   ~StatusSink( void )
		{  }

	virtual void OnBeginReport( const ReportSys::Context* context )
	{
		if ( context->GetReportLevel() == 0 )
		{
			CONSOLE_CURSOR_INFO info;
			::GetConsoleCursorInfo( GetFile(), &info );
			info.bVisible = FALSE;
			::SetConsoleCursorInfo( GetFile(), &info );

			::GetConsoleScreenBufferInfo( GetFile(), &m_BeginInfo );
			if ( m_EndInfo.dwSize.X == 0 )
			{
				::GetConsoleScreenBufferInfo( GetFile(), &m_EndInfo );
			}
			ClearRegion();
		}

		Inherited::OnBeginReport( context );
	}

	virtual void OnEndReport( const ReportSys::Context* context )
	{
		if ( context->GetReportLevel() == 1 )
		{
			::GetConsoleScreenBufferInfo( GetFile(), &m_EndInfo );
			m_BeginInfo.dwCursorPosition.X = m_EndInfo.dwCursorPosition.X;
			m_BeginInfo.dwCursorPosition.Y = (short)(m_EndInfo.dwCursorPosition.Y - m_RowCount);
			::SetConsoleCursorPosition( GetFile(), m_BeginInfo.dwCursorPosition );
		}

		Inherited::OnEndReport( context );
	}

	virtual bool OutputString( const ReportSys::Context* context, const char* text, int len, bool startLine, bool endLine )
	{
		m_Column += len;
		if ( m_Column >= m_BeginInfo.dwSize.X )
		{
			m_RowCount += m_Column / m_BeginInfo.dwSize.X;
			m_Column %= m_BeginInfo.dwSize.X;
		}

		if ( endLine )
		{
			m_Column = 0;
			++m_RowCount;
		}

		return ( Inherited::OutputString( context, text, len, startLine, endLine ) );
	}

	void PrepForOtherOutput( void )
	{
		ClearRegion();

		::ZeroObject( m_BeginInfo );
		::ZeroObject( m_EndInfo );

		CONSOLE_CURSOR_INFO info;
		::GetConsoleCursorInfo( GetFile(), &info );
		info.bVisible = TRUE;
		::SetConsoleCursorInfo( GetFile(), &info );
	}

	void ClearRegion( void )
	{
		if ( m_BeginInfo.dwSize.X != 0 )
		{
			int clear = (m_BeginInfo.dwSize.X * (m_EndInfo.dwCursorPosition.Y - m_BeginInfo.dwCursorPosition.Y)) + m_EndInfo.dwCursorPosition.X;
			if ( clear > 0 )
			{
				::SetConsoleCursorPosition( GetFile(), m_BeginInfo.dwCursorPosition );

				m_Filler.resize( clear, ' ' );
				GetFile().Write( &*m_Filler.begin(), clear );

				::SetConsoleCursorPosition( GetFile(), m_BeginInfo.dwCursorPosition );

				m_Column = m_BeginInfo.dwCursorPosition.X;
				m_RowCount = 0;
			}
		}
	}

private:
	HANDLE m_Console;
	CONSOLE_SCREEN_BUFFER_INFO m_BeginInfo;
	CONSOLE_SCREEN_BUFFER_INFO m_EndInfo;
	int m_Column, m_RowCount;
	stdx::fast_vector <char> m_Filler;

	SET_NO_COPYING( StatusSink );
};

#define gStatusSink StatusSink::GetSingleton()

//////////////////////////////////////////////////////////////////////////////
// class BuilderApp declaration

class BuilderApp : public AppModule, public Singleton <BuilderApp>
{
public:
	SET_INHERITED( BuilderApp, AppModule );

	BuilderApp( void );
	virtual ~BuilderApp( void );

	ReportSys::Context& GetStatusContext( void )  {  return ( *m_StatusContext );  }

	typedef stringtool::CommandLineExtractor::TokenColl TokenColl;

	bool BeginReportStatus( bool animateStatus = false );
	void EndReportStatus  ( bool animateStatus = true );
	void AnimateStatus    ( void );

private:

// Methods.

	virtual bool OnInitCore    ( void );
	virtual bool OnInitServices( void );
	virtual bool OnExecute     ( void );
	virtual bool OnShutdown    ( void );

	bool EasyBuildTank( const gpwstring& tankFileName, const TokenColl& tokens );

public:

	enum eProcess
	{
		PROCESS_EXTRACT,
		PROCESS_FIND,
		PROCESS_VERIFY,
	};

	static bool ProcessFiles( const gpwstring& tankFileName, const TokenColl& tokens, eProcess process, const gpstring& outDir );
	static bool ProcessFiles( FileSys::TankFileMgr& tankFileMgr, const gpstring& path, const gpstring& fileSpec, bool recursive, eProcess process );
	static bool ExtractFiles ( const gpwstring& tankFileName, const TokenColl& tokens, const gpstring& outDir = gpstring::EMPTY )  {  return ( ProcessFiles( tankFileName, tokens, PROCESS_EXTRACT, outDir ) );  }
	static bool FindFiles    ( const gpwstring& tankFileName, const TokenColl& tokens, const gpstring& outDir = gpstring::EMPTY )  {  return ( ProcessFiles( tankFileName, tokens, PROCESS_FIND, outDir ) );  }
	static bool VerifyFiles  ( const gpwstring& tankFileName, const TokenColl& tokens, const gpstring& outDir = gpstring::EMPTY )  {  return ( ProcessFiles( tankFileName, tokens, PROCESS_VERIFY, outDir ) );  }
	static void OutputRatios ( void );

private:

// Data.

	// owned systems
	std::auto_ptr <FileSys::PathFileMgr>	m_PathFileMgr;
	std::auto_ptr <FuelSys>					m_FuelSys;
	std::auto_ptr <StatusSink>				m_StatusSink;
	std::auto_ptr <ReportSys::Context>		m_StatusContext;
	stringtool::CommandLineExtractor		m_CommandLine;

	SET_NO_COPYING( BuilderApp );
};

#define gBuilderApp Singleton <BuilderApp>::GetSingleton()

#define gStatusContext gBuilderApp.GetStatusContext()
#define gpstatus( msg ) MACRO_REPORT( &gStatusContext , msg )
#define gpstatusf( msg ) MACRO_REPORTF( &gStatusContext, msg )

//////////////////////////////////////////////////////////////////////////////
// class CmdProcessor declaration

class CmdProcessor
{
public:
	SET_NO_INHERITED( CmdProcessor );

	CmdProcessor( const gpstring& initialTank = gpstring::EMPTY );
   ~CmdProcessor( void );

	bool Execute( void );

private:
	bool Open       ( gpstring spec );
	bool Close      ( void );
	bool List       ( void );
	bool IsOpen     ( void ) const;
	bool BuildPacker( int bitsNeeded );

	typedef stdx::fast_vector <gpstring> StringColl;
	typedef stdx::fast_vector <BYTE> Buffer;

	StringColl                 m_TankNameColl;
	StringColl                 m_PathNameColl;
	my FileSys::MultiFileMgr*  m_MasterFileMgr;
	FileSys::CmdProcessor      m_Processor;
	Buffer                     m_BitBuffer;
	my FuBi::BitPacker*        m_BitPacker;

	SET_NO_COPYING( CmdProcessor );
};

//////////////////////////////////////////////////////////////////////////////

#endif  // __MAIN_H

//////////////////////////////////////////////////////////////////////////////
