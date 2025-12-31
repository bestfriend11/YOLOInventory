#if !defined(AFX_DIALOG_HOTPOINT_INFORMATION_H__B59883D1_AABD_4EC1_AED3_7CA820AED242__INCLUDED_)
#define AFX_DIALOG_HOTPOINT_INFORMATION_H__B59883D1_AABD_4EC1_AED3_7CA820AED242__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Hotpoint_Information.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hotpoint_Information dialog

class Dialog_Hotpoint_Information : public CDialog
{
// Construction
public:
	Dialog_Hotpoint_Information(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Hotpoint_Information)
	enum { IDD = IDD_DIALOG_HOTPOINT_INFORMATION };
	CEdit	m_name;
	CEdit	m_id;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Hotpoint_Information)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Hotpoint_Information)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_HOTPOINT_INFORMATION_H__B59883D1_AABD_4EC1_AED3_7CA820AED242__INCLUDED_)
