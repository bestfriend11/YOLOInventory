#if !defined(AFX_DIALOG_NODE_FADE_GROUPS_H__0E773689_DA12_4548_820F_96AF4861C74F__INCLUDED_)
#define AFX_DIALOG_NODE_FADE_GROUPS_H__0E773689_DA12_4548_820F_96AF4861C74F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Node_Fade_Groups.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Fade_Groups dialog

class Dialog_Node_Fade_Groups : public CDialog, public Singleton <Dialog_Node_Fade_Groups>
{
// Construction
public:
	Dialog_Node_Fade_Groups(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Node_Fade_Groups)
	enum { IDD = IDD_DIALOG_NODE_FADE_GROUPS };
	CListCtrl	m_listFades;
	CEdit	m_section;
	CEdit	m_object;
	CEdit	m_level;
	CButton	m_checkSection;
	CButton	m_checkObject;
	CButton	m_checkLevel;
	CComboBox	m_groups;
	//}}AFX_DATA

	void SetNewGroupName( gpstring sName )	{ m_sNewGroup = sName; }
	gpstring & GetNewGroupName()			{ return m_sNewGroup; }

	void RefreshGroups();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Node_Fade_Groups)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	gpstring m_sNewGroup;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Node_Fade_Groups)
	afx_msg void OnSelchangeComboGroups();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemoveSelection();
	afx_msg void OnButtonNewGroup();
	afx_msg void OnButtonDeleteGroup();
	afx_msg void OnButtonHide();
	afx_msg void OnButtonShow();
	afx_msg void OnButtonSelect();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gDialogNodeFadeGroups Dialog_Node_Fade_Groups::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NODE_FADE_GROUPS_H__0E773689_DA12_4548_820F_96AF4861C74F__INCLUDED_)
