// GUIMasterView.cpp : implementation of the CGUIMasterView class
//

#include "stdafx.h"
#include "GUIMaster.h"

#include "GUIMasterDoc.h"
#include "GUIMasterView.h"

#include "GUIWorkspace.h"
#include "GUIMasterTypes.h"

#include "GUIMasterShell.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterView

IMPLEMENT_DYNCREATE(CGUIMasterView, CView)

BEGIN_MESSAGE_MAP(CGUIMasterView, CView)
	//{{AFX_MSG_MAP(CGUIMasterView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterView construction/destruction

CGUIMasterView::CGUIMasterView()
{
	// TODO: add construction code here

}

CGUIMasterView::~CGUIMasterView()
{
}

BOOL CGUIMasterView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterView drawing

void CGUIMasterView::OnDraw(CDC* pDC)
{
	CGUIMasterDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterView diagnostics

#ifdef _DEBUG
void CGUIMasterView::AssertValid() const
{
	CView::AssertValid();
}

void CGUIMasterView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CGUIMasterDoc* CGUIMasterView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGUIMasterDoc)));
	return (CGUIMasterDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CGUIMasterView message handlers

int CGUIMasterView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

void CGUIMasterView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	RECT rect;
	GetClientRect( &rect );
	BOOL bSuccess = m_guiWorkspace.CreateEx( WS_EX_CLIENTEDGE, NULL, "Workspace", 
											 WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CHILD | WS_VISIBLE,
											 rect.left+((rect.right-rect.left)/2)-(INITIAL_WIDTH/2), rect.top+((rect.bottom-rect.top)/2)-(INITIAL_HEIGHT/2),
											 INITIAL_WIDTH+4, INITIAL_HEIGHT+4, this->GetSafeHwnd(), NULL, NULL );

	gGUIMaster.SetRenderHWnd( m_guiWorkspace.GetSafeHwnd() );		
	gGUIMaster.Init();
	
}

void CGUIMasterView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if ( m_guiWorkspace.GetSafeHwnd() )
	{
		CRect rect;
		GetClientRect( &rect );

		RECT currRect;
		RECT test;
		m_guiWorkspace.GetClientRect( &currRect );
		m_guiWorkspace.GetWindowRect( &test );
		m_guiWorkspace.SetWindowPos( &this->wndTop, rect.left+((rect.right-rect.left)/2)-(currRect.right/2), rect.top+((rect.bottom-rect.top)/2)-(currRect.bottom/2),
									currRect.right-currRect.left, currRect.bottom-currRect.top, SWP_NOSIZE );	
		m_guiWorkspace.GetWindowRect( &test );
	}
}

void CGUIMasterView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CRect guiRect;
	m_guiWorkspace.GetWindowRect( guiRect );
	CPoint pt = point;
	ClientToScreen( &pt );

	gGUIMaster.SetLButtonDown( true );
	if ( (( pt.x >= guiRect.left ) && ( pt.x <= guiRect.right )) &&
		 (( pt.y <= guiRect.top ) || ( pt.y >= guiRect.bottom )) )
	{
		gGUIMaster.CreateReferenceLine( X_AXIS );	
	}
	else
	{
		gGUIMaster.CreateReferenceLine( Y_AXIS );
	}
	
	
	CView::OnLButtonDown(nFlags, point);
}
