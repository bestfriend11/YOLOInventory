#if !defined(AFX_PROPPAGE_NODE_SET_H__CE78EF2A_5117_441E_860D_CE5AE4341945__INCLUDED_)
#define AFX_PROPPAGE_NODE_SET_H__CE78EF2A_5117_441E_860D_CE5AE4341945__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Node_Set.h : header file
//

#include "gpcore.h"
#include <string>
#include <map>
#include "gpstring.h"

// Type Definitions
typedef std::map< gpstring, gpstring, istring_less > StringMap;

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Set dialog

class PropPage_Node_Set : public CPropertyPage
{
// Construction
public:
	PropPage_Node_Set(CWnd* pParent = NULL);   // standard constructor

	StringMap		m_node_set_map;
	gpstring		m_sNodeSet;
	gpstring		m_sNodeVer;

	void SetNodeSet();

	void Refresh();

// Dialog Data
	//{{AFX_DATA(PropPage_Node_Set)
	enum { IDD = IDD_PROPPAGE_NODE_SET };
	CListBox	m_textures;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Node_Set)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PropPage_Node_Set)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEditchangeComboNodeset();
	afx_msg void OnEditchangeComboNodeversion();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_NODE_SET_H__CE78EF2A_5117_441E_860D_CE5AE4341945__INCLUDED_)
