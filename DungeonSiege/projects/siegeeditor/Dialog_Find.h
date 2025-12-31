#if !defined(AFX_DIALOG_FIND_H__9611D2B1_B08F_43E1_AC1E_5C4C76AE1CA5__INCLUDED_)
#define AFX_DIALOG_FIND_H__9611D2B1_B08F_43E1_AC1E_5C4C76AE1CA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Find.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Find dialog

class Dialog_Find : public CDialog
{
// Construction
public:
	Dialog_Find(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Find)
	enum { IDD = IDD_DIALOG_FIND };
	CEdit	m_screen_name;
	CEdit	m_scid;
	CEdit	m_description;
	CEdit	m_block_name;
	CComboBox	m_type_combo;
	CComboBox	m_content_combo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Find)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Find)
	afx_msg void OnFind();
	afx_msg void OnFindNext();
	afx_msg void OnSelectAll();
	afx_msg void OnClose();
	afx_msg void OnReplace();
	afx_msg void OnReplaceall();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_FIND_H__9611D2B1_B08F_43E1_AC1E_5C4C76AE1CA5__INCLUDED_)
