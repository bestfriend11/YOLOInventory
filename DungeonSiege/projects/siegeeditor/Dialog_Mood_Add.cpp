// Dialog_Mood_Add.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_Add.h"
#include "Dialog_Mood_Editor.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Add dialog


Dialog_Mood_Add::Dialog_Mood_Add(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_Add::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_Add)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Mood_Add::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_Add)
	DDX_Control(pDX, IDC_EDIT_NAME, m_Name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_Add, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_Add)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Add message handlers

void Dialog_Mood_Add::OnOK() 
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
			if ( gpstring( (*iMood)->GetName() ).same_no_case( "mood_setting*" ) )
			{
				gpstring sName;
				(*iMood)->Get( "mood_name", sName );
				if ( sName.same_no_case( rName.GetBuffer( 0 ) ) )
				{
					MessageBoxEx( this->GetSafeHwnd(), "A mood of this name already exists. Please select a different name.", "Add Mood", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
			}
			else if ( (*iMood)->IsDirectory() && gpstring( (*iMood)->GetName() ).same_no_case( gDialogMoodEditor.GetSelectedFolder() ) )
			{
				hFolder = *iMood;
			}
		}
		
		if ( hFolder )
		{
			gpstring sFile = hFolder->GetName();
			sFile.appendf( "_moods.gas" ); 
			FuelHandle hMood = hFolder->CreateChildBlock( "mood_setting*", sFile );
			hMood->Set( "mood_name", rName.GetBuffer( 0 ) );
			hMood->Set( "transition_time", 0.0f );
			hMood->Set( "interior", false );
		}
	}
	
	CDialog::OnOK();
}

void Dialog_Mood_Add::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}
