#if !defined(AFX_DIALOG_MOOD_GENERAL_H__17E079F5_7D0E_4159_A1B2_417FC4DABF9C__INCLUDED_)
#define AFX_DIALOG_MOOD_GENERAL_H__17E079F5_7D0E_4159_A1B2_417FC4DABF9C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mood_General.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_General dialog

class Dialog_Mood_General : public CDialog
{
// Construction
public:
	Dialog_Mood_General(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mood_General)
	enum { IDD = IDD_DIALOG_MOOD_GENERAL };
	CEdit	m_TransitionTime;
	CEdit	m_MoodName;
	CEdit	m_FrustumWidth;
	CEdit	m_FrustumHeight;
	CButton	m_InteriorMood;
	CButton	m_FrustumEnable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mood_General)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	gpstring m_sName;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mood_General)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	afx_msg void OnCheckFrustumEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MOOD_GENERAL_H__17E079F5_7D0E_4159_A1B2_417FC4DABF9C__INCLUDED_)
