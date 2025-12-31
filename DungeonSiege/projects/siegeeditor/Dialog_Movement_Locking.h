#if !defined(AFX_Dialog_Movement_Locking_H__6C0EC18D_B98B_401C_8A38_7A3621D88C7A__INCLUDED_)
#define AFX_Dialog_Movement_Locking_H__6C0EC18D_B98B_401C_8A38_7A3621D88C7A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Movement_Locking.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Movement_Locking dialog

class Dialog_Movement_Locking : public CDialog
{
// Construction
public:
	Dialog_Movement_Locking(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Movement_Locking)
	enum { IDD = IDD_DIALOG_MOVE_LOCK };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Movement_Locking)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Movement_Locking)
	afx_msg void OnRadioX();
	afx_msg void OnRadioY();
	afx_msg void OnRadioZ();
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioNone();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_Dialog_Movement_Locking_H__6C0EC18D_B98B_401C_8A38_7A3621D88C7A__INCLUDED_)
