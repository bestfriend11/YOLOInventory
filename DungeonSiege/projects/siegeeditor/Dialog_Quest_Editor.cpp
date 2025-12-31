// Dialog_Quest_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Editor.h"
#include "Dialog_Quest_Manager.h"
#include "EditorRegion.h"
#include "FileSys.h"
#include "Dialog_Quest_Update.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Editor dialog


Dialog_Quest_Editor::Dialog_Quest_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Editor)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Quest_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Editor)
	DDX_Control(pDX, IDC_LIST_QUEST_UPDATES, m_QuestUpdates);
	DDX_Control(pDX, IDC_EDIT_SCREEN_NAME, m_ScreenName);
	DDX_Control(pDX, IDC_EDIT_QUEST_IMAGE, m_QuestImage);
	DDX_Control(pDX, IDC_COMBO_VICTORY_SAMPLE, m_VictorySample);
	DDX_Control(pDX, IDC_COMBO_CHAPTER, m_Chapter);
	DDX_Control(pDX, IDC_COMBO_AWARD_SKRIT, m_AwardSkrit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Editor)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Editor message handlers

BOOL Dialog_Quest_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_QuestUpdates.InsertColumn( 0, "Description", LVCFMT_LEFT, 210 );
	m_QuestUpdates.InsertColumn( 1, "Step", LVCFMT_LEFT, 50 );

	// Grab a list of audio samples
	{
		m_VictorySample.AddString( "<none>" );	
		FileSys::RecursiveFinder finder( "sound/effects/*.wav", 0 );
		gpstring fileName;
		while ( finder.GetNext( fileName ) )
		{
			m_VictorySample.AddString( FileSys::GetFileNameOnly( fileName ) );
		}
		m_VictorySample.SetCurSel( 0 );
	}

	// Grab a list of chapters
	m_Chapter.AddString( "<none>" );
	FuelHandle hChapters = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:chapters" );
	if ( hChapters )
	{
		FuelHandleList hlChapters = hChapters->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlChapters.begin(); i != hlChapters.end(); ++i )
		{
			m_Chapter.AddString( (*i)->GetName() );
		}
	}
	m_Chapter.SetCurSel( 0 );

	// Grab a list of award skrits.
	{
		m_AwardSkrit.AddString( "<none>" );
		FuelHandle hQuestDir = gEditorRegion.GetMapHandle()->GetChildBlock( "quests" );
		gpstring sPath = FuelAddressToFilePath( hQuestDir->GetAddress() );
		sPath.appendf( "\\*.skrit" );
		FileSys::RecursiveFinder finder( sPath.c_str(), 0 );
		gpstring fileName;
		while ( finder.GetNext( fileName ) )
		{
			m_VictorySample.AddString( FileSys::GetFileNameOnly( fileName ) );
		}
		m_AwardSkrit.SetCurSel( 0 );
	}
	
	FuelHandle hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:quests" );
	if ( hQuests )
	{
		FuelHandleList hlQuests = hQuests->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlQuests.begin(); i != hlQuests.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( gQuestManager.GetSelectedQuest() ) )
			{
				gpstring sScreenName;
				(*i)->Get( "screen_name", sScreenName );
				m_ScreenName.SetWindowText( sScreenName );

				gpstring sQuestImage;
				(*i)->Get( "quest_image", sQuestImage );
				m_QuestImage.SetWindowText( sQuestImage );

				gpstring sVictorySample;
				(*i)->Get( "victory_sample", sVictorySample );
				int find = m_VictorySample.FindString( 0, sVictorySample.c_str() );
				if ( find != CB_ERR )
				{
					m_VictorySample.SetCurSel( find );
				}

				gpstring sChapter;
				(*i)->Get( "chapter", sChapter );
				find = m_Chapter.FindString( 0, sChapter.c_str() );
				if ( find != CB_ERR )
				{
					m_Chapter.SetCurSel( find );
				}

				gpstring sAwardSkrit;
				(*i)->Get( "award_skrit", sAwardSkrit );
				find = m_AwardSkrit.FindString( 0, sAwardSkrit.c_str() );
				
				// Get the child update blocks
				m_hlUpdates = (*i)->ListChildBlocks( 1 );

				break;
			}
		}
	}

	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Quest_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Quest_Editor::OnOK() 
{
	CString rScreenName, rQuestImage, rVictorySample, rChapter, rAwardSkrit;
	m_ScreenName.GetWindowText( rScreenName );
	m_QuestImage.GetWindowText( rQuestImage );
	m_VictorySample.GetWindowText( rVictorySample );
	m_Chapter.GetWindowText( rChapter );
	m_AwardSkrit.GetWindowText( rAwardSkrit );

	FuelHandle hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:quests" );
	if ( hQuests )
	{
		FuelHandleList hlQuests = hQuests->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlQuests.begin(); i != hlQuests.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( gQuestManager.GetSelectedQuest() ) )
			{
				(*i)->Set( "screen_name", rScreenName.GetBuffer( 0 ) );
				
				if ( rQuestImage.IsEmpty() )
				{
					(*i)->Remove( "quest_image" );
				}
				else
				{
					(*i)->Set( "quest_image", rQuestImage.GetBuffer( 0 ) );
				}
				
				if ( rVictorySample.CompareNoCase( "<none>" ) != 0 )
				{
					(*i)->Set( "victory_sample", rVictorySample.GetBuffer( 0 ) );
				}
				else
				{
					(*i)->Remove( "victory_sample" );
				}

				if ( rChapter.CompareNoCase( "<none>" ) != 0 )
				{
					(*i)->Set( "chapter", rChapter.GetBuffer( 0 ) );
				}
				else
				{
					(*i)->Remove( "chapter" );
				}

				if ( rAwardSkrit.CompareNoCase( "<none>" ) != 0 )
				{
					(*i)->Set( "award_skrit", rAwardSkrit.GetBuffer( 0 ) );
				}
				else 
				{
					(*i)->Remove( "award_skrit" );
				}

				break;
			}
		}
	}	
	
	CDialog::OnOK();
}


