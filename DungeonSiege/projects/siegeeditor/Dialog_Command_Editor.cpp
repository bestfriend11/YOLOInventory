// Dialog_Command_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Command_Editor.h"
#include "EditorObjects.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Command_Editor dialog


Dialog_Command_Editor::Dialog_Command_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Command_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Command_Editor)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Command_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Command_Editor)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Command_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Command_Editor)
	ON_BN_CLICKED(IDC_RADIO_SELECT, OnRadioSelect)
	ON_BN_CLICKED(IDC_RADIO_LINK, OnRadioLink)
	ON_BN_CLICKED(IDC_RADIO_MOVE, OnRadioMove)
	ON_BN_CLICKED(IDC_RADIO_PATROL, OnRadioPatrol)
	ON_BN_CLICKED(IDC_RADIO_STOP, OnRadioStop)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Command_Editor message handlers

BOOL Dialog_Command_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gEditorObjects.SetSequenceEditing( true );
	gEditorObjects.SetSequenceType( SEQUENCE_SELECT );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Command_Editor::OnRadioSelect() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_SELECT );	
}

void Dialog_Command_Editor::OnRadioLink() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_LINK );
	gPreferences.SetLinkMode( true );
}

void Dialog_Command_Editor::OnRadioMove() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_MOVE );	
}

void Dialog_Command_Editor::OnRadioPatrol() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_PATROL );	
}

void Dialog_Command_Editor::OnRadioStop() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_STOP );	
}

/*  $$$ remove once we're sure we don't need it -cq
void Dialog_Command_Editor::OnRadioFollow() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_FOLLOW );	
}

void Dialog_Command_Editor::OnRadioGuard() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_GUARD );	
}

void Dialog_Command_Editor::OnRadioAttack() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_ATTACK );
	
}

void Dialog_Command_Editor::OnRadioGet() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_GET );	
}

void Dialog_Command_Editor::OnRadioDrop() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_DROP );	
}

void Dialog_Command_Editor::OnRadioUse() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_USE );	
}

void Dialog_Command_Editor::OnRadioEquip() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_EQUIP );	
}

void Dialog_Command_Editor::OnRadioGive() 
{
	gEditorObjects.SetSequenceType( SEQUENCE_GIVE );	
}
*/

BOOL Dialog_Command_Editor::DestroyWindow() 
{
	gEditorObjects.SetSequenceEditing( false );
	gEditorObjects.SetSequenceType( SEQUENCE_SELECT );
	
	return CDialog::DestroyWindow();
}

void Dialog_Command_Editor::OnDestroy() 
{
	CDialog::OnDestroy();
	
	gEditorObjects.SetSequenceEditing( false );
	gEditorObjects.SetSequenceType( SEQUENCE_SELECT );	
}

void Dialog_Command_Editor::OnClose() 
{
	gEditorObjects.SetSequenceEditing( false );
	gEditorObjects.SetSequenceType( SEQUENCE_SELECT );	
	
	CDialog::OnClose();
}

void Dialog_Command_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
