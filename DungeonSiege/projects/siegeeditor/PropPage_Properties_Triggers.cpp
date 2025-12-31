// PropPage_Properties_Triggers.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Triggers.h"
#include "Dialog_Trigger_Add_Condition.h"
#include "Dialog_Trigger_Add_Action.h"
#include "Trigger_Sys.h"
#include "Trigger_Conditions.h"
#include "EditorTriggers.h"
#include "gridctrl\\GridBtnCellCombo.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Triggers property page

IMPLEMENT_DYNCREATE(PropPage_Properties_Triggers, CResizablePage)

PropPage_Properties_Triggers::PropPage_Properties_Triggers() : CResizablePage(PropPage_Properties_Triggers::IDD)
{
	m_bApply = false;

	//{{AFX_DATA_INIT(PropPage_Properties_Triggers)
	//}}AFX_DATA_INIT
}

PropPage_Properties_Triggers::~PropPage_Properties_Triggers()
{
}

void PropPage_Properties_Triggers::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Triggers)
	DDX_Control(pDX, IDC_CHECK_CAN_SELF_DESTRUCT, m_selfdestruct);
	DDX_Control(pDX, IDC_CHECK_SINGLEPLAYER, m_singleplayer);
	DDX_Control(pDX, IDC_CHECK_MULTIPLAYER, m_multiplayer);
	DDX_Control(pDX, IDC_CHECK_ACTIVE, m_active);
	DDX_Control(pDX, IDC_EDIT_OCCUPANTS, m_occupants);
	DDX_Control(pDX, IDC_EDIT_DELAY, m_delay);
	DDX_Control(pDX, IDC_EDIT_RESET_DELAY, m_resetdelay);
	DDX_Control(pDX, IDC_CHECK_FLIPFLOP, m_flipflop);
	DDX_Control(pDX, IDC_CHECK_ONE_SHOT, m_oneshot);
	DDX_Control(pDX, IDC_TREE_TRIGGERS, m_triggers);
	DDX_Control(pDX, IDC_STATIC_REPLACE_CONDITIONS, m_replace_conditions);
	DDX_Control(pDX, IDC_STATIC_REPLACE_ACTIONS, m_replace_actions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Triggers, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Triggers)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_TRIGGERS, OnSelchangedTreeTriggers)
	ON_EN_CHANGE(IDC_EDIT_RESET_DELAY, OnChangeEditResetDelay)
	ON_BN_CLICKED(IDC_BUTTON_ADD_CONDITION, OnButtonAddCondition)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_CONDITION, OnButtonRemoveCondition)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ACTION, OnButtonAddAction)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ACTIONS, OnButtonRemoveActions)
	ON_BN_CLICKED(IDC_BUTTON_NEW_TRIGGER, OnButtonNewTrigger)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_TRIGGER, OnButtonRemoveTrigger)
	ON_BN_CLICKED(IDC_CHECK_ONE_SHOT, OnCheckOneShot)
	ON_BN_CLICKED(IDC_CHECK_FLIPFLOP, OnCheckFlipflop)
	ON_EN_CHANGE(IDC_EDIT_DELAY, OnChangeEditDelay)
	ON_EN_CHANGE(IDC_EDIT_OCCUPANTS, OnChangeEditOccupants)
	ON_BN_CLICKED(IDC_CHECK_ACTIVE, OnCheckActive)
	ON_BN_CLICKED(IDC_CHECK_SINGLEPLAYER, OnCheckSingleplayer)
	ON_BN_CLICKED(IDC_CHECK_MULTIPLAYER, OnCheckMultiplayer)
	ON_BN_CLICKED(IDC_CHECK_CAN_SELF_DESTRUCT, OnCheckCanSelfDestruct)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Triggers message handlers

