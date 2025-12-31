#if !defined(AFX_DIALOG_OBJECT_SPOT_SNAPPER_H__A169B450_706B_4418_9CA7_51D698F749A2__INCLUDED_)
#define AFX_DIALOG_OBJECT_SPOT_SNAPPER_H__A169B450_706B_4418_9CA7_51D698F749A2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Spot_Snapper.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Spot_Snapper dialog

class Dialog_Object_Spot_Snapper : public CDialog
{
// Construction
public:
	Dialog_Object_Spot_Snapper(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Spot_Snapper)
	enum { IDD = IDD_DIALOG_OBJECT_SPOT_SNAPPER };
	CListCtrl	m_list_spots;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Spot_Snapper)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Spot_Snapper)
	afx_msg void OnButtonSnap();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_SPOT_SNAPPER_H__A169B450_706B_4418_9CA7_51D698F749A2__INCLUDED_)
