// Dialog_Triggers.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "siegeeditor.h"
#include "Dialog_Triggers.h"
#include "GizmoManager.h"
#include "Dialog_Trigger_Add_Condition.h"
#include "Dialog_Trigger_Add_Action.h"
#include "Trigger_Sys.h"
#include "EditorTriggers.h"
#include "GoCore.h"
#include "gridctrl\\GridBtnCellCombo.h"
#include "stringtool.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Triggers dialog


Dialog_Triggers::Dialog_Triggers(CWnd* pParent /*=NULL*/)
	: CResizableDialog(Dialog_Triggers::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Triggers)
	//}}AFX_DATA_INIT
}


void Dialog_Triggers::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Triggers)
	DDX_Control(pDX, IDC_TREE_TRIGGERS, m_triggers);
	DDX_Control(pDX, IDC_STATIC_REPLACE_ACTIONS, m_replace_actions);
	DDX_Control(pDX, IDC_STATIC_REPLACE_CONDITIONS, m_replace_conditions);
	DDX_Control(pDX, IDC_EDIT_RESET_DELAY, m_resetdelay);
	DDX_Control(pDX, IDC_CHECK_ONE_SHOT, m_oneshot);
	DDX_Control(pDX, IDC_CHECK_FLIPFLOP, m_flipflop);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Triggers, CResizableDialog)
	//{{AFX_MSG_MAP(Dialog_Triggers)
	ON_BN_CLICKED(IDC_BUTTON_ADD_CONDITION, OnButtonAddCondition)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_CONDITION, OnButtonRemoveCondition)
	ON_BN_CLICKED(IDC_BUTTON_ADD_ACTION, OnButtonAddAction)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_ACTIONS, OnButtonRemoveActions)
	ON_BN_CLICKED(IDC_BUTTON_NEW_TRIGGER, OnButtonNewTrigger)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_TRIGGER, OnButtonRemoveTrigger)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_TRIGGERS, OnSelchangedTreeTriggers)
	ON_BN_CLICKED(IDC_CHECK_ONE_SHOT, OnCheckOneShot)
	ON_BN_CLICKED(IDC_CHECK_FLIPFLOP, OnCheckFlipflop)
	ON_EN_CHANGE(IDC_EDIT_RESET_DELAY, OnChangeEditResetDelay)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Triggers message handlers

BOOL Dialog_Triggers::OnInitDialog() 
{
	CResizableDialog::OnInitDialog();

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
	conditionRect.top		= replaceCRect.top - dlgRect.top - GetSystemMetrics( SM_CYCAPTION );
	conditionRect.bottom	= replaceCRect.bottom - dlgRect.top - GetSystemMetrics( SM_CYCAPTION );
	
	actionRect.left			= replaceARect.left - dlgRect.left;
	actionRect.right		= replaceARect.right - dlgRect.left;
	actionRect.top			= replaceARect.top - dlgRect.top - GetSystemMetrics( SM_CYCAPTION );
	actionRect.bottom		= replaceARect.bottom - dlgRect.top - GetSystemMetrics( SM_CYCAPTION );
	
	m_grid_conditions.Create( conditionRect, this, IDC_GRID_CONDITIONS, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	
	m_grid_actions.Create( actionRect, this, IDC_GRID_ACTIONS, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	

	ShowSizeGrip( TRUE );
	
	AddAnchor( IDC_STATIC_TRIGGERS, TOP_LEFT, BOTTOM_LEFT );
	AddAnchor( IDC_TREE_TRIGGERS, TOP_LEFT, BOTTOM_LEFT );

	AddAnchor( IDC_STATIC_PROPERTIES, TOP_LEFT );
	//AddAnchor( IDC_CHECK_ONE_SHOT, TOP_LEFT );
	//AddAnchor( IDC_CHECK_FLIPFLOP, TOP_LEFT );
	AddAnchor( IDC_EDIT_RESET_DELAY, TOP_LEFT );
	AddAnchor( IDC_STATIC_RESET_DELAY, TOP_LEFT );

	AddAnchor( IDC_STATIC_CONDITIONS, TOP_LEFT, MIDDLE_RIGHT );	
	AddAnchor( IDC_GRID_CONDITIONS, TOP_LEFT, MIDDLE_RIGHT );

	AddAnchor( IDC_STATIC_ACTIONS, MIDDLE_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_GRID_ACTIONS, MIDDLE_LEFT, BOTTOM_RIGHT );
	
	AddAnchor( IDOK, TOP_RIGHT );
	AddAnchor( IDCANCEL, TOP_RIGHT );

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
	
	SetupTriggerTree();	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Triggers::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void Dialog_Triggers::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void Dialog_Triggers::OnButtonAddCondition() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() )
	{
		gEditorTriggers.SetCurrentConditionIndex( INVALID_INDEX );
		Dialog_Trigger_Add_Condition dlgCondition( this );
		dlgCondition.DoModal();
		InitializeTriggerGrids();	
	}
}

void Dialog_Triggers::OnButtonRemoveCondition() 
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
		
			pTrigger->RemoveCondition( data );
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
				
					pTrigger->RemoveCondition( data );
				}
			}	
		}
		
		InitializeTriggerGrids();	
	}
}

