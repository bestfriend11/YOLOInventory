#if !defined(AFX_DIALOG_METER_POINT_EDITOR_H__E0871AD2_B4A3_49C5_9174_CDAE2139FA15__INCLUDED_)
#define AFX_DIALOG_METER_POINT_EDITOR_H__E0871AD2_B4A3_49C5_9174_CDAE2139FA15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Meter_Point_Editor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Meter_Point_Editor dialog

class Dialog_Meter_Point_Editor : public CDialog
{
// Construction
public:
	Dialog_Meter_Point_Editor(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Meter_Point_Editor)
	enum { IDD = IDD_DIALOG_METER_POINT_EDITOR };
	CStatic	m_Color;
	CButton	m_ShowDistances;
	CButton	m_DrawMeterPoints;
	CButton	m_PlaceMeterPoint;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Meter_Point_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	HBRUSH m_hBrush;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Meter_Point_Editor)
	afx_msg void OnCheckMeterPoints();
	afx_msg void OnButtonHelp();
	virtual void OnOK();
	afx_msg void OnClose();
	afx_msg void OnCheckDrawMeterPoints();
	afx_msg void OnCheckShowPointDistances();
	afx_msg void OnButtonColor();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_METER_POINT_EDITOR_H__E0871AD2_B4A3_49C5_9174_CDAE2139FA15__INCLUDED_)
