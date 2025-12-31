#if !defined(AFX_PROPPAGE_NODE_STITCHING_H__44FADFE2_EF7B_4C91_857E_3F5AD248BE62__INCLUDED_)
#define AFX_PROPPAGE_NODE_STITCHING_H__44FADFE2_EF7B_4C91_857E_3F5AD248BE62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PropPage_Node_Stitching.h : header file
//

#include <string>

/////////////////////////////////////////////////////////////////////////////
// PropPage_Node_Stitching dialog

class PropPage_Node_Stitching : public CPropertyPage
{
// Construction
public:
	PropPage_Node_Stitching(CWnd* pParent = NULL);   // standard constructor

	int m_stitch_id;
	gpstring m_sRegion;
	

	void SetNodeStitching();

// Dialog Data
	//{{AFX_DATA(PropPage_Node_Stitching)
	enum { IDD = IDD_PROPPAGE_NODE_STITCHING };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropPage_Node_Stitching)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(PropPage_Node_Stitching)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnChangeStitchOrder();
	afx_msg void OnEditchangeStitchRegion();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonStitchManager();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPPAGE_NODE_STITCHING_H__44FADFE2_EF7B_4C91_857E_3F5AD248BE62__INCLUDED_)