void Dialog_Triggers::OnButtonAddAction() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	if ( hObject.IsValid() )
	{
		gEditorTriggers.SetCurrentActionIndex( INVALID_INDEX );
		Dialog_Trigger_Add_Action dlgAction( this );
		dlgAction.DoModal();
		InitializeTriggerGrids();
	}
}

void Dialog_Triggers::OnButtonRemoveActions() 
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
		
			pTrigger->RemoveAction( data );
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
				
					pTrigger->RemoveAction( data );
				}
			}				
		}
		InitializeTriggerGrids();
	}
}

void Dialog_Triggers::OnButtonNewTrigger() 
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

void Dialog_Triggers::OnButtonRemoveTrigger() 
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

void Dialog_Triggers::OnSelchangedTreeTriggers(NMHDR* pNMHDR, LRESULT* pResult) 
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
		m_oneshot.SetCheck( pTrigger->IsSingleShot() );

		gpstring sDelay;
		sDelay.assignf( "%f", pTrigger->GetResetDuration() );
		m_resetdelay.SetWindowText( sDelay.c_str() );

		InitializeTriggerGrids();
	}
	
	*pResult = 0;
}


void Dialog_Triggers::SetupTriggerTree()
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

	m_triggers.Expand( m_triggers.GetRootItem(), TVE_EXPAND );
}


bool Dialog_Triggers::RetrieveSelectedData( Goid & object, int & index )
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


void Dialog_Triggers::InitializeTriggerGrids()
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
		
	m_grid_actions.InsertColumn( "Name" );
	m_grid_actions.InsertColumn( "Parameter" );
	m_grid_actions.InsertColumn( "Value" );
	m_grid_actions.InsertColumn( "Condition" );	
	m_grid_actions.InsertColumn( "Delay" );
	
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
		for ( unsigned int i = 0; i != pTrigger->GetConditionCount(); ++i ) 
		{
			trigger::Parameter	*pFormat = 0;
			trigger::Params		*pParams = 0;
			gpstring			sCondition;
			pTrigger->GetConditionInfo( i, sCondition, &pFormat, &pParams );
			
			int newRow = m_grid_conditions.InsertRow( sCondition.c_str() );			
			m_grid_conditions.SetItemState( newRow, 0, GVIS_READONLY );
			m_grid_conditions.SetItemData( newRow, 0, i );
			
			// Get the parameters and their values
			int double_index	= 0;
			int int_index		= 0;
			int string_index	= 0;
			for ( unsigned int j = 0; j != pFormat->Size(); ++j ) 
			{
				trigger::Parameter::format *pParamFormat = 0;
				pFormat->Get( j, &pParamFormat );
				int dataRow = m_grid_conditions.InsertRow( "" );
				m_grid_conditions.SetItemText( dataRow, 1, pParamFormat->sName.c_str() );	
				m_grid_conditions.SetItemData( dataRow, 1, i );
				m_grid_conditions.SetItemState( dataRow, 1, GVIS_READONLY );
					
				if ( pParamFormat->sValues.size() == 0 )
				{
					gpstring sValue;
					if ( pParamFormat->isFloat ) {
						sValue.assignf( "%f", pParams->floats[double_index++] );				
					}
					else if ( pParamFormat->isInt ) {
						sValue.assignf( "%d", pParams->ints[int_index++] );				
					}
					else if ( pParamFormat->isString ) {
						sValue.assignf( "%s", pParams->strings[string_index++].c_str() );				
					}

					m_grid_conditions.SetItemText( dataRow, 2, sValue.c_str() );				
					m_grid_conditions.SetItemData( dataRow, 2, i );
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

					m_grid_conditions.SetItemText( dataRow, 2, pParams->strings[string_index++].c_str() );
					m_grid_conditions.SetItemData( dataRow, 2, i );
					pGridBtnCell->SetComboString( sa );
				}
			}				
		}
	}

	// Setup Actions Grid
	{
		for ( unsigned int i = 0; i != pTrigger->GetActionCount(); ++i ) 
		{
			trigger::Parameter	*pFormat = 0;
			trigger::Params		*pParams = 0;
			gpstring			sAction;
			pTrigger->GetActionInfo( i, sAction, &pFormat, &pParams );
			
			int newRow = m_grid_actions.InsertRow( sAction.c_str() );			
			m_grid_actions.SetItemState( newRow, 0, GVIS_READONLY );
			m_grid_actions.SetItemData( newRow, 0, i );
			
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
				m_grid_actions.SetItemData( dataRow, 1, i );
				m_grid_actions.SetItemState( dataRow, 1, GVIS_READONLY );
				
				if ( pParamFormat->sValues.size() == 0 )
				{				
					gpstring sValue;
					if ( pParamFormat->isFloat ) 
					{
						sValue.assignf( "%f", pParams->floats[double_index++] );				
					}
					else if ( pParamFormat->isInt ) 
					{
						sValue.assignf( "%d", pParams->ints[int_index++] );				
					}
					else if ( pParamFormat->isString ) 
					{
						sValue.assignf( "%s", pParams->strings[string_index++].c_str() );				
					}

					m_grid_actions.SetItemText( dataRow, 2, sValue.c_str() );				
					m_grid_actions.SetItemData( dataRow, 2, i );
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

					m_grid_actions.SetItemText( dataRow, 2, pParams->strings[string_index++].c_str() );
					m_grid_actions.SetItemData( dataRow, 2, i );
					pGridBtnCell->SetComboString( sa );
				}				
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
					m_grid_actions.SetItemData( newRow, 3, i );
				}
				else
				{					
					m_grid_actions.SetItemText( newRow, 3, "true" );					
				}
				m_grid_actions.SetItemData( newRow, 3, i );
			}

			{
				gpstring sDelay;
				sDelay.assignf( "%f", pParams->Delay );
				m_grid_actions.SetItemText( newRow, 4, sDelay.c_str() );
				m_grid_actions.SetItemData( newRow, 4, i );
			}
		}
	}

	m_grid_actions.AutoSize();
	m_grid_conditions.AutoSize();
}

