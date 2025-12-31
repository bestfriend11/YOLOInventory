#if !defined(AFX_DIALOG_SAVE_CUSTOM_VIEW_H__AEECC71E_7471_4E56_97DC_6861072B431F__INCLUDED_)
#define AFX_DIALOG_SAVE_CUSTOM_VIEW_H__AEECC71E_7471_4E56_97DC_6861072B431F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Save_Custom_View.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Custom_View dialog

class Dialog_Save_Custom_View : public CDialog
{
// Construction
public:
	Dialog_Save_Custom_View(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Save_Custom_View)
	enum { IDD = IDD_DIALOG_SAVE_CUSTOM_VIEW };
	CEdit	m_customView;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Save_Custom_View)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Save_Custom_View)
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SAVE_CUSTOM_VIEW_H__AEECC71E_7471_4E56_97DC_6861072B431F__INCLUDED_)
