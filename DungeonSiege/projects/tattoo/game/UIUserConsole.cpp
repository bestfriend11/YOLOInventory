#include "Precomp_Game.h"

#if !GP_RETAIL

#include "AppModule.h"
#include "GpConsole.h"
#include "InputBinder.h"
#include "Services.h"
#include "Siege_Console.h"
#include "Ui.h"
#include "UIGameConsole.h"
#include "UiUserConsole.h"
#include "WinX.h"
#include "DebugProbe.h"



UIUserConsole::UIUserConsole()
{
	m_DevConsoleMode = DCM_FULL;

	stdx::make_auto_ptr( mInputBinderConsoleVisible, new InputBinder( "console_visible" ) );
	stdx::make_auto_ptr( mInputBinderConsoleHidden, new InputBinder( "console_hidden" ) );

	gAppModule.RegisterBinder( mInputBinderConsoleVisible.get(), 10 );
	gAppModule.RegisterBinder( mInputBinderConsoleHidden.get(), 20 );

	gGpConsole.SetCheatMirrorCb( makeFunctor( *this, &UIUserConsole::CheatMirrorProc ) );

	PublishInterfaceToInputBinder();
	BindInputToPublishedInterface();
}


UIUserConsole::~UIUserConsole()
{
}


void UIUserConsole::SetIsVisible( bool flag )
{
	mIsVisible = flag;

	gFooterConsole.SetVisibility( mIsVisible );
	gOutputConsole.SetVisibility( mIsVisible && ( m_DevConsoleMode == DCM_SPLIT ) );

	if( m_DevConsoleMode == DCM_FULL )
	{
		// main output console
		gOutputConsole.SetSize( 1.0f, 1.0f );
		gOutputConsole.SetPosition( 0, 0 );
		gOutputConsole.SetVisibility( mIsVisible );
		gFooterConsole.SetVisibility( false );
	}
	else if( m_DevConsoleMode == DCM_SPLIT )
	{
		// main output console
		gOutputConsole.SetSize( 1.0f, 0.6f );
		gOutputConsole.SetPosition( 0, 0 );
		gOutputConsole.SetVisibility( mIsVisible );

		// footer
		gFooterConsole.SetSize( 1.0f, 0.4f );
		gFooterConsole.SetPosition( 0, 0.6f );
		gFooterConsole.SetVisibility( mIsVisible );
	}
	else
	{
		gpassertm( 0, "invalid flow" );
	}

	// user console input binders are always active...
	mInputBinderConsoleVisible->SetIsActive( flag );
	mInputBinderConsoleHidden->SetIsActive( !flag );
}


InputBinder & UIUserConsole::GetInputBinder()
{
	gpassert( mInputBinderConsoleVisible.get() );
	gpassert( mInputBinderConsoleHidden.get() );

	if( IsVisible() ) {
		return( *mInputBinderConsoleVisible );
	}
	else {
		return( *mInputBinderConsoleHidden );
	}
}




