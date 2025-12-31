#if !defined(AFX_DIALOG_REGION_NEW_HOTPOINT_H__3079FFF0_8948_477B_B4A6_7098DCEC30BA__INCLUDED_)
#define AFX_DIALOG_REGION_NEW_HOTPOINT_H__3079FFF0_8948_477B_B4A6_7098DCEC30BA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Region_New_Hotpoint.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_New_Hotpoint dialog

class Dialog_Region_New_Hotpoint : public CDialog
{
// Construction
public:
	Dialog_Region_New_Hotpoint(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Region_New_Hotpoint)
	enum { IDD = IDD_DIALOG_REGION_NEW_HOTPOINT };
	CEdit	m_name;
	CEdit	m_id;
	CComboBox	m_existing_hotpoint;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Region_New_Hotpoint)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Region_New_Hotpoint)
	afx_msg void OnSelchangeComboExistingHotpoint();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGION_NEW_HOTPOINT_H__3079FFF0_8948_477B_B4A6_7098DCEC30BA__INCLUDED_)
