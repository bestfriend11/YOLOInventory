// Dialog_Trigger_Add_Condition.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Trigger_Add_Condition.h"
#include "Trigger_Sys.h"
#include "Trigger_Conditions.h"
#include "EditorTriggers.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Add_Condition dialog


Dialog_Trigger_Add_Condition::Dialog_Trigger_Add_Condition(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Add_Condition::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Add_Condition)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Add_Condition::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Add_Condition)
	DDX_Control(pDX, IDC_COMBO_CONDITONS, m_conditions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Add_Condition, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Add_Condition)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Add_Condition message handlers

BOOL Dialog_Trigger_Add_Condition::OnInitDialog() 
{
	CDialog::OnInitDialog();

	ModifyStyleEx( 0, WS_EX_TOPMOST, SWP_NOMOVE );
	SetWindowPos( &wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE ); 
	
	trigger::Parameter *pFormat = 0;
	m_conditions.ResetContent();

	// Collect the list of all possible condition names
	const trigger::gpstring_coll& names = gTriggerSys.GetConditionNames();
	for (trigger::gpstring_coll::const_iterator n = names.begin(); n != names.end(); ++n)
	{
		m_conditions.AddString( *n );		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Trigger_Add_Condition::OnOK() 
{
	int sel = m_conditions.GetCurSel();
	if ( sel != CB_ERR )
	{
		GoHandle hObject( gEditorTriggers.GetCurrentGoid() );
	
		trigger::Params params;
		trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
		trigger::Trigger *pTrigger = 0;
		pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
				
		CString rText;
		m_conditions.GetLBText( sel, rText );
		
		gpstring conditionName = rText.GetBuffer( 0 );

		trigger::Condition* pCondition = gTriggerSys.NewCondition(conditionName);

		if (pCondition)
		{
/* Default parameters are already initialized -- biddle

			// Initialize parameters
			trigger::Parameter ParameterList;
			pCondition->FetchParameterList(ParameterList);
			
			trigger::Params condition_params(0);	// Use 0 for now, paramID is set only if condition is added
			condition_params.SubGroup = 0;

			for ( unsigned int i = 0; i != ParameterList.Size(); ++i ) {
				
				trigger::Parameter::format *pParamFormat = 0;
				ParameterList.Get( i, &pParamFormat );																						
				
				gpstring sValue;
				if ( pParamFormat->isFloat ) {
					condition_params.floats.push_back( pParamFormat->f_default );
				}
				else if ( pParamFormat->isInt || pParamFormat->isGoid ) {
					condition_params.ints.push_back( pParamFormat->i_default );
				}
				else if ( pParamFormat->isString ) {				
					condition_params.strings.push_back( pParamFormat->s_default );
				}
			}
*/	
			if (pTrigger->AddCondition( pTrigger->GetNextParamID() , pCondition ))
			{
				pCondition->SetTrigger(pTrigger);
			}
			else
			{
				delete pCondition;
			}
		}
		else
		{
			delete pCondition;
		}


	}
	
	CDialog::OnOK();
}
