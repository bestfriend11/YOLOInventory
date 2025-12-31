// LeftView.cpp : implementation of the CLeftView class
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"

#include "SiegeEditorDoc.h"
#include "LeftView.h"
#include "ComponentList.h"
#include "ImageListDefines.h"
#include "EditorTerrain.h"
#include "CustomViewer.h"
#include "Preferences.h"
#include "EditorObjects.h"
#include "MeshPreviewer.h"
#include "GoData.h"
#include "FileSys.h"
#include "FileSysUtils.h"
#include "FuelDB.h"
#include "EditorRegion.h"
#include "MenuCommands.h"
#include "EditorMap.h"
#include "namingkey.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CView)

BEGIN_MESSAGE_MAP(CLeftView, CView)
	//{{AFX_MSG_MAP(CLeftView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_RBUTTONUP()
	ON_WM_PARENTNOTIFY()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftView construction/destruction

CLeftView::CLeftView()
	: m_hHitItem( TVI_ROOT )
	, m_bItemDragging( false )
	, m_pDragImageList( 0 )
	, m_selectedCustomItem( TVI_ROOT )
	, m_bLoadedNodeMeshTree( false )
	, m_bLoadedNodeGlobalTree( false )
	, m_hTerrainNodes( TVI_ROOT )
{
	// TODO: add construction code here

}

CLeftView::~CLeftView()
{
	m_wndMainMenu.DestroyMenu();
	m_wndCustomMenu.DestroyMenu();
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView drawing

void CLeftView::OnDraw(CDC* pDC)
{
	CSiegeEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}


void CLeftView::OnInitialUpdate()
{
	CView::OnInitialUpdate();	
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSiegeEditorDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSiegeEditorDoc)));
	return (CSiegeEditorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

int CLeftView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CRect rectDummy;
	rectDummy.SetRectEmpty ();

	// Create tabs window:
	if (!m_wndTabs.Create( TCS_BOTTOM | WS_CHILD | WS_VISIBLE, rectDummy, this, WORKSPACE_TAB))
	{
		TRACE0("Failed to create workspace tab window\n");
		return -1;      // fail to create
	}

	CFont font;
	font.CreateStockObject( DEFAULT_GUI_FONT );
	m_wndTabs.SetFont( &font );

	//m_wndTabs.SetImageList (IDB_WORKSPACE, 16, RGB (255, 0, 255));

	// Create tree windows.
	// TODO: create your own tab windows here:
	const DWORD dwViewStyle =	WS_CHILD | WS_VISIBLE | TVS_HASLINES | 
								TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS;
	
	if (!m_wndTree1.Create (dwViewStyle, rectDummy, &m_wndTabs, TREE_MAIN) ||
		!m_wndTree2.Create (dwViewStyle, rectDummy, &m_wndTabs, TREE_CUSTOM))
	{
		TRACE0("Failed to create workspace view\n");
		return -1;      // fail to create
	}

	SetWindowLong( m_wndTree1.m_hWnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE );
	SetWindowLong( m_wndTree2.m_hWnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE );

	m_wndTree2.ShowWindow( SW_HIDE );

	// Attach tree windows to tab:
	m_wndTabs.InsertItem( TCIF_TEXT, 0, "Main", 0, 0, 0, 0 );
	m_wndTabs.InsertItem( TCIF_TEXT, 1, "Custom View", 0, 0, 0, 0 );	
	
	RECT rSpin = { 0, 0, 15, 25 };
	RECT rEdit = { 0, 0, 40, 200 };
	RECT rStatic1 = { 0, 0, 70, 20 };
	RECT rStatic2 = { 0, 0, 100, 20 };
	RECT rButton = { 0, 0, 100, 20 };

	m_wndAttachButton.Create( "Attach Node", WS_CHILD | WS_VISIBLE, rButton, this, ID_ATTACH_BUTTON );

	m_wndSourceSpin.Create( WS_CHILD | WS_VISIBLE, rSpin, this, SOURCE_SPINNER );
	m_wndDestSpin.Create( WS_CHILD | WS_VISIBLE, rSpin, this, DEST_SPINNER );
	m_wndSourceCombo.Create( WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_CLIPSIBLINGS | BS_GROUPBOX | CBS_HASSTRINGS | WS_VSCROLL, 
							rEdit, this, SOURCE_COMBO );
	m_wndDestCombo.Create( WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_CLIPSIBLINGS | BS_GROUPBOX | CBS_HASSTRINGS | WS_VSCROLL,
							rEdit, this, DEST_COMBO );	
	
	m_wndStaticSource.Create( "Source Doors:", WS_CHILD | WS_VISIBLE, rStatic1, this );
	m_wndStaticDest.Create( "Destination Doors:", WS_CHILD | WS_VISIBLE, rStatic2, this );

	m_wndStaticSource.SetWindowText( "Source Doors:" );
	m_wndStaticDest.SetWindowText( "Destination Doors:" );
		
	m_wndStaticSource.SetFont( &font );
	m_wndStaticDest.SetFont( &font );	
	m_wndAttachButton.SetFont( &font );

	CreatePopupMenus();
	
	return 0;
}

void CLeftView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	// Tab control should cover a whole client area:
	m_wndTabs.SetWindowPos (NULL, -1, -1, cx, cy-100,
		SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

	const int edit_width = 40;
	const int spin_width = 15;
	const int bottom_dist = 35;
	RECT rect;
	m_wndTabs.GetClientRect( &rect );

	// Set the tree control sizes
	m_wndTree1.SetWindowPos( NULL, rect.left+5, rect.top+5, cx-10, cy-130, SWP_NOACTIVATE );
	m_wndTree2.SetWindowPos( NULL, rect.left+5, rect.top+5, cx-10, cy-130, SWP_NOACTIVATE );

	rect.left += 10;
	m_wndSourceCombo.SetWindowPos( NULL, rect.left, rect.bottom+bottom_dist, 0, 0, SWP_NOSIZE    ); 
	m_wndSourceSpin.SetWindowPos( NULL, rect.left+edit_width, rect.bottom+bottom_dist, 0, 0, SWP_NOSIZE    ); 
	m_wndStaticSource.SetWindowPos( NULL, rect.left, rect.bottom+10, 0, 0, SWP_NOSIZE    ); 

	m_wndDestCombo.SetWindowPos( NULL, rect.left+edit_width*2, rect.bottom+bottom_dist, 0, 0, SWP_NOSIZE    ); 
	m_wndDestSpin.SetWindowPos( NULL, rect.left+edit_width*3, rect.bottom+bottom_dist, 0, 0, SWP_NOSIZE    ); 
	m_wndStaticDest.SetWindowPos( NULL, rect.left+edit_width*2, rect.bottom+10, 0, 0, SWP_NOSIZE    ); 

	m_wndAttachButton.SetWindowPos( NULL, rect.left+30, rect.bottom+bottom_dist+40, 0, 0, SWP_NOSIZE    ); 	
}

