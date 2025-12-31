// SiegeEditorView.h : interface of the CSiegeEditorView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SIEGEEDITORVIEW_H__12FD9A61_B13E_4D3A_997C_E1D8D33D8128__INCLUDED_)
#define AFX_SIEGEEDITORVIEW_H__12FD9A61_B13E_4D3A_997C_E1D8D33D8128__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "gpcore.h"
#include "vector_3.h"


class CSiegeEditorView : public CView, public Singleton <CSiegeEditorView>
{
protected: // create from serialization only
	CSiegeEditorView();
	DECLARE_DYNCREATE(CSiegeEditorView)

// Attributes
public:
	CSiegeEditorDoc* GetDocument();

	void CreatePopupMenus();

	CMenu				m_wndProperties;
	CMenu				m_wndNodeProperties;	
	CMenu				m_wndStartingPosProp;
	CMenu				m_wndHotpointProp;
	CMenu				m_wndDecalProp;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSiegeEditorView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSiegeEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	bool IsMouseOnTerrain();
	vector_3	GetHitFaceNormal() { return m_faceNormal; }
	void		SetHitFaceNormal( vector_3 normal ) { m_faceNormal = normal; }

	bool IsShiftDown() { return m_bShiftDown; }

private:

	bool m_bValidPlace;
	bool m_bShiftDown;
	bool m_bMoving;
	bool m_bCameraScrolling;
	vector_3 m_faceNormal;
	POINT m_lastPoint;
	bool m_bNodeJustSnapped;

protected:
	
// Generated message map functions
protected:
	//{{AFX_MSG(CSiegeEditorView)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	afx_msg void OnMenuSelect( UINT nItemID, UINT nFlags, HMENU hSysMenu );
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SiegeEditorView.cpp
inline CSiegeEditorDoc* CSiegeEditorView::GetDocument()
   { return (CSiegeEditorDoc*)m_pDocument; }
#endif


#define gSiegeEditorView CSiegeEditorView::GetSingleton()

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIEGEEDITORVIEW_H__12FD9A61_B13E_4D3A_997C_E1D8D33D8128__INCLUDED_)