BOOL PropPage_Properties_Triggers::OnInitDialog() 
{
	CResizablePage::OnInitDialog();
	
	RECT dlgRect;
	this->GetWindowRect( &dlgRect );
	
	RECT conditionRect;
	RECT actionRect;
	RECT replaceCRect;
	RECT replaceARect;

	m_replace_conditions.GetWindowRect( &replaceCRect );
	m_replace_actions.GetWindowRect( &replaceARect );

	conditionRect.left		= replaceCRect.left - dlgRect.left;
	conditionRect.right		= replaceCRect.right - dlgRect.left;
	conditionRect.top		= replaceCRect.top - dlgRect.top;
	conditionRect.bottom	= replaceCRect.bottom - dlgRect.top;
	
	actionRect.left			= replaceARect.left - dlgRect.left;
	actionRect.right		= replaceARect.right - dlgRect.left;
	actionRect.top			= replaceARect.top - dlgRect.top;
	actionRect.bottom		= replaceARect.bottom - dlgRect.top;
	
	m_grid_conditions.Create( conditionRect, this, IDC_GRID_CONDITIONS, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	
	m_grid_actions.Create( actionRect, this, IDC_GRID_ACTIONS, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	
	
	AddAnchor( IDC_STATIC_TRIGGERS, TOP_LEFT, BOTTOM_LEFT );
	AddAnchor( IDC_TREE_TRIGGERS, TOP_LEFT, BOTTOM_LEFT );

	AddAnchor( IDC_STATIC_PROPERTIES, TOP_LEFT );
	
	AddAnchor( IDC_EDIT_RESET_DELAY, TOP_LEFT );
	AddAnchor( IDC_STATIC_RESET_DELAY, TOP_LEFT );

	AddAnchor( IDC_STATIC_CONDITIONS, TOP_LEFT, MIDDLE_RIGHT );	
	AddAnchor( IDC_GRID_CONDITIONS, TOP_LEFT, MIDDLE_RIGHT );

	AddAnchor( IDC_STATIC_ACTIONS, MIDDLE_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_GRID_ACTIONS, MIDDLE_LEFT, BOTTOM_RIGHT );
	
	AddAnchor( IDC_BUTTON_ADD_CONDITION, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_REMOVE_CONDITION, TOP_RIGHT );
	
	AddAnchor( IDC_BUTTON_ADD_ACTION, BOTTOM_RIGHT );
	AddAnchor( IDC_BUTTON_REMOVE_ACTIONS, BOTTOM_RIGHT );	

	AddAnchor( IDC_STATIC_REPLACE_CONDITIONS, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_STATIC_REPLACE_ACTIONS, TOP_LEFT, BOTTOM_RIGHT );	

	AddAnchor( IDC_BUTTON_NEW_TRIGGER, BOTTOM_LEFT );
	AddAnchor( IDC_BUTTON_REMOVE_TRIGGER, BOTTOM_LEFT );

	m_replace_actions.ShowWindow( SW_HIDE );
	m_replace_conditions.ShowWindow( SW_HIDE );

	m_grid_conditions.SetEditable();
	m_grid_conditions.SetRowResize();
	m_grid_conditions.SetColumnResize();
	m_grid_conditions.SetFixedColumnSelection(TRUE);
    m_grid_conditions.SetFixedRowSelection(TRUE);
	m_grid_conditions.SetFixedRowCount();

	m_grid_actions.SetEditable();
	m_grid_actions.SetRowResize();
	m_grid_actions.SetColumnResize();
	m_grid_actions.SetFixedColumnSelection(TRUE);
    m_grid_actions.SetFixedRowSelection(TRUE);
	m_grid_actions.SetFixedRowCount();

	m_BtnDataBaseAction.SetGrid( &m_grid_actions );
	m_BtnDataBaseCondition.SetGrid( &m_grid_conditions );
	
	Reset();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void PropPage_Properties_Triggers::OnSelchangedTreeTriggers(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	Goid object = GOID_INVALID;
	int index = 0;
	if ( RetrieveSelectedData( object, index ) )
	{
		gEditorTriggers.SetCurrentGoid( object );
		gEditorTriggers.SetCurrentTriggerIndex( index );
		
		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
		trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
		trigger::Trigger *pTrigger = 0;
		pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		m_flipflop.SetCheck( pTrigger->IsFlipFlop() );

		m_oneshot.SetCheck( pTrigger->IsOneShot() );

		m_active.SetCheck( pTrigger->GetRequestedActive() );	
		m_singleplayer.SetCheck( pTrigger->GetIsSingle() );
		m_multiplayer.SetCheck( pTrigger->GetIsMulti() );
		m_selfdestruct.SetCheck( pTrigger->CanSelfDestruct() );

		gpstring sResetDelay;
		sResetDelay.assignf( "%f", pTrigger->GetResetDuration() );
		m_resetdelay.SetWindowText( sResetDelay.c_str() );

		gpstring sDelay;
		sDelay.assignf( "%f", pTrigger->GetDelay() );
		m_delay.SetWindowText( sDelay.c_str() );

		m_occupants.SetWindowText( pTrigger->GetGroupName().c_str() );

		m_oneshot.EnableWindow( TRUE );
		m_flipflop.EnableWindow( TRUE );
		m_active.EnableWindow( TRUE );
		m_resetdelay.EnableWindow( TRUE );
		m_delay.EnableWindow( TRUE );
		m_occupants.EnableWindow( TRUE );
		m_singleplayer.EnableWindow( TRUE );
		m_multiplayer.EnableWindow( TRUE );
		m_selfdestruct.EnableWindow( TRUE );

		InitializeTriggerGrids();
	}
	
	*pResult = 0;
}

void PropPage_Properties_Triggers::OnChangeEditResetDelay() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

	CString rDelay;
	m_resetdelay.GetWindowText( rDelay );
	gpstring sDelay = rDelay;
	double duration = 0;
	stringtool::Get( sDelay, duration );
	pTrigger->SetResetDuration( duration );		
	
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Triggers::OnButtonAddCondition() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() )
	{
		gEditorTriggers.SetCurrentConditionIndex( INVALID_INDEX );
		Dialog_Trigger_Add_Condition dlgCondition( this );		
		dlgCondition.DoModal();
		InitializeTriggerGrids();	
		m_bApply = true;
		gObjectProperties.Evaluate();
	}	
}

void PropPage_Properties_Triggers::OnButtonRemoveCondition() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() )
	{		
		CCellRange range = m_grid_conditions.GetSelectedCellRange();

		if (( range.GetMaxRow() == -1 ) || ( range.GetMaxCol() == -1 ))
		{
			return;
		}

		if ( range.Count() == 1 )
		{
			DWORD data = m_grid_conditions.GetItemData( range.GetMinRow(), range.GetMinCol() );
			trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
			trigger::Trigger *pTrigger = 0;
			pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
			pTrigger->RemoveCondition( (WORD)data );
		}
		else
		{
			for ( int i = range.GetMinCol(); i != range.GetMaxCol(); ++i )
			{
				for ( int j = range.GetMinRow(); j != range.GetMaxRow(); ++j )
				{			
					DWORD data = m_grid_conditions.GetItemData( j, i );
					trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
					trigger::Trigger *pTrigger = 0;
					pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
				
					pTrigger->RemoveCondition( (WORD)data );
				}
			}	
		}
		
		InitializeTriggerGrids();	
		m_bApply = true;
		gObjectProperties.Evaluate();
	}	
}

