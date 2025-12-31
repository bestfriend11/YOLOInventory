/**************************************************************************

Filename		: UIPartyManager.h

Class			: UIPartyManager

Description		: This is a class that handles all the necessary functions
				  needed to maintain the player's party.  This includes:
				  equipment, inventory, interface settings/movement, and 
				  user input/interaction with gui elements as it relates
				  to the party.

Author			: Chad Queen

				(C)opyright Gas Powered Games 1999

**************************************************************************/


#pragma once
#ifndef _UIPARTYMANAGER_H_
#define _UIPARTYMANAGER_H_


// Forward Declarations
class UIPartyMember;
class UIWindow;
class Skill;


// Include Files
#include "GoInventory.h"
#include "UIDialogueHandler.h"
#include "UIPartyMember.h"
#include "UIGame.h"



// Enumerated Data Types
enum GUI_STATISTIC
{
	// Standard
	NAME,
	CLASS,
	MAX_HEALTH,
	MAX_MANA,

	// Gold
	GOLD_VALUE,
	PACKMULE_GOLD_VALUE,
	MULTI_GOLD_1,
	MULTI_GOLD_2,
	MULTI_GOLD_3,
	MULTI_GOLD_4,
	MULTI_GOLD_5,
	MULTI_GOLD_6,
	MULTI_GOLD_7,
	MULTI_GOLD_8,
	
	// Stats
	INT_VALUE,
	STRENGTH_VALUE,
	DEX_VALUE,	
	MELEE_DAMAGE_VALUE,	
	RANGED_DAMAGE_VALUE,	
	ARMOR_VALUE,	
	
	// Skill Experience
	MELEE_EXP_VALUE,
	RANGED_EXP_VALUE,	
	DARKMAGIC_EXP_VALUE,	
	GOODMAGIC_EXP_VALUE,		
	MELEE_BAR,	
	RANGED_BAR,
	GOODMAGIC_BAR,
	DARKMAGIC_BAR,
	HEALTH_BAR,
	MANA_BAR,
	STRENGTH_BAR,
	DEXTERITY_BAR,
	INTELLIGENCE_BAR,
	
	// These are for the character selection indicators on the portraits
	PICTURE_SELECT_1,
	PICTURE_SELECT_2,
	PICTURE_SELECT_3,
	PICTURE_SELECT_4,
	PICTURE_SELECT_5,
	PICTURE_SELECT_6,
	PICTURE_SELECT_7,
	PICTURE_SELECT_8,	

	// The rest are for the advanced infobar stuff.
	TEXT_INFORMATION,
	
	// Status Bar for enemies
	ENEMY_HEALTH_BAR,	

	// Mouse listener window for rotating the paperdoll
	CHAR_RECT,

	SELECT_ALL,

	POTION_HEALTH,
	POTION_MANA,

	DATA_DOCKBAR,
	
	TRADE_GOLD,

	// For when the character is being guarded 
};

#define STATS_MAX_INDEX 51
#define INVALID_MEMBER_INDEX -1

enum eMultiBar
{
	SET_BEGIN_ENUM( MB_, 0 ),

	MB_HEALTH_1,
	MB_MANA_1,
	MB_HEALTH_2,
	MB_MANA_2,
	MB_HEALTH_3,
	MB_MANA_3,
	MB_HEALTH_4,
	MB_MANA_4,
	MB_HEALTH_5,
	MB_MANA_5,
	MB_HEALTH_6,
	MB_MANA_6,
	MB_HEALTH_7,
	MB_MANA_7,	

	SET_END_ENUM( MB_ ),
};

struct SelectedMembers
{
	GoidColl group;
	Goid	 focusGoid;
	bool	 bExplicitlySet;	
};

struct UserEdTooltip
{
	gpstring sEvent;
	gpstring sTip;
};

