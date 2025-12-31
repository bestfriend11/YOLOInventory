#if !defined(AFX_DIALOG_HIDE_COMPONENTS_H__4B490568_B570_4A21_ABC4_6BAAD147AB55__INCLUDED_)
#define AFX_DIALOG_HIDE_COMPONENTS_H__4B490568_B570_4A21_ABC4_6BAAD147AB55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Hide_Components.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hide_Components dialog

class Dialog_Hide_Components : public CDialog
{
// Construction
public:
	Dialog_Hide_Components(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Hide_Components)
	enum { IDD = IDD_DIALOG_HIDE_COMPONENTS };
	CEdit	m_componentName;
	CListBox	m_components;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Hide_Components)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RefreshHiddenComponents();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Hide_Components)
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonAdd();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_HIDE_COMPONENTS_H__4B490568_B570_4A21_ABC4_6BAAD147AB55__INCLUDED_)
