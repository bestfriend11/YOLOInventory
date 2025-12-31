// Dialog_Mood_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_Editor.h"
#include "ImageListDefines.h"
#include "Dialog_Mood_Add.h"
#include "Dialog_Mood_Add_Folder.h"
#include "Mood.h"
#include "Preferences.h"
#include "Dialog_Mood_General.h"
#include "Dialog_Mood_Fog.h"
#include "Dialog_Mood_Music.h"
#include "Dialog_Mood_Weather.h"
#include "Config.h"
#include "Services.h"
#include "FileSysUtils.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Editor dialog


Dialog_Mood_Editor::Dialog_Mood_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_Editor)
	//}}AFX_DATA_INIT
}


void Dialog_Mood_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_Editor)
	DDX_Control(pDX, IDC_TREE_MOODS_TANKED, m_Tanked);
	DDX_Control(pDX, IDC_TREE_MOODS, m_Moods);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_Editor)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_PROPERTIES, OnButtonEditProperties)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_WEATHER, OnButtonEditWeather)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_FOG, OnButtonEditFog)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_MUSIC, OnButtonEditMusic)
	ON_BN_CLICKED(IDC_BUTTON_TEST, OnButtonTest)
	ON_BN_CLICKED(IDC_BUTTON_ADD_FOLDER, OnButtonAddFolder)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStop)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_TEST_AMBIENT, OnButtonTestAmbient)
	ON_BN_CLICKED(IDC_BUTTON_TEST_BATTLE, OnButtonTestBattle)
	ON_BN_CLICKED(IDC_BUTTON_TEST_STANDARD, OnButtonTestStandard)
	ON_NOTIFY(NM_CLICK, IDC_TREE_MOODS, OnClickTreeMoods)
	ON_NOTIFY(NM_CLICK, IDC_TREE_MOODS_TANKED, OnClickTreeMoodsTanked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Editor message handlers

void Dialog_Mood_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Mood_Editor::OnButtonAdd() 
{
	gpstring sFolder;
	int nImage = 0;
	int nSelectedImage = 0;
	m_Moods.GetItemImage( m_Moods.GetSelectedItem(), nImage, nSelectedImage );
	if ( nImage == IMAGE_CLOSEFOLDER )
	{
		CString rFolder = m_Moods.GetItemText( m_Moods.GetSelectedItem() );
		sFolder = rFolder.GetBuffer( 0 );
	}
	else if ( nImage == IMAGE_LTBLUEBOX )
	{
		CString rFolder = m_Moods.GetItemText( m_Moods.GetParentItem( m_Moods.GetSelectedItem() ) );
		sFolder = rFolder.GetBuffer( 0 );
	}

	if ( sFolder.empty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a folder to add the mood to.", "Add Mood", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	SetSelectedFolder( sFolder );

	Dialog_Mood_Add dlg;
	dlg.DoModal();

	SaveMoods();

	Refresh();
}

void Dialog_Mood_Editor::OnButtonRemove() 
{
	gpstring sFolder;
	int nImage = 0;
	int nSelectedImage = 0;
	m_Moods.GetItemImage( m_Moods.GetSelectedItem(), nImage, nSelectedImage );
	if ( nImage == IMAGE_LTBLUEBOX )
	{
		CString rFolder = m_Moods.GetItemText( m_Moods.GetParentItem( m_Moods.GetSelectedItem() ) );
		sFolder = rFolder.GetBuffer( 0 );
	}
	else
	{
		return;
	}

	CString rName = m_Moods.GetItemText( m_Moods.GetSelectedItem() );

	gpstring sMood;
	if ( !sFolder.empty() )
	{
		sMood.assignf( "::moods:%s", sFolder.c_str() );
	}

	FuelHandle hMoodDir( sMood );
	if ( hMoodDir )
	{
		FuelHandleList hlMoods = hMoodDir->ListChildBlocks( 1, "", "mood_setting*" );
		FuelHandleList::iterator iMood;
		for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
		{
			gpstring sName;
			(*iMood)->Get( "mood_name", sName );
			if ( sName.same_no_case( rName.GetBuffer( 0 ) ) )
			{
				hMoodDir->DestroyChildBlock( *iMood );
				return;
			}
		}
	}	

	Refresh();
}

FuelHandle & Dialog_Mood_Editor::GetSelectedMood()
{	
	gpstring sFolder;
	int nImage = 0;
	int nSelectedImage = 0;
	m_Moods.GetItemImage( m_Moods.GetSelectedItem(), nImage, nSelectedImage );
	if ( nImage == IMAGE_LTBLUEBOX )
	{
		CString rFolder = m_Moods.GetItemText( m_Moods.GetParentItem( m_Moods.GetSelectedItem() ) );
		CString rName = m_Moods.GetItemText( m_Moods.GetSelectedItem() );
		gpstring sDir;

		bool bFound = false;
		{
			sDir.assignf( "::moods:%s", rFolder.GetBuffer( 0 ) );
			FuelHandle hMoodDir( sDir );		
			if ( hMoodDir )
			{
				FuelHandleList hlMoods = hMoodDir->ListChildBlocks( 1, "", "mood_setting*" );
				FuelHandleList::iterator iMood;
				for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
				{
					gpstring sName;
					(*iMood)->Get( "mood_name", sName );
					if ( sName.same_no_case( rName.GetBuffer( 0 ) ) )
					{
						m_SelectedMood = *iMood;
						bFound = true;
						break;
					}
				}		
			}
		}

		if ( !bFound )
		{
			sDir.assignf( "::moods_home:%s", rFolder.GetBuffer( 0 ) );
			FuelHandle hMoodDir( sDir );
			if ( hMoodDir )
			{
				FuelHandleList hlMoods = hMoodDir->ListChildBlocks( 1, "", "mood_setting*" );
				FuelHandleList::iterator iMood;
				for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
				{
					gpstring sName;
					(*iMood)->Get( "mood_name", sName );
					if ( sName.same_no_case( rName.GetBuffer( 0 ) ) )
					{
						m_SelectedMood = *iMood;
						break;
					}
				}		
			}
		}
	}
	else
	{
		m_SelectedMood = FuelHandle();
	}

	return m_SelectedMood;
}

void Dialog_Mood_Editor::OnButtonEditProperties() 
{
	FuelHandle hMood( gDialogMoodEditor.GetSelectedMood() );
	if ( hMood )
	{
		Dialog_Mood_General dlg;
		dlg.DoModal();
	}	
	else
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a mood to edit.", "Mood Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}
}

void Dialog_Mood_Editor::OnButtonEditWeather() 
{
	FuelHandle hMood( gDialogMoodEditor.GetSelectedMood() );
	if ( hMood )
	{
		Dialog_Mood_Weather dlg;
		dlg.DoModal();	
	}
	else
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a mood to edit.", "Mood Weather", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}
}

void Dialog_Mood_Editor::OnButtonEditFog() 
{
	FuelHandle hMood( gDialogMoodEditor.GetSelectedMood() );
	if ( hMood )
	{
		Dialog_Mood_Fog dlg;
		dlg.DoModal();	
	}	
	else
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a mood to edit.", "Mood Fog", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}
}

void Dialog_Mood_Editor::OnButtonEditMusic() 
{
	FuelHandle hMood( gDialogMoodEditor.GetSelectedMood() );
	if ( hMood )
	{
		Dialog_Mood_Music dlg;
		dlg.DoModal();	
	}	
	else
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a mood to edit.", "Mood Music", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}
}

BOOL Dialog_Mood_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_Moods.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );	
	m_Tanked.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );	
	
	gpstring outPath = gConfig.ResolvePathVars( "%out%", gServices.IsEditor() ? Config::PVM_ERROR_ON_FAIL : Config::PVM_NONE );	
	outPath.appendf( "\\world\\global\\moods" );

	gpstring homePath = gConfig.ResolvePathVars( "%home%", gServices.IsEditor() ? Config::PVM_ERROR_ON_FAIL : Config::PVM_NONE );	
	homePath.appendf( "\\world\\global\\moods" );

	if ( !FileSys::DoesPathExist( outPath.c_str() ) )
	{
		FileSys::CreateFullDirectory( outPath.c_str() );
	}	

	FuelDB * pMoodDb = gFuelSys.AddTextDb( "moods" );
	pMoodDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, outPath, outPath );

	FuelDB * pHomeMoodDb = gFuelSys.AddTextDb( "moods_home" );
	pHomeMoodDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, homePath, outPath );

	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Mood_Editor::OnButtonTest() 
{
	SaveMoods();
	
	gpstring sFolder;
	int nImage = 0;
	int nSelectedImage = 0;
	
	bool bTested = false;
	if ( m_Moods.GetSelectedItem() )
	{
		m_Moods.GetItemImage( m_Moods.GetSelectedItem(), nImage, nSelectedImage );
		if ( nImage == IMAGE_LTBLUEBOX )
		{
			CString rMood = m_Moods.GetItemText( m_Moods.GetSelectedItem() );
			gMood.InitMoods( true );
			gMood.AddMood( GetSelectedMood() );			

			gMood.SetEnabled( true );
			gMood.SetMood( rMood.GetBuffer( 0 ), 0.0f );				
			gPreferences.SetUpdateWeather( true );
			bTested = true;
		}
	}
	else if ( m_Tanked.GetSelectedItem() )
	{
		m_Tanked.GetItemImage( m_Tanked.GetSelectedItem(), nImage, nSelectedImage );
		if ( nImage == IMAGE_LTBLUEBOX )
		{
			CString rMood = m_Tanked.GetItemText( m_Tanked.GetSelectedItem() );
			gMood.InitMoods( true );			
			gMood.AddMood( GetSelectedMood() );			

			gMood.SetEnabled( true );
			gMood.SetMood( rMood.GetBuffer( 0 ), 0.0f );				
			gPreferences.SetUpdateWeather( true );
			bTested = true;
		}
	}
	
	if ( !bTested )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a mood to test.", "Test Mood", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}		
}


