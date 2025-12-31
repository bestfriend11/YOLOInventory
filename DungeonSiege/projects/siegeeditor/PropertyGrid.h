#if !defined(AFX_PROPERTYGRID_H__0F00568E_2666_40B6_ABC3_CBC0F4AD3485__INCLUDED_)
#define AFX_PROPERTYGRID_H__0F00568E_2666_40B6_ABC3_CBC0F4AD3485__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropertyGrid.h : header file
//
#include "gridctrl\\GridCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CPropertyGrid window

class CPropertyGrid : public CGridCtrl
{
// Construction
public:
	CPropertyGrid();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropertyGrid)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPropertyGrid();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPropertyGrid)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPERTYGRID_H__0F00568E_2666_40B6_ABC3_CBC0F4AD3485__INCLUDED_)
