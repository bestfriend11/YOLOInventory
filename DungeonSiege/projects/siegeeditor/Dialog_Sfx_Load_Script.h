#if !defined(AFX_DIALOG_SFX_LOAD_SCRIPT_H__33455B3D_5624_4D84_9B38_2F7D474E65BE__INCLUDED_)
#define AFX_DIALOG_SFX_LOAD_SCRIPT_H__33455B3D_5624_4D84_9B38_2F7D474E65BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Sfx_Load_Script.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Load_Script dialog

class Dialog_Sfx_Load_Script : public CDialog
{
// Construction
public:
	Dialog_Sfx_Load_Script(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Sfx_Load_Script)
	enum { IDD = IDD_DIALOG_LOAD_SFX };
	CComboBox	m_comboScripts;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Sfx_Load_Script)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Sfx_Load_Script)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SFX_LOAD_SCRIPT_H__33455B3D_5624_4D84_9B38_2F7D474E65BE__INCLUDED_)
