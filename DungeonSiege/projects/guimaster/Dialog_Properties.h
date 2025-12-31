#if !defined(AFX_DIALOG_PROPERTIES_H__E952521D_EF70_46E6_9E4A_F6E67A3D825C__INCLUDED_)
#define AFX_DIALOG_PROPERTIES_H__E952521D_EF70_46E6_9E4A_F6E67A3D825C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Properties.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialog_Properties dialog

class CDialog_Properties : public CDialog
{
// Construction
public:
	CDialog_Properties(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialog_Properties)
	enum { IDD = IDD_DIALOG_PROPERTIES };
	CEdit	m_edit_height;
	CEdit	m_edit_width;
	CStatic	m_texture_name;
	CEdit	m_edit_y2;
	CEdit	m_edit_y1;
	CEdit	m_edit_x2;
	CEdit	m_edit_x1;
	CEdit	m_edit_v2;
	CEdit	m_edit_v1;
	CEdit	m_edit_u2;
	CEdit	m_edit_u1;
	CEdit	m_edit_name;
	CEdit	m_edit_alpha;
	CButton	m_check_tiled;
	CButton	m_check_right_anchor;
	CButton	m_check_bottom_anchor;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialog_Properties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialog_Properties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButtonLoadtexture();
	afx_msg void OnButtonNotexture();
	afx_msg void OnButtonResizerect();
	afx_msg void OnButtonResizeuvrect();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_PROPERTIES_H__E952521D_EF70_46E6_9E4A_F6E67A3D825C__INCLUDED_)
