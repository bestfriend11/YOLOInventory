// Dialog_Hide_Components.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Hide_Components.h"
#include "EditorObjects.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hide_Components dialog


Dialog_Hide_Components::Dialog_Hide_Components(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Hide_Components::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Hide_Components)
	//}}AFX_DATA_INIT
}


void Dialog_Hide_Components::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Hide_Components)
	DDX_Control(pDX, IDC_EDIT_COMPONENT, m_componentName);
	DDX_Control(pDX, IDC_LIST_COMPONENTS, m_components);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Hide_Components, CDialog)
	//{{AFX_MSG_MAP(Dialog_Hide_Components)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Hide_Components message handlers

void Dialog_Hide_Components::OnButtonRemove() 
{
	if ( m_components.GetCurSel() == LB_ERR )
	{
		return;
	}

	CString rComponent;
	m_components.GetText( m_components.GetCurSel(), rComponent );
	gEditorObjects.RemoveHiddenComponent( rComponent.GetBuffer( 0 ) );
	
	RefreshHiddenComponents();
}

void Dialog_Hide_Components::OnButtonAdd() 
{
	CString rComponent;
	m_componentName.GetWindowText( rComponent );
	if ( rComponent.IsEmpty() == false )
	{
		gEditorObjects.AddHiddenComponent( rComponent.GetBuffer( 0 ) );
	}

	RefreshHiddenComponents();
}

BOOL Dialog_Hide_Components::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	RefreshHiddenComponents();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Hide_Components::RefreshHiddenComponents()
{
	m_components.ResetContent();
	StringVec hiddenComponents = gEditorObjects.GetHiddenComponents();
	StringVec::iterator i;
	for ( i = hiddenComponents.begin(); i != hiddenComponents.end(); ++i )
	{
		m_components.AddString( (*i).c_str() );
	}
}
void Dialog_Hide_Components::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
