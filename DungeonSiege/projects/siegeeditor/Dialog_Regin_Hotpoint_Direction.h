#if !defined(AFX_DIALOG_REGIN_HOTPOINT_DIRECTION_H__D758A281_91B8_4CB7_AB61_1D6FFDF3EB62__INCLUDED_)
#define AFX_DIALOG_REGIN_HOTPOINT_DIRECTION_H__D758A281_91B8_4CB7_AB61_1D6FFDF3EB62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Regin_Hotpoint_Direction.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Regin_Hotpoint_Direction dialog

class Dialog_Regin_Hotpoint_Direction : public CDialog
{
// Construction
public:
	Dialog_Regin_Hotpoint_Direction(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Regin_Hotpoint_Direction)
	enum { IDD = IDD_DIALOG_REGION_HOTPOINT_DIRECTION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Regin_Hotpoint_Direction)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Regin_Hotpoint_Direction)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGIN_HOTPOINT_DIRECTION_H__D758A281_91B8_4CB7_AB61_1D6FFDF3EB62__INCLUDED_)
