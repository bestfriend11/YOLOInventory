#pragma once
#include "afxcmn.h"


// Dialog_Save_As_DSMap dialog

class Dialog_Save_As_DSMap : public CDialog
{
public:
	Dialog_Save_As_DSMap(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Save_As_DSMap)
	enum { IDD = IDD_DIALOG_SAVE_AS_DSMAP };
	CButton	m_OkButton;
	CListCtrl m_MapChoice;
	CString m_DstFileName;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Save_As_DSMap)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	CString GetSelectedMap();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Save_As_DSMap)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBrowseDestFile();
	afx_msg void OnClickMapChoice(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeDestFileChoice();
	afx_msg void OnItemActivateMapChoice(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
