#if !defined(AFX_DIALOG_DECAL_H__C4515274_4158_46E7_A977_4555D45FF9CD__INCLUDED_)
#define AFX_DIALOG_DECAL_H__C4515274_4158_46E7_A977_4555D45FF9CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Decal.h : header file
//

#include "resource.h"
#include "quat.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Decal dialog

class Dialog_Decal : public CDialog
{
// Construction
public:
	Dialog_Decal(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Decal)
	enum { IDD = IDD_DIALOG_DECAL };
	CEdit	m_lod;
	CEdit	m_id;
	CButton	m_visible;
	CStatic	m_pos_node;
	CStatic	m_pos_z;
	CStatic	m_pos_y;
	CStatic	m_pos_x;
	CSpinButtonCtrl	m_spin_z;
	CSpinButtonCtrl	m_spin_y;
	CSpinButtonCtrl	m_spin_x;
	CEdit	m_near_plane;
	CEdit	m_far_plane;
	CComboBox	m_combo_z;
	CComboBox	m_combo_y;
	CComboBox	m_combo_x;
	CStatic	m_texture_name;
	CEdit	m_vertical;
	CEdit	m_horizontal;
	//}}AFX_DATA

	void Reset();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Decal)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	std::vector< DWORD > m_selected;
	Quat				 m_orient;

	void CalculateOrientation();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Decal)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditHorizontal();
	afx_msg void OnChangeEditVertical();
	afx_msg void OnChangeEditNearPlane();
	afx_msg void OnChangeEditFarPlane();
	afx_msg void OnDeltaposSpinX(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinY(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinZ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboX();
	afx_msg void OnSelchangeComboY();
	afx_msg void OnSelchangeComboZ();
	afx_msg void OnCheckVisible();
	afx_msg void OnChangeEditLod();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_DECAL_H__C4515274_4158_46E7_A977_4555D45FF9CD__INCLUDED_)
