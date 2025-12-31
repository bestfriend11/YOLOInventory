// Dialog_Quest_Chapter_Update.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Quest_Chapter_Update.h"
#include "EditorRegion.h"
#include "Dialog_Quest_Chapter_Editor.h"
#include "FileSysUtils.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Update dialog


Dialog_Quest_Chapter_Update::Dialog_Quest_Chapter_Update(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Quest_Chapter_Update::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Quest_Chapter_Update)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Quest_Chapter_Update::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Quest_Chapter_Update)
	DDX_Control(pDX, IDC_EDIT_ORDER, m_Order);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_Description);
	DDX_Control(pDX, IDC_COMBO_SAMPLE, m_Samples);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Quest_Chapter_Update, CDialog)
	//{{AFX_MSG_MAP(Dialog_Quest_Chapter_Update)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Quest_Chapter_Update message handlers

BOOL Dialog_Quest_Chapter_Update::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Grab a list of audio samples
	{
		m_Samples.AddString( "<none>" );	
		FileSys::RecursiveFinder finder( "sound/voices/*.mp3", 0 );
		gpstring fileName;
		while ( finder.GetNext( fileName ) )
		{
			m_Samples.AddString( FileSys::GetFileNameOnly( fileName ) );
		}
		m_Samples.SetCurSel( 0 );
	}
	
	FuelHandle hUpdate = gChapterEditor.GetSelectedHandle();
	if ( hUpdate )
	{
		int order = 0;		
		if ( hUpdate->Get( "order", order ) )
		{
			gpstring sOrder;
			sOrder.assignf( "%d", order );
			m_Order.SetWindowText( sOrder.c_str() );
			m_sOriginalOrder = sOrder;
		}

		gpstring sDescription;
		hUpdate->Get( "description", sDescription );
		m_Description.SetWindowText( sDescription.c_str() );

		gpstring sSample;
		hUpdate->Get( "sample", sSample );
		int find = m_Samples.FindString( 0, sSample );
		if ( find != CB_ERR )
		{
			m_Samples.SetCurSel( find );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Quest_Chapter_Update::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

void Dialog_Quest_Chapter_Update::OnOK() 
{
	CString rOrder, rDescription, rSample;
	m_Order.GetWindowText( rOrder );
	m_Description.GetWindowText( rDescription );
	m_Samples.GetWindowText( rSample );
	
	FuelHandle hUpdate = gChapterEditor.GetSelectedHandle();
	if ( hUpdate )
	{
		if ( !rOrder.IsEmpty() )
		{
			int order = 0;
			stringtool::Get( rOrder.GetBuffer( 0 ), order );

			if ( !m_sOriginalOrder.same_no_case( rOrder.GetBuffer( 0 ) ) )
			{
				if ( gChapterEditor.DoesOrderExist( order ) )
				{
					MessageBoxEx( this->GetSafeHwnd(), "An update with this order already exists.  Please choose a different order.", "Quest Update", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
					return;
				}
			}

			hUpdate->Set( "order", order );
		}
		else
		{
			hUpdate->Remove( "order" );
		}

		hUpdate->Set( "description", rDescription.GetBuffer( 0 ) );

		if ( !rSample.IsEmpty() && rSample.CompareNoCase( "<none>" ) != 0 )
		{
			hUpdate->Set( "sample", rSample.GetBuffer( 0 ) );
		}
		else
		{
			hUpdate->Remove( "sample" );
		}
	}
	
	CDialog::OnOK();
}
