#if !defined(AFX_DIALOG_OBJECT_INVENTORY_H__0E50686F_BC6F_4485_B213_9B9A0A9CD4B8__INCLUDED_)
#define AFX_DIALOG_OBJECT_INVENTORY_H__0E50686F_BC6F_4485_B213_9B9A0A9CD4B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Inventory.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Inventory dialog

class Dialog_Object_Inventory : public CDialog
{
// Construction
public:
	Dialog_Object_Inventory(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Inventory)
	enum { IDD = IDD_DIALOG_OBJECT_INVENTORY };
	CListCtrl	m_list_current;
	CListCtrl	m_list_available;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Inventory)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Inventory)
	afx_msg void OnRemove();
	afx_msg void OnRemoveall();
	afx_msg void OnAdd();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_INVENTORY_H__0E50686F_BC6F_4485_B213_9B9A0A9CD4B8__INCLUDED_)
