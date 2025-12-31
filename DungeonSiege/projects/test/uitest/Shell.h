/**************************************************************************

Filename		: Shell.h

Description		: Function Prototypes/Includes for Test Application

Creation Date	: 6/30/99

**************************************************************************/


#pragma once 
#ifndef _SHELL_H_
#define _SHELL_H_


// Forward Declarations
class execution_environment;
class RapiOwner;
class Rapi;
class FuelDB;
class IMG;
class IMG_Default_File_Formats;
class UIShell;
namespace FileSys { class IFileMgr; }


#include "Smart_Pointer.h"


// Class Definitions
class Shell : public Singleton <Shell>
{
public:

// Ctor/dtor.

	Shell( HINSTANCE inst );
   ~Shell( void );

	bool Init( int cmdShow );	// create window, initialize member objects

// Methods.

	void Draw( void );			// draws the current scene
	int  Main( void );			// executes until closed

// Query.

	UIShell& GetUIShell()  {  gpassert( DoesUIShellExist() );  return ( *m_UIShell );  }
	bool DoesUIShellExist() const  {  return ( m_UIShell != NULL );  }

private:

	static LRESULT CALLBACK WndProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );

// Private variables.

	HWND				m_hWnd;			// app window handle
	HINSTANCE			m_hInstance;	// app module instance

// Base services.

	smart_pointer <execution_environment   > m_ExecutionEnv;
	smart_pointer <FileSys::IFileMgr	   > m_FileMgr;
	smart_pointer <RapiOwner			   > m_RapiOwner;
	smart_pointer <FuelDB				   > m_FuelDB;
	smart_pointer <IMG					   > m_IMG;
	smart_pointer <IMG_Default_File_Formats> m_IMG_Default_File_Formats;
	Rapi					                *m_Rapi;

// UITest services.

	smart_pointer <UIShell				   > m_UIShell;
};

#define g_Shell Shell::GetSingleton()
#define g_UIShell g_Shell.GetUIShell()

inline bool DoesShellExist  ( void )  {  return ( Shell::DoesSingletonExist() );  }
inline bool DoesUIShellExist( void )  {  return ( DoesShellExist() && g_Shell.DoesUIShellExist() );  }

#endif
