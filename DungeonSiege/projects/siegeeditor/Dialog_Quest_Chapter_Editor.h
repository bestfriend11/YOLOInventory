#if !defined(AFX_DIALOG_QUEST_CHAPTER_EDITOR_H__54918FB3_8F09_4FF9_B038_B2AAA39D82F5__INCLUDED_)
#define AFX_DIALOG_QUEST_CHAPTER_EDITOR_H__54918FB3_8F09_4FF9_B038_B2AAA39D82F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Quest_Chapter_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Editor dialog

class Dialog_Quest_Chapter_Editor : public CDialog, public Singleton <Dialog_Quest_Chapter_Editor>
{
// Construction
public:
	Dialog_Quest_Chapter_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Quest_Chapter_Editor)
	enum { IDD = IDD_DIALOG_QUEST_CHAPTER_EDITOR };
	CListCtrl	m_ChapterUpdates;
	CEdit	m_ScreenName;
	CEdit	m_Image;
	//}}AFX_DATA

	FuelHandle GetSelectedHandle() { return m_hSelectedHandle; }
	bool DoesOrderExist( int order );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Quest_Chapter_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	FuelHandleList	m_hlUpdates;
	FuelHandle		m_hSelectedHandle;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Quest_Chapter_Editor)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonRemove();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gChapterEditor Dialog_Quest_Chapter_Editor::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_QUEST_CHAPTER_EDITOR_H__54918FB3_8F09_4FF9_B038_B2AAA39D82F5__INCLUDED_)
