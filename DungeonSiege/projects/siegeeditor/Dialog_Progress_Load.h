#if !defined(AFX_DIALOG_PROGRESS_LOAD_H__16A357DF_AD82_4C2B_90C8_EE3E8520BBDC__INCLUDED_)
#define AFX_DIALOG_PROGRESS_LOAD_H__16A357DF_AD82_4C2B_90C8_EE3E8520BBDC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Progress_Load.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Progress_Load dialog

class Dialog_Progress_Load : public CDialog
{
// Construction
public:
	Dialog_Progress_Load(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Progress_Load)
	enum { IDD = IDD_DIALOG_PROGRESS_LOAD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Progress_Load)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Progress_Load)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PROGRESS_LOAD_H__16A357DF_AD82_4C2B_90C8_EE3E8520BBDC__INCLUDED_)
