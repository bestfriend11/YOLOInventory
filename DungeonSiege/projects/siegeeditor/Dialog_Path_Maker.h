#if !defined(AFX_DIALOG_PATH_MAKER_H__120B3014_7F6E_49B2_BE96_90627D7A706C__INCLUDED_)
#define AFX_DIALOG_PATH_MAKER_H__120B3014_7F6E_49B2_BE96_90627D7A706C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Path_Maker.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Maker dialog

class Dialog_Path_Maker : public CDialog
{
// Construction
public:
	Dialog_Path_Maker(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Path_Maker)
	enum { IDD = IDD_DIALOG_PATH_MAKER };
	CSpinButtonCtrl	m_spinRadius;
	CEdit	m_tolerance;
	CEdit	m_radius;
	CButton	m_autosize;
	CComboBox	m_comboPaths;
	CButton	m_placePoints;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Path_Maker)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Path_Maker)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonRemovePath();
	afx_msg void OnButtonReports();
	afx_msg void OnCheckPlacePoints();
	afx_msg void OnClose();
	afx_msg void OnSelchangeComboPaths();
	afx_msg void OnCheckAutosize();
	afx_msg void OnChangeEditTolerance();
	afx_msg void OnChangeEditRadius();
	afx_msg void OnDeltaposSpinRadius(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonDuplicate();
	afx_msg void OnButtonElev();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PATH_MAKER_H__120B3014_7F6E_49B2_BE96_90627D7A706C__INCLUDED_)
