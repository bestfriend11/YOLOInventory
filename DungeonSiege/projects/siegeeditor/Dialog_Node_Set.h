#if !defined(AFX_DIALOG_NODE_SET_H__A21E2A04_8C15_4879_A99A_0DAE65F3B840__INCLUDED_)
#define AFX_DIALOG_NODE_SET_H__A21E2A04_8C15_4879_A99A_0DAE65F3B840__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Node_Set.h : header file
//


#include "gpcore.h"
#include <string>
#include <map>
#include "gpstring.h"

// Type Definitions
typedef std::map< gpstring, gpstring, istring_less > StringMap;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Set dialog

class Dialog_Node_Set : public CDialog
{
// Construction
public:
	Dialog_Node_Set(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Node_Set)
	enum { IDD = IDD_DIALOG_NODE_SET };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	StringMap		m_node_set_map;
	gpstring		m_sNodeSet;
	gpstring		m_sNodeVer;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Node_Set)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Node_Set)
	afx_msg void OnEditchangeComboSet();
	afx_msg void OnEditchangeComboVersion();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NODE_SET_H__A21E2A04_8C15_4879_A99A_0DAE65F3B840__INCLUDED_)
