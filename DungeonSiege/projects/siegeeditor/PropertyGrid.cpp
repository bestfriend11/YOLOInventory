// PropertyGrid.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "PropertyGrid.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropertyGrid

CPropertyGrid::CPropertyGrid()
{
}

CPropertyGrid::~CPropertyGrid()
{
}


BEGIN_MESSAGE_MAP(CPropertyGrid, CGridCtrl)
	//{{AFX_MSG_MAP(CPropertyGrid)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertyGrid message handlers

int CPropertyGrid::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CGridCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
			
	return 0;
}
