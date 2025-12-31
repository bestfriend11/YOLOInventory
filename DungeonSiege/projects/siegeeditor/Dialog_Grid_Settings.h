#if !defined(AFX_DIALOG_GRID_SETTINGS_H__CD16FA8D_FB85_4C22_A6CB_EE474E6C06FE__INCLUDED_)
#define AFX_DIALOG_GRID_SETTINGS_H__CD16FA8D_FB85_4C22_A6CB_EE474E6C06FE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Grid_Settings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Grid_Settings dialog

class Dialog_Grid_Settings : public CDialog
{
// Construction
public:
	Dialog_Grid_Settings(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Grid_Settings)
	enum { IDD = IDD_DIALOG_GRID_SETTINGS };
	CButton	m_LabelInterval;
	CStatic	m_Conversion;
	CStatic	m_Color;
	CEdit	m_Size;
	CEdit	m_OriginZ;
	CEdit	m_OriginY;
	CEdit	m_OriginX;
	CEdit	m_Interval;
	CButton	m_EnableGrid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Grid_Settings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	void Refresh();

	HBRUSH m_hBrush;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Grid_Settings)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckEnableGrid();
	afx_msg void OnButtonHelp();
	afx_msg void OnButtonColor();
	afx_msg void OnButtonUseSelectedNodeCenter();
	afx_msg void OnButtonUseDefaultCenter();
	afx_msg void OnChangeEditSize();
	afx_msg void OnChangeEditInterval();
	afx_msg void OnChangeEditOriginX();
	afx_msg void OnChangeEditOriginY();
	afx_msg void OnChangeEditOriginZ();
	afx_msg void OnCheckLabelsInterval();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_GRID_SETTINGS_H__CD16FA8D_FB85_4C22_A6CB_EE474E6C06FE__INCLUDED_)
