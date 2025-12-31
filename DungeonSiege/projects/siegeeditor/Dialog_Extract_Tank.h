#if !defined(AFX_DIALOG_EXTRACT_TANK_H__F61A0860_0EBC_40B3_98B5_C6D2DA292DCA__INCLUDED_)
#define AFX_DIALOG_EXTRACT_TANK_H__F61A0860_0EBC_40B3_98B5_C6D2DA292DCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Extract_Tank.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Extract_Tank dialog

class Dialog_Extract_Tank : public CDialog
{
// Construction
public:
	Dialog_Extract_Tank(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Extract_Tank)
	enum { IDD = IDD_DIALOG_EXTRACT_TANK };
	CString	m_SourceFile;
	CString	m_DstFolder;
	BOOL    m_DefaultDstFolder;
	BOOL	m_OverwriteFiles;
	BOOL	m_ExtractLqdFiles;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Extract_Tank)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Extract_Tank)
	afx_msg void OnBrowseSourceFile();
	afx_msg void OnBrowseDestFolder();
	virtual void OnOK();
	afx_msg void OnDefaultDstFolder();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_EXTRACT_TANK_H__F61A0860_0EBC_40B3_98B5_C6D2DA292DCA__INCLUDED_)
