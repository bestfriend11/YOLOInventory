#if !defined(AFX_DIALOG_SFX_NEW_SCRIPT_H__28099D56_EB45_4C79_A4D7_1812B6D164A4__INCLUDED_)
#define AFX_DIALOG_SFX_NEW_SCRIPT_H__28099D56_EB45_4C79_A4D7_1812B6D164A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Sfx_New_Script.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_New_Script dialog

class Dialog_Sfx_New_Script : public CDialog
{
// Construction
public:
	Dialog_Sfx_New_Script(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Sfx_New_Script)
	enum { IDD = IDD_DIALOG_NEW_SFX };
	CEdit	m_editName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Sfx_New_Script)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Sfx_New_Script)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SFX_NEW_SCRIPT_H__28099D56_EB45_4C79_A4D7_1812B6D164A4__INCLUDED_)
