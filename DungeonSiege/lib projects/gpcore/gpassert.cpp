//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpassert.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "GpAssert.h"

#ifndef NOGPCORE

#include "AppModule.h"
#include "AtlX.h"
#include "DebugHelp.h"
#include "DrWatson.h"
#include "FileSysXfer.h"
#include "KernelTool.h"
#include "ReportSysDialogs.h"
#include "StdHelp.h"
#include "StringTool.h"
#include "WinX.h"

#include <cstdio>
#include <vector>
#include <set>

#include "Res\GpCoreRes.rh"

//////////////////////////////////////////////////////////////////////////////
// debugger output macros

#undef OutputDebugStringF
void OutputDebugStringF( const char* format, ... )
{
	::OutputDebugString( auto_dynamic_vsnprintf( format, va_args( format ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// class AssertDlg declaration

class AssertDlg : atlx::DialogImpl <AssertDlg>
{
public:
	SET_INHERITED( AssertDlg, atlx::DialogImpl <AssertDlg> );

// Ctor/dtor.

	AssertDlg( const AssertData& data );
   ~AssertDlg( void );

// Commands.

	int DoModal( HWND parent = NULL );

// More/less.

	void Fold      ( bool fold = true );
	void Unfold    ( void )                 {  Fold( false );  }
	bool IsFolded  ( void ) const           {  return ( ms_Persist.m_IsFolded );  }
	void ToggleFold( void )                 {  Fold( !IsFolded() );  }

// Feedback.

	int GetIgnoreCount      ( void ) const  {  return ( m_IgnoreCount );  }
	int GetGlobalIgnoreCount( void ) const  {  return ( m_GlobalIgnoreCount );  }

private:

// Message map.

	BEGIN_MSG_MAP( AssertDlg )
		MESSAGE_CRACKER     ( WM_INITDIALOG,     OnInitDialog     )
		MESSAGE_CRACKER     ( WM_DESTROY,        OnDestroy        )
		MESSAGE_CRACKER     ( WM_GETMINMAXINFO,  OnGetMinMaxInfo  )
		MESSAGE_CRACKER     ( WM_SIZE,           OnSize           )
		MESSAGE_CRACKER     ( WM_SETCURSOR,      OnSetCursor      )
		MESSAGE_CRACKER     ( WM_ERASEBKGND,     OnEraseBkgnd     )
		MESSAGE_CRACKER     ( WM_PAINT,          OnPaint          )
		MESSAGE_CRACKER     ( WM_NCHITTEST,      OnNcHitTest      )
		COMMAND_CODE_HANDLER( BN_CLICKED,        OnButton         )
	END_MSG_MAP()

// Message handlers.

	BOOL    OnInitDialog    ( HWND focusctl, LPARAM init, BOOL& handled );
	void    OnDestroy       ( BOOL& handled );
	void    OnGetMinMaxInfo ( MINMAXINFO* minMaxInfo, BOOL& handled );
	void    OnSize          ( UINT state, int cx, int cy, BOOL& handled );
	BOOL    OnSetCursor     ( HWND hwndCursor, UINT codeHitTest, UINT msg, BOOL& handled );
	BOOL    OnEraseBkgnd    ( HDC hdc, BOOL& handled );
	void    OnPaint         ( BOOL& handled );
	UINT    OnNcHitTest     ( int x, int y, BOOL& handled );
	LRESULT OnButton        ( WORD code, WORD id, HWND ctl, BOOL& handled );

// Utility.

	// wrappers/helpers
	bool CopyTextToClipboard( atlx::Edit& edit );

	// utility
	void  SetFoldExtraEnabling( bool enable );
	void  InvalidateGrip      ( void );
	GRect GetClientGripRect   ( void );

// Types.

	struct Persist
	{
		bool  m_IsFolded;
		bool  m_PauseOnContinue;
		GRect m_LastRect;

		Persist( void )
		{
			m_IsFolded = true;
			m_PauseOnContinue = false;
			m_LastRect.Set( 0xFFFFFFFF, 0, 0, 0 );
		}
	};

// Private data.

	// statics
	static Persist ms_Persist;

	// spec
	const AssertData&             m_Assert;
	ReportSys::AutoSafeMessageBox m_AutoSafe;
	CONTEXT                       m_ThreadContext;
	bool                          m_IsContextValid;

	// controls
	atlx::Edit   m_AssertText;
	atlx::Edit   m_TraceText;
	atlx::Button m_MoreBtn;
	atlx::Button m_VerboseCheckBox;
	atlx::Button m_PauseCheckBox;

	// dialog state
	GRect       m_OriginalRect;      	// original size of rect
	int         m_FoldedHeight;			// how tall it is folded up
	int         m_UnfoldedHeight;		// how tall it is unfolded
	int         m_RightBorder;			// width of right border
	int         m_BottomBorder;			// width of bottom border
	winx::Font  m_EditFont;				// font to use for edit control
	winx::Brush m_Brush;				// brush for background painting

	// feedback
	int m_IgnoreCount;			// ignore count for future assertions like this
	int m_GlobalIgnoreCount;	// ignore count for all future assertions
};

//////////////////////////////////////////////////////////////////////////////
// class AssertDlg implementation

static const GSize GRIP_SIZE( 15, 15 );
static const int   ASSERT_FONT_SIZE   = 9;
static const char  ASSERT_FONT_NAME[] = "Lucida Console";

AssertDlg::Persist AssertDlg::ms_Persist;

static DWORD s_PrimaryThreadId = ::GetCurrentThreadId();

AssertDlg :: AssertDlg( const AssertData& data )
	: m_AutoSafe( true ), m_Assert( data )
{
	m_FoldedHeight       = 0;
	m_UnfoldedHeight     = 0;
	m_RightBorder        = 0;
	m_BottomBorder       = 0;
	m_IgnoreCount        = 0;
	m_GlobalIgnoreCount  = 0;

	m_OriginalRect.SetEmpty();

	m_ThreadContext.ContextFlags = CONTEXT_FULL;
	m_IsContextValid = !!::GetThreadContext( ::GetCurrentThread(), &m_ThreadContext );
}

AssertDlg :: ~AssertDlg( void )
{
	if ( ms_Persist.m_PauseOnContinue && AppModule::DoesSingletonExist() )
	{
		gAppModule.UserPause();
	}
}

int AssertDlg :: DoModal( HWND parent )
{
	static const WORD dlgTemplateData[] =
	{
		#define GP_DIALOG_ASSERT 1

		#if GP_DEBUG
		#include "Res\GpCoreRes-Debug.dat"
		#elif GP_RELEASE
		#include "Res\GpCoreRes-Release.dat"
		#elif GP_RETAIL
		#include "Res\GpCoreRes-Retail.dat"
		#elif GP_PROFILING
		#include "Res\GpCoreRes-Profiling.dat"
		#endif

		#undef  GP_DIALOG_ASSERT
	};

	if ( (parent == NULL) && (::GetCurrentThreadId() == s_PrimaryThreadId) )
	{
		if ( AppModule::DoesSingletonExist() && gAppModule.HasMainWnd() )
		{
			parent = gAppModule.GetMainWnd();
		}
		else
		{
			parent = winx::GetFrontWindow();
		}
	}

	return ( Inherited::DoModal( rcast <const DLGTEMPLATE*> ( dlgTemplateData ), parent ) );
}

void AssertDlg :: Fold( bool fold )
{
	if ( IsFolded() == fold )
	{
		return;
	}

	ms_Persist.m_IsFolded = fold;
	InvalidateGrip();

	// update button
	m_MoreBtn.SetText( fold ? "&More" : "&Less");

	// if unfolding, show controls before the MoveWindow()
	if ( !fold )
	{
		SetFoldExtraEnabling( true );
	}

	// resize
	GRect wndRect = GetWindowRect();
	wndRect.bottom = wndRect.top + (fold ? m_FoldedHeight : m_UnfoldedHeight);
	MoveWindow( wndRect );

	// if folding, hide controls after the MoveWindow()
	if ( fold )
	{
		SetFoldExtraEnabling( false );
	}

	InvalidateGrip();
}

BOOL AssertDlg :: OnInitDialog( HWND /*focusctl*/, LPARAM /*init*/, BOOL& /*handled*/ )
{
	char buffer[ 6000 ];

// Initialize controls.

	m_AssertText      = GetDlgItem( IDC_ASSERT_TEXT );
	m_TraceText       = GetDlgItem( IDC_TRACE_TEXT );
	m_MoreBtn         = GetDlgItem( IDC_MORE );
	m_VerboseCheckBox = GetDlgItem( IDC_MINIDUMP_VERBOSE );
	m_PauseCheckBox   = GetDlgItem( IDC_PAUSE_ON_CONTINUE );

// Initialize defaults.

	// simple stuff
	HICON errorIcon = ::LoadIcon( NULL, (m_Assert.m_Style == AssertData::STYLE_HATCHED) ? IDI_ERROR : IDI_WARNING );
	winx::Edit::SetText( GetDlgItem( IDC_IGNORE_COUNT ), "0" );
	winx::Static::SetIcon( GetDlgItem( IDC_THEICON ), errorIcon );

	// do title
	const char* type = m_Assert.m_Type;
	if ( !type || !*type )
	{
		type = "<Unknown>";
	}
	sprintf( buffer, "GPCore %s Failure", type );
	winx::Static::SetText( *this, buffer );

	// do icon
	SetIcon( errorIcon );

	// build brush
	if ( m_Assert.m_Style == AssertData::STYLE_HATCHED )
	{
		m_Brush.CreateHatch( HS_FDIAGONAL, RGB( 255, 0, 0 ) );
	}

// Initialize text.

	// build a font spec
	winx::LogFont logFont;
	logFont.SetHeightPoints( ASSERT_FONT_SIZE );
	logFont.SetName( ASSERT_FONT_NAME );
	logFont.SetPitchAndFamily( FIXED_PITCH | FF_MODERN );

	// create the font
	m_EditFont.Create( logFont );

	// set font for controls
	m_AssertText.SetWindowFont( m_EditFont, FALSE );
	m_TraceText.SetWindowFont( m_EditFont, FALSE );

	// get text for control
	m_Assert.FormatAssertion( buffer, ELEMENT_COUNT( buffer ), true );
	gpdebugger( buffer );

	// init control with text
	m_AssertText.SetText( buffer );
	m_AssertText.SetSelNone();

// Initialize geometry.

	// dialog rect
	GetWindowRect( m_OriginalRect );
	m_UnfoldedHeight = m_OriginalRect.bottom - m_OriginalRect.top;

	// adjustments for folded size
	GRect childRect = GetDlgItemWindowRect( IDC_DIVIDER );
	m_FoldedHeight = childRect.top - m_OriginalRect.top;

	// borders
	childRect = m_TraceText.GetWindowRect();
	m_RightBorder  = m_OriginalRect.right  - childRect.right;
	m_BottomBorder = m_OriginalRect.bottom - childRect.bottom;

	// put me in the right spot (if centering, use unfolded dialog)
	if ( ms_Persist.m_LastRect.left != 0xFFFFFFFF )
	{
		MoveWindow( ms_Persist.m_LastRect );
		m_UnfoldedHeight = ms_Persist.m_LastRect.Height();
	}
	else
	{
		// center window
		if ( AppModule::DoesSingletonExist() && gAppModule.HasVisibleMainWnd() )
		{
			GRect rect( GetWindowRect() );
			rect.CenterIn( gAppModule.GetGameRect() );
			SetWindowPos( NULL, rect.left, rect.top, -1, -1, SWP_NOSIZE );
		}
		else
		{
			CenterWindow();
		}
	}

// Initialize persistent stuff.

	// fold state
	bool old = IsFolded();
	ms_Persist.m_IsFolded = false;
	Fold( old );

	// pause state
	m_PauseCheckBox.SetCheckBool( ms_Persist.m_PauseOnContinue );

// Finish up.

	// fullscreen means the main window is always on top - assert must be too
	if ( AppModule::DoesSingletonExist() && gAppModule.IsFullScreen() )
	{
		SetAlwaysOnTop();
	}

	// finish
	GetDlgItem( IDIGNORE ).SetFocus();			// choose ignore as focus
	SetForeground();							// set us as foreground
	SetActiveWindow();							// activate and move to top

	// done
	return ( FALSE );
}

void AssertDlg :: OnDestroy( BOOL& handled )
{
	ms_Persist.m_LastRect = GetWindowRect();
	ms_Persist.m_LastRect.bottom = ms_Persist.m_LastRect.top + m_UnfoldedHeight;
	handled = FALSE;
}

void AssertDlg :: OnGetMinMaxInfo( MINMAXINFO* minMaxInfo, BOOL& handled )
{
	handled = FALSE;

	minMaxInfo->ptMinTrackSize.x = m_OriginalRect.right - m_OriginalRect.left;

	if ( IsFolded() )
	{
		minMaxInfo->ptMinTrackSize.y = m_FoldedHeight;
		minMaxInfo->ptMaxTrackSize.y = m_FoldedHeight;
	}
	else
	{
		minMaxInfo->ptMinTrackSize.y = m_OriginalRect.bottom - m_OriginalRect.top;
	}

	InvalidateGrip();
}

void AssertDlg :: OnSize( UINT state, int /*cx*/, int cy, BOOL& handled )
{
	handled = FALSE;

	if ( state != 0 )
	{
		return;
	}

	// update saved size for next time we fold then unfold
	if ( !IsFolded() )
	{
		m_UnfoldedHeight = cy;
	}

// Update sizes of children.

	// get dialog size
	GSize clientSize = GetClientSize();

	// resize assert text box
	GRect childRect = GetDlgItemRect( m_AssertText );
	childRect.right = clientSize.cx - m_RightBorder;
	m_AssertText.SetWindowPos( NULL, -1, -1, childRect.Width(), childRect.Height(),
							   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );

	// move more/less button
	childRect = GetDlgItemRect( m_MoreBtn );
	childRect.left = (clientSize.cx - m_RightBorder) - (childRect.right - childRect.left);
	m_MoreBtn.SetWindowPos( NULL, childRect.left, childRect.top, -1, -1,
							SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );

	// resize trace text box
	childRect = GetDlgItemRect( m_TraceText );
	childRect.right  = clientSize.cx - m_RightBorder;
	childRect.bottom = clientSize.cy - m_BottomBorder;
	m_TraceText.SetWindowPos( NULL, -1, -1, childRect.Width(), childRect.Height(),
							  SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | (IsFolded() ? SWP_NOREDRAW : 0) );
}

BOOL AssertDlg :: OnSetCursor( HWND hwndCursor, UINT codeHitTest, UINT /*msg*/, BOOL& handled )
{
	handled = FALSE;

	if ( IsFolded() && (m_hWnd == hwndCursor) )
	{
		switch ( codeHitTest )
		{
			case ( HTTOPLEFT ):
			case ( HTBOTTOMLEFT ):
			case ( HTTOPRIGHT ):
			case ( HTBOTTOMRIGHT ):
			{
				::SetCursor( LoadCursor( NULL, IDC_SIZEWE ) );
				handled = TRUE;
			}	break;

			case ( HTTOP ):
			case ( HTBOTTOM ):
			{
				::SetCursor( LoadCursor( NULL, IDC_ARROW ) );
				handled = TRUE;
			}	break;
		}
	}
	return ( handled );
}

BOOL AssertDlg :: OnEraseBkgnd( HDC hdc, BOOL& handled )
{
	if ( m_Brush.DoesExist() )
	{
		::SetBkColor( hdc, ::GetSysColor( COLOR_BTNFACE ) );
		::FillRect( hdc, GetClientRect(), m_Brush );
		return ( TRUE );
	}
	else
	{
		handled = FALSE;
		return ( FALSE );
	}
}

void AssertDlg :: OnPaint( BOOL& /*handled*/ )
{
	PAINTSTRUCT paint;
	HDC hdc = BeginPaint( &paint );
	DrawFrameControl( hdc, GetClientGripRect(), DFC_SCROLL, DFCS_SCROLLSIZEGRIP );
	EndPaint( &paint );
}

UINT AssertDlg :: OnNcHitTest( int x, int y, BOOL& handled )
{
	handled = false;

	UINT ht = FORWARD_WM_NCHITTEST( m_hWnd, x, y, ::DefWindowProc );
	if ( (ht == HTCLIENT) || (ht == HTNOWHERE) )
	{
		// get grip
		GRect rect = GetClientToScreen( GetClientGripRect() );

		// extend out to window edge
		GRect wndRect = GetWindowRect();
		rect.right = wndRect.right;
		rect.bottom = wndRect.bottom;

		// test
		if ( rect.Contains( GPoint( x, y ) ) )
		{
			ht = IsFolded() ? HTRIGHT : HTBOTTOMRIGHT;
			handled = true;
		}
	}
	return ( ht );
}

LRESULT AssertDlg :: OnButton( WORD /*code*/, WORD id, HWND /*ctl*/, BOOL& handled )
{
	switch ( id )
	{

	// Folded.

		case ( IDABORT  ):           {  EndDialog( IDABORT  );  }  break;
		case ( IDRETRY  ):           {  EndDialog( IDRETRY  );  }  break;
		case ( IDIGNORE ):           {  EndDialog( IDIGNORE );  }  break;
		case ( IDCANCEL ):           {  EndDialog( IDCANCEL );  }  break;
		case ( IDC_IGNORE_ALWAYS ):  {  m_IgnoreCount = -1;  EndDialog( IDIGNORE );  }  break;
		case ( IDC_COPY_ASSERT ):    {  CopyTextToClipboard( m_AssertText );  }  break;
		case ( IDC_MORE ):           {  ToggleFold();  }  break;

	// Unfolded.

		case ( IDC_IGNORE_THIS ):    {  m_IgnoreCount = GetDlgItemInt( IDC_IGNORE_COUNT );  EndDialog( IDIGNORE );  }  break;
		case ( IDC_IGNORE_ALL ):     {  m_GlobalIgnoreCount = GetDlgItemInt( IDC_IGNORE_COUNT );  EndDialog( IDIGNORE );  }  break;

		case ( IDC_GENERATE_CRASH_DUMP ):
		{
			// waitaminnit
			winx::WaitCursor goGettaCupOfCoffee;

			// build it
			gpdebugger( "Building minidump...\n" );

			bool verbose = m_VerboseCheckBox.GetCheckBool();

			gpstring fileName;
			if ( gDbgSymbolEngine.MiniDump( verbose, NULL, &fileName ) )
			{
				gpmessageboxf(( "%s minidump successfully taken to '%s'.\n\nIf "
							    "you are reporting a bug, be sure to include "
							    "the error text and stack trace from this "
							    "dialog as well as the dslog on disk.\n",
							    verbose ? "Verbose" : "Plain",
							    fileName.c_str() ));
			}
			else if ( ::GetLastError() == ERROR_DLL_INIT_FAILED )
			{
				gperrorbox( "Unable to generate minidump. Either DBGHELP.DLL "
							"does not exist on this machine, or it's not at "
							"least version 5.1. To fix this problem, find a "
							"box with Windows XP on it and copy the DBGHELP.DLL "
							"from its system32 directory to this app's EXE "
							"directory (and then re-run this app).\n" );
			}
			else
			{
				gperrorboxf(( "Failed to create minidump, last error is "
							  "0X%08X ('%s')\n",
							  ::GetLastError(),
							  stringtool::GetLastErrorText().c_str() ));
			}
		}
		break;

		case ( IDC_TRACE_ACQUIRE ):
		{
			// waitaminnit
			winx::WaitCursor goGettaCupOfCoffee;

			// enable all trace-related controls
			winx::Button::Enable( GetDlgItem( IDC_TRACE_COPY  ), true );
			winx::Static::Enable( GetDlgItem( IDC_STACK_TRACE ), true );
			winx::Edit  ::Enable( GetDlgItem( IDC_TRACE_TEXT  ), true );

			// acquire trace
			gpdebugger( "Acquiring trace...\n" );
			ReportSys::EditCtlSink editSink( m_TraceText );
			editSink.ClearText();
			ReportSys::LocalContext context( &editSink, false );
			DbgSymbolEngine::eError rc = DbgSymbolEngine::ERR_SUCCESS;
			if ( m_IsContextValid )
			{
				rc = gDbgSymbolEngine.StackTrace(
						::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentThreadId(),
						m_ThreadContext, context, m_Assert.m_StartFrame );

				// send to debugger too. do it through the context directly so
				// that our log hook on debugger context catches it too.
				gDbgSymbolEngine.StackTrace(
						::GetCurrentProcess(), ::GetCurrentThread(), ::GetCurrentThreadId(),
						m_ThreadContext, gDebuggerContext, m_Assert.m_StartFrame );
			}
			else
			{
				rc = gDbgSymbolEngine.StackTrace( context, m_Assert.m_StartFrame );
			}

			// gather extra info, do it twice (same reason as stack trace)
			{
				DrWatson::Info info;
				FileSys::ReportWriter writer( context );
				context.OutputEol();
				info.m_Writer = &writer;
				DrWatson::ExecuteInfoCallbacks( info );
			}
			{
				DrWatson::Info info;
				FileSys::ReportWriter writer( gDebuggerContext );
				gDebuggerContext.OutputEol();
				info.m_Writer = &writer;
				DrWatson::ExecuteInfoCallbacks( info );
			}

			// disable acquire button on success
			if ( rc == DbgSymbolEngine::ERR_SUCCESS )
			{
				winx::Button::Enable( GetDlgItem( IDC_TRACE_ACQUIRE ), false );
			}

			// back to top
			m_TraceText.ScrollToTop();

		}	break;

		case ( IDC_TRACE_COPY ):
		{
			CopyTextToClipboard( m_TraceText );
		}	break;

		case ( IDC_PAUSE_ON_CONTINUE ):
		{
			ms_Persist.m_PauseOnContinue = !ms_Persist.m_PauseOnContinue;
		}
		break;

	// Unknown.

		default:  handled = FALSE;
	}

	// done
	return ( 0 );
}

bool AssertDlg :: CopyTextToClipboard( atlx::Edit& edit )
{
	bool rc = edit.CopyTextToClipboard();
	if ( !rc )
	{
		gperrorboxf(( "Unable to copy text to clipboard!\n\nError = 0x%08X ('%s')\n",
					  ::GetLastError(), stringtool::GetLastErrorText().c_str() ));
	}
	return ( rc );
}

void AssertDlg :: SetFoldExtraEnabling( bool enable )
{
	// check the focus to make sure we aren't going to disable it
	if ( !enable )
	{
		HWND focusWnd = ::GetFocus();
		if ( focusWnd != NULL )
		{
			int focusID = ::GetDlgCtrlID( focusWnd );
			if ( focusID >= IDC_DIVIDER )
			{
				m_MoreBtn.SetFocus();
			}
		}
	}

	for (  HWND i = winx::GetNextSibling( GetDlgItem( IDC_DIVIDER ) )
		 ; i != NULL
		 ; i = winx::GetNextSibling( i ) )
	{
		BOOL wasEnabled;

		// store the old enabling value in the user data area so we don't have
		//  to track it with our own collection
		if ( enable )
		{
			wasEnabled = ::GetWindowLong( i, GWL_USERDATA );
			if ( wasEnabled )
			{
				::EnableWindow( i, TRUE );
			}
		}
		else
		{
			wasEnabled = !::EnableWindow( i, enable );
			::SetWindowLong( i, GWL_USERDATA, wasEnabled );
		}
	}
}

void AssertDlg :: InvalidateGrip( void )
{
	InvalidateRect( GetClientGripRect(), TRUE );
}

GRect AssertDlg :: GetClientGripRect( void )
{
	GRect rect = GetClientRect();
	rect.left = rect.right - GRIP_SIZE.cx;
	rect.top  = rect.bottom - GRIP_SIZE.cy;
	return ( rect );
}

//////////////////////////////////////////////////////////////////////////////
// assertion support

AssertInstance* AssertInstance::ms_Root = NULL;

bool AssertInstance :: operator < ( const AssertInstance& other ) const
{
	if (    ((      m_File == NULL) || (*      m_File == '\0'))
		&& !((other.m_File == NULL) || (*other.m_File == '\0')) )
	{
		return ( true );
	}

	if (    ((other.m_File == NULL) || (*other.m_File == '\0'))
		&& !((      m_File == NULL) || (*      m_File == '\0')) )
	{
		return ( false );
	}

	int comp = ::strcmp( m_File, other.m_File );
	if ( comp < 0 )
	{
		return ( true );
	}

	if ( comp > 0 )
	{
		return ( false );
	}

	return ( m_Line < other.m_Line );
}

struct CompareByPtr
{
	template <typename T>
	bool operator () ( const T* l, const T* r ) const
	{
		return ( *l < *r );
	}
};

void AssertInstance :: ReportSummary( ReportSys::Context* context )
{
	ReportSys::ContextRef ctx( context ? context : &gGenericContext );

// Gather data.

	typedef std::set <AssertInstance*, CompareByPtr> InstanceColl;
	InstanceColl coll;

	for ( AssertInstance* i = AssertInstance::ms_Root ; i != NULL ; i = i->m_Next )
	{
		if ( i->m_FailCount > 0 )
		{
			coll.insert( i );
		}
	}

// Output list of assertions.

	if ( !coll.empty() )
	{
		ReportSys::AutoReport autoReport( ctx );

		ctx->Output( "Assertions ignored during session:\n"
				     "----------------------------------\n" );
		InstanceColl::const_iterator j, jbegin = coll.begin(), jend = coll.end();
		int count = 0;
		for ( j = jbegin ; j != jend ; ++j, ++count )
		{
			ctx->OutputF( "%4d: Failed %4d times, ", count, (*j)->m_FailCount );
			if ( ((*j)->m_File != NULL) && (*(*j)->m_File != '\0') )
			{
				ctx->OutputF( "File = \"%s\", Line = %d\n", (*j)->m_File, (*j)->m_Line );
			}
			else
			{
				ctx->Output( "<Location unknown>\n" );
			}
		}

		ctx->Output( "----------------------------------\n" );
	}

// Summarize.

	if ( AssertData::ms_FailedCount > 0 )
	{
		ctx->OutputF( "Total failed assertions: %d\n", AssertData::ms_FailedCount );
	}
}

int  AssertData::ms_AssertLevel = 0;
int  AssertData::ms_FailedCount = 0;
int  AssertData::ms_GlobalIgnoreCount = 0;
char AssertData::ms_Buffer[ BUFFER_SIZE ] = { 0 };
bool AssertData::ms_TempBad = false;

void AssertData :: PrintF( const char* format, ... )
{
	::dynamic_vsnprintf( ms_Buffer, ELEMENT_COUNT( ms_Buffer ), NULL, format, va_args( format ) );
}

void AssertData :: FormatAssertion( char* buffer, int maxLen, bool convertNewlines ) const
{
	// pre-terminate and remove it from set
	buffer[ maxLen - 1 ] = '\0';
	--maxLen;

	// build location
	char location[ 500 ];
	if ( m_File && *m_File )
	{
		const char* filename = m_File + strrcspn( m_File, "\\/:" );
		if ( *filename == '\0' )
		{
			::_snprintf( location, ELEMENT_COUNT( location ), "%s line %d", m_File, m_Line );
		}
		else
		{
			++filename;
			::_snprintf( location, ELEMENT_COUNT( location ), "%s line %d (at %.*s)",
						 filename, m_Line, filename - m_File, m_File );
		}
		ELEMENT_LAST( location ) = '\0';
	}
	else
	{
		::strcpy( location, "<Unknown>" );
	}

	// build module
	char module[MAX_PATH];
	::GetModuleFileName( NULL, module, ELEMENT_COUNT( module ) );

	// build a prettier version for the dialog
	if (   ((m_Type       && *m_Type      ) && !advance_snprintf( buffer, maxLen, "Type         : %s\r\n",                    m_Type        ))
		|| ((m_Expression && *m_Expression) && !advance_snprintf( buffer, maxLen, "Expression   : %s\r\n",                    m_Expression  ))
		||                                     !advance_snprintf( buffer, maxLen, "Module       : %s\r\n",                    module        )
		||                                     !advance_snprintf( buffer, maxLen, "Location     : %s\r\n",                    location      )
		||                                     !advance_snprintf( buffer, maxLen, "Fail Count   : %d (w:%d, e:%d, x:%d)\r\n", m_FailCount, ReportSys::GetTotalWarningCount(), ReportSys::GetTotalErrorCount(), ::GlobalGetUnfilteredHandledExceptionCount() )
		||                                     !advance_snprintf( buffer, maxLen, "Exe timestamp: %s\r\n\r\n",                __DATE__ " " __TIME__ )
		  )
	{
		return;
	}

	// include message if there is one
	if ( m_Message && *m_Message )
	{
		// get some vars
		const char* in = m_Message;
		const char* search = in;
		maxLen -= 2;					// leave room for trailing \r\n\0 (null term already accounted for above)

		// convert \n to \r\n - required by edit control
		if ( convertNewlines )
		{
			const char* found;
			while ( (maxLen > 0) && ((found = ::strchr( search, '\n' )) != NULL) )
			{
				int len = min( maxLen, found - in );

				// only do it if not already preceded by \r
				if ( (found == search) || (found[ -1 ] != '\r') )
				{
					// copy everything up until now
					::memcpy( buffer, in, len );

					// advance pointers
					buffer += len;
					in = found + 1;
					search = in;
					maxLen -= len;

					// terminate w/ eol
					if ( maxLen > 2 )
					{
						*buffer++ = '\r';
						*buffer++ = '\n';
						maxLen -= 2;
					}
					else
					{
						maxLen = 0;
					}
				}
				else
				{
					// just advance to the next \n
					search = found + 1;
				}
			}
		}

		// all done
		if ( maxLen > 0 )
		{
			buffer += stringtool::CopyString( buffer, in, maxLen );
		}

		// terminate
		*buffer++ = '\r';
		*buffer++ = '\n';
		*buffer++ = '\0';
	}
}

ReportSys::Context& GetAssertOutContext( void )
{
	// if we're nested two deep in asserting, just go to the debugger, the
	// original assert is outputting back into another assert.

	if ( AssertData::ms_AssertLevel > 1 )
	{
		static ReportSys::LocalContext sMiniAssertOutContext( &gDebuggerSink, false );
		return ( sMiniAssertOutContext );
	}
	else
	{
		static ReportSys::Context sAssertOutContext( &gErrorContext, "AssertOut", "Error" );
		return ( sAssertOutContext );
	}
}

// make sure the assert context always exists
struct AutoCreateContext
{
	AutoCreateContext( void )
	{
		GetAssertOutContext();
	}
};

static AutoCreateContext s_AutoCreateContext;
static kerneltool::Critical s_AssertCritical;

struct ShutdownDetector
{
	bool m_Shutdown;

	ShutdownDetector( void )	: m_Shutdown( false )  {  }
   ~ShutdownDetector( void )	{  m_Shutdown = true;  }
};

static ShutdownDetector s_ShutdownDetector;
static LONG s_InAdvanced = -1;

// return true if we want to do a breakpoint
bool AssertData :: Execute( int* ignore, bool errorReport, bool incErrorCount )
{
	// early bailout if we're too far gone
	if ( s_ShutdownDetector.m_Shutdown )
	{
		// do not do the assert dialog while we're in the middle of a fatal
		if ( ReportSys::IsFatalOccurring() )
		{
			return ( false );
		}
		return ( DoSimpleDialog() );
	}

// Open scope for thread safety lock

	bool fatal = false;
	bool rc = false;

	// remember last error
	DWORD lastError = ::GetLastError();

	// record this error
	if ( incErrorCount )
	{
		ReportSys::IncErrorCount();
	}

	{
		kerneltool::Critical::Lock locker( s_AssertCritical );

	// Try for early bailout using ignore codes. Global always overrides.

		if ( (ms_GlobalIgnoreCount < 0) || ((ignore != NULL) && (*ignore < 0)) )  // <0 means "ignore always"
		{
			return ( false );
		}
		if ( ms_GlobalIgnoreCount > 0 )  // >0 means "ignore some more"
		{
			--ms_GlobalIgnoreCount;
			return ( false );
		}
		if ( (ignore != NULL) && (*ignore > 0) )  // >0 means "ignore some more"
		{
			--*ignore;
			return ( false );
		}

	// Ok have to do actual dialog now.

		struct AutoAssert
		{
			AutoAssert( void )
			{
				++AssertData::ms_AssertLevel;
			}
		   ~AutoAssert( void )
			{
				--AssertData::ms_AssertLevel;
			}
		};

		AutoAssert autoAssert;

		// first echo to output window if requested
		if ( errorReport && !::IsInFinalExit() )
		{
			gpstring escaped;
			if ( (m_Message != NULL) && (*m_Message != '\0') )
			{
				stringtool::EscapeCharactersToTokens( m_Message, escaped );
			}
			GetAssertOutContext().OutputF(
					"\n\n"
					"***************** Expr: %s\n"
					"* ASSERT FAILED * %s%s\n"
					"***************** File: %s, Line: %d\n\n",
					m_Expression,
					!escaped.empty() ? "Mesg: " : "",
					!escaped.empty() ? escaped.c_str() : "<General>",
					m_File, m_Line );
		}

		// do not do the assert dialog while we're in the middle of a fatal
		if ( ReportSys::IsFatalOccurring() )
		{
			return ( false );
		}

		// now do either the advanced dialog if we can or simple one if it's locked out
		if ( (InterlockedIncrement( &s_InAdvanced ) == 0) && !::IsInFinalExit() )
		{
			rc = DoAdvancedDialog( fatal, ignore );
			InterlockedDecrement( &s_InAdvanced );
		}
		else
		{
			InterlockedDecrement( &s_InAdvanced );
			rc = DoSimpleDialog();
		}
	}

// Done.

	::SetLastError( lastError );

	if ( fatal )
	{
		gpfatal( "Assertion dialog aborted by user." );
	}

	return ( rc );
}

// This code is adapted from assert.c in the Visual C++ CRT source.

static const char PROGINTRO[] =   "Program: ";
static const char FILEINTRO[] =   "File: ";
static const char LINEINTRO[] =   "Line: ";
static const char EXPRINTRO[] =   "Expression: ";
static const char HELPINTRO[] =   "(Abort = quit app; Retry = debug; Ignore = continue execution)";

static const char DOTDOTDOT[]  = "...";
static const char NEWLINE[]    = "\n";
static const char DBLNEWLINE[] = "\n\n";

static const int DOTDOTDOTSZ  = ELEMENT_COUNT( DOTDOTDOT  ) - 1;
static const int NEWLINESZ    = ELEMENT_COUNT( NEWLINE    ) - 1;
static const int DBLNEWLINESZ = ELEMENT_COUNT( DBLNEWLINE ) - 1;

static const int MAXLINELEN  = 60;				// max length for line in message box
static const int ASSERTBUFSZ = MAXLINELEN * 9;	// 9 lines in message box

bool AssertData :: DoSimpleDialog( void )
{
	std::auto_ptr <ReportSys::AutoSafeMessageBox> autoSafe;
	std::auto_ptr <kerneltool::Thread::AutoPauser> autoThreadPauser;

	// only do fancy stuff if we're not too far gone
	if ( !s_ShutdownDetector.m_Shutdown )
	{
		stdx::make_auto_ptr( autoSafe, new ReportSys::AutoSafeMessageBox( true ) );
		stdx::make_auto_ptr( autoThreadPauser, new kerneltool::Thread::AutoPauser );
	}

	char * pch;
	char assertbuf[ASSERTBUFSZ] = "";
	char progname[MAX_PATH];

	/*
	 * Line 1: box intro
	 */
	if ( m_Message && *m_Message )
	{
		if ( strlen( m_Message ) >= (MAXLINELEN * 2) )
		{
			memcpy( assertbuf, m_Message, MAXLINELEN * 2 );
			assertbuf[ (MAXLINELEN * 2) + 0 ] = '.';
			assertbuf[ (MAXLINELEN * 2) + 1 ] = '.';
			assertbuf[ (MAXLINELEN * 2) + 2 ] = '.';
			assertbuf[ (MAXLINELEN * 2) + 3 ] = '\0';
		}
		else
		{
			strcpy( assertbuf, m_Message );
		}
		strcat( assertbuf, DBLNEWLINE );
	}

	/*
	 * Line 2: program
	 */
	strcat( assertbuf, PROGINTRO );

	if ( !GetModuleFileName( NULL, progname, MAX_PATH ))
		strcpy( progname, "<program name unknown>");

	pch = (char *)progname;

	/* sizeof(PROGINTRO) includes the NULL terminator */
	if ( sizeof(PROGINTRO) + strlen(progname) + NEWLINESZ > MAXLINELEN )
	{
		pch += (sizeof(PROGINTRO) + strlen(progname) + NEWLINESZ) - MAXLINELEN;
		stringtool::CopyString( pch, DOTDOTDOT, DOTDOTDOTSZ );
	}

	strcat( assertbuf, pch );
	strcat( assertbuf, NEWLINE );

	/*
	 * Line 3: file
	 */
	strcat( assertbuf, FILEINTRO );

	/* sizeof(FILEINTRO) includes the NULL terminator */
	const char* file = (m_File && *m_File) ? m_File : "<Unknown>";
	if ( sizeof(FILEINTRO) + strlen(file) + NEWLINESZ >
		 MAXLINELEN )
	{
		/* too long. use only the first part of the filename string */
		strncat( assertbuf, file, MAXLINELEN - sizeof(FILEINTRO)
				 - DOTDOTDOTSZ - NEWLINESZ );
		/* append trailing "..." */
		strcat( assertbuf, DOTDOTDOT );
	}
	else
		/* plenty of room on the line, just append the filename */
		strcat( assertbuf, file );

	strcat( assertbuf, NEWLINE );

	/*
	 * Line 4: line
	 */
	strcat( assertbuf, LINEINTRO );
	if (m_File && *m_File)
	{
		_itoa( m_Line, assertbuf + strlen(assertbuf), 10 );
	}
	else
	{
		strcat( assertbuf, "<Unknown>" );
	}
	strcat( assertbuf, DBLNEWLINE );

	/*
	 * Line 5: message
	 */
	strcat( assertbuf, EXPRINTRO );

	/* sizeof(HELPINTRO) includes the NULL terminator */

	const char* expr = (m_Expression && *m_Expression) ? m_Expression : "<Unknown>";
	if (    strlen(assertbuf) +
			strlen(expr) +
			2*DBLNEWLINESZ +
			sizeof(HELPINTRO) > ASSERTBUFSZ )
	{
		strncat( assertbuf, expr,
			ASSERTBUFSZ -
			(strlen(assertbuf) +
			DOTDOTDOTSZ +
			2*DBLNEWLINESZ +
			sizeof(HELPINTRO)) );
		strcat( assertbuf, DOTDOTDOT );
	}
	else
		strcat( assertbuf, expr );

	strcat( assertbuf, DBLNEWLINE );

	/*
	 * Line 6: help
	 */
	strcat(assertbuf, HELPINTRO);

	char titleBuf[ 100 ];
	sprintf( titleBuf, "GPCore %s Failure", m_Type ? m_Type : "<Unknown>" );

	HWND hWndParent = winx::GetFrontWindow();

	int nCode = ::MessageBox( hWndParent, assertbuf, titleBuf,
							  MB_ABORTRETRYIGNORE | MB_ICONHAND | MB_SETFOREGROUND | MB_TASKMODAL );

	if ( nCode == IDRETRY )
	{
		OutputDebugString( "User broke into debugger\n" );
		return ( true );		// breakpoint
	}

	if ( nCode == IDIGNORE )
	{
		OutputDebugString( "User ignored the assertion\n" );
		return ( false );
	}

	if ( nCode == IDABORT )
	{
		OutputDebugString( "User aborted the assertion\n" );
		exit( 'ABRT' );
	}

	return ( true );
}

bool AssertData :: DoAdvancedDialog( bool& fatal, int* ignore )
{
	// special: if atlx not constructed yet, do the simple dialog
	if ( !CanDoAdvancedDialog() )
	{
		return ( DoSimpleDialog() );
	}

	// get some vars
	int rc = 0, ignoreCount = 0, globalIgnore = 0;

	// do the dialog - scope it so it always resumes threads etc.
	{
		AssertDlg dlg( *this );
		rc = dlg.DoModal();
		ignoreCount = dlg.GetIgnoreCount();
		globalIgnore = dlg.GetGlobalIgnoreCount();
	}

	// process result
	switch ( rc )
	{
		case ( IDRETRY ):
		{
			OutputDebugString( "User broke into debugger\n" );
			return ( true );		// breakpoint
		}
		break;

		case ( IDIGNORE ):
		{
			OutputDebugString( "User ignored the assertion\n" );
			if ( ignore != NULL )
			{
				*ignore = ignoreCount;
			}
			ms_GlobalIgnoreCount = globalIgnore;
			return ( false );		// continue
		}
		break;

		case ( IDABORT ):
		{
			fatal = true;
		}
		break;

		case ( -1 ):
		{
			// failed for some reason...
			return ( DoSimpleDialog() );
		}
		break;
	}

	return ( true );
}

bool AssertData :: CanDoAdvancedDialog( void )
{
	return ( atlx::Module::DoesSingletonExist() );
}

bool AssertData :: IsAssertOccurring( void )
{
	return ( s_InAdvanced > -1 );
}

//////////////////////////////////////////////////////////////////////////////
// function gpassert_box implementation

struct AssertBoxKey
{
	const char* m_File;
	int m_Line;

	AssertBoxKey( const char* file, int line )
	{
		m_File = file;
		m_Line = line;
	}

	bool operator < ( const AssertBoxKey& other ) const
	{
		return (   (m_File < other.m_File)
				|| (!(other.m_File < m_File) && (m_Line < other.m_Line)) );
	}
};

bool gpassert_box( const char* exp, const char* msg, const char* file, int line,
				   const char* type, bool localStorage, bool errorReport,
				   bool incErrorCount, DWORD startFrame, bool dbgBreak,
				   AssertData::eStyle style )
{
	// use local storage if requested
	typedef std::list <gpstring> FileColl;
	static FileColl s_FileColl;
	if ( localStorage )
	{
		// insert and reassign
		FileColl::iterator inserted = s_FileColl.insert( s_FileColl.begin(), gpstring( msg ? msg : "" ) );
		file = inserted->c_str();
	}

	// add entry in database
	typedef std::map <AssertBoxKey, AssertInstance> AssertColl;
	static AssertColl s_AssertColl;
	std::pair <AssertColl::iterator, bool> insertrc
		= s_AssertColl.insert( std::make_pair( AssertBoxKey( file, line ), AssertInstance() ) );
	AssertInstance& inst = insertrc.first->second;
	if ( insertrc.second )
	{
		inst.m_File = file;
		inst.m_Line = line;
		inst.Register();
	}

	// do the assertion box
	++AssertData::ms_FailedCount;
	if ( startFrame == 0 )
	{
		_asm  {  mov [startFrame], ebp  }
	}
	if ( AssertData( type, exp, file, line, msg, startFrame, ++inst.m_FailCount, style ).Execute( &inst.m_IgnoreCount, errorReport, incErrorCount ) )
	{
		if ( dbgBreak )
		{
			_asm  {  int 3  }
		}
		return ( true );
	}
	return ( false );
}

//////////////////////////////////////////////////////////////////////////////

#endif // NOGPCORE

//////////////////////////////////////////////////////////////////////////////