struct strmap_less
{
	bool operator()(const UserEdTooltip & x, const UserEdTooltip & y) const
	{
		return( ( compare_no_case( x.sEvent.c_str(), y.sEvent.c_str() ) < 0 ) );
	}
	bool operator()(const gpstring & x, UserEdTooltip & y) const
	{
		return( ( compare_no_case( x.c_str(), y.sEvent.c_str() ) < 0 ) );
	}
	bool operator()(const UserEdTooltip & x, const gpstring & y) const
	{
		return( ( compare_no_case( x.sEvent.c_str(), y.c_str() ) < 0 ) );
	}

};

typedef std::vector< UserEdTooltip > UserEdMap;


class UIPartyManager : public Singleton <UIPartyManager>
{
public:	

	// Constructor
	UIPartyManager();
	void Init();
	
	// Destructor
	~UIPartyManager();
	void Deinit();

	// Updating
	void		Update( float secondsElapsed );
	void		Draw();
	void		UpdatePlayerStats		();

	void		EnsureSelection();
	
	void		ShowHireStats( Go * pHire );

	void		CalculateSkillTextColor	( Skill * pSkill, UIText * pText );	
	void		CalculateAlterationEffects( Go * pMember, 
											int & healthDelta, int & manaDelta, 
											int & armorDelta, int & meleeDamageDelta,
											int & rangedDamageDelta );
	void		CalculateStatTextColor( int delta, UIText * pText );
	void		UpdateInfoBox			();		

	void		DrawStatusBar			( Go * pObject, UIStatusBar * pHealth, UIStatusBar * pMana );
	void		DrawMemberStatusBars	( int memberIndex );	
	void		DrawTeamStatusBars		();
	void		DrawEnemyStatusBars		( Go * pObject );
	void		DrawStatusBars			();	

	void		SetDrawStatusBars( bool bDraw ) { m_bDrawStatusBars = bDraw; }
	bool		GetDrawStatusBars()				{ return m_bDrawStatusBars; }
	bool		ToggleStatusBars();

	void		SetDrawMemberLabels( bool bSet );
	bool		GetDrawMemberLabels()				{ return m_bDrawMemberLabels; }
	bool		ToggleMemberLabels();
	void		DrawMemberLabels();

	void		SetPortraitInactiveColor( DWORD dwColor )	{ m_dwInactiveColor = dwColor;	}
	DWORD		GetPortraitInactiveColor()					{ return m_dwInactiveColor;		}

	void		UpdateQuestCallback( int questId );

	void		Xfer( FuBi::PersistContext& persist );
	void		PostXfer();

	void		EnableHealthButton( bool bEnable );
	void		EnableManaButton( bool bEnable );

	//********************* UI Party Management ********************

	// This constructs the ui information using the party that is 
	// currently active within the game.  
	void ConstructUIParty();
	void DeconstructUIParty();
	void ConstructCharacterInterface( int ID, bool bDeselectAll = true, bool bStore = false );

	void SetCharacterActive ( bool bActive );
	void SetSelectedCharacterActive();

	void SetInventoryActive	( bool bActive, bool bStore = false );
	void SetSelectedInventoryActive();

	bool SetSpellsActive	( bool bActive );
	void SetSelectedSpellsActive();

	Goid GetSelectedCharacter();
	void SetSelectedCharacterIndex( int sel ) { m_SelectedMember = sel; }
	int	 GetSelectedCharacterIndex() { return m_SelectedMember; }
	UIPartyMember * GetSelectedCharacterFromIndex( int index ) { return m_UIPartyMembers[index]; } 
	UIPartyMember * GetSelectedPartyMember() { return m_UIPartyMembers[m_SelectedMember]; } 
	UIPartyMember * GetCharacterFromGoid( Goid member, int & index );

	void GetSelectedPartyMembers( GoidColl & party );
	void GetPartyMembers( GoidColl & party );
	
	bool ActivateInventory( bool bSelecting = false );
	void ActivateSingleInventory( Go * pMember );
	bool ActivateCharacterStats();
	bool ActivateSpellBook();
	Goid GetFocusSpellBook() { return m_SpellBookFocus; }
	void RenameSelectedSpellBook( const gpwstring & customName );
	
