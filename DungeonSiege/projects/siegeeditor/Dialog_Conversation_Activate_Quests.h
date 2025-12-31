#if !defined(AFX_DIALOG_CONVERSATION_ACTIVATE_QUESTS_H__5CD0C7DE_D6A7_42BD_A4DA_18E16E07CC1A__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_ACTIVATE_QUESTS_H__5CD0C7DE_D6A7_42BD_A4DA_18E16E07CC1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Activate_Quests.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Activate_Quests dialog

class Dialog_Conversation_Activate_Quests : public CDialog
{
// Construction
public:
	Dialog_Conversation_Activate_Quests(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Activate_Quests)
	enum { IDD = IDD_DIALOG_CONVERSATION_ACTIVATE_QUESTS };
	CComboBox	m_Order;
	CListCtrl	m_ListQuests;
	CComboBox	m_AvailableQuests;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Activate_Quests)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Activate_Quests)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonAdd();
	afx_msg void OnSelchangeComboQuests();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_ACTIVATE_QUESTS_H__5CD0C7DE_D6A7_42BD_A4DA_18E16E07CC1A__INCLUDED_)
