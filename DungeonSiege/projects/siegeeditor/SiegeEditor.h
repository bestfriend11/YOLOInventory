// SiegeEditor.h : main header file for the SIEGEEDITOR application
//

#if !defined(AFX_SIEGEEDITOR_H__68D44B11_F6D6_4B26_9D07_4D2846A327EA__INCLUDED_)
#define AFX_SIEGEEDITOR_H__68D44B11_F6D6_4B26_9D07_4D2846A327EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


class SiegeEditorShell;


/////////////////////////////////////////////////////////////////////////////
// CSiegeEditorApp:
// See SiegeEditor.cpp for the implementation of this class
//

class CSiegeEditorApp : public CWinApp, public Singleton <CSiegeEditorApp>
{
public:
	CSiegeEditorApp();
	~CSiegeEditorApp();

	void MRUFileHandler( UINT i );

	void ShowMaximized();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSiegeEditorApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	virtual int ExitInstance();
	virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CSiegeEditorApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()

private:

	SiegeEditorShell * m_pShell;

};


#define gSiegeEditor CSiegeEditorApp::GetSingleton()

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIEGEEDITOR_H__68D44B11_F6D6_4B26_9D07_4D2846A327EA__INCLUDED_)