void CLeftView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	dc.SetBkColor( GetSysColor(COLOR_MENU) );
	CBrush brush;
	brush.CreateSolidBrush( GetSysColor(COLOR_3DFACE) );
	RECT rect;
	this->GetClientRect( &rect );
	dc.FillRect( &rect, &brush );	

	// Do not call CView::OnPaint() for painting messages
}

BOOL CLeftView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{	
	NMHDR * pNm = (NMHDR *)lParam;
	switch ( pNm->idFrom ) {
	case WORKSPACE_TAB:
		{
			switch ( pNm->code ) {
			case TCN_SELCHANGE:
				{
					if ( pNm->hwndFrom == m_wndTabs.m_hWnd ) {
						if ( m_wndTabs.GetCurFocus() == 0 ) {
							m_wndTree1.ShowWindow( SW_SHOW );
							m_wndTree2.ShowWindow( SW_HIDE );
							gPreferences.SetCustomTabActive( false );
							PublishDestDoorList();
						}
						else {
							m_wndTree1.ShowWindow( SW_HIDE );
							m_wndTree2.ShowWindow( SW_SHOW );
							gPreferences.SetCustomTabActive( true );
							PublishDestDoorList();
						}
					}
				}
				break;
			}
		}
		break;
	case TREE_MAIN:
	case TREE_CUSTOM:
		{
			switch ( pNm->code ) {
			case TVN_SELCHANGED:
				{
					PublishDestDoorList();

					DWORD dwData = 0;
					gpstring sTemplateName;
					CString rText;
					bool bDecal = false;
					
					if ( pNm->idFrom == TREE_MAIN ) {
						dwData = m_wndTree1.GetItemData( m_wndTree1.GetSelectedItem() );
						gComponentList.GetTemplateNameFromMapId( dwData, sTemplateName );
						rText = m_wndTree1.GetItemText( m_wndTree1.GetSelectedItem() );

						int nImage = 0;
						int nSelectedImage = 0;
						m_wndTree1.GetItemImage( m_wndTree1.GetSelectedItem(), nImage, nSelectedImage );						
						if ( nImage == IMAGE_DECAL )
						{
							bDecal = true;
						}
					}
					else if ( pNm->idFrom == TREE_CUSTOM ) {						
						dwData = m_wndTree2.GetItemData( m_wndTree2.GetSelectedItem() );
						gComponentList.GetTemplateNameFromMapId( dwData, sTemplateName );
						rText = m_wndTree2.GetItemText( m_wndTree2.GetSelectedItem() );						
						m_selectedCustomItem = m_wndTree2.GetSelectedItem();
						
						int nImage = 0;
						int nSelectedImage = 0;
						m_wndTree2.GetItemImage( m_wndTree2.GetSelectedItem(), nImage, nSelectedImage );						
						if ( nImage == IMAGE_DECAL )
						{
							bDecal = true;
						}
					}					

					if (( rText.CompareNoCase( "Spotlight" ) == 0 ) || ( rText.CompareNoCase( "Point Light" ) == 0 ) || 
						( rText.CompareNoCase( "Hotpoint" ) == 0 ) || ( rText.CompareNoCase( "Starting Position" ) == 0 ) || bDecal )
					{
						sTemplateName = rText.GetBuffer( 0 );
						gEditorObjects.SetSelectedTemplateName( sTemplateName );

						if ( bDecal )
						{
							gPreviewer.LoadPreviewObject( sTemplateName );
						}
					}
					else if ( dwData != 0 ) 					
					{	
						gPreviewer.LoadPreviewObject( sTemplateName );						
						gEditorObjects.SetSelectedTemplateName( sTemplateName );
					}
					else 
					{						
						if ( IsSNOFile( rText.GetBuffer( rText.GetLength() ) ) ) 
						{
							gpstring sNode = gComponentList.GetNodeGUID( rText.GetBuffer( rText.GetLength() ) );
							gPreviewer.LoadPreviewNode( sNode ); 							
						}
						gEditorObjects.SetSelectedTemplateName( "" );
					}					

				}
				break;
			case NM_RCLICK:
				{
					CMenu menu;

					POINT pt = { 0, 0 };
					GetCursorPos( &pt );										

					POINT ptTree = pt;
					if ( pNm->idFrom == TREE_MAIN ) 
					{
						m_wndTree1.ScreenToClient( &ptTree );
					}
					else
					{
						m_wndTree2.ScreenToClient( &ptTree );
					}

					unsigned int nFlags = 0;
					
					if ( pNm->idFrom == TREE_MAIN ) 
					{
						m_hHitItem = m_wndTree1.HitTest( ptTree, &nFlags );
					}
					else
					{
						m_hHitItem = m_wndTree2.HitTest( ptTree, &nFlags );
					}

					if ( m_hHitItem != NULL )
					{
						CString rTest;

						if ( pNm->idFrom == TREE_MAIN ) 
						{
							rTest = m_wndTree1.GetItemText( m_hHitItem );
						}
						else
						{
							rTest = m_wndTree2.GetItemText( m_hHitItem );
						}						
					}

					if ( m_hHitItem == NULL ) 
					{
						m_hHitItem = TVI_ROOT;
					}
					else
					{
						if ( pNm->idFrom == TREE_MAIN ) 
						{
							m_wndTree1.Select( m_hHitItem, TVGN_CARET );
						}
						else
						{
							m_wndTree2.Select( m_hHitItem, TVGN_CARET );
						}
					}

					if ( pNm->idFrom == TREE_MAIN ) {						
						m_wndMainMenu.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );						
					}
					else if ( pNm->idFrom == TREE_CUSTOM ) {
						m_wndCustomMenu.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, this, NULL );
					}					
				}
				break;
			case TVN_BEGINDRAG:
				{					
					if ( pNm->idFrom == TREE_CUSTOM )
					{
						NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNm;  
						HTREEITEM h = pNMTreeView->itemNew.hItem;
						
						if ( m_wndTree2.GetSelectedItem() != h )
						{
							m_wndTree2.SelectItem( h );
						}

						int nImage = 0, nSelectedImage = 0;
						m_wndTree2.GetItemImage( h, nImage, nSelectedImage );
						if ( nImage == IMAGE_3BOX )
						{
							break;
						}

						if ( m_pDragImageList ) {
							delete m_pDragImageList;
							m_pDragImageList = 0;
						}
						m_pDragImageList = m_wndTree2.CreateDragImage( m_wndTree2.GetSelectedItem() );					
						m_pDragImageList->BeginDrag( 0, CPoint( 0, 0 ) );										

						MapWindowPoints(NULL, &pNMTreeView->ptDrag, 1);
						m_pDragImageList->DragEnter( NULL, pNMTreeView->ptDrag );
						
						// ShowCursor( FALSE ); 
						SetCapture();
					
						m_bItemDragging = true;						
					}
				}
				break;
			case TVN_BEGINLABELEDIT:
				{					
					HTREEITEM h = m_wndTree2.GetSelectedItem();
					
					int image = 0;
					int selImage = 0;
					m_wndTree2.GetItemImage( h, image, selImage );

					if ( (image != IMAGE_CLOSEFOLDER) || (image != IMAGE_OPENFOLDER)  )
					{
						return TRUE;
					}
				}
				break;
			case TVN_ENDLABELEDIT:
				{					
					NMTVDISPINFO * ptvdi = (NMTVDISPINFO *) lParam; 
					HTREEITEM h = m_wndTree2.GetSelectedItem();
					
					int image = 0;
					int selImage = 0;
					m_wndTree2.GetItemImage( h, image, selImage );

					if ( (image == IMAGE_CLOSEFOLDER) || (image == IMAGE_OPENFOLDER)  )
					{
						m_wndTree2.SetItemText( h, ((*ptvdi).item).pszText );
						return TRUE;
					}					
				}
				break;
			}			
		}
		break;
	case SOURCE_SPINNER:
		{
			NMUPDOWN *pUpdown = (LPNMUPDOWN) lParam;
			if ( pUpdown->iDelta >= 0 ) {
				int index = m_wndSourceCombo.GetCurSel();
				if ( index <= 0 ) {
					index = GetNumSourceDoors();
				}
				else {
					index--;
				}
				m_wndSourceCombo.SetCurSel( index );		

				if ( m_wndSourceCombo.GetCount() == 0 ) {
					return FALSE;
				}

				CString rSelection;
				m_wndSourceCombo.GetLBText( index, rSelection );
				gEditorTerrain.SetSelectedSourceDoorID( atoi( rSelection.GetBuffer( rSelection.GetLength() ) ) );		
			}
			else {
				int index = m_wndSourceCombo.GetCurSel();
				if ( index >= GetNumSourceDoors() ) {
					index = 0;
				}
				else {
					index++;
				}
				m_wndSourceCombo.SetCurSel( index );		

				if ( m_wndSourceCombo.GetCount() == 0 ) {
					return FALSE;
				}

				CString rSelection;
				m_wndSourceCombo.GetLBText( index, rSelection );
				gEditorTerrain.SetSelectedSourceDoorID( atoi( rSelection.GetBuffer( rSelection.GetLength() ) ) );					
			}
		}
		break;
	case DEST_SPINNER:
		{
			NMUPDOWN *pUpdown = (LPNMUPDOWN) lParam;
			if ( pUpdown->iDelta >= 0 ) {			
				int index = m_wndDestCombo.GetCurSel();
				if ( index <= 0 ) {
					index = GetNumDestDoors();
				}
				else {
					index--;
				}
				m_wndDestCombo.SetCurSel( index );
				
				if ( m_wndDestCombo.GetCount() == 0 ) {
					return FALSE;
				}

				CString rSelection;
				m_wndDestCombo.GetLBText( index, rSelection );

				CString rSource;
				m_wndSourceCombo.GetLBText( m_wndSourceCombo.GetCurSel(), rSource );
				
				// Attach the nodes
				RequestAttach(	rSource.GetBuffer( rSource.GetLength() ), 
								rSelection.GetBuffer( rSelection.GetLength() ) );									
			}
			else {				
				int index = m_wndDestCombo.GetCurSel();
				if ( index >= GetNumDestDoors() ) {
					index = 0;
				}
				else {
					index++;
				}
				m_wndDestCombo.SetCurSel( index );
				
				if ( m_wndDestCombo.GetCount() == 0 ) {
					return FALSE;
				}

				CString rSelection;
				m_wndDestCombo.GetLBText( index, rSelection );

				if ( m_wndSourceCombo.GetCurSel() == CB_ERR ) {
					return FALSE;
				}

				CString rSource;
				m_wndSourceCombo.GetLBText( m_wndSourceCombo.GetCurSel(), rSource );
				
				// Attach the nodes
				RequestAttach(	rSource.GetBuffer( rSource.GetLength() ), 
								rSelection.GetBuffer( rSelection.GetLength() ) );									
			}
		}
		break;
	case SOURCE_COMBO:
		{
			switch ( pNm->code ) {
			case CBN_EDITCHANGE:
				{
					int index = m_wndSourceCombo.GetCurSel();					
					CString rSelection;
					m_wndSourceCombo.GetLBText( index, rSelection );
					gEditorTerrain.SetSelectedSourceDoorID( atoi( rSelection.GetBuffer( rSelection.GetLength() ) ) );
				}
				break;
			}
		}
		break;
	case DEST_COMBO:
		{
			switch ( pNm->code ) {
			case CBN_EDITCHANGE:
				{
					int index = m_wndDestCombo.GetCurSel();					
					CString rSelection;
					m_wndDestCombo.GetLBText( index, rSelection );					
					gEditorTerrain.SetDestConnectionID( atoi( rSelection.GetBuffer( rSelection.GetLength() ) ) );
				}
				break;
			}
		}
		break;	
	}
	
	return CView::OnNotify(wParam, lParam, pResult);
}


