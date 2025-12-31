#if !defined(AFX_DIALOG_COMMAND_EDITOR_H__38B0F4C9_FCD6_4440_B104_5F49DD663568__INCLUDED_)
#define AFX_DIALOG_COMMAND_EDITOR_H__38B0F4C9_FCD6_4440_B104_5F49DD663568__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Command_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Command_Editor dialog

class Dialog_Command_Editor : public CDialog
{
// Construction
public:
	Dialog_Command_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Command_Editor)
	enum { IDD = IDD_DIALOG_COMMAND_EDITOR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Command_Editor)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Command_Editor)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioSelect();
	afx_msg void OnRadioLink();
	afx_msg void OnRadioMove();
	afx_msg void OnRadioPatrol();
	afx_msg void OnRadioStop();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_COMMAND_EDITOR_H__38B0F4C9_FCD6_4440_B104_5F49DD663568__INCLUDED_)
