#if !defined(AFX_DIALOG_PREFERENCES_H__6A2F2B27_8767_49AA_ADCC_400627C710FC__INCLUDED_)
#define AFX_DIALOG_PREFERENCES_H__6A2F2B27_8767_49AA_ADCC_400627C710FC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Preferences.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Preferences dialog

class Dialog_Preferences : public CDialog
{
// Construction
public:
	Dialog_Preferences(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Preferences)
	enum { IDD = IDD_DIALOG_PREFERENCES };
	CButton	m_DrawCameraTarget;
	CButton	m_CameraRestrict;
	CButton	m_update_siege_engine;
	CButton	m_light_hud;
	CButton	m_drawLogicalFlags;
	CButton	m_drawCommands;
	CButton	m_right_click_scrolling;
	CButton	m_floors_only;
	CButton	m_update_weather;
	CButton	m_update_sound;
	CButton	m_draw_stitch;
	CButton	m_draw_water;
	CButton	m_draw_lnodes;
	CButton	m_draw_normals;
	CButton	m_update_world;
	CButton	m_draw_north_vector;
	CButton	m_fps;
	CEdit	m_zoom_rate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Preferences)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Preferences)
	afx_msg void OnWireframe();
	afx_msg void OnDoordraw();
	afx_msg void OnFloordraw();
	afx_msg void OnBoxes();
	afx_msg void OnCheckDrawDoorNum();
	afx_msg void OnEffects();
	afx_msg void OnDlights();
	afx_msg void OnSlights();
	afx_msg void OnDshadows();
	afx_msg void OnSshadows();
	afx_msg void OnDshading();
	afx_msg void OnSshading();
	afx_msg void OnIgnorevertexcolor();
	afx_msg void OnPolystats();
	afx_msg void OnDisable();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnEditchangeComboContentDisplay();
	afx_msg void OnChangeEditPrecisionAmount();
	afx_msg void OnCheckFps();
	afx_msg void OnChangeEditZoomRate();
	afx_msg void OnCheckDrawNorthVector();
	afx_msg void OnCheckDrawnormals();
	afx_msg void OnCheckLogicalnodedraw();
	afx_msg void OnCheckWaterdraw();
	afx_msg void OnCheckUpdateWorld();
	afx_msg void OnCheckDrawStitchDoors();
	afx_msg void OnCheckUpdateweather();
	afx_msg void OnCheckUpdatesound();
	afx_msg void OnCheckFloorsonly();
	afx_msg void OnCheckRightClickScrolling();
	afx_msg void OnCheckDrawCommands();
	afx_msg void OnCheckLogicalFlag();
	afx_msg void OnButtonSetClearColor();
	afx_msg void OnCheckLightHud();
	afx_msg void OnCheckUpdateSiegeEngine();
	afx_msg void OnButtonHelp();
	afx_msg void OnCheckCameraGame();
	afx_msg void OnCheckDrawCameraTarget();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PREFERENCES_H__6A2F2B27_8767_49AA_ADCC_400627C710FC__INCLUDED_)