void CLeftView::RequestAttach( char * szSource, char * szDest )
{		
	if (( m_wndSourceCombo.GetCurSel() != CB_ERR ) && ( m_wndDestCombo.GetCurSel() != CB_ERR )) {
		gEditorTerrain.AttachSiegeNodes( atoi( szSource ), atoi( szDest ) );
	}
}



void CLeftView::InitializeMainTreeData()
{
	//m_ToolTips.Create( this );
	//m_wndTree1.SetToolTips( &m_ToolTips );
	m_wndTree1.DeleteAllItems();
	PublishNodeList();
	PublishObjectList();
	PublishNonParentObjectList();
}



void CLeftView::InitializeCustomTreeData()
{
	m_wndTree2.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	m_wndTree2.SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	m_wndTree2.DeleteAllItems();
	gCustomViewer.LoadAllCustomViews( m_wndTree2 );
}


void CLeftView::PublishNodeList()
{
	m_wndTree1.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	m_wndTree1.SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	m_wndTree1.DeleteAllItems();	
	m_hTerrainNodes = m_wndTree1.InsertItem( "Terrain Nodes", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );			
	PublishMeshNodeList();
}


void CLeftView::InitializeNodeList( bool bForce )
{
	if ( gEditorMap.GetHasNodeMeshIndex() )
	{
		if ( !GetLoadedNodeMeshTree() || bForce )
		{
			SetLoadedNodeMeshTree( true );
			SetLoadedNodeGlobalTree( false );
			PublishMeshNodeList();
		}	
	}
	else
	{
		if ( !GetLoadedNodeGlobalTree() || bForce )
		{
			SetLoadedNodeMeshTree( true );
			SetLoadedNodeGlobalTree( false );
			PublishGlobalNodeList();			
		}
	}
}


