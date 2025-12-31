//////////////////////////////////////////////////////////////////////////////
//
// File     :  UIEmoteList.cpp
// Author(s):  Jessica Tams
//
// Summary  :  UI for the selection and listing of the emotes.
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __UIEMOTELIST_H
#define __UIEMOTELIST_H


#include "UI.h"
#include "ui_emotes.h"

class UIEmoteList
{

public:

	UIEmoteList();
	~UIEmoteList(){}
	
	void Init		( );

	bool ToggleEmoteList( const KeyInput & key );
	void SetVisible	( bool visible = true );
	bool IsVisible	( )	const { return m_IsVisible; }
	
	void Update		( double /*seconds_elapsed*/ ) {}

	void SelectEmote(  );
	bool EmoteListSelect1( const KeyInput & key );
	bool EmoteListSelect2( const KeyInput & key );
	bool EmoteListSelect3( const KeyInput & key );
	bool EmoteListSelect4( const KeyInput & key );
	bool EmoteListSelect5( const KeyInput & key );
	bool EmoteListSelect6( const KeyInput & key );
	bool EmoteListSelect7( const KeyInput & key );
	bool EmoteListSelect8( const KeyInput & key );
	bool EmoteListSelect9( const KeyInput & key );
	bool EmoteListSelect0( const KeyInput & key );

private:
	bool HotKeyEmoteListSelect( int emoteSelected );

	void ListAllEmotes( UIListbox* pListBox, eUIEmoteType emoteType = UI_EMOTE_TYPE_ANY, bool categoriesOnly = true ) const;
	void AddTextCategory( UIListbox* pListBox, const gpwstring& category, int categoryCount, int &elementCount ) const;
	void AddTextureCategory( UIListbox* pListBox, const gpwstring& category, int categoryCount, int& elementCount ) const;

	bool m_IsVisible;
	bool m_IsInitialized;

	int m_categorySelected;

	gpwstring m_preAppendString;
	gpwstring m_postAppendString;

};

#endif