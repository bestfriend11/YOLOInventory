// Dialog_Sfx_Load_Script.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Sfx_Load_Script.h"
#include "EditorSfx.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Load_Script dialog


Dialog_Sfx_Load_Script::Dialog_Sfx_Load_Script(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Sfx_Load_Script::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Sfx_Load_Script)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Sfx_Load_Script::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Sfx_Load_Script)
	DDX_Control(pDX, IDC_COMBO_SCRIPT, m_comboScripts);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Sfx_Load_Script, CDialog)
	//{{AFX_MSG_MAP(Dialog_Sfx_Load_Script)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Load_Script message handlers

BOOL Dialog_Sfx_Load_Script::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hEffects( "world:global:effects" );
	if ( hEffects.IsValid() )
	{
		FuelHandleList hlEffects = hEffects->ListChildBlocks( -1, "", "effect_script*" );
		FuelHandleList::iterator i;
		for ( i = hlEffects.begin(); i != hlEffects.end(); ++i )
		{
			gpstring sName;
			(*i)->Get( "name", sName );

			bool bSiegeEditor = false;
			(*i)->Get( "editor_generated", bSiegeEditor );
			if ( !bSiegeEditor )
			{
				continue;
			}

			m_comboScripts.AddString( sName.c_str() );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Sfx_Load_Script::OnOK() 
{
	int sel = m_comboScripts.GetCurSel();
	if ( sel == CB_ERR )
	{
		return;
	}

	CString rName;
	m_comboScripts.GetLBText( sel, rName );

	gEditorSfx.SetSelectedScript( rName.GetBuffer( 0 ) );	
	
	CDialog::OnOK();
}

void Dialog_Sfx_Load_Script::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