void Dialog_Quest_Editor::Refresh()
{
	m_QuestUpdates.DeleteAllItems();
	FuelHandleList::iterator i;
	int index = 0;
	for ( i = m_hlUpdates.begin(); i != m_hlUpdates.end(); ++i, index++ )
	{
		gpstring sDescription;		
		(*i)->Get( "description", sDescription );		
		if ( sDescription.empty() )
		{
			sDescription = "<none>";
		}

		int item = m_QuestUpdates.InsertItem( index, sDescription.c_str() );
		int order = 0;
		if ( (*i)->Get( "order", order ) )
		{			
			gpstring sOrder;
			sOrder.assignf( "%d", order );
			m_QuestUpdates.SetItemText( item, 1, sOrder.c_str() );
		}		
	}
}

void Dialog_Quest_Editor::OnButtonAdd() 
{	
	gpstring sAddress;
	sAddress.assignf( "quests:quests:%s", gQuestManager.GetSelectedQuest().c_str() );
	FuelHandle hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( sAddress );
	if ( hQuests )
	{
		FuelHandle hNewUpdate = hQuests->CreateChildBlock( "*" );
		m_hlUpdates.push_back( hNewUpdate );
	}
	Refresh();
}

void Dialog_Quest_Editor::OnButtonEdit() 
{
	int sel = m_QuestUpdates.GetSelectionMark();
	FuelHandleList::iterator i;
	int index = 0;
	for ( i = m_hlUpdates.begin(); i != m_hlUpdates.end(); ++i, index++ )
	{
		if ( index == sel )
		{
			m_hSelected = (*i);
			Dialog_Quest_Update dlg;
			dlg.DoModal();
			break;
		}
	}	
	Refresh();
}

void Dialog_Quest_Editor::OnButtonRemove() 
{
	int sel = m_QuestUpdates.GetSelectionMark();
	FuelHandleList::iterator i;
	int index = 0;
	for ( i = m_hlUpdates.begin(); i != m_hlUpdates.end(); ++i, index++ )
	{
		if ( index == sel )
		{
			FuelHandle hParent = (*i)->GetParent();
			hParent->DestroyChildBlock( *i );
			m_hlUpdates.erase( i );
			break;
		}
	}

	Refresh();
}

bool Dialog_Quest_Editor::DoesOrderExist( int order )
{
	FuelHandleList::iterator i;
	int index = 0;
	for ( i = m_hlUpdates.begin(); i != m_hlUpdates.end(); ++i, index++ )
	{
		int existingOrder = 0;
		if ( (*i)->Get( "order", existingOrder ) )
		{					
			if ( order == existingOrder )
			{
				return true;
			}
		}
	}

	return false;
}