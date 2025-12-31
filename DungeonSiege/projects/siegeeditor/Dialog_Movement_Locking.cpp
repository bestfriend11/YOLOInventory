// Dialog_Movement_Locking.cpp : implementation file
//

#include "PrecompEditor.h"
#include "Preferences.h"
#include "siegeeditor.h"
#include "Dialog_Movement_Locking.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Movement_Locking dialog


Dialog_Movement_Locking::Dialog_Movement_Locking(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Movement_Locking::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Movement_Locking)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Movement_Locking::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Movement_Locking)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Movement_Locking, CDialog)
	//{{AFX_MSG_MAP(Dialog_Movement_Locking)
	ON_BN_CLICKED(IDC_RADIO_X, OnRadioX)
	ON_BN_CLICKED(IDC_RADIO_Y, OnRadioY)
	ON_BN_CLICKED(IDC_RADIO_Z, OnRadioZ)
	ON_BN_CLICKED(IDC_RADIO_NONE, OnRadioNone)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Movement_Locking message handlers

void Dialog_Movement_Locking::OnRadioX() 
{
	gPreferences.SetMoveLockMode( MOVE_LOCK_X );	
}

void Dialog_Movement_Locking::OnRadioY() 
{
	gPreferences.SetMoveLockMode( MOVE_LOCK_Y );	
}

void Dialog_Movement_Locking::OnRadioZ() 
{
	gPreferences.SetMoveLockMode( MOVE_LOCK_Z );	
}

BOOL Dialog_Movement_Locking::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	switch ( gPreferences.GetMoveLockMode() )
	{
	case MOVE_LOCK_X:
		((CButton *)(GetDlgItem(IDC_RADIO_X)))->SetCheck( true );
		break;
	case MOVE_LOCK_Y:
		((CButton *)(GetDlgItem(IDC_RADIO_Y)))->SetCheck( true );
		break;
	case MOVE_LOCK_Z:
		((CButton *)(GetDlgItem(IDC_RADIO_Z)))->SetCheck( true );
		break;
	default:
		((CButton *)(GetDlgItem(IDC_RADIO_NONE)))->SetCheck( true );
		break;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Movement_Locking::OnRadioNone() 
{
	gPreferences.SetMoveLockMode( MOVE_LOCK_NONE );		
}

void Dialog_Movement_Locking::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
