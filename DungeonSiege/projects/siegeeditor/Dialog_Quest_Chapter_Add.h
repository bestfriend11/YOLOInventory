#if !defined(AFX_DIALOG_QUEST_CHAPTER_ADD_H__8387A774_0B87_4C0A_B23B_2CA192EDCFF5__INCLUDED_)
#define AFX_DIALOG_QUEST_CHAPTER_ADD_H__8387A774_0B87_4C0A_B23B_2CA192EDCFF5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Quest_Chapter_Add.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Add dialog

class Dialog_Quest_Chapter_Add : public CDialog
{
// Construction
public:
	Dialog_Quest_Chapter_Add(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Quest_Chapter_Add)
	enum { IDD = IDD_DIALOG_QUEST_CHAPTER_ADD };
	CEdit	m_Chapter;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Quest_Chapter_Add)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Quest_Chapter_Add)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_QUEST_CHAPTER_ADD_H__8387A774_0B87_4C0A_B23B_2CA192EDCFF5__INCLUDED_)
