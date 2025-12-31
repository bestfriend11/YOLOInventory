//////////////////////////////////////////////////////////////////////////////
//
// File     :  ui_emotes.cpp
// Author(s):  Jessica Tams
//
// Summary  :  Layer to handle emotes in ui text windows
//
// Copyright © 2002 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////
//
//	Todo:
//	escape sequences
//	devonly command
//		

#include "precomp_gamegui.h"
#include "ui_emotes.h"

UIEmotes::UIEmotes()
{
	Init();
}

UIEmotes::~UIEmotes()
{
	// remove all of our emote textures!
	for ( EmoteMap::iterator emote = m_emotes.begin(); emote != m_emotes.end(); ++emote )
	{
		if ( emote->second.m_index != 0 && emote->second.m_emoteType == UI_EMOTE_TYPE_TEXTURE )
		{
			gDefaultRapi.DestroyTexture( emote->second.m_index );
		}
	}
	m_emotes.clear();
}

void UIEmotes :: Init()
{
	// clear out all old emotes
	m_emotes.clear();

	// set the defaults
	m_emoteTagBegin.assign(L"{{{");
	m_emoteTagEnd.assign(L"}}}");
	m_escapeSequence.assign(L"/noemotes");
	m_listAllSequence.assign(L"/listall");
	m_maxEmotesPerCategory = 20;

	

	FastFuelHandle hEmotesFile( "ui:config:emotes" );
	if ( hEmotesFile.IsValid() )
	{
		/////////////////////////////////////////////////////
		// properties
		FastFuelHandle hProperties = hEmotesFile.GetChildNamed( "properties" );
		if( hProperties.IsValid() )
		{
			m_emoteTagBegin = ToUnicode( hProperties.GetString("internal_emote_begin",ToAnsi(m_emoteTagBegin)) );
			m_emoteTagEnd = ToUnicode( hProperties.GetString("internal_emote_end",ToAnsi(m_emoteTagEnd)) );
			m_escapeSequence = ToUnicode( hProperties.GetString("escape_sequence",ToAnsi(m_escapeSequence)) );
			m_listAllSequence = ToUnicode( hProperties.GetString("list_all_sequence",ToAnsi(m_listAllSequence)) );
			m_maxEmotesPerCategory = hProperties.GetInt("max_emotes_category", m_maxEmotesPerCategory );
		}


		FastFuelHandle hCategoryList = hEmotesFile.GetChildNamed( "categories" );
		if( hCategoryList.IsValid() )
		{
			gpwstring inputString;
			gpstring key;

			FastFuelFindHandle hCategoryListFind(hCategoryList);

			while( hCategoryListFind.GetNextKeyAndValue( key, inputString ) )
			{
				m_categories.push_back( ReportSys::TranslateW( inputString ) );
			}
		}

		/////////////////////////////////////////////////////
		// get all of the texture emotes listed in the gas file
		

		FastFuelHandleColl hEmotesList;
		hEmotesFile.ListChildrenTyped( hEmotesList, "emote" );

		if( ! hEmotesList.empty() )
		{
			gpwstring inputString;
			gpwstring codeName;
			gpwstring category;
			int size;

			for( FastFuelHandleColl::iterator hEmote = hEmotesList.begin(); hEmote != hEmotesList.end(); ++hEmote )
			{
				inputString	= ReportSys::TranslateW( hEmote->GetString("input_string") );
				codeName	= ReportSys::TranslateW( hEmote->GetString("code_name") );
				category	= ReportSys::TranslateW( hEmote->GetString("category") );
				size		= hEmote->GetInt("size");

				gpassert( !codeName.empty() );
				gpassert( !inputString.empty() );

				m_emotes[codeName] = Emote( UI_EMOTE_TYPE_TEXTURE, inputString, 0, size, category );
			}
		}
		
		/////////////////////////////////////////////////////
		// get all of the color changes listed in the gas file
		FastFuelHandleColl hColorList;
		hEmotesFile.ListChildrenTyped( hColorList, "color" );
		if( !hColorList.empty() )
		{
			gpwstring codeName;
			gpwstring inputString;
			int color;

			for( FastFuelHandleColl::iterator hColor = hColorList.begin(); hColor != hColorList.end(); ++hColor )
			{
				codeName	= ReportSys::TranslateW( hColor->GetString("code_name") );
				inputString	= ReportSys::TranslateW( hColor->GetString("input_string") );
				color		= hColor->GetInt("color");

				gpassert( !codeName.empty() );
				gpassert( !inputString.empty() );
				
				//Emote(  eUIEmoteType emoteType, gpwstring inputString, int index, int size )
				m_emotes[codeName] = Emote( UI_EMOTE_TYPE_COLOR, inputString, color, 0, gpwstring() );
			}
		}

		/////////////////////////////////////////////////////
		// get all of the text replacement emotes in the gas file
		FastFuelHandleColl hTextEmoteList;
		hEmotesFile.ListChildrenTyped( hTextEmoteList, "text_emote" );
		if( !hTextEmoteList.empty() )
		{
			gpwstring screenGroup;
			gpwstring screenIndividual;
			gpwstring inputString;
			gpwstring category;

			for( FastFuelHandleColl::iterator hTextEmote = hTextEmoteList.begin(); hTextEmote != hTextEmoteList.end(); ++hTextEmote )
			{
				screenGroup			= ReportSys::TranslateW( hTextEmote->GetString("screen_group") );
				screenIndividual	= ReportSys::TranslateW( hTextEmote->GetString("screen_individual") );
				inputString			= ReportSys::TranslateW( hTextEmote->GetString("input_string") );
				category			= ReportSys::TranslateW( hTextEmote->GetString("category") );

				gpassert( !inputString.empty() );
				gpassert( !screenGroup.empty() );

				//TextEmote(  eUIEmoteType emoteType, gpwstring screenGroup, gpwstring screenIndividual, gpwstring category, intlistOrder ); )
				m_textEmotes[inputString] = TextEmote( UI_EMOTE_TYPE_TEXT, screenGroup, screenIndividual, category );
			}
		}
	}	
}

