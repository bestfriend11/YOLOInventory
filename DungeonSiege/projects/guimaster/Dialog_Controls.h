#if !defined(AFX_DIALOG_CONTROLS_H__BA65A246_9418_44A8_9596_A120DC9DDC70__INCLUDED_)
#define AFX_DIALOG_CONTROLS_H__BA65A246_9418_44A8_9596_A120DC9DDC70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Controls.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CDialog_Controls dialog

class CDialog_Controls : public CDialog
{
// Construction
public:
	CDialog_Controls(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Controls)
	enum { IDD = IDD_DIALOG_CONTROLS };
	CListBox	m_controls;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Controls)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Controls)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListControls();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONTROLS_H__BA65A246_9418_44A8_9596_A120DC9DDC70__INCLUDED_)