void CLeftView::PublishGlobalNodeList()
{	
	m_wndTree1.DeleteItem( m_hTerrainNodes );
	m_hTerrainNodes = m_wndTree1.InsertItem( "Terrain Nodes", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );

	StringVec	node_list;
	StringVec::iterator	i;	
	node_list.clear();
	gComponentList.RetrieveNodeCollection( node_list );

	for ( i = node_list.begin(); i != node_list.end(); i++ ) 
	{
		HTREEITEM hParent = CheckParentDirectory( (*i), m_wndTree1.m_hWnd, m_hTerrainNodes );
		if ( hParent == 0 ) 
		{
			hParent = m_hTerrainNodes;
		}
		HTREEITEM hItem = m_wndTree1.InsertItem( RemoveDirectories((char *)((*i).c_str())), IMAGE_SN, IMAGE_SN, hParent, TVI_SORT );		
	}
}


void CLeftView::PublishMeshNodeList()
{
	m_wndTree1.DeleteItem( m_hTerrainNodes );
	m_hTerrainNodes = m_wndTree1.InsertItem( "Terrain Nodes", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );

	StringVec	node_list;
	StringVec::iterator	i;	
	node_list.clear();
	gComponentList.RetrieveNodeMeshCollection( node_list );

	for ( i = node_list.begin(); i != node_list.end(); i++ ) 
	{
		HTREEITEM hParent = CheckParentDirectory( (*i), m_wndTree1.m_hWnd, m_hTerrainNodes );
		if ( hParent == 0 ) 
		{
			hParent = m_hTerrainNodes;
		}
		HTREEITEM hItem = m_wndTree1.InsertItem( RemoveDirectories((char *)((*i).c_str())), IMAGE_SN, IMAGE_SN, hParent, TVI_SORT );		
	}
}


HTREEITEM CLeftView::CheckParentDirectory( gpstring item_name, HWND hWndTree, HTREEITEM hTop )
{
	std::vector< DIRECTORY_FOLDER > directory_names;
	
	int numStrings = stringtool::GetNumDelimitedStrings( item_name, '\\' );
	bool bForwardSlash = false;

	if ( numStrings == 0 )
	{
		numStrings = stringtool::GetNumDelimitedStrings( item_name, '/' );
		bForwardSlash = true;
	}

	for ( int i = 0; i < numStrings-1; ++i ) 
	{
		DIRECTORY_FOLDER dir;				
		gpstring sDirText = dir.text;
		if ( bForwardSlash )
		{
			stringtool::GetDelimitedValue( item_name, '/', i, sDirText );
		}
		else
		{
			stringtool::GetDelimitedValue( item_name, '\\', i, sDirText );
		}
		dir.text = sDirText;
		dir.hItem = 0;
		directory_names.push_back( dir );
	}

	TVITEM			tvi;
	TVINSERTSTRUCT	tvis;
	char			szText[1024] = "";
	
	HTREEITEM hChild	= TreeView_GetChild( hWndTree, hTop );
	tvi.hItem			= hChild;
	tvi.mask			= TVIF_TEXT;
	tvi.pszText			= szText;
	tvi.cchTextMax		= sizeof( szText );
	TreeView_GetItem( hWndTree, &tvi );

	std::vector< DIRECTORY_FOLDER >::iterator j;
	for ( j = directory_names.begin(); j != directory_names.end(); ++j ) {
		while ( hChild ) {
			if ( strcmp( tvi.pszText, (*j).text.c_str() ) ) {
				hChild	= TreeView_GetNextSibling( hWndTree, hChild );
				tvi.hItem				= hChild;
				tvi.mask				= TVIF_TEXT;
				tvi.pszText				= szText;
				tvi.cchTextMax			= sizeof( szText );
				TreeView_GetItem( hWndTree, &tvi );
			}
			else {
				(*j).hItem = hChild;
				hChild = TreeView_GetChild( hWndTree, hChild );
				tvi.hItem				= hChild;
				tvi.mask				= TVIF_TEXT;
				tvi.pszText				= szText;
				tvi.cchTextMax			= sizeof( szText );
				TreeView_GetItem( hWndTree, &tvi );
				if ( hChild != NULL ) {
					++j;
					if ( j != directory_names.end() ) {
						continue;
					}
					else {
						goto RESUME;
					}
				}
			}
		}
	}

RESUME:
	std::vector< DIRECTORY_FOLDER >::iterator k;
	HTREEITEM hActiveDir = hTop;
	for ( k = directory_names.begin(); k != directory_names.end(); ++k ) {
		if ( (*k).hItem ) {
			hActiveDir = (*k).hItem; 
			continue;
		}
		else {
			tvi.cChildren		= 1;
			tvi.cchTextMax		= MAX_PATH;
			tvi.iImage			= IMAGE_CLOSEFOLDER;
			tvi.iSelectedImage	= IMAGE_CLOSEFOLDER;
			tvi.lParam			= 0;
			tvi.mask			= TVIF_HANDLE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
			tvi.pszText			= (char *)(*k).text.c_str();
			tvi.state			= 0;
			tvi.stateMask		= 0;

			std::vector< DIRECTORY_FOLDER >::iterator l;
			if ( k != directory_names.begin() ) {
				l = --k;
				tvis.hParent		=  (*l).hItem;
				k++;
			}
			else {
				tvis.hParent = hTop;
			}
			
			tvis.hInsertAfter	= TVI_SORT;
			tvis.item			= tvi;
			tvi.hItem			= NULL;

			tvi.hItem			= TreeView_InsertItem( hWndTree, &tvis );

			(*k).hItem = tvi.hItem;
			hActiveDir = tvi.hItem;
		}
	}
	return hActiveDir;
}



