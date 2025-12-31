// Dialog_Item_Viewer.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Item_Viewer.h"
#include "EditorObjects.h"
#include "GoData.h"
#include "ImageListDefines.h"
#include "ComponentList.h"
#include "GoInventory.h"
#include "GoDefs.h"
#include "namingkey.h"
#include "worldmessage.h"
#include "GoSupport.h"
#include "FuBiDefs.h"
#include "FuBiSchema.h"
#include "FileSys.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Item_Viewer dialog


Dialog_Item_Viewer::Dialog_Item_Viewer(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Item_Viewer::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Item_Viewer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Item_Viewer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Item_Viewer)
	DDX_Control(pDX, IDC_TREE_ITEMS, m_items);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Item_Viewer, CDialog)
	//{{AFX_MSG_MAP(Dialog_Item_Viewer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Item_Viewer message handlers

BOOL Dialog_Item_Viewer::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_items.SetImageList( gEditor.GetImageList(), TVSIL_NORMAL );
	m_items.SetImageList( gEditor.GetImageList(), TVSIL_STATE );
	m_items.DeleteAllItems();	

	
	ContentDb::DataTemplateIter i;
	for ( i = gContentDb.GetDataTemplateRootsBegin(); i != gContentDb.GetDataTemplateRootsEnd(); ++i )
	{
		RecursiveAddTemplate( (*i), TVI_ROOT );
	}	

	/*
	FuelHandle hMacro( "world:contentdb:macros" );
	if ( hMacro.IsValid() )
	{
		HTREEITEM hItem = m_items.InsertItem( "Macros", IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, TVI_ROOT, TVI_SORT );
		if ( hMacro->FindFirstKeyAndValue() )
		{
			gpstring sKey;
			gpstring sValue;
			while ( hMacro->GetNextKeyAndValue( sKey, sValue ) )
			{
				m_items.InsertItem( sKey, IMAGE_GO, IMAGE_GO, hItem, TVI_SORT );
			}
		}
	}
	*/
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Item_Viewer::RecursiveAddTemplate( GoDataTemplate * pTemplate, HTREEITEM hParent )
{
	ItemViewerFilter vf = gEditorObjects.GetItemViewerFilter();

	HTREEITEM hItem;
	if ( pTemplate->GetChildBegin() != pTemplate->GetChildEnd() )
	{
		hItem = m_items.InsertItem( pTemplate->GetName(), IMAGE_CLOSEFOLDER, IMAGE_CLOSEFOLDER, hParent, TVI_SORT );
	}
	else
	{
		bool bSuccess = true;
		if ( vf.slot != ES_NONE )
		{
			const GoDataComponent * pComponent = pTemplate->FindComponentByName( "gui" );
			if ( pComponent )
			{
				eEquipSlot slot = ES_NONE;
				pComponent->GetRecord()->Get( "equip_slot", slot );
				if ( slot != vf.slot )
				{
					bSuccess = false; 
				}
			}
			else
			{
				bSuccess = false;
			}
		}
		else if ( vf.bSpell )
		{
			const GoDataComponent * pComponent = pTemplate->FindComponentByName( "magic" );
			if ( pComponent )
			{
				eMagicClass mc;
				pComponent->GetRecord()->Get( "magic_class", mc );
				if ( !IsSpell( mc ) )
				{
					bSuccess = false;
				}			
			}
			else
			{
				bSuccess = false;
			}
		}

		if ( bSuccess )
		{
			hItem = m_items.InsertItem( pTemplate->GetName(), IMAGE_GO, IMAGE_GO, hParent, TVI_SORT );
			m_items.SetItemData( hItem, gComponentList.GetCurrentTemplateId() );		
		}
	}

	GoDataTemplate::ChildIter i;
	for ( i = pTemplate->GetChildBegin(); i != pTemplate->GetChildEnd(); ++i )
	{
		RecursiveAddTemplate( (*i), hItem );
	}

	int nImage = 0;
	int nSelectedImage = 0;
	m_items.GetItemImage( hItem, nImage, nSelectedImage );
	if ( nImage == IMAGE_CLOSEFOLDER || nImage == IMAGE_OPENFOLDER )
	{
		HTREEITEM hChild = m_items.GetChildItem( hItem );
		if ( hChild == NULL ) 
		{
			m_items.DeleteItem( hItem );
		}
	}
}

void Dialog_Item_Viewer::OnOK() 
{
	HTREEITEM hItem = m_items.GetSelectedItem();
	if ( hItem == NULL )
	{
		return;
	}

	CString rTemplate = m_items.GetItemText( hItem );

	gEditorObjects.SetViewerSelectedTemplate( rTemplate.GetBuffer( 0 ) );
	
	CDialog::OnOK();
}

void Dialog_Item_Viewer::OnCancel() 
{
	gEditorObjects.SetViewerSelectedTemplate( "" );
	
	CDialog::OnCancel();
}