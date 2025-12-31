// ObjectPropertySheet.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "ObjectPropertySheet.h"
#include "PropPage_Lighting.h"
#include "GizmoManager.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectPropertySheet

IMPLEMENT_DYNAMIC(CObjectPropertySheet, CPropertySheet)

CObjectPropertySheet::CObjectPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CObjectPropertySheet::CObjectPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CObjectPropertySheet::~CObjectPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CObjectPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CObjectPropertySheet)
	ON_WM_CREATE()
	ON_BN_CLICKED(ID_OBJECT_OK_BUTTON, OnObjectOkay)
	ON_BN_CLICKED(ID_OBJECT_APPLY_BUTTON, OnObjectApply)
	ON_BN_CLICKED(ID_OBJECT_HELP_BUTTON, OnObjectHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectPropertySheet message handlers

int CObjectPropertySheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectWnd;
	GetWindowRect( rectWnd );

	CRect rectOk;
	rectOk.top		= rectWnd.bottom - 30;
	rectOk.bottom	= rectWnd.bottom - 5;
	rectOk.left		= rectWnd.right - 105;
	rectOk.right	= rectWnd.right - 200;
	m_wndOk.Create( "OK", WS_VISIBLE | WS_CHILD, rectOk, this, ID_OBJECT_OK_BUTTON );

	CRect rectApply;
	rectApply.top		= rectWnd.bottom - 30;
	rectApply.bottom	= rectWnd.bottom - 5;
	rectApply.left		= rectWnd.right - 5;
	rectApply.right		= rectWnd.right - 100;
	m_wndApply.Create( "Apply", WS_VISIBLE | WS_CHILD, rectApply, this, ID_OBJECT_APPLY_BUTTON );
	
	m_wndHelp.Create( "Help", WS_VISIBLE | WS_CHILD, rectApply, this, ID_OBJECT_HELP_BUTTON );

	CFont font;
	font.CreateStockObject( DEFAULT_GUI_FONT );
	m_wndOk.SetFont( &font );
	m_wndApply.SetFont( &font );
	m_wndHelp.SetFont( &font );

	return 0;
}

BOOL CObjectPropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	CRect rectWnd;
	GetWindowRect( rectWnd );	
	MoveWindow( rectWnd.left, rectWnd.top, rectWnd.Width(), rectWnd.Height() + 35 );
	
	CRect rectOk;
	CRect rectApply;
	CRect rectHelp;
	GetClientRect( rectWnd );
	rectOk.top		= rectWnd.bottom - 30;
	rectOk.bottom	= rectWnd.bottom - 5;
	rectOk.right	= rectWnd.right - 85;
	rectOk.left		= rectWnd.right - 160;

	rectApply.top		= rectWnd.bottom - 30;
	rectApply.bottom	= rectWnd.bottom - 5;
	rectApply.right		= rectWnd.right - 5;
	rectApply.left		= rectWnd.right - 80;

	rectHelp.top		= rectWnd.bottom - 30;
	rectHelp.bottom		= rectWnd.bottom - 5;
	rectHelp.left		= rectWnd.left + 5;
	rectHelp.right		= rectWnd.left + 85;
	
	m_wndOk.MoveWindow( rectOk, TRUE );	
	m_wndApply.MoveWindow( rectApply, TRUE );
	m_wndHelp.MoveWindow( rectHelp, TRUE );
	
	return bResult;
}


void CObjectPropertySheet::OnObjectOkay()
{
	PropPage_Lighting * pLighting = (PropPage_Lighting *)GetPage( 1 );
	pLighting->Apply();

	CloseWindow();
	DestroyWindow();
}


void CObjectPropertySheet::OnObjectApply()
{
	PropPage_Lighting * pLighting = (PropPage_Lighting *)GetPage( 1 );
	pLighting->Apply();
}


void CObjectPropertySheet::OnObjectHelp()
{
	HELPINFO lhelpInfo;
	CObjectPropertySheet::OnHelpInfo(&lhelpInfo);		
}

LRESULT CObjectPropertySheet::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	return CPropertySheet::WindowProc(message, wParam, lParam);
}


