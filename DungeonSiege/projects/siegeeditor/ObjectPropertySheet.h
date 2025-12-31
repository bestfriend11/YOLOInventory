#if !defined(AFX_OBJECTPROPERTYSHEET_H__0A102EC9_45D0_463B_9063_8455AAFE90D6__INCLUDED_)
#define AFX_OBJECTPROPERTYSHEET_H__0A102EC9_45D0_463B_9063_8455AAFE90D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPropertySheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectPropertySheet

class CObjectPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CObjectPropertySheet)

// Construction
public:
	CObjectPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CObjectPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

private:

	CButton m_wndOk;
	CButton m_wndApply;
	CButton m_wndHelp;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectPropertySheet)
	public:
	virtual BOOL OnInitDialog();
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectPropertySheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CObjectPropertySheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnObjectOkay();
	afx_msg void OnObjectApply();
	afx_msg void OnObjectHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPROPERTYSHEET_H__0A102EC9_45D0_463B_9063_8455AAFE90D6__INCLUDED_)
