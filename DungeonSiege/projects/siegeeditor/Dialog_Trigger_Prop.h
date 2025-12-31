#if !defined(AFX_DIALOG_TRIGGER_PROP_H__B8170C84_6E1C_4ED9_BF7A_A708F9D74D6F__INCLUDED_)
#define AFX_DIALOG_TRIGGER_PROP_H__B8170C84_6E1C_4ED9_BF7A_A708F9D74D6F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Prop.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Prop dialog

class Dialog_Trigger_Prop : public CDialog
{
// Construction
public:
	Dialog_Trigger_Prop(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Prop)
	enum { IDD = IDD_DIALOG_TRIGGER_PROP };
	CButton	m_flipflop;
	CStatic	m_name;
	CListBox	m_listConditions;
	CListBox	m_listActions;
	CEdit	m_resetDelay;
	CButton	m_oneShot;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Prop)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RefreshLists();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Prop)
	afx_msg void OnButtonNewcondition();
	afx_msg void OnButtonEditcondition();
	afx_msg void OnButtonRemoveconditon();
	afx_msg void OnButtonNewaction();
	afx_msg void OnButtonEditaction();
	afx_msg void OnButtonRemoveaction();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_PROP_H__B8170C84_6E1C_4ED9_BF7A_A708F9D74D6F__INCLUDED_)
