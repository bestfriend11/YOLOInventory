#if !defined(AFX_DIALOG_BATCH_LNC_GENERATION_H__D3D6BAD1_6BC0_407D_98D0_C2D94B01F4EA__INCLUDED_)
#define AFX_DIALOG_BATCH_LNC_GENERATION_H__D3D6BAD1_6BC0_407D_98D0_C2D94B01F4EA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Batch_LNC_Generation.h : header file
//

#include "EditorMap.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Batch_LNC_Generation dialog

class Dialog_Batch_LNC_Generation : public CDialog
{
// Construction
public:
	Dialog_Batch_LNC_Generation(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Batch_LNC_Generation)
	enum { IDD = IDD_DIALOG_BATCH_LNC_GENERATION };
	CButton	m_ConvertGlobalNodes;
	CButton	m_MeshRangeRemap;
	CButton	m_full_recalc;
	CButton	m_load_logical;
	CButton	m_load_lights;
	CButton	m_load_decals;
	CTreeCtrl	m_tree_maps;
	CSpinButtonCtrl	m_t;
	CButton	m_full_save;
	CButton	m_delete_lnc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Batch_LNC_Generation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void ConvertNodeIndex( MapToRegionMap & regionMap );
	void PruneBuildToRegions( MapToRegionMap & regionMap );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Batch_LNC_Generation)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonDeselectAll();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_BATCH_LNC_GENERATION_H__D3D6BAD1_6BC0_407D_98D0_C2D94B01F4EA__INCLUDED_)
