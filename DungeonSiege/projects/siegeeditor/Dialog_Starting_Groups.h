#if !defined(AFX_DIALOG_STARTING_GROUPS_H__740318DA_6065_441C_ABDA_C01D8AFDD7BD__INCLUDED_)
#define AFX_DIALOG_STARTING_GROUPS_H__740318DA_6065_441C_ABDA_C01D8AFDD7BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Starting_Groups.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Groups dialog

class Dialog_Starting_Groups : public CDialog
{
// Construction
public:
	Dialog_Starting_Groups(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Starting_Groups)
	enum { IDD = IDD_DIALOG_STARTING_GROUPS };
	CListBox	m_groups;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Starting_Groups)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Starting_Groups)
	afx_msg void OnButtonNew();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonDelete();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTING_GROUPS_H__740318DA_6065_441C_ABDA_C01D8AFDD7BD__INCLUDED_)
