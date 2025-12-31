#if !defined(AFX_DIALOG_PARTY_CREATOR_H__485F9DBD_4744_4CD7_9540_A2834947F5B0__INCLUDED_)
#define AFX_DIALOG_PARTY_CREATOR_H__485F9DBD_4744_4CD7_9540_A2834947F5B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Party_Creator.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Party_Creator dialog

class Dialog_Party_Creator : public CDialog
{
// Construction
public:
	Dialog_Party_Creator(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Party_Creator)
	enum { IDD = IDD_DIALOG_PARTY_CREATOR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Party_Creator)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Party_Creator)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PARTY_CREATOR_H__485F9DBD_4744_4CD7_9540_A2834947F5B0__INCLUDED_)
