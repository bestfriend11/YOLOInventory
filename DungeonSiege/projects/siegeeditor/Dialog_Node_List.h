#if !defined(AFX_DIALOG_NODE_LIST_H__CE9FA8B5_2A6C_4B6F_8BF6_5B7B384F1F0A__INCLUDED_)
#define AFX_DIALOG_NODE_LIST_H__CE9FA8B5_2A6C_4B6F_8BF6_5B7B384F1F0A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Node_List.h : header file
//

#include "SiegeEditorTypes.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_List dialog

class Dialog_Node_List : public CDialog
{
// Construction
public:
	Dialog_Node_List(CWnd* pParent = NULL);   // standard constructor

	StringVec m_node_list;

// Dialog Data
	//{{AFX_DATA(Dialog_Node_List)
	enum { IDD = IDD_DIALOG_NODE_LIST };
	CListBox	m_node_listbox;
	CEdit	m_guid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Node_List)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Node_List)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSelectNode();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	afx_msg void OnChangeEditGuid();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NODE_LIST_H__CE9FA8B5_2A6C_4B6F_8BF6_5B7B384F1F0A__INCLUDED_)
