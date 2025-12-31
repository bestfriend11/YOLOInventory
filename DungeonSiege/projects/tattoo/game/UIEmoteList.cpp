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

#include "Precomp_Game.h"
#include "UIEmoteList.h"
#include "UIPartyManager.h"
#include "UIGameConsole.h"
#include "ui_emotes.h"
#include "inputBinder.h"
#include "server.h"
#include "player.h"


UIEmoteList :: UIEmoteList()
{
	m_IsInitialized		= false;
	m_IsVisible			= false;
	m_categorySelected	= -1;
	m_preAppendString	= L"  ";
	m_postAppendString	= L". ";
}

void UIEmoteList :: Init()
{
	if ( ::IsSinglePlayer() )
	{
		gperror("trying to initalize the emotelist in singleplayer.");
		return;
	}

	// Reinit ui
	gUIShell.ActivateInterface( "ui:interfaces:backend:emote_list", false );

	UIListbox* pListBox = (UIListbox*) gUIShell.FindUIWindow( "emotelist_list", "emote_list" );
	if( pListBox )
	{
		// put all the emotes in our emote list.
		pListBox->RemoveAllElements();
		ListAllEmotes( pListBox );
	}


	FastFuelHandle hEmotesFile( "ui:config:emotes" );
	if ( hEmotesFile.IsValid() )
	{
		/////////////////////////////////////////////////////
		// properties
		FastFuelHandle hProperties = hEmotesFile.GetChildNamed( "properties" );
		if( hProperties.IsValid() )
		{
			m_preAppendString = ToUnicode(hProperties.GetString("pre_string_emote_list",ToAnsi(m_preAppendString)));
			m_postAppendString = ToUnicode(hProperties.GetString("post_string_emote_list",ToAnsi(m_postAppendString)));
		}
	}
	m_IsInitialized = true;
}

// set the list visible/not visible.  takes care of closing other windows.
void UIEmoteList :: SetVisible( bool visible )
{
	gpassert( m_IsInitialized );

	if ( visible && !m_IsVisible )
	{
		gUIPartyManager.CloseAllCharacterMenus();
		gUIShell.ShowInterface( "emote_list" );
		m_IsVisible = true;
	}
	else if ( !visible && m_IsVisible )
	{
		gUIShell.HideInterface( "emote_list" );
		m_IsVisible = false;
	}

	m_categorySelected = -1;
}

//
//	command for the outside to toggle the emote list on and off.
//
bool UIEmoteList :: ToggleEmoteList( const KeyInput & key )
{
	if ( ::IsMultiPlayer() && m_IsInitialized )
	{
		static bool keyUp = true;

		if ( keyUp && (key.GetQualifiers() & Keys::Q_KEY_DOWN) )
		{
			keyUp = false;
			SetVisible( !IsVisible() );
		}
		else if ( key.GetQualifiers() & Keys::Q_KEY_UP )
		{
			keyUp = true;
		}
	}
	return true;
}

//
// select the current emote and send it out to the chat box
//
void UIEmoteList :: SelectEmote(  )
{
	gpassert( m_IsInitialized );

	UIListbox* pListBox = (UIListbox*) gUIShell.FindUIWindow( "emotelist_list", "emote_list" );
	if( pListBox )
	{
		// check to make sure this isn't a heading! which would be a mutliple of the max
		// category headings...
		int tag = pListBox->GetSelectedTag();
		if( tag != UIListbox::INVALID_TAG && pListBox->GetSelectedTag() % gUIEmotes.GetMaxEmotesCategory() != 0 )
		{
			gpwstring text;
			text = pListBox->GetSelectedText( );

			gpwstring::size_type findIndex = text.find( m_postAppendString );
			if( findIndex < text.size() )
			{
				findIndex = text.size() - findIndex - m_postAppendString.size();
				if( findIndex > 0 )
				{
					text = text.right_safe( findIndex );
				}
			}

			// send this chat to all players!
			gUIGameConsole.SubmitChat( text, "game_console_mp", true );
			SetVisible( false );
		}
	}	
	else
	{
		SetVisible( false );
	}
}

//
// Function for handling the hotkey combos to select an emote
//
bool UIEmoteList :: HotKeyEmoteListSelect( int emoteSelected )
{
	if( ::IsMultiPlayer() && IsVisible() )
	{
		gpassert( m_IsInitialized );

		UIListbox* pListBox = (UIListbox*) gUIShell.FindUIWindow( "emotelist_list", "emote_list" );
		if( pListBox )
		{
			if(  m_categorySelected	== -1 )
			{
				m_categorySelected = emoteSelected;
				pListBox->SelectElement( gUIEmotes.GetMaxEmotesCategory() * m_categorySelected );
			}
			else 
			{
				pListBox->SelectElement( gUIEmotes.GetMaxEmotesCategory() * m_categorySelected + emoteSelected + 1 );
				SelectEmote( );
			}
		}
	}
	return true;
}

