// GUIMaster.h : main header file for the GUIMASTER application
//

#if !defined(AFX_GUIMASTER_H__FBA830D1_42AF_4710_BD47_3CA040057769__INCLUDED_)
#define AFX_GUIMASTER_H__FBA830D1_42AF_4710_BD47_3CA040057769__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "GUIMasterShell.h"

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterApp:
// See GUIMaster.cpp for the implementation of this class
//

class CGUIMasterApp : public CWinApp
{
public:
	CGUIMasterApp();
	~CGUIMasterApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGUIMasterApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CGUIMasterApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	GUIMasterShell * m_pShell;

};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GUIMASTER_H__FBA830D1_42AF_4710_BD47_3CA040057769__INCLUDED_)
