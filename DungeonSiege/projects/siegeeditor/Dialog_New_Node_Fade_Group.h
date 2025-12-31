#if !defined(AFX_DIALOG_NEW_NODE_FADE_GROUP_H__6EF61764_12B8_424F_AD15_0C9683806655__INCLUDED_)
#define AFX_DIALOG_NEW_NODE_FADE_GROUP_H__6EF61764_12B8_424F_AD15_0C9683806655__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_New_Node_Fade_Group.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Node_Fade_Group dialog

class Dialog_New_Node_Fade_Group : public CDialog
{
// Construction
public:
	Dialog_New_Node_Fade_Group(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_New_Node_Fade_Group)
	enum { IDD = IDD_DIALOG_NEW_NODE_FADE_GROUP };
	CEdit	m_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_New_Node_Fade_Group)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_New_Node_Fade_Group)
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NEW_NODE_FADE_GROUP_H__6EF61764_12B8_424F_AD15_0C9683806655__INCLUDED_)
