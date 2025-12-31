#if !defined(AFX_DIALOG_OBJECT_VALIDATION_H__843494F2_1B89_4CB7_9B75_B21F57366088__INCLUDED_)
#define AFX_DIALOG_OBJECT_VALIDATION_H__843494F2_1B89_4CB7_9B75_B21F57366088__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Object_Validation.h : header file
//

#include "GoData.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Validation dialog

class Dialog_Object_Validation : public CDialog
{
// Construction
public:
	Dialog_Object_Validation(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Object_Validation)
	enum { IDD = IDD_DIALOG_OBJECT_VALIDATION };
	CListBox	m_searchResults;
	CButton	m_delete_inventory;
	CListBox	m_map_list;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Object_Validation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RecursiveValidateTemplates( GoDataTemplate * pTemplate, StringVec & vBadTemplates );

	void RetrieveObjectList( FuelHandle & hItems );
	void RecursiveRetrieveObjectList( GoDataTemplate * pTemplate, FuelHandle & hItems );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Object_Validation)
	afx_msg void OnButtonValidateMap();
	afx_msg void OnButtonValidateAll();
	afx_msg void OnButtonValidateAllScids();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonValidateTemplates();
	afx_msg void OnButtonValidateBookmarks();
	afx_msg void OnButtonDeleteAspectOverride();
	afx_msg void OnButtonDeleteInventory();
	afx_msg void OnButtonSearchCmd();
	afx_msg void OnSelchangeListSearchResults();
	afx_msg void OnButtonAutoClean();
	afx_msg void OnButtonEquipmentList();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_OBJECT_VALIDATION_H__843494F2_1B89_4CB7_9B75_B21F57366088__INCLUDED_)
