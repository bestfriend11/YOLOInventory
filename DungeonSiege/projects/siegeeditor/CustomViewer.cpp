//////////////////////////////////////////////////////////////////////////////
//
// File     :  CustomViewer.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



// Include Files
#include "PrecompEditor.h"
#include "CustomViewer.h"
#include "ImageListDefines.h"
#include "Dialog_NewFolder.h"
#include "ComponentList.h"
#include "CustomViewer.h"
#include "Preferences.h"
#include "LeftView.h"
#include "Dialog_Save_Custom_View.h"
#include "Dialog_Load_Custom_View.h"
#include "Config.h"
#include "FileSys.h"
#include "FileSysUtils.h"
#include "Dialog_Custom_View_Rename.h"


void CustomViewer::AddToCustomView( HTREEITEM hItem, CTreeCtrl & wndMain, CTreeCtrl & wndCustom, HTREEITEM hMaster )
{	
	gLeftView.RetrieveCustomFolder( hMaster );
	
	int nImage			= 0;
	int nSelectedImage	= 0;
	wndMain.GetItemImage( hItem, nImage, nSelectedImage );
	HTREEITEM hParent = wndCustom.InsertItem( wndMain.GetItemText( hItem ), nImage, nSelectedImage, hMaster );	
	wndCustom.SetItemData( hParent, wndMain.GetItemData( hItem ) );
	AddChildToCustomView( wndMain, wndCustom, wndMain.GetChildItem( hItem ), hParent );
	m_bChanged = true;
}


void CustomViewer::AddChildToCustomView( CTreeCtrl & wndMain, CTreeCtrl & wndCustom, HTREEITEM hChild, HTREEITEM hParent )
{
	while ( hChild != NULL ) 
	{
		int nImage			= 0;
		int nSelectedImage	= 0;
		wndMain.GetItemImage( hChild, nImage, nSelectedImage );	
		HTREEITEM hNew = wndCustom.InsertItem( wndMain.GetItemText( hChild ), nImage, nSelectedImage, hParent );
		wndCustom.SetItemData( hNew, wndMain.GetItemData( hChild ) );
		AddChildToCustomView( wndMain, wndCustom, wndMain.GetChildItem( hChild ), hNew );
		hChild = wndMain.GetNextSiblingItem( hChild );		
	}

	m_bChanged = true;
}


void CustomViewer::AddToCustomView( gpstring sTemplate, int image, CTreeCtrl & wndCustom, HTREEITEM hMaster )
{
	gLeftView.RetrieveCustomFolder( hMaster );
	int id = 0;
	gComponentList.GetMapIdFromTemplateName( sTemplate, id );
	gpstring sName;
	gComponentList.GetNameFromMapId( id, sName );
	HTREEITEM hItem = wndCustom.InsertItem( sName.c_str(), image, image, hMaster );		
	wndCustom.SetItemData( hItem, id );

	m_bChanged = true;
}


void CustomViewer::NewFolder( CTreeCtrl &wndCustom, HTREEITEM hItem )
{
	Dialog_NewFolder dlgFolder;
	dlgFolder.DoModal();

	if ( hItem == TVI_ROOT ) {
		wndCustom.InsertItem( GetNewFolderName().c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hItem );		
		return;
	}

	int nImage			= 0;
	int nSelectedImage	= 0;
	wndCustom.GetItemImage( hItem, nImage, nSelectedImage );
	if ( nImage == IMAGE_CLOSEFOLDER ) 
	{
		wndCustom.InsertItem( GetNewFolderName().c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hItem );		
	}	
	else 
	{
		HTREEITEM hParent = wndCustom.GetParentItem( hItem );
		wndCustom.InsertItem( GetNewFolderName().c_str(), IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hParent );		
	}	

	m_bChanged = true;
}


void CustomViewer::RenameFolder( CTreeCtrl & wndCustom, HTREEITEM hItem )
{
	int nImage			= 0;
	int nSelectedImage	= 0;
	wndCustom.GetItemImage( hItem, nImage, nSelectedImage );
	if ( nImage == IMAGE_CLOSEFOLDER || nImage == IMAGE_OPENFOLDER ) 
	{
		Dialog_Custom_View_Rename dlg;
		dlg.DoModal();
	}
}


