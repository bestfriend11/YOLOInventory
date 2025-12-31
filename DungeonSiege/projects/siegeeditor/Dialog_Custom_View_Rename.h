#if !defined(AFX_DIALOG_CUSTOM_VIEW_RENAME_H__E6A50BDD_5B92_4B77_BC89_2E7701B98E1D__INCLUDED_)
#define AFX_DIALOG_CUSTOM_VIEW_RENAME_H__E6A50BDD_5B92_4B77_BC89_2E7701B98E1D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Custom_View_Rename.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Custom_View_Rename dialog

class Dialog_Custom_View_Rename : public CDialog
{
// Construction
public:
	Dialog_Custom_View_Rename(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Custom_View_Rename)
	enum { IDD = IDD_DIALOG_CUSTOM_VIEW_RENAME };
	CEdit	m_Name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Custom_View_Rename)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Custom_View_Rename)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CUSTOM_VIEW_RENAME_H__E6A50BDD_5B92_4B77_BC89_2E7701B98E1D__INCLUDED_)