	// Expert Mode Settings

		bool ExpertMode();
		bool IsExpertModeEnabled() { return ( m_bExpertMode || m_bExpertModeActive ); }

		// User Side		
		void SetExpertMode( bool bExpert )	{ m_bExpertMode = bExpert; ExpertMode();	}
		bool GetExpertMode()				{ return m_bExpertMode;						}
		bool ActivateExpertMode();

		// Code Side
		void SetExpertModeActive( bool bExpert )	{ m_bExpertModeActive = bExpert; ExpertMode();	}
		bool GetExpertModeActive()					{ return m_bExpertModeActive;		}
	
	bool IsPlayerPartyMember( Goid object, int & index );
	bool IsPlayerPartyMember( Goid object );
	bool IsPartyMemberSelected();
	
	void SelectValidPortraits( const GopColl & guids );
	void SelectPortrait( int charIndex );
	void SelectPortraitIndirect( int charIndex );

	// Close all party member sheet related gui's
	bool CloseAllCharacterMenus();

	// Get a go's "active" icon.  Used for quick weapon access
	gpstring	GetActiveIconFromGO( Go * pGO );
	
	// Inventory Visibility
	void SetInventoriesVisible( bool bVisible ) { m_bInventoryActive = bVisible;	}
	bool GetInventoriesVisible()				{ return m_bInventoryActive;		}

	// Hide a Character's Inventory
	void HideInventory( gpstring sGroup, int charIndex );

		// Rotate the 3D paperdoll
	void RotatePaperDoll( float rotateValue );

	bool GetMustHaveSelection() { return m_bMustHaveSelection; }
	void SetMustHaveSelection( bool bSelection ) { m_bMustHaveSelection = bSelection; }

	void SetLastSelectedMembers( GoidColl & members );
	void GetLastSelectedMembers( GoidColl & members );

	void DisbandSelectedMember();
	void DisbandCancelSelectedMember();
	void DisbandCancelAll();
	void DisbandDialog();
	bool DisbandButton();
	bool CanDisband( Goid member, Go * & pTalker );
	void DisbandDeadMember( Go * pMember );

	void ArrangeFormation( const gpstring & sShape );
	
	bool SortMemberInventory( int index );
	bool SortInventory();

	bool ToggleQuestLog();

	void LoreBookDialog( Go * pLoreBook );
	void HideLoreBook();

	void GetPortraitOrderedParty( GoidColl & partyColl );
	int  GetMemberPortraitRank( Go * pMember );

	bool IsCharacterInterfaceBusy( Go * pMember, bool bForce = false );

	void SetRehireObject( Goid object ) { m_memberRehire = object;	}
	Goid GetRehireObject()				{ return m_memberRehire;	}

	void SetMultipleDisband( bool bSet )	{ m_bMultipleDisband = bSet; }
	bool GetMultipleDisband()				{ return m_bMultipleDisband; }

	void SetSingleInventoryVisible( bool bVisible ) { m_bSingleInventoryVisible = bVisible; }
	bool GetSingleInventoryVisible()				{ return m_bSingleInventoryVisible;		}

	void PortraitRollover( UIWindow * pWindow );
	void PortraitRolloff( UIWindow * pWindow );

	void ButtonCloseCharacter( int charIndex );

	void CloseSpellBook();

	// Character Respawning
	void RespawnCharacter();
FEX FuBiCookie RSCreateTombstone( Goid member );
FEX FuBiCookie RSReviveCharacter( Goid member );

	// Group setting
	void SelectGroup( int index );
	bool GetGroup1();
	bool GetGroup2();
	bool GetGroup3();
	bool GetGroup4();
	bool GetGroup5();
	bool GetGroup6();
	bool GetGroup7();
	bool GetGroup8();

	void SwitchGroups( int charIndex1, int charIndex2 );

