#if !defined(AFX_DIALOG_TRIGGER_ADD_CONDITION_H__B63874B6_8C88_41B3_879F_5272D8DB1C09__INCLUDED_)
#define AFX_DIALOG_TRIGGER_ADD_CONDITION_H__B63874B6_8C88_41B3_879F_5272D8DB1C09__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Add_Condition.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Add_Condition dialog

class Dialog_Trigger_Add_Condition : public CDialog
{
// Construction
public:
	Dialog_Trigger_Add_Condition(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Add_Condition)
	enum { IDD = IDD_DIALOG_ADD_CONDITION };
	CComboBox	m_conditions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Add_Condition)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Add_Condition)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_ADD_CONDITION_H__B63874B6_8C88_41B3_879F_5272D8DB1C09__INCLUDED_)
