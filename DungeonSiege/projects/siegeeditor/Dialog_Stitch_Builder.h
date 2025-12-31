#if !defined(AFX_DIALOG_STITCH_BUILDER_H__DFA23914_62F9_4FBA_B1FB_07BEED49D95A__INCLUDED_)
#define AFX_DIALOG_STITCH_BUILDER_H__DFA23914_62F9_4FBA_B1FB_07BEED49D95A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Stitch_Builder.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Builder dialog

class Dialog_Stitch_Builder : public CDialog
{
// Construction
public:
	Dialog_Stitch_Builder(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Stitch_Builder)
	enum { IDD = IDD_DIALOG_STITCH_BUILDER };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Stitch_Builder)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Stitch_Builder)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBuild();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STITCH_BUILDER_H__DFA23914_62F9_4FBA_B1FB_07BEED49D95A__INCLUDED_)
