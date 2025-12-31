// Dialog_Object_List.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Object_List.h"
#include "EditorObjects.h"
#include "EditorCamera.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_List dialog


Dialog_Object_List::Dialog_Object_List(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_List::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_List)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_List::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_List)
	DDX_Control(pDX, IDC_LIST_OBJECTS, m_objectList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_List, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_List)
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)	
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT, OnButtonDeselect)
	ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL, OnButtonDeselectAll)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL_TEMPLATE, OnButtonSelectAllTemplate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_List message handlers

void Dialog_Object_List::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void Dialog_Object_List::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void Dialog_Object_List::OnButtonSelect() 
{
	int sel = m_objectList.GetSelectionMark();
	if ( sel == LB_ERR )
	{
		return;
	}
	CString rText;
	rText = m_objectList.GetItemText( sel, 0 );
	int temp = 0;
	stringtool::Get( rText.GetBuffer( 0 ), temp );	
	Goid object = gGoDb.FindGoidByScid( MakeScid( temp ) );
	if ( object == GOID_INVALID )
	{
		return;
	}

	gGizmoManager.SelectGizmo( 	MakeInt( object ) );

	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ));
		SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());						
		vector_3 worldPos = node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
		gEditorCamera.SetPosition( worldPos.x, worldPos.y, worldPos.z );			
	}
}


BOOL Dialog_Object_List::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_objectList.InsertColumn( 0, "Scid", LVCFMT_LEFT, 50 );
	m_objectList.InsertColumn( 1, "Template", LVCFMT_LEFT, 100 );
	m_objectList.InsertColumn( 2, "Position", LVCFMT_LEFT, 250 );

	RebuildObjectList();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Object_List::OnButtonDelete() 
{
	int sel = m_objectList.GetSelectionMark();
	if ( sel == LB_ERR )
	{
		return;
	}
	CString rText;
	rText = m_objectList.GetItemText( sel, 0 );
	int temp = 0;
	stringtool::Get( rText.GetBuffer( 0 ), temp );
	Scid scid = MakeScid( temp );
	gEditorObjects.DeleteObject( scid );
	RebuildObjectList();
}


void Dialog_Object_List::RebuildObjectList()
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );

	m_objectList.DeleteAllItems();
	int index = 0;
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		gpstring sScid;
		sScid.assignf( "0x%x", MakeInt( hObject->GetScid() ) );

		gpstring sPos;
		sPos.assignf( "%f, %f, %f, 0x%x",	hObject->GetPlacement()->GetPosition().pos.x,
											hObject->GetPlacement()->GetPosition().pos.y,
											hObject->GetPlacement()->GetPosition().pos.z,
											hObject->GetPlacement()->GetPosition().node.GetValue() );

		int item = m_objectList.InsertItem( index, sScid.c_str() );
		m_objectList.SetItem( item, 1, LVIF_TEXT, hObject->GetTemplateName(), 0, 0, 0, 0 );
		m_objectList.SetItem( item, 2, LVIF_TEXT, sPos.c_str(), 0, 0, 0, 0 );		
	}
}

void Dialog_Object_List::OnButtonDeselect() 
{
	int sel = m_objectList.GetSelectionMark();
	if ( sel == LB_ERR )
	{
		return;
	}
	CString rText;
	rText = m_objectList.GetItemText( sel, 0 );
	int temp = 0;
	stringtool::Get( rText.GetBuffer( 0 ), temp );	
	Goid object = gGoDb.FindGoidByScid( MakeScid( temp ) );
	gGizmoManager.DeselectGizmo( MakeInt( object ) );	
}

void Dialog_Object_List::OnButtonDeselectAll() 
{
	gGizmoManager.DeselectAll();	
}

void Dialog_Object_List::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Object_List::OnButtonSelectAllTemplate() 
{
	int sel = m_objectList.GetSelectionMark();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rText;
	rText = m_objectList.GetItemText( sel, 1 );

	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );
		
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( gpstring( hObject->GetTemplateName() ).same_no_case( rText.GetBuffer( 0 ) ) )
		{
			gGizmoManager.SelectGizmo( MakeInt( *i ) );		
		}
	}
}
