/**************************************************************************

Filename		: CTest.h

Description		: Function Prototypes/Includes for Test Application

Creation Date	: 4/29/99

Author			: Chad Queen

**************************************************************************/


#ifndef _CTEST_H_
#define _CTEST_H_

#include "Main.h"
#include "system_information.h"
#include "ErrorLog.h"


class CTest
{
public:
	// Public Methods

	// Constructor
	CTest();

	// Destructor
	~CTest();

	// Set the current HWND
	void SetHWnd( HWND hWnd );

	// Set the current HINSTANCE
	void SetHInstance( HINSTANCE hInstance );

	// Initialize member objects
	void Init();

	// Get the user's available services and add them to the combo box
	void TestStuff();	


	// Public Variables
	GPSysInfo	*m_SysInfo;
	ErrorLog	*m_ErrorLog;
	

private:
	// Private Methods

	
	// Private Variables
	HWND		m_hWnd;
	HINSTANCE	m_hInstance;
};

#endif