// Dialog_Trigger_Value.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "SiegeEditor.h"
#include "Dialog_Trigger_Value.h"
#include "Trigger_Sys.h"
#include "EditorTriggers.h"
#include "SiegeEditorShell.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Value dialog


Dialog_Trigger_Value::Dialog_Trigger_Value(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Value::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Value)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Value::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Value)
	DDX_Control(pDX, IDC_STATIC_TYPE, m_type);
	DDX_Control(pDX, IDC_STATIC_NAME, m_name);
	DDX_Control(pDX, IDC_EDIT_PARAMETER, m_editParameter);
	DDX_Control(pDX, IDC_COMBO_PARAMETER, m_comboParameter);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Value, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Value)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Value message handlers

void Dialog_Trigger_Value::OnOK() 
{
	if ( gEditorTriggers.GetSelectedParameter()->bHasValues ) {
		int sel = m_comboParameter.GetCurSel();
		if ( sel != CB_ERR )
		{
			CString rText;
			m_comboParameter.GetLBText( sel, rText );		
			gEditorTriggers.GetSelectedParameter()->sValue = rText.GetBuffer( 0 );
		}
	}
	else {
		CString rText;
		m_editParameter.GetWindowText( rText );
		gEditorTriggers.GetSelectedParameter()->sValue = rText.GetBuffer( 0 );
	}
	
	CDialog::OnOK();
}

BOOL Dialog_Trigger_Value::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_name.SetWindowText( gEditorTriggers.GetSelectedParameter()->sName.c_str() );
	m_type.SetWindowText( gEditorTriggers.GetSelectedParameter()->sType.c_str() );

	if ( gEditorTriggers.GetSelectedParameter()->bHasValues )
	{	
		m_comboParameter.ResetContent();		
		StringVec values = gEditorTriggers.GetSelectedParameter()->values;
		StringVec::iterator i;
		for (	i = values.begin();	i != values.end(); ++i ) {			
			m_comboParameter.AddString( (*i).c_str() );
		}		

		m_comboParameter.ShowWindow( SW_SHOW );
		m_editParameter.ShowWindow( SW_HIDE );

		int sel = m_comboParameter.FindString( 0, gEditorTriggers.GetSelectedParameter()->sValue.c_str() );
		m_comboParameter.SetCurSel( sel );		
	}
	else {
		m_comboParameter.ShowWindow( SW_HIDE );
		m_editParameter.ShowWindow( SW_SHOW );
		m_editParameter.SetWindowText( gEditorTriggers.GetSelectedParameter()->sValue.c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
