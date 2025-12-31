#if !defined(AFX_OBJECT_PROPERTY_SHEET_H__23293978_0182_4C3F_8F3B_CB745EC0B412__INCLUDED_)
#define AFX_OBJECT_PROPERTY_SHEET_H__23293978_0182_4C3F_8F3B_CB745EC0B412__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Object_Property_Sheet.h : header file
//

#define TEMPLATE_PAGE	0
#define TRIGGERS_PAGE	1
#define ROTATION_PAGE	2
#define EQUIPMENT_PAGE	3
#define INVENTORY_PAGE	4
#define LIGHTING_PAGE	5
#define GIZMO_PAGE		6

#include "ResizableSheet.h"
#include "ResizeCtrl.h"
#include "gpcore.h"

/////////////////////////////////////////////////////////////////////////////
// Object_Property_Sheet

class Object_Property_Sheet : public CResizableSheet, public Singleton <Object_Property_Sheet>
{
	DECLARE_DYNAMIC(Object_Property_Sheet)

// Construction
public:
	Object_Property_Sheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	Object_Property_Sheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

	void Reset();	
	bool Evaluate();
	void Apply();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Object_Property_Sheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~Object_Property_Sheet();

	// Generated message map functions
protected:

	CButton		m_wndOk;
	CButton		m_wndApply;
	CButton		m_wndCancel;
	CButton		m_wndHelp;
	CResizeCtrl	m_resize;
	bool		m_bMinimized;
	
	//{{AFX_MSG(Object_Property_Sheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnObjectOkay();
	afx_msg void OnObjectApply();	
	afx_msg void OnObjectCancel();
	afx_msg void OnObjectHelp();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#define gObjectProperties Object_Property_Sheet::GetSingleton()

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECT_PROPERTY_SHEET_H__23293978_0182_4C3F_8F3B_CB745EC0B412__INCLUDED_)