void CustomViewer::RemoveFromCustomView( CTreeCtrl & wndCustom )
{	
	int nImage			= 0;
	int nSelectedImage	= 0;
	wndCustom.GetItemImage( wndCustom.GetSelectedItem(), nImage, nSelectedImage );
	if ( nImage == IMAGE_3BOX )
	{
		FuelHandle hEditor( "config:editor" );
		if ( hEditor )
		{
			CString rName = wndCustom.GetItemText( wndCustom.GetSelectedItem() );
			FuelHandle hView = hEditor->GetChildBlock( rName.GetBuffer( 0 ) );
			if ( hView )
			{
				hEditor->DestroyChildBlock( hView, true );				
				wndCustom.DeleteItem( wndCustom.GetChildItem( wndCustom.GetSelectedItem() ) );
				SaveCustomView( wndCustom );
				wndCustom.DeleteItem( wndCustom.GetSelectedItem() );
				LoadAllCustomViews( wndCustom );
				return;
			}
		}
	}

	wndCustom.DeleteItem( wndCustom.GetSelectedItem() );	

	m_bChanged = true;
}


void CustomViewer::CompileCustomData( CTreeCtrl & wndCustom, FuelHandle hCustomView )
{
	HTREEITEM hItem = wndCustom.GetSelectedItem();
	if ( hItem != NULL ) 
	{
		CString sItem	= wndCustom.GetItemText( hItem );
		
		int nImage			= 0;
		int nSelectedImage	= 0;
		wndCustom.GetItemImage( hItem, nImage, nSelectedImage );
		
		if ( nImage != IMAGE_CLOSEFOLDER && nImage != IMAGE_OPENFOLDER && nImage != IMAGE_3BOX )
		{
			gpstring sName;			
			if ( gComponentList.GetTemplateNameFromMapId( wndCustom.GetItemData( hItem ), sName ) )
			{
				sItem = sName.c_str();			
			}
			else 
			{
				sItem = wndCustom.GetItemText( hItem );
			}

			gpstring sGItem = sItem.GetBuffer( 0 );
			bool bHasSpaces = true;
			if ( sGItem.find( " " ) != gpstring::npos )
			{
				stringtool::ReplaceAllChars( sGItem, ' ', '_' );
			}

			FuelHandle hCvItem = hCustomView->CreateChildBlock( sGItem );			
			hCvItem->SetType( "cv_item" );
			hCvItem->Set( "image", nImage );
			hCvItem->Set( "selected_image", nSelectedImage );
			hCvItem->Set( "has_spaces", bHasSpaces );
		}
		else if ( nImage != IMAGE_3BOX )
		{				
			gpstring sItem;
			CString rItem = wndCustom.GetItemText( hItem );
			sItem = rItem.GetBuffer( 0 );
			bool bHasSpaces = false;
			if ( sItem.find( " " ) != gpstring::npos )
			{
				stringtool::ReplaceAllChars( sItem, ' ', '_' );
				bHasSpaces = true;
			}
			FuelHandle hFolder = hCustomView->CreateChildBlock( sItem );
			hFolder->SetType( "cv_folder" );		
			hFolder->Set( "has_spaces", bHasSpaces );
			CompileChildCustomData( wndCustom, wndCustom.GetChildItem( hItem ), hFolder );
		}		
		else
		{
			CompileChildCustomData( wndCustom, wndCustom.GetChildItem( hItem ), hCustomView );
		}
	}
}


void CustomViewer::CompileChildCustomData( CTreeCtrl & wndCustom, HTREEITEM hChild, FuelHandle hCustomView )
{
	while ( hChild != NULL ) 
	{

		CString sItem	= wndCustom.GetItemText( hChild );
		
		int nImage			= 0;
		int nSelectedImage	= 0;
		wndCustom.GetItemImage( hChild, nImage, nSelectedImage );
		
		if ( nImage != IMAGE_CLOSEFOLDER && nImage != IMAGE_OPENFOLDER )
		{
			gpstring sName;			
			if ( gComponentList.GetTemplateNameFromMapId( wndCustom.GetItemData( hChild ), sName ) )
			{
				sItem = sName.c_str();			
			}
			else 
			{
				sItem = wndCustom.GetItemText( hChild );
			}

			gpstring sGItem = sItem.GetBuffer( 0 );
			bool bHasSpaces = false;
			if ( sGItem.find( " " ) != gpstring::npos )
			{
				stringtool::ReplaceAllChars( sGItem, ' ', '_' );
				bHasSpaces = true;
			}

			FuelHandle hCvItem = hCustomView->CreateChildBlock( sGItem );
			hCvItem->SetType( "cv_item" );
			hCvItem->Set( "image", nImage );
			hCvItem->Set( "selected_image", nSelectedImage );
			hCvItem->Set( "has_spaces", bHasSpaces );
		}
		else
		{				
			gpstring sItem;
			CString rItem = wndCustom.GetItemText( hChild );
			sItem = rItem.GetBuffer( 0 );
			bool bHasSpaces = false;
			if ( sItem.find( " " ) != gpstring::npos )
			{
				stringtool::ReplaceAllChars( sItem, ' ', '_' );
				bHasSpaces = true;
			}
			FuelHandle hFolder = hCustomView->CreateChildBlock( sItem );
			hFolder->SetType( "cv_folder" );			
			hFolder->Set( "has_spaces", bHasSpaces );
			CompileChildCustomData( wndCustom, wndCustom.GetChildItem( hChild ), hFolder );			
		}	
		
		hChild = wndCustom.GetNextSiblingItem( hChild );
	}
}


