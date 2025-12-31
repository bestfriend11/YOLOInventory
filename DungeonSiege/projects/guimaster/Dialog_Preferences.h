#if !defined(AFX_DIALOG_PREFERENCES_H__24E38C97_6A97_4C68_8BA0_861EB0F515B1__INCLUDED_)
#define AFX_DIALOG_PREFERENCES_H__24E38C97_6A97_4C68_8BA0_861EB0F515B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Preferences.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Preferences dialog

class CDialog_Preferences : public CDialog
{
// Construction
public:
	CDialog_Preferences(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Preferences)
	enum { IDD = IDD_DIALOG_PREFERENCES };
	CButton	m_check_move;
	CButton	m_check_resize;
	CButton	m_check_hide_blanks;
	CButton	m_draw_guides;
	CButton	m_check_parent;
	CButton	m_check_radio;
	CButton	m_check_group;
	CButton	m_check_dock;
	CButton	m_check_snap;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Preferences)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Preferences)
	afx_msg void OnButtonBgColor();
	afx_msg void OnCheckSnaptoguides();
	afx_msg void OnCheckParentMove();
	afx_msg void OnCheckGroupMove();
	afx_msg void OnCheckDockGroupMove();
	afx_msg void OnCheckRadioGroupMove();
	afx_msg void OnCheckDrawGuides();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckHideBlanks();
	afx_msg void OnCheckDisableResize();
	afx_msg void OnCheckDisableMove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PREFERENCES_H__24E38C97_6A97_4C68_8BA0_861EB0F515B1__INCLUDED_)
