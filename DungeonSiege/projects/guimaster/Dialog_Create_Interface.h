#if !defined(AFX_DIALOG_CREATE_INTERFACE_H__3BDD2B00_1795_4934_87AE_05E30C95019F__INCLUDED_)
#define AFX_DIALOG_CREATE_INTERFACE_H__3BDD2B00_1795_4934_87AE_05E30C95019F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Create_Interface.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Create_Interface dialog

class CDialog_Create_Interface : public CDialog
{
// Construction
public:
	CDialog_Create_Interface(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Create_Interface)
	enum { IDD = IDD_DIALOG_CREATE_INTERFACE };
	CEdit	m_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Create_Interface)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Create_Interface)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CREATE_INTERFACE_H__3BDD2B00_1795_4934_87AE_05E30C95019F__INCLUDED_)