void CustomViewer::LoadAllCustomViews( CTreeCtrl & wndCustom )
{	
	wndCustom.DeleteAllItems();
	FuelHandle hEditor( "config:editor" );
	if ( hEditor )
	{
		hEditor.Reload();
		FuelHandleList hlViews = hEditor->ListChildBlocks( 1, "custom_view_v1" );
		FuelHandleList::iterator i;
		for ( i = hlViews.begin(); i != hlViews.end(); ++i )
		{
			LoadCustomView( wndCustom, (*i)->GetName() );
		}
	}
}


void CustomViewer::LoadCustomView( CTreeCtrl & wndCustom, gpstring sView )
{
	gpstring sAddress = "config:editor:";
	sAddress += sView;
	FuelHandle hCustomView( sAddress );
	if ( hCustomView.IsValid() == false )
	{
		return;
	}

	HTREEITEM hItem = wndCustom.InsertItem( sView.c_str(), IMAGE_3BOX, IMAGE_3BOX, TVI_ROOT );
	FuelHandleList hlRoot = hCustomView->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hlRoot.begin(); i != hlRoot.end(); ++i )
	{
		gpstring sType = (*i)->GetType();
		if ( sType.same_no_case( "cv_folder" ) )
		{
			gpstring sItem = (*i)->GetName();

			bool bHasSpaces = false;
			(*i)->Get( "has_spaces", bHasSpaces );
			if ( bHasSpaces )
			{
				stringtool::ReplaceAllChars( sItem, '_', ' ' );
			}
			HTREEITEM hChild = wndCustom.InsertItem( sItem, IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hItem );
			RecursiveLoadCustomView( wndCustom, hChild, *i );
		}
		else if ( sType.same_no_case( "cv_item" ) )
		{
			int	image = 0;
			int	selImage = 0;

			(*i)->Get( "image", image );
			(*i)->Get( "selected_image", selImage );

			gpstring sItem = (*i)->GetName();
			bool bHasSpaces = false;
			(*i)->Get( "has_spaces", bHasSpaces );
			if ( bHasSpaces )
			{
				stringtool::ReplaceAllChars( sItem, '_', ' ' );
			}
			
			HTREEITEM hNew = wndCustom.InsertItem( sItem, image, selImage, hItem );
			int id = 0;
			gComponentList.GetMapIdFromTemplateName( (*i)->GetName(), id );			
			wndCustom.SetItemData( hNew, id );
		}
	}
}


void CustomViewer::LoadCustomView( CTreeCtrl & wndCustom )
{
	Dialog_Load_Custom_View dlg;
	if ( dlg.DoModal() != IDOK )
	{
		return;
	}
	
	wndCustom.DeleteAllItems();		

	gpstring sAddress = "config:editor:";
	sAddress += gPreferences.GetCustomViewSaveName();
	FuelHandle hCustomView( sAddress );	

	if ( hCustomView.IsValid() == false )
	{
		return;
	}

	HTREEITEM hItem = TVI_ROOT;
	FuelHandleList hlRoot = hCustomView->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hlRoot.begin(); i != hlRoot.end(); ++i )
	{
		gpstring sType = (*i)->GetType();
		if ( sType.same_no_case( "cv_folder" ) )
		{
			gpstring sItem = (*i)->GetName();
			bool bHasSpaces = false;
			(*i)->Get( "has_spaces", bHasSpaces );
			if ( bHasSpaces )
			{
				stringtool::ReplaceAllChars( sItem, '_', ' ' );
			}

			HTREEITEM hChild = wndCustom.InsertItem( sItem, IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hItem );
			RecursiveLoadCustomView( wndCustom, hChild, *i );
		}
		else if ( sType.same_no_case( "cv_item" ) )
		{
			int	image = 0;
			int	selImage = 0;

			(*i)->Get( "image", image );
			(*i)->Get( "selected_image", selImage );

			gpstring sItem = (*i)->GetName();
			bool bHasSpaces = false;
			(*i)->Get( "has_spaces", bHasSpaces );
			if ( bHasSpaces )
			{
				stringtool::ReplaceAllChars( sItem, '_', ' ' );
			}
			
			HTREEITEM hNew = wndCustom.InsertItem( sItem, image, selImage, hItem );
			int id = 0;
			gComponentList.GetMapIdFromTemplateName( (*i)->GetName(), id );			
			wndCustom.SetItemData( hNew, id );
		}
	}
	
	m_bChanged = false;
}