//
// Returns a string with all the emotes in it!
//
void UIEmotes :: ListAllEmotes( gpwstring &outPreparedString, eUIEmoteType emoteType /*=UI_EMOTE_TYPE_ANY*/ ) const
{
	outPreparedString.erase();

	if( emoteType == UI_EMOTE_TYPE_TEXTURE || emoteType == UI_EMOTE_TYPE_ANY )
	{
		for ( EmoteMap::const_iterator emote = m_emotes.begin(); emote != m_emotes.end(); ++emote )
		{
			outPreparedString.appendf( L"%s  ", emote->second.m_inputString );
		}
	}
	if( emoteType == UI_EMOTE_TYPE_TEXT || emoteType == UI_EMOTE_TYPE_ANY )
	{
		for ( TextEmoteMap::const_iterator textEmote = m_textEmotes.begin(); textEmote != m_textEmotes.end(); ++textEmote )
		{
			if( textEmote->first != NULL )
			{
				outPreparedString.appendf( L"%s  ", textEmote->first );
			}
			else
			{
				gperror( "Bad emote trying to be listed" );
			}	
		}
	}
}
//
// replaces the keyword input_string with out screen name
//
bool UIEmotes :: PrepareStringTextEmotes( const gpwstring &inString, gpwstring &outPreparedString, const gpwstring &playerName, const gpwstring &targetIndividualName, eUITextEmoteType textEmoteType /*= UI_TEXT_EMOTE_TYPE_GROUP*/ ) const
{
	outPreparedString = inString;
	bool foundTextEmote = false;

	// this is a special "list all function"
	gpwstring::size_type findIndex = outPreparedString.find( m_listAllSequence );
	if( findIndex != gpwstring::npos )
	{
		ListAllEmotes( outPreparedString );
		outPreparedString.insert( 0, m_escapeSequence );
		foundTextEmote = true;
	}
	else
	{
		// check to see if we should replace the emotes!
		// if we shouldn't replace the emotes, then just return
		findIndex = outPreparedString.find( m_escapeSequence );

		if( findIndex != gpwstring::npos )
		{
			outPreparedString.erase( findIndex, m_escapeSequence.size() );
		}
		else
		{
			// replace the text with the desired text of the emote!
			for ( TextEmoteMap::const_iterator textEmote = m_textEmotes.begin(); textEmote != m_textEmotes.end(); ++textEmote )
			{
				findIndex = outPreparedString.find( textEmote->first );

				while( findIndex != gpwstring::npos )
				{
					foundTextEmote = true;
					outPreparedString.erase( findIndex, textEmote->first.size() );

					if( textEmoteType == UI_TEXT_EMOTE_TYPE_INDIVIDUAL )
					{
						outPreparedString.insert( findIndex, textEmote->second.m_screenIndividual );
					}			
					else //(commented for docuementation) if( textEmoteType == UI_TEXT_EMOTE_TYPE_GROUP )
					{
						outPreparedString.insert( findIndex, textEmote->second.m_screenGroup );
					}

					findIndex = outPreparedString.find( textEmote->first );
				}
			} 

			if( foundTextEmote )
			{
				TextEmoteKeywordReplacement( outPreparedString, playerName, targetIndividualName );
			}
		}
	}

	return foundTextEmote;
}