void CLeftView::PublishObjectList()
{	
	HTREEITEM hTreeGO = m_wndTree1.InsertItem( "Game Objects", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );
		
	// HTREEITEM hSpecial = m_wndTree1.InsertItem( "Specialization Sorted", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, hTreeGO, TVI_SORT );
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveInsertTemplate( (*i) );
	}	

	PublishViewSearchObjectList( hTreeGO );

	if ( !m_wndTree1.ItemHasChildren( hTreeGO ) )
	{
		MessageBoxEx( this->GetSafeHwnd(), "No objects found in the search categories, reverting to specialization object listing. Do you have your SiegeEditorExtras.dres in the right directory?", "Object Tree Listing", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );

		for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
		{
			RecursiveInsertTemplateToTree( (*i), hTreeGO );
		}	
	}
}


void CLeftView::PublishViewSearchObjectList( HTREEITEM hItem )
{		
	ViewSearchFolderVec searchFolders;
	ViewSearchFolderVec::iterator i;

	gComponentList.LoadViewSearchDb( searchFolders );
	gComponentList.ViewSearch( searchFolders );

	HTREEITEM hRoot = hItem;

	for ( i = searchFolders.begin(); i != searchFolders.end(); ++i )
	{
		if ( (*i).sParent.empty() )
		{
			hItem = m_wndTree1.InsertItem( (*i).sName.c_str(), IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, hRoot, TVI_SORT );
			m_loader_map.insert( std::pair< gpstring, HTREEITEM >( (*i).sName, hItem ) );			

			ViewSearchIdMatches::iterator j;
			for ( j = (*i).matches.begin(); j != (*i).matches.end(); ++j )
			{
				gpstring sTemplate;
				gComponentList.GetTemplateNameFromMapId( (*j), sTemplate );
				ObjectComponent oc = gComponentList.GetObjectComponent( sTemplate );

				HTREEITEM hChild = m_wndTree1.InsertItem( oc.sName.c_str(), IMAGE_GO, IMAGE_GO, hItem, TVI_SORT );
				m_wndTree1.SetItemData( hChild, *j );
			}			
		}
		else
		{
			HTREEITEM hFound = FindItemByName( (*i).sParent );
			if ( hFound == NULL ) 
			{
				continue;
			}

			hItem = m_wndTree1.InsertItem( (*i).sName.c_str(), IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, hFound, TVI_SORT );			
			
			ViewSearchIdMatches::iterator j;
			for ( j = (*i).matches.begin(); j != (*i).matches.end(); ++j )
			{
				gpstring sTemplate;
				gComponentList.GetTemplateNameFromMapId( (*j), sTemplate );
				ObjectComponent oc = gComponentList.GetObjectComponent( sTemplate );

				HTREEITEM hChild = m_wndTree1.InsertItem( oc.sName.c_str(), IMAGE_GO, IMAGE_GO, hItem, TVI_SORT );
				m_wndTree1.SetItemData( hChild, *j );
			}

			m_loader_map.insert( std::pair< gpstring, HTREEITEM >( (*i).sName, hItem ) );
		}
	}

	m_wndTree1.SortChildren( hRoot );
}


HTREEITEM CLeftView::FindItemByName( gpstring sName )
{
	std::map< gpstring, HTREEITEM, istring_less >::iterator i;
	i = m_loader_map.find( sName );
	if ( i == m_loader_map.end() ) 
	{
		return 0;
	}
	return (*i).second;
}


void CLeftView::PublishNonParentObjectList()
{
	HTREEITEM hLights = m_wndTree1.InsertItem( "Lights", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );
		
	m_wndTree1.SetItemData( m_wndTree1.InsertItem( "Point Light", IMAGE_LIGHT, IMAGE_LIGHT, hLights, TVI_SORT ), -1 );
	m_wndTree1.SetItemData( m_wndTree1.InsertItem( "Spotlight", IMAGE_LIGHT, IMAGE_LIGHT, hLights, TVI_SORT ), -1 );	

	HTREEITEM hNonMesh = m_wndTree1.InsertItem( "Siege Objects", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );	
	m_wndTree1.SetItemData( m_wndTree1.InsertItem( "Starting Position", IMAGE_STARTINGPOS, IMAGE_STARTINGPOS, hNonMesh, TVI_SORT ), -1 );

	HTREEITEM hDecals = m_wndTree1.InsertItem( "Decals", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, hNonMesh, TVI_SORT );

	FileSys::RecursiveFinder finder( "art/bitmaps/decals/*", 0 );
	gpstring path;
	while ( finder.GetNext( path ) )
	{
		if ( FileSys::IFileMgr::IsAliasedFrom( path, ".%img%" ) )
		{
			HTREEITEM hParent = CheckParentDirectory( path, m_wndTree1.m_hWnd, hDecals );
			if ( hParent == NULL )
			{
				hParent = hDecals;
			}

			gpstring sLocation;
			if ( gNamingKey.BuildContentLocation( path.c_str(), sLocation ) )
			{
				m_wndTree1.SetItemData( m_wndTree1.InsertItem( path, IMAGE_DECAL, IMAGE_DECAL, hParent, TVI_SORT ), -1 );	
			}
		}
	}
}


