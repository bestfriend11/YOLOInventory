#if !defined(AFX_DIALOG_OBJECT_EQUIPMENT_H__ACAE03F0_13D6_4FB5_B391_CEA3E1C1BA55__INCLUDED_)
#define AFX_DIALOG_OBJECT_EQUIPMENT_H__ACAE03F0_13D6_4FB5_B391_CEA3E1C1BA55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Equipment.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Equipment dialog

#include "gpcore.h"
#include <map>

typedef std::pair< gpstring, gpstring > EquipPair;
typedef std::map< gpstring, gpstring, istring_less > EquipMap;

class Dialog_Object_Equipment : public CDialog
{
// Construction
public:
	Dialog_Object_Equipment(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Equipment)
	enum { IDD = IDD_DIALOG_OBJECT_EQUIPMENT };
	CEdit	m_screen_name;	
	CEdit	m_fuel_name;
	CListCtrl	m_list_items;
	CComboBox	m_equip_slot;
	//}}AFX_DATA

	EquipMap m_equip_map;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Equipment)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Equipment)
	afx_msg void OnClickListItems(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditchangeComboEquipslot();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_EQUIPMENT_H__ACAE03F0_13D6_4FB5_B391_CEA3E1C1BA55__INCLUDED_)
