#if !defined(AFX_DIALOG_STARTUP_H__9ED75A05_0574_464D_A085_AB277FFCE4D9__INCLUDED_)
#define AFX_DIALOG_STARTUP_H__9ED75A05_0574_464D_A085_AB277FFCE4D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Startup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Startup dialog

class CDialog_Startup : public CDialog
{
// Construction
public:
	CDialog_Startup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Startup)
	enum { IDD = IDD_DIALOG_START };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Startup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Startup)
	afx_msg void OnButtonCreate();
	afx_msg void OnButtonLoad();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTUP_H__9ED75A05_0574_464D_A085_AB277FFCE4D9__INCLUDED_)
