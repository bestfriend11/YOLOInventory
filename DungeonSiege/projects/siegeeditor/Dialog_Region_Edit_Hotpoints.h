#if !defined(AFX_DIALOG_REGION_EDIT_HOTPOINTS_H__00B6AB86_FC8D_49C3_9DCE_3AE799793D98__INCLUDED_)
#define AFX_DIALOG_REGION_EDIT_HOTPOINTS_H__00B6AB86_FC8D_49C3_9DCE_3AE799793D98__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Region_Edit_Hotpoints.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Edit_Hotpoints dialog

class Dialog_Region_Edit_Hotpoints : public CDialog
{
// Construction
public:
	Dialog_Region_Edit_Hotpoints(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Region_Edit_Hotpoints)
	enum { IDD = IDD_DIALOG_REGION_EDIT_HOTPOINTS };
	CListBox	m_hotpoints;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Region_Edit_Hotpoints)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Region_Edit_Hotpoints)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonEditDirectional();
	virtual void OnOK();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonRemove();
	afx_msg void OnSelchangeListHotpoints();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGION_EDIT_HOTPOINTS_H__00B6AB86_FC8D_49C3_9DCE_3AE799793D98__INCLUDED_)
