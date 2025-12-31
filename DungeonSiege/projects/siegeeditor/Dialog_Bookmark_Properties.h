#if !defined(AFX_DIALOG_BOOKMARK_PROPERTIES_H__27A58997_E4E0_471E_9988_8A6F357E3887__INCLUDED_)
#define AFX_DIALOG_BOOKMARK_PROPERTIES_H__27A58997_E4E0_471E_9988_8A6F357E3887__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Bookmark_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Bookmark_Properties dialog

class Dialog_Bookmark_Properties : public CDialog
{
// Construction
public:
	Dialog_Bookmark_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Bookmark_Properties)
	enum { IDD = IDD_DIALOG_BOOKMARK_PROPERTIES };
	CEdit	m_z;
	CEdit	m_y;
	CEdit	m_x;
	CEdit	m_orbit;
	CEdit	m_node;
	CEdit	m_description;
	CEdit	m_bookmark;
	CEdit	m_distance;
	CEdit	m_azimuth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Bookmark_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Bookmark_Properties)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonUseSelectedNode();
	afx_msg void OnButtonUseCurrentPosition();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_BOOKMARK_PROPERTIES_H__27A58997_E4E0_471E_9988_8A6F357E3887__INCLUDED_)
