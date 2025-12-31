#if !defined(AFX_DIALOG_NODE_MATCHES_H__F32B933D_FEA7_4E07_A816_5A84DEBBDFB1__INCLUDED_)
#define AFX_DIALOG_NODE_MATCHES_H__F32B933D_FEA7_4E07_A816_5A84DEBBDFB1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Node_Matches.h : header file
//

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Matches dialog

class Dialog_Node_Matches : public CDialog, public Singleton <Dialog_Node_Matches>
{
// Construction
public:
	Dialog_Node_Matches(CWnd* pParent = NULL);   // standard constructor	

// Dialog Data
	//{{AFX_DATA(Dialog_Node_Matches)
	enum { IDD = IDD_DIALOG_NODE_MATCHES };
	CTreeCtrl	m_Matches;
	CEdit	m_SelectedDoor;
	CEdit	m_SelectedSno;
	CEdit	m_SearchLevel;
	//}}AFX_DATA	

	void Refresh();

	CTreeCtrl & GetMatchTree() { return m_Matches; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Node_Matches)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL	

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Node_Matches)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonMatch();
	afx_msg void OnSelchangedTreeMatches(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonAttach();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gDialogNodeMatches Dialog_Node_Matches::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NODE_MATCHES_H__F32B933D_FEA7_4E07_A816_5A84DEBBDFB1__INCLUDED_)
