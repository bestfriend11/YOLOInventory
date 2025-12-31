#if !defined(AFX_DIALOG_TRIGGER_EDITOR_H__A480FF0E_BE1B_44FF_A8A1_1D9430E88AAC__INCLUDED_)
#define AFX_DIALOG_TRIGGER_EDITOR_H__A480FF0E_BE1B_44FF_A8A1_1D9430E88AAC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Editor.h : header file
//

#include "GoDefs.h"


/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Editor dialog

class Dialog_Trigger_Editor : public CDialog
{
// Construction
public:
	Dialog_Trigger_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Editor)
	enum { IDD = IDD_DIALOG_TRIGGER_EDITOR };
	CTreeCtrl	m_wndTriggers;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void SetupTriggerTree();	
	bool RetrieveSelectedData( Goid & object, int & index );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Editor)
	virtual void OnOK();
	afx_msg void OnNewTrigger();
	afx_msg void OnEditTrigger();
	afx_msg void OnRemoveTrigger();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_EDITOR_H__A480FF0E_BE1B_44FF_A8A1_1D9430E88AAC__INCLUDED_)
