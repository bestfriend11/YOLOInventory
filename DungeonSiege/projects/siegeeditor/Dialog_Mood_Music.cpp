// Dialog_Mood_Music.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_Music.h"
#include "Dialog_Mood_Editor.h"
#include "Dialog_Resource_Tree.h"
#include "Mood.h"
#include "Preferences.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Music dialog


Dialog_Mood_Music::Dialog_Mood_Music(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_Music::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_Music)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Mood_Music::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_Music)
	DDX_Control(pDX, IDC_EDIT_STANDARD_SOUND, m_StandardSound);
	DDX_Control(pDX, IDC_EDIT_STANDARD_REPEAT_DELAY, m_StandardRepeatDelay);
	DDX_Control(pDX, IDC_EDIT_STANDARD_INTRO_DELAY, m_StandardIntroDelay);
	DDX_Control(pDX, IDC_EDIT_BATTLE_SOUND, m_BattleSound);
	DDX_Control(pDX, IDC_EDIT_BATTLE_REPEAT_DELAY, m_BattleRepeatDelay);
	DDX_Control(pDX, IDC_EDIT_BATTLE_INTRO_DELAY, m_BattleIntroDelay);
	DDX_Control(pDX, IDC_EDIT_AMBIENT_SOUND, m_AmbientSound);
	DDX_Control(pDX, IDC_EDIT_AMBIENT_REPEAT_DELAY, m_AmbientRepeatDelay);
	DDX_Control(pDX, IDC_EDIT_AMBIENT_INTRO_DELAY, m_AmbientIntroDelay);
	DDX_Control(pDX, IDC_COMBO_ROOM_TYPE, m_RoomType);
	DDX_Control(pDX, IDC_CHECK_MUSIC_ENABLE, m_MusicEnable);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_Music, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_Music)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_AMBIENT_FILES, OnButtonAmbientFiles)
	ON_BN_CLICKED(IDC_BUTTON_STANDARD_SOUND_FILES, OnButtonStandardSoundFiles)
	ON_BN_CLICKED(IDC_BUTTON_BATTLE_SOUND_FILES, OnButtonBattleSoundFiles)
	ON_BN_CLICKED(IDC_CHECK_MUSIC_ENABLE, OnCheckMusicEnable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Music message handlers

BOOL Dialog_Mood_Music::OnInitDialog() 
{
	CDialog::OnInitDialog();

	for ( int i = (int)RT_BEGIN; i != (int)RT_END; ++i )
	{
		eMoodRoomType type = (eMoodRoomType)i;
		m_RoomType.AddString( ::ToString( type ) );
	}
	
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		FuelHandle hMusic = hMood->GetChildBlock( "music" );
		if ( hMusic )
		{
			m_MusicEnable.SetCheck( TRUE );

			gpstring sTemp;
			float fTemp = 0.0f;

			hMusic->Get( "ambient_track", sTemp );
			m_AmbientSound.SetWindowText( sTemp.c_str() );
			
			hMusic->Get( "ambient_intro_delay", fTemp );
			sTemp.assignf( "%f", fTemp );
			m_AmbientIntroDelay.SetWindowText( sTemp.c_str() );

			hMusic->Get( "ambient_repeat_delay", fTemp );
			sTemp.assignf( "%f", fTemp );
			m_AmbientRepeatDelay.SetWindowText( sTemp.c_str() );

			hMusic->Get( "standard_track", sTemp );
			m_StandardSound.SetWindowText( sTemp.c_str() );
			
			hMusic->Get( "standard_intro_delay", fTemp );
			sTemp.assignf( "%f", fTemp );
			m_StandardIntroDelay.SetWindowText( sTemp.c_str() );

			hMusic->Get( "standard_repeat_delay", fTemp );
			sTemp.assignf( "%f", fTemp );
			m_StandardRepeatDelay.SetWindowText( sTemp.c_str() );

			hMusic->Get( "battle_track", sTemp );
			m_BattleSound.SetWindowText( sTemp.c_str() );
			
			hMusic->Get( "battle_intro_delay", fTemp );
			sTemp.assignf( "%f", fTemp );
			m_BattleIntroDelay.SetWindowText( sTemp.c_str() );

			hMusic->Get( "battle_repeat_delay", fTemp );
			sTemp.assignf( "%f", fTemp );
			m_BattleRepeatDelay.SetWindowText( sTemp.c_str() );

			hMusic->Get( "room_type", sTemp );
			int sel = m_RoomType.FindString( 0, sTemp.c_str() );
			m_RoomType.SetCurSel( sel );
		}
		else
		{
			m_StandardSound.EnableWindow( FALSE );
			m_StandardRepeatDelay.EnableWindow( FALSE );
			m_StandardIntroDelay.EnableWindow( FALSE );

			m_AmbientSound.EnableWindow( FALSE );
			m_AmbientRepeatDelay.EnableWindow( FALSE );
			m_AmbientIntroDelay.EnableWindow( FALSE );

			m_BattleSound.EnableWindow( FALSE );
			m_BattleRepeatDelay.EnableWindow( FALSE );
			m_BattleIntroDelay.EnableWindow( FALSE );

			m_RoomType.EnableWindow( FALSE );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Mood_Music::OnOK() 
{
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		FuelHandle hMusic = hMood->GetChildBlock( "music" );
		if ( m_MusicEnable.GetCheck() )
		{
			if ( !hMusic )
			{
				hMusic = hMood->CreateChildBlock( "music" );
			}

			if ( hMusic )
			{
				CString rTemp;
				float fTemp = 0.0f;

				m_AmbientSound.GetWindowText( rTemp );
				hMusic->Set( "ambient_track", rTemp.GetBuffer( 0 ) );

				m_AmbientIntroDelay.GetWindowText( rTemp );
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );
				hMusic->Set( "ambient_intro_delay", fTemp );

				m_AmbientRepeatDelay.GetWindowText( rTemp );
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );
				hMusic->Set( "ambient_repeat_delay", fTemp );

				m_StandardSound.GetWindowText( rTemp );
				hMusic->Set( "standard_track", rTemp.GetBuffer( 0 ) );

				m_StandardIntroDelay.GetWindowText( rTemp );
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );
				hMusic->Set( "standard_intro_delay", fTemp );

				m_StandardRepeatDelay.GetWindowText( rTemp );
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );
				hMusic->Set( "standard_repeat_delay", fTemp );

				m_BattleSound.GetWindowText( rTemp );
				hMusic->Set( "battle_track", rTemp.GetBuffer( 0 ) );

				m_BattleIntroDelay.GetWindowText( rTemp );
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );
				hMusic->Set( "battle_intro_delay", fTemp );

				m_BattleRepeatDelay.GetWindowText( rTemp );
				stringtool::Get( rTemp.GetBuffer( 0 ), fTemp );
				hMusic->Set( "battle_repeat_delay", fTemp );

				CString rType;
				m_RoomType.GetWindowText( rType );
				hMusic->Set( "room_type", rType.GetBuffer( 0 ) );
			}
		}
		else
		{
			if ( hMusic )
			{
				hMood->DestroyChildBlock( hMusic );
			}
		}

		hMood->GetDB()->SaveChanges();		
	}
	
	
	CDialog::OnOK();
}

