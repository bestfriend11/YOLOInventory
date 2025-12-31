#if !defined(AFX_DIALOG_REGION_SETTINGS_H__C587E221_FD03_4DF9_A616_38D811738930__INCLUDED_)
#define AFX_DIALOG_REGION_SETTINGS_H__C587E221_FD03_4DF9_A616_38D811738930__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Region_Settings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Settings dialog

class Dialog_Region_Settings : public CDialog
{
// Construction
public:
	Dialog_Region_Settings(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Region_Settings)
	enum { IDD = IDD_DIALOG_REGION_SETTINGS };
	CComboBox	m_environment_map;
	CEdit	m_north_z;
	CEdit	m_north_x;
	CEdit	m_region_guid;
	CEdit	m_name;
	CEdit	m_description;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Region_Settings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Region_Settings)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGION_SETTINGS_H__C587E221_FD03_4DF9_A616_38D811738930__INCLUDED_)
