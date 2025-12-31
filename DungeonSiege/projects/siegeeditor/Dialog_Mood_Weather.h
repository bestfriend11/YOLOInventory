#if !defined(AFX_DIALOG_MOOD_WEATHER_H__30131002_CD2E_4D1A_B665_F2DBFED88B7D__INCLUDED_)
#define AFX_DIALOG_MOOD_WEATHER_H__30131002_CD2E_4D1A_B665_F2DBFED88B7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_Weather.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Weather dialog

class Dialog_Mood_Weather : public CDialog
{
// Construction
public:
	Dialog_Mood_Weather(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_Weather)
	enum { IDD = IDD_DIALOG_MOOD_WEATHER };
	CEdit	m_WindVelocity;
	CEdit	m_WindDirection;
	CEdit	m_SnowDensity;
	CEdit	m_RainDensity;
	CButton	m_EnableWind;
	CButton	m_RainEnable;
	CButton	m_Lightning;
	CButton	m_EnableSnow;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_Weather)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_Weather)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	afx_msg void OnCheckRainEnable();
	afx_msg void OnCheckEnableSnow();
	afx_msg void OnCheckEnableWind();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_WEATHER_H__30131002_CD2E_4D1A_B665_F2DBFED88B7D__INCLUDED_)
