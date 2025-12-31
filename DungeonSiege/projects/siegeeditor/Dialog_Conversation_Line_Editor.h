#if !defined(AFX_DIALOG_CONVERSATION_LINE_EDITOR_H__5BBA73FE_C58E_4EB6_9D41_A8D704FB4EBE__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_LINE_EDITOR_H__5BBA73FE_C58E_4EB6_9D41_A8D704FB4EBE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Line_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Line_Editor dialog

class Dialog_Conversation_Line_Editor : public CDialog
{
// Construction
public:
	Dialog_Conversation_Line_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Line_Editor)
	enum { IDD = IDD_DIALOG_CONVERSATION_LINE_EDITOR };
	CEdit	m_ScrollRate;
	CEdit	m_Order;
	CEdit	m_Line;
	CComboBox	m_ButtonChoice;
	CComboBox	m_AudioSample;
	CButton	m_QuestDialogue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Line_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Line_Editor)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonActivateQuests();
	afx_msg void OnButtonCompleteQuests();
	afx_msg void OnButtonCustomButtons();
	afx_msg void OnChangeEditTextScrollRate();
	afx_msg void OnChangeEditOrder();
	afx_msg void OnCheckQuestDialogue();
	afx_msg void OnSelchangeComboAudioSample();
	afx_msg void OnSelchangeComboButtonChoice();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_LINE_EDITOR_H__5BBA73FE_C58E_4EB6_9D41_A8D704FB4EBE__INCLUDED_)
