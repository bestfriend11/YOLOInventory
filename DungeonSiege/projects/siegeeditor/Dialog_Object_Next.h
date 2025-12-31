#if !defined(AFX_DIALOG_OBJECT_NEXT_H__BD90743E_026B_4772_82E8_F2C6FF9B3D3D__INCLUDED_)
#define AFX_DIALOG_OBJECT_NEXT_H__BD90743E_026B_4772_82E8_F2C6FF9B3D3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Next.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Next dialog

class Dialog_Object_Next : public CDialog
{
// Construction
public:
	Dialog_Object_Next(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Next)
	enum { IDD = IDD_DIALOG_OBJECT_NEXT };
	CEdit	m_edit_max_scale;
	CEdit	m_edit_min_scale;
	CButton	m_yaw;
	CButton	m_roll;
	CButton	m_randomScale;
	CButton	m_pitch;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Next)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Next)
	virtual void OnOK();
	afx_msg void OnButtonDefaults();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_NEXT_H__BD90743E_026B_4772_82E8_F2C6FF9B3D3D__INCLUDED_)
