/**************************************************************************

Filename		: UIPartyMember.h

Class			: UIPartyMember

Description		: Every character in the player's party must have one of
				  these UI counterparts in order to maintain and organize
				  their separate UI information

Last Modified	: 1/20/00

Author			: Chad Queen

**************************************************************************/

#pragma once
#ifndef __UI_PARTYMEMBER_H
#define __UI_PARTYMEMBER_H


// Forward Delcarations
class UICharacterDisplay;
class UIStatusBar;
class UIWindow;



// Enumerated Data Types
enum ACTIVE_SELECTION
{
	SELECTION_MELEE_WEAPON,
	SELECTION_RANGED_WEAPON,
	SELECTION_PRIMARY_SPELL,
	SELECTION_SECONDARY_SPELL,
};


enum GUI_WINDOW_INDEX
{	
	GUI_HEALTH,
	GUI_MANA,
	GUI_PORTRAIT,
	GUI_DEATH,
	GUI_GHOST_TIMER,
	GUI_SKILL_BAR_1,
	GUI_SKILL_BAR_2,
	GUI_SKILL_BAR_3,
	GUI_SKILL_BAR_4,	
	GUI_SKILL_BAR_ACTIVE,
	GUI_FUEL_BAR,
	GUI_MIN_FUEL_BAR,
	GUI_AIS_MELEE_WEAPON,
	GUI_AIS_RANGED_WEAPON,
	GUI_AIS_PRIMARY_SPELL,
	GUI_AIS_SECONDARY_SPELL,	
	GUI_HEALTH_FLOAT,
	GUI_MANA_FLOAT,
	GUI_EXPERT_PANEL,
	GUI_ACTIVE_MINIMIZED,
	GUI_PORTRAIT_ROLLOVER,
	GUI_MULTI_GOLD,
	GUI_HEALTH_WARNING,
	GUI_UNCONSCIOUS,
	GUI_PACK_PANEL_MAX,
	GUI_PACK_PANEL_MIN,
	GUI_GUARDED,

	GUI_MAX_INDEX,
};





// This class contains all the interface specific information that each party member
// must have. ( essentially ui preferences )
class UIPartyMember
{
public:

	// Constructor
	UIPartyMember( Goid go );									

	// Destructor
	~UIPartyMember();

	Goid GetGoid()											{ return m_goid;						}

	// UI Option Settings
	bool IsInventoryExpanded()									{ return m_bInventoryExpanded;			}
	bool IsSpellsExpanded()										{ return m_bSpellsExpanded;				}
	bool IsGoodMagicSelected()									{ return m_bGoodMagicSelected;			}
	bool IsCharacterExpanded()									{ return m_bCharacterExpanded;			}
	bool IsNavBarVisible()										{ return m_bNavBarVisible;				}

	void SetInventoryExpanded( bool bExpanded )					{ m_bInventoryExpanded	= bExpanded;	}
	void SetSpellsExpanded( bool bExpanded )					{ m_bSpellsExpanded		= bExpanded;	}		
	void SetGoodMagicSelected( bool bGoodMagic )				{ m_bGoodMagicSelected	= bGoodMagic;	}
	void SetCharacterExpanded( bool bExpanded )					{ m_bCharacterExpanded	= bExpanded;	}
	void SetNavBarVisible( bool bVisible)						{ m_bNavBarVisible		= bVisible;		}

	// Find the active weapon/magic/item slot on the UI
	void				SetActiveSlot( ACTIVE_SELECTION slot )	{ m_ActiveSlot = slot;					}
	ACTIVE_SELECTION	GetActiveSlot()							{ return m_ActiveSlot;					}

	UICharacterDisplay * GetUICharacterDisplay()				{ return m_pCharacterDisplay;			}
	void SetPaperDollVisible( bool bVisible )					{ m_bPaperDollVisible = bVisible;		}
	bool GetPaperDollVisible()									{ return m_bPaperDollVisible;			}
	void RenderPaperDoll();

	UIWindow * GetGUIWindow( GUI_WINDOW_INDEX index )			{ return m_pWindow[index];				}
	void SetGUIWindow( UIWindow *pWindow, GUI_WINDOW_INDEX index ) { m_pWindow[index] = pWindow;		}

	void SetHeight( float height )								{ m_Height = height;					}
	float GetHeight()											{ return m_Height;						}

private:

	bool				m_bInventoryExpanded;
	bool				m_bSpellsExpanded;
	bool				m_bCharacterExpanded;
	bool				m_bNavBarVisible;
	bool				m_bGoodMagicSelected;
	bool				m_bPaperDollVisible;
	ACTIVE_SELECTION	m_ActiveSlot;
	float				m_Height;

	UIWindow			*m_pWindow[GUI_MAX_INDEX];

	Goid			m_goid;
	UICharacterDisplay	*m_pCharacterDisplay;
};


#endif 