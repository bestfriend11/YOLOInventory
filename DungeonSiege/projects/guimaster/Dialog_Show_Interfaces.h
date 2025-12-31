#if !defined(AFX_DIALOG_SHOW_INTERFACES_H__39EF020D_6947_4DAD_8800_0BF0214EF541__INCLUDED_)
#define AFX_DIALOG_SHOW_INTERFACES_H__39EF020D_6947_4DAD_8800_0BF0214EF541__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Show_Interfaces.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Show_Interfaces dialog

class CDialog_Show_Interfaces : public CDialog
{
// Construction
public:
	CDialog_Show_Interfaces(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Show_Interfaces)
	enum { IDD = IDD_DIALOG_SHOWINTERFACES };
	CStatic	m_current_interface;
	CListBox	m_active_interfaces;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Show_Interfaces)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Show_Interfaces)
	afx_msg void OnButtonShow();
	afx_msg void OnButtonHide();
	afx_msg void OnButtonEditInterface();
	afx_msg void OnButtonClose();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SHOW_INTERFACES_H__39EF020D_6947_4DAD_8800_0BF0214EF541__INCLUDED_)
