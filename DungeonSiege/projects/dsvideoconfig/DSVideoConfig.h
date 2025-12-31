//////////////////////////////////////////////////////////////////////////////
//
// File     :  DSVideoConfig.cpp
// Author(s):  Chad Queen, James Loe
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "gpcore.h"

// Dialog Methods

	// Callback 
	BOOL CALLBACK DialogVideoConfigProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

	// Initialization
	void Init( HWND hDlg );

	void		TranslateDriverDescToUnicode( gpwstring & sDriverDesc );
	void		TranslateDriverDescToAnsi( gpwstring & sDriverDesc );
	
	gpwstring	GetResourceString( unsigned int id );

	// Query Methods
	void GetDriver		( HWND hDlg, gpwstring & sDriver );
	void GetResolution	( HWND hDlg, char * szResolution );
	void GetShadow		( HWND hDlg, char * szShadow );
	void GetFilter		( HWND hDlg, char * szFilter );	

	// Manipulation
	void AddDriver			( HWND hDlg, const char * szDriver );
	void AddResolution		( HWND hDlg, const char * szResolution );
	void AddShadowOption	( HWND hDlg, const char * szShadow );
	void AddFilterOption	( HWND hDlg, const char * szFilter );
	void ClearDrivers		( HWND hDlg );
	void ClearResolutions	( HWND hDlg );
	void SelectDriver		( HWND hDlg, gpwstring & sDriver );
	void SelectResolution	( HWND hDlg, const char * szResolution );
	void SelectShadow		( HWND hDlg, const char * szShadow );
	void SelectFilter		( HWND hDlg, const char * szFilter );

	// Events
	void NewDriverSelected( HWND hDlg );
	void NewResolutionSelected( HWND hDlg );
	void NewShadowSelected( HWND hDlg );
	void NewFilterSelected( HWND hDlg );
	

// Config Methods
void SaveConfiguration( HWND hDlg );
void TestConfiguration( HWND hDlg );