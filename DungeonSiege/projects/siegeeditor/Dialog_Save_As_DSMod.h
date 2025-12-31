#if !defined(AFX_DIALOG_SAVE_AS_DSMOD_H__4E476451_8BC4_4D76_8D71_6C9D02943374__INCLUDED_)
#define AFX_DIALOG_SAVE_AS_DSMOD_H__4E476451_8BC4_4D76_8D71_6C9D02943374__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Save_As_DSMod.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Save_As_DSMod dialog

class Dialog_Save_As_DSMod : public CDialog
{
// Construction
public:
	Dialog_Save_As_DSMod(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Save_As_DSMod)
	enum { IDD = IDD_DIALOG_SAVE_AS_DSMOD };
	CString	m_SourceFolder;
	CString	m_DstFileName;
	CString	m_OutputPrefix;
	CString	m_Title;
	CString	m_Author;
	CString	m_Copyright;
	CString	m_MinVersion;
	CString	m_Description;
	CString	m_Priority;
	BOOL	m_LqdInclude;
	BOOL	m_LqdCompile;
	BOOL	m_LqdDelete;
	BOOL	m_DevOnly;
	BOOL	m_CompressFiles;
	BOOL	m_ReprocessFiles;
	BOOL	m_ProtectedContent;
	BOOL	m_VerifyTank;
	BOOL	m_UseBackup;
	BOOL	m_GenerateDump;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Save_As_DSMod)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Save_As_DSMod)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBrowseSourceFolder();
	afx_msg void OnBrowseDestFile();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:

	void SetDefaultCopyrightText();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SAVE_AS_DSMOD_H__4E476451_8BC4_4D76_8D71_6C9D02943374__INCLUDED_)
