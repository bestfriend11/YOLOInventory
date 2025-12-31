#if !defined(AFX_DIALOG_CAMERA_CONTROLLER_H__5EEE1D79_BBB7_43C0_BF44_3ABA9FE31A2B__INCLUDED_)
#define AFX_DIALOG_CAMERA_CONTROLLER_H__5EEE1D79_BBB7_43C0_BF44_3ABA9FE31A2B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Camera_Controller.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Camera_Controller dialog

class Dialog_Camera_Controller : public CDialog
{
// Construction
public:
	Dialog_Camera_Controller(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Camera_Controller)
	enum { IDD = IDD_DIALOG_CAMERA_CONTROLLER };
	CEdit	m_snapIncrement;
	CEdit	m_translation;
	CEdit	m_rotation;
	CEdit	m_distance;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Camera_Controller)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Camera_Controller)
	afx_msg void OnDeltaposSpinAzimuth(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinForwardbackward(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinInout(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinLeftright(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinOrbit(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditTranslation();
	afx_msg void OnChangeEditRotation();
	afx_msg void OnChangeEditDistance();
	afx_msg void OnDeltaposSpinOrbitSnap(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonAzimuthReset();
	afx_msg void OnButtonOrbitReset();
	afx_msg void OnChangeEditSnapIncrement();
	afx_msg void OnDeltaposSpinAzimuthSnap(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_CAMERA_CONTROLLER_H__5EEE1D79_BBB7_43C0_BF44_3ABA9FE31A2B__INCLUDED_)
