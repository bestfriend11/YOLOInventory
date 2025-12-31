// Dialog_Quest_Chapter_Add.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Chapter_Add.h"
#include "EditorRegion.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Add dialog


Dialog_Quest_Chapter_Add::Dialog_Quest_Chapter_Add(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Chapter_Add::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Chapter_Add)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Quest_Chapter_Add::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Chapter_Add)
	DDX_Control(pDX, IDC_EDIT_CHAPTER, m_Chapter);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Chapter_Add, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Chapter_Add)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Add message handlers

void Dialog_Quest_Chapter_Add::OnOK() 
{
	CString rChapter;
	m_Chapter.GetWindowText( rChapter );

	FuelHandle hChapter = gEditorRegion.GetMapHandle()->GetChildBlock( "quests:chapters" );
	if ( !hChapter )
	{
		hChapter = gEditorRegion.GetMapHandle()->GetChildBlock( "quests" );
		if ( !hChapter )
		{
			hChapter = gEditorRegion.GetMapHandle()->CreateChildDirBlock( "quests" );
		}

		if ( hChapter )
		{
			hChapter = hChapter->CreateChildBlock( "chapters", "quests.gas" );
		}
	}

	if ( hChapter )
	{
		FuelHandleList hlChapter = hChapter->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlChapter.begin(); i != hlChapter.end(); ++i )
		{
			if ( gpstring( (*i)->GetName() ).same_no_case( rChapter.GetBuffer( 0 ) ) )
			{
				MessageBoxEx( this->GetSafeHwnd(), "A chapter with this name already exists, please type in a different name.", "Add New Chapter", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
		}
		
		gpstring sTemp = rChapter.GetBuffer( 0 );
		if( !FileSys::IsValidFileName( sTemp, false, false ) )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Invalid chapter name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Add Chapter", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;			
		}

		hChapter->CreateChildBlock( rChapter.GetBuffer( 0 ) );
	}	
	
	CDialog::OnOK();
}
