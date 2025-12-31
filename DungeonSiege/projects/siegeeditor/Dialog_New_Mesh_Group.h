#if !defined(AFX_DIALOG_NEW_MESH_GROUP_H__972D13E1_3759_4EA7_A29B_C1D8145AB1EA__INCLUDED_)
#define AFX_DIALOG_NEW_MESH_GROUP_H__972D13E1_3759_4EA7_A29B_C1D8145AB1EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_New_Mesh_Group.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_New_Mesh_Group dialog

class Dialog_New_Mesh_Group : public CDialog
{
// Construction
public:
	Dialog_New_Mesh_Group(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_New_Mesh_Group)
	enum { IDD = IDD_DIALOG_NEW_MESH_GROUP };
	CEdit	m_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_New_Mesh_Group)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_New_Mesh_Group)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_NEW_MESH_GROUP_H__972D13E1_3759_4EA7_A29B_C1D8145AB1EA__INCLUDED_)
