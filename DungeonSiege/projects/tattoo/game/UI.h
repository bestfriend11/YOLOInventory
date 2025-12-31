/*=======================================================================================

  UI

  purpose:	This is the master class which owns all the different user-interface layers.
			User input comes in here, then it is dispatched to all the user interface
			layers, according to a priority.

  author:	Bartosz Kijanka, Chad Queen

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
#ifndef __UI_H
#define __UI_H
#pragma once


class InputBinder;
class UIFrontend;
class UIMultiplayer;
class UIGame;
class UIOptions;
class UIUserConsole;
class UIShell;
class UIIntro;
class UIOutro;
class WorldMessage;


class UI : public Singleton <UI>
{
public:
	UI();
	~UI();

	void Update( double secondselapsed );
	bool UpdateShell( double seconds, int cursorX, int cursorY	);	// update the GameGui lib

	void Draw( double secondselapsed );

#	if !GP_RETAIL
	UIUserConsole&       GetUIUserConsole()			{  return ( *m_UIUserConsole       ); }
#	endif // !GP_RETAIL
	UIOptions&           GetUIOptions()				{  return ( *m_UIOptions           ); }
	UIFrontend&          GetUIFrontend()			{  return ( *m_UIFrontend          ); }
	UIMultiplayer&       GetUIMultiplayer()			{  return ( *m_UIMultiplayer       ); }
	UIGame&				 GetUIGame()				{  return ( *m_UIGame			   ); }
	UIShell&             GetGameGUI()				{  return ( *m_GameGUI             ); }

	// $$$ these helper functions are unneccessary once singleton checking support is in skrit.
FEX bool				 HasUIGame()				{  return ( m_UIGame		? true : false ); }
FEX bool				 HasUIMultiplayer()			{  return ( m_UIMultiplayer ? true : false ); }
FEX bool				 HasUIFrontend()			{  return ( m_UIFrontend	? true : false ); }
FEX bool				 HasUIOptions()				{  return ( m_UIOptions		? true : false ); }


	void OnAppActivate( bool activate );

	bool RequestQuitGame() const;		

	void HandleBroadcast( WorldMessage & msg );

private:
	// $ these objects are guaranteed to exist after UI ctor

#	if !GP_RETAIL
	UIUserConsole*			m_UIUserConsole;		// the developer-only console
#	endif // !GP_RETAIL
	UIFrontend*				m_UIFrontend;			// main menus
	UIGame*					m_UIGame;				// in-game, world etc.
	UIMultiplayer*			m_UIMultiplayer; 		// multi-player lobby
	UIOptions*				m_UIOptions;			// the game options menu
	UIIntro*				m_UIIntro;				// the game intro 
	UIOutro*				m_UIOutro;				// the end game interface
	UIShell*				m_GameGUI;

	bool					m_PreTurtlePausing;

	FUBI_SINGLETON_CLASS( UI, "This is the UI singleton; controls all the child UI components: Options, Frontend, Game, MP" );
};

#define gUI UI::GetSingleton()




class UILayer
{
public:
	
	UILayer(){};
	~UILayer(){};

	virtual void Update( double secondsElapsed ) = 0;
	virtual void Draw( double secondsElapsed ) = 0;
	virtual InputBinder & GetInputBinder() = 0;

	virtual bool IsVisible() = 0;
	virtual void SetIsVisible( bool flag ) = 0;

	virtual void OnAppActivate( bool activate ) = 0;

protected:

	virtual void PublishInterfaceToInputBinder() = 0;
	virtual void BindInputToPublishedInterface() = 0;
};


#endif
