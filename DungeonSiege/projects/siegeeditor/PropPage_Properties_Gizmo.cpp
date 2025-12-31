// PropPage_Properties_Gizmo.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Gizmo.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Gizmo dialog

IMPLEMENT_DYNCREATE(PropPage_Properties_Gizmo, CResizablePage)

PropPage_Properties_Gizmo::PropPage_Properties_Gizmo()
	: CResizablePage(PropPage_Properties_Gizmo::IDD)
	, m_bApply( false )
{
	//{{AFX_DATA_INIT(PropPage_Properties_Gizmo)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void PropPage_Properties_Gizmo::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Gizmo)
	DDX_Control(pDX, IDC_CHECK_NODE_POSITION_OVERRIDE, m_nodeOverride);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Gizmo, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Gizmo)
	ON_BN_CLICKED(IDC_CHECK_NODE_POSITION_OVERRIDE, OnCheckNodePositionOverride)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Gizmo message handlers



void PropPage_Properties_Gizmo::Reset()
{
	if ( !GetSafeHwnd() )
	{
		return;
	}
}


void PropPage_Properties_Gizmo::Apply()
{
	m_bApply = false;

	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedGizmoIDs( objects );
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		if ( m_nodeOverride.GetCheck() )
		{
			gGizmoManager.GetGizmo( *i )->bNodeOverride = true;
		}
		else
		{
			gGizmoManager.GetGizmo( *i )->bNodeOverride = false;
		}
	}
}

void PropPage_Properties_Gizmo::Cancel()
{
}

void PropPage_Properties_Gizmo::OnCheckNodePositionOverride() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

BOOL PropPage_Properties_Gizmo::OnInitDialog() 
{
	CResizablePage::OnInitDialog();
	
	if ( gGizmoManager.GetNumGizmosSelected() > 0 )
	{
		if ( gGizmoManager.GetGizmo( gGizmoManager.GetSelectedGizmoID() )->bNodeOverride )
		{
			m_nodeOverride.SetCheck( true );
		}		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BOOL PropPage_Properties_Gizmo::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR * pNm = (NMHDR *)lParam;
	if ( pNm->code == 0xffffff37 )
	{
		if ( gObjectProperties.Evaluate() )
		{
			int result = MessageBoxEx( this->GetSafeHwnd(), "You have changed the properties on this page.  Would you like to apply the changes?", "Apply Changes?", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
			if ( result == IDYES ) 
			{
				gObjectProperties.Apply();
			}	
		}
	}
	
	return CResizablePage::OnNotify(wParam, lParam, pResult);
}
