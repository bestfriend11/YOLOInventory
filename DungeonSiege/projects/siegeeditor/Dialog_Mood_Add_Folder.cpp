// Dialog_Mood_Add_Folder.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_Add_Folder.h"
#include "Dialog_Mood_Editor.h"
#include "FileSys.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Add_Folder dialog


Dialog_Mood_Add_Folder::Dialog_Mood_Add_Folder(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_Add_Folder::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_Add_Folder)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Mood_Add_Folder::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_Add_Folder)
	DDX_Control(pDX, IDC_EDIT_NAME, m_Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_Add_Folder, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_Add_Folder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Add_Folder message handlers

void Dialog_Mood_Add_Folder::OnOK() 
{
	CString rName;
	m_Name.GetWindowText( rName );

	FuelHandle hMoods( "::moods:root" );
	if ( hMoods )
	{
		FuelHandleList hlMoods = hMoods->ListChildBlocks( 2 );
		FuelHandleList::iterator iMood;
		FuelHandle hFolder;
		for ( iMood = hlMoods.begin(); iMood != hlMoods.end(); ++iMood )
		{
			if ( (*iMood)->IsDirectory() )
			{
				if ( gpstring( (*iMood)->GetName() ).same_no_case( rName.GetBuffer( 0 ) ) )
				{
					MessageBoxEx( this->GetSafeHwnd(), "A folder of this name already exists. Please select a different name.", "Add Mood Folder", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
			}
		}

		if( !FileSys::IsValidFileName( rName.GetBuffer( 0 ), false, false ) )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Invalid mood folder name.  Please do not use these characters: " BAD_FILENAME_CHARS_ONLY " and space.", "Region Name", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;			
		}

		hMoods->CreateChildDirBlock( rName.GetBuffer( 0 ) );
	}

		
	CDialog::OnOK();
}
