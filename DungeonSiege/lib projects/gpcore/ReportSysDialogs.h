//////////////////////////////////////////////////////////////////////////////
//
// File     :  ReportSysDialogs.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains Win32 GDI support for dialog display and manipulation
//             of ReportSys functionality.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __REPORTSYSDIALOGS_H
#define __REPORTSYSDIALOGS_H

//////////////////////////////////////////////////////////////////////////////

#include "AtlX.h"
#include "ReportSys.h"

namespace ReportSys  {  // begin of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
// class MgrDlg declaration

class MgrDlg : public atlx::DialogImpl <MgrDlg>
{
public:
	SET_INHERITED( MgrDlg, atlx::DialogImpl <MgrDlg> );

	MgrDlg( HWND parent = NULL );
	virtual ~MgrDlg( void );

private:
	typedef atlx::ResizableImpl <ThisType> Resizer;

	BEGIN_MSG_MAP( ThisType )
		CHAIN_MSG_MAP_MEMBER( m_Resizer )
		MESSAGE_CRACKER( WM_INITDIALOG, OnInitDialog )
	END_MSG_MAP()

	BOOL OnInitDialog( HWND focusctl, LPARAM init, BOOL& handled );

	// state
	Resizer m_Resizer;				// auto resizer

	SET_NO_COPYING( MgrDlg );
};

//////////////////////////////////////////////////////////////////////////////
// class EditCtlSink declaration

class EditCtlSink : public StreamSink
{
public:
	SET_INHERITED( EditCtlSink, StreamSink );

	EditCtlSink( atlx::Edit editCtl = NULL );
	virtual ~EditCtlSink( void );

	// setup
	void SetEditCtl ( atlx::Edit editCtl );
	void ClearText  ( void );
	void AppendText ( const char* text );

	// overrides
	virtual bool IsEnabled    ( void ) const					{  return ( Inherited::IsEnabled() && m_EditCtl.IsWindow() && m_EditCtl.IsWindowEnabled() );  }
	virtual void OnBeginReport( const Context* context );
	virtual void OnEndReport  ( const Context* context );
	virtual bool OutputString ( const Context* context,
								const char* text, int len,
								bool startLine, bool endLine );

private:
	atlx::Edit m_EditCtl;
	int        m_LimitText;
	int        m_EndChopPos;

	SET_NO_COPYING( EditCtlSink );
};

//////////////////////////////////////////////////////////////////////////////
// class SimpleDlgSink declaration

// note: to keep track of who calls what - each function is tagged with who
//       normally will call it - the primary thread (PRI) or the auxiliary (AUX
//       = AsyncWnd message pump) thread. anything that queries or modifies data
//       from AUX must be locked for all AUX usage and all mod usage in PRI.

class SimpleDlgSink;
typedef atlx::AsyncWnd <SimpleDlgSink, atlx::DialogImpl <SimpleDlgSink> > SimpleDlgSinkBase;

class SimpleDlgSink : public SimpleDlgSinkBase, public MultiSink
{
public:
	SET_INHERITED2( SimpleDlgSink, SimpleDlgSinkBase, MultiSink );
	typedef Inherited1 DlgInherited;
	typedef Inherited2 SinkInherited;

	// ctor/dtor
	/*PRI*/ SimpleDlgSink( HWND parent = NULL, const char* name = NULL );
	/*PRI*/ virtual ~SimpleDlgSink( void );

	// overrides
	/*PRI*/ virtual bool IsEnabled   ( void ) const				{  return ( SinkInherited::IsEnabled() && IsWindow() && IsWindowEnabled() );  }
	/*PRI*/ virtual void SetName     ( const char* name );

	// factory
	/*PRI*/ static Sink* CreateSink( const char* name, const char* type, const char* sinkName );

	// $$$ support auto-hiding after xx sec of inactivity

private:
	/*PRI*/ virtual bool Create( const HWND& parent, const char* threadName = NULL );
	/*AUX*/ virtual bool ThreadCreate( const HWND& parent );
	/*AUX*/ void UpdateWindowName( void );

	typedef atlx::ResizableImpl     <ThisType> Resizer;
	typedef atlx::PositionSaverImpl <ThisType> PositionSaver;

	BEGIN_MSG_MAP( ThisType )
		MESSAGE_CRACKER( WM_INITDIALOG, OnPreInitDialog )
		CHAIN_MSG_MAP_MEMBER( m_Resizer )
		CHAIN_MSG_MAP_MEMBER( m_PositionSaver )
		MESSAGE_CRACKER( WM_DESTROY, OnDestroy )
		CHAIN_MSG_MAP( DlgInherited )
		MESSAGE_CRACKER( WM_INITDIALOG, OnInitDialog )
	END_MSG_MAP()

	/*AUX*/ BOOL OnPreInitDialog( HWND focusctl, LPARAM init, BOOL& handled );
	/*AUX*/ BOOL OnInitDialog( HWND focusctl, LPARAM init, BOOL& handled );
	/*AUX*/ void OnDestroy( BOOL& handled );

	// state
	EditCtlSink   m_EditSink;			// where the output goes
	winx::Font    m_EditFont;			// font to use for edit control
	Resizer       m_Resizer;			// auto resizer
	PositionSaver m_PositionSaver;		// auto keeps position

	SET_NO_COPYING( SimpleDlgSink );
};

//////////////////////////////////////////////////////////////////////////////
// class ErrorCountDlgSink declaration

#if 0

ErrorCountDlgSink;
typedef atlx::AsyncWnd <ErrorCountDlgSink, atlx::DialogImpl <ErrorCountDlgSink> ErrorCountDlgSinkBase;

class ErrorCountDlgSink : public ErrorCountDlgSink, public Sink
{
public:
	SET_INHERITED2( ErrorCountDlgSink, ErrorCountDlgSinkBase, Sink );
	typedef Inherited1 DlgInherited;
	typedef Inherited2 SinkInherited;

	// ctor/dtor
	/*PRI*/ ErrorCountDlgSink( HWND parent = NULL, const char* name = NULL );
	/*PRI*/ virtual ~ErrorCountDlgSink( void );

	// overrides
	/*PRI*/ virtual bool IsEnabled   ( void ) const				{  return ( SinkInherited::IsEnabled() && IsWindow() && IsWindowEnabled() );  }
	/*PRI*/ virtual void OnEndReport ( const Context* context );

	// factory
	/*PRI*/ static Sink* CreateSink( const char* name, const char* type, const char* params );

private:
	/*PRI*/ virtual bool Create( const HWND& parent );
	/*AUX*/ virtual bool ThreadCreate( const HWND& parent );

	typedef atlx::ResizableImpl <ThisType> Resizer;

	BEGIN_MSG_MAP( ThisType )
		CHAIN_MSG_MAP( DlgInherited )
		MESSAGE_CRACKER( WM_INITDIALOG, OnInitDialog )
	END_MSG_MAP()

	/*AUX*/ BOOL OnInitDialog( HWND focusctl, LPARAM init, BOOL& handled );

	// state
	winx::Font m_EditFont;			// font to use for edit control

	SET_NO_COPYING( ErrorCountDlgSink );
};

#endif // 0

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace ReportSys

#endif  // __REPORTSYSDIALOGS_H

//////////////////////////////////////////////////////////////////////////////
