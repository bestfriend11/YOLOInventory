#if !defined(AFX_DIALOG_TRIGGER_PROPERTIES_H__96496107_7E7E_4EBB_B019_D5DD74DA779C__INCLUDED_)
#define AFX_DIALOG_TRIGGER_PROPERTIES_H__96496107_7E7E_4EBB_B019_D5DD74DA779C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Trigger_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Properties dialog

class Dialog_Trigger_Properties : public CDialog
{
// Construction
public:
	Dialog_Trigger_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Trigger_Properties)
	enum { IDD = IDD_DIALOG_TRIGGER_PROPERTIES };
	CButton	m_oneShot;
	CEdit	m_resetDelay;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Trigger_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Trigger_Properties)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_TRIGGER_PROPERTIES_H__96496107_7E7E_4EBB_B019_D5DD74DA779C__INCLUDED_)
