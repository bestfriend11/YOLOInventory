#if !defined(AFX_PROPPAGE_PROPERTIES_TEMPLATE_H__55FBAF6F_5D5A_4517_9F9C_9428796312C2__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_TEMPLATE_H__55FBAF6F_5D5A_4517_9F9C_9428796312C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Template.h : header file
//

#include "PropertyGrid.h"
#include "GridCtrl\\BtnDataBase.h"
#include "ResizablePage.h"
#include "GoDefs.h"
#include <map>


/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Template dialog

class PropPage_Properties_Template : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Template)

// Construction
public:
	PropPage_Properties_Template();
	~PropPage_Properties_Template();

	typedef std::map< gpstring, int, istring_less > ComponentRefMap;
	typedef std::pair< gpstring, int > ComponentRefPair;

	void Reset();
	void Apply();
	void Cancel();

	bool CanApply() { return m_bApply; }

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Template)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_TEMPLATE };
	CButton	m_fourcc;
	CComboBox	m_combo_template;
	CButton	m_hex;
	CEdit	m_screen_name;
	CEdit	m_doc;
	CStatic	m_replace;
	CButton	m_fields;
	CEdit	m_template;
	CEdit	m_scid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Template)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	
	bool			m_bApply;
	CPropertyGrid	m_grid;	
	CBtnDataBase	m_BtnDataBase;
	GoidColl	m_Selected;		
	ComponentRefMap m_componentMap;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Template)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckHex();
	afx_msg void OnEditchangeComboTemplate();
	afx_msg void OnCheckFourcc();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_TEMPLATE_H__55FBAF6F_5D5A_4517_9F9C_9428796312C2__INCLUDED_)
