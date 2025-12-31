#if !defined(AFX_DIALOG_STARTING_GROUP_PROPERTIES_H__EACF6B80_223F_4BE8_881A_D46A8BF57B74__INCLUDED_)
#define AFX_DIALOG_STARTING_GROUP_PROPERTIES_H__EACF6B80_223F_4BE8_881A_D46A8BF57B74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Starting_Group_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_Properties dialog

class Dialog_Starting_Group_Properties : public CDialog
{
// Construction
public:
	Dialog_Starting_Group_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Starting_Group_Properties)
	enum { IDD = IDD_DIALOG_STARTING_GROUP_PROPERTIES };
	CListCtrl	m_levels;
	CEdit	m_screenname;
	CButton	m_dev_only;
	CStatic	m_name;
	CEdit	m_description;
	CButton	m_default;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Starting_Group_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Starting_Group_Properties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonAddLevel();
	afx_msg void OnButtonRemoveLevel();
	afx_msg void OnButtonRemoveAll();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void RefreshLevels();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTING_GROUP_PROPERTIES_H__EACF6B80_223F_4BE8_881A_D46A8BF57B74__INCLUDED_)
