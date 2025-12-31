#if !defined(AFX_DIALOG_PATH_REPORTER_H__B7236CF7_54D2_4BD5_814F_822C92B2993D__INCLUDED_)
#define AFX_DIALOG_PATH_REPORTER_H__B7236CF7_54D2_4BD5_814F_822C92B2993D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Path_Reporter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Reporter dialog

class Dialog_Path_Reporter : public CDialog
{
// Construction
public:
	Dialog_Path_Reporter(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Path_Reporter)
	enum { IDD = IDD_DIALOG_PATH_REPORTER };
	CButton	m_region;
	CButton	m_placeditem_point;
	CButton	m_placeditem;
	CButton	m_path_point;
	CButton	m_partymember_point;
	CButton	m_partymember;
	CButton	m_enemyitem_point;
	CButton	m_enemyitem;
	CButton	m_enemy_point;
	CButton	m_enemy;
	CButton	m_containeritem_point;
	CButton	m_containeritem;
	CButton	m_allitem_point;
	CButton	m_allitem;
	CButton	m_pathSummary;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Path_Reporter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Path_Reporter)
	afx_msg void OnButtonGenerateReports();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PATH_REPORTER_H__B7236CF7_54D2_4BD5_814F_822C92B2993D__INCLUDED_)