void Dialog_Mood_Editor::Refresh()
{
	m_Moods.DeleteAllItems();

	StringSet moodSet;
	StringSet dirSet;

	FuelHandle hMoods( "::moods:root" );
	if ( hMoods )
	{
		FuelHandleList hlMoods = hMoods->ListChildBlocks( 1 );
		FuelHandleList::iterator iMood;
		for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
		{
			if ( (*iMood)->IsDirectory() )
			{
				HTREEITEM hParent = m_Moods.InsertItem( (*iMood)->GetName(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER );				
				dirSet.insert( (*iMood)->GetName() );

				FuelHandleList hlChildMoods = (*iMood)->ListChildBlocks( -1, "", "mood_setting*" );
				FuelHandleList::iterator iChild;
				for ( iChild = hlChildMoods.begin(); iChild != hlChildMoods.end(); ++iChild )
				{
					gpstring sMood;
					(*iChild)->Get( "mood_name", sMood );
					m_Moods.InsertItem( sMood.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hParent );
					moodSet.insert( sMood );
				}	
			}

			if ( gpstring( (*iMood)->GetName() ).same_no_case( "mood_setting*" ) )
			{
				gpstring sMood;
				(*iMood)->Get( "mood_name", sMood );
				m_Moods.InsertItem( sMood.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, TVI_ROOT );			
				moodSet.insert( sMood );
			}
		}
	}

	FuelHandle hMoodsHome( "::moods_home:root" );
	if ( hMoodsHome )
	{
		FuelHandleList hlMoods = hMoodsHome->ListChildBlocks( 1 );
		FuelHandleList::iterator iMood;
		for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
		{
			if ( (*iMood)->IsDirectory() )
			{
				HTREEITEM hParent = m_Moods.InsertItem( (*iMood)->GetName(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER );				
				dirSet.insert( (*iMood)->GetName() );

				FuelHandleList hlChildMoods = (*iMood)->ListChildBlocks( -1, "", "mood_setting*" );
				FuelHandleList::iterator iChild;
				for ( iChild = hlChildMoods.begin(); iChild != hlChildMoods.end(); ++iChild )
				{
					gpstring sMood;
					(*iChild)->Get( "mood_name", sMood );
					m_Moods.InsertItem( sMood.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hParent );
					moodSet.insert( sMood );
				}	
			}

			if ( gpstring( (*iMood)->GetName() ).same_no_case( "mood_setting*" ) )
			{
				gpstring sMood;
				(*iMood)->Get( "mood_name", sMood );
				m_Moods.InsertItem( sMood.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, TVI_ROOT );			
				moodSet.insert( sMood );
			}
		}
	}

	FastFuelHandle hTankMoods( "world:global:moods" );
	if ( hTankMoods )
	{
		FastFuelHandleColl hlMoods;
		hTankMoods.ListChildren( hlMoods, 1 );
		FastFuelHandleColl::iterator iMood;
		for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
		{
			if ( (*iMood).IsDirectory() )
			{
				StringSet::iterator iFound = dirSet.find( (*iMood).GetName() );
				if ( iFound != dirSet.end() )
				{
					continue;
				}

				HTREEITEM hParent = m_Tanked.InsertItem( (*iMood).GetName(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER );				

				FastFuelHandleColl hlChildMoods;
				(*iMood).ListChildrenNamed( hlChildMoods, "mood_setting*", -1 );
				FastFuelHandleColl::iterator iChild;
				for ( iChild = hlChildMoods.begin(); iChild != hlChildMoods.end(); ++iChild )
				{
					gpstring sMood;
					(*iChild).Get( "mood_name", sMood );
					StringSet::iterator iFound = moodSet.find( sMood );
					if ( iFound == moodSet.end() )
					{
						m_Tanked.InsertItem( sMood.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hParent );
					}
				}	
			}

			if ( gpstring( (*iMood).GetName() ).same_no_case( "mood_setting*" ) )
			{
				gpstring sMood;
				(*iMood).Get( "mood_name", sMood );
				StringSet::iterator iFound = moodSet.find( sMood );
				if ( iFound == moodSet.end() )
				{
					m_Tanked.InsertItem( sMood.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, TVI_ROOT );			
				}
			}
		}
	}
}

bool Dialog_Mood_Editor::DoesMoodExist( const gpstring & sMood )
{
	FuelHandle hMoods( "::moods:root" );
	if ( hMoods )
	{
		FuelHandleList hlMoods = hMoods->ListChildBlocks( 2 );
		FuelHandleList::iterator iMood;
		FuelHandle hFolder;
		for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
		{
			if ( gpstring( (*iMood)->GetName() ).same_no_case( "mood_setting*" ) )
			{
				gpstring sName;
				(*iMood)->Get( "mood_name", sName );
				if ( sName.same_no_case( sMood ) )
				{					
					return true;
				}
			}
		}
	}	

	return false;
}


void Dialog_Mood_Editor::OnButtonAddFolder() 
{
	Dialog_Mood_Add_Folder dlg;
	dlg.DoModal();
	SaveMoods();
	Refresh();		
}

void Dialog_Mood_Editor::OnButtonStop() 
{
	gMood.Shutdown();
	gPreferences.SetClearColor( gPreferences.GetClearColor() );	
	gMood.StopAmbientTrack( 0.0f );
	gMood.StopBattleTrack( 0.0f );
	gMood.StopStandardTrack( 0.0f );
}

void Dialog_Mood_Editor::OnClose() 
{	
	ShutdownDbs();

	CDialog::OnClose();
}

void Dialog_Mood_Editor::OnDestroy() 
{
	CDialog::OnDestroy();	
}

void Dialog_Mood_Editor::OnOK() 
{
	ShutdownDbs();

	CDialog::OnOK();
}

void Dialog_Mood_Editor::OnButtonTestAmbient() 
{		
	gMood.StopBattleTrack( 0.0f );
	gMood.StopStandardTrack( 0.0f );

	gMood.PlayAmbientTrack( 0.0f );	
	gMood.ForceUpdateMusic( 0.0f );

	gPreferences.SetUpdateSound( true );
}

void Dialog_Mood_Editor::OnButtonTestBattle() 
{
	gMood.StopAmbientTrack( 0.0f );	
	gMood.StopStandardTrack( 0.0f );

	gMood.PlayBattleTrack( 0.0f );
	gMood.ForceUpdateMusic( 0.0f );

	gPreferences.SetUpdateSound( true );
}

void Dialog_Mood_Editor::OnButtonTestStandard() 
{
	gMood.StopAmbientTrack( 0.0f );
	gMood.StopBattleTrack( 0.0f );	

	gMood.PlayStandardTrack( 0.0f );	
	gMood.ForceUpdateMusic( 0.0f );

	gPreferences.SetUpdateSound( true );
}

void Dialog_Mood_Editor::SaveMoods()
{
	FuelHandle hMood( "::moods:root" );
	if ( hMood )
	{
		hMood->GetDB()->SaveChanges();
	}

	FuelHandle hMoodHome( "::moods_home:root" );
	if ( hMoodHome )
	{
		hMoodHome->GetDB()->SaveChanges();
	}
}

void Dialog_Mood_Editor::ShutdownDbs()
{
	SaveMoods();

	FuelHandle hMood( "::moods:root" );
	if ( hMood )
	{
		gFuelSys.Remove( hMood->GetDB() );			
	}

	FuelHandle hMoodHome( "::moods_home:root" );
	if ( hMoodHome )
	{
		gFuelSys.Remove( hMoodHome->GetDB() );		
	}	
}


void Dialog_Mood_Editor::OnClickTreeMoods(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_Tanked.SelectItem( NULL );	
	*pResult = 0;
}

void Dialog_Mood_Editor::OnClickTreeMoodsTanked(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_Moods.SelectItem( NULL );	
	*pResult = 0;
}
