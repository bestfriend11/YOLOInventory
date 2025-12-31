#if !defined(AFX_DIALOG_REGION_HOTPOINT_DIRECTION_H__4FB032CA_2A86_48D5_B6BD_00743F353381__INCLUDED_)
#define AFX_DIALOG_REGION_HOTPOINT_DIRECTION_H__4FB032CA_2A86_48D5_B6BD_00743F353381__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Region_Hotpoint_Direction.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Hotpoint_Direction dialog

class Dialog_Region_Hotpoint_Direction : public CDialog
{
// Construction
public:
	Dialog_Region_Hotpoint_Direction(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Region_Hotpoint_Direction)
	enum { IDD = IDD_DIALOG_REGION_HOTPOINT_DIRECTION };
	CComboBox	m_orbit;
	CEdit	m_editZ;
	CEdit	m_editX;
	//}}AFX_DATA

	void RecalculateDirection();
	void RecalculateAngle();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Region_Hotpoint_Direction)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	vector_3 m_originalNorth;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Region_Hotpoint_Direction)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnChangeEditX();
	afx_msg void OnChangeEditZ();
	afx_msg void OnSelchangeComboOrbit();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGION_HOTPOINT_DIRECTION_H__4FB032CA_2A86_48D5_B6BD_00743F353381__INCLUDED_)
