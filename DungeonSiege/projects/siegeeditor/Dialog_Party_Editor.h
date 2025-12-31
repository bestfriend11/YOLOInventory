#if !defined(AFX_DIALOG_PARTY_EDITOR_H__132DF9F5_619B_4E9A_906D_02C1D6499AA4__INCLUDED_)
#define AFX_DIALOG_PARTY_EDITOR_H__132DF9F5_619B_4E9A_906D_02C1D6499AA4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Party_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Party_Editor dialog

class Dialog_Party_Editor : public CDialog
{
// Construction
public:
	Dialog_Party_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Party_Editor)
	enum { IDD = IDD_DIALOG_PARTY_EDITOR };
	CEdit	m_edit_name;
	CListCtrl	m_actor_list;
	CListCtrl	m_member_list;
	CComboBox	m_party_combo;	
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Party_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Party_Editor)
	afx_msg void OnEditchangePartycombo();
	virtual void OnOK();
	afx_msg void OnAddmember();
	afx_msg void OnRemovemember();
	afx_msg void OnRemoveallmembers();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonApply();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PARTY_EDITOR_H__132DF9F5_619B_4E9A_906D_02C1D6499AA4__INCLUDED_)
