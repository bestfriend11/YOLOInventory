// Dialog_Decal_Preferences.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "siegeeditor.h"
#include "Dialog_Decal_Preferences.h"
#include "Preferences.h"
#include "stringtool.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Decal_Preferences dialog


Dialog_Decal_Preferences::Dialog_Decal_Preferences(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Decal_Preferences::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Decal_Preferences)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Decal_Preferences::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Decal_Preferences)
	DDX_Control(pDX, IDC_EDIT_VERTICAL, m_vertical);
	DDX_Control(pDX, IDC_EDIT_HORIZONTAL, m_horizontal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Decal_Preferences, CDialog)
	//{{AFX_MSG_MAP(Dialog_Decal_Preferences)
	ON_EN_CHANGE(IDC_EDIT_HORIZONTAL, OnChangeEditHorizontal)
	ON_EN_CHANGE(IDC_EDIT_VERTICAL, OnChangeEditVertical)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Decal_Preferences message handlers

BOOL Dialog_Decal_Preferences::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gpstring sTemp;
	sTemp.assignf( "%f", gPreferences.GetDecalHorizontal() );
	m_horizontal.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetDecalVertical() );
	m_vertical.SetWindowText( sTemp.c_str() );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Decal_Preferences::OnOK() 
{
	CString rHoriz;
	CString rVert;

	m_horizontal.GetWindowText( rHoriz );
	m_vertical.GetWindowText( rVert );

	float horiz = 0.0f;
	float vert = 0.0f;
	stringtool::Get( rHoriz.GetBuffer( 0 ), horiz );
	stringtool::Get( rVert.GetBuffer( 0 ), vert );	

	gPreferences.SetDecalHorizontal( horiz );
	gPreferences.SetDecalVertical( vert );
	
	CDialog::OnOK();
}

void Dialog_Decal_Preferences::OnChangeEditHorizontal() 
{
	CString rHoriz;
	m_horizontal.GetWindowText( rHoriz );
	float horiz = 0.0f;
	if ( rHoriz.IsEmpty() == false )
	{
		stringtool::Get( rHoriz.GetBuffer( 0 ), horiz );
		gPreferences.SetDecalHorizontal( horiz );		
	}
}

void Dialog_Decal_Preferences::OnChangeEditVertical() 
{
	CString rVert;
	m_vertical.GetWindowText( rVert );
	float vert = 0.0f;	
	if ( rVert.IsEmpty() == false )
	{
		stringtool::Get( rVert.GetBuffer( 0 ), vert );	
		gPreferences.SetDecalVertical( vert );
	}
}

void Dialog_Decal_Preferences::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