void PropPage_Properties_Triggers::OnButtonAddAction() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() )
	{
		gEditorTriggers.SetCurrentActionIndex( INVALID_INDEX );
		Dialog_Trigger_Add_Action dlgAction( this );
		dlgAction.DoModal();
		InitializeTriggerGrids();
		m_bApply = true;
		gObjectProperties.Evaluate();
	}	
}

void PropPage_Properties_Triggers::OnButtonRemoveActions() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() )
	{
		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
		CCellRange range = m_grid_actions.GetSelectedCellRange();

		if (( range.GetMaxRow() == -1 ) || ( range.GetMaxCol() == -1 ))
		{
			return;
		}

		if ( range.Count() == 1 )
		{
			DWORD data = m_grid_actions.GetItemData( range.GetMinRow(), range.GetMinCol() );
			trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
			trigger::Trigger *pTrigger = 0;
			pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
			pTrigger->RemoveAction( (WORD)data );
			pTrigger->UpdateTriggerFlags();
		}
		else
		{
			for ( int i = range.GetMinCol(); i != range.GetMaxCol(); ++i )
			{
				for ( int j = range.GetMinRow(); j != range.GetMaxRow(); ++j )
				{
					DWORD data = m_grid_actions.GetItemData( j, i );
					trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
					trigger::Trigger *pTrigger = 0;
					pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
				
					pTrigger->RemoveAction( (WORD)data );
					pTrigger->UpdateTriggerFlags();
				}
			}				
		}
		InitializeTriggerGrids();
		m_bApply = true;
		gObjectProperties.Evaluate();
	}	
}

void PropPage_Properties_Triggers::OnButtonNewTrigger() 
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
		pTrigger->SetOwner(hObject.Get());
	}

	SetupTriggerTree();			

	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Triggers::OnButtonRemoveTrigger() 
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
			m_bApply = true;
			gObjectProperties.Evaluate();

			m_oneshot.EnableWindow( FALSE );
			m_flipflop.EnableWindow( FALSE );
			m_active.EnableWindow( FALSE );
			m_resetdelay.EnableWindow( FALSE );
			m_delay.EnableWindow( FALSE );
			m_occupants.EnableWindow( FALSE );			
			m_singleplayer.EnableWindow( FALSE );
			m_multiplayer.EnableWindow( FALSE );
			m_selfdestruct.EnableWindow( FALSE );

			Reset();
		}
	}	
}

void PropPage_Properties_Triggers::OnCheckOneShot() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
	bool oneshot = m_oneshot.GetCheck() ? true : false;
	if (pTrigger->IsOneShot() != oneshot)
	{
		if ( oneshot && !pTrigger->GetConditions().empty() )
		{
			// Chad we probably want to put up a warning here before we munge the 
			// conditions. We have to ensure that there are no illegal boundary checks --biddle
			pTrigger->ConvertConditionsToOneShot();
		}

		pTrigger->SetIsOneShot( oneshot );		
	}
	
	m_bApply = true;
	gObjectProperties.Evaluate();

}

void PropPage_Properties_Triggers::OnCheckFlipflop() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
	pTrigger->SetIsFlipFlop( m_flipflop.GetCheck() ? true : false );		
	m_bApply = true;
	gObjectProperties.Evaluate();
}


void PropPage_Properties_Triggers::SetupTriggerTree()
{
	m_triggers.DeleteAllItems();
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
			case GIZMO_TUNINGPOINT:
			case GIZMO_OBJECTGIZMO:
			case GIZMO_OBJECT:
				{
					GoHandle hObject( MakeGoid(*i) );
					if ( hObject.IsValid() )
					{
						gpstring sObject;
						sObject.assignf("%s , Goid: %d, Scid: 0x%08X", hObject->GetTemplateName(), MakeInt(hObject->GetGoid()), MakeInt(hObject->GetScid()) );

						HTREEITEM hTreeParent = m_triggers.InsertItem(	TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE,
																			sObject, IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, 0, 0, *i, TVI_ROOT, TVI_SORT );		

						trigger::Storage	*pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());

						if ( pTriggerStorage )
						{
							for ( unsigned int j = 0; j != pTriggerStorage->Size(); ++j ) 
							{
								gpstring sTrigger;
								sTrigger.assignf( "trigger_%d", j );
								m_triggers.InsertItem( sTrigger, IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hTreeParent, TVI_SORT );										
							}
						}					
					}
				}
				break;
			}
		}
	}	
}


