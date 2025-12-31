#if !defined(AFX_DIALOG_SAVE_REGION_H__27E283FD_34AA_4EB7_A203_90846A6A80FB__INCLUDED_)
#define AFX_DIALOG_SAVE_REGION_H__27E283FD_34AA_4EB7_A203_90846A6A80FB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Save_Region.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_Region dialog

class Dialog_Save_Region : public CDialog
{
// Construction
public:
	Dialog_Save_Region(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Save_Region)
	enum { IDD = IDD_DIALOG_SAVE_REGION };
	CButton	m_startingpos;
	CButton	m_stitch;
	CButton	m_nodes;
	CButton	m_lights;
	CButton	m_gameobjects;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Save_Region)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Save_Region)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SAVE_REGION_H__27E283FD_34AA_4EB7_A203_90846A6A80FB__INCLUDED_)
