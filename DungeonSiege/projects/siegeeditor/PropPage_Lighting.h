#if !defined(AFX_PROPPAGE_LIGHTING_H__E8A7A638_A6F5_42D1_A52B_8B252649376C__INCLUDED_)
#define AFX_PROPPAGE_LIGHTING_H__E8A7A638_A6F5_42D1_A52B_8B252649376C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Lighting.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PropPage_Lighting dialog

class PropPage_Lighting : public CPropertyPage
{
// Construction
public:
	PropPage_Lighting(CWnd* pParent = NULL);   // standard constructor

	void Apply();

// Dialog Data
	//{{AFX_DATA(PropPage_Lighting)
	enum { IDD = IDD_PROPPAGE_LIGHTING };
	CComboBox	m_orbit;
	CComboBox	m_azimuth;
	CButton	m_ontimer;
	CButton	m_affect_terrain;
	CButton	m_affect_items;
	CButton	m_affect_actors;
	CButton	m_active;
	CButton	m_draw_shadows;
	CButton	m_occlude;
	CEdit	m_dir_x;
	CEdit	m_dir_y;
	CEdit	m_dir_z;
	CEdit	m_intensity;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Lighting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RecalculateAngles();
	void RecalculateDirection();

	// Generated message map functions
	//{{AFX_MSG(PropPage_Lighting)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonGolight();
	afx_msg void OnSelchangeComboOrbit();
	afx_msg void OnSelchangeComboAzimuth();
	afx_msg void OnChangeEditDirX();
	afx_msg void OnChangeEditDirY();
	afx_msg void OnChangeEditDirZ();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_LIGHTING_H__E8A7A638_A6F5_42D1_A52B_8B252649376C__INCLUDED_)
