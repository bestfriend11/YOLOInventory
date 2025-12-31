#if !defined(AFX_DIALOG_CONVERSATION_NEW_H__B1DD37A8_51DE_4170_AB2A_DDDB71B0B42A__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_NEW_H__B1DD37A8_51DE_4170_AB2A_DDDB71B0B42A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_New.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_New dialog

class Dialog_Conversation_New : public CDialog
{
// Construction
public:
	Dialog_Conversation_New(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_New)
	enum { IDD = IDD_DIALOG_CONVERSATION_NEW };
	CComboBox	m_Existing;
	CEdit	m_Conversation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_New)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_New)
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_NEW_H__B1DD37A8_51DE_4170_AB2A_DDDB71B0B42A__INCLUDED_)
