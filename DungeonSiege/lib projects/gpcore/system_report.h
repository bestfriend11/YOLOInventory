#pragma once
#ifndef __SYSTEM_REPORT_H
#define __SYSTEM_REPORT_H




/**************************************************************************

Filename		: system_report.h

Description		: Header for system information retrieval.

Author			: Chad Queen

Creation Date	: 5/05/99

How to Use		:
---------- 
1) Create the GPSysInfo object
2) Call GetAllInfo();
3) That's it!

**************************************************************************/

#if !GP_RETAIL


class system_report
{
public:
	// existence
	system_report();
	~system_report();

	// Gets all the system information; only function you need to call.
	void Get( gpstring & info );

	gpstring GetMachineName();
	gpstring GetUser();
	gpstring GetOS();
	gpstring GetProcessor();
	gpstring GetRAMSize();
	gpstring GetIPAddress();
	gpstring GetLocalDrives();
	gpstring GetProcesses();

	// Sound information
	gpstring GetSoundInfo();

	// Display information
	gpstring GetVideoInfo();

private:

	void  DrawLine();  // Draws a line within the current string

	gpstring	FormatNumber( ULONGLONG number ); 
	DWORD	WINAPI GetFileVersionEx (	char *szFileName, DWORD * pdwVersionHi, 
										DWORD * pdwVersionLo,  
										char *szAttributes, char *szDate );
	
	// Private Variables
	gpstring		m_szSysInfo;		// Holds the master string for the computer's data
	BOOL			m_bWinNT;			// Set to TRUE if user has WinNT; used for GetProcessor()
										// information retrieval.
};


#endif // !GP_RETAIL


#endif // __SYSTEM_REPORT_H
