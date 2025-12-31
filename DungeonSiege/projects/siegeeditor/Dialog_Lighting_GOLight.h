#if !defined(AFX_DIALOG_LIGHTING_GOLIGHT_H__97027D73_468E_4BF7_86D1_AE0D2468F3E3__INCLUDED_)
#define AFX_DIALOG_LIGHTING_GOLIGHT_H__97027D73_468E_4BF7_86D1_AE0D2468F3E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Lighting_GOLight.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_GOLight dialog

class Dialog_Lighting_GOLight : public CDialog
{
// Construction
public:
	Dialog_Lighting_GOLight(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Lighting_GOLight)
	enum { IDD = IDD_DIALOG_LIGHTING_GOLIGHT };
	CListBox	m_list_scripts;
	CEdit	m_z;
	CEdit	m_y;
	CEdit	m_x;
	CEdit	m_guid;
	CEdit	m_scid;
	CEdit	m_custom_script;
	CComboBox	m_available_scripts;
	CButton	m_initial_active;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Lighting_GOLight)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Lighting_GOLight)
	virtual void OnOK();
	afx_msg void OnButtonDeleteGolight();
	afx_msg void OnSelchangeListScripts();
	afx_msg void OnButtonAddScript();
	afx_msg void OnButtonRemoveScript();
	afx_msg void OnButtonRemoveAllScripts();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_LIGHTING_GOLIGHT_H__97027D73_468E_4BF7_86D1_AE0D2468F3E3__INCLUDED_)
