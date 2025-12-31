#if !defined(AFX_DIALOG_REGION_NEW_H__8BE4E48F_CB08_4101_AB64_57CE4613CF3B__INCLUDED_)
#define AFX_DIALOG_REGION_NEW_H__8BE4E48F_CB08_4101_AB64_57CE4613CF3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Region_New.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_New dialog

class Dialog_Region_New : public CDialog
{
// Construction
public:
	Dialog_Region_New(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Region_New)
	enum { IDD = IDD_DIALOG_REGION_NEW };
	CEdit	m_MeshRange;
	CComboBox	m_mapName;
	CEdit	m_scidRange;
	CEdit	m_regionId;
	CTreeCtrl	m_treeNodes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Region_New)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Region_New)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnClickRootnodes(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonNewMap();
	afx_msg void OnButtonDefaultScidRange();
	afx_msg void OnButtonDefaultRegionId();
	afx_msg void OnSelchangeMapname();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonDefaultMeshRange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGION_NEW_H__8BE4E48F_CB08_4101_AB64_57CE4613CF3B__INCLUDED_)
