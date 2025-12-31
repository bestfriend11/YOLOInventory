#if !defined(AFX_DIALOG_STARTING_GROUP_ADD_LEVEL_H__006B95C0_1C9E_4E63_9F57_DF79A9E6BEFF__INCLUDED_)
#define AFX_DIALOG_STARTING_GROUP_ADD_LEVEL_H__006B95C0_1C9E_4E63_9F57_DF79A9E6BEFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Starting_Group_Add_Level.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Group_Add_Level dialog

class Dialog_Starting_Group_Add_Level : public CDialog
{
// Construction
public:
	Dialog_Starting_Group_Add_Level(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Starting_Group_Add_Level)
	enum { IDD = IDD_DIALOG_STARTING_GROUP_ADD_LEVEL };
	CEdit	m_level;
	CEdit	m_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Starting_Group_Add_Level)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Starting_Group_Add_Level)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTING_GROUP_ADD_LEVEL_H__006B95C0_1C9E_4E63_9F57_DF79A9E6BEFF__INCLUDED_)
