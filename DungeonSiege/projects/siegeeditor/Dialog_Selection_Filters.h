#if !defined(AFX_DIALOG_SELECTION_FILTERS_H__66E7B4C1_898A_400A_8836_1612E3BA951F__INCLUDED_)
#define AFX_DIALOG_SELECTION_FILTERS_H__66E7B4C1_898A_400A_8836_1612E3BA951F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Selection_Filters.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Selection_Filters dialog

class Dialog_Selection_Filters : public CDialog
{
// Construction
public:
	Dialog_Selection_Filters(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Selection_Filters)
	enum { IDD = IDD_DIALOG_SELECTION_FILTERS };
	CButton	m_selectAll;
	CButton	m_checkNonInteractive;
	CButton	m_checkNonGizmos;
	CButton	m_checkInteractive;
	CButton	m_checkGizmos;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Selection_Filters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Selection_Filters)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SELECTION_FILTERS_H__66E7B4C1_898A_400A_8836_1612E3BA951F__INCLUDED_)
