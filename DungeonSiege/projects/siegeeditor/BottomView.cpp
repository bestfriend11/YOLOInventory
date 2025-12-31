// BottomView.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "SiegeEditorDoc.h"
#include "BottomView.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// BottomView

IMPLEMENT_DYNCREATE(BottomView, CView)

BottomView::BottomView()
{
}

BottomView::~BottomView()
{
}


BEGIN_MESSAGE_MAP(BottomView, CView)
	//{{AFX_MSG_MAP(BottomView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// BottomView drawing

void BottomView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// BottomView diagnostics

#ifdef _DEBUG
void BottomView::AssertValid() const
{
	CView::AssertValid();
}

void BottomView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSiegeEditorDoc* BottomView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSiegeEditorDoc)));
	return (CSiegeEditorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// BottomView message handlers

int BottomView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	RECT rect;
	GetClientRect( &rect );
	m_wndEdit.Create(	ES_READONLY | ES_AUTOVSCROLL | ES_MULTILINE | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
						rect, this, IDC_CONSOLE_LISTVIEW );	
	CFont font;
	font.CreateStockObject( DEFAULT_GUI_FONT );
	m_wndEdit.SetFont( &font );	
	
	return 0;
}


void BottomView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();	
}

void BottomView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	RECT rect;
	GetClientRect( &rect );
	m_wndEdit.MoveWindow( rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top );	
}
