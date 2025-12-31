// Dialog_Mood_General.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_General.h"
#include "Dialog_Mood_Editor.h"
#include "stringtool.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_General dialog


Dialog_Mood_General::Dialog_Mood_General(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_General::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_General)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Mood_General::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_General)
	DDX_Control(pDX, IDC_EDIT_TRANSITION_TIME, m_TransitionTime);
	DDX_Control(pDX, IDC_EDIT_MOOD_NAME, m_MoodName);
	DDX_Control(pDX, IDC_EDIT_FRUSTUM_WIDTH, m_FrustumWidth);
	DDX_Control(pDX, IDC_EDIT_FRUSTUM_HEIGHT, m_FrustumHeight);
	DDX_Control(pDX, IDC_CHECK_INTERIOR_MOOD, m_InteriorMood);
	DDX_Control(pDX, IDC_CHECK_FRUSTUM_ENABLE, m_FrustumEnable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_General, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_General)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_CHECK_FRUSTUM_ENABLE, OnCheckFrustumEnable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_General message handlers

BOOL Dialog_Mood_General::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		gpstring sName;
		hMood->Get( "mood_name", sName );
		m_MoodName.SetWindowText( sName.c_str() );
		m_sName = sName;

		float transition = 0.0f;
		hMood->Get( "transition_time", transition );
		gpstring sTransition;
		sTransition.assignf( "%f", transition );
		m_TransitionTime.SetWindowText( sTransition.c_str() );

		bool bInterior = false;
		hMood->Get( "interior", bInterior );
		m_InteriorMood.SetCheck( bInterior ? TRUE : FALSE );

		FuelHandle hFrustum = hMood->GetChildBlock( "frustum" );
		if ( hFrustum )
		{
			m_FrustumEnable.SetCheck( TRUE );

			gpstring sTemp;

			float frustumWidth = 0.0f;		
			hFrustum->Get( "frustum_width", frustumWidth );
			sTemp.assignf( "%f", frustumWidth );
			m_FrustumWidth.SetWindowText( sTemp.c_str() );

			float frustumHeight = 0.0f;
			hFrustum->Get( "frustum_height", frustumHeight );
			sTemp.assignf( "%f", frustumHeight );
			m_FrustumHeight.SetWindowText( sTemp.c_str() );			
		}
		else
		{			
			m_FrustumWidth.EnableWindow( FALSE );
			m_FrustumHeight.EnableWindow( FALSE );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Mood_General::OnOK() 
{
	CString rName;
	m_MoodName.GetWindowText( rName );
	if ( rName.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please enter a name for the mood", "Mood Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	if ( gDialogMoodEditor.DoesMoodExist( rName.GetBuffer( 0 ) ) && !m_sName.same_no_case( rName.GetBuffer( 0 ) ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "A mood of this name already exists. Please select a different name.", "Mood Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	FuelHandle hMood  = gDialogMoodEditor.GetSelectedMood();
	if ( !hMood )
	{
		return;
	}

	// Set the mood name
	hMood->Set( "mood_name", rName.GetBuffer( 0 ) );

	// Set the transition time
	CString rTransition;
	m_TransitionTime.GetWindowText( rTransition );
	float transition = 0.0f;
	stringtool::Get( rTransition.GetBuffer( 0 ), transition );
	hMood->Set( "transition_time", transition );

	// Set the interior bool
	bool bInterior = m_InteriorMood.GetCheck() ? true : false;
	hMood->Set( "interior", bInterior );

	if ( m_FrustumEnable.GetCheck() )
	{
		CString rWidth, rHeight;
		m_FrustumWidth.GetWindowText( rWidth );
		m_FrustumHeight.GetWindowText( rHeight );
		float width = 0.0f;
		stringtool::Get( rWidth.GetBuffer( 0 ), width );
		float height = 0.0f;
		stringtool::Get( rHeight.GetBuffer( 0 ), height );		

		FuelHandle hFrustum = hMood->GetChildBlock( "frustum" );
		if ( !hFrustum )
		{
			hFrustum = hMood->CreateChildBlock( "frustum" );
		}

		hFrustum->Set( "frustum_width", width );
		hFrustum->Set( "frustum_height", height );
	}
	else
	{
		FuelHandle hFrustum = hMood->GetChildBlock( "frustum" );
		if ( hFrustum )
		{
			hMood->DestroyChildBlock( hFrustum );
		}
	}

	hMood->GetDB()->SaveChanges();
	
	CDialog::OnOK();
}

void Dialog_Mood_General::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void Dialog_Mood_General::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Mood_General::OnCheckFrustumEnable() 
{
	m_FrustumWidth.EnableWindow( m_FrustumEnable.GetCheck() );
	m_FrustumHeight.EnableWindow( m_FrustumEnable.GetCheck() );	
}
