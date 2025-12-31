#if !defined(AFX_GUIWORKSPACE_H__C2AEE2AC_BC79_430C_AFE3_F25AE87D2664__INCLUDED_)
#define AFX_GUIWORKSPACE_H__C2AEE2AC_BC79_430C_AFE3_F25AE87D2664__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GUIWorkspace.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGUIWorkspace window

class CGUIWorkspace : public CWnd
{
// Construction
public:
	CGUIWorkspace();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGUIWorkspace)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGUIWorkspace();

	// Generated message map functions
protected:

	CMenu m_wndProperties;

	//{{AFX_MSG(CGUIWorkspace)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GUIWORKSPACE_H__C2AEE2AC_BC79_430C_AFE3_F25AE87D2664__INCLUDED_)
