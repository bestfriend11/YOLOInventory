#if !defined(AFX_BOTTOMVIEW_H__FDA53339_FC0D_4CA0_B3F2_23B4CCAC281B__INCLUDED_)
#define AFX_BOTTOMVIEW_H__FDA53339_FC0D_4CA0_B3F2_23B4CCAC281B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BottomView.h : header file
//

class CSiegeEditorDoc;

#include "gpcore.h"
#include "FuBi.h"
#include "ReportSys.h"


/////////////////////////////////////////////////////////////////////////////
// BottomView view

class BottomView : public CView, public Singleton <BottomView>
{
protected:
	BottomView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(BottomView)

// Attributes
public:
	CSiegeEditorDoc* GetDocument();	
	CEdit &		GetConsoleEditCtrl() { return m_wndEdit; }

private:

	CEdit		m_wndEdit;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(BottomView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~BottomView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(BottomView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in BottomView.cpp
inline CSiegeEditorDoc* BottomView::GetDocument()
   { return (CSiegeEditorDoc*)m_pDocument; }
#endif


#define gBottomView BottomView::GetSingleton()


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BOTTOMVIEW_H__FDA53339_FC0D_4CA0_B3F2_23B4CCAC281B__INCLUDED_)
