// Dialog_Sfx_New_Script.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Sfx_New_Script.h"
#include "EditorSfx.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_New_Script dialog


Dialog_Sfx_New_Script::Dialog_Sfx_New_Script(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Sfx_New_Script::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Sfx_New_Script)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Sfx_New_Script::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Sfx_New_Script)
	DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Sfx_New_Script, CDialog)
	//{{AFX_MSG_MAP(Dialog_Sfx_New_Script)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_New_Script message handlers

void Dialog_Sfx_New_Script::OnOK() 
{
	CString rName;
	m_editName.GetWindowText( rName );

	FuelHandle hEffects( "world:global:effects" );
	if ( hEffects.IsValid() )
	{
		FuelHandleList::iterator i;
		FuelHandleList hlEffects = hEffects->ListChildBlocks( -1, "", "effect_script*" );
		for ( i = hlEffects.begin(); i != hlEffects.end(); ++i )
		{
			gpstring sEffect;
			(*i)->Get( "name", sEffect );
			if ( sEffect.same_no_case( rName.GetBuffer( 0 ) ) )
			{
				MessageBox( "An effect with this name already exists, please select another name.", "Script Already Exists", MB_OK );
				return;
			}
		}
	}

	FuelHandle hNew = hEffects->CreateChildBlock( "effect_script*", rName.GetBuffer( 0 ) );
	hNew->Set( "name", rName.GetBuffer( 0 ) );
	gEditorSfx.SetSelectedScript( rName.GetBuffer( 0 ) );		

	CDialog::OnOK();
}

BOOL Dialog_Sfx_New_Script::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Sfx_New_Script::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
