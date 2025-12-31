#if !defined(AFX_PROPPAGE_PROPERTIES_EQUIPMENT_H__A6D877C3_B4EE_4A20_9DCD_7C40EAF95D0A__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_EQUIPMENT_H__A6D877C3_B4EE_4A20_9DCD_7C40EAF95D0A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Equipment.h : header file
//

#include "ResizablePage.h"
#include "gpcore.h"
#include <map>

typedef std::pair< gpstring, gpstring > EquipPair;
typedef std::map< gpstring, gpstring, istring_less > EquipMap;


/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Equipment dialog

class PropPage_Properties_Equipment : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Equipment)

// Construction
public:
	PropPage_Properties_Equipment();
	~PropPage_Properties_Equipment();

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Equipment)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_EQUIPMENT };
	CComboBox	m_selected_slot;
	CEdit	m_documentation;
	CEdit	m_screen_name;
	CEdit	m_fuel_name;
	CComboBox	m_items;
	CComboBox	m_equip_slot;
	//}}AFX_DATA

	EquipMap m_equip_map;

	void Reset();
	void Apply();
	void Cancel();
	bool CanApply() { return m_bApply; }	

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Equipment)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	bool m_bApply;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Equipment)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeComboEquipslot();
	afx_msg void OnEditchangeComboItems();
	afx_msg void OnRadioTemplate();
	afx_msg void OnRadioScreenName();
	afx_msg void OnRadioDocumentation();
	afx_msg void OnSelchangeComboSelected();
	afx_msg void OnButtonAdvanced();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_EQUIPMENT_H__A6D877C3_B4EE_4A20_9DCD_7C40EAF95D0A__INCLUDED_)
