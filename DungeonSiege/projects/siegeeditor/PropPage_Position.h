#if !defined(AFX_PROPPAGE_POSITION_H__D792D0E5_1A18_450C_8EB5_75313A82C7B0__INCLUDED_)
#define AFX_PROPPAGE_POSITION_H__D792D0E5_1A18_450C_8EB5_75313A82C7B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Position.h : header file
//

#include "RoundSliderCtrl.h"
#include "quat.h"

/////////////////////////////////////////////////////////////////////////////
// PropPage_Position dialog

class PropPage_Position : public CPropertyPage
{
// Construction
public:
	PropPage_Position(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(PropPage_Position)
	enum { IDD = IDD_PROPPAGE_POSITION };
	CSpinButtonCtrl	m_spin_z;
	CSpinButtonCtrl	m_spin_y;
	CSpinButtonCtrl	m_spin_x;
	CEdit	m_edit_yaw;
	CEdit	m_edit_roll;
	CEdit	m_edit_pitch;
	CRoundSliderCtrl	m_slider_x;
	CRoundSliderCtrl	m_slider_y;
	CRoundSliderCtrl	m_slider_z;
	float	m_scale;
	float	m_pos_x;
	float	m_pos_y;
	float	m_pos_z;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Position)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	Quat m_orient;

	void Apply();

	// Generated message map functions
	//{{AFX_MSG(PropPage_Position)
	afx_msg void OnChangeEditX();
	afx_msg void OnChangeEditY();
	afx_msg void OnChangeEditZ();
	afx_msg void OnChangeEditScale();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditPitch();
	afx_msg void OnChangeEditRoll();
	afx_msg void OnChangeEditYaw();
	afx_msg void OnDeltaposSpinX(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinY(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinZ(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	bool m_bParamsInit;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_POSITION_H__D792D0E5_1A18_450C_8EB5_75313A82C7B0__INCLUDED_)