void CustomViewer::RecursiveLoadCustomView( CTreeCtrl & wndCustom, HTREEITEM hItem, FuelHandle hCustomView )
{
	FuelHandleList hlRoot = hCustomView->ListChildBlocks( 1 );
	FuelHandleList::iterator i;
	for ( i = hlRoot.begin(); i != hlRoot.end(); ++i )
	{
		gpstring sType = (*i)->GetType();
		if ( sType.same_no_case( "cv_folder" ) )
		{
			gpstring sItem = (*i)->GetName();
			bool bHasSpaces = false;
			(*i)->Get( "has_spaces", bHasSpaces );
			if ( bHasSpaces )
			{
				stringtool::ReplaceAllChars( sItem, '_', ' ' );
			}
			HTREEITEM hChild = wndCustom.InsertItem( sItem, IMAGE_CLOSEFOLDER, IMAGE_OPENFOLDER, hItem );
			RecursiveLoadCustomView( wndCustom, hChild, *i );
		}
		else if ( sType.same_no_case( "cv_item" ) )
		{
			int	image = 0;
			int	selImage = 0;

			(*i)->Get( "image", image );
			(*i)->Get( "selected_image", selImage );

			gpstring sItem = (*i)->GetName();
			bool bHasSpaces = false;
			(*i)->Get( "has_spaces", bHasSpaces );
			if ( bHasSpaces )
			{
				stringtool::ReplaceAllChars( sItem, '_', ' ' );
			}
						
			HTREEITEM hNew = wndCustom.InsertItem( sItem, image, selImage, hItem );
			int id = 0;
			gComponentList.GetMapIdFromTemplateName( (*i)->GetName(), id );			
			wndCustom.SetItemData( hNew, id );
		}
	}
}


void CustomViewer::SaveCustomView( CTreeCtrl & wndCustom )
{
	if ( !(wndCustom.GetParentItem( wndCustom.GetSelectedItem() ) == TVI_ROOT || 
		  wndCustom.GetParentItem( wndCustom.GetSelectedItem() ) == 0) )
	{
		MessageBoxEx( wndCustom.GetSafeHwnd(), "Please select the top-level custom view folder you would like to save.", "Save Custom View", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
		return;
	}	

	int nImage			= 0;
	int nSelectedImage	= 0;
	wndCustom.GetItemImage( wndCustom.GetSelectedItem(), nImage, nSelectedImage );
	if ( nImage != IMAGE_3BOX )
	{
		Dialog_Save_Custom_View dlg;
		if ( dlg.DoModal() != IDOK )
		{
			return;
		}
	}
	else
	{
		CString rItem = wndCustom.GetItemText( wndCustom.GetSelectedItem() );
		gPreferences.SetCustomViewSaveName( rItem.GetBuffer( 0 ) );
	}

	gpstring path = gConfig.ResolvePathVars( "%out%/config/editor" );
	if ( !FileSys::DoesPathExist( path ) )
	{
		FileSys::CreateFullDirectory( path );
	}

	FuelDB * pCustomViewDb = gFuelSys.AddTextDb( "custom_view" );
	gpstring sDir = gConfig.ResolvePathVars( "%home%" );
	sDir.appendf( "\\config\\editor" );
	pCustomViewDb->Init( FuelDB::OPTION_READ | FuelDB::OPTION_WRITE | FuelDB::OPTION_JIT_READ, sDir, sDir );
	
	gpstring sAddress = "::custom_view:";	
	sAddress.appendf( "%s", gPreferences.GetCustomViewSaveName().c_str() );
	FuelHandle hCustomView( sAddress );
	FuelHandle hParent( "::custom_view" );
	
	if ( hCustomView.IsValid() )
	{	
		hParent->DestroyChildBlock( hCustomView );		
	}

	gpstring sFile = gPreferences.GetCustomViewSaveName();
	sFile += ".gas";

	hCustomView = hParent->CreateChildBlock( gPreferences.GetCustomViewSaveName(), sFile.c_str() );
	hCustomView->SetType( "custom_view_v1" );

	CompileCustomData( wndCustom, hCustomView );	

	hCustomView->GetDB()->SaveChanges();

	gFuelSys.Remove( hCustomView->GetDB() );
	
	m_bChanged = false;

	LoadAllCustomViews( wndCustom );
}


void CustomViewer::GetCustomViews( StringVec & customViews )
{
	FuelHandle hConfig( "config:editor" );
	if ( hConfig.IsValid() )
	{
		FuelHandleList hlCustomViews = hConfig->ListChildBlocks( 1, "custom_view_v1" );
		FuelHandleList::iterator i;
		for ( i = hlCustomViews.begin(); i != hlCustomViews.end(); ++i )
		{
			customViews.push_back( (*i)->GetName() );
		}
	}
}