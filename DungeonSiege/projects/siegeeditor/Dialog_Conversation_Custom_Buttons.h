#if !defined(AFX_DIALOG_CONVERSATION_CUSTOM_BUTTONS_H__B0133DAC_9EB5_4164_B216_5EDADA1B4B60__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_CUSTOM_BUTTONS_H__B0133DAC_9EB5_4164_B216_5EDADA1B4B60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Custom_Buttons.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Custom_Buttons dialog

class Dialog_Conversation_Custom_Buttons : public CDialog
{
// Construction
public:
	Dialog_Conversation_Custom_Buttons(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Custom_Buttons)
	enum { IDD = IDD_DIALOG_CUSTOM_BUTTONS };
	CEdit	m_Value2;
	CEdit	m_Value1;
	CEdit	m_Text2;
	CEdit	m_Text1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Custom_Buttons)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Custom_Buttons)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_CUSTOM_BUTTONS_H__B0133DAC_9EB5_4164_B216_5EDADA1B4B60__INCLUDED_)
