#if !defined(AFX_DIALOG_CONVERSATION_NEW_GLOBAL_H__2883B795_236E_4E71_9ECE_4F0D7CDEAA0B__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_NEW_GLOBAL_H__2883B795_236E_4E71_9ECE_4F0D7CDEAA0B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_New_Global.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_New_Global dialog

class Dialog_Conversation_New_Global : public CDialog
{
// Construction
public:
	Dialog_Conversation_New_Global(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_New_Global)
	enum { IDD = IDD_DIALOG_CONVERSATION_NEW_GLOBAL };
	CEdit	m_Conversation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_New_Global)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_New_Global)
	afx_msg void OnButtonHelp();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_NEW_GLOBAL_H__2883B795_236E_4E71_9ECE_4F0D7CDEAA0B__INCLUDED_)
