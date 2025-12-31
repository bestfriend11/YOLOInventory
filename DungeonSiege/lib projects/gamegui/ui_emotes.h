//////////////////////////////////////////////////////////////////////////////
//
// File     :  Emotes.cpp
// Author(s):  Jessica Tams
//
// Summary  :  Layer to handle emotes in ui text windows
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _EMOTES_H_
#define _EMOTES_H_

// Include Files
#include <vector>
#include "ui_types.h"
#include "ui_interface.h"

enum eUIEmoteType
{
	UI_EMOTE_TYPE_ANY, // emote type not defined.  
	UI_EMOTE_TYPE_TEXTURE,
	UI_EMOTE_TYPE_TEXT,
	UI_EMOTE_TYPE_COLOR,
};

enum eUITextEmoteType
{
	UI_TEXT_EMOTE_TYPE_ANY,
	UI_TEXT_EMOTE_TYPE_GROUP,
	UI_TEXT_EMOTE_TYPE_INDIVIDUAL,
};

class UIWindow;

class UIEmotes : public Singleton< UIEmotes >
{

private:

	struct Emote
	{
		Emote(  eUIEmoteType emoteType, const gpwstring &inputString, int index, int size, const gpwstring &category )
		{
			m_emoteType = emoteType;
			m_inputString.assign( inputString );
			m_category.assign( category );
			m_index = index;
			m_size = size;
		}

		Emote(  )
		{
			m_index = 0;
			m_size = 0;
			m_emoteType = UI_EMOTE_TYPE_ANY;
		}

		eUIEmoteType m_emoteType;
		gpwstring	m_inputString;
		gpwstring	m_category;
		int			m_index;
		int			m_size;

	};

	struct TextEmote
	{
		TextEmote(  eUIEmoteType emoteType, const gpwstring &screenGroup, const gpwstring &screenIndividual, const gpwstring &category )
		{
			m_emoteType = emoteType;
			m_screenIndividual.assign( screenIndividual );
			m_screenGroup.assign( screenGroup );
			m_category.assign( category );
		}

		TextEmote(  )
		{
			m_emoteType = UI_EMOTE_TYPE_ANY;
		}

		eUIEmoteType m_emoteType;
		gpwstring	m_screenIndividual;
		gpwstring	m_screenGroup;
		gpwstring	m_category;

	};

	typedef std::map< gpwstring, Emote, istring_less > EmoteMap;
	typedef std::map< gpwstring, TextEmote, istring_less > TextEmoteMap;
	typedef std::list< gpwstring > CategoryColl;

	bool ChangeColor			( EmoteMap::const_iterator findColorEmote, DWORD &outColor ) const;
	bool DrawTexture			( EmoteMap::iterator findTextureEmote, const UIRect &rectPosition, UIWindow* parent, bool renderTexture = true );

	EmoteMap		m_emotes;
	TextEmoteMap	m_textEmotes;
	CategoryColl	m_categories;

	gpwstring m_emoteTagBegin;
	gpwstring m_emoteTagEnd;
	gpwstring m_escapeSequence;
	gpwstring m_listAllSequence;

	int m_maxEmotesPerCategory;


public:
	UIEmotes();
	~UIEmotes();

	void Init();

	void Draw					( RapiFont *font, int left, int top, const gpwstring &text, DWORD color, UIWindow* parent, bool uselinear, int &width, int &height, bool renderText = true );
	void Draw					( RapiFont *font, int left, int top, const gpwstring &text, DWORD color, UIWindow* parent, bool uselinear = false );
	void CalculateStringSize	( RapiFont *font, const gpwstring &text, int &width, int &height );

	void PrepareString			( gpwstring &preparedString )const ;
	bool PrepareStringTextEmotes( const gpwstring &inString, gpwstring &outPreparedString, const gpwstring &playerName, const gpwstring &targetIndividualName, eUITextEmoteType textEmoteType = UI_TEXT_EMOTE_TYPE_GROUP )const ;
	void TextEmoteKeywordReplacement( gpwstring &outPreparedString, const gpwstring &playerName, const gpwstring &targetIndividualName )const ;

	void ListAllEmotes			( gpwstring &outPreparedString, eUIEmoteType emoteType = UI_EMOTE_TYPE_ANY )const ;

	EmoteMap&				GetEmotes()			 {	return m_emotes;		}
	TextEmoteMap&			GetTextEmotes()		 {	return m_textEmotes;	}
	CategoryColl&			GetCategories()		 {	return m_categories;	}
	int						GetMaxEmotesCategory()  { return m_maxEmotesPerCategory; }


};

#define gUIEmotes UIEmotes::GetSingleton()

#endif

