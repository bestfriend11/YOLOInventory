#if !defined(AFX_DIALOG_LOAD_INTERFACE_H__B3905FCE_2F3C_47BA_BE7A_5FDAD45BA88B__INCLUDED_)
#define AFX_DIALOG_LOAD_INTERFACE_H__B3905FCE_2F3C_47BA_BE7A_5FDAD45BA88B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Load_Interface.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Load_Interface dialog

class CDialog_Load_Interface : public CDialog
{
// Construction
public:
	CDialog_Load_Interface(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Load_Interface)
	enum { IDD = IDD_DIALOG_LOAD_INTERFACE };
	CEdit	m_add;
	CListBox	m_interfaces;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Load_Interface)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Load_Interface)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonProcess();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_LOAD_INTERFACE_H__B3905FCE_2F3C_47BA_BE7A_5FDAD45BA88B__INCLUDED_)