bool PropPage_Properties_Triggers::RetrieveSelectedData( Goid & object, int & index )
{	
	HTREEITEM hSelected = m_triggers.GetSelectedItem();
	HTREEITEM hParent	= m_triggers.GetParentItem( hSelected );

	if ( hSelected == NULL ) {
		return false;
	}
	
	if (( hParent == TVI_ROOT ) || ( hParent == NULL )) 
	{
		object = MakeGoid(m_triggers.GetItemData( hSelected ));	
		return false;
	}
	
	object			= MakeGoid(m_triggers.GetItemData( hParent ));
	CString rText	= m_triggers.GetItemText( hSelected );
	gpstring sText	= rText.GetBuffer( 0 );
	gpstring sIndex = sText.substr( strlen( "trigger_" ), sText.size() );
	index			= atoi( sIndex.c_str() );

	return true;
}


void PropPage_Properties_Triggers::InitializeTriggerGrids()
{
	m_grid_conditions.DeleteAllItems();
	m_grid_actions.DeleteAllItems();

	m_grid_conditions.SetEditable( TRUE );
	m_grid_conditions.SetRowResize( TRUE );
	m_grid_conditions.SetColumnResize( TRUE );
	m_grid_conditions.SetFixedColumnSelection(TRUE);
    m_grid_conditions.SetFixedRowSelection(TRUE);
	m_grid_conditions.SetFixedRowCount( 1 );	

	m_grid_actions.SetEditable( TRUE );
	m_grid_actions.SetRowResize( TRUE );
	m_grid_actions.SetColumnResize( TRUE );
	m_grid_actions.SetFixedColumnSelection(TRUE);
    m_grid_actions.SetFixedRowSelection(TRUE);
	m_grid_actions.SetFixedRowCount( 1 );	

	m_grid_conditions.InsertColumn( "Name" );
	m_grid_conditions.InsertColumn( "Parameter" );
	m_grid_conditions.InsertColumn( "Value" );
	m_grid_conditions.InsertColumn( "Group" );
	m_grid_conditions.InsertColumn( "Doc" );
	m_grid_conditions.InsertColumn( "ID" );
		
	m_grid_actions.InsertColumn( "Name" );
	m_grid_actions.InsertColumn( "Parameter" );
	m_grid_actions.InsertColumn( "Value" );
	m_grid_actions.InsertColumn( "Condition" );	
	m_grid_actions.InsertColumn( "Delay" );
	m_grid_actions.InsertColumn( "Group" );
	m_grid_actions.InsertColumn( "Doc" );
	
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() == false )
	{
		return;
	}
	
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

	// Setup Conditions Grid
	{
		for (trigger::ConditionConstIter cit = pTrigger->GetConditions().begin(); cit != pTrigger->GetConditions().end(); ++cit)
		{
			trigger::Parameter	pFormat;
			trigger::Params		pParams;
			gpstring			sCondition;

			pTrigger->GetConditionInfo( (*cit).first , sCondition, pFormat, pParams );
			gpassert(pParams.ParamID != 0);
		
			int newRow = m_grid_conditions.InsertRow( sCondition.c_str() );			
			m_grid_conditions.SetItemState( newRow, 0, GVIS_READONLY );
			m_grid_conditions.SetItemData( newRow, 0, pParams.ParamID );

			gpstring sConditionId;
			sConditionId.assignf( "0x%04x", (*cit).second->GetID() );
			m_grid_conditions.SetItemText( newRow, 5, sConditionId );
			m_grid_conditions.SetItemData( newRow, 5, pParams.ParamID );
			
			// Get the parameters and their values
			int double_index	= 0;
			int int_index		= 0;
			int string_index	= 0;
			for ( unsigned int j = 0; j != pFormat.Size(); ++j ) 
			{
				trigger::Parameter::format *pParamFormat = 0;
				pFormat.Get( j, &pParamFormat );
				int dataRow = m_grid_conditions.InsertRow( "" );
				m_grid_conditions.SetItemText( dataRow, 1, pParamFormat->sName.c_str() );	
				m_grid_conditions.SetItemData( dataRow, 1, pParams.ParamID );
				m_grid_conditions.SetItemState( dataRow, 1, GVIS_READONLY );				
					
				if ( pParamFormat->sValues.size() == 0 )
				{
					gpstring sValue;
					if ( pParamFormat->isFloat ) {
						sValue.assignf( "%f", pParams.floats[double_index++] );				
					}
					else if ( pParamFormat->isInt || pParamFormat->isGoid ) 					
					{
						if ( pParamFormat->isBool )
						{
							if ( pParams.ints[int_index] == 1 )
							{
								sValue.assign( "true" );
							}
							else
							{
								sValue.assign( "false" );
							}							

							int_index++;
						}
						else if ( gPreferences.GetDisplayHexadecimal() || pParamFormat->bSaveAsHex )
						{
							sValue.assignf( "0x%08X", pParams.ints[int_index++] );
						}
						else
						{
							sValue.assignf( "%d", pParams.ints[int_index++] );				
						}
					}
					else if ( pParamFormat->isString ) {
						sValue.assignf( "%s", pParams.strings[string_index++].c_str() );				
					}

					m_grid_conditions.SetItemText( dataRow, 2, sValue.c_str() );				
					m_grid_conditions.SetItemData( dataRow, 2, pParams.ParamID );
				}
				else
				{
					m_grid_conditions.SetCellType( dataRow, 2, RUNTIME_CLASS(CGridBtnCellCombo) );					
					CGridBtnCellCombo* pGridBtnCell = (CGridBtnCellCombo*)m_grid_conditions.GetCell( dataRow, 2 );
					pGridBtnCell->SetBtnDataBase( &m_BtnDataBaseCondition );												
					pGridBtnCell->SetComboStyle( CBS_DROPDOWN );

					CStringArray sa;					
					StringVec::iterator iValues;
					for ( iValues = pParamFormat->sValues.begin(); iValues != pParamFormat->sValues.end(); ++iValues )
					{
						sa.Add( (*iValues).c_str() );
					}

					m_grid_conditions.SetItemText( dataRow, 2, pParams.strings[string_index++].c_str() );
					m_grid_conditions.SetItemData( dataRow, 2, pParams.ParamID );
					pGridBtnCell->SetComboString( sa );
				}
				
				if ( newRow != dataRow ) 
				{
					m_grid_conditions.SetItemState( dataRow, 3, GVIS_READONLY );
					m_grid_conditions.SetItemState( dataRow, 4, GVIS_READONLY );					
				}
				m_grid_conditions.SetItemData( dataRow, 3, pParams.ParamID );
				m_grid_conditions.SetItemData( dataRow, 4, pParams.ParamID );				
			}

			{
				gpstring sGroup;
				sGroup.assignf( "%d", pParams.SubGroup );
				m_grid_conditions.SetItemText( newRow, 3, sGroup.c_str() );
				m_grid_conditions.SetItemData( newRow, 3, pParams.ParamID);
			}

			{
				gpstring sDoc = pParams.Doc;
				m_grid_conditions.SetItemText( newRow, 4, sDoc.c_str() );
				m_grid_conditions.SetItemData( newRow, 4, pParams.ParamID );
			}
			
			m_grid_conditions.InsertRow( "" );
		}
	}

	// Setup Actions Grid
	{
		for (trigger::ActionConstIter ait = pTrigger->GetActions().begin(); ait != pTrigger->GetActions().end(); ++ait)
		{
			trigger::Parameter	*pFormat;
			trigger::Params		*pParams;
			gpstring			sAction;

			pTrigger->GetActionInfo( (*ait).first, sAction, pFormat, pParams );
			gpassert(pParams->ParamID != 0);
			
			int newRow = m_grid_actions.InsertRow( sAction.c_str() );			
			m_grid_actions.SetItemState( newRow, 0, GVIS_READONLY );
			m_grid_actions.SetItemData( newRow, 0, pParams->ParamID);
			
			// Get the parameters and their values
			int double_index	= 0;
			int int_index		= 0;
			int string_index	= 0;
			for ( unsigned int j = 0; j != pFormat->Size(); ++j ) 
			{
				trigger::Parameter::format *pParamFormat = 0;
				pFormat->Get( j, &pParamFormat );

				int dataRow = m_grid_actions.InsertRow( "" );
				m_grid_actions.SetItemText( dataRow, 1, pParamFormat->sName.c_str() );	
				m_grid_actions.SetItemData( dataRow, 1, pParams->ParamID );
				m_grid_actions.SetItemState( dataRow, 1, GVIS_READONLY );
				
				if ( pParamFormat->sValues.size() == 0 )
				{				
					gpstring sValue;
					if ( pParamFormat->isFloat ) 
					{
						sValue.assignf( "%f", pParams->floats[double_index++] );				
					}
					else if ( pParamFormat->isInt || pParamFormat->isGoid ) 
					{		
						if ( pParamFormat->isBool )
						{
							if ( pParams->ints[int_index] == 1 )
							{
								sValue.assign( "true" );
							}
							else
							{
								sValue.assign( "false" );
							}							

							int_index++;
						}
						else if ( gPreferences.GetDisplayHexadecimal() || pParamFormat->bSaveAsHex )
						{
							sValue.assignf( "0x%08X", pParams->ints[int_index++] );
						}
						else
						{
							sValue.assignf( "%d", pParams->ints[int_index++] );				
						}
					}
					else if ( pParamFormat->isString ) 
					{
						sValue.assignf( "%s", pParams->strings[string_index++].c_str() );				
					}

					m_grid_actions.SetItemText( dataRow, 2, sValue.c_str() );				
					m_grid_actions.SetItemData( dataRow, 2, pParams->ParamID);
				}
				else
				{					
					m_grid_actions.SetCellType( dataRow, 2, RUNTIME_CLASS(CGridBtnCellCombo) );
					CGridBtnCellCombo* pGridBtnCell = (CGridBtnCellCombo*)m_grid_actions.GetCell( dataRow, 2 );
					pGridBtnCell->SetBtnDataBase( &m_BtnDataBaseAction );												
					pGridBtnCell->SetComboStyle( CBS_DROPDOWN );					

					CStringArray sa;					
					StringVec::iterator iValues;
					for ( iValues = pParamFormat->sValues.begin(); iValues != pParamFormat->sValues.end(); ++iValues )
					{
						sa.Add( (*iValues).c_str() );
					}

					if ( pParams->strings.size() > string_index )
					{
						m_grid_actions.SetItemText( dataRow, 2, pParams->strings[string_index++].c_str() );
					}
					m_grid_actions.SetItemData( dataRow, 2, pParams->ParamID );
					pGridBtnCell->SetComboString( sa );
				}		
				
				if ( newRow != dataRow ) 
				{
					m_grid_actions.SetItemState( dataRow, 3, GVIS_READONLY );
					m_grid_actions.SetItemState( dataRow, 4, GVIS_READONLY );
					m_grid_actions.SetItemState( dataRow, 5, GVIS_READONLY );
					m_grid_actions.SetItemState( dataRow, 6, GVIS_READONLY );					
				}
				m_grid_actions.SetItemData( dataRow, 3, pParams->ParamID );
				m_grid_actions.SetItemData( dataRow, 4, pParams->ParamID );
				m_grid_actions.SetItemData( dataRow, 5, pParams->ParamID );
				m_grid_actions.SetItemData( dataRow, 6, pParams->ParamID );
			}

			{
				m_grid_actions.SetCellType( newRow, 3, RUNTIME_CLASS(CGridBtnCellCombo) );
				CGridBtnCellCombo* pGridBtnCell = (CGridBtnCellCombo*)m_grid_actions.GetCell( newRow, 3 );
				pGridBtnCell->SetBtnDataBase( &m_BtnDataBaseAction );												
				pGridBtnCell->SetComboStyle( CBS_DROPDOWN );					
				CStringArray sa;
				sa.Add( "true" );
				sa.Add( "false" );
				pGridBtnCell->SetComboString( sa );

				if ( pParams->bWhenFalse )
				{				
					m_grid_actions.SetItemText( newRow, 3, "false" );
				}
				else
				{					
					m_grid_actions.SetItemText( newRow, 3, "true" );					
				}
				m_grid_actions.SetItemData( newRow, 3, pParams->ParamID );
			}

			{
				gpstring sDelay;
				sDelay.assignf( "%f", pParams->Delay );
				m_grid_actions.SetItemText( newRow, 4, sDelay.c_str() );
				m_grid_actions.SetItemData( newRow, 4, pParams->ParamID );
			}

			{
				gpstring sGroup;
				sGroup.assignf( "%d", pParams->SubGroup );
				m_grid_actions.SetItemText( newRow, 5, sGroup.c_str() );
				m_grid_actions.SetItemData( newRow, 5, pParams->ParamID );
			}

			{
				gpstring sDoc = pParams->Doc;
				m_grid_actions.SetItemText( newRow, 6, sDoc.c_str() );
				m_grid_actions.SetItemData( newRow, 6, pParams->ParamID );
			}

			m_grid_actions.InsertRow( "" );
		}
	}

	m_grid_actions.AutoSize();
	m_grid_conditions.AutoSize();
}


