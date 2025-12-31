#if !defined(AFX_DIALOG_CREATE_BACKGROUND_H__CCB9EAA9_E3FF_4893_B3DD_393D9523209A__INCLUDED_)
#define AFX_DIALOG_CREATE_BACKGROUND_H__CCB9EAA9_E3FF_4893_B3DD_393D9523209A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Create_Background.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Create_Background dialog

class Dialog_Create_Background : public CDialog
{
// Construction
public:
	Dialog_Create_Background(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Create_Background)
	enum { IDD = IDD_DIALOG_SET_BACKGROUND };
	CEdit	m_editBg;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Create_Background)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Create_Background)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CREATE_BACKGROUND_H__CCB9EAA9_E3FF_4893_B3DD_393D9523209A__INCLUDED_)
