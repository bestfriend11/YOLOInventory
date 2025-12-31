#if !defined(AFX_DIALOG_NEW_MAP_H__C5A5640D_D870_497E_94B8_32D081F0FFE1__INCLUDED_)
#define AFX_DIALOG_NEW_MAP_H__C5A5640D_D870_497E_94B8_32D081F0FFE1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_New_Map.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Map dialog

class Dialog_New_Map : public CDialog
{
// Construction
public:
	Dialog_New_Map(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_New_Map)
	enum { IDD = IDD_DIALOG_NEW_MAP };
	CEdit	m_worldInterestRadius;
	CEdit	m_worldFrustumRadius;
	CEdit	m_description;
	CComboBox	m_minutes;
	CComboBox	m_hours;
	CButton	m_devOnly;
	CEdit	m_screen_name;
	CEdit	m_map_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_New_Map)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_New_Map)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDefaultWorldFrustumRadius();
	afx_msg void OnButtonDefaultWorldInterestRadius();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NEW_MAP_H__C5A5640D_D870_497E_94B8_32D081F0FFE1__INCLUDED_)