void UIUserConsole::PublishInterfaceToInputBinder()
{
	bool oldVisibility = IsVisible();

	// for the console-is-not-visible binder we just want a key to switch binders
	SetIsVisible( false );
	GetInputBinder().PublishVoidInterface( "console_toggle_visibility",	makeFunctor( *this, &UIUserConsole::ConsoleToggleVisibility ) );
	GetInputBinder().PublishVoidInterface( "console_toggle_mode",		makeFunctor( *this, &UIUserConsole::ConsoleToggleMode ) );
	GetInputBinder().PublishVoidInterface( "debug_net_probe",			makeFunctor( *this, &UIUserConsole::DebugNetProbe		) );

	// for the console-is-visible we want to publish a number of interfaces
	SetIsVisible( true );
	GetInputBinder().PublishVoidInterface( "console_toggle_visibility",	makeFunctor( *this, &UIUserConsole::ConsoleToggleVisibility ) );
	GetInputBinder().PublishVoidInterface( "console_toggle_mode",		makeFunctor( *this, &UIUserConsole::ConsoleToggleMode ) );
	GetInputBinder().PublishVoidInterface( "console_clear",				makeFunctor( *this, &UIUserConsole::ConsoleClear ) );
	GetInputBinder().PublishVoidInterface( "console_scroll_up",			makeFunctor( *this, &UIUserConsole::ConsoleScrollUp ) );
	GetInputBinder().PublishVoidInterface( "console_scroll_down",		makeFunctor( *this, &UIUserConsole::ConsoleScrollDown ) );
	GetInputBinder().PublishVoidInterface( "console_home",				makeFunctor( *this, &UIUserConsole::ConsoleHome ) );
	GetInputBinder().PublishVoidInterface( "console_end",				makeFunctor( *this, &UIUserConsole::ConsoleEnd ) );
	GetInputBinder().PublishVoidInterface( "console_last_input",		makeFunctor( *this, &UIUserConsole::ConsoleLastInput ) );
	GetInputBinder().PublishVoidInterface( "console_next_input",		makeFunctor( *this, &UIUserConsole::ConsoleNextInput ) );
	GetInputBinder().PublishVoidInterface( "console_next_command",		makeFunctor( *this, &UIUserConsole::ConsoleNextCommand ) );
	GetInputBinder().PublishVoidInterface( "console_previous_command",	makeFunctor( *this, &UIUserConsole::ConsolePreviousCommand ) );
	GetInputBinder().PublishVoidInterface( "console_backspace",			makeFunctor( *this, &UIUserConsole::ConsoleBackspace ) );
	GetInputBinder().PublishVoidInterface( "console_execute",			makeFunctor( *this, &UIUserConsole::ConsoleExecute ) );
	GetInputBinder().PublishVoidInterface( "console_copy_command_line",	makeFunctor( *this, &UIUserConsole::ConsoleCopyCommandLine ) );
	GetInputBinder().PublishVoidInterface( "console_paste_command_line",makeFunctor( *this, &UIUserConsole::ConsolePasteCommandLine ) );
	GetInputBinder().PublishVoidInterface( "console_cut_command_line",	makeFunctor( *this, &UIUserConsole::ConsoleCutCommandLine ) );

	GetInputBinder().PublishAllKeysInterface ( makeFunctor( *this, &UIUserConsole::KeyCallback ) );
	GetInputBinder().PublishAllCharsInterface( makeFunctor( *this, &UIUserConsole::CharCallback ) );

	SetIsVisible( oldVisibility );
}




void UIUserConsole::BindInputToPublishedInterface()
{
	bool oldVisibility = IsVisible();

	// bind to hidden console input binder
	SetIsVisible( false );

	if ( !gWorldOptions.GetForceRetailContent() )
	{
		GetInputBinder().BindDefaultInputsToPublishedInterfaces();
	}
	else
	{
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_SPECIAL, Keys::Q_KEY_DOWN ), "console_toggle_visibility" );
	}

	// bind to visible console input binder

	SetIsVisible( true );

	if ( !gWorldOptions.GetForceRetailContent() )
	{
		GetInputBinder().BindDefaultInputsToPublishedInterfaces();
	}
	else
	{
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_SPECIAL , Keys::Q_KEY_DOWN                        ), "console_toggle_visibility" );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_SPECIAL , Keys::Q_KEY_DOWN | Keys::Q_CONTROL      ), "console_toggle_mode"       );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_ESCAPE  , Keys::Q_KEY_DOWN                        ), "console_clear"             );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_PAGEUP  , Keys::Q_KEY_DOWN | Keys::Q_IGNORE_SHIFT ), "console_scroll_up"         );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_PAGEDOWN, Keys::Q_KEY_DOWN | Keys::Q_IGNORE_SHIFT ), "console_scroll_down"       );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_UP      , Keys::Q_KEY_DOWN                        ), "console_last_input"        );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_DOWN    , Keys::Q_KEY_DOWN                        ), "console_next_input"        );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_HOME    , Keys::Q_KEY_DOWN                        ), "console_home"              );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_TAB     , Keys::Q_KEY_DOWN                        ), "console_end"               );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_END     , Keys::Q_KEY_DOWN                        ), "console_end"               );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_LEFT    , Keys::Q_KEY_DOWN                        ), "console_next_command"      );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_RIGHT   , Keys::Q_KEY_DOWN                        ), "console_previous_command"  );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_BACK    , Keys::Q_KEY_DOWN                        ), "console_backspace"         );
		GetInputBinder().BindInputToPublishedInterface( KeyInput( Keys::KEY_RETURN  , Keys::Q_KEY_DOWN                        ), "console_execute"           );
	}

	SetIsVisible( oldVisibility );
}




bool UIUserConsole::ConsoleScrollDown()
{
	if( !gAppModule.GetShiftKey() ) {
		gOutputConsole.ScrollDown( 1 );
	}
	else {
		gOutputConsole.ScrollDown( 5 );
	}

	return( true );
}




bool UIUserConsole::ConsoleScrollUp()
{
	if( !gAppModule.GetShiftKey() ) {
		gOutputConsole.ScrollUp( 1 );
	}
	else {
		gOutputConsole.ScrollUp( 5 );
	}
	return( true );
}