void CLeftView::RecursiveInsertTemplate( GoDataTemplate * pTemplate )
{
	if ( pTemplate->GetChildBegin() == pTemplate->GetChildEnd() )
	{
		gComponentList.InsertTemplateId( pTemplate->GetName() );
	}

	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveInsertTemplate( (*i) );
	}
}


void CLeftView::RecursiveInsertTemplateToTree( GoDataTemplate * pTemplate, HTREEITEM hParent )
{
	HTREEITEM hItem;
	if ( pTemplate->GetChildBegin() != pTemplate->GetChildEnd() )
	{
		hItem = m_wndTree1.InsertItem( pTemplate->GetName(), IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, hParent, TVI_SORT );
	}
	else
	{
		hItem = m_wndTree1.InsertItem( pTemplate->GetName(), IMAGE_GO, IMAGE_GO, hParent, TVI_SORT );
		m_wndTree1.SetItemData( hItem, gComponentList.GetCurrentTemplateId() );
		gComponentList.InsertTemplateId( pTemplate->GetName() );
	}

	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveInsertTemplateToTree( (*i), hItem );
	}
}


void CLeftView::PublishDestDoorList()
{	
	CString rNode;
	if ( m_wndTabs.GetCurFocus() == 0 ) 
	{
		rNode = m_wndTree1.GetItemText( m_wndTree1.GetSelectedItem() );
	}
	else 
	{
		rNode = m_wndTree1.GetItemText( m_wndTree2.GetSelectedItem() );
	}

	m_wndDestCombo.ResetContent();	
	StringVec dest_door_list;	
	gpstring sGuid = gComponentList.GetNodeGUID( rNode.GetBuffer( rNode.GetLength() ) );
	if ( sGuid.empty() ) 
	{
		return;
	}

	siege::database_guid DestNode = siege::database_guid( sGuid.c_str() );
	gEditorTerrain.SetDestDoors( DestNode );	
	dest_door_list = gEditorTerrain.GetDestDoors();
		
	StringVec::iterator i;
	for ( i = dest_door_list.begin(); i != dest_door_list.end(); ++i ) 
	{
		m_wndDestCombo.AddString( (*i).c_str() );
	}

	m_destNum = dest_door_list.size()-1;
}


void CLeftView::PublishDestDoorList( const gpstring & sSno )
{
	m_wndDestCombo.ResetContent();	
	StringVec dest_door_list;	
	gpstring sGuid = gComponentList.GetNodeGUID( (char *)sSno.c_str() );
	if ( sGuid.empty() ) 
	{
		return;
	}

	siege::database_guid DestNode = siege::database_guid( sGuid.c_str() );
	gEditorTerrain.SetDestDoors( DestNode );	
	dest_door_list = gEditorTerrain.GetDestDoors();
		
	StringVec::iterator i;
	for ( i = dest_door_list.begin(); i != dest_door_list.end(); ++i ) 
	{
		m_wndDestCombo.AddString( (*i).c_str() );
	}

	m_destNum = dest_door_list.size()-1;
}


void CLeftView::SetDestDoor( int door )
{
	gpstring sDoor;
	sDoor.assignf( "%d", door );
	int found = m_wndDestCombo.FindString( 0, sDoor.c_str() );
	if ( found != CB_ERR )
	{
		m_wndDestCombo.SetCurSel( found );
	}
}


void CLeftView::PublishSourceDoorList()
{
	m_wndSourceCombo.ResetContent();
	StringVec source_door_list;

	source_door_list = gEditorTerrain.GetSourceDoors();
	
	StringVec::iterator i;
	for ( i = source_door_list.begin(); i != source_door_list.end(); ++i ) {
		m_wndSourceCombo.AddString( (*i).c_str() );		
	}

	m_wndSourceCombo.SetCurSel( 0 );
	m_sourceNum = source_door_list.size()-1;	
}

bool IsSNOFile( char *szFile )
{
	UINT i = strlen( szFile ) - 3;
	if (( szFile[i] == 's' ) || ( szFile[i] == 'S' )) {
		if (( szFile[i+1] == 'n' ) || ( szFile[i+1] == 'N' )) {
			if (( szFile[i+2] == 'o' ) || ( szFile[i+2] == 'O' )) {
				return true;
			}
		}
	}
	return false;
}

BOOL CLeftView::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if ( LOWORD(wParam) == ID_ATTACH_BUTTON ) {
		if ( HIWORD(wParam) == BN_CLICKED ) {

			if ( m_wndDestCombo.GetCurSel() == CB_ERR || m_wndSourceCombo.GetCurSel() == CB_ERR ) {
				return CView::OnCommand(wParam, lParam);
			}

			CString rSelection;
			m_wndDestCombo.GetLBText( m_wndDestCombo.GetCurSel(), rSelection );

			CString rSource;
			m_wndSourceCombo.GetLBText( m_wndSourceCombo.GetCurSel(), rSource );
		
			// Attach the nodes
			RequestAttach(	rSource.GetBuffer( rSource.GetLength() ), 
							rSelection.GetBuffer( rSelection.GetLength() ) );
			
		}
	}	
	
	switch ( wParam ) {
	case ID_ADDTOCUSTOMVIEW:		
		gCustomViewer.AddToCustomView( m_wndTree1.GetSelectedItem(), m_wndTree1, m_wndTree2, GetSelectedCustomItem() );
		break;
	case ID_NEWFOLDER:					
		gCustomViewer.NewFolder( m_wndTree2, m_hHitItem );
		break;
	case ID_RENAMEFOLER:
		gCustomViewer.RenameFolder( m_wndTree2, m_hHitItem );
		break;
	case ID_REMOVESELECTED:
		gCustomViewer.RemoveFromCustomView( m_wndTree2 );
		break;
	case ID_LOADALLCUSTOMVIEWS:
		gCustomViewer.LoadAllCustomViews( m_wndTree2 );
		break;
	case ID_SAVECUSTOMVIEW:
		gCustomViewer.SaveCustomView( m_wndTree2 );
		break;
	case ID_VIEW_MESHPREVIEWER:
		{
			MenuFunctions mf;
			mf.MeshPreviewer();

			if ( !gEditorObjects.GetSelectedTemplateName().empty() )
			{
				if ( gEditorObjects.GetSelectedTemplateName().same_no_case( "Spotlight" ) || 
					 gEditorObjects.GetSelectedTemplateName().same_no_case( "Point Light" ) || 
					 gEditorObjects.GetSelectedTemplateName().same_no_case( "Hotpoint" ) || 
					 gEditorObjects.GetSelectedTemplateName().same_no_case( "Starting Position" ) )
				{
					break;
				}

				gPreviewer.LoadPreviewObject( gEditorObjects.GetSelectedTemplateName() );				
			}
			else if ( gEditorTerrain.GetSecondaryNode() != siege::UNDEFINED_GUID )
			{
				gPreviewer.LoadPreviewNode( gEditorTerrain.GetSecondaryNode().ToString() ); 				
			}
		}
		break;
	}
	
	return CView::OnCommand(wParam, lParam);
}


