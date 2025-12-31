#if !defined(AFX_DIALOG_OBJECT_TEMPLATE_H__60C0A31A_D321_4669_A73A_9D735DB254EE__INCLUDED_)
#define AFX_DIALOG_OBJECT_TEMPLATE_H__60C0A31A_D321_4669_A73A_9D735DB254EE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Template.h : header file
//

#include "resource.h"
#include "PropertyGrid.h"
#include "GridCtrl\\BtnDataBase.h"
#include "ResizableDialog.h"
#include <map>
#include "GoDefs.h"


/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Template dialog

class Dialog_Object_Template : public CResizableDialog
{
// Construction
public:
	Dialog_Object_Template(CWnd* pParent = NULL);   // standard constructor	

	typedef std::map< gpstring, int, istring_less > ComponentRefMap;
	typedef std::pair< gpstring, int > ComponentRefPair;


// Dialog Data
	//{{AFX_DATA(Dialog_Object_Template)
	enum { IDD = IDD_DIALOG_OBJECT_TEMPLATE };
	CEdit	m_template;
	CEdit	m_scid;
	CButton	m_fields;
	CTreeCtrl	m_components;	
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Template)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	CPropertyGrid	m_grid;	
	CBtnDataBase	m_BtnDataBase;
	GoidColl	m_Selected;		

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Template)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSelchangedTreeComponents(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_TEMPLATE_H__60C0A31A_D321_4669_A73A_9D735DB254EE__INCLUDED_)
