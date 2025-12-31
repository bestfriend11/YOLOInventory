#if !defined(AFX_DIALOG_MAP_NEW_HOTPOINT_H__40AC2CCB_74FF_481C_8B34_926D05647020__INCLUDED_)
#define AFX_DIALOG_MAP_NEW_HOTPOINT_H__40AC2CCB_74FF_481C_8B34_926D05647020__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Map_New_Hotpoint.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Map_New_Hotpoint dialog

class Dialog_Map_New_Hotpoint : public CDialog
{
// Construction
public:
	Dialog_Map_New_Hotpoint(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Map_New_Hotpoint)
	enum { IDD = IDD_DIALOG_MAP_NEW_HOTPOINT };
	CEdit	m_edit_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Map_New_Hotpoint)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Map_New_Hotpoint)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_MAP_NEW_HOTPOINT_H__40AC2CCB_74FF_481C_8B34_926D05647020__INCLUDED_)
