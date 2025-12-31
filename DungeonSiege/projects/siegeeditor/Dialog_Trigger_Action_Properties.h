#if !defined(AFX_DIALOG_TRIGGER_ACTION_PROPERTIES_H__578918E1_6865_4E8E_B95F_B6DA2FBB227B__INCLUDED_)
#define AFX_DIALOG_TRIGGER_ACTION_PROPERTIES_H__578918E1_6865_4E8E_B95F_B6DA2FBB227B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Action_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Action_Properties dialog

#include "SiegeEditorShell.h"

class Dialog_Trigger_Action_Properties : public CDialog
{
// Construction
public:
	Dialog_Trigger_Action_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Action_Properties)
	enum { IDD = IDD_DIALOG_TRIGGER_ACTION_PROPERTIES };
	CButton	m_whenfalse;
	CTreeCtrl	m_wndParameters;
	CComboBox	m_actions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Action_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	StringVec m_parameters;
	StringVec m_values;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Action_Properties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnEditchangeActionCombo();
	afx_msg void OnClickParameterTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_ACTION_PROPERTIES_H__578918E1_6865_4E8E_B95F_B6DA2FBB227B__INCLUDED_)
