#if !defined(AFX_DIALOG_QUEST_MANAGER_H__96B49104_FE06_4700_8B7D_54BAD3FC7F30__INCLUDED_)
#define AFX_DIALOG_QUEST_MANAGER_H__96B49104_FE06_4700_8B7D_54BAD3FC7F30__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Quest_Manager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Manager dialog

class Dialog_Quest_Manager : public CDialog, public Singleton <Dialog_Quest_Manager>
{
// Construction
public:
	Dialog_Quest_Manager(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Quest_Manager)
	enum { IDD = IDD_DIALOG_QUEST_MANAGER };
	CListBox	m_Quests;
	CListBox	m_Chapters;
	//}}AFX_DATA

	const gpstring & GetSelectedQuest() { return m_sSelectedQuest; }
	const gpstring & GetSelectedChapter() { return m_sSelectedChapter; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Quest_Manager)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL	

// Implementation
protected:

	void Refresh();
	gpstring m_sSelectedQuest;
	gpstring m_sSelectedChapter;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Quest_Manager)
	afx_msg void OnButtonAddQuest();
	afx_msg void OnButtonEditQuest();
	afx_msg void OnButtonRemoveQuest();
	afx_msg void OnButtonAddChapter();
	afx_msg void OnButtonEditChapter();
	afx_msg void OnButtonRemoveChapter();
	afx_msg void OnButtonHelp();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gQuestManager Dialog_Quest_Manager::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_QUEST_MANAGER_H__96B49104_FE06_4700_8B7D_54BAD3FC7F30__INCLUDED_)
