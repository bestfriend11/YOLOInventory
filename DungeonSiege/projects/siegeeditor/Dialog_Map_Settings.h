#if !defined(AFX_DIALOG_MAP_SETTINGS_H__54864926_1E46_477E_AC1D_B6F91A47794F__INCLUDED_)
#define AFX_DIALOG_MAP_SETTINGS_H__54864926_1E46_477E_AC1D_B6F91A47794F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Map_Settings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_Settings dialog

class Dialog_Map_Settings : public CDialog
{
// Construction
public:
	Dialog_Map_Settings(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Map_Settings)
	enum { IDD = IDD_DIALOG_MAP_SETTINGS };
	CEdit	m_worldInterestRadius;
	CEdit	m_worldFrustumRadius;
	CEdit	m_screen_name;
	CEdit	m_description;
	CEdit	m_nodeid;
	CComboBox	m_minutes;
	CComboBox	m_hour;
	CEdit	m_up;
	CEdit	m_orbit;
	CEdit	m_north;
	CEdit	m_nearclip;
	CEdit	m_min_distance;
	CEdit	m_min_azimuth;
	CEdit	m_max_distance;
	CEdit	m_max_azimuth;
	CStatic	m_map_name;
	CEdit	m_fov;
	CEdit	m_farclip;
	CEdit	m_east;
	CEdit	m_distance;
	CEdit	m_azimuth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Map_Settings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Map_Settings)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonUseCurrentPosition();
	afx_msg void OnButtonUseSelectedNode();
	afx_msg void OnButtonDefaultWorldFrustum();
	afx_msg void OnButtonWorldInterestRadius();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MAP_SETTINGS_H__54864926_1E46_477E_AC1D_B6F91A47794F__INCLUDED_)
