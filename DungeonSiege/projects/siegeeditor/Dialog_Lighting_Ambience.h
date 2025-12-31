#if !defined(AFX_DIALOG_LIGHTING_AMBIENCE_H__10218DFF_4576_47F9_AB95_AAAB2CE775EF__INCLUDED_)
#define AFX_DIALOG_LIGHTING_AMBIENCE_H__10218DFF_4576_47F9_AB95_AAAB2CE775EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Lighting_Ambience.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_Ambience dialog

class Dialog_Lighting_Ambience : public CDialog
{
// Construction
public:
	Dialog_Lighting_Ambience(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Lighting_Ambience)
	enum { IDD = IDD_DIALOG_LIGHTING_AMBIENCE };
	CEdit	m_terrain_color;
	CEdit	m_object_color;
	CEdit	m_actor_color;
	CEdit	m_actor_ambience;
	CEdit	m_object_ambience;
	CEdit	m_terrain_ambience;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Lighting_Ambience)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	DWORD GetColor( DWORD dwColor );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Lighting_Ambience)
	afx_msg void OnChangeTerrainAmbience();
	afx_msg void OnChangeObjectAmbience();
	virtual void OnOK();
	afx_msg void OnButtonUpdate();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonTerrainColor();
	afx_msg void OnButtonObjectColor();
	afx_msg void OnButtonActorColor();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_LIGHTING_AMBIENCE_H__10218DFF_4576_47F9_AB95_AAAB2CE775EF__INCLUDED_)
