//////////////////////////////////////////////////////////////////////////////
//
// File     :  ReportSysDialogs.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GpCore.h"
#include "ReportSysDialogs.h"

#include "Res\GpCoreRes.rh"

namespace ReportSys  {  // begin of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////

DECLARE_SINK_FACTORY( SimpleDialog, SimpleDlgSink::CreateSink );

#if 0
DECLARE_SINK_FACTORY( ErrorCountDialog, ErrorCountDlgSink::CreateSink );
#endif // 0

//////////////////////////////////////////////////////////////////////////////
// class MgrDlg implementation

MgrDlg :: MgrDlg( HWND parent )
{
//	$$$
	UNREFERENCED_PARAMETER( parent );
}

MgrDlg :: ~MgrDlg( void )
{
	// this space intentionally left blank...
}

BOOL MgrDlg :: OnInitDialog( HWND focusctl, LPARAM init, BOOL& handled )
{
	UNREFERENCED_PARAMETER( focusctl );
	UNREFERENCED_PARAMETER( init );
	UNREFERENCED_PARAMETER( handled );
	return ( FALSE );
}

//////////////////////////////////////////////////////////////////////////////
// class EditCtlSink implementation

EditCtlSink :: EditCtlSink( atlx::Edit editCtl )
{
	SetEditCtl( editCtl );
}

EditCtlSink :: ~EditCtlSink( void )
{
	// this space intentionally left blank...
}

void EditCtlSink :: SetEditCtl( atlx::Edit editCtl )
{
	m_EditCtl = editCtl;
	if ( m_EditCtl )
	{
		m_EditCtl.SetLimitText( 256 * 1024 );		// 1/4 meg seems like a good limit...
		m_LimitText = m_EditCtl.GetLimitText();
		gpassertm( m_EditCtl.GetStyle() & ES_MULTILINE, "EditCtlSink can only be used on multiline edit controls" );
	}
	else
	{
		m_LimitText = 0;
	}
	m_EndChopPos = 0;
}

void EditCtlSink :: ClearText( void )
{
	m_EditCtl.ClearText();
	m_EndChopPos = 0;
}

void EditCtlSink :: AppendText( const char* text )
{
	// chop if it's time
	if ( (m_LimitText - m_EditCtl.GetTextLength()) < (int)::strlen( text ) )
	{
		m_EditCtl.SetSel( 0, m_EndChopPos );
		m_EditCtl.ReplaceSel( "" );
		m_EditCtl.SetSelAtEnd();

		m_EndChopPos = 0;
	}

	m_EditCtl.AppendText( text );
}

void EditCtlSink :: OnBeginReport( const Context* context )
{
	Inherited::OnBeginReport( context );
	if ( IsEnabled() && (context->GetReportLevel() == 0) )
	{
		m_EditCtl.SetRedraw( FALSE );
	}
}

void EditCtlSink :: OnEndReport( const Context* context )
{
	Inherited::OnEndReport( context );
	if ( IsEnabled() && (context->GetReportLevel() == 1) )
	{
		m_EditCtl.SetRedraw( TRUE );
	}
}

bool EditCtlSink :: OutputString( const Context* context,
								  const char* text, int len,
								  bool startLine, bool endLine )
{
	gpassert( len >= 0 );
	gpassert( IsEnabled() );

	bool success = Inherited::OutputString( context, text, len, startLine, endLine );

	// it's a new line
	if ( startLine )
	{
		// update chop if necessary
		if ( m_EndChopPos == 0 )
		{
			int chopDist = m_LimitText / 4;		// 25% chop-out
			int pos = m_EditCtl.GetTextLength();
			if ( pos > chopDist )
			{
				m_EndChopPos = pos;
			}
		}

		// write indent
		int indent = GetIndentSpaces( context );
		if ( indent > 0 )
		{
			AppendText( MakeIndentText( indent ) );
		}
	}

	// write text
	std::vector <char> buffer( len + 1 );
	::memcpy( buffer.begin(), text, len );
	buffer[ len ] = '\0';
	AppendText( buffer.begin() );

	// terminate
	if ( endLine )
	{
		AppendText( "\r\n" );
	}

	// done
	return ( success );
}

//////////////////////////////////////////////////////////////////////////////
// class SimpleDlgSink implementation

static const int  CONSOLE_FONT_SIZE   = 9;
static const char CONSOLE_FONT_NAME[] = "Lucida Console";

/*PRI*/ SimpleDlgSink :: SimpleDlgSink( HWND parent, const char* name )
{
	// configure self
	SetType( "SimpleDialog" );
	SetName( name );

	// now create the dialog
	Create( parent, (name && (*name)) ? name : NULL );
}

/*PRI*/ SimpleDlgSink :: ~SimpleDlgSink( void )
{
	// this space intentionally left blank...
}

/*PRI*/ void SimpleDlgSink :: SetName( const char* name )
{
	SinkInherited::SetName( name );

	if ( !GetName().empty() )
	{
		m_PositionSaver.SetName( GetName() );
		m_PositionSaver.ClearOptions( PositionSaver::OPTION_DISABLE );
	}
	else
	{
		m_PositionSaver.SetOptions( PositionSaver::OPTION_DISABLE );
	}

	UpdateWindowName();
}

/*PRI*/ Sink* SimpleDlgSink :: CreateSink( const char* name, const char* /*type*/, const char* /*params*/ )
{
	return ( new SimpleDlgSink( NULL, name ) );
}

/*AUX*/ BOOL SimpleDlgSink :: OnPreInitDialog( HWND /*focusctl*/, LPARAM /*init*/, BOOL& handled )
{
	handled = false;

	// setup components
	m_Resizer.SetMinimumSize( GSize::ZERO );
	m_Resizer.FixCornerToParent( GetDlgItem( IDC_SCROLLBACK ), atlx::CORNER_BOTTOMRIGHT );

	return ( TRUE );
}

/*AUX*/ BOOL SimpleDlgSink :: OnInitDialog( HWND /*focusctl*/, LPARAM /*init*/, BOOL& /*handled*/ )
{
	// setup edit
	m_EditSink.SetEditCtl( atlx::Edit( GetDlgItem( IDC_SCROLLBACK ) ) );
	AddSink( &m_EditSink, false );

	// set font for control
	m_EditFont.Create( winx::LogFont( CONSOLE_FONT_NAME, CONSOLE_FONT_SIZE ) );
	GetDlgItem( IDC_SCROLLBACK ).SetWindowFont( m_EditFont, FALSE );

	// update name based on sink name
	UpdateWindowName();
	
	// auto center if placement failed
	if ( !m_PositionSaver.IsInitialized() )
	{
		CenterWindow();
	}

	// done
	return ( TRUE );
}

/*AUX*/ void SimpleDlgSink :: OnDestroy( BOOL& handled )
{
	handled = false;

	// prevent from receiving future reports
	Enable( false );
}

/*PRI*/ bool SimpleDlgSink :: Create( const HWND& parent, const char* threadName )
{
	return ( Inherited1::Create( parent     ? parent     : winx::GetFrontWindow(),
								 threadName ? threadName : "SmpDlgSnk" ) );
}

/*AUX*/ bool SimpleDlgSink :: ThreadCreate( const HWND& parent )
{
	static const WORD dlgTemplate[] =
	{
		#define GP_DIALOG_EDITSINK 1

		#if GP_DEBUG
		#include "Res\GpCoreRes-Debug.dat"
		#elif GP_RELEASE
		#include "Res\GpCoreRes-Release.dat"
		#elif GP_RETAIL
		#include "Res\GpCoreRes-Retail.dat"
		#elif GP_PROFILING
		#include "Res\GpCoreRes-Profiling.dat"
		#endif

		#undef  GP_DIALOG_EDITSINK
	};

	return ( AtlInherited::Create( rcast <const DLGTEMPLATE*> ( dlgTemplate ), parent ) != NULL );
}

/*AUX*/ void SimpleDlgSink :: UpdateWindowName( void )
{
	if ( IsWindow() )
	{
		if ( !GetName().empty() )
		{
			SetWindowText( gpstringf( "Console: %s", GetName().c_str() ) );
		}
		else
		{
			SetWindowText( "Console" );
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// class ErrorCountDlgSink implementation

#if 0

//$$$ finish

/*PRI*/ ErrorCountDlgSink :: ErrorCountDlgSink( HWND parent, const char* name )
	: SinkInherited( name )
{
	SetType( "ErrorCountDialog" );
	Create( parent );
}

/*PRI*/ ErrorCountDlgSink :: ~ErrorCountDlgSink( void )
{
	// this space intentionally left blank...
}

/*PRI*/ void ErrorCountDlgSink :: OnEndReport( const Context* context )
{
	SinkInherited::OnEndReport( context );
	Update();
}

/*PRI*/ Sink* ErrorCountDlgSink :: CreateSink( const char* name, const char* /*type*/, const char* /*params*/ )
{
	return ( new ErrorCountDlgSink( NULL, name ) );
}

/*AUX*/ BOOL ErrorCountDlgSink :: OnInitDialog( HWND /*focusctl*/, LPARAM /*init*/, BOOL& /*handled*/ )
{
	// no minimum size
	m_Resizer.SetMinimumSize( GSize::ZERO );

	// auto resize edit control
	m_Resizer.FixCornerToParent( GetDlgItem( IDC_SCROLLBACK ), atlx::CORNER_BOTTOMRIGHT );

	// set font for control
	m_EditFont.Create( winx::LogFont( CONSOLE_FONT_NAME, CONSOLE_FONT_SIZE ) );
	GetDlgItem( IDC_SCROLLBACK ).SetWindowFont( m_EditFont, FALSE );

	// done
	return ( TRUE );
}

/*PRI*/ bool ErrorCountDlgSink :: Create( const HWND& parent )
{
	return ( Inherited1::Create( parent ? parent : winx::GetFrontWindow() ) );
}

/*AUX*/ bool ErrorCountDlgSink :: ThreadCreate( const HWND& parent )
{
	static const WORD dlgTemplate[] =
	{
		#define GP_DIALOG_COUNTBOX 1

		#if GP_DEBUG
		#include "Res\GpCoreRes-Debug.dat"
		#elif GP_RELEASE
		#include "Res\GpCoreRes-Release.dat"
		#elif GP_RETAIL
		#include "Res\GpCoreRes-Retail.dat"
		#elif GP_PROFILING
		#include "Res\GpCoreRes-Profiling.dat"
		#endif

		#undef  GP_DIALOG_COUNTBOX
	};

	return ( AtlInherited::Create( rcast <const DLGTEMPLATE*> ( dlgTemplate ), parent ) != NULL );
}

#endif // 0

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace ReportSys

//////////////////////////////////////////////////////////////////////////////
