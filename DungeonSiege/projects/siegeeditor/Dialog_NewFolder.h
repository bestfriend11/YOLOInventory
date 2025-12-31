#if !defined(AFX_DIALOG_NEWFOLDER_H__5EB27C7C_B08B_457F_91DC_D347D830E040__INCLUDED_)
#define AFX_DIALOG_NEWFOLDER_H__5EB27C7C_B08B_457F_91DC_D347D830E040__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_NewFolder.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_NewFolder dialog

class Dialog_NewFolder : public CDialog
{
// Construction
public:
	Dialog_NewFolder(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_NewFolder)
	enum { IDD = IDD_DIALOG_NEWFOLDER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_NewFolder)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_NewFolder)
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NEWFOLDER_H__5EB27C7C_B08B_457F_91DC_D347D830E040__INCLUDED_)
