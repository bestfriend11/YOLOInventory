#if !defined(AFX_DIALOG_CONVERSATION_COMPLETE_QUESTS_H__7AD35783_57C7_4A44_A06B_7E90F374249F__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_COMPLETE_QUESTS_H__7AD35783_57C7_4A44_A06B_7E90F374249F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Complete_Quests.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Complete_Quests dialog

class Dialog_Conversation_Complete_Quests : public CDialog
{
// Construction
public:
	Dialog_Conversation_Complete_Quests(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Complete_Quests)
	enum { IDD = IDD_DIALOG_CONVERSATION_COMPLETE_QUESTS };
	CListBox	m_ListQuests;
	CComboBox	m_AvailableQuests;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Complete_Quests)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Complete_Quests)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_COMPLETE_QUESTS_H__7AD35783_57C7_4A44_A06B_7E90F374249F__INCLUDED_)
