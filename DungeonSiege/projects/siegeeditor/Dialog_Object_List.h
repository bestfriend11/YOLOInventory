#if !defined(AFX_DIALOG_OBJECT_LIST_H__D86E696C_F927_4D3A_AB89_990AD544237C__INCLUDED_)
#define AFX_DIALOG_OBJECT_LIST_H__D86E696C_F927_4D3A_AB89_990AD544237C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_List.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_List dialog

class Dialog_Object_List : public CDialog
{
// Construction
public:
	Dialog_Object_List(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_List)
	enum { IDD = IDD_DIALOG_OBJECT_LIST };
	CListCtrl	m_objectList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_List)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RebuildObjectList();

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_List)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonSelect();
	virtual BOOL OnInitDialog();	
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonDeselect();
	afx_msg void OnButtonDeselectAll();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonSelectAllTemplate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_LIST_H__D86E696C_F927_4D3A_AB89_990AD544237C__INCLUDED_)