void PropPage_Properties_Triggers::Reset()
{
	if ( !GetSafeHwnd() )
	{
		return;
	}

	Goid object = GOID_INVALID;
	int index = 0;
	if ( RetrieveSelectedData( object, index ) )
	{
		gEditorTriggers.SetCurrentGoid( object );
		gEditorTriggers.SetCurrentTriggerIndex( index );
	}
	else
	{
		gEditorTriggers.SetCurrentGoid( GOID_INVALID );
		gEditorTriggers.SetCurrentTriggerIndex( 0 );
	}

	SetupTriggerTree();	
	m_bApply = false;
	gObjectProperties.Evaluate();

	m_oneshot.EnableWindow( FALSE );
	m_flipflop.EnableWindow( FALSE );
	m_active.EnableWindow( FALSE );
	m_resetdelay.EnableWindow( FALSE );
	m_delay.EnableWindow( FALSE );
	m_occupants.EnableWindow( FALSE );
	m_singleplayer.EnableWindow( FALSE );
	m_multiplayer.EnableWindow( FALSE );
	m_selfdestruct.EnableWindow( FALSE );

	m_triggers.Expand( m_triggers.GetRootItem(), TVE_EXPAND );
	HTREEITEM hChild = m_triggers.GetChildItem( m_triggers.GetRootItem() );
	if ( hChild )
	{
		m_triggers.SelectItem( hChild );
	}

	InitializeTriggerGrids();
}


