#if !defined(AFX_PROPPAGE_PROPERTIES_LIGHTING_H__C1E111BB_BB33_4035_9D77_D924F39D3B56__INCLUDED_)
#define AFX_PROPPAGE_PROPERTIES_LIGHTING_H__C1E111BB_BB33_4035_9D77_D924F39D3B56__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Properties_Lighting.h : header file
//

#include "ResizablePage.h"

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Lighting dialog

class PropPage_Properties_Lighting : public CResizablePage
{
	DECLARE_DYNCREATE(PropPage_Properties_Lighting)

// Construction
public:
	PropPage_Properties_Lighting();
	~PropPage_Properties_Lighting();

// Dialog Data
	//{{AFX_DATA(PropPage_Properties_Lighting)
	enum { IDD = IDD_PROPPAGE_PROPERTIES_LIGHTING };
	CEdit	m_edit_guid;
	CEdit	m_edit_oradius;
	CEdit	m_edit_iradius;
	CEdit	m_edit_intensity;
	CEdit	m_edit_z;
	CEdit	m_edit_y;
	CEdit	m_edit_x;
	CEdit	m_edit_red;
	CEdit	m_edit_green;
	CEdit	m_edit_blue;
	CEdit	m_edit_color;
	CButton	m_on_timer;
	CButton	m_occlude;
	CButton	m_active;
	CButton	m_draw_shadows;
	CButton	m_affect_terrain;
	CButton	m_affect_items;
	CButton	m_affect_actors;
	//}}AFX_DATA

	void Reset();
	void Cancel();
	void Apply();

	bool CanApply() { return m_bApply; }
	
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Properties_Lighting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	bool m_bApply;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Properties_Lighting)
	afx_msg void OnButtonColorStandard();
	afx_msg void OnButtonColorAdvanced();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditColorRed();
	afx_msg void OnChangeEditColorGreen();
	afx_msg void OnChangeEditColorBlue();
	afx_msg void OnChangeEditDirX();
	afx_msg void OnChangeEditDirY();
	afx_msg void OnChangeEditDirZ();
	afx_msg void OnChangeEditIradius();
	afx_msg void OnChangeEditOradius();
	afx_msg void OnChangeEditIntensity();
	afx_msg void OnCheckOcclude();
	afx_msg void OnCheckDrawshadows();
	afx_msg void OnCheckAffectItems();
	afx_msg void OnCheckOntimer();
	afx_msg void OnCheckActive();
	afx_msg void OnCheckAffectActors();
	afx_msg void OnCheckAffectTerrain();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HBRUSH m_hBrush;

};

void SetCustomColors( COLORREF *colors );
void GetCustomColors( COLORREF *colors );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_PROPERTIES_LIGHTING_H__C1E111BB_BB33_4035_9D77_D924F39D3B56__INCLUDED_)