void CLeftView::OnAttach()
{
	if ( m_wndDestCombo.GetCurSel() == CB_ERR || m_wndSourceCombo.GetCurSel() == CB_ERR ) {
		return;
	}

	CString rSelection;
	m_wndDestCombo.GetLBText( m_wndDestCombo.GetCurSel(), rSelection );

	CString rSource;
	m_wndSourceCombo.GetLBText( m_wndSourceCombo.GetCurSel(), rSource );

	// Attach the nodes
	RequestAttach(	rSource.GetBuffer( rSource.GetLength() ), 
					rSelection.GetBuffer( rSelection.GetLength() ) );
}

void CLeftView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnRButtonUp(nFlags, point);
}


void CLeftView::OnParentNotify( UINT message, LPARAM lParam )
{	
	if ( message == WM_LBUTTONDOWN ) 
	{
		POINT clientPt = { LOWORD(lParam), HIWORD(lParam) };
		unsigned int nFlags = 0;
		m_hDragItem = m_wndTree2.HitTest( clientPt, &nFlags );		
	}		
}


void CLeftView::CreatePopupMenus()
{
	m_wndMainMenu.CreatePopupMenu();
	m_wndMainMenu.AppendMenu( MF_ENABLED, ID_ADDTOCUSTOMVIEW, "Add to Custom View" );
	m_wndMainMenu.AppendMenu( MF_SEPARATOR );
	m_wndMainMenu.AppendMenu( MF_ENABLED, ID_VIEW_MESHPREVIEWER, "Mesh Previewer" );

	m_wndCustomMenu.CreatePopupMenu();
	m_wndCustomMenu.AppendMenu( MF_ENABLED, ID_NEWFOLDER, "New Folder" );
	m_wndCustomMenu.AppendMenu( MF_ENABLED, ID_REMOVESELECTED, "Remove Selected" );	
	m_wndCustomMenu.AppendMenu( MF_ENABLED, ID_RENAMEFOLER, "Rename Folder" );
	m_wndCustomMenu.AppendMenu( MF_SEPARATOR );
	m_wndCustomMenu.AppendMenu( MF_ENABLED, ID_LOADALLCUSTOMVIEWS, "Reload Custom Views" );
	m_wndCustomMenu.AppendMenu( MF_ENABLED, ID_SAVECUSTOMVIEW, "Save Selected Custom View" );	
	m_wndCustomMenu.AppendMenu( MF_SEPARATOR );
	m_wndCustomMenu.AppendMenu( MF_ENABLED, ID_VIEW_MESHPREVIEWER, "Mesh Previewer" );
}

void CLeftView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if ( m_bItemDragging ) {		
		POINT clientPt = point;
		POINT dragPt = point;
		MapWindowPoints(NULL, &dragPt, 1);
		m_pDragImageList->DragMove( dragPt );				
		unsigned int nFlags = 0;
		HTREEITEM hTarget = m_wndTree2.HitTest( clientPt, &nFlags );
		if ( hTarget != NULL ) {
			m_hTarget = hTarget;

			if ( m_hTarget != m_hDragItem )
			{				
				m_pDragImageList->DragLeave( NULL );
				m_wndTree2.SelectDropTarget( m_hTarget );
				m_pDragImageList->DragEnter( NULL, CPoint( 0, 0 ) );
			}
			//m_wndTree2.SelectItem( m_hTarget );
		}
	}
	
	CView::OnMouseMove(nFlags, point);
}

void CLeftView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if ( m_bItemDragging ) {				
		m_bItemDragging = false;
		m_pDragImageList->EndDrag();
		Delete( m_pDragImageList );
		m_wndTree2.SelectDropTarget( NULL );

		if ( !m_hDragItem ) {
			return;
		}

		if ( CanMoveFolderToFolder( m_hDragItem, m_hTarget ) ) {
			gCustomViewer.AddToCustomView( m_hDragItem, m_wndTree2, m_wndTree2, m_hTarget );		
			m_wndTree2.DeleteItem( m_hDragItem );				
			m_wndTree2.SortChildren( m_hTarget );
		}				
	}	

	ReleaseCapture();	
	CView::OnLButtonUp(nFlags, point);
}

void CLeftView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CView::OnLButtonDown(nFlags, point);

	/*
	if ( GetKeyState( VK_CONTROL ) & 0x80 )
	{
		HTREEITEM hTarget = m_wndTree2.HitTest( point, &nFlags );
		if ( hTarget != NULL ) 
		{
			m_wndTree2.SetItemState( hTarget, TVIS_SELECTED, TVIS_SELECTED );
		}
	}
	*/
}


bool CLeftView::CanMoveFolderToFolder( HTREEITEM hSource, HTREEITEM & hDest )
{	
	if ( hSource == hDest ) 
	{
		return false;
	}

	bool bChange = false;
	int image = 0;
	int selImage = 0;		
	m_wndTree2.GetItemImage( hDest, image, selImage );
	if ( image != IMAGE_CLOSEFOLDER && image != IMAGE_OPENFOLDER )
	{
		bChange = true;
	}

	HTREEITEM hTest = hDest;
	while ( (hTest != TVI_ROOT) && (hTest != NULL) ) {
		if ( hTest == hSource ) {
			return false;
		}		

		hTest = m_wndTree2.GetParentItem( hTest );

		if ( bChange )
		{
			hDest = hTest;
			bChange = false;
		}	
	}

	return true;
}


