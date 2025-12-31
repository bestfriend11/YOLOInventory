// Dialog_Quest_Manager.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Manager.h"
#include "EditorRegion.h"
#include "Dialog_Quest_Add.h"
#include "Dialog_Quest_Editor.h"
#include "Dialog_Quest_Chapter_Add.h"
#include "Dialog_Quest_Chapter_Editor.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Manager dialog


Dialog_Quest_Manager::Dialog_Quest_Manager(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Manager::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Manager)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Quest_Manager::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Manager)
	DDX_Control(pDX, IDC_LIST_QUESTS, m_Quests);
	DDX_Control(pDX, IDC_LIST_CHAPTERS, m_Chapters);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Manager, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Manager)
	ON_BN_CLICKED(IDC_BUTTON_ADD_QUEST, OnButtonAddQuest)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_QUEST, OnButtonEditQuest)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_QUEST, OnButtonRemoveQuest)
	ON_BN_CLICKED(IDC_BUTTON_ADD_CHAPTER, OnButtonAddChapter)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_CHAPTER, OnButtonEditChapter)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_CHAPTER, OnButtonRemoveChapter)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Manager message handlers

BOOL Dialog_Quest_Manager::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Quest_Manager::OnButtonAddQuest() 
{
	Dialog_Quest_Add dlg;
	dlg.DoModal();
	Refresh();
}

void Dialog_Quest_Manager::OnButtonEditQuest() 
{
	if ( m_Quests.GetCurSel() == LB_ERR )
	{
		return;
	}

	CString rQuest;
	m_Quests.GetText( m_Quests.GetCurSel(), rQuest );
	if ( rQuest.IsEmpty() )
	{
		return;
	}
	m_sSelectedQuest = rQuest.GetBuffer( 0 );

	Dialog_Quest_Editor dlg;
	dlg.DoModal();
	Refresh();
}

void Dialog_Quest_Manager::OnButtonRemoveQuest() 
{
	CString rQuest;
	m_Quests.GetText( m_Quests.GetCurSel(), rQuest );

	FuelHandle hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:quests" );
	if ( hQuests )
	{
		FuelHandleList hlQuests = hQuests->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlQuests.begin(); i != hlQuests.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( rQuest.GetBuffer( 0 ) ) )
			{
				hQuests->DestroyChildBlock( *i );
				break;
			}
		}
	}

	Refresh();
}

void Dialog_Quest_Manager::OnButtonAddChapter() 
{
	Dialog_Quest_Chapter_Add dlg;
	dlg.DoModal();
	Refresh();
}

void Dialog_Quest_Manager::OnButtonEditChapter() 
{
	if ( m_Chapters.GetCurSel() == LB_ERR )
	{
		return;
	}

	CString rChapter;
	m_Chapters.GetText( m_Chapters.GetCurSel(), rChapter );
	if ( rChapter.IsEmpty() )
	{
		return;
	}

	m_sSelectedChapter = rChapter.GetBuffer( 0 );

	Dialog_Quest_Chapter_Editor dlg;
	dlg.DoModal();	
}

void Dialog_Quest_Manager::OnButtonRemoveChapter() 
{
	CString rChapter;
	m_Chapters.GetText( m_Chapters.GetCurSel(), rChapter );
	FuelHandle hChapters = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:chapters" );
	if ( hChapters )
	{
		FuelHandleList hlChapters = hChapters->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlChapters.begin(); i != hlChapters.end(); ++i )
		{		
			if ( gpstring( (*i)->GetName() ).same_no_case( rChapter.GetBuffer( 0 ) ) )
			{
				hChapters->DestroyChildBlock( *i );
				break;
			}
		}
	}
	
	Refresh();
}

void Dialog_Quest_Manager::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Quest_Manager::Refresh()
{
	m_Quests.ResetContent();
	m_Chapters.ResetContent();
	FuelHandle hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:quests" );
	if ( hQuests )
	{
		FuelHandleList hlQuests = hQuests->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlQuests.begin(); i != hlQuests.end(); ++i )
		{
			m_Quests.AddString( (*i)->GetName() );
		}
	}

	FuelHandle hChapters = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:chapters" );
	if ( hChapters )
	{
		FuelHandleList hlChapters = hChapters->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlChapters.begin(); i != hlChapters.end(); ++i )
		{
			m_Chapters.AddString( (*i)->GetName() );
		}
	}
}