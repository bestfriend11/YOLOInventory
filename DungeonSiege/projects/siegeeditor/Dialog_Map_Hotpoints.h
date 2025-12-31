#if !defined(AFX_DIALOG_MAP_HOTPOINTS_H__FE023BD7_DD55_4826_890F_642DBC39AF06__INCLUDED_)
#define AFX_DIALOG_MAP_HOTPOINTS_H__FE023BD7_DD55_4826_890F_642DBC39AF06__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Map_Hotpoints.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Hotpoints dialog

class Dialog_Map_Hotpoints : public CDialog
{
// Construction
public:
	Dialog_Map_Hotpoints(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Map_Hotpoints)
	enum { IDD = IDD_DIALOG_MAP_HOTPOINTS };
	CListBox	m_hotpoints;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Map_Hotpoints)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	int m_highestHotpointID;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Map_Hotpoints)
	afx_msg void OnButtonRemove();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MAP_HOTPOINTS_H__FE023BD7_DD55_4826_890F_642DBC39AF06__INCLUDED_)