void PropPage_Properties_Triggers::Apply()
{
	m_bApply = false;
}


void PropPage_Properties_Triggers::Cancel()
{
}

BOOL PropPage_Properties_Triggers::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
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

	if ( m_grid_conditions.GetSafeHwnd() && (wParam == (WPARAM)m_grid_conditions.GetDlgCtrlID()))
    {
		GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;
        if (GVN_ENDLABELEDIT == pDispInfo->hdr.code)
        {
			CString rParameter = m_grid_conditions.GetItemText( pDispInfo->item.row, pDispInfo->item.col-1 );
			if (( rParameter.IsEmpty() == false ) && ( pDispInfo->item.col == 2 ))
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	pFormat;
				trigger::Params		pParams;
				gpstring			sCondition;

				int index = m_grid_conditions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetConditionInfo( index, sCondition, pFormat, pParams );
				gpassert(pParams.ParamID != 0);

				// Get the parameters and their values
				int double_index	= 0;
				int int_index		= 0;
				int string_index	= 0;
				for ( unsigned int i = 0; i != pFormat.Size(); ++i ) 
				{
					trigger::Parameter::format *pParamFormat = 0;
					pFormat.Get( i, &pParamFormat );

					CString rValue = m_grid_conditions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
					gpstring sValue = rValue.GetBuffer( 0 );

					if ( pParamFormat->sName.same_no_case( rParameter.GetBuffer( 0 ) ) )
					{						
						if ( pParamFormat->isFloat ) 
						{
							stringtool::Get( sValue, pParams.floats[double_index] );
						}
						else if ( pParamFormat->isInt || pParamFormat->isGoid ) 
						{
							if ( pParamFormat->isBool )
							{
								if ( sValue.same_no_case( "true" ) )
								{
									pParams.ints[int_index] = 1;
								}
								else if ( sValue.same_no_case( "false" ) )
								{
									pParams.ints[int_index] = 0;
								}
								else
								{
									pParams.ints[int_index] = 0;
								}
							}
							else
							{
								stringtool::Get( sValue, pParams.ints[int_index] );
							}
						}
						else if ( pParamFormat->isString ) 
						{
							pParams.strings[string_index] = sValue;
						}
					}

					if ( pParamFormat->isFloat ) 
					{						
						double_index++;
					}
					else if ( pParamFormat->isInt || pParamFormat->isGoid ) 
					{					
						int_index++;
					}
					else if ( pParamFormat->isString ) 
					{					
						string_index++;
					}
				}
				
				// Store the updated info back into the condition
				pTrigger->SetConditionInfo( pParams.ParamID, pParams );
				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
			else if ( pDispInfo->item.col == 3 )
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	pFormat;
				trigger::Params		pParams;
				gpstring			sCondition;

				int index = m_grid_conditions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetConditionInfo( index, sCondition, pFormat, pParams );
				gpassert(pParams.ParamID != 0);

				CString rCondition = m_grid_conditions.GetItemText( pDispInfo->item.row, 0 );
				if ( !sCondition.same_no_case( rCondition.GetBuffer( 0 ) ) || sCondition.empty() )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				CString rValue = m_grid_conditions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				gpstring sValue = rValue.GetBuffer( 0 );
				
				int group = 0;
				stringtool::Get( rValue.GetBuffer( 0 ), group );
				pParams.SubGroup = group;		
				
				// Store the updated info back into the condition
				pTrigger->SetConditionInfo( pParams.ParamID, pParams );
				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();			
			}
			else if ( pDispInfo->item.col == 4 )
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	pFormat;
				trigger::Params		pParams;
				gpstring			sCondition;

				int index = m_grid_conditions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetConditionInfo( index, sCondition, pFormat, pParams );
				gpassert(pParams.ParamID != 0);

				CString rCondition = m_grid_conditions.GetItemText( pDispInfo->item.row, 0 );
				if ( !sCondition.same_no_case( rCondition.GetBuffer( 0 ) ) || sCondition.empty() )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				CString rValue = m_grid_conditions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				gpstring sValue = rValue.GetBuffer( 0 );
				
				pParams.Doc = sValue;		
				
				// Store the updated info back into the condition
				pTrigger->SetConditionInfo( pParams.ParamID, pParams );
				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
		}
	}
	else if ( m_grid_actions.GetSafeHwnd() && (wParam == (WPARAM)m_grid_actions.GetDlgCtrlID()))
	{
		GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;
        if (GVN_ENDLABELEDIT == pDispInfo->hdr.code)
        {			
			CString rParameter = m_grid_actions.GetItemText( pDispInfo->item.row, pDispInfo->item.col-1 );
			
			if ( pDispInfo->item.col == 3 )
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	*pFormat = 0;
				trigger::Params		*pParams = 0;
				gpstring			sAction;

				int index = m_grid_actions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetActionInfo( index, sAction, pFormat, pParams );
				gpassert(pParams->ParamID != 0);

				CString rAction = m_grid_actions.GetItemText( pDispInfo->item.row, 0 );
				if ( !sAction.same_no_case( rAction.GetBuffer( 0 ) ) || sAction.empty() )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				CString rValue = m_grid_actions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				gpstring sValue = rValue.GetBuffer( 0 );
				
				if ( sValue.same_no_case( "true" ) )
				{
					pParams->bWhenFalse = false;
				}
				else if ( sValue.same_no_case( "false" ) )
				{
					pParams->bWhenFalse = true;
				}

				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
			else if ( pDispInfo->item.col == 4 )
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	*pFormat = 0;
				trigger::Params		*pParams = 0;
				gpstring			sAction;

				int index = m_grid_actions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetActionInfo( index, sAction, pFormat, pParams );
				gpassert(pParams->ParamID != 0);

				CString rAction = m_grid_actions.GetItemText( pDispInfo->item.row, 0 );
				if ( !sAction.same_no_case( rAction.GetBuffer( 0 ) ) || sAction.empty() )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				CString rValue = m_grid_actions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				gpstring sValue = rValue.GetBuffer( 0 );
				
				float delay = 0;
				stringtool::Get( rValue.GetBuffer( 0 ), delay );
				pParams->Delay = delay;		
				
				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
			else if ( pDispInfo->item.col == 5 )
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	*pFormat = 0;
				trigger::Params		*pParams = 0;
				gpstring			sAction;

				int index = m_grid_actions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetActionInfo( index, sAction, pFormat, pParams );
				gpassert(pParams->ParamID != 0);

				CString rAction = m_grid_actions.GetItemText( pDispInfo->item.row, 0 );
				if ( !sAction.same_no_case( rAction.GetBuffer( 0 ) ) || sAction.empty() )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				CString rValue = m_grid_actions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				gpstring sValue = rValue.GetBuffer( 0 );
				
				int group = 0;
				stringtool::Get( rValue.GetBuffer( 0 ), group );
				pParams->SubGroup = group;		
				
				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
			else if ( pDispInfo->item.col == 6 )
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	*pFormat = 0;
				trigger::Params		*pParams = 0;
				gpstring			sAction;

				int index = m_grid_actions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetActionInfo( index, sAction, pFormat, pParams );
				gpassert(pParams->ParamID != 0);

				CString rAction = m_grid_actions.GetItemText( pDispInfo->item.row, 0 );
				if ( !sAction.same_no_case( rAction.GetBuffer( 0 ) ) || sAction.empty() )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				CString rValue = m_grid_actions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				gpstring sValue = rValue.GetBuffer( 0 );
				
				pParams->Doc = sValue;		
				
				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
			else if (( rParameter.IsEmpty() == false ) && ( pDispInfo->item.col != 1 ))
			{
				GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
				if ( hObject.IsValid() == false )
				{
					return CDialog::OnNotify(wParam, lParam, pResult);
				}
				
				trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

				trigger::Parameter	*pFormat = 0;
				trigger::Params		*pParams = 0;
				gpstring			sAction;

				int index = m_grid_actions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetActionInfo( index, sAction, pFormat, pParams );
				gpassert(pParams->ParamID != 0);

				// Get the parameters and their values
				int double_index	= 0;
				int int_index		= 0;
				int string_index	= 0;
				for ( unsigned int i = 0; i != pFormat->Size(); ++i ) 
				{
					trigger::Parameter::format *pParamFormat = 0;
					pFormat->Get( i, &pParamFormat );

					CString rValue = m_grid_actions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );					

					if ( pParamFormat->sName.same_no_case( rParameter.GetBuffer( 0 ) ) )
					{						
						if ( pParamFormat->isFloat ) 
						{
							gpstring sValue = rValue.GetBuffer( 0 );
							stringtool::Get( sValue, pParams->floats[double_index] );
						}
						else if ( pParamFormat->isInt || pParamFormat->isGoid ) 
						{
							gpstring sValue = rValue.GetBuffer( 0 );							
						
							if ( pParamFormat->isBool )
							{
								if ( sValue.same_no_case( "true" ) )
								{
									pParams->ints[int_index] = 1;
								}
								else if ( sValue.same_no_case( "false" ) )
								{
									pParams->ints[int_index] = 0;
								}
								else
								{
									pParams->ints[int_index] = 0;
								}
							}
							else
							{
								stringtool::Get( sValue, pParams->ints[int_index] );
							}						
						}
						else if ( pParamFormat->isString ) 
						{
							if ( pParams->strings.size() == string_index )
							{
								pParams->strings.push_back( rValue.GetBuffer( 0 ) );
							}
							else
							{
								pParams->strings[string_index] = rValue.GetBuffer( 0 );
							}
						}
					}

					if ( pParamFormat->isFloat ) 
					{						
						double_index++;
					}
					else if ( pParamFormat->isInt || pParamFormat->isGoid ) 
					{					
						int_index++;
					}
					else if ( pParamFormat->isString ) 
					{					
						string_index++;
					}
				}	

				{
					gpstring errmsg;
					errmsg.assignf("Invalid parameter entered");
					if (!pTrigger->Validate(errmsg))
					{
						gperrorf((errmsg.c_str()));
					}
				}
				m_bApply = true;
				gObjectProperties.Evaluate();
			}
		}
	}
	
	return CResizablePage::OnNotify(wParam, lParam, pResult);
}

