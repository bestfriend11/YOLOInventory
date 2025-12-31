#if !defined(AFX_DIALOG_TRIGGER_CONDITION_PROPERTIES_H__CB463564_1E22_4144_8CBF_579E9B3ADD65__INCLUDED_)
#define AFX_DIALOG_TRIGGER_CONDITION_PROPERTIES_H__CB463564_1E22_4144_8CBF_579E9B3ADD65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Condition_Properties.h : header file
//

#include "gpcore.h"
#include "SiegeEditorTypes.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Condition_Properties dialog

class Dialog_Trigger_Condition_Properties : public CDialog
{
// Construction
public:
	Dialog_Trigger_Condition_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Condition_Properties)
	enum { IDD = IDD_DIALOG_TRIGGER_CONDITION_PROPERTIES };
	CComboBox	m_conditions;
	CTreeCtrl	m_wndParameters;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Condition_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	StringVec m_parameters;
	StringVec m_values;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Condition_Properties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnEditchangeConditionCombo();
	afx_msg void OnClickParameterTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_CONDITION_PROPERTIES_H__CB463564_1E22_4144_8CBF_579E9B3ADD65__INCLUDED_)
