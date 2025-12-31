#if !defined(AFX_DIALOG_MOOD_FOG_H__B6912EB9_1C7C_417E_AB4E_294AF1BB3FFE__INCLUDED_)
#define AFX_DIALOG_MOOD_FOG_H__B6912EB9_1C7C_417E_AB4E_294AF1BB3FFE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_Fog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Fog dialog

class Dialog_Mood_Fog : public CDialog
{
// Construction
public:
	Dialog_Mood_Fog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_Fog)
	enum { IDD = IDD_DIALOG_MOOD_FOG };
	CStatic	m_FogColor;
	CEdit	m_NearDistance;
	CEdit	m_LdNearDistance;
	CEdit	m_LdFarDistance;
	CEdit	m_FogDensity;
	CEdit	m_FarDistance;
	CButton	m_FogEnable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_Fog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	HBRUSH m_hBrush;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_Fog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonChangeColor();
	afx_msg void OnCheckFogEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_FOG_H__B6912EB9_1C7C_417E_AB4E_294AF1BB3FFE__INCLUDED_)
