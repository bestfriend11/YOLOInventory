/**************************************************************************

Filename		: system_report.cpp

Description		: Retrieves information about a specific computer to a
				  string. See HEADER FILE to see steps for use.

Creation Date	: 5/05/99

**************************************************************************/


// Include Files

#include "precomp_gpcore.h"

#if !GP_RETAIL

#include "system_report.h"

#pragma warning ( push, 1 )		// ignore any warnings or changes that win32 has
#include "mmsystem.h"
#include <tlhelp32.h>
#include <LMCONS.H>
#include <dsound.h>
#include <math.h>
#include <ddraw.h>
#include <limits>
#pragma warning ( pop )			// back to normal

#pragma comment(lib, "version")

// DirectSound Enumeration Callback
BOOL CALLBACK DSEnumCallback( LPGUID lpGuid, LPCSTR lpstrDescription, 
							  LPCSTR lpstrModule, LPVOID lpData );

// Calculate the power 
ULONGLONG power( ULONGLONG x, ULONGLONG y );


const unsigned int MAXCHARS = 1024*4;




/**************************************************************************

Function		: system_report()

Purpose			: Initializes member variables/objects

Returns			: No return value

**************************************************************************/
system_report::system_report()
{
	m_bWinNT = FALSE;
}


/**************************************************************************

Function		: ~system_report()

Purpose			: Destructor, deallocates and destorys uneeded objects
				  and memory

Returns			: No return value

**************************************************************************/
system_report::~system_report()
{
}


/**************************************************************************

Function		: Get()

Purpose			: The only function the programmer needs to call to get
				  the master string with all the user's information

Returns			: void 

**************************************************************************/
void system_report::Get( gpstring & output )
{	
	output += "======================\n";
	output += "system information    \n";
	output += "======================\n";

	GetMachineName	();
	GetUser			();
	GetOS			();
	GetProcessor	();
	GetRAMSize		();
	GetIPAddress	();
	GetLocalDrives	();
	GetSoundInfo	();
	GetVideoInfo	();
	GetProcesses	();

	output += m_szSysInfo;
}


/**************************************************************************

Function		: GetMachineName()

Purpose			: Gets the computer name for the current computer

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetMachineName()
{
	char			szComputerName[MAX_COMPUTERNAME_LENGTH + 1 ];
	char			szFinal[MAXCHARS]								= "Machine Name	: ";
	DWORD			nSize											= MAX_COMPUTERNAME_LENGTH + 1;
	HRESULT			hRet;
	gpstring		MachineName;
	
	// Get the computers name 
	hRet = GetComputerName( szComputerName, &nSize );
	if ( hRet == 0 )
		::strcpy( szComputerName, "<Failed>" );
	
	// Copy to the main string
	strcat( szComputerName, "\n" );
	strcat( szFinal, szComputerName );
	

	m_szSysInfo.append( szFinal );
	MachineName.assign( szComputerName );
	DrawLine();

	return MachineName;
}


/**************************************************************************

Function		: GetUser()

Purpose			: Gets the user name the computer is currently registered
				  under

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetUser()
{
	char		szUserName[ UNLEN + 1 ];
	char		szFinal[MAXCHARS]		= "User Name	: ";
	DWORD		nSize					= UNLEN + 1;
	HRESULT		hRet;
	gpstring	UserName;
	
	// Get the user name
	hRet = GetUserName( szUserName, &nSize );
	if ( hRet == 0 )
		::strcpy( szUserName, "<Failed>" );
	
	// Copy to the main string
	strcat( szUserName, "\n" );
	strcat( szFinal, szUserName );
	m_szSysInfo.append( szFinal );
	DrawLine();

	UserName.assign( szUserName );
	return UserName;
}


/**************************************************************************

Function		: GetOS()

Purpose			: Retrieves the OS name, full version number, build number,
				  and the current ( if any ) service pack number installed

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetOS()
{
	OSVERSIONINFO	OSInfo;
	gpstring		OSName;

	// Fill in the OS information structure
	OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx( &OSInfo );
	
	m_szSysInfo.append( "OS Information: \n" );
	DrawLine();

	// Get the base platform information
	switch ( OSInfo.dwPlatformId ) {
	case VER_PLATFORM_WIN32s :
		OSName.append( "Windows 3.1 " );
		break;
	case VER_PLATFORM_WIN32_WINDOWS:
		OSName.append( "Windows 95/98 " );
		break;
	case VER_PLATFORM_WIN32_NT:
		OSName.append( "Windows NT " );
		m_bWinNT = TRUE;
		break;
	}
	
	char szTemp[MAXCHARS];
		
	// Get the version number
	sprintf( szTemp, "%d", OSInfo.dwMajorVersion );
	OSName.append( szTemp );
	OSName.append( "." );
	sprintf( szTemp, "%d", OSInfo.dwMinorVersion );
	OSName.append( szTemp );

	// Get the build number
	OSName.append( " Build " );
	sprintf( szTemp, "%d", OSInfo.dwBuildNumber );
	OSName.append( szTemp );
	OSName.append( " " );
	
	// Get the Service Pack information ( if any exists )
	OSName.append( OSInfo.szCSDVersion );
	
	m_szSysInfo.append( OSName );
	m_szSysInfo.append( "\n\n" );

	return OSName;
}


/**************************************************************************

Function		: GetProcessor()

Purpose			: Gets processor type and speed

Returns			: gpstring

**************************************************************************/

