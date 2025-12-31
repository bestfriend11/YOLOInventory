#if !defined(AFX_DIALOG_SCID_REMAPPER_H__99D14C2F_4364_4C0F_A244_CB61BA42A954__INCLUDED_)
#define AFX_DIALOG_SCID_REMAPPER_H__99D14C2F_4364_4C0F_A244_CB61BA42A954__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Scid_Remapper.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Scid_Remapper dialog

class Dialog_Scid_Remapper : public CDialog
{
// Construction
public:
	Dialog_Scid_Remapper(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Scid_Remapper)
	enum { IDD = IDD_DIALOG_SCID_REMAPPER };
	CEdit	m_scidRange;
	CTreeCtrl	m_regions;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Scid_Remapper)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RemapScidsForRegion( FuelHandle hRegion, int scidRange );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Scid_Remapper)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonRemapScids();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SCID_REMAPPER_H__99D14C2F_4364_4C0F_A244_CB61BA42A954__INCLUDED_)
