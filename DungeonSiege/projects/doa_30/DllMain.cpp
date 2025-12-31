#include "MAXScrpt.h" 

extern void DOA_init(void);

HMODULE hInstance = NULL; 

BOOL APIENTRY 

DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 

{ 

    static BOOL controlsInit = FALSE; 

    switch (ul_reason_for_call) 

    { 

        case DLL_PROCESS_ATTACH: 

            // Hang on to this DLL's instance handle. 

            hInstance = hModule; 

            if (!controlsInit) 

            { 

                controlsInit = TRUE; 

                // Initialize Win95 controls 

                InitCommonControls(); 

            } 

            break; 

    } 

    return(TRUE); 

} 

__declspec( dllexport ) void 

LibInit() 

{ 

    // do any setup here. MAXScript is fully up at this point. 
	DOA_init();

} 

__declspec( dllexport ) const TCHAR * 

LibDescription() 

{ 

return _T("MAXscript DLX support of Physique"); 

} 

__declspec( dllexport ) ULONG 

LibVersion() 

{ 

return VERSION_3DSMAX; 

} 