bool CLeftView::RetrieveCustomFolder( HTREEITEM & hDest )
{
	bool bChange = false;
	int image = 0;
	int selImage = 0;
	if ( hDest == TVI_ROOT )
	{
		return true;
	}

	m_wndTree2.GetItemImage( hDest, image, selImage );
	if ( image != IMAGE_CLOSEFOLDER && image != IMAGE_OPENFOLDER )
	{
		bChange = true;
	}

	HTREEITEM hTest = hDest;
	while ( (hTest != TVI_ROOT) && (hTest != NULL) ) {
	
		hTest = m_wndTree2.GetParentItem( hTest );

		if ( bChange )
		{
			hDest = hTest;
			bChange = false;
		}	
	}

	return true;
}


void CLeftView::SwitchContentDisplay()
{	
	// Switch over the main view
	HTREEITEM hItem = m_wndTree1.GetRootItem();
	gpstring sName;	
	
	while ( hItem ) 
	{

		if ( m_wndTree1.GetItemData( hItem ) != 0 ) 
		{
			if ( gComponentList.GetNameFromMapId( m_wndTree1.GetItemData( hItem ), sName ) ) 
			{
				m_wndTree1.SetItemText( hItem, sName.c_str() );
			}
		}

		SwitchContentDisplayChildren( m_wndTree1, m_wndTree1.GetChildItem( hItem ) );
		hItem = m_wndTree1.GetNextSiblingItem( hItem );
	}


	// Switch over the custom view
	hItem = m_wndTree2.GetRootItem();
		
	while ( hItem ) 
	{

		if ( m_wndTree2.GetItemData( hItem ) != 0 ) 
		{
			if ( gComponentList.GetNameFromMapId( m_wndTree2.GetItemData( hItem ), sName ) ) 
			{
				m_wndTree2.SetItemText( hItem, sName.c_str() );
			}
		}

		SwitchContentDisplayChildren( m_wndTree2, m_wndTree2.GetChildItem( hItem ) );
		hItem = m_wndTree2.GetNextSiblingItem( hItem );
	}
}


void CLeftView::SwitchContentDisplayChildren( CTreeCtrl & wndTree, HTREEITEM hItem )
{
	gpstring sName;
	while ( hItem ) 
	{

		CString rTest = wndTree.GetItemText( hItem );

		if ( wndTree.GetItemData( hItem ) != 0 ) {
			if ( gComponentList.GetNameFromMapId( wndTree.GetItemData( hItem ), sName ) ) 
			{				
				int image1 = 0;
				int image2 = 0;
				wndTree.GetItemImage( hItem, image1, image2 );
				if ( image1 != IMAGE_CLOSEFOLDER  ) 
				{
					wndTree.SetItemText( hItem, sName.c_str() );	
				}			
			}
		}

		SwitchContentDisplayChildren( wndTree, wndTree.GetChildItem( hItem ) );
		hItem = wndTree.GetNextSiblingItem( hItem );
	}
}


void CLeftView::RotateSourceDoorLeft()
{
	int index = m_wndSourceCombo.GetCurSel();

	if ( index == CB_ERR ) 
	{
		return;
	}

	if ( index <= 0 ) 
	{
		index = GetNumSourceDoors();
	}
	else 
	{
		index--;
	}
	m_wndSourceCombo.SetCurSel( index );		

	if ( m_wndSourceCombo.GetCount() == 0 ) 
	{
		return;
	}

	CString rSelection;
	m_wndSourceCombo.GetLBText( index, rSelection );
	gEditorTerrain.SetSelectedSourceDoorID( atoi( rSelection.GetBuffer( rSelection.GetLength() ) ) );

}


void CLeftView::RotateSourceDoorRight()
{
	int index = m_wndSourceCombo.GetCurSel();

	if ( index == CB_ERR ) 
	{
		return;
	}

	if ( index >= GetNumSourceDoors() ) 
	{
		index = 0;		
	}
	else 
	{
		index++;
	}
	m_wndSourceCombo.SetCurSel( index );		

	if ( m_wndSourceCombo.GetCount() == 0 ) 
	{
		return;
	}

	CString rSelection;
	m_wndSourceCombo.GetLBText( index, rSelection );
	gEditorTerrain.SetSelectedSourceDoorID( atoi( rSelection.GetBuffer( rSelection.GetLength() ) ) );
}


void CLeftView::RotateDestDoorUp()
{
	int index = m_wndDestCombo.GetCurSel();

	if ( index == CB_ERR ) 
	{
		index = 0;
		if ( m_wndDestCombo.GetCount() == 0 ) 
		{
			return;
		}
	}


	if ( index <= 0 ) 
	{
		index = GetNumDestDoors();
	}
	else 
	{
		index--;
	}
	m_wndDestCombo.SetCurSel( index );
	
	if ( m_wndDestCombo.GetCount() == 0 ) 
	{
		return;
	}

	CString rSelection;
	m_wndDestCombo.GetLBText( index, rSelection );

	CString rSource;
	m_wndSourceCombo.GetLBText( m_wndSourceCombo.GetCurSel(), rSource );
	
	// Attach the nodes
	RequestAttach(	rSource.GetBuffer( rSource.GetLength() ), 
					rSelection.GetBuffer( rSelection.GetLength() ) );
}


void CLeftView::RotateDestDoorDown()
{
	int index = m_wndDestCombo.GetCurSel();

	if ( index == CB_ERR ) 
	{
		index = 0;
		if ( m_wndDestCombo.GetCount() == 0 ) 
		{
			return;
		}		
	}

	if ( index >= GetNumDestDoors() ) 
	{
		index = 0;
	}
	else 
	{
		index++;
	}
	m_wndDestCombo.SetCurSel( index );
	
	if ( m_wndDestCombo.GetCount() == 0 ) 
	{
		return;
	}

	CString rSelection;
	m_wndDestCombo.GetLBText( index, rSelection );

	CString rSource;
	m_wndSourceCombo.GetLBText( m_wndSourceCombo.GetCurSel(), rSource );
	
	// Attach the nodes
	RequestAttach(	rSource.GetBuffer( rSource.GetLength() ), 
					rSelection.GetBuffer( rSelection.GetLength() ) );
}


HWND CLeftView::GetTreeHWnd()
{	
	if ( m_wndTabs.GetCurFocus() == 0 ) 
	{
		return m_wndTree1.GetSafeHwnd();
	}
	else 
	{
		return m_wndTree2.GetSafeHwnd();
	}

	return NULL;
}

BOOL CLeftView::DestroyWindow() 
{
	
	
	return CView::DestroyWindow();
}

void CLeftView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}