	void GetSelectedGroup( SelectedMembers & group );
	bool SetGroup1();
	bool SetGroup2();
	bool SetGroup3();
	bool SetGroup4();
	bool SetGroup5();
	bool SetGroup6();
	bool SetGroup7();
	bool SetGroup8();

	void GetGroupBinding( Goid member, gpstring & sSave, gpstring & sRecall );

	bool DoesPartyHaveGhost( GoidColl & selectedParty );

	void CastOnCharacterPortrait( int charIndex );

	// ******************** Orders *********************************

	// Activate the field command GUI
	bool ActivateFieldCommands();
	bool GetFieldCommands() { return m_bFieldCommands; }

	void SetTargetingOrder( eFocusOrders order );
	void SetAttackOrder( eCombatOrders order );
	void SetMovementOrder( eMovementOrders order );

	void SetFullInventoryHeight( int height )	{ m_fullInventoryHeight = height;	}
	int  GetFullInventoryHeight()				{ return m_fullInventoryHeight;		}

	void MinimizeFieldCommands();
	void MaximizeFieldCommands();
	
	void MovementOrderRotate();
	void AttackOrderRotate();
	void TargetingOrderRotate();

	void HandleGuiOrder( const gpstring & sOrder );

	void InitOrders();

	void SetCommandsActive( bool bSet );
	void SetFormationsActive( bool bSet );

	bool GetCommandsActive() { return m_bCommandsActive; }
	bool GetFormationsActive() { return m_bFormationsActive; }

	void ShowEquipment();
	void HideEquipment();

	void OnScreenSize();

	void SetPositiveTextColor( DWORD dwColor ) { m_dwPosTextColor = dwColor; }
	void SetNegativeTextColor( DWORD dwColor ) { m_dwNegTextColor = dwColor; }
	DWORD GetPositiveTextColor()				   { return m_dwPosTextColor; }
	DWORD GetNegativeTextColor()				   { return m_dwNegTextColor; }

	void HandleMenusFromCommand();

	bool CollectLoot();

	bool CycleFormations();

	bool GetRolloverCast()			{ return m_bRolloverCast; }
	Goid GetRolloverCastObject()	{ return m_rollCastObject; }
	Goid GetRolloverCastMember()	{ return m_rollCastMember; }
	Goid GetRolloverCastSpell()		{ return m_rollCastSpell; }

	void SetRolloverCast( bool bSet ) { m_bRolloverCast = bSet; }
	void SetRolloverCastObject( Goid object ) { m_rollCastObject = object; }
	void SetRolloverCastMember( Goid member ) { m_rollCastMember = member; }
	void SetRolloverCastSpell( Goid spell ) { m_rollCastSpell = spell; }

	void SetRolloverGet( bool bSet )	{ m_bRolloverGet = bSet; }
	bool GetRolloverGet()				{ return m_bRolloverGet; }

	void SetRolloverAttack( bool bSet ) { m_bRolloverAttack = bSet; }
	bool GetRolloverAttack()			{ return m_bRolloverAttack; }

	void SetMaxGenericStateDisplay( int max )	{ m_maxGenericStateDisplay = max;	}
	int	 GetMaxGenericStateDisplay()			{ return m_maxGenericStateDisplay;	}

	int	 GetDataBarVariation()			{ return m_dataBarVariation; }

	// ******************** Misc *********************************

	bool IsInDataDockBar();
	void EndDataBarDock();
FEX Goid DoesActivePartyHaveTemplate( const char * sTemplate, Goid target );
	void GetOpenInventories( GoidColl & openInv ) { openInv = m_openInventories; }

	void SetFollowMode( bool bSet );
FEX	bool GetFollowMode()			{ return m_bFollowMode; }

	void AssignDefaultGroupKey( int index, Goid member );

	void UpdateRespawn();

	void ResetPartyGridScale();

	void RolloverHighlightCb( bool & bHighlight );

	void PartyCb( void );

	UIDialogueHandler * GetUIDialogueHandler() { return m_pDialogueHandler; }

	bool GetEquipmentVisible() { return m_bEquipmentVisible; }

