#if !defined(AFX_DIALOG_RESOURCE_TREE_H__6F56AE19_545E_4DB9_AA4F_5BBBD5CC86E8__INCLUDED_)
#define AFX_DIALOG_RESOURCE_TREE_H__6F56AE19_545E_4DB9_AA4F_5BBBD5CC86E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Resource_Tree.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Resource_Tree dialog

class Dialog_Resource_Tree : public CDialog
{
// Construction
public:
	Dialog_Resource_Tree(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Resource_Tree)
	enum { IDD = IDD_DIALOG_RESOURCE_TREE };
	CTreeCtrl	m_Resources;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Resource_Tree)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	HTREEITEM RecursiveFindResourceFolder( HTREEITEM hChild, HTREEITEM & hParent, const gpstring & sValue );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Resource_Tree)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_RESOURCE_TREE_H__6F56AE19_545E_4DB9_AA4F_5BBBD5CC86E8__INCLUDED_)
