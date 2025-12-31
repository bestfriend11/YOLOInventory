#if !defined(AFX_DIALOG_MAP_BOOKMARKS_H__97561AA3_922C_4E26_A72A_64E52F958888__INCLUDED_)
#define AFX_DIALOG_MAP_BOOKMARKS_H__97561AA3_922C_4E26_A72A_64E52F958888__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Map_Bookmarks.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Bookmarks dialog

class Dialog_Map_Bookmarks : public CDialog
{
// Construction
public:
	Dialog_Map_Bookmarks(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Map_Bookmarks)
	enum { IDD = IDD_DIALOG_BOOKMARKS };
	CEdit	m_description;
	CEdit	m_bookmark_name;
	CEdit	m_up;
	CEdit	m_orbit;
	CEdit	m_north;
	CListBox	m_list_bookmarks;
	CEdit	m_node;
	CEdit	m_east;
	CEdit	m_distance;
	CEdit	m_azimuth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Map_Bookmarks)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Map_Bookmarks)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonUseCurrentPosition();
	afx_msg void OnButtonUseSelectedNode();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonNew();
	afx_msg void OnSelchangeListBookmarks();
	afx_msg void OnButtonSetData();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MAP_BOOKMARKS_H__97561AA3_922C_4E26_A72A_64E52F958888__INCLUDED_)
