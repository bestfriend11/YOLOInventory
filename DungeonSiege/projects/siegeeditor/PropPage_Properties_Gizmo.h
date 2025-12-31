#if !defined(AFX_PROPPAGE_PROPERTIES_GIZMO_H__3896E502_8AB1_4253_A209_EFE387E0893C__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_GIZMO_H__3896E502_8AB1_4253_A209_EFE387E0893C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Gizmo.h : header file
//

#include "ResizablePage.h"

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Gizmo dialog

class PropPage_Properties_Gizmo : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Gizmo)

// Construction
public:
	PropPage_Properties_Gizmo();   // standard constructor

	void Apply();
	void Reset();
	void Cancel();

	bool CanApply() { return m_bApply; }	

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Gizmo)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_GIZMO };
	CButton	m_nodeOverride;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Gizmo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	bool m_bApply;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Gizmo)
	afx_msg void OnCheckNodePositionOverride();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_GIZMO_H__3896E502_8AB1_4253_A209_EFE387E0893C__INCLUDED_)
