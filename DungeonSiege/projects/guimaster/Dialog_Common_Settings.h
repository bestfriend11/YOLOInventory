#if !defined(AFX_DIALOG_COMMON_SETTINGS_H__8BD9C2FC_AD41_4A48_A308_6B94303E2698__INCLUDED_)
#define AFX_DIALOG_COMMON_SETTINGS_H__8BD9C2FC_AD41_4A48_A308_6B94303E2698__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Common_Settings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Common_Settings dialog

class CDialog_Common_Settings : public CDialog
{
// Construction
public:
	CDialog_Common_Settings(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Common_Settings)
	enum { IDD = IDD_DIALOG_COMMON_SETTINGS };
	CEdit	m_edit_template;
	CComboBox	m_combo_template;
	CButton	m_check_common;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Common_Settings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Common_Settings)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_COMMON_SETTINGS_H__8BD9C2FC_AD41_4A48_A308_6B94303E2698__INCLUDED_)
