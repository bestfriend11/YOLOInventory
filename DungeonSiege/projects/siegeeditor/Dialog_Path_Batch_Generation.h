#if !defined(AFX_DIALOG_PATH_BATCH_GENERATION_H__0B4E7949_3E69_4807_938D_54DC018BC17F__INCLUDED_)
#define AFX_DIALOG_PATH_BATCH_GENERATION_H__0B4E7949_3E69_4807_938D_54DC018BC17F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Path_Batch_Generation.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Batch_Generation dialog

class Dialog_Path_Batch_Generation : public CDialog
{
// Construction
public:
	Dialog_Path_Batch_Generation(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Path_Batch_Generation)
	enum { IDD = IDD_DIALOG_PATH_BATCH_GENERATION };
	CButton	m_tuningLoad;
	CTreeCtrl	m_tree_maps;
	CButton	m_masterStats;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Path_Batch_Generation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Path_Batch_Generation)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonDeselectAll();
	afx_msg void OnButtonSelectMap();
	afx_msg void OnButtonDeselectMap();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PATH_BATCH_GENERATION_H__0B4E7949_3E69_4807_938D_54DC018BC17F__INCLUDED_)
