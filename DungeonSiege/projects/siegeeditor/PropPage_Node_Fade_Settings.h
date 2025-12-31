#if !defined(AFX_PROPPAGE_NODE_FADE_SETTINGS_H__D2B4DFFF_C88E_46B7_AF2A_27A38FE83264__INCLUDED_)
#define AFX_PROPPAGE_NODE_FADE_SETTINGS_H__D2B4DFFF_C88E_46B7_AF2A_27A38FE83264__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Node_Fade_Settings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Fade_Settings dialog

class PropPage_Node_Fade_Settings : public CPropertyPage
{
// Construction
public:
	PropPage_Node_Fade_Settings(CWnd* pParent = NULL);   // standard constructor

	// Member Variables
	int			m_node_section;
	int			m_node_level;
	int			m_node_object;
	int			m_bInitialized;

	void SetFadeSettings();

// Dialog Data
	//{{AFX_DATA(PropPage_Node_Fade_Settings)
	enum { IDD = IDD_PROPPAGE_NODE_FADE_SETTINGS };
	CEdit	m_section;
	CEdit	m_object;
	CEdit	m_level;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Node_Fade_Settings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PropPage_Node_Fade_Settings)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNodesection();
	afx_msg void OnChangeNodelevel();
	afx_msg void OnChangeNodeobject();
	afx_msg void OnButtonShow();
	afx_msg void OnButtonHide();
	afx_msg void OnButtonSelect();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_NODE_FADE_SETTINGS_H__D2B4DFFF_C88E_46B7_AF2A_27A38FE83264__INCLUDED_)
