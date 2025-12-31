// SecondaryFrm.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "SecondaryFrm.h"

#include "BottomView.h"
#include "SiegeEditorView.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSecondaryFrm

IMPLEMENT_DYNCREATE(CSecondaryFrm, CFrameWnd)

CSecondaryFrm::CSecondaryFrm()
{
}

CSecondaryFrm::~CSecondaryFrm()
{
}


BEGIN_MESSAGE_MAP(CSecondaryFrm, CFrameWnd)
	//{{AFX_MSG_MAP(CSecondaryFrm)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecondaryFrm message handlers

BOOL CSecondaryFrm::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	if (!m_wndSplitterConsole.CreateStatic(this, 2, 1)) {
		return FALSE;
	}

	if (!m_wndSplitterConsole.CreateView( 1, 0, RUNTIME_CLASS(BottomView), CSize(100, 100), pContext) ||
		!m_wndSplitterConsole.CreateView( 0, 0, RUNTIME_CLASS(CSiegeEditorView), CSize(100, 100), pContext))				
	{
		m_wndSplitterConsole.DestroyWindow();
		return FALSE;
	}
	
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}
