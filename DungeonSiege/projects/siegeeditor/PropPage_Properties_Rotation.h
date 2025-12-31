#if !defined(AFX_PROPPAGE_PROPERTIES_ROTATION_H__6F42C107_3F94_407C_973E_DAFE60D11487__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_ROTATION_H__6F42C107_3F94_407C_973E_DAFE60D11487__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Rotation.h : header file
//

#include "ResizablePage.h"
#include "RoundSliderCtrl.h"
#include "quat.h"

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Rotation dialog

class PropPage_Properties_Rotation : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Rotation)

// Construction
public:
	PropPage_Properties_Rotation();
	~PropPage_Properties_Rotation();

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Rotation)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_ROTATION };
	CSpinButtonCtrl	m_spin_scale;
	CEdit	m_edit_scale;
	CEdit	m_rotationIncrement;
	CSpinButtonCtrl	m_spin_z;
	CSpinButtonCtrl	m_spin_y;
	CSpinButtonCtrl	m_spin_x;
	CRoundSliderCtrl m_slider_z;
	CRoundSliderCtrl m_slider_y;
	CRoundSliderCtrl m_slider_x;
	CComboBox	m_degrees_z;
	CComboBox	m_degrees_y;
	CComboBox	m_degrees_x;
	//}}AFX_DATA

	Quat m_orient;
	bool m_bParamsInit;

	void Apply();
	void Cancel();
	void Reset();

	bool CanApply() { return false; }	

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Rotation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	bool m_bApply;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Rotation)
	virtual BOOL OnInitDialog();
	afx_msg void OnDeltaposSpinX(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinY(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinZ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboDegreesX();
	afx_msg void OnSelchangeComboDegreesY();
	afx_msg void OnSelchangeComboDegreesZ();
	afx_msg void OnChangeEditRotationSnapIncrement();
	afx_msg void OnEditchangeComboDegreesX();
	afx_msg void OnEditchangeComboDegreesY();
	afx_msg void OnEditchangeComboDegreesZ();
	afx_msg void OnDeltaposSpinScale(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeEditScale();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_ROTATION_H__6F42C107_3F94_407C_973E_DAFE60D11487__INCLUDED_)