//
// add all text emotes for a given category to the listbox.  use category and elementCounts to determine the tab
// to use in the listbox.  the counts are used so we can hook these up to the number keys.
//
void UIEmoteList :: AddTextCategory( UIListbox* pListBox, const gpwstring& category, int categoryCount, int &elementCount ) const
{
	gpassert( pListBox );

	gpwstring preparedString;

	// add the elements under the categories
	for ( UIEmotes::TextEmoteMap::const_iterator textEmote = gUIEmotes.GetTextEmotes().begin(); textEmote != gUIEmotes.GetTextEmotes().end(); ++textEmote )
	{
		if( textEmote->first != NULL )
		{
			if( ! wcsicmp( textEmote->second.m_category, category ) )
			{
				preparedString.assignf( L"%s%d%s%s", m_preAppendString.c_str(), elementCount, m_postAppendString.c_str(), textEmote->second.m_screenGroup );
				gUIEmotes.PrepareString( preparedString );
				gUIEmotes.TextEmoteKeywordReplacement( preparedString, gServer.GetLocalHumanPlayer()->GetHeroName(), gpwtranslate( $MSG$ "Everyone" ) );

				pListBox->AddElement( preparedString, gUIEmotes.GetMaxEmotesCategory() * categoryCount + elementCount );
				pListBox->SetElementTagToolTip( gUIEmotes.GetMaxEmotesCategory() * categoryCount + elementCount, textEmote->first );

				elementCount ++;
			}
		}
		else
		{
			gperror( "Bad emote trying to be added to a listbox!!! " );
		}	
	}
}

//
// add all texture emotes for a given category to the listbox.  use category and elementCounts to determine the tab
// to use in the listbox.  the counts are used so we can hook these up to the number keys.
//
void UIEmoteList :: AddTextureCategory( UIListbox* pListBox, const gpwstring& category, int categoryCount, int &elementCount  ) const
{
	gpassert( pListBox );

	gpwstring preparedString;

	for ( UIEmotes::EmoteMap::const_iterator emote = gUIEmotes.GetEmotes().begin(); emote != gUIEmotes.GetEmotes().end(); ++emote )
	{
		if( emote->second.m_emoteType == UI_EMOTE_TYPE_TEXTURE )
		{
			if( emote->second.m_inputString != NULL )
			{
				if( ! wcsicmp( emote->second.m_category, category ) )
				{
					preparedString.assignf( L"%s%d%s%s", m_preAppendString.c_str(), elementCount, m_postAppendString.c_str(), emote->second.m_inputString );
					gUIEmotes.PrepareString( preparedString );

					pListBox->AddElement( preparedString, gUIEmotes.GetMaxEmotesCategory() * categoryCount + elementCount );
					pListBox->SetElementTagToolTip( gUIEmotes.GetMaxEmotesCategory() * categoryCount + elementCount, emote->second.m_inputString );

					elementCount ++;
				}
			}
		}
	}
}

//
//
// Returns a listbox with all the emotes of a certain type in it!
//	you can specify if you only want emotes with categories, so we don't get lots of crap emotes!
//
void UIEmoteList :: ListAllEmotes( UIListbox* pListBox, eUIEmoteType emoteType /*=UI_EMOTE_TYPE_ANY*/, bool categoriesOnly /*=true*/ ) const
{
	gpassert( pListBox );

	if( pListBox )
	{
		int categoryCount = 0;
		int elementCount = 1;
		gpwstring preparedString;

		// add the categories
		for ( UIEmotes::CategoryColl::const_iterator category = gUIEmotes.GetCategories().begin(); category != gUIEmotes.GetCategories().end(); ++category )
		{
			elementCount = 1;

			// if we take all categories or if we have a category
			if( !categoriesOnly || !category->empty() )
			{
				// add the category heading 
				preparedString.assignf( L"%d%s%s", categoryCount + 1, m_postAppendString.c_str(), (*category) );
				gUIEmotes.PrepareString( preparedString );
				pListBox->AddElement( preparedString, gUIEmotes.GetMaxEmotesCategory() * categoryCount );

				// add the category elements
				if( emoteType == UI_EMOTE_TYPE_TEXT || emoteType == UI_EMOTE_TYPE_ANY )
				{
					AddTextCategory( pListBox, (*category), categoryCount, elementCount );
				}

				if( emoteType == UI_EMOTE_TYPE_TEXTURE || emoteType == UI_EMOTE_TYPE_ANY )
				{
					AddTextureCategory( pListBox, (*category), categoryCount, elementCount );
				}

				// add a spacer at the end
				pListBox->AddElement( L"", gUIEmotes.GetMaxEmotesCategory() * categoryCount + elementCount );
				pListBox->SetElementSelectable( gUIEmotes.GetMaxEmotesCategory() * categoryCount + elementCount, false);
				elementCount ++;
			}

			categoryCount ++;
		}
	}
}

bool UIEmoteList :: EmoteListSelect1( const KeyInput &/*key*/ )
{
	return HotKeyEmoteListSelect( 0 );
}
bool UIEmoteList :: EmoteListSelect2( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 1 );
}
bool UIEmoteList :: EmoteListSelect3( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 2 );
}
bool UIEmoteList :: EmoteListSelect4( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 3 );
}
bool UIEmoteList :: EmoteListSelect5( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 4 );
}
bool UIEmoteList :: EmoteListSelect6( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 5 );
}
bool UIEmoteList :: EmoteListSelect7( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 6 );
}
bool UIEmoteList :: EmoteListSelect8( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 7 );
}
bool UIEmoteList :: EmoteListSelect9( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 8 );
}
bool UIEmoteList :: EmoteListSelect0( const KeyInput & /*key*/ )
{
	return HotKeyEmoteListSelect( 9 );
}
