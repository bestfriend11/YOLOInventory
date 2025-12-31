#if !defined(AFX_DIALOG_LIGHTING_EDIT_DIRECTIONAL_H__64A8BE98_D95B_48F2_97B0_A71C2C3D82CB__INCLUDED_)
#define AFX_DIALOG_LIGHTING_EDIT_DIRECTIONAL_H__64A8BE98_D95B_48F2_97B0_A71C2C3D82CB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Lighting_Edit_Directional.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Lighting_Edit_Directional dialog

class Dialog_Lighting_Edit_Directional : public CDialog
{
// Construction
public:
	Dialog_Lighting_Edit_Directional(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Lighting_Edit_Directional)
	enum { IDD = IDD_DIALOG_LIGHTING_EDIT_DIRECTIONAL };
	CListBox	m_light_list;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Lighting_Edit_Directional)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Lighting_Edit_Directional)
	afx_msg void OnNewLight();
	afx_msg void OnEditLight();
	afx_msg void OnRemoveLight();
	afx_msg void OnButtonUpdate();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeDirectionalLightList();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_LIGHTING_EDIT_DIRECTIONAL_H__64A8BE98_D95B_48F2_97B0_A71C2C3D82CB__INCLUDED_)
