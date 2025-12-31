#if !defined(AFX_DIALOG_GROUP_H__CDE26C57_597F_4B42_9F95_FDCB59BD33CE__INCLUDED_)
#define AFX_DIALOG_GROUP_H__CDE26C57_597F_4B42_9F95_FDCB59BD33CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Group.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Group dialog

class CDialog_Group : public CDialog
{
// Construction
public:
	CDialog_Group(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Group)
	enum { IDD = IDD_DIALOG_GROUP };
	CEdit	m_edit_radio;
	CEdit	m_edit_group;
	CEdit	m_edit_dockbar;
	CComboBox	m_combo_parent;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Group)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Group)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_GROUP_H__CDE26C57_597F_4B42_9F95_FDCB59BD33CE__INCLUDED_)
