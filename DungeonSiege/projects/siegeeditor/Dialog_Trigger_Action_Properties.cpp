// Dialog_Trigger_Action_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Trigger_Action_Properties.h"
#include "Trigger_Sys.h"
#include "World.h"
#include "EditorTriggers.h"
#include "stringtool.h"
#include "Dialog_Trigger_Value.h"
#include "GoCore.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Action_Properties dialog


Dialog_Trigger_Action_Properties::Dialog_Trigger_Action_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Action_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Action_Properties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Action_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Action_Properties)
	DDX_Control(pDX, IDC_CHECK_WHENFALSE, m_whenfalse);
	DDX_Control(pDX, IDC_PARAMETER_TREE, m_wndParameters);
	DDX_Control(pDX, IDC_ACTION_COMBO, m_actions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Action_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Action_Properties)
	ON_CBN_SELCHANGE(IDC_ACTION_COMBO, OnEditchangeActionCombo)
	ON_NOTIFY(NM_DBLCLK, IDC_PARAMETER_TREE, OnClickParameterTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Action_Properties message handlers

BOOL Dialog_Trigger_Action_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	trigger::Parameter *pFormat = 0;
	m_actions.ResetContent();
	gpstring sName;
	for ( unsigned int i = 0; i != gTriggerSys.GetActionCount(); ++i ) {
		gTriggerSys.GetActionInfo( i, sName, &pFormat );
		m_actions.AddString( sName.c_str() );		
	}
			
	m_parameters.clear();
	m_values.clear();

	if ( gEditorTriggers.GetCurrentActionIndex() != INVALID_INDEX ) 
	{
		m_actions.EnableWindow( FALSE );

		trigger::Parameter	*pFormat = 0;
		trigger::Params		*pParams = 0;
		gpstring			sAction;

		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
		trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
		trigger::Trigger *pTrigger = 0;
		pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
		pTrigger->GetActionInfo( gEditorTriggers.GetCurrentActionIndex(),
									sAction, &pFormat, &pParams );

		m_whenfalse.SetCheck( pParams->bWhenFalse ? 1 : 0 );

		int action_sel = m_actions.FindString( 0, sAction.c_str() );
		m_actions.SetCurSel( action_sel );
		m_actions.SelectString( 0, sAction.c_str() );
		m_actions.SetTopIndex( action_sel );	
		
		int double_index	= 0;
		int int_index		= 0;
		int string_index	= 0;
		for ( unsigned int i = 0; i != pFormat->Size(); ++i ) {
			trigger::Parameter::format *pParamFormat = 0;
			pFormat->Get( i, &pParamFormat );					

			HTREEITEM hTreeParent = m_wndParameters.InsertItem(	TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE,
																pParamFormat->sName.c_str(), IMAGE_3BOX, IMAGE_3BOX, 0, 0, i, TVI_ROOT, TVI_SORT );		

			m_parameters.push_back( pParamFormat->sName );
			
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

			m_values.push_back( sValue );							
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Trigger_Action_Properties::OnOK() 
{
	GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	
	trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );

	trigger::Params params;
	int selection = m_actions.GetCurSel();
	if ( selection != CB_ERR ) {

		gpstring sName;
		trigger::Parameter *pFormat = 0;
		gTriggerSys.GetActionInfo( selection, sName, &pFormat );
		
		StringVec::iterator j;
		int value_index = 0;
		for ( j = m_parameters.begin(); j != m_parameters.end(); ++j ) {						
			for ( unsigned int i = 0; i != pFormat->Size(); ++i ) {							
				trigger::Parameter::format *pParamFormat = 0;
				pFormat->Get( i, &pParamFormat );						
				
				if ( (*j).same_no_case( pParamFormat->sName )) {
					if ( pParamFormat->isFloat ) {
						double dvalue = atof( m_values[value_index].c_str() );
						params.floats.push_back( dvalue );
					}
					else if ( pParamFormat->isInt ) {
						gpstring sInteger = m_values[value_index];
						int number = 0;
						stringtool::Get( sInteger, number );						
						params.ints.push_back( number );
					}
					else if ( pParamFormat->isString ) {
						params.strings.push_back( m_values[value_index] );
					}
				}
			}
			++value_index;
		}
		
		if ( gEditorTriggers.GetCurrentActionIndex() != INVALID_INDEX ) {
			trigger::Parameter	*pFormat = 0;
			trigger::Params		*pParams = 0;
			gpstring			sAction;

			pTrigger->GetActionInfo( gEditorTriggers.GetCurrentActionIndex(),
										sAction, &pFormat, &pParams );
			pParams->floats	= params.floats;
			pParams->ints		= params.ints;
			pParams->strings	= params.strings;
			pParams->bWhenFalse	= m_whenfalse.GetCheck() ? true : false;
		}
		else {			
			params.bWhenFalse = m_whenfalse.GetCheck() ? true : false;
			gTriggerSys.Add_Action( sName, params, *pTrigger );			
		}
	}
	
	m_actions.ResetContent();	
	for ( unsigned int i = 0; i != pTrigger->GetActionCount(); ++i ) {
		trigger::Parameter	*pFormat = 0;
		trigger::Params		*pParams = 0;
		gpstring				sAction;
		pTrigger->GetActionInfo( i, sAction, &pFormat, &pParams );					
		m_actions.AddString( sAction.c_str() );		
	}
	
	CDialog::OnOK();
}

void Dialog_Trigger_Action_Properties::OnEditchangeActionCombo() 
{
	m_parameters.clear();
	m_values.clear();	
	m_wndParameters.DeleteAllItems();
	
	int selection = m_actions.GetCurSel();	
	if (( selection != CB_ERR ) && ( gEditorTriggers.GetCurrentActionIndex() == INVALID_INDEX )) {					
		gpstring sName;
		trigger::Parameter *pFormat = 0;
		gTriggerSys.GetActionInfo( selection, sName, &pFormat );		
		
		for ( unsigned int i = 0; i != pFormat->Size(); ++i ) {
			trigger::Parameter::format *pParamFormat = 0;
			pFormat->Get( i, &pParamFormat );																				
		
			HTREEITEM hTreeParent = m_wndParameters.InsertItem(	TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT | TVIF_SELECTEDIMAGE,
																pParamFormat->sName.c_str(), IMAGE_3BOX, IMAGE_3BOX, 0, 0, i, TVI_ROOT, TVI_SORT );		

			m_parameters.push_back( pParamFormat->sName );
			
			gpstring sValue;
			if ( pParamFormat->isFloat ) {
				sValue.assignf( "%f", pParamFormat->f_default );				
				m_values.push_back( sValue );	
			}
			else if ( pParamFormat->isInt ) {
				sValue.assignf( "%d", pParamFormat->i_default );				
				m_values.push_back( sValue );	
			}
			else if ( pParamFormat->isString ) {				
				m_values.push_back( pParamFormat->s_default );	
			}
		}					
	}		
}

void Dialog_Trigger_Action_Properties::OnClickParameterTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CString rText;
	HTREEITEM hSelected = m_wndParameters.GetSelectedItem();
	rText = m_wndParameters.GetItemText( hSelected );
	DWORD data = m_wndParameters.GetItemData( hSelected );

	CString rAction;
	int selection = m_actions.GetCurSel();
	m_actions.GetLBText( selection, rAction );

	gpstring sName;
	trigger::Parameter *pFormat = 0;
	
	if ( selection != CB_ERR ) {					
		gTriggerSys.GetActionInfo( selection, sName, &pFormat );
	}			
					
	// Get the number of the parameter the user has selected
	trigger::Parameter::format *pParamFormat = 0;					
	
	for ( unsigned int k = 0; k != pFormat->Size(); ++k ) {
		pFormat->Get( k, &pParamFormat );
		if ( pParamFormat->sName.same_no_case( rText.GetBuffer( 0 ) )) {
			break;
		}
	}

	gEditorTriggers.GetSelectedParameter()->sName = rText.GetBuffer( 0 );	

	if ( pParamFormat->sValues.size() == 0 ) {							

		gEditorTriggers.GetSelectedParameter()->bHasValues = false;

		// Needs the combo box
		gpstring sValue;
		if ( pParamFormat->isFloat == true ) {
			gEditorTriggers.GetSelectedParameter()->sType = "Floating point";							
			
			if ( !m_values[k].empty() ) {									
				gEditorTriggers.GetSelectedParameter()->sValue = m_values[k];
			}
			else {				
				sValue.assignf( "%f", pParamFormat->f_default );
				gEditorTriggers.GetSelectedParameter()->sValue = sValue;
			}
		}
		else if ( pParamFormat->isInt == true ) {
			gEditorTriggers.GetSelectedParameter()->sType = "Integer";							
			
			if ( !m_values[k].empty() ) {									
				gEditorTriggers.GetSelectedParameter()->sValue = m_values[k];
			}
			else {				
				sValue.assignf( "%d", pParamFormat->i_default );
				gEditorTriggers.GetSelectedParameter()->sValue = sValue;
			}								
		}
		else if ( pParamFormat->isString == true ) {
			gEditorTriggers.GetSelectedParameter()->sType = "String";
			
			if ( !m_values[k].empty() ) {									
				gEditorTriggers.GetSelectedParameter()->sValue = m_values[k];
			}
			else {				
				sValue.assignf( "%s", pParamFormat->s_default.c_str() );
				gEditorTriggers.GetSelectedParameter()->sValue = sValue;
			}														
		}
	}
	else {

		gEditorTriggers.GetSelectedParameter()->bHasValues = true;
		
		if ( pParamFormat->isFloat == true ) {
			gEditorTriggers.GetSelectedParameter()->sType = "Floating point";
		}
		else if ( pParamFormat->isInt == true ) {
			gEditorTriggers.GetSelectedParameter()->sType = "Integer";
		}
		else if ( pParamFormat->isString == true ) {
			gEditorTriggers.GetSelectedParameter()->sType = "String";
		}

		StringVec::iterator j;
		for ( j = pParamFormat->sValues.begin(); j != pParamFormat->sValues.end(); ++j ) {
			gEditorTriggers.GetSelectedParameter()->values.push_back( (*j).c_str() );								
		}

		gEditorTriggers.GetSelectedParameter()->sValue = m_values[k];
	}
	
	Dialog_Trigger_Value dlgValue;
	dlgValue.DoModal();
	
	m_values[k] = gEditorTriggers.GetSelectedParameter()->sValue;
	
	*pResult = 0;
}
