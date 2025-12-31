#if !defined(AFX_DIALOG_STARTING_POSITIONS_H__249CCEA7_0776_486A_8E0E_D0E88E1CCE2A__INCLUDED_)
#define AFX_DIALOG_STARTING_POSITIONS_H__249CCEA7_0776_486A_8E0E_D0E88E1CCE2A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Starting_Positions.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Starting_Positions dialog

class Dialog_Starting_Positions : public CDialog
{
// Construction
public:
	Dialog_Starting_Positions(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Starting_Positions)
	enum { IDD = IDD_DIALOG_STARTING_POSITIONS };
	CEdit	m_camera_z;
	CEdit	m_camera_y;
	CEdit	m_camera_x;
	CEdit	m_camera_node;
	CEdit	m_id;
	CEdit	m_up;
	CEdit	m_orbit;
	CEdit	m_north;
	CListBox	m_positions;
	CEdit	m_node;
	CEdit	m_east;
	CEdit	m_distance;
	CEdit	m_azimuth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Starting_Positions)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(Dialog_Starting_Positions)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonNew();
	afx_msg void OnButtonPreview();
	afx_msg void OnButtonRemove();
	afx_msg void OnButtonSetdata();
	afx_msg void OnButtonUseSelectedNode();
	afx_msg void OnButtonUseCurrentPosition();
	afx_msg void OnSelchangeListPositions();
	afx_msg void OnButtonUseSelectedNodeCamera();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_STARTING_POSITIONS_H__249CCEA7_0776_486A_8E0E_D0E88E1CCE2A__INCLUDED_)
