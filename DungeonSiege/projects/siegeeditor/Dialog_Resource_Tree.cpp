// Dialog_Resource_Tree.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Resource_Tree.h"
#include "FileSys.h"
#include "Preferences.h"
#include "ImageListDefines.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace FileSys;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Resource_Tree dialog


Dialog_Resource_Tree::Dialog_Resource_Tree(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Resource_Tree::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Resource_Tree)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Resource_Tree::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Resource_Tree)
	DDX_Control(pDX, IDC_TREE_RESOURCES, m_Resources);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Resource_Tree, CDialog)
	//{{AFX_MSG_MAP(Dialog_Resource_Tree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Resource_Tree message handlers

BOOL Dialog_Resource_Tree::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_Resources.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	
	int numPaths = stringtool::GetNumDelimitedStrings( gPreferences.GetResourcePath().c_str(), ',' ); 
	for ( int iPath = 0; iPath != numPaths; ++iPath )
	{
		gpstring sCurrentPath;
		stringtool::GetDelimitedValue( gPreferences.GetResourcePath().c_str(), ',', iPath, sCurrentPath );

		RecursiveFinder hPath( sCurrentPath );
		if ( hPath )
		{
			gpstring sFile;
			while ( hPath.GetNext( sFile, true ) )
			{
				gpstring sResource;
				stringtool::ReplaceAllChars( sFile, '\\', '/' );
				int numParts = stringtool::GetNumDelimitedStrings( sFile.c_str(), '/' );
				stringtool::GetDelimitedValue( sFile.c_str(), '/', numParts-1, sResource );
				sResource = sResource.substr( 0, sResource.size()-4 );
				int findPos = sFile.find( sResource );
				if ( findPos != gpstring::npos )
				{
					sFile = sFile.substr( 0, findPos );
				}

				gpstring sResourcePath = sFile;
				sResourcePath = sResourcePath.substr( 0, sResourcePath.size()-1 );
				int numStrings = stringtool::GetNumDelimitedStrings( sResourcePath.c_str(), '/' );
				HTREEITEM hParent = TVI_ROOT;
				for ( int i = 0; i != numStrings; ++i )
				{
					HTREEITEM hItem = NULL;
					gpstring sValue;
					stringtool::GetDelimitedValue( sResourcePath.c_str(), '/', i, sValue );	
					
					if ( m_Resources.GetCount() != 0 )
					{						
						HTREEITEM hSearch = m_Resources.GetNextItem( TVI_ROOT, TVGN_ROOT );
						while ( hSearch )
						{						
							CString rSearch = m_Resources.GetItemText( hSearch );
							if ( rSearch.CompareNoCase( sValue.c_str() ) == 0 )
							{
								hItem = hSearch;
								break;
							}						
							
							HTREEITEM hNewParent = hSearch;
							hSearch = m_Resources.GetNextItem( hNewParent, TVGN_CHILD );
							if ( hSearch )
							{
								hSearch = RecursiveFindResourceFolder( hSearch, hParent, sValue );
								if ( hSearch )
								{
									hItem = hSearch;
									break;
								}
							}

							hSearch = m_Resources.GetNextItem( hNewParent, TVGN_NEXT );
						}
					}
						
					if ( hItem == NULL )
					{
						hItem = m_Resources.InsertItem( sValue.c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hParent );
					}

					hParent = hItem;
				}								
				
				m_Resources.InsertItem( sResource.c_str(), IMAGE_LTBLUEBOX, IMAGE_LTBLUEBOX, hParent );						
			}
		}
	}

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Resource_Tree::OnOK() 
{
	CString rText = m_Resources.GetItemText( m_Resources.GetSelectedItem() );
	gPreferences.SetSelectedResource( rText.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}


HTREEITEM Dialog_Resource_Tree::RecursiveFindResourceFolder( HTREEITEM hChild, HTREEITEM & hParent, const gpstring & sValue )
{
	HTREEITEM hItem = NULL;
	while ( hChild )
	{
		CString rSearch = m_Resources.GetItemText( hChild );
		if ( rSearch.CompareNoCase( sValue.c_str() ) == 0 )
		{
			return hChild;			
		}	

		int nImage			= 0;
		int nSelectedImage	= 0;
		m_Resources.GetItemImage( hChild, nImage, nSelectedImage );
		if ( nImage == IMAGE_LTBLUEBOX )
		{
			return NULL;
		}
		
		HTREEITEM hNewParent = hChild;
		hChild = m_Resources.GetNextItem( hNewParent, TVGN_CHILD );
		if ( hChild )
		{
			RecursiveFindResourceFolder( hChild, hParent, sValue );
		}

		hChild = m_Resources.GetNextItem( hNewParent, TVGN_NEXT );
	}

	return NULL;
}