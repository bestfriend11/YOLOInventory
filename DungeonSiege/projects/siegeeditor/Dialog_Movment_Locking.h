#if !defined(AFX_DIALOG_MOVMENT_LOCKING_H__6C0EC18D_B98B_401C_8A38_7A3621D88C7A__INCLUDED_)
#define AFX_DIALOG_MOVMENT_LOCKING_H__6C0EC18D_B98B_401C_8A38_7A3621D88C7A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Movment_Locking.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Movment_Locking dialog

class Dialog_Movment_Locking : public CDialog
{
// Construction
public:
	Dialog_Movment_Locking(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Movment_Locking)
	enum { IDD = IDD_DIALOG_MOVE_LOCK };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Movment_Locking)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Movment_Locking)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOVMENT_LOCKING_H__6C0EC18D_B98B_401C_8A38_7A3621D88C7A__INCLUDED_)
