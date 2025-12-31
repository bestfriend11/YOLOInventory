// NodePropertySheet.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "NodePropertySheet.h"
#include "PropPage_Node_Fade_Settings.h"
#include "PropPage_Node_General.h"
#include "PropPage_Node_Set.h"
#include "PropPage_Node_Stitching.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int m_activePage = 0;

/////////////////////////////////////////////////////////////////////////////
// NodePropertySheet

IMPLEMENT_DYNAMIC(NodePropertySheet, CPropertySheet)

NodePropertySheet::NodePropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)	
{	
	SetDlgCtrlID( ID_PROPERTYSHEET_NODE_PROPERTIES );
}

NodePropertySheet::NodePropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	SetDlgCtrlID( ID_PROPERTYSHEET_NODE_PROPERTIES );
}

NodePropertySheet::~NodePropertySheet()
{	
}


BEGIN_MESSAGE_MAP(NodePropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(NodePropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// NodePropertySheet message handlers


BOOL NodePropertySheet::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if ( wParam == IDOK )
	{
		((PropPage_Node_General *)GetPage( 0 ))->SetNodeGeneric();
		((PropPage_Node_Fade_Settings *)GetPage( 1 ))->SetFadeSettings();
		((PropPage_Node_Set *)GetPage( 2 ))->SetNodeSet();
		((PropPage_Node_Stitching *)GetPage( 3 ))->SetNodeStitching();		
	}
	else if ( wParam == IDHELP )
	{
		HELPINFO lhelpInfo;
		CPropertySheet::OnHelpInfo(&lhelpInfo);	
		return TRUE;
	}
	
	return CPropertySheet::OnCommand(wParam, lParam);
}

BOOL NodePropertySheet::DestroyWindow() 
{
	m_activePage = GetTabControl()->GetCurSel();
	
	return CPropertySheet::DestroyWindow();
}

BOOL NodePropertySheet::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{	
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

BOOL NodePropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
	SetActivePage( m_activePage );

	
	return bResult;
}
