// Dialog_Region_Load.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Dialog_Region_Load.h"
#include "resource.h"
#include "EditorRegion.h"
#include "ImageListDefines.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Load dialog


Dialog_Region_Load::Dialog_Region_Load(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Region_Load::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Region_Load)
	//}}AFX_DATA_INIT
}


void Dialog_Region_Load::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Region_Load)
	DDX_Control(pDX, IDC_CHECK_FULL_RECALC, m_fullRecalc);
	DDX_Control(pDX, IDC_CHECK_TUNING_LOAD, m_tuningLoad);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Region_Load, CDialog)
	//{{AFX_MSG_MAP(Dialog_Region_Load)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Region_Load message handlers

BOOL Dialog_Region_Load::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	PublishRegionList( this->GetDlgItem(IDC_REGION_TREE) );	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PublishRegionList( CWnd *pWindow )
{
	CTreeCtrl * pTree = (CTreeCtrl *)pWindow;
	pTree->SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	pTree->SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	pTree->DeleteAllItems();

	StringVec			map_list;
	StringVec			region_list;
	StringVec::iterator	i;
	StringVec::iterator	j;
	
	map_list.clear();
	gEditorRegion.GetMapList( map_list );		
	for ( i = map_list.begin(); i != map_list.end(); i++ ) {
		HTREEITEM hParent = pTree->InsertItem( (char *)(*i).c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, TVI_ROOT, TVI_SORT );		
		region_list.clear();
		gEditorRegion.GetRegionList( (*i), region_list );
		for ( j = region_list.begin(); j != region_list.end(); ++j ) {
			pTree->InsertItem( (char *)(*j).c_str(), IMAGE_REGION, IMAGE_REGION, hParent, TVI_SORT );
		}
	}	
}


void Dialog_Region_Load::OnOK() 
{
	BOOL bLNCRecalc = false;
	BOOL bLoadLights = false;
	BOOL bLoadDecals = false;
	BOOL bLoadFlags = false;

	BOOL bTuningLoad = m_tuningLoad.GetCheck();
	BOOL bFullRecalc = m_fullRecalc.GetCheck();	

	CTreeCtrl * pTree = (CTreeCtrl *)this->GetDlgItem(IDC_REGION_TREE);
	CString sRegion = pTree->GetItemText( pTree->GetSelectedItem() );
	int image = 0, selImage = 0;
	pTree->GetItemImage( pTree->GetSelectedItem(), image, selImage );
	if ( image != IMAGE_REGION )
	{
		MessageBoxEx( this->GetSafeHwnd(), "Please select a region.", "Region Selection", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}

	HTREEITEM hParent = pTree->GetParentItem( pTree->GetSelectedItem() );
	CString sMap	= pTree->GetItemText( hParent );	
	
	gEditorRegion.LoadRegion( sRegion.GetBuffer( sRegion.GetLength() ), sMap.GetBuffer( sMap.GetLength() ), bLNCRecalc ? true : false, bLoadLights ? true : false, bLoadDecals ? true : false, bTuningLoad ? true : false, bLoadFlags ? true : false, bFullRecalc ? true : false );
	
	CDialog::OnOK();
}

void Dialog_Region_Load::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);	
}

