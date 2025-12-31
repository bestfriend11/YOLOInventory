#if !defined(AFX_DIALOG_STARTING_GROUP_NEW_H__0B811382_61EB_4AA9_87C3_A5920AE5E561__INCLUDED_)
#define AFX_DIALOG_STARTING_GROUP_NEW_H__0B811382_61EB_4AA9_87C3_A5920AE5E561__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Starting_Group_New.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_New dialog

class Dialog_Starting_Group_New : public CDialog
{
// Construction
public:
	Dialog_Starting_Group_New(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Starting_Group_New)
	enum { IDD = IDD_DIALOG_STARTING_GROUP_NEW };
	CEdit	m_name;
	CEdit	m_description;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Starting_Group_New)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Starting_Group_New)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTING_GROUP_NEW_H__0B811382_61EB_4AA9_87C3_A5920AE5E561__INCLUDED_)
