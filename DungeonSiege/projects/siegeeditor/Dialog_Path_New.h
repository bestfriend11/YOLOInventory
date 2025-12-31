#if !defined(AFX_DIALOG_PATH_NEW_H__BBD6C060_4F8F_49A8_8BA4_F95164488D27__INCLUDED_)
#define AFX_DIALOG_PATH_NEW_H__BBD6C060_4F8F_49A8_8BA4_F95164488D27__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Path_New.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_New dialog

class Dialog_Path_New : public CDialog
{
// Construction
public:
	Dialog_Path_New(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Path_New)
	enum { IDD = IDD_DIALOG_PATH_NEW };
	CEdit	m_path;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Path_New)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Path_New)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	DWORD m_color;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PATH_NEW_H__BBD6C060_4F8F_49A8_8BA4_F95164488D27__INCLUDED_)
