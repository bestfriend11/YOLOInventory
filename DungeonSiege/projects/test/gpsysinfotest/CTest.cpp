/**************************************************************************

Filename		: CTest.cpp

Description		: Test Application Class

Creation Date	: 4/29/99

Author			: Chad Queen

**************************************************************************/


// Includes
#include "CTest.h"
#include "Main.h"
#include "InitWin.h"


/**************************************************************************

Function		: CTest()

Purpose			: Constructor 

Returns			: No return type

Author			: CQ

**************************************************************************/
CTest::CTest()
{
	m_SysInfo	= NULL;
	m_ErrorLog	= NULL;
}


/**************************************************************************

Function		: ~CTest()

Purpose			: Destructor

Returns			: No return type

Author			: CQ

**************************************************************************/
CTest::~CTest()
{	
	if ( m_SysInfo )
		delete m_SysInfo;
	if ( m_ErrorLog )
		delete m_ErrorLog;
}


/**************************************************************************

Function		: Init()

Purpose			: Initializes member objects

Returns			: Void

Author			: CQ

**************************************************************************/
void CTest::Init()
{	
	std::string szSysInfo;

	if ( !m_SysInfo ) {
		m_SysInfo = new GPSysInfo();
		m_SysInfo->GetAllInfo( &szSysInfo );
	}

	if ( !m_ErrorLog ) {
		m_ErrorLog = new ErrorLog();
		m_ErrorLog->CreateLog();
		m_ErrorLog->AppendLog( szSysInfo );
		m_ErrorLog->WriteLog();
	}

	int j = 0;
	int i = 100/j;
}


/**************************************************************************

Function		: SetHWnd()

Purpose			: Sets the current window handle

Returns			: Void

Author			: CQ

**************************************************************************/
void CTest::SetHWnd( HWND hWnd )
{
	m_hWnd = hWnd;
}


/**************************************************************************

Function		: SetHInstance()

Purpose			: Sets the current window instance

Returns			: Void

Author			: CQ

**************************************************************************/
void CTest::SetHInstance( HINSTANCE hInstance )
{
	m_hInstance = hInstance;
}


/**************************************************************************

Function		: TestStuff()

Purpose			: Tests some stuff

Returns			: Void

Author			: CQ

**************************************************************************/
void CTest::TestStuff()
{
}