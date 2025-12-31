// Dialog_Quest_Chapter_Editor.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Chapter_Editor.h"
#include "Dialog_Quest_Manager.h"
#include "Dialog_Quest_Chapter_Update.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Editor dialog


Dialog_Quest_Chapter_Editor::Dialog_Quest_Chapter_Editor(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Chapter_Editor::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Chapter_Editor)
	//}}AFX_DATA_INIT
}


void Dialog_Quest_Chapter_Editor::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Chapter_Editor)
	DDX_Control(pDX, IDC_LIST_QUEST_UPDATES, m_ChapterUpdates);
	DDX_Control(pDX, IDC_EDIT_SCREEN_NAME, m_ScreenName);
	DDX_Control(pDX, IDC_EDIT_CHAPTER_IMAGE, m_Image);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Chapter_Editor, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Chapter_Editor)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Editor message handlers

BOOL Dialog_Quest_Chapter_Editor::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ChapterUpdates.InsertColumn( 0, "Description", LVCFMT_LEFT, 210 );
	m_ChapterUpdates.InsertColumn( 1, "Step", LVCFMT_LEFT, 50 );

	FuelHandle hChapters = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:chapters" );
	if ( hChapters )
	{
		FuelHandleList hlChapters = hChapters->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlChapters.begin(); i != hlChapters.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( gQuestManager.GetSelectedChapter() ) )
			{
				gpstring sScreenName;
				(*i)->Get( "screen_name", sScreenName );
				m_ScreenName.SetWindowText( sScreenName );

				gpstring sChapterImage;
				(*i)->Get( "chapter_image", sChapterImage );
				m_Image.SetWindowText( sChapterImage );

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

void Dialog_Quest_Chapter_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Quest_Chapter_Editor::OnButtonAdd() 
{
	gpstring sAddress;
	sAddress.assignf( "quests:chapters:%s", gQuestManager.GetSelectedChapter().c_str() );
	FuelHandle hChapters = gEditorRegion.GetMapHandle()->GetChildBlock( sAddress );
	if ( hChapters )
	{
		FuelHandle hNewUpdate = hChapters->CreateChildBlock( "*" );
		m_hlUpdates.push_back( hNewUpdate );
	}
	Refresh();	
}

void Dialog_Quest_Chapter_Editor::OnButtonEdit() 
{
	int sel = m_ChapterUpdates.GetSelectionMark();
	FuelHandleList::iterator i;
	int index = 0;
	for ( i = m_hlUpdates.begin(); i != m_hlUpdates.end(); ++i, index++ )
	{
		if ( index == sel )
		{
			m_hSelectedHandle = (*i);
			Dialog_Quest_Chapter_Update dlg;
			dlg.DoModal();
			break;
		}
	}
	Refresh();
}

void Dialog_Quest_Chapter_Editor::OnButtonRemove() 
{
	int sel = m_ChapterUpdates.GetSelectionMark();
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

void Dialog_Quest_Chapter_Editor::Refresh()
{
	m_ChapterUpdates.DeleteAllItems();
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

		int item = m_ChapterUpdates.InsertItem( index, sDescription.c_str() );
		int order = 0;
		if ( (*i)->Get( "order", order ) )
		{			
			gpstring sOrder;
			sOrder.assignf( "%d", order );
			m_ChapterUpdates.SetItemText( item, 1, sOrder.c_str() );
		}			
	}
}

void Dialog_Quest_Chapter_Editor::OnOK() 
{
	CString rScreenName, rImage;
	m_ScreenName.GetWindowText( rScreenName );
	m_Image.GetWindowText( rImage );

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

				if ( rImage.IsEmpty() )
				{
					(*i)->Remove( "chapter_image" );
				}
				else
				{
					(*i)->Set( "chapter_image", rImage.GetBuffer( 0 ) );
				}
				break;
			}
		}
	}
	
	CDialog::OnOK();
}

bool Dialog_Quest_Chapter_Editor::DoesOrderExist( int order )
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