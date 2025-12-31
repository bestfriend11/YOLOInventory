#if !defined(AFX_DIALOG_STITCH_TOOL_H__7FECD033_8769_4C9B_B053_57095C74413A__INCLUDED_)
#define AFX_DIALOG_STITCH_TOOL_H__7FECD033_8769_4C9B_B053_57095C74413A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Stitch_Tool.h : header file
//

#include "EditorTerrain.h"

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Tool dialog

class Dialog_Stitch_Tool : public CDialog
{
// Construction
public:
	Dialog_Stitch_Tool(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Stitch_Tool)
	enum { IDD = IDD_DIALOG_STITCH_TOOL };
	CComboBox	m_StitchRegion;
	CEdit	m_Stitch;
	CComboBox	m_Stitches;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Stitch_Tool)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:	

	StitchedVector m_StitchedVector;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Stitch_Tool)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAddStitches();
	afx_msg void OnButtonUndo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STITCH_TOOL_H__7FECD033_8769_4C9B_B053_57095C74413A__INCLUDED_)