void PropPage_Properties_Triggers::OnChangeEditDelay() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

	CString rDelay;
	m_delay.GetWindowText( rDelay );
	gpstring sDelay = rDelay;
	double duration = 0;
	stringtool::Get( sDelay, duration );
	pTrigger->SetDelay( duration );		
	
	m_bApply = true;
	gObjectProperties.Evaluate();	
}

void PropPage_Properties_Triggers::OnChangeEditOccupants() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

	CString rOccupants;
	m_occupants.GetWindowText( rOccupants );	
	pTrigger->SetGroupName( rOccupants.GetBuffer( 0 ) );		
	
	m_bApply = true;
	gObjectProperties.Evaluate();
	
}

void PropPage_Properties_Triggers::OnCheckActive() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
	pTrigger->SetRequestedActive( m_active.GetCheck() ? true : false );		
	m_bApply = true;
	gObjectProperties.Evaluate();	
}

void PropPage_Properties_Triggers::OnCheckSingleplayer() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
	pTrigger->SetIsSingle( m_singleplayer.GetCheck() ? true : false );		
	m_bApply = true;
	gObjectProperties.Evaluate();	
}

void PropPage_Properties_Triggers::OnCheckMultiplayer() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
	pTrigger->SetIsMulti( m_multiplayer.GetCheck() ? true : false );		
	m_bApply = true;
	gObjectProperties.Evaluate();		
}

void PropPage_Properties_Triggers::OnCheckCanSelfDestruct() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
	pTrigger->SetCanSelfDestruct( m_selfdestruct.GetCheck() ? true : false );		
	m_bApply = true;
	gObjectProperties.Evaluate();				
}

void PropPage_Properties_Triggers::BeginTriggerEdit()
{
	/*
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
	
	for ( trigger::ConditionConstIter cit = pTrigger->GetConditions().begin(); cit != pTrigger->GetConditions().end(); ++cit )
	{
		m_conditionIdMap.insert( cit.first
	}
	*/
}

void PropPage_Properties_Triggers::EndTriggerEdit()
{
}
