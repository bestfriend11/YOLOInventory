#if !defined(AFX_DIALOG_BOOKMARK_LIST_H__F75C9307_6AF4_4A99_8113_046BCD6904C4__INCLUDED_)
#define AFX_DIALOG_BOOKMARK_LIST_H__F75C9307_6AF4_4A99_8113_046BCD6904C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Bookmark_List.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Bookmark_List dialog

class Dialog_Bookmark_List : public CDialog
{
// Construction
public:
	Dialog_Bookmark_List(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Bookmark_List)
	enum { IDD = IDD_DIALOG_BOOKMARK_LIST };
	CListBox	m_bookmarks;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Bookmark_List)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RefreshBookmarks();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Bookmark_List)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonProperties();
	afx_msg void OnSelchangeListBookmarks();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_BOOKMARK_LIST_H__F75C9307_6AF4_4A99_8113_046BCD6904C4__INCLUDED_)
