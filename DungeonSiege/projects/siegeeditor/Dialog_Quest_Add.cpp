// Dialog_Quest_Add.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Add.h"
#include "EditorRegion.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Add dialog


Dialog_Quest_Add::Dialog_Quest_Add(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Add::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Add)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Quest_Add::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Add)
	DDX_Control(pDX, IDC_EDIT_QUEST, m_Quest);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Add, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Add)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Add message handlers

void Dialog_Quest_Add::OnOK() 
{
	CString rQuest;
	m_Quest.GetWindowText( rQuest );

	FuelHandle hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:quests" );
	if ( !hQuests )
	{
		hQuests = gEditorRegion.GetMapHandle()->GetChildBlock( "quests" );
		if ( !hQuests )
		{
			hQuests = gEditorRegion.GetMapHandle()->CreateChildDirBlock( "quests" );
		}

		if ( hQuests )
		{
			hQuests = hQuests->CreateChildBlock( "quests", "quests.gas" );
		}
	}

	if ( hQuests )
	{
		FuelHandleList hlQuests = hQuests->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlQuests.begin(); i != hlQuests.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( rQuest.GetBuffer( 0 ) ) )
			{
				MessageBoxEx( this->GetSafeHwnd(), "A quest with this name already exists, please type in a different name.", "Add New Quest", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
		}

		gpstring sTemp = rQuest.GetBuffer( 0 );
		if( !FileSys::IsValidFileName( sTemp, false, false ) )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Invalid quest name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Add Quest", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;				
		}

		hQuests->CreateChildBlock( rQuest.GetBuffer( 0 ) );
	}	
	
	CDialog::OnOK();
}
