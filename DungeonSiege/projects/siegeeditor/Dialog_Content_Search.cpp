// Dialog_Content_Search.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Content_Search.h"
#include "ComponentList.h"
#include "EditorObjects.h"
#include "LeftView.h"
#include "CustomViewer.h"
#include "ImageListDefines.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Content_Search dialog


Dialog_Content_Search::Dialog_Content_Search(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Content_Search::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Content_Search)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Content_Search::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Content_Search)
	DDX_Control(pDX, IDC_CONTENTLIST, m_content);
	DDX_Control(pDX, IDC_CONTENT_TYPE, m_type);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Content_Search, CDialog)
	//{{AFX_MSG_MAP(Dialog_Content_Search)
	ON_NOTIFY(NM_CLICK, IDC_CONTENTLIST, OnClickContentlist)
	ON_BN_CLICKED(IDC_BUTTON_ADD_CUSTOM, OnButtonAddCustom)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Content_Search message handlers

void Dialog_Content_Search::OnOK() 
{
	m_content.DeleteAllItems();

	CString rBlockName;
	CString rScreenName;
	CString rDescription;
	CString rType;
	CString rScid;
	
	GetDlgItem( IDC_CONTENTBLOCKNAME )->GetWindowText( rBlockName );
	GetDlgItem( IDC_CONTENTSCREENNAME )->GetWindowText( rScreenName );
	GetDlgItem( IDC_CONTENTDESCRIPTION )->GetWindowText( rDescription );	

	if ( m_type.GetCurSel() == CB_ERR ) {
		rType = "";
	}
	else {
		m_type.GetLBText( m_type.GetCurSel(), rType );
	}

	ComponentVec content;
	gComponentList.SearchAndRetrieveContentNames( content,	rBlockName.GetBuffer( 0 ), 
															rScreenName.GetBuffer( 0 ), 
															rDescription.GetBuffer( 0 ), 
															rType.GetBuffer( 0 ) );

	ComponentVec::iterator i;
	int index = 0;
	for ( i = content.begin(); i != content.end(); ++i ) 
	{
		index = m_content.InsertItem( index, (*i).sFuelName.c_str() );		
		m_content.SetItemText( index, 1, (*i).sScreenName.c_str() );
		m_content.SetItemText( index, 2, (*i).sDevName.c_str() );
		index++;
	}	
}

void Dialog_Content_Search::OnCancel() 
{	
	CDialog::OnCancel();
}

BOOL Dialog_Content_Search::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_content.InsertColumn( 0, "Template Name", LVCFMT_LEFT, 200 );
	m_content.InsertColumn( 1, "Screen Name", LVCFMT_LEFT, 200 );
	m_content.InsertColumn( 2, "Description", LVCFMT_LEFT, 1000 );	

	StringVec types;
	StringVec::iterator i;
	gComponentList.GetTemplateTypes( types );
	m_type.AddString( "" );
	for ( i = types.begin(); i != types.end(); ++i ) 
	{
		m_type.AddString( (*i).c_str() );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Content_Search::OnClickContentlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if ( m_content.GetSelectionMark() != -1 ) 
	{
		CString rText = m_content.GetItemText( m_content.GetSelectionMark(), 0 );
		gEditorObjects.SetSelectedTemplateName( rText.GetBuffer( 0 ) );		
	}
	
	*pResult = 0;
}

void Dialog_Content_Search::OnButtonAddCustom() 
{
	for ( int i = 0; i != m_content.GetItemCount(); ++i )
	{
		CString rText;
		rText = m_content.GetItemText( i, 0 );
		
		gCustomViewer.AddToCustomView( rText.GetBuffer( 0 ), IMAGE_GO, gLeftView.GetCustomView(), gLeftView.GetSelectedCustomItem() );
		
	}	
}

void Dialog_Content_Search::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