const int PENTIUM1 = 5;

gpstring system_report::GetProcessor()
{
	gpstring ProcessorType;

	// Get generic system information
	SYSTEM_INFO SystemInfo;
	GetSystemInfo( &SystemInfo );
	
	char szTemp[MAXCHARS];	// Used to hold temporary string data
	BOOL bCPUfound = FALSE;	// Used to make sure processor type is returned

	// Get Processor Type 
	m_szSysInfo.append( "CPU Information: \n" );	
	DrawLine();


	// Calculate the Intel CPU type
	if ( SystemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) { 
		ProcessorType.append( "Intel " );
		switch ( SystemInfo.wProcessorLevel  ) {
		case 3:
			ProcessorType.append( "80386" );
			bCPUfound = TRUE;
			break;
		case 4:
			ProcessorType.append( "80486" );
			bCPUfound = TRUE;
			break;
		default:
			if ( SystemInfo.wProcessorLevel >= PENTIUM1 ) {
				sprintf( szTemp, "Pentium %d", SystemInfo.wProcessorLevel - PENTIUM1 + 1 );
				ProcessorType.append( szTemp );
				bCPUfound = TRUE;
			}			
			break;
		}		
	}

	// Calculate for possible alternate processors in WinNT machines
	if ( m_bWinNT )	{
		switch ( SystemInfo.wProcessorArchitecture ) {
		case PROCESSOR_ARCHITECTURE_MIPS:
			ProcessorType.append( "MIPS CPU" );
			break;
		case PROCESSOR_ARCHITECTURE_ALPHA:
			ProcessorType.append( "Alpha CPU" );
			break;
		case PROCESSOR_ARCHITECTURE_PPC:
			ProcessorType.append( "PPC CPU" );
			break;
		case PROCESSOR_ARCHITECTURE_UNKNOWN:
			ProcessorType.append( "Unknown CPU" );
			break;
		}
		bCPUfound = TRUE;
	}

	// For Win95 Computers that do not calculate the wProcessorLevel value
	if ( bCPUfound == FALSE ) {
		switch ( SystemInfo.dwProcessorType ) {
		case PROCESSOR_INTEL_386:
			ProcessorType.append( "Intel 80386" );
			break;
		case PROCESSOR_INTEL_486:
			ProcessorType.append( "Intel 80486" );
			break;
		case PROCESSOR_INTEL_PENTIUM:
			ProcessorType.append( "Intel Pentium" );
			break;
		default:
			sprintf( szTemp, "%d", SystemInfo.dwProcessorType );
			ProcessorType.append( szTemp );
			break;
		}
	}

	// Calculate CPU Speed
	sprintf( szTemp, "\nNumber of Processors	: %d", SystemInfo.dwNumberOfProcessors );
	ProcessorType.append( szTemp );
	
	sprintf( szTemp, "\nAllocation Granularity	: %d", SystemInfo.dwAllocationGranularity );
	ProcessorType.append( szTemp );

	sprintf( szTemp, "\nPage Size		: %d", SystemInfo.dwPageSize );
	ProcessorType.append( szTemp );

	sprintf( szTemp, "\nActive Processor Mask	: %d", SystemInfo.dwActiveProcessorMask );
	ProcessorType.append( szTemp );
	
	m_szSysInfo.append( ProcessorType );
	m_szSysInfo.append( "\n\n" );

	return ProcessorType;
}


