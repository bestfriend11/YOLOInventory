#if !defined(AFX_DIALOG_ITEM_VIEWER_H__9A961067_0203_4186_AA69_D34010A2A626__INCLUDED_)
#define AFX_DIALOG_ITEM_VIEWER_H__9A961067_0203_4186_AA69_D34010A2A626__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Item_Viewer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Item_Viewer dialog

class Dialog_Item_Viewer : public CDialog
{
// Construction
public:
	Dialog_Item_Viewer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Item_Viewer)
	enum { IDD = IDD_DIALOG_ITEM_VIEWER };
	CTreeCtrl	m_items;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Item_Viewer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	void RecursiveAddTemplate( GoDataTemplate * pTemplate, HTREEITEM hParent );

	// Generated message map functions
	//{{AFX_MSG(Dialog_Item_Viewer)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_ITEM_VIEWER_H__9A961067_0203_4186_AA69_D34010A2A626__INCLUDED_)
