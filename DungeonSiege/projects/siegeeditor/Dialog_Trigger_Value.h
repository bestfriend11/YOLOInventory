#if !defined(AFX_DIALOG_TRIGGER_VALUE_H__3BE322E7_61A8_40FA_B586_9C266507F4F6__INCLUDED_)
#define AFX_DIALOG_TRIGGER_VALUE_H__3BE322E7_61A8_40FA_B586_9C266507F4F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Value.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Value dialog

class Dialog_Trigger_Value : public CDialog
{
// Construction
public:
	Dialog_Trigger_Value(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Value)
	enum { IDD = IDD_DIALOG_TRIGGER_VALUE };
	CStatic	m_type;
	CStatic	m_name;
	CEdit	m_editParameter;
	CComboBox	m_comboParameter;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Value)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Value)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_VALUE_H__3BE322E7_61A8_40FA_B586_9C266507F4F6__INCLUDED_)
