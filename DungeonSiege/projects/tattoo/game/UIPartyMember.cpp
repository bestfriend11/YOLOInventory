/**************************************************************************

Filename		: UICharacterDisplay.cpp

Class			: UICharacterDisplay

Description		: Handles the in-game 3D character model display for the 
				  paperdoll.

Last Modified	: 1/19/00

Author			: Chad Queen

**************************************************************************/



// Include Files
#include "precomp_game.h"
#include "CameraAgent.h"
#include "UICharacterDisplay.h"
#include "ui_statusbar.h"
#include "UIPartyMember.h"


/**************************************************************************

Function		: UICharacterDisplay()

Purpose			: Constructor

Returns			: No return value

**************************************************************************/
UIPartyMember::UIPartyMember( Goid go )
: m_bPaperDollVisible( false )
{ 
	m_goid = go;						
	m_pCharacterDisplay = new UICharacterDisplay( go );
}



/**************************************************************************

Function		: ~UICharacterDisplay()

Purpose			: Destructor

Returns			: No return value

**************************************************************************/
UIPartyMember::~UIPartyMember()
{
	if ( m_pCharacterDisplay )
	{
		Delete( m_pCharacterDisplay );
	}
}


/**************************************************************************

Function		: RenderPaperDoll()

Purpose			: Renders the paperdoll of the party member

Returns			: void

**************************************************************************/
void UIPartyMember::RenderPaperDoll()
{
	GetUICharacterDisplay()->Draw();
}