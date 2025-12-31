#pragma once
#ifndef __UIUSERCONSOLE_H
#define __UIUSERCONSOLE_H

#if !GP_RETAIL


class InputBinder;
class KeyInput;

namespace siege {
	class console;
}




#include "UI.h"




/*=======================================================================================

  UIUserConsole

  purpose:	This is the highest-priority user interface layer.  It sits on top of everything
			and has the first chance at any input.  This is the layer where the developer
			console lives.  This layer is strictly for development purposes only.

  author:	Bartosz Kijanka

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class UIUserConsole : public UILayer
{
public:
	// existence
	UIUserConsole();
	~UIUserConsole();

	void Update( double /*secondsElapsed*/ ){};
	void Draw( double /*secondsElapsed*/ ){};

	InputBinder & GetInputBinder();

	bool IsVisible() { return mIsVisible; }
	void SetIsVisible( bool flag );

	//----- InputBinder callbacks ------

	// all keys funneled through here
	bool KeyCallback( const KeyInput& key );
	bool CharCallback( wchar_t c );

	// console
	bool ConsoleToggleVisibility();
	bool ConsoleToggleMode();
	bool ConsoleClear();
	bool ConsoleScrollUp();
	bool ConsoleScrollDown();
	bool ConsoleDelete();
	bool ConsoleHome();
	bool ConsoleEnd();
	bool ConsoleLastInput();
	bool ConsoleNextInput();
	bool ConsolePreviousCommand();
	bool ConsoleNextCommand();
	bool ConsoleBackspace();
	bool ConsoleExecute();
	bool ConsoleCopyCommandLine();
	bool ConsolePasteCommandLine();
	bool ConsoleCutCommandLine();

	bool	DebugNetProbe();

	virtual void OnAppActivate( bool /*activate*/ )  {  }


protected:

	void PublishInterfaceToInputBinder();
	void BindInputToPublishedInterface();


private:

	void CheatMirrorProc( const gpstring& line );

	enum eDevConsoleMode
	{
		DCM_FULL	= 0,
		DCM_SPLIT,
	};

	bool mIsVisible;

	eDevConsoleMode	m_DevConsoleMode;

	std::auto_ptr< InputBinder > mInputBinderConsoleVisible;
	std::auto_ptr< InputBinder > mInputBinderConsoleHidden;
};


#endif // !GP_RETAIL


#endif
