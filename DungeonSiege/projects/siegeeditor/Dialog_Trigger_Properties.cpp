// Dialog_Trigger_Properties.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "Trigger_Sys.h"
#include "SiegeEditor.h"
#include "Dialog_Trigger_Properties.h"
#include "EditorTriggers.h"
#include "SiegeEditorShell.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Properties dialog


Dialog_Trigger_Properties::Dialog_Trigger_Properties(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Trigger_Properties::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Trigger_Properties)
	//}}AFX_DATA_INIT
}


void Dialog_Trigger_Properties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Trigger_Properties)
	DDX_Control(pDX, IDC_ONESHOT, m_oneShot);
	DDX_Control(pDX, IDC_RESET_DELAY, m_resetDelay);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Trigger_Properties, CDialog)
	//{{AFX_MSG_MAP(Dialog_Trigger_Properties)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Trigger_Properties message handlers

BOOL Dialog_Trigger_Properties::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GOHandle hObject( gEditorTriggers.GetCurrentGOGUID() );
	trigger::Storage	*pTriggerStorage = hObject->GetChildTriggerStorage();
	trigger::Trigger *pTrigger = 0;
	pTriggerStorage->Get( gEditorTriggers.GetCurrentTriggerIndex(), &pTrigger );
	gpstring sReset;
	sReset.assignf( "%f", pTrigger->GetResetDuration() );
	m_resetDelay.SetWindowText( sReset );	
	m_oneShot.SetCheck( (int)pTrigger->IsSingleShot() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
