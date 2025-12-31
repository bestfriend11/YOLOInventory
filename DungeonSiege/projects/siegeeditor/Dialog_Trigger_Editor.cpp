// Dialog_Trigger_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"

#include "SiegeEditor.h"
#include "Dialog_Trigger_Editor.h"
#include "GizmoManager.h"
#include "Dialog_Trigger_Prop.h"
#include "EditorTriggers.h"
#include "Go.h"
#include "ImageListDefines.h"
#include "world.h"
#include "Trigger_Sys.h"
#include "Contentdb.h"
#include "GoCore.h"

using namespace trigger;

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Editor dialog


Dialog_Trigger_Editor::Dialog_Trigger_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Editor)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Editor)
	DDX_Control(pDX, IDC_TRIGGER_TREE, m_wndTriggers);
	//}}AFX_DATA_MAP
}


void Dialog_Trigger_Editor::SetupTriggerTree()
{
	m_wndTriggers.DeleteAllItems();
	std::vector< DWORD > gizmo_ids;
	gGizmoManager.GetSelectedGizmoIDs( gizmo_ids );
	std::vector< DWORD >::iterator i;
	for ( i = gizmo_ids.begin(); i != gizmo_ids.end(); ++i ) 
	{
		Gizmo * pGizmo = gGizmoManager.GetGizmo( *i );
		if ( pGizmo )
		{
			switch ( pGizmo->type )
			{
			case GIZMO_OBJECTGIZMO:
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid(*i) );
					if ( hObject.IsValid() )
					{
						gpstring sObject;
						sObject.assignf("Fuel Name: %s , Goid: %d, Scid: 0x%08X", hObject->GetTemplateName(), MakeInt(hObject->GetGoid()), MakeInt(hObject->GetScid()) );

						HTREEITEM hTreeParent = m_wndTriggers.InsertItem(	TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE,
																			sObject, IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, 0, 0, *i, TVI_ROOT, TVI_SORT );		

						trigger::Storage	*pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());

						if ( pTriggerStorage )
						{
							for ( unsigned int j = 0; j != pTriggerStorage->Size(); ++j ) 
							{
								gpstring sTrigger;
								sTrigger.assignf( "trigger_%d", j );
								m_wndTriggers.InsertItem( sTrigger, IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hTreeParent, TVI_SORT );										
							}
						}					
					}
				}
				break;
			}
		}
	}	
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Editor)
	ON_BN_CLICKED(IDC_NEW_TRIGGER, OnNewTrigger)
	ON_BN_CLICKED(IDC_EDIT_TRIGGER, OnEditTrigger)
	ON_BN_CLICKED(IDC_REMOVE_TRIGGER, OnRemoveTrigger)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Editor message handlers

void Dialog_Trigger_Editor::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void Dialog_Trigger_Editor::OnNewTrigger() 
{
	Goid object = GOID_INVALID;
	int index = 0;
	RetrieveSelectedData( object, index );
	if (( object == 0 ) || ( object == GOID_INVALID ))
	{
		return;
	}
	
	trigger::Trigger *pTrigger = new trigger::Trigger;

	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		hObject->GetCommon()->GetValidInstanceTriggers().Add_Trigger( pTrigger );
	}

	SetupTriggerTree();
}

void Dialog_Trigger_Editor::OnEditTrigger() 
{
	Goid object = GOID_INVALID;
	int index = 0;
	if ( RetrieveSelectedData( object, index ) )
	{
		gEditorTriggers.SetCurrentGoid( object );
		gEditorTriggers.SetCurrentTriggerIndex( index );
		Dialog_Trigger_Prop dlgProp;
		dlgProp.DoModal();		
	}
}

void Dialog_Trigger_Editor::OnRemoveTrigger() 
{
	Goid object = GOID_INVALID;
	int index = 0;
	if ( RetrieveSelectedData( object, index ) )
	{
		GoHandle hObject( object );
		if ( hObject.IsValid() )
		{
			trigger::Storage	*pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
			trigger::Trigger *pTrigger = 0;
			pTriggerStorage->Get( index, &pTrigger );				
			pTriggerStorage->Remove_Trigger( pTrigger );
			SetupTriggerTree();
		}
	}	
}


bool Dialog_Trigger_Editor::RetrieveSelectedData( Goid & object, int & index )
{
	HTREEITEM hSelected = m_wndTriggers.GetSelectedItem();
	HTREEITEM hParent	= m_wndTriggers.GetParentItem( hSelected );

	if ( hSelected == NULL ) {
		return false;
	}
	
	if (( hParent == TVI_ROOT ) || ( hParent == NULL )) 
	{
		object = MakeGoid(m_wndTriggers.GetItemData( hSelected ));	
		return false;
	}
	
	object			= MakeGoid(m_wndTriggers.GetItemData( hParent ));
	CString rText	= m_wndTriggers.GetItemText( hSelected );
	gpstring sText	= rText.GetBuffer( 0 );
	gpstring sIndex = sText.substr( strlen( "trigger_" ), sText.size() );
	index			= atoi( sIndex.c_str() );

	return true;
}

BOOL Dialog_Trigger_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetupTriggerTree();	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
