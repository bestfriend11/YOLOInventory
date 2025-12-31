// Network_transport_test.h : main header file for the NETWORK_TRANSPORT_TEST application
//

#if !defined(AFX_NETWORK_TRANSPORT_TEST_H__E2C2CDC4_E062_11D2_B5CA_006008688288__INCLUDED_)
#define AFX_NETWORK_TRANSPORT_TEST_H__E2C2CDC4_E062_11D2_B5CA_006008688288__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CNetwork_transport_testApp:
// See Network_transport_test.cpp for the implementation of this class
//

class CNetwork_transport_testApp : public CWinApp
{
public:
	CNetwork_transport_testApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetwork_transport_testApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNetwork_transport_testApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETWORK_TRANSPORT_TEST_H__E2C2CDC4_E062_11D2_B5CA_006008688288__INCLUDED_)
