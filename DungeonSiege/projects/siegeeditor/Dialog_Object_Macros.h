#if !defined(AFX_DIALOG_OBJECT_MACROS_H__55B81D71_0634_4B81_9100_B0508BC9AD43__INCLUDED_)
#define AFX_DIALOG_OBJECT_MACROS_H__55B81D71_0634_4B81_9100_B0508BC9AD43__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Macros.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Macros dialog

class Dialog_Object_Macros : public CDialog
{
// Construction
public:
	Dialog_Object_Macros(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Macros)
	enum { IDD = IDD_DIALOG_MACROS };
	CListBox	m_macros;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Macros)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Macros)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDefineMacro();
	afx_msg void OnButtonDeleteMacro();
	afx_msg void OnButtonPlaceMacro();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_MACROS_H__55B81D71_0634_4B81_9100_B0508BC9AD43__INCLUDED_)
