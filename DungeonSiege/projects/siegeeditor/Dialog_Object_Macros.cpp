// Dialog_Object_Macros.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Object_Macros.h"
#include "EditorObjects.h"
#include "EditorTerrain.h"
#include "GizmoManager.h"
#include "Dialog_New_Object_Macro.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Macros dialog


Dialog_Object_Macros::Dialog_Object_Macros(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Object_Macros::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Macros)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Object_Macros::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Macros)
	DDX_Control(pDX, IDC_LIST_MACROS, m_macros);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Macros, CDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Macros)
	ON_BN_CLICKED(IDC_BUTTON_DEFINE_MACRO, OnButtonDefineMacro)
	ON_BN_CLICKED(IDC_BUTTON_DELETE_MACRO, OnButtonDeleteMacro)
	ON_BN_CLICKED(IDC_BUTTON_PLACE_MACRO, OnButtonPlaceMacro)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Macros message handlers

BOOL Dialog_Object_Macros::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	Refresh();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Object_Macros::OnButtonDefineMacro() 
{
	Dialog_New_Object_Macro dlg;
	dlg.DoModal();

	Refresh();
}

void Dialog_Object_Macros::OnButtonDeleteMacro() 
{
	int sel = m_macros.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rMacro;
	m_macros.GetText( sel, rMacro );
	
	gEditorObjects.DeleteMacro( rMacro.GetBuffer( 0 ) );

	Refresh();
}

void Dialog_Object_Macros::OnButtonPlaceMacro() 
{
	int sel = m_macros.GetCurSel();
	if ( sel == LB_ERR )
	{
		return;
	}

	CString rMacro;
	m_macros.GetText( sel, rMacro );
	
	gEditorObjects.PlaceMacroInSelectedNode( rMacro.GetBuffer( 0 ) );	
}


void Dialog_Object_Macros::Refresh()
{
	m_macros.ResetContent();

	StringVec macros;
	StringVec::iterator i;
	gEditorObjects.GetMacroNames( macros );
	for ( i = macros.begin(); i != macros.end(); ++i )
	{
		m_macros.AddString( (*i).c_str() );
	}		
}

void Dialog_Object_Macros::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
