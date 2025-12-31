#if !defined(AFX_DIALOG_SFX_PARAMETERS_H__D4C39F16_1977_4EB9_8416_FA5864AB6DA5__INCLUDED_)
#define AFX_DIALOG_SFX_PARAMETERS_H__D4C39F16_1977_4EB9_8416_FA5864AB6DA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Sfx_Parameters.h : header file
//

#include "ResizableDialog.h"
#include "PropertyGrid.h"
#include "gridctrl\\BtnDataBase.h"

#define PARAMETER_COL 0
#define PARAM_TYPE_COL 1
#define PARAM_VALUE_COL 2
#define PARAM_DOC_COL 3

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Parameters dialog

class Dialog_Sfx_Parameters : public CResizableDialog
{
// Construction
public:
	Dialog_Sfx_Parameters(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Sfx_Parameters)
	enum { IDD = IDD_DIALOG_SFX_PARAMETERS };
	CStatic	m_remove;
	CButton	m_effectName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Sfx_Parameters)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	gpstring BuildParameterStringFromGrid();
	void	 BuildGridFromParameterString();

	CPropertyGrid	m_grid;	
	CBtnDataBase	m_BtnDataBase;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Sfx_Parameters)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonAddParameter();
	afx_msg void OnButtonRemoveParameter();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SFX_PARAMETERS_H__D4C39F16_1977_4EB9_8416_FA5864AB6DA5__INCLUDED_)
