#if !defined(AFX_DIALOG_STITCH_MANAGER_H__451AFC79_55D6_443D_AF6E_F7FFE6FB8E29__INCLUDED_)
#define AFX_DIALOG_STITCH_MANAGER_H__451AFC79_55D6_443D_AF6E_F7FFE6FB8E29__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Stitch_Manager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Manager dialog

class Dialog_Stitch_Manager : public CDialog
{
// Construction
public:
	Dialog_Stitch_Manager(CWnd* pParent = NULL);   // standard constructor

	void BuildStitchHelperTree();

	HTREEITEM m_hItem;
	HTREEITEM m_hRegionItem;	

// Dialog Data
	//{{AFX_DATA(Dialog_Stitch_Manager)
	enum { IDD = IDD_DIALOG_STITCH_MANAGER };
	CListCtrl	m_RegionInfo;
	CTreeCtrl	m_StitchTree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Stitch_Manager)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	typedef std::map< gpstring, int, istring_less> RegionToStitchMap;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Stitch_Manager)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonRemoveAll();
	afx_msg void OnButtonOpen();
	afx_msg void OnButtonHelp();
	afx_msg void OnSelchangedTreeStitch(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STITCH_MANAGER_H__451AFC79_55D6_443D_AF6E_F7FFE6FB8E29__INCLUDED_)
