//////////////////////////////////////////////////////////////////////////////
//
// File     :  CustomViewer.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __CUSTOMVIEWER_H
#define __CUSTOMVIEWER_H


#include "gpcore.h"
#include "gpstring.h"
#include <vector>
#include <map>


class CustomViewer : public Singleton <CustomViewer>
{
public:

	CustomViewer() { m_bChanged = false; }
	~CustomViewer() {}

	// Editing
	void AddToCustomView( HTREEITEM hItem, CTreeCtrl & wndMain, CTreeCtrl & wndCustom, HTREEITEM hMaster = TVI_ROOT );
	void AddToCustomView( gpstring sTemplate, int image, CTreeCtrl & wndCustom, HTREEITEM hMaster = TVI_ROOT );
	void AddChildToCustomView( CTreeCtrl & wndMain, CTreeCtrl & wndCustom, HTREEITEM hChild, HTREEITEM hParent );
	void CompileCustomData( CTreeCtrl & wndCustom, FuelHandle hCustomView );
	void CompileChildCustomData( CTreeCtrl & wndCustom, HTREEITEM hChild, FuelHandle hCustomView );

	void AddToCustomView( DWORD dwScid, CTreeCtrl & wndCustom );	
	void RemoveFromCustomView( CTreeCtrl & wndCustom );
	void NewFolder( CTreeCtrl & wndCustom, HTREEITEM hItem );
	void RenameFolder( CTreeCtrl & wndCustom, HTREEITEM hItem );

	// Persistence
	void LoadAllCustomViews( CTreeCtrl & wndCustom );
	void LoadCustomView( CTreeCtrl & wndCustom, gpstring sView );
	void LoadCustomView( CTreeCtrl & wndCustom );
	void RecursiveLoadCustomView( CTreeCtrl & wndCustom, HTREEITEM hItem, FuelHandle hCustomView );
	void SaveCustomView( CTreeCtrl & wndCustom );	
	void GetCustomViews( StringVec & customViews );

	void			SetNewFolderName( gpstring sName )		{ m_sNewFolderName = sName;	}
	gpstring		GetNewFolderName()						{ return m_sNewFolderName;	}

	bool			GetViewChanged()	{ return m_bChanged; }
	
private:

	gpstring m_sNewFolderName;	
	bool m_bChanged;

};

#define gCustomViewer CustomViewer::GetSingleton()

#endif