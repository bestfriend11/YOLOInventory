#if !defined(AFX_DIALOG_HUDS_H__AEA399C0_88D1_48F7_949C_B69EF606EF7D__INCLUDED_)
#define AFX_DIALOG_HUDS_H__AEA399C0_88D1_48F7_949C_B69EF606EF7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Huds.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Huds dialog

class Dialog_Huds : public CDialog
{
// Construction
public:
	Dialog_Huds(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Huds)
	enum { IDD = IDD_DIALOG_HUDS };
	CButton	m_messages;
	CButton	m_labels;
	CButton	m_ai_sensors;
	CButton	m_ai_pathfinder;
	CButton	m_singleObject;
	int		m_group;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Huds)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Huds)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_HUDS_H__AEA399C0_88D1_48F7_949C_B69EF606EF7D__INCLUDED_)
