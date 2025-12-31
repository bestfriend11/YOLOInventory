#if !defined(AFX_DIALOG_CONVERSATION_ADD_GOODBYE_H__9A3C4667_D274_49FD_905E_EF920A5AC0A2__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_ADD_GOODBYE_H__9A3C4667_D274_49FD_905E_EF920A5AC0A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Add_Goodbye.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Add_Goodbye dialog

class Dialog_Conversation_Add_Goodbye : public CDialog
{
// Construction
public:
	Dialog_Conversation_Add_Goodbye(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Add_Goodbye)
	enum { IDD = IDD_DIALOG_CONVERSATION_ADD_GOODBYE };
	CEdit	m_Order;
	CComboBox	m_Samples;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Add_Goodbye)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Add_Goodbye)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_ADD_GOODBYE_H__9A3C4667_D274_49FD_905E_EF920A5AC0A2__INCLUDED_)