	// Keep a list of all human players that are being guarded
FEX	void SAddCharacterAsGuarded( Goid source, Goid character );
FEX void RCAddCharacterAsGuarded( Goid character, DWORD machineId );
	void AddCharacterAsGuarded( Goid character );

FEX	void SRemoveCharacterAsGuarded( Goid source, Goid character );
FEX void RCRemoveCharacterAsGuarded( Goid character, DWORD machineId );
	void RemoveCharacterAsGuarded( Goid character );

	const GoidColl & GetGuardedCharacters( void ) const	{ return m_GuardedCharacters;	}
	void ClearGuardedCharacters( void )					{ m_GuardedCharacters.clear();	}

	void AdjustAwpToggleLocation( void );

	void CastCb( Goid spell );

	void SetRolloverItem( Goid item )	{ m_RolloverItem = item; }
	Goid GetRolloverItem()				{ return m_RolloverItem; }

	void	SetRolloverTimer( float time )	{ m_RolloverTimer = time; }
	float	GetRolloverTimer()				{ return m_RolloverTimer; }

private:

	void InitUserEdTooltips();
	gpwstring RetrieveGuiUserEdTooltip( gpstring sEvent );
	gpwstring RetrieveItemUserEdTooltip( gpstring sEvent );

	UIPartyMember *								m_UIPartyMembers[MAX_PARTY_MEMBERS];
	UIText *									m_UIMemberLabels[MAX_PARTY_MEMBERS];

	typedef std::map< Goid, int > PartyIndexMap;
	PartyIndexMap								m_PartyIndexMap;

	int											m_SelectedMember;
	Goid										m_ScreenParty;				// this is the id of the current UI active party	
	unsigned int								m_ScreenPartySize;
	bool										m_bExpertMode;
	bool										m_bExpertModeActive;
	bool										m_bInventoryActive;
	bool										m_bSingleInventoryVisible;
	UIWindow *									m_pWindow[STATS_MAX_INDEX];	
	UIWindow *									m_pMultiBars[MB_COUNT];
	bool										m_bStartupTrack;
	UIDialogueHandler *							m_pDialogueHandler;
	bool										m_bDrawStatusBars;
	bool										m_bDrawMemberLabels;
	bool										m_bMustHaveSelection;
	GoidColl									m_LastSelectedMembers;
	int											m_fullInventoryHeight;
	DWORD										m_dwInactiveColor;
	Goid										m_memberRehire;
	Goid										m_memberDisband;
	bool										m_bMultipleDisband;
	GoidColl									m_disbandRequests;
	bool										m_bFieldCommands;
	Goid										m_memberCommands;
	UserEdMap									m_userItemEdMap;
	UserEdMap									m_userGuiEdMap;
	bool										m_bEquipmentVisible;
	bool										m_bSelecting;
	DWORD										m_dwPosTextColor;
	DWORD										m_dwNegTextColor;
	bool										m_bCommandsActive;
	bool										m_bFormationsActive;
	bool										m_bInitializingOrders;
	bool										m_bFollowMode;
	bool										m_bUpdateRespawn;
	bool										m_bActivateInvSound;
	bool										m_bGridPlace;
	Goid										m_SpellBookFocus;

	// User education rollover stuff
	bool										m_bRolloverCast;
	Goid										m_rollCastObject;
	Goid										m_rollCastMember;
	Goid										m_rollCastSpell;	
	bool										m_bRolloverGet;
	bool										m_bRolloverAttack;
	int											m_dataBarVariation;
	int											m_maxGenericStateDisplay;

	float										m_RolloverTimer;
	Goid										m_RolloverItem;

	SelectedMembers								m_group[MAX_PARTY_MEMBERS];

	GoidColl									m_openInventories;
	GoidColl									m_partyOrder;	
	GoidColl									m_GuardedCharacters;

	FUBI_SINGLETON_CLASS( UIPartyManager, "This is the UIPartyManager singleton" );
};


#define gUIPartyManager UIPartyManager::GetSingleton()


#endif