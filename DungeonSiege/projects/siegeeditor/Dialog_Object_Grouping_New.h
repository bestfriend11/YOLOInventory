#if !defined(AFX_DIALOG_OBJECT_GROUPING_NEW_H__BA1C6D4A_5C4F_4C92_AD5A_4D4C2F01CB5D__INCLUDED_)
#define AFX_DIALOG_OBJECT_GROUPING_NEW_H__BA1C6D4A_5C4F_4C92_AD5A_4D4C2F01CB5D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Grouping_New.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Grouping_New dialog

class Dialog_Object_Grouping_New : public CDialog
{
// Construction
public:
	Dialog_Object_Grouping_New(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Grouping_New)
	enum { IDD = IDD_DIALOG_OBJECT_GROUPING_NEW };
	CEdit	m_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Grouping_New)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Grouping_New)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_GROUPING_NEW_H__BA1C6D4A_5C4F_4C92_AD5A_4D4C2F01CB5D__INCLUDED_)
