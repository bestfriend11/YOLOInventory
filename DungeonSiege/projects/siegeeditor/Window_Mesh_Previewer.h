#if !defined(AFX_WINDOW_MESH_PREVIEWER_H__E7B67283_36BD_438A_A8FE_94B50E8D2E7D__INCLUDED_)
#define AFX_WINDOW_MESH_PREVIEWER_H__E7B67283_36BD_438A_A8FE_94B50E8D2E7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Window_Mesh_Previewer.h : header file
//

#define ID_WINDOW_MESH_PREVIEWER 3111

/////////////////////////////////////////////////////////////////////////////
// Window_Mesh_Previewer window

class Window_Mesh_Previewer : public CWnd
{
// Construction
public:
	Window_Mesh_Previewer();

private:

	int m_x;
	int m_y;
	bool m_bDrag;

// Attributes
public:

// Operations
public:

	void CloseForLoad();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Window_Mesh_Previewer)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~Window_Mesh_Previewer();

	// Generated message map functions
protected:

	CStatusBar  m_wndStatusBar;

	//{{AFX_MSG(Window_Mesh_Previewer)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewResetcamera();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnViewBackgroundcolor();
	afx_msg void OnPreferencesWireframe();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINDOW_MESH_PREVIEWER_H__E7B67283_36BD_438A_A8FE_94B50E8D2E7D__INCLUDED_)
