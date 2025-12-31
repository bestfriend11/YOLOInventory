// Dialog_Trigger_Add_Action.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Trigger_Add_Action.h"
#include "Trigger_Sys.h"
#include "EditorTriggers.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Add_Action dialog


Dialog_Trigger_Add_Action::Dialog_Trigger_Add_Action(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Add_Action::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Add_Action)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Add_Action::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Add_Action)
	DDX_Control(pDX, IDC_COMBO_ACTIONS, m_actions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Add_Action, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Add_Action)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Add_Action message handlers

BOOL Dialog_Trigger_Add_Action::OnInitDialog() 
{
	CDialog::OnInitDialog();

	ModifyStyleEx( 0, WS_EX_TOPMOST, SWP_NOMOVE );
	SetWindowPos( &wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ); 
		
	trigger::Parameter *pFormat = 0;
	m_actions.ResetContent();

	const trigger::gpstring_coll& namelist = gTriggerSys.GetActionNames();
	for ( trigger::gpstring_coll::const_iterator i = namelist.begin(); i != namelist.end(); ++i ) 
	{
		m_actions.AddString( (*i).c_str() );		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Trigger_Add_Action::OnOK() 
{
	int sel = m_actions.GetCurSel();
	if ( sel != CB_ERR )
	{
		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	
		trigger::Params params;
		trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
		trigger::Trigger *pTrigger = 0;
		pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
				
		CString rText;
		m_actions.GetLBText( sel, rText );

		gpstring sAction = rText.GetBuffer( 0 );

		trigger::Parameter *pFormat = gTriggerSys.GetActionFormat( sAction );

		trigger::Params action_params(pTrigger->GetNextParamID());		

		for ( unsigned int i = 0; i != pFormat->Size(); ++i )
		{
			trigger::Parameter::format *pParamFormat = 0;
			pFormat->Get( i, &pParamFormat );																						
			
			gpstring sValue;
			if ( pParamFormat->isFloat )
			{
				action_params.floats.push_back( pParamFormat->f_default );
			}
			else if ( pParamFormat->isInt || pParamFormat->isGoid )
			{
				action_params.ints.push_back( pParamFormat->i_default );
			}
			else if ( pParamFormat->isString )
			{				
				action_params.strings.push_back( pParamFormat->s_default );
			}
		}

		params.Delay = 0;
		params.SubGroup = 0;

		trigger::Action* pAction = gTriggerSys.NewAction(rText.GetBuffer( 0 ));
		pTrigger->AddAction(action_params.ParamID,pAction,action_params);
		pTrigger->UpdateTriggerFlags();

		{
			gpstring errmsg;
			errmsg.assignf("Adding action has created an invalid trigger");
			if (!pTrigger->Validate(errmsg))
			{
				gperrorf((errmsg.c_str()));
			}
		}
	}
	
	CDialog::OnOK();
}