void UIEmotes :: TextEmoteKeywordReplacement( gpwstring &outPreparedString, const gpwstring &playerName, const gpwstring &targetIndividualName ) const
{
	// replace our gas tags with the real names of the player and the target of the comment
	gpwstring::size_type findIndex = outPreparedString.find( L"<player>" );
	if( findIndex != gpwstring::npos )
	{
		outPreparedString.erase( findIndex, gpwstring(L"<player>").size());
		outPreparedString.insert( findIndex, playerName );
	}

	findIndex = outPreparedString.find( L"<target>" );
	if( findIndex != gpwstring::npos )
	{
		outPreparedString.erase( findIndex, gpwstring(L"<target>").size());
		outPreparedString.insert( findIndex, targetIndividualName );
	}
}


// go through a string to process all inputText and set the code name with tags on either end.
// this will make drawing each frame faster...  while not by much :)
void UIEmotes :: PrepareString( gpwstring &outPreparedString ) const
{
	// check to see if we should replace the emotes!
	// if we shouldn't replace the emotes, then just return
	gpwstring::size_type findIndex = outPreparedString.find( m_escapeSequence );

	if( findIndex != gpwstring::npos )
	{
		outPreparedString.erase( findIndex, m_escapeSequence.size() );
	}
	else
	{
		for ( EmoteMap::const_iterator emote = m_emotes.begin(); emote != m_emotes.end(); ++emote )
		{
			findIndex = outPreparedString.find( emote->second.m_inputString );

			while( findIndex != gpwstring::npos )
			{
				gpwstring replacement;
				replacement.assignf( L"%s%s%s", m_emoteTagBegin.c_str(), emote->first.c_str(), m_emoteTagEnd.c_str() );

				outPreparedString.erase( findIndex, emote->second.m_inputString.size() );
				outPreparedString.insert( findIndex, replacement ); 

				findIndex = outPreparedString.find( emote->second.m_inputString );
			}
		}
	}
}

// returns the size of our text with any emotes inserted.
void UIEmotes :: CalculateStringSize( RapiFont *font, const gpwstring &text, int &width, int &height )
{
	Draw( font, 0, 0, text, 0, NULL, false, width, height, false );
}

void UIEmotes :: Draw( RapiFont *font, int left, int top, const gpwstring &text, DWORD color, UIWindow* parent, bool uselinear )
{
	int width = 0;
	int height = 0;
	Draw( font, left, top, text, color, parent, uselinear, width, height, true );
}