/**************************************************************************

Function		: GetRAMSize()

Purpose			: Gets the amount of RAM a user has

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetRAMSize()
{
	MEMORYSTATUS	MemoryStatus;
	GlobalMemoryStatus( &MemoryStatus );

	gpstring		RAMSize;
	
	m_szSysInfo.append( "RAM Information: \n" );
	DrawLine();

	RAMSize.append( "Total Physical Memory ( in bytes ): " );
	RAMSize.append( FormatNumber( MemoryStatus.dwTotalPhys ) );
	RAMSize.append( "\n" );

	RAMSize.append( "Total Virtual  Memory ( in bytes ): " );
	RAMSize.append( FormatNumber( MemoryStatus.dwTotalVirtual ) );
	RAMSize.append( "\n" );
	
	m_szSysInfo.append( RAMSize );
	m_szSysInfo.append( "\n" );

	return RAMSize;
}


/**************************************************************************

Function		: GetIPAddress()

Purpose			: Gets the user's current IP address

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetIPAddress()
{
	gpstring IPAddress = SysInfo::GetIPAddress();

	//strcpy(g_machine.szIPAddresses, string);
	m_szSysInfo.append( "IP Address:\n" );
	DrawLine();
	m_szSysInfo.append( IPAddress );
	m_szSysInfo.append( "\n\n" );

	return IPAddress;
}


/**************************************************************************

Function		: GetLocalDrives()

Purpose			: Gets local and logical drives and their information

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetLocalDrives()
{
	char		szDrives[MAXCHARS];
	DWORD		dwDrives = 0;
	gpstring	LocalDrives;

	dwDrives = GetLogicalDrives();
	
	GetLogicalDriveStrings( MAXCHARS, szDrives );
	
	LocalDrives.append( "Logical Drive Information: \n" );

	DrawLine();
	LocalDrives.append( "Available Drives: \n" );
	
	int		currChar = 0;
	char	szTemp[MAXCHARS];
	
	// Parse through all available drives
	for ( ; ; ) {
		strcpy( szTemp, szDrives );
		if ( !strcmp( "", szTemp ) )
			break;
		currChar++;
		strcpy( szDrives, szDrives+((strlen(szTemp)+1))*currChar);
		LocalDrives.append( szTemp );

		// Get information about selected drive
		LocalDrives.append( " Type: " );
		UINT uiDriveType;
		uiDriveType = GetDriveType( szTemp );
		switch ( uiDriveType ) {
		case DRIVE_UNKNOWN:
			LocalDrives.append( "Unknown" );
			break;
		case DRIVE_NO_ROOT_DIR:
			LocalDrives.append( "No Root Dir" );
			break;
		case DRIVE_REMOVABLE:
			LocalDrives.append( "Removable Drive" );
			break;
		case DRIVE_FIXED:
			LocalDrives.append( "Fixed Drive" );
			break;
		case DRIVE_REMOTE:
			LocalDrives.append( "Remote Drive" );
			break;
		case DRIVE_CDROM:
			LocalDrives.append( "CD-ROM" );
			break;
		case DRIVE_RAMDISK:
			LocalDrives.append( "RAM Disk" );
			break;
		}

		ULARGE_INTEGER FreeBytesAvailableToCaller = { 0, 0 };
		ULARGE_INTEGER TotalNumberOfBytes = { 0, 0 };
		ULARGE_INTEGER TotalNumberOfFreeBytes = { 0, 0 };
		
		HRESULT hRet=0;
		if( uiDriveType != DRIVE_REMOVABLE ) {
			hRet = GetDiskFreeSpaceEx( szTemp, &FreeBytesAvailableToCaller, &TotalNumberOfBytes,
								&TotalNumberOfFreeBytes );
		}

		// Is there Disk space information available
		if ( hRet != 0 ) {
			LocalDrives.append( "\nFree Bytes Available to Caller: " );
			LocalDrives.append( FormatNumber( FreeBytesAvailableToCaller.QuadPart ) );
			LocalDrives.append( "\nTotal Number of Bytes         : " );
			LocalDrives.append( FormatNumber( TotalNumberOfBytes.QuadPart ) );
			LocalDrives.append( "\nTotal Number of Free Bytes    : " );
			LocalDrives.append( FormatNumber( TotalNumberOfFreeBytes.QuadPart ) );
		}	
		m_szSysInfo.append( LocalDrives );
		m_szSysInfo.append( "\n\n" );
	}

	return LocalDrives;
}


/**************************************************************************

Function		: GetFileVersionEx()

Purpose			: Gets a specific file version from a file

Returns			: void

**************************************************************************/
DWORD WINAPI system_report::GetFileVersionEx(char *szFileName, DWORD * pdwVersionHi, DWORD * pdwVersionLo, char *szAttributes, char *szDate )
{
	void *				buffer;
	VS_FIXEDFILEINFO *	verinfo;
	DWORD				dwSize;
	char				string[1024];

	if (szFileName==NULL)
	{
		szFileName=string;
		GetModuleFileName(NULL, string, sizeof(string));
	}

	dwSize=GetFileVersionInfoSize(szFileName, 0);
	if (!dwSize)
		return 0;

	buffer=malloc(dwSize);
	if (!buffer)
		return 0;

	if (!GetFileVersionInfo(szFileName, 0, dwSize, buffer))
	{
		free(buffer);
		return 0;
	}

	if (!VerQueryValue(buffer, "\\", (void **)&verinfo, (UINT *)&dwSize))
	{
		free(buffer);
		return 0;
	}

	if (pdwVersionHi)
		*pdwVersionHi = verinfo->dwFileVersionMS;

	if (pdwVersionLo)
		*pdwVersionLo = verinfo->dwFileVersionLS;

	string[0]=0;
	if (verinfo->dwFileFlagsMask & VS_FF_PRERELEASE)
	{
		if (verinfo->dwFileFlags & VS_FF_PRERELEASE)
			strcat(string, "Beta ");
		else
			strcat(string, "Final ");
	}
	if (verinfo->dwFileFlagsMask & VS_FF_DEBUG)
	{
		if (verinfo->dwFileFlags & VS_FF_DEBUG)
			strcat(string, "Debug");
		else
			strcat(string, "Release");
	}

	if (verinfo->dwFileFlagsMask & VS_FF_PATCHED)
	{
		if (verinfo->dwFileFlags & VS_FF_PATCHED)
		{
			if (string[0])
				strcat(string, ", ");
			strcat(string, "Patched");
		}
	}

	if (verinfo->dwFileFlagsMask & VS_FF_PRIVATEBUILD)
	{
		if (verinfo->dwFileFlags & VS_FF_PRIVATEBUILD)
		{
			if (string[0])
				strcat(string, ", ");
			strcat(string, "Private Build");
		}
	}

	if (verinfo->dwFileFlagsMask & VS_FF_SPECIALBUILD)
	{
		if (verinfo->dwFileFlags & VS_FF_SPECIALBUILD)
		{
			if (string[0])
				strcat(string, ", ");
			strcat(string, "Special Build");
		}
	}
	strcpy(szAttributes, string);


	char		szPath[MAX_PATH];
	HANDLE		hFile;

	hFile=INVALID_HANDLE_VALUE;
	//hFile=CreateFile(szFileName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		GetWindowsDirectory(szPath, sizeof(szPath));
		strcat(szPath, "\\");
		strcat(szPath, szFileName);
		hFile=CreateFile(szPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			GetSystemDirectory(szPath, sizeof(szPath));
			strcat(szPath, "\\");
			strcat(szPath, szFileName);
			hFile=CreateFile(szPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			
			if (hFile == INVALID_HANDLE_VALUE)
			{
				HINSTANCE	hInst=LoadLibraryEx(szFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
				if (hInst)
				{
					GetModuleFileName(hInst, szPath, sizeof(szPath));
					FreeLibrary(hInst);
					hFile=CreateFile(szPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
				}
			}
		}
	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fi;
		if (GetFileInformationByHandle(hFile, &fi))
		{
			SYSTEMTIME st;
			FILETIME lft;
			FileTimeToLocalFileTime(&fi.ftLastWriteTime, &lft);
			FileTimeToSystemTime(&lft, &st);

			wsprintf(szDate, "%d/%02d/%02d %2d:%02d%c",
				st.wMonth,
				st.wDay,
				st.wYear,
				((st.wHour+11)%12)+1,
				st.wMinute,
				st.wHour>11 ? 'p' : 'a');			
		}

		CloseHandle(hFile);
	}

	free(buffer);

	return *pdwVersionHi;
}


/**************************************************************************

Function		: GetSoundInfo()

Purpose			: Gets all the sound card information for a computer

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetSoundInfo()
{
	gpstring SoundInfo;

	m_szSysInfo.append( "Sound Card Information: \n" );	
	DrawLine();
	m_szSysInfo.append( "Sound Driver List: \n");
//	if ( !CHECKDX( DirectSoundEnumerate( ( LPDSENUMCALLBACK )DSEnumCallback, &SoundInfo ) ) )
//		return "";

	m_szSysInfo.append( SoundInfo );
	m_szSysInfo.append( "\n\n" );

	return SoundInfo;
}


/**************************************************************************

Function		: DSEnumCallback()

Purpose			: Callback for DirectSoundEnumerate

Returns			: BOOL CALLBACK

**************************************************************************/
BOOL CALLBACK DSEnumCallback( LPGUID /*lpGuid*/, LPCSTR lpstrDescription, 
							  LPCSTR /*lpstrModule*/, LPVOID lpData )
{
	gpstring *szSysInfo = ( gpstring *)lpData;	
	szSysInfo->append( lpstrDescription );
	szSysInfo->append( "\n" );
	return TRUE;
}


/**************************************************************************

Function		: GetVideoInfo()

Purpose			: Gets video card information from the computer.

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetVideoInfo()
{
	LPDIRECTDRAW	lpDD;
	LPDIRECTDRAW4	lpDD4;
	gpstring		VideoInfo;

	DirectDrawCreate( NULL, &lpDD, NULL );
	lpDD->QueryInterface( IID_IDirectDraw4, ( LPVOID *)&lpDD4 );

	DDDEVICEIDENTIFIER dID;
	lpDD4->GetDeviceIdentifier( &dID, 0 );
	
	// Get the Video Card Capabilities
	DDCAPS driverCaps;
	DDCAPS softwareCaps;
	driverCaps.dwSize	= sizeof( DDCAPS );
	softwareCaps.dwSize = sizeof( DDCAPS );
	lpDD->GetCaps( &driverCaps, &softwareCaps );
	
	// Release the DirectDraw object
	lpDD->Release();
	lpDD4->Release();

	char szTemp[MAXCHARS];
	m_szSysInfo.append( "Display Adapter Information: \n" );
	DrawLine();
	sprintf( szTemp, "Driver Name		: %s\n", dID.szDriver );
	VideoInfo.append( szTemp );
	sprintf( szTemp, "Driver Information	: %s\n", dID.szDescription );
	VideoInfo.append( szTemp );
	sprintf( szTemp, "Driver Version		: %u\n", dID.liDriverVersion );
	VideoInfo.append( szTemp );
	VideoInfo.append( "Total Video Memory	: " );
	VideoInfo.append( FormatNumber( driverCaps.dwVidMemTotal ) );
	VideoInfo.append( "\nFree Video Memory	: " );
	VideoInfo.append( FormatNumber( driverCaps.dwVidMemFree ) );
	VideoInfo.append( "\n" );

	m_szSysInfo.append( VideoInfo );
	m_szSysInfo.append( "\n\n" );

	return VideoInfo;
}


/**************************************************************************

Function		: DrawLine()

Purpose			: Adds a line to the main string

Returns			: void

**************************************************************************/
void system_report::DrawLine()
{
	m_szSysInfo.append( "--------------------------------------------\n" );
}


/**************************************************************************

Function		: FormatNumber()

Purpose			: Takes a large number and converts it to a string with
				  commas.

Returns			: void

**************************************************************************/
gpstring system_report::FormatNumber( ULONGLONG number ) 
{
	// get null-terminated buffer, start out right before term
	char buffer[ 40 ];
	char* iter = buffer + ELEMENT_COUNT( buffer ) - 1;
	*iter-- = '\0';

	// loop over each digit, prepending as we go
	int comma = 0;
	for ( ; ; )
	{
		// except for first position, put a comma every third time
		if ( ++comma == 4 )
		{
			*iter-- = ',';
			comma = 1;
		}

		// put digit in there
		*iter = scast <char> ( scast <int> ( number % 10 ) + '0' );
		if ( (number /= 10) == 0 )
		{
			break;
		}

		--iter;
	}

	return ( iter );
}


/**************************************************************************

Function		: GetProcesses();

Purpose			: Lists all the processes that are running at a given time.

Returns			: gpstring

**************************************************************************/
gpstring system_report::GetProcesses()
{
	HANDLE			hSnapshot;
	PROCESSENTRY32	pEntry;
	HRESULT			hRet;
	char			szTemp[MAXCHARS];
	gpstring		ProcessList;

	hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	hRet = Process32First( hSnapshot, &pEntry );
	if ( hRet == FALSE )
		return "";

	m_szSysInfo.append( "\nRunning Processes:\n");
	DrawLine();
	sprintf( szTemp, "%s\n", pEntry.szExeFile );
	ProcessList.append( szTemp );
	
	while ( hRet != FALSE ) {
		hRet = Process32Next( hSnapshot, &pEntry );
		sprintf( szTemp, "%s\n", pEntry.szExeFile );
		ProcessList.append( szTemp );	
	}

	m_szSysInfo.append( ProcessList );

	return ProcessList;
}


/**************************************************************************

Function		: power()

Purpose			: Takes the base number and the power and figures out its
				  value in an unsigned long long format.

Returns			: ULONGLONG

**************************************************************************/
ULONGLONG power( ULONGLONG x, ULONGLONG y )
{
	ULONGLONG tempX = x;
	for ( int i = 0; i < y; i++ )
		x *= tempX;
	return x;
}


#endif // !GP_RETAIL