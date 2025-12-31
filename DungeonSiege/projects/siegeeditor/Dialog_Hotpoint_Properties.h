#if !defined(AFX_DIALOG_HOTPOINT_PROPERTIES_H__2CF3E5EA_64EA_444B_9F27_EC68EEAAE4E8__INCLUDED_)
#define AFX_DIALOG_HOTPOINT_PROPERTIES_H__2CF3E5EA_64EA_444B_9F27_EC68EEAAE4E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Hotpoint_Properties.h : header file
//
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hotpoint_Properties dialog

class Dialog_Hotpoint_Properties : public CDialog
{
// Construction
public:
	Dialog_Hotpoint_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Hotpoint_Properties)
	enum { IDD = IDD_DIALOG_HOTPOINT_PROPERTIES };
	CComboBox	m_existing_hotpoint;
	CEdit	m_name;
	CEdit	m_id;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Hotpoint_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Hotpoint_Properties)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboExistingHotpoint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_HOTPOINT_PROPERTIES_H__2CF3E5EA_64EA_444B_9F27_EC68EEAAE4E8__INCLUDED_)