// draws the text and emotes to the screen!
// optional parameter computes the string size, and does not actually draw anything to the screen
void UIEmotes :: Draw( RapiFont *font, int left, int top, const gpwstring &text, DWORD color, UIWindow* parent, bool uselinear, int &width, int &height, bool renderText )
{
	gpassert( font != NULL );
	gpassert( renderText == false || parent != NULL );
	
	int addOffsetLeft = 0;
	int addOffsetTop = 0;

	width = 0;
	height = 0;

	gpwstring preparedString = text;

	gpwstring::size_type start = 0;			// start of the pass
	gpwstring::size_type front = 0;			// first instance of a special character
	gpwstring::size_type back = 0;			// matching instance of the special character
	gpwstring::size_type offsetLeft = 0;	// space used by stuff we have already drawn
	gpwstring::size_type offsetTop = 0;		// space used by stuff we have already drawn(not used)


	gpwstring textureName;
	front = preparedString.find( m_emoteTagBegin, start );
	back = preparedString.find( m_emoteTagEnd, front );
	
	while( start != gpwstring::npos && front != gpwstring::npos && back != gpwstring::npos )
	{
		addOffsetLeft = 0;
		addOffsetTop = 0;

		/////////////////////////////////////////////////
		// draw the first section of text
		gpwstring stringFront = preparedString.mid_safe(start,front-start);
		if( renderText )
		{
			font->Print( left + offsetLeft, top + offsetTop, stringFront.c_str(), color, uselinear );
		}

		// calculate the offset caused by this addition		
		font->CalculateStringSize( stringFront, addOffsetLeft, addOffsetTop );
		offsetLeft	= offsetLeft	+ addOffsetLeft;
		width = width + addOffsetLeft;
		height = max( height, addOffsetTop );
		

		/////////////////////////////////////////////////
		// find the emote that we need to process
		gpwstring emoteCodeName = preparedString.mid_safe( front + m_emoteTagBegin.size(), back - (front + m_emoteTagBegin.size()) );
		EmoteMap::iterator findEmote = m_emotes.find( emoteCodeName ); 

		/////////////////////////////////////////////////
		// process our emote
		if( ChangeColor( findEmote, color ) )
		{
			// functionality in change color.
		}
		else if( DrawTexture( findEmote, UIRect(left + offsetLeft, top + offsetTop, left + offsetLeft + findEmote->second.m_size, top + offsetTop + findEmote->second.m_size), parent, renderText ) )
		{
			// textures take up space, so make sure to increment out offset!
			offsetLeft	= offsetLeft + findEmote->second.m_size;

			width = width + findEmote->second.m_size;
			height = max( height, findEmote->second.m_size );
		}
		else
		{
			gperrorf(("Unknown emote code name: %s", ToAnsi(emoteCodeName)));
		}

		/////////////////////////////////////////////////
		// determine if there are any more textures
		start = back + m_emoteTagEnd.size();
		front = preparedString.find( m_emoteTagBegin, start );
		back = preparedString.find( m_emoteTagEnd, front );
	}	

	if( renderText )
	{
		font->Print( left + offsetLeft , top + offsetTop , preparedString.mid_safe(start,front-start).c_str(), color, uselinear );
	}

	addOffsetLeft = 0;
	addOffsetTop = 0;

	font->CalculateStringSize( preparedString.mid_safe(start,front-start).c_str(), addOffsetLeft, addOffsetTop );

	width = width + addOffsetLeft;
	height = max( height, addOffsetTop );
	
}

// Test if this emote should change the color of the rest of the line
// if we change the color, return true, if not return false.
bool UIEmotes :: ChangeColor( EmoteMap::const_iterator findColorEmote, DWORD &outColor ) const
{
	// find the code name in out emote map, if we find it, and it is a color
	// then set outColor to that emote's color
	if( findColorEmote != m_emotes.end() && findColorEmote->second.m_emoteType == UI_EMOTE_TYPE_COLOR )
	{
		outColor = findColorEmote->second.m_index;
		return( true );
	}
	return( false );
}

// Test if this emote should add a little texture.
// if so, add the texture and return true.
bool UIEmotes :: DrawTexture( EmoteMap::iterator findTextureEmote, const UIRect &rectPosition, UIWindow* parent, bool renderTexture )
{
	if( findTextureEmote != m_emotes.end() && findTextureEmote->second.m_emoteType == UI_EMOTE_TYPE_TEXTURE )
	{
		// make sure the texture is set!  If not, then try to load the texture.
		if( findTextureEmote->second.m_index == 0 )
		{
			findTextureEmote->second.m_index = gUITextureManager.LoadTexture( ToAnsi(findTextureEmote->first) );
		}

		if( findTextureEmote->second.m_index != 0 )
		{
			// draw the texture, using the parent uiwindow.
			if( renderTexture )
			{
				gpassert( parent != NULL );

				parent->DrawComponent( rectPosition, findTextureEmote->second.m_index, 1 );
			}
			return( true );
		}
		else
		{
			gperrorf(( "Can not load the emote texture: %s", ToAnsi(findTextureEmote->first) ));
		}
	}
	return( false );
}

