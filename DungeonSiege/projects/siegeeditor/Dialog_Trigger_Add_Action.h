#if !defined(AFX_DIALOG_TRIGGER_ADD_ACTION_H__4EDF3ED8_A01B_409E_9FC8_987F23352C60__INCLUDED_)
#define AFX_DIALOG_TRIGGER_ADD_ACTION_H__4EDF3ED8_A01B_409E_9FC8_987F23352C60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Add_Action.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Add_Action dialog

class Dialog_Trigger_Add_Action : public CDialog
{
// Construction
public:
	Dialog_Trigger_Add_Action(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Add_Action)
	enum { IDD = IDD_DIALOG_ADD_ACTION };
	CComboBox	m_actions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Add_Action)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Add_Action)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_ADD_ACTION_H__4EDF3ED8_A01B_409E_9FC8_987F23352C60__INCLUDED_)
