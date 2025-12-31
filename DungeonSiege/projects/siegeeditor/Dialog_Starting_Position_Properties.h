#if !defined(AFX_DIALOG_STARTING_POSITION_PROPERTIES_H__010F9685_D026_45C3_A268_E0EC0AAF5FCC__INCLUDED_)
#define AFX_DIALOG_STARTING_POSITION_PROPERTIES_H__010F9685_D026_45C3_A268_E0EC0AAF5FCC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Starting_Position_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Position_Properties dialog

class Dialog_Starting_Position_Properties : public CDialog
{
// Construction
public:
	Dialog_Starting_Position_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Starting_Position_Properties)
	enum { IDD = IDD_DIALOG_STARTING_POSITION_PROPERTIES };
	CComboBox	m_startGroups;
	CEdit	m_id;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Starting_Position_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Starting_Position_Properties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTING_POSITION_PROPERTIES_H__010F9685_D026_45C3_A268_E0EC0AAF5FCC__INCLUDED_)
