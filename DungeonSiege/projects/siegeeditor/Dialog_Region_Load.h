#if !defined(AFX_DIALOG_REGION_LOAD_H__B9D74370_AC1A_41F2_8D7E_FFDC1576A8BF__INCLUDED_)
#define AFX_DIALOG_REGION_LOAD_H__B9D74370_AC1A_41F2_8D7E_FFDC1576A8BF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Region_Load.h : header file
//

#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Load dialog

class Dialog_Region_Load : public CDialog
{
// Construction
public:
	Dialog_Region_Load(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Region_Load)
	enum { IDD = IDD_DIALOG_REGION_LOAD };
	CButton	m_fullRecalc;
	CButton	m_tuningLoad;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Region_Load)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Region_Load)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


// Publish a list of regions to the dialog box tree control
void PublishRegionList( CWnd *pTree );


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_REGION_LOAD_H__B9D74370_AC1A_41F2_8D7E_FFDC1576A8BF__INCLUDED_)
