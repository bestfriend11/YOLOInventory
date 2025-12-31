// GUIMasterView.h : interface of the CGUIMasterView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_GUIMASTERVIEW_H__E463EDE7_DBFB_46D0_BD61_00749C03EF39__INCLUDED_)
#define AFX_GUIMASTERVIEW_H__E463EDE7_DBFB_46D0_BD61_00749C03EF39__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "GUIWorkspace.h"

class CGUIMasterView : public CView
{
protected: // create from serialization only
	CGUIMasterView();
	DECLARE_DYNCREATE(CGUIMasterView)

// Attributes
public:
	CGUIMasterDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGUIMasterView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGUIMasterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	CGUIWorkspace m_guiWorkspace;

// Generated message map functions
protected:
	//{{AFX_MSG(CGUIMasterView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in GUIMasterView.cpp
inline CGUIMasterDoc* CGUIMasterView::GetDocument()
   { return (CGUIMasterDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GUIMASTERVIEW_H__E463EDE7_DBFB_46D0_BD61_00749C03EF39__INCLUDED_)
