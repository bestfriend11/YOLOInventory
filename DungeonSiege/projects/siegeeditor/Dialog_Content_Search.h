#if !defined(AFX_DIALOG_CONTENT_SEARCH_H__549D2F4C_0F95_4EFB_9F4F_2654E556BE22__INCLUDED_)
#define AFX_DIALOG_CONTENT_SEARCH_H__549D2F4C_0F95_4EFB_9F4F_2654E556BE22__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Content_Search.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Content_Search dialog

class Dialog_Content_Search : public CDialog
{
// Construction
public:
	Dialog_Content_Search(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Content_Search)
	enum { IDD = IDD_DIALOG_CONTENT_SEARCH };
	CListCtrl	m_content;
	CComboBox	m_type;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Content_Search)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Content_Search)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnClickContentlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonAddCustom();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CONTENT_SEARCH_H__549D2F4C_0F95_4EFB_9F4F_2654E556BE22__INCLUDED_)
