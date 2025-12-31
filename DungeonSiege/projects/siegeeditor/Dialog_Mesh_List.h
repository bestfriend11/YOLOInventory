#if !defined(AFX_DIALOG_MESH_LIST_H__67CF6C4E_BC3C_42DF_B31B_FD4023433067__INCLUDED_)
#define AFX_DIALOG_MESH_LIST_H__67CF6C4E_BC3C_42DF_B31B_FD4023433067__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Mesh_List.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mesh_List dialog

class Dialog_Mesh_List : public CDialog, public Singleton <Dialog_Mesh_List>
{
// Construction
public:
	Dialog_Mesh_List(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Mesh_List)
	enum { IDD = IDD_DIALOG_MESH_LIST };
	CListCtrl	m_mesh_group;
	CListCtrl	m_meshes;
	CComboBox	m_groups;
	//}}AFX_DATA

	void SetMeshGroupName( gpstring sName ) { m_meshName = sName; }
	gpstring & GetMeshGroupName()			{ return m_meshName; }
	void RefreshGroups();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Mesh_List)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
// Implementation
protected:

	gpstring m_meshName;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Mesh_List)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSelectMesh();
	afx_msg void OnButtonCopyToClipboard();
	afx_msg void OnSelchangeComboGroups();
	afx_msg void OnButtonNewGroup();
	afx_msg void OnButtonDeleteGroup();
	afx_msg void OnButtonSelectGroup();
	afx_msg void OnButtonHideGroup();
	afx_msg void OnButtonShowGroup();
	afx_msg void OnButtonAddToGroup();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#define gDialogMeshList Dialog_Mesh_List::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MESH_LIST_H__67CF6C4E_BC3C_42DF_B31B_FD4023433067__INCLUDED_)
