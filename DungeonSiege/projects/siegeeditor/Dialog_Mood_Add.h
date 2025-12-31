#if !defined(AFX_DIALOG_MOOD_ADD_H__3684E77E_289F_41B6_8FC7_6450ACB4D9B7__INCLUDED_)
#define AFX_DIALOG_MOOD_ADD_H__3684E77E_289F_41B6_8FC7_6450ACB4D9B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_Add.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Add dialog

class Dialog_Mood_Add : public CDialog
{
// Construction
public:
	Dialog_Mood_Add(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_Add)
	enum { IDD = IDD_DIALOG_MOOD_ADD };
	CEdit	m_Name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_Add)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_Add)
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_ADD_H__3684E77E_289F_41B6_8FC7_6450ACB4D9B7__INCLUDED_)
