// Dialog_Stitch_Tool.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Stitch_Tool.h"
#include "siege_engine.h"
#include "stitch_helper.h"
#include "EditorRegion.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Tool dialog


Dialog_Stitch_Tool::Dialog_Stitch_Tool(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Stitch_Tool::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Stitch_Tool)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Stitch_Tool::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Stitch_Tool)
	DDX_Control(pDX, IDC_COMBO_STITCH_REGION, m_StitchRegion);
	DDX_Control(pDX, IDC_EDIT_STITCH, m_Stitch);
	DDX_Control(pDX, IDC_COMBO_STITCH_IDS, m_Stitches);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Stitch_Tool, CDialog)
	//{{AFX_MSG_MAP(Dialog_Stitch_Tool)
	ON_BN_CLICKED(IDC_BUTTON_ADD_STITCHES, OnButtonAddStitches)
	ON_BN_CLICKED(IDC_BUTTON_UNDO, OnButtonUndo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Stitch_Tool message handlers

BOOL Dialog_Stitch_Tool::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	StringVec region_list;
	StringVec::iterator j;
	gEditorRegion.GetRegionList( gEditorRegion.GetMapName().c_str(), region_list );
	for ( j = region_list.begin(); j != region_list.end(); ++j ) 
	{		
		if ( (*j).same_no_case( gEditorRegion.GetRegionName() ) ) 		
		{
			continue;
		}
		
		StitchVector region_stitches;
		StitchVector::iterator i;
		gStitchHelper.GetMapStitchVector( (*j).c_str(), region_stitches );
		for ( i = region_stitches.begin(); i != region_stitches.end(); ++i ) 
		{		
			if ( (*i).sRegion.same_no_case( gEditorRegion.GetRegionName() ) )
			{
				gpstring sTemp;
				sTemp.assignf( "0x%08X - Region: %s", (*i).nodeID, (*j).c_str() );
				int index = m_Stitches.AddString( sTemp );
				m_Stitches.SetItemData( index, (*i).nodeID );				
			}
		}

		m_StitchRegion.AddString( (*j).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Stitch_Tool::OnButtonAddStitches() 
{	
	m_StitchedVector.clear();

	CString rText;
	m_Stitch.GetWindowText( rText );	
	int id = 0;
	
	if ( !rText.IsEmpty() )
	{
		gpstring sId = rText.GetBuffer( 0 );		
		stringtool::Get( sId.c_str(), id );			
	}
	else
	{
		int sel = m_Stitches.GetCurSel();
		if ( sel == CB_ERR )
		{
			MessageBoxEx( this->GetSafeHwnd(), "Please enter or select a starting stitch ID.", "Stitch Tool", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
			return;
		}

		m_Stitches.GetWindowText( rText );
		if ( !rText.IsEmpty() )
		{
			id = m_Stitches.GetItemData( sel );
			gpstring sId = rText.GetBuffer( 0 );
			int pos = sId.find( "- Region: " );
			if ( pos != gpstring::npos )
			{
				pos += sizeof( "- Region: " );
				sId = sId.substr( pos, sId.size() );

				m_StitchRegion.SelectString( 0, sId.c_str() );
			}		
		}
	}

	CString rRegion;
	m_StitchRegion.GetWindowText( rRegion );
	if ( rRegion.IsEmpty() )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please enter or select a region to stitch to.", "Stitch Tool", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	gEditorTerrain.AutoStitch( id, rRegion.GetBuffer( 0 ), m_StitchedVector );		
}

void Dialog_Stitch_Tool::OnButtonUndo() 
{
	StitchedVector::iterator i;
	for ( i = m_StitchedVector.begin(); i != m_StitchedVector.end(); ++i )
	{
		gStitchHelper.RemoveNodeIDFromStitchHelper( *i );
	}	
}
