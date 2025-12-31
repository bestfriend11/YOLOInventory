#if !defined(AFX_DIALOG_OBJECT_GROUPING_H__A54E9F6B_D2D3_4BC3_876A_CF17125ED309__INCLUDED_)
#define AFX_DIALOG_OBJECT_GROUPING_H__A54E9F6B_D2D3_4BC3_876A_CF17125ED309__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Grouping.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Grouping dialog

class Dialog_Object_Grouping : public CDialog, public Singleton <Dialog_Object_Grouping>
{
// Construction
public:
	Dialog_Object_Grouping(CWnd* pParent = NULL);   // standard constructor

	bool GetRotateMode();

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Grouping)
	enum { IDD = IDD_DIALOG_OBJECT_GROUPING };
	CEdit	m_scaleMin;
	CEdit	m_scaleMax;
	CButton	m_rotateMode;
	CTreeCtrl	m_groups;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Grouping)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void GetSelectedGroups( StringVec & groups );
	void Refresh();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Grouping)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonDeselectAll();
	afx_msg void OnCheckRotateGroupMode();
	afx_msg void OnButtonRandomRotation();
	afx_msg void OnButtonShow();
	afx_msg void OnButtonHide();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonAddSelection();
	afx_msg void OnButtonRemoveSelection();
	afx_msg void OnButtonShowSelection();
	afx_msg void OnButtonHideSelection();
	afx_msg void OnButtonSelectGroup();
	afx_msg void OnButtonDeselectGroup();
	afx_msg void OnButtonScale();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#define gObjectGrouping Dialog_Object_Grouping::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_GROUPING_H__A54E9F6B_D2D3_4BC3_876A_CF17125ED309__INCLUDED_)
