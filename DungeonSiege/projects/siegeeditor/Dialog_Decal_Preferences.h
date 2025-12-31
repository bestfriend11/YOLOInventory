#if !defined(AFX_DIALOG_DECAL_PREFERENCES_H__554FDF22_0646_499B_82D6_BF0014080E3E__INCLUDED_)
#define AFX_DIALOG_DECAL_PREFERENCES_H__554FDF22_0646_499B_82D6_BF0014080E3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Decal_Preferences.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Decal_Preferences dialog

class Dialog_Decal_Preferences : public CDialog
{
// Construction
public:
	Dialog_Decal_Preferences(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Decal_Preferences)
	enum { IDD = IDD_DIALOG_DECAL_PREFERENCES };
	CEdit	m_vertical;
	CEdit	m_horizontal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Decal_Preferences)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Decal_Preferences)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeEditHorizontal();
	afx_msg void OnChangeEditVertical();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_DECAL_PREFERENCES_H__554FDF22_0646_499B_82D6_BF0014080E3E__INCLUDED_)
