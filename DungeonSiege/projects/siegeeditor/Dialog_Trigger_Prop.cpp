// Dialog_Trigger_Prop.cpp : implementation file
//


#include "PrecompEditor.h"
#include "stdafx.h"
#include "Trigger_Sys.h"
#include "SiegeEditor.h"
#include "Dialog_Trigger_Prop.h"
#include "EditorTriggers.h"
#include "SiegeEditorShell.h"
#include "Dialog_Trigger_Condition_Properties.h"
#include "Dialog_Trigger_Action_Properties.h"
#include "GoCore.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Prop dialog


Dialog_Trigger_Prop::Dialog_Trigger_Prop(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Prop::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Prop)
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Prop::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Prop)
	DDX_Control(pDX, IDC_CHECK_FLIPFLOP, m_flipflop);
	DDX_Control(pDX, IDC_STATIC_NAME, m_name);
	DDX_Control(pDX, IDC_LIST_CONDITIONS, m_listConditions);
	DDX_Control(pDX, IDC_LIST_ACTIONS, m_listActions);
	DDX_Control(pDX, IDC_EDIT_RESET_DELAY, m_resetDelay);
	DDX_Control(pDX, IDC_CHECK_ONESHOT, m_oneShot);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Prop, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Prop)
	ON_BN_CLICKED(IDC_BUTTON_NEWCONDITION, OnButtonNewcondition)
	ON_BN_CLICKED(IDC_BUTTON_EDITCONDITION, OnButtonEditcondition)
	ON_BN_CLICKED(IDC_BUTTON_REMOVECONDITON, OnButtonRemoveconditon)
	ON_BN_CLICKED(IDC_BUTTON_NEWACTION, OnButtonNewaction)
	ON_BN_CLICKED(IDC_BUTTON_EDITACTION, OnButtonEditaction)
	ON_BN_CLICKED(IDC_BUTTON_REMOVEACTION, OnButtonRemoveaction)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Prop message handlers

void Dialog_Trigger_Prop::OnButtonNewcondition() 
{
	gEditorTriggers.SetCurrentConditionIndex( INVALID_INDEX );
	Dialog_Trigger_Condition_Properties dlgCondition;
	dlgCondition.DoModal();
	RefreshLists();
	
}

void Dialog_Trigger_Prop::OnButtonEditcondition() 
{
	if ( m_listConditions.GetCurSel() != LB_ERR )
	{
		gEditorTriggers.SetCurrentConditionIndex( m_listConditions.GetCurSel() );
		Dialog_Trigger_Condition_Properties dlgCondition;
		dlgCondition.DoModal();	
	}
	RefreshLists();
}

void Dialog_Trigger_Prop::OnButtonRemoveconditon() 
{
	if ( m_listConditions.GetCurSel() != LB_ERR )
	{
		int selection = m_listConditions.GetCurSel();
		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );

		trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
		trigger::Trigger *pTrigger = 0;
		pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
		pTrigger->RemoveCondition( selection );
		gEditorTriggers.SetCurrentConditionIndex( INVALID_INDEX );
		m_listConditions.DeleteString( m_listConditions.GetCurSel() );
	}
	
}

void Dialog_Trigger_Prop::OnButtonNewaction() 
{
	gEditorTriggers.SetCurrentActionIndex( INVALID_INDEX );
	Dialog_Trigger_Action_Properties dlgAction;
	dlgAction.DoModal();
	RefreshLists();	
}

void Dialog_Trigger_Prop::OnButtonEditaction() 
{
	if ( m_listActions.GetCurSel() != LB_ERR )
	{
		gEditorTriggers.SetCurrentActionIndex( m_listActions.GetCurSel() );
		Dialog_Trigger_Action_Properties dlgAction;
		dlgAction.DoModal();	
	}	
	RefreshLists();
}

void Dialog_Trigger_Prop::OnButtonRemoveaction() 
{
	if ( m_listActions.GetCurSel() != LB_ERR )
	{
		int selection = m_listActions.GetCurSel();
		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );

		trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
		trigger::Trigger *pTrigger = 0;
		pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
		pTrigger->RemoveAction( selection );
		gEditorTriggers.SetCurrentActionIndex( INVALID_INDEX );
		m_listActions.DeleteString( m_listActions.GetCurSel() );		
	}	
}

BOOL Dialog_Trigger_Prop::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );

	m_listActions.ResetContent();
	m_listConditions.ResetContent();	

	trigger::Storage	*pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	gpstring sReset;
	sReset.assignf( "%f", pTrigger->GetResetDuration() );
	m_resetDelay.SetWindowText( sReset );	
	m_oneShot.SetCheck( (int)pTrigger->IsSingleShot() );
	m_flipflop.SetCheck( (int)pTrigger->IsFlipFlop() );
	gpstring sName;
	sName.assignf( "trigger_%d", gEditorTriggers.GetCurrentTriggerIndex() );
	m_name.SetWindowText( sName );
	RefreshLists();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Trigger_Prop::OnOK() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );

	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	
	int check = m_oneShot.GetCheck();
	if ( check != 0 )
	{
		pTrigger->SetIsSingleShot( true );
	}
	else
	{
		pTrigger->SetIsSingleShot( false );
	}

	check = m_flipflop.GetCheck();
	if ( check != 0 )
	{
		pTrigger->SetIsFlipFlop( true );
	}
	else
	{
		pTrigger->SetIsFlipFlop( false );
	}

	CString rText;
	m_resetDelay.GetWindowText( rText );
	pTrigger->SetResetDuration( atof( rText.GetBuffer( 0 ) ) );
	
	CDialog::OnOK();
}


void Dialog_Trigger_Prop::RefreshLists()
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );

	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		
	m_listConditions.ResetContent();

	for ( unsigned int i = 0; i != pTrigger->GetConditionCount(); ++i ) {
		trigger::Parameter	*pFormat = 0;
		trigger::Params		*pParams = 0;
		gpstring			sCondition;
		pTrigger->GetConditionInfo( i, sCondition, &pFormat, &pParams );
		m_listConditions.AddString( sCondition.c_str() );		
	}
	
	gEditorTriggers.SetCurrentConditionIndex( INVALID_INDEX );

	
	m_listActions.ResetContent();

	for ( i = 0; i != pTrigger->GetActionCount(); ++i ) {
		trigger::Parameter	*pFormat = 0;
		trigger::Params		*pParams = 0;
		gpstring			sAction;
		pTrigger->GetActionInfo( i, sAction, &pFormat, &pParams );
		m_listActions.AddString( sAction.c_str() );		
	}
	
	gEditorTriggers.SetCurrentConditionIndex( INVALID_INDEX );
}