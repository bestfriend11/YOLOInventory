#if !defined(AFX_DIALOG_PROGRESS_SAVE_H__DA424FB9_0985_4EB8_811F_6AC1A97A829C__INCLUDED_)
#define AFX_DIALOG_PROGRESS_SAVE_H__DA424FB9_0985_4EB8_811F_6AC1A97A829C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Progress_Save.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Progress_Save dialog

class Dialog_Progress_Save : public CDialog
{
// Construction
public:
	Dialog_Progress_Save(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Progress_Save)
	enum { IDD = IDD_DIALOG_PROGRESS_SAVE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Progress_Save)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Progress_Save)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PROGRESS_SAVE_H__DA424FB9_0985_4EB8_811F_6AC1A97A829C__INCLUDED_)
