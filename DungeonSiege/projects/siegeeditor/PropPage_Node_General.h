#if !defined(AFX_PROPPAGE_NODE_GENERAL_H__FA4C60DB_4DC9_48E2_907F_2BAB29B86112__INCLUDED_)
#define AFX_PROPPAGE_NODE_GENERAL_H__FA4C60DB_4DC9_48E2_907F_2BAB29B86112__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Node_General.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_General dialog

class PropPage_Node_General : public CPropertyPage
{
// Construction
public:
	PropPage_Node_General(CWnd* pParent = NULL);   // standard constructor

	void SetNodeGeneric();

// Dialog Data
	//{{AFX_DATA(PropPage_Node_General)
	enum { IDD = IDD_PROPPAGE_NODE_GENERAL };
	CButton	m_light_locked;
	CButton	m_camera_fade;
	CButton	m_bounds_camera;
	CButton	m_occlude_camera;
	CButton	m_hide_node;
	CButton	m_occlude;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Node_General)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:	

	bool m_bOccludesLight;
	bool m_bNodeVisible;
	bool m_bOccludeCamera;
	bool m_bBoundsCamera;
	bool m_bCameraFade;
	bool m_bLightLocked;

	// Generated message map functions
	//{{AFX_MSG(PropPage_Node_General)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckOcclude();
	afx_msg void OnCheckHide();
	afx_msg void OnCheckOccludeCamera();
	afx_msg void OnCheckBoundsCamera();
	afx_msg void OnCheckCameraFade();
	afx_msg void OnCheckLightLocked();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_NODE_GENERAL_H__FA4C60DB_4DC9_48E2_907F_2BAB29B86112__INCLUDED_)
