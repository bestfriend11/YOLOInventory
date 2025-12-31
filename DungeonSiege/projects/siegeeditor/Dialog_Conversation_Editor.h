#if !defined(AFX_DIALOG_CONVERSATION_EDITOR_H__BCF93A66_0297_47BE_B55B_9BCCD98C0243__INCLUDED_)
#define AFX_DIALOG_CONVERSATION_EDITOR_H__BCF93A66_0297_47BE_B55B_9BCCD98C0243__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Conversation_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Conversation_Editor dialog

class Dialog_Conversation_Editor : public CDialog, public Singleton <Dialog_Conversation_Editor>
{
// Construction
public:
	Dialog_Conversation_Editor(CWnd* pParent = NULL);   // standard constructor

	FuelHandle GetSelectedTextHandle() { return m_hSelected; }
	FuelHandle GetSelectedGoodbyeHandle() { return m_hGoodbye; }

// Dialog Data
	//{{AFX_DATA(Dialog_Conversation_Editor)
	enum { IDD = IDD_DIALOG_CONVERSATION_EDITOR };
	CListCtrl	m_Lines;
	CListCtrl	m_Goodbyes;
	CStatic	m_Conversation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Conversation_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL	

// Implementation
protected:

	FuelHandleList m_hlLines;
	FuelHandleList m_hlGoodbyes;
	FuelHandle m_hSelected;
	FuelHandle m_hGoodbye;
	void	Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Conversation_Editor)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonNewLine();
	afx_msg void OnButtonEditLine();
	afx_msg void OnButtonRemoveLine();
	afx_msg void OnButtonNewGoodbye();
	afx_msg void OnButtonRemoveGoodbye();
	afx_msg void OnDblclkListLines();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gConversationEditor Singleton <Dialog_Conversation_Editor>::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONVERSATION_EDITOR_H__BCF93A66_0297_47BE_B55B_9BCCD98C0243__INCLUDED_)
