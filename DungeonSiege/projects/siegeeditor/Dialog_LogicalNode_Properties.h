#if !defined(AFX_DIALOG_LOGICALNODE_PROPERTIES_H__F7B584B7_BAB7_459B_9090_11956F1654D8__INCLUDED_)
#define AFX_DIALOG_LOGICALNODE_PROPERTIES_H__F7B584B7_BAB7_459B_9090_11956F1654D8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_LogicalNode_Properties.h : header file
//

#include "siege_logical_node.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_LogicalNode_Properties dialog

class Dialog_LogicalNode_Properties : public CDialog
{
// Construction
public:
	Dialog_LogicalNode_Properties(CWnd* pParent = NULL);   // standard constructor

	void AddLogicalFlag( siege::eLogicalNodeFlags testFlag );
	void CheckLogicalFlag( siege::eLogicalNodeFlags currentFlags, siege::eLogicalNodeFlags testFlag );

// Dialog Data
	//{{AFX_DATA(Dialog_LogicalNode_Properties)
	enum { IDD = IDD_DIALOG_LOGICALNODE_PROPERTIES };
	CListBox	m_logicalFlags;
	CComboBox	m_availableFlags;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_LogicalNode_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_LogicalNode_Properties)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAddFlag();
	virtual void OnOK();
	afx_msg void OnButtonRemoveFlag();
	afx_msg void OnButtonClearFlags();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_LOGICALNODE_PROPERTIES_H__F7B584B7_BAB7_459B_9090_11956F1654D8__INCLUDED_)
