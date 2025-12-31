#if !defined(AFX_DIALOG_QUEST_EDITOR_H__DEAC520A_24E6_48DA_9FF9_F402AC20AC64__INCLUDED_)
#define AFX_DIALOG_QUEST_EDITOR_H__DEAC520A_24E6_48DA_9FF9_F402AC20AC64__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Quest_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Editor dialog

class Dialog_Quest_Editor : public CDialog, public Singleton <Dialog_Quest_Editor>
{
// Construction
public:
	Dialog_Quest_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Quest_Editor)
	enum { IDD = IDD_DIALOG_QUEST_EDITOR };
	CListCtrl	m_QuestUpdates;
	CEdit	m_ScreenName;
	CEdit	m_QuestImage;
	CComboBox	m_VictorySample;
	CComboBox	m_Chapter;
	CComboBox	m_AwardSkrit;
	//}}AFX_DATA
	
	FuelHandle GetSelectedHandle() { return m_hSelected; }
	bool DoesOrderExist( int order );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Quest_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL	

// Implementation
protected:

	void Refresh();

	FuelHandleList	m_hlUpdates;
	FuelHandle		m_hSelected;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Quest_Editor)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	virtual void OnOK();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonRemove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gQuestEditor Dialog_Quest_Editor::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_QUEST_EDITOR_H__DEAC520A_24E6_48DA_9FF9_F402AC20AC64__INCLUDED_)
