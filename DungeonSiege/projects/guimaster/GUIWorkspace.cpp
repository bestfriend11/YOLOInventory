// GUIWorkspace.cpp : implementation file
//

#include "stdafx.h"
#include "GUIMaster.h"
#include "GUIWorkspace.h"
#include "GUIMasterShell.h"
#include "Menu.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGUIWorkspace

CGUIWorkspace::CGUIWorkspace()
{
}

CGUIWorkspace::~CGUIWorkspace()
{
}


BEGIN_MESSAGE_MAP(CGUIWorkspace, CWnd)
	//{{AFX_MSG_MAP(CGUIWorkspace)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_CHAR()
	ON_WM_MENUSELECT()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_WINDOW_PROPERTIES, MenuFunctions::Properties)
	ON_COMMAND(ID_WINDOW_BRINGTOFRONT, MenuFunctions::BringToFront)
	ON_COMMAND(ID_WINDOW_MESSAGES, MenuFunctions::Messages)	
	ON_COMMAND(ID_WINDOW_COMMON_SETTINGS, MenuFunctions::CommonSettings)
	ON_COMMAND(ID_WINDOW_GROUP, MenuFunctions::GroupSettings)
	ON_COMMAND(ID_WINDOW_TEXT,MenuFunctions::TextSettings)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGUIWorkspace message handlers

void CGUIWorkspace::OnLButtonDown(UINT nFlags, CPoint point) 
{
	gGUIMaster.OnLButtonDown();
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CGUIWorkspace::OnLButtonUp(UINT nFlags, CPoint point) 
{
	gGUIMaster.OnLButtonUp();
	
	CWnd::OnLButtonUp(nFlags, point);
}

void CGUIWorkspace::OnMouseMove(UINT nFlags, CPoint point) 
{
	gGUIMaster.OnMouseMove();
	
	CWnd::OnMouseMove(nFlags, point);
}

int CGUIWorkspace::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_wndProperties.CreatePopupMenu();
	m_wndProperties.AppendMenu( MF_ENABLED, ID_WINDOW_PROPERTIES, "Properties" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_WINDOW_MESSAGES, "Messages" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_WINDOW_COMMON_SETTINGS, "Common Control Settings" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_WINDOW_GROUP, "Group Settings" );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_WINDOW_TEXT, "Text Settings" );

	m_wndProperties.AppendMenu( MF_SEPARATOR );
	m_wndProperties.AppendMenu( MF_ENABLED, ID_WINDOW_BRINGTOFRONT, "Bring To Front" );
	
	return 0;
}

void CGUIWorkspace::OnRButtonDown(UINT nFlags, CPoint point) 
{
	if ( gGUIMaster.HitDetect( point.x, point.y, false ) )
	{
		POINT pt = point;
		ClientToScreen( &pt );
		m_wndProperties.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
	}
	
	CWnd::OnRButtonDown(nFlags, point);
}

void CGUIWorkspace::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CWnd::OnRButtonUp(nFlags, point);
}

void CGUIWorkspace::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	
	CWnd::OnChar(nChar, nRepCnt, nFlags);
}

void CGUIWorkspace::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu) 
{
	CWnd::OnMenuSelect(nItemID, nFlags, hSysMenu);	
}
