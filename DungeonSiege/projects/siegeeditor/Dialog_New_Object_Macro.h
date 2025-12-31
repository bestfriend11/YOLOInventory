#if !defined(AFX_DIALOG_NEW_OBJECT_MACRO_H__C6F498C9_248F_4FDA_8249_78E3F2F11743__INCLUDED_)
#define AFX_DIALOG_NEW_OBJECT_MACRO_H__C6F498C9_248F_4FDA_8249_78E3F2F11743__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_New_Object_Macro.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Object_Macro dialog

class Dialog_New_Object_Macro : public CDialog
{
// Construction
public:
	Dialog_New_Object_Macro(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_New_Object_Macro)
	enum { IDD = IDD_DIALOG_NEW_MACRO };
	CEdit	m_macroName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_New_Object_Macro)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_New_Object_Macro)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NEW_OBJECT_MACRO_H__C6F498C9_248F_4FDA_8249_78E3F2F11743__INCLUDED_)