bool UIUserConsole::KeyCallback( const KeyInput& key )
{
	// eat all but our precision camera keys and some other useful selections
#	if !GP_RETAIL
	switch ( Keys::MakeInt( key.GetKey() ) )
	{
		case ( Keys::KEY_INT_DIVIDE      ):
		case ( Keys::KEY_INT_NUMPAD1     ):
		case ( Keys::KEY_INT_NUMPAD2     ):
		case ( Keys::KEY_INT_NUMPAD3     ):
		case ( Keys::KEY_INT_NUMPAD5     ):
		case ( Keys::KEY_INT_INSERT      ):
		case ( Keys::KEY_INT_PAUSE       ):
		case ( Keys::KEY_INT_PRINTSCREEN ):
		{
			gAppModule.SetSkipNextChar();
			return ( false );
		}	break;
	}
#	endif // !GP_RETAIL

	return ( true );
}



bool UIUserConsole::CharCallback( wchar_t c )
{
	// add key input to console line
	gpstring consoleOutput;

	// $ note that the cast is ok - only allowing ansi input in user console...
	gGpConsole.CompoundInputLine( (char)c, consoleOutput );

	if( !consoleOutput.empty() )
	{
		gpgeneric( consoleOutput );
	}

	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );

	return true;
}



bool UIUserConsole::ConsoleToggleVisibility()
{
	if( !gDebugProbe.IsProbing() )
	{
		SetIsVisible( !IsVisible() );
	}
	return( true );
}



bool UIUserConsole::ConsoleClear()
{
	gOutputConsole.Clear();
	return( true );
}



bool UIUserConsole::ConsoleHome()
{
	gpstring console_output;
	gGpConsole.SetInputLineToHistoryIndex( 0 );
	gGpConsole.ProcessInputLine( console_output );

	if( !console_output.empty() ) {
		gpgeneric( console_output );
	}

	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );
	return( true );
}



bool UIUserConsole::ConsoleEnd()
{
	gGpConsole.SetInputLineToCompleteCommand();
	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );

	return( true );
}



bool UIUserConsole::ConsoleLastInput()
{
	gGpConsole.SetInputLineToLastHistory();
	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );
	return( true );
}



bool UIUserConsole::ConsoleNextInput()
{
	gGpConsole.SetInputLineToNextHistory();
	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );
	return( true );
}



bool UIUserConsole::ConsolePreviousCommand()
{
	gGpConsole.SetInputLineToPreviousCommand();
	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );
	return( true );
}



bool UIUserConsole::ConsoleNextCommand()
{
	gGpConsole.SetInputLineToNextCommand();
	gOutputConsole.SetInputLine( gGpConsole.GetInputLineMatch() );
	return( true );
}



bool UIUserConsole::ConsoleBackspace()
{
	return ( CharCallback( '\b' ) );
}



bool UIUserConsole::ConsoleExecute()
{
	return ( CharCallback( '\n' ) );
}



bool UIUserConsole::ConsoleCopyCommandLine()
{
	winx::SetClipboardText( gGpConsole.GetInputLine() );
	return ( true );
}


bool UIUserConsole::ConsolePasteCommandLine()
{
	gpstring str = winx::GetClipboardText();
	bool isSkrit = !str.empty() && (str[ 0 ] == '/');

	if ( !str.empty() )
	{
		// $$$ HACKA HACKA HACKA - better to insert directly and not do this
		//     waaaaacky loop shit. -sb

		gpstring::const_iterator i, begin = str.begin(), end = str.end();
		for ( i = begin ; i != end ; ++i )
		{
			// convert newlines etc. into spaces so it all goes into a single command
			if ( isSkrit && ::isspace( *i ) )
			{
				CharCallback( ' ' );
			}
			else if ( *i != '\r' )
			{
				CharCallback( *i );
			}
		}

		if ( !isSkrit )
		{
			CharCallback( '\n' );
		}
	}

	return ( true );
}


bool UIUserConsole::ConsoleCutCommandLine()
{
	winx::SetClipboardText( gGpConsole.GetInputLine() );
	gGpConsole.ClearInputLine();
	gOutputConsole.SetInputLine( gpstring::EMPTY );

	return ( true );
}


bool UIUserConsole::ConsoleToggleMode()
{
	if( m_DevConsoleMode == DCM_FULL )
	{
		m_DevConsoleMode = DCM_SPLIT;
	}
	else
	{
		m_DevConsoleMode = DCM_FULL;
	}

	SetIsVisible( IsVisible() );
	return true;
}


bool UIUserConsole :: DebugNetProbe()
{
	gDebugProbe.ToggleMode( DP_NET );
	return true;
}


void UIUserConsole :: CheatMirrorProc( const gpstring& line )
{
	gUIGameConsole.ReceiveConsoleText( ::ToUnicode( line ) );
}


#endif // !GP_RETAIL
