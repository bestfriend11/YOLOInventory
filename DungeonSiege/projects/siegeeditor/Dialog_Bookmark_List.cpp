// Dialog_Bookmark_List.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Bookmark_List.h"
#include "EditorMap.h"
#include "Dialog_Bookmark_Properties.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Bookmark_List dialog


Dialog_Bookmark_List::Dialog_Bookmark_List(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Bookmark_List::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Bookmark_List)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Bookmark_List::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Bookmark_List)
	DDX_Control(pDX, IDC_LIST_BOOKMARKS, m_bookmarks);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Bookmark_List, CDialog)
	//{{AFX_MSG_MAP(Dialog_Bookmark_List)
	ON_BN_CLICKED(IDC_BUTTON_NEW, OnButtonNew)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, OnButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_PROPERTIES, OnButtonProperties)
	ON_LBN_SELCHANGE(IDC_LIST_BOOKMARKS, OnSelchangeListBookmarks)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Bookmark_List message handlers

BOOL Dialog_Bookmark_List::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	RefreshBookmarks();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Bookmark_List::OnButtonNew() 
{
	int index = 0;
	gpstring sName = "new";
	SiegePos spos;
	gpstring sNewName = sName;	
	while ( !gEditorMap.AddBookmark( sNewName, spos, 0.0f, 0.0f, 0.0f ) ) 
	{	
		sNewName.assignf( "%s_%d", sName.c_str(), index++ );
	}	

	gEditorMap.SetSelectedBookmark( sNewName );
	Dialog_Bookmark_Properties dlg;
	if ( dlg.DoModal() == IDCANCEL )
	{
		gEditorMap.RemoveBookmark( sNewName.c_str() );
	}

	RefreshBookmarks();
}

void Dialog_Bookmark_List::OnButtonRemove() 
{
	if ( m_bookmarks.GetCurSel() == LB_ERR ) 
	{
		return;
	}

	CString rBookmark;
	m_bookmarks.GetText( m_bookmarks.GetCurSel(), rBookmark );
	gEditorMap.RemoveBookmark( rBookmark.GetBuffer( 0 ) );

	RefreshBookmarks();	
}


void Dialog_Bookmark_List::RefreshBookmarks()
{
	m_bookmarks.ResetContent();
	StringVec bookmarks;
	StringVec::iterator i;
	gEditorMap.GetBookmarks( bookmarks );
	for ( i = bookmarks.begin(); i != bookmarks.end(); ++i ) 
	{
		m_bookmarks.AddString( (*i).c_str() );
	}
}

void Dialog_Bookmark_List::OnButtonProperties() 
{
	Dialog_Bookmark_Properties dlg;
	dlg.DoModal();
	
	RefreshBookmarks();
}

void Dialog_Bookmark_List::OnSelchangeListBookmarks() 
{
	if ( m_bookmarks.GetCurSel() == LB_ERR ) 
	{
		return;
	}

	CString rBookmark;
	m_bookmarks.GetText( m_bookmarks.GetCurSel(), rBookmark );
	gEditorMap.SetSelectedBookmark( rBookmark.GetBuffer( 0 ) );	
}

void Dialog_Bookmark_List::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
