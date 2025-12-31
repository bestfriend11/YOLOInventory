#if !defined(AFX_DIALOG_LOAD_CUSTOM_VIEW_H__5E8D5006_CC3D_4834_94D3_704F8A9B2DE9__INCLUDED_)
#define AFX_DIALOG_LOAD_CUSTOM_VIEW_H__5E8D5006_CC3D_4834_94D3_704F8A9B2DE9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Load_Custom_View.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Load_Custom_View dialog

class Dialog_Load_Custom_View : public CDialog
{
// Construction
public:
	Dialog_Load_Custom_View(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Load_Custom_View)
	enum { IDD = IDD_DIALOG_LOAD_CUSTOM_VIEW };
	CListBox	m_customViews;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Load_Custom_View)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Load_Custom_View)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_LOAD_CUSTOM_VIEW_H__5E8D5006_CC3D_4834_94D3_704F8A9B2DE9__INCLUDED_)