void Dialog_Mood_Music::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void Dialog_Mood_Music::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Mood_Music::OnButtonAmbientFiles() 
{
	gPreferences.SetResourcePath( "sound/*.wav,sound/*.mp3" );

	Dialog_Resource_Tree dlg;
	if ( dlg.DoModal() == IDCANCEL )
	{
		return;
	}

	m_AmbientSound.SetWindowText( gPreferences.GetSelectedResource().c_str() );
}

void Dialog_Mood_Music::OnButtonStandardSoundFiles() 
{
	gPreferences.SetResourcePath( "sound/*.wav,sound/*.mp3" );

	Dialog_Resource_Tree dlg;
	if ( dlg.DoModal() == IDCANCEL )
	{
		return;
	}

	m_StandardSound.SetWindowText( gPreferences.GetSelectedResource().c_str() );
}

void Dialog_Mood_Music::OnButtonBattleSoundFiles() 
{
	gPreferences.SetResourcePath( "sound/*.wav,sound/*.mp3" );

	Dialog_Resource_Tree dlg;
	if ( dlg.DoModal() == IDCANCEL )
	{
		return;
	}

	m_BattleSound.SetWindowText( gPreferences.GetSelectedResource().c_str() );
}

void Dialog_Mood_Music::OnCheckMusicEnable() 
{
	m_StandardSound.EnableWindow( m_MusicEnable.GetCheck() );
	m_StandardRepeatDelay.EnableWindow( m_MusicEnable.GetCheck() );
	m_StandardIntroDelay.EnableWindow( m_MusicEnable.GetCheck() );

	m_AmbientSound.EnableWindow( m_MusicEnable.GetCheck() );
	m_AmbientRepeatDelay.EnableWindow( m_MusicEnable.GetCheck() );
	m_AmbientIntroDelay.EnableWindow( m_MusicEnable.GetCheck() );

	m_BattleSound.EnableWindow( m_MusicEnable.GetCheck() );
	m_BattleRepeatDelay.EnableWindow( m_MusicEnable.GetCheck() );
	m_BattleIntroDelay.EnableWindow( m_MusicEnable.GetCheck() );

	m_RoomType.EnableWindow( m_MusicEnable.GetCheck() );
}