BOOL Dialog_Triggers::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if ( m_grid_conditions.GetSafeHwnd() && (wParam == (WPARAM)m_grid_conditions.GetDlgCtrlID()))
    {
		GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;
        if (GVN_ENDLABELEDIT == pDispInfo->hdr.code)
        {
			CString rParameter = m_grid_conditions.GetItemText( pDispInfo->item.row, pDispInfo->item.col-1 );
			if ( rParameter.IsEmpty() == false )
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
				gpstring			sCondition;

				int index = m_grid_conditions.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				pTrigger->GetConditionInfo( index, sCondition, &pFormat, &pParams );

				// Get the parameters and their values
				int double_index	= 0;
				int int_index		= 0;
				int string_index	= 0;
				for ( unsigned int i = 0; i != pFormat->Size(); ++i ) 
				{
					trigger::Parameter::format *pParamFormat = 0;
					pFormat->Get( i, &pParamFormat );

					CString rValue = m_grid_conditions.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
					gpstring sValue = rValue.GetBuffer( 0 );

					if ( pParamFormat->sName.same_no_case( rParameter.GetBuffer( 0 ) ) )
					{						
						if ( pParamFormat->isFloat ) 
						{
							stringtool::Get( sValue, pParams->floats[double_index] );
						}
						else if ( pParamFormat->isInt ) 
						{
							stringtool::Get( sValue, pParams->ints[int_index] );
						}
						else if ( pParamFormat->isString ) 
						{
							pParams->strings[string_index] = sValue;
						}
					}

					if ( pParamFormat->isFloat ) 
					{						
						double_index++;
					}
					else if ( pParamFormat->isInt ) 
					{					
						int_index++;
					}
					else if ( pParamFormat->isString ) 
					{					
						string_index++;
					}
				}							
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
				pTrigger->GetActionInfo( index, sAction, &pFormat, &pParams );

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
				pTrigger->GetActionInfo( index, sAction, &pFormat, &pParams );

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
			}
			else if ( rParameter.IsEmpty() == false )
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
				pTrigger->GetActionInfo( index, sAction, &pFormat, &pParams );

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
						else if ( pParamFormat->isInt ) 
						{
							gpstring sValue = rValue.GetBuffer( 0 );
							stringtool::Get( sValue, pParams->ints[int_index] );
						}
						else if ( pParamFormat->isString ) 
						{
							pParams->strings[string_index] = rValue.GetBuffer( 0 );
						}
					}

					if ( pParamFormat->isFloat ) 
					{						
						double_index++;
					}
					else if ( pParamFormat->isInt ) 
					{					
						int_index++;
					}
					else if ( pParamFormat->isString ) 
					{					
						string_index++;
					}
				}							
			}
		}
	}
	
	return CDialog::OnNotify(wParam, lParam, pResult);
}

void Dialog_Triggers::OnCheckOneShot() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
	pTrigger->SetIsSingleShot( m_oneshot.GetCheck() ? true : false );	
}

void Dialog_Triggers::OnCheckFlipflop() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
	pTrigger->SetIsFlipFlop( m_flipflop.GetCheck() ? true : false );	
}

void Dialog_Triggers::OnChangeEditResetDelay() 
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
}
