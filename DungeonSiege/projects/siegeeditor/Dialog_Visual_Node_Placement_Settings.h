#if !defined(AFX_DIALOG_VISUAL_NODE_PLACEMENT_SETTINGS_H__828929A6_1B25_4409_8F64_D49771EFAD4A__INCLUDED_)
#define AFX_DIALOG_VISUAL_NODE_PLACEMENT_SETTINGS_H__828929A6_1B25_4409_8F64_D49771EFAD4A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Visual_Node_Placement_Settings.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Dialog_Visual_Node_Placement_Settings dialog

class Dialog_Visual_Node_Placement_Settings : public CDialog
{
// Construction
public:
	Dialog_Visual_Node_Placement_Settings(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(Dialog_Visual_Node_Placement_Settings)
	enum { IDD = IDD_DIALOG_VISUAL_NODE_PLACEMENT_SETTINGS };
	CButton	m_DrawDoors;
	CButton	m_DoorGuess;
	CEdit	m_SnapDelay;
	CButton	m_RotateOrient;
	CStatic	m_ValidColor;
	CStatic	m_InvalidColor;
	CStatic	m_DestinationDoorColor;
	CEdit	m_SnapSensitivity;
	CButton	m_DrawMeshBox;
	CButton	m_DrawDoorNumbers;
	CButton	m_DrawDestId;
	CButton	m_AllowInvalidMatch;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Visual_Node_Placement_Settings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	DWORD GetColor( DWORD oldColor );

	HBRUSH m_hBrushValid;
	HBRUSH m_hBrushInvalid;
	HBRUSH m_hBrushDest;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Visual_Node_Placement_Settings)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	afx_msg void OnCheckDrawDestId();
	afx_msg void OnCheckDrawDoorNumbers();
	afx_msg void OnCheckDrawMeshBox();
	afx_msg void OnButtonValidColor();
	afx_msg void OnButtonInvalidColor();
	afx_msg void OnButtonDestinationIdColor();
	afx_msg void OnButtonDefaultSnap();
	afx_msg void OnCheckAllowInvalidDoorMatch();
	afx_msg void OnCheckRotateOrient();
	afx_msg void OnChangeEditSnapSensitivity();
	afx_msg void OnChangeEditSnapDelay();
	afx_msg void OnButtonDefaultSnapDelay();
	afx_msg void OnCheckDrawDoors();
	afx_msg void OnCheckDoorGuess();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_VISUAL_NODE_PLACEMENT_SETTINGS_H__828929A6_1B25_4409_8F64_D49771EFAD4A__INCLUDED_)
