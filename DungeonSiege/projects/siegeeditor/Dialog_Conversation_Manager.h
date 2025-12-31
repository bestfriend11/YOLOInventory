#if !defined(AFX_DIALOG_CONVERSATION_MANAGER_H__3A267857_A447_4128_98DC_12C213CB3034__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_MANAGER_H__3A267857_A447_4128_98DC_12C213CB3034__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Manager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Manager dialog

class Dialog_Conversation_Manager : public CDialog, public Singleton <Dialog_Conversation_Manager>
{
// Construction
public:
	Dialog_Conversation_Manager(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Manager)
	enum { IDD = IDD_DIALOG_CONVERSATION_MANAGER };
	CStatic	m_TemplateName;
	CStatic	m_ScreenName;
	CStatic	m_Scid;
	CListBox	m_ExistingConversations;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Manager)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL	

// Implementation
protected:

	void Refresh();

	gpstring m_sSelectedConversation;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Manager)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gConversationManager Singleton <Dialog_Conversation_Manager>::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_MANAGER_H__3A267857_A447_4128_98DC_12C213CB3034__INCLUDED_)
