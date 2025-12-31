#if !defined(AFX_PROPPAGE_PROPERTIES_INVENTORY_H__8818C0A1_D064_4E05_A4A9_D504EEE91AC2__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_INVENTORY_H__8818C0A1_D064_4E05_A4A9_D504EEE91AC2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Inventory.h : header file
//

#include "ResizablePage.h"

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Inventory dialog

class PropPage_Properties_Inventory : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Inventory)

// Construction
public:
	PropPage_Properties_Inventory();
	~PropPage_Properties_Inventory();

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Inventory)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_INVENTORY };
	CListCtrl	m_list_current;
	CComboBox	m_items;
	//}}AFX_DATA

	void Apply();
	void Reset();
	void Cancel();

	bool CanApply() { return m_bApply; }	

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Inventory)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	bool m_bApply;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Inventory)
	afx_msg void OnRemove();
	afx_msg void OnRemoveall();
	afx_msg void OnAdd();
	afx_msg void OnRadioTemplate();
	afx_msg void OnRadioScreenName();
	afx_msg void OnRadioDocumentation();
	virtual BOOL OnInitDialog();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnButtonAdvanced();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_INVENTORY_H__8818C0A1_D064_4E05_A4A9_D504EEE91AC2__INCLUDED_)
