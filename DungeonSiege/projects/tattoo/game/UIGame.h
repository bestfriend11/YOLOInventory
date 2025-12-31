/**************************************************************************

Filename		: UIGame.h

Class			: UIGame

Description		: Handles all the in-game interface elements.  This includes
				  the GUI, and all user input handling

Authors			: Bartosz Kijanka, Chad Queen

					(C)opyright Gas Powered Games 1999

**************************************************************************/



#pragma once
#ifndef __UIGAME_H
#define __UIGAME_H

#include "ui.h"
#include "siege_pos.h"
#include "GoDefs.h"
#include "Rapi.h"

// Forward Class Definitions
class UICamera;
class UIGame_console_commands;
class Command;
class GoHandle;
class UI;
class InputBinder;
class Input;
class KeyInput;
class UIShell;
class UIWindow;
class UIItem;
class UIText;
class UIInfoSlot;
class UIStatusBar;
class UIPartyMember;
class UICharacterDisplay;
class CameraController;
class UICommands;
class UIPartyManager;
class UITextBox;
class UIInventoryManager;
class UIMenuManager;
class UIStoreManager;
class UIItemOverlay;
class UIGameConsole;
class UIPlayerRanks;
class UIEmoteList;
class UITeamManager;
class UIDialogueHandler;

// Forward Enums
enum eCameraMode;

// Namespace Forward Declarations
namespace siege {
	class database_guid;
	class Camera;
}


// Enumerated Data Types
enum ActionSource { SOURCE_NONE, SOURCE_INVENTORY, SOURCE_PAPERDOLL, SOURCE_ACTOR };
enum WatchWindows { BEHAVIOR_WATCH, ORDERS_WATCH, SKILLS_WATCH, STAT_WATCH };

enum eBackendMode
{
	// Default
	BMODE_NONE,

	// Can draw the green selection box
	BMODE_SELECTION_BOX,

	// Dragging an item from the world
	BMODE_DRAG_ITEM,

	// Dragging an item from inventory
	BMODE_DRAG_INVENTORY,

	// Formations
	BMODE_ASSIGN_FORMATION_SPOT,
	BMODE_ASSIGN_FORMATION_POS,
};


enum eBackendDialog
{
	DIALOG_NONE,

	DIALOG_OK,

	DIALOG_OVERWRITE_SAVED_GAME,
	DIALOG_EXIT_GAME,
	DIALOG_EXIT_WINDOWS,
	DIALOG_EXIT_GAME_STAGING_AREA,
	DIALOG_DISBAND,
	DIALOG_PACK_REHIRE,
	DIALOG_DEFEAT_LOAD_GAME,
	DIALOG_DEFEAT_MAIN_MENU,
	DIALOG_QUICK_SAVE,
	DIALOG_NO_QUICK_SAVE,
	DIALOG_QUICK_LOAD,
	DIALOG_CONFIRM_SAVE_DELETE,
	DIALOG_CONFIRM_SELL_ALL,
};


enum SELECTION_MODE
{
	SELECTION_INVALID,
	SELECTION_MOVE,
	SELECTION_ATTACK,
};


enum eJipPlayerMenuCommands
{
	MENU_JIP_PLAYER_KICK,
	MENU_JIP_PLAYER_BAN,
};

struct RolloverObjectData
{
	Goid object;
	double secondsTotal;
	UITextBox *	pText;
};

#if !GP_RETAIL
typedef std::map< float, int > LodToPolyMap;
#endif


// Constants
const int ROLLOVER_TIME = 2;
const int MAX_PARTY_MEMBERS = 8;
const int MAX_SPELL_SLOTS = 11;

const float DEFEAT_FADE_OUT_TIME = 1.75f;

// Class Definitions
class UIGame : public UILayer, public Singleton <UIGame>
{

public:

	// Constructor
	UIGame	();

	// Destructor
	~UIGame	();

	//********************* Updating/Rendering *********************

	void Update					( double seconds_elapsed );
	
	void UpdateLoadProgress		( float progress, bool forceBegin3D, const gpwstring & sInfo );
	void UpdateSaveGameProgress	( float progress, bool forceBegin3D, const gpwstring & sInfo );
	void SetJipLoad				( bool bSet ) { m_bJipLoad = bSet; }
	bool IsJipLoad				( void ) { return m_bJipLoad; }
	
FEX	void SetEscNisToFrontEnd	( bool bSet ) { m_bEscNisToFrontEnd = bSet; }

	void Draw					( double seconds_elapsed );
	
	bool IsVisible()			{ return( m_bIsVisible ); }
	
	void SetIsVisible			( bool flag );
	
	void SetAsActive			( bool flag );
	
	bool IsActive()				{ return m_bActive; }

	void SetSelectionBox		( bool flag );
	
	void SetInfoMode			( gpstring mode );
	
	gpstring GetInfoMode()		{ return m_sInfoMode; }

	void Deinit();

	bool Xfer( FuBi::PersistContext& persist );

	//********************** ZDelta fog ********************************

	void UpdateZDeltaFog( const float delta );



	//********************* Input Handling Routines ****************

	InputBinder &	GetInputBinder()						{ return( *m_pUserInputBinder );  }

	InputBinder &	GetGameInputBinder()					{ return( *m_pGameInputBinder ); }
	InputBinder &	GetUserInputBinder()					{ return( *m_pUserInputBinder ); }
	InputBinder &	GetNisInputBinder()						{ return( *m_pNisInputBinder );  }

	void			PublishInterfaceToInputBinder();
	void			BindInputToPublishedInterface();

	// Input Binder Callbacks

	// Mouse
	bool OnDeltaXCursor( INT32 );
	bool OnDeltaYCursor( INT32 );
	bool OnDeltaZCursor( INT32 );

	// LEFT mouse
	bool OnSelectionDown();
	bool OnSelectionUp	();
	bool IsSelectionDown()						{ return m_bSelectionDown; }

	// MIDDLE mouse
	bool OnSpecialDown	();
	bool OnSpecialUp	();
	bool IsSpecialDown	()						{ return m_bSpecialDown; }

	// RIGHT mouse
	bool OnContextDown	();
	bool OnContextUp	();
	bool IsContextDown	()						{ return m_bContextDown; }
	
	void SetInvertCameraXAxis( bool flag )					{ m_bInvertCameraXAxis = flag; }
	void SetInvertCameraYAxis( bool flag )					{ m_bInvertCameraYAxis = flag; }
	bool GetInvertCameraXAxis()								{ return m_bInvertCameraXAxis; }
	bool GetInvertCameraYAxis()								{ return m_bInvertCameraYAxis; }

	// Game simulation speed
	bool RSGamePauseToggle();

	//FUBI_EXPORT void RSSetGamePause( bool flag );
	//FUBI_EXPORT void RCSetGamePause( bool flag );
	//FUBI_MEMBER_SET( RCSetGamePause, +SERVER );

	bool GameSpeedUp	();
	bool GameSpeedDown	();
	bool GameSpeedReset	();
	bool GameSpeedMin	();
	bool GameSpeedMax	();
	bool GameSingleStep	();

	// App activation status
	virtual void OnAppActivate( bool activate );

	// Escape key pressed in game
	bool InGamePressedEscape();

	// Activate/Deactivate the GUI Editbox
	bool ToggleGUIEditBox();
	bool ToggleGUIEditBoxTeam();
	bool ToggleGUIEditBoxEveryone();
	bool ShowGUIEditBox( bool bTeam = false, bool bEveryone = false );
	bool UpdateGuiEditBox( bool bTeam = false, bool bEveryone = false );
	bool HideGUIEditBox();

	// Messaging
	void SSendMessage( const gpwstring & sMessage );
FEX	void RCSendMessage( const gpwstring & sMessage );

	// Hide/Show MP player rankings
	bool TogglePlayerRanks( const KeyInput & key );

	// Hide/Show MP player labels
	bool TogglePlayerLabels();

	// Hide/Show game timer
	bool ToggleGameTimer();

	// Hide/Show ui item labels
	bool ToggleItemLabels();
	void SetItemLabels( bool bSet );

	// Hide/Show mini-map
	bool ToggleMiniMap();

	// Hide/Show button help bar
	bool ToggleButtonBar();

	bool ShowInGameOptions();

	void ShowDialog( eBackendDialog dialog, gpwstring dialogText = L"" );

	eBackendDialog GetBackendDialog() { return m_BackendDialog; }

	//********************* Camera **********************


	bool SetCameraNoneModeOn();
	bool SetCameraDollyModeOn();
	bool SetCameraZoomModeOn();
	bool SetCameraOrbitModeOn();
	bool SetCameraCraneModeOn();
	bool SetCameraTrackModeOn();

	bool ToggleCameraTrackAndHold();

	bool CameraZoomIn( const Input & input );
	bool CameraZoomOut( const Input & input );
	bool CameraRotateLeft( const KeyInput & key );
	bool CameraRotateRight( const KeyInput & key );
	bool CameraRotateUp( const KeyInput & key );
	bool CameraRotateDown( const KeyInput & key );
	bool CameraFreeLook( const KeyInput & key );
	void CameraInputRecalibrate( bool bForce = false );

	void SetCameraScreenEdgeRotation( const bool rot )		{ m_bCameraScreenEdgeRotate = rot; }
	bool IsCameraScreenEdgeRotation()						{ return m_bCameraScreenEdgeRotate; }

	void SetCameraScreenHorizTrack( const bool track )		{ m_bScreenHorizTrack = track; }
	bool IsCameraScreenHorizTrack()							{ return m_bScreenHorizTrack; }

	void SetCameraScreenVertTrack( const bool track )		{ m_bScreenVertTrack = track; }
	bool IsCameraScreenVertTrack()							{ return m_bScreenVertTrack; }

	void SetHorizontalScreenEdgeBuffer( const float buf )	{ m_camHorizontalEdgeBuffer = buf; }
	void SetVerticalScreenEdgeBuffer( const float buf )		{ m_camVerticalEdgeBuffer = buf; }

	float GetMapShowBorderMeters() { return m_MapShowBorderMeters; }

	//********************* UI States/Exiting **********************

	bool RequestQuit();
	bool ExitNis();

	void ShowDefeatScreen( double secondsElapsed );

	//********************* Object Selection Routines **************

	void	Selection_Clear		();
	bool	Selection_Delete	();					// delete current selection
	bool	Selection_Copy		();					// copy current selection into a 'clipboard'
	bool	Selection_Paste		();					// paste current selection - center at cursor
	void	SelectionChanged	( void );
FEX	GoidColl & GetSelectedItems	();					// retrieve ONLY the ITEMS that are currently selected by the user


#	if !GP_RETAIL
	
	bool	DebugMindProbe		();					// toggle object ai probe on current object
	bool	DebugMind2Probe		();
	bool	DebugCombatProbe	();					// toggle object combat probe on current object
	bool	DebugBodyProbe		();					// toggle object body probe on current object
	bool	DebugAspectProbe	();					// toggle object aspect probe on current object
	bool	DebugProbeVerboseToggle();				// toggle verbose debug probe mode
	bool	SetDebugProbeObject();					// helper for the above methods

	bool	DebugObjectUpdateBreak	();				// GODB will break into debugger before updating object under cursor
	bool	DebugToggleSelectionBrain();			// toggle clearing/setting the brains of the selected objects

	bool	DebugHudObject();
	bool	DebugHudActors();
	bool	DebugHudItems();
	bool	DebugHudCommands();
	bool	DebugHudMisc();
	bool	DebugHudAll();
	bool	DebugHudLabels();
	bool	DebugHudErrors();
	bool	DebugHudDepth();
	bool	DebugIncrementAnimation();	

#	endif // !GP_RETAIL

	bool 	MoveOrderFree();    
	bool 	MoveOrderLimited(); 
	bool 	MoveOrderNever();   

	bool 	FightOrderAlways();     
	bool 	FightOrderBackOnly();  
	bool 	FightOrderNever();      

	bool	TargetOrderClosest();
	bool	TargetOrderWeakest();
	bool	TargetOrderStrongest();

	bool	PartyHealBodyWithPotions();
	bool	PartyHealMagicWithPotions();
	bool	PartyHealBodyWithMagic();

	//----- formations

	bool	FormationIncreaseSpacing();
	bool	FormationDecreaseSpacing();

	void	RankPartyMembers();
	
	bool	GetShowFormation();
	void	SetShowFormation( float delay = 0.0f );

	void	ResetFormationTimer() { m_TimeToDisplayFormation = ::GetGlobalSeconds(); }

	bool	SelectObjectsInBox();

	void	DrawSelectionIndicators();				// Draws selection indicators around selected objects
	void	DrawAreaOfEffectVolume( Goid weapon, Goid target );
	void	AreaOfEffectScriptCallback( SFxSID id );

	void	CreateSelectedActorPartyList( GoidColl& selectedparty );
FEX	bool	SelectAllPartyMembers		();
	bool	SelectPlayer				( bool forward );		// $$$ these should be renamed to SelectXxxPartyMember()!! -sb
FEX	bool	SelectNextPlayer			();
FEX	bool	SelectLastPlayer			();
	bool	SelectLeadPlayer			();
	bool	Contains					( Goid go, GoidColl objects );
	bool	Contains					( Goid go, GopColl objects );
	bool	Contains					( Goid go, const GopSet objects );

	// Multiplayer
	void	DrawMultiplayerLabels();

	void	ShowMultiplayerHostOptions();
	void	CloseMultiplayerHostOptions( bool setOptions );

	// Teleportation
	bool	TeleportGameObjectToMouse		();

	// Modify the speed bias on a character
	bool	Selection_SpeedUp();
	bool	Selection_SpeedDown();
	bool	Selection_SpeedReset();

	void	SetResizeLabels( bool bSet )	{ m_bResizeLabels = bSet; }
	bool	GetResizeLabels()				{ return m_bResizeLabels; }


	//********************* Debugging Routines *********************

#	if !GP_RETAIL
	
	bool	IsProbing() const;
	//void	SetDebugObjectProbeMode( const eDEBUG_PROBE_MODE mode )		{ m_Debug_object_probe_mode = mode; }
	//eDEBUG_PROBE_MODE GetDebugObjectProbeMode() const					{ return m_Debug_object_probe_mode; }

	bool	ToggleObjectProbeMode();
	bool	CIR_exp() const;
	void	DisplayCursor( gpstring filename );
	void	SetGUIWarningCount( unsigned int count );
	void	SetGUIErrorCount( unsigned int count );
	void	ResetGUIWarningCount();
	void	ResetGUIErrorCount();
	void	CheckLodPolyCount();
	void	SetPolyAlert( bool bSet ) { m_bPolyAlert = bSet; }
	bool	GetPolyAlert()			  { return m_bPolyAlert; }

#	endif // !GP_RETAIL


	//********************* Object Information Routines ************

	Goid	GetGoUnderCursor		() const;
	Goid	GetActorWhoCarriesObject( Goid object );
	bool	IsGameObjectDraggable	( Goid object );

	// Accessors
	UICommands *			GetUICommands()								{ return m_pUICommands; }
	
	UIPartyManager *		GetUIPartyManager()							{ return m_pUIPartyManager; }

	UIItemOverlay *			GetUIItemOverlay()							{ return m_pUIItemOverlay; }

	UIPlayerRanks *			GetUIPlayerRanks()							{ return m_pUIPlayerRanks; }

	UIEmoteList *			GetUIEmoteList()							{ return m_pUIEmoteList; }
	
	SiegePos				GetLastHitTerrainPos()						{ return m_LastHitTerrainPosition; }
	
	eBackendMode			GetBackendMode()							{ return m_BackendMode;	}
	
	void					SetBackendMode( eBackendMode mode )			{ m_BackendMode = mode; }
	
	SELECTION_MODE			GetSelectionMode()							{ return m_SelectionMode; }
	
	void					SetSelectionMode( SELECTION_MODE mode )		{ m_SelectionMode = mode; }


	Goid ClosestSelectedCharacter( Goid go );

	Goid ClosestSelectedCharacter( SiegePos const & position );


	//********************* GameGUI Callbacks **********************
	
	void GameGUICallback( gpstring const & message, UIWindow & ui_window );



	//////////////////////////////////////////////////////////////
	// dragging

	void SetDragged( Goid object );
	
	void SetDragged( const GoidColl& objects );

	void SetDragged( const GopColl& objects );
	
	Goid GetDragged();

	GoidColl & GetDraggedColl()				{ return m_DraggedObjects; }

	void OrderDraggedRelativeToObject( Goid object );

	bool GetIsAutoInventoryUp()				{ return m_bAutoInventoryUp; }
	void SetAutoInventoryUp( bool bSet )	{ m_bAutoInventoryUp = bSet; }

	void ResetDragTimer();

	//////////////////////////////////////////////////////////////
	// debug

#	if !GP_RETAIL
	bool SuperRecovery();		// $$$ for demos only... remove later
#	endif // !GP_RETAIL

private:

	void PrivateRenderGoSelection( Go* go, siege::database_guid& last_node, DWORD color, bool bFocus );
	

	//********************* Networking Routines ********************

	//********************* End Networking Routines ****************
	
	InputBinder *								m_pGameInputBinder;
	InputBinder *								m_pUserInputBinder;
	InputBinder *								m_pNisInputBinder;
#	if !GP_RETAIL
	UIGame_console_commands *					m_pConsoleCommands;
	std::vector<gpstring>						m_ConsoleHistory;	
#	endif // !GP_RETAIL
	GoidColl									m_Clipboard;
	UICamera *									m_pUICamera;
	UICommands *								m_pUICommands;
	UIPartyManager *							m_pUIPartyManager;
	UIInventoryManager *						m_pUIInventoryManager;
	UIMenuManager *								m_pUIMenuManager;
	UIStoreManager *							m_pUIStoreManager;
	UITeamManager *								m_pUITeamManager;
	UIItemOverlay *								m_pUIItemOverlay;
	UIGameConsole *								m_pUIGameConsole;
	UIPlayerRanks *								m_pUIPlayerRanks;	
	UIEmoteList *								m_pUIEmoteList;	
	
	bool										m_bLastPause;

	// formations
	bool										m_bFormationMode;
	double										m_TimeToDisplayFormation;

	
	// mouse
	bool										m_bSelectionDown;
	bool										m_bSpecialDown;
	bool										m_bContextDown;	
	bool										m_bInvertCameraXAxis;
	bool										m_bInvertCameraYAxis;

	bool										m_bIsVisible;
	Goid										m_LastHitGO;
	SiegePos									m_LastHitTerrainPosition;
	bool										m_bLastHitTerrainPositionValid;
	bool										m_bActive;
	eBackendMode								m_BackendMode;
	gpstring									m_sInfoMode;
	SELECTION_MODE								m_SelectionMode;
	sVertex										m_SelectionVerts[3];
	unsigned int								m_SelectionTextureDefault;
	unsigned int								m_SelectionTextureFocus;
	float										m_DragTimer;
	int											m_DragX;
	int											m_DragY;

	DWORD										m_Item_selector_color;
	DWORD										m_Enemy_selector_color;
	DWORD										m_Friend_selector_color;
	DWORD										m_Party_member_selector_color;
	DWORD										m_Focus_go_selector_color;
	DWORD										m_Dead_enemy_selector_color;
	DWORD										m_Dead_friend_selector_color;

	eCameraMode									m_oldCameraMode;
	bool										m_bEditBoxVisible;
	RolloverObjectData							m_RolloverObjData;
	bool										m_bPlayerRanksVisible;
	float										m_TimeToShowPlayerRanks;
	bool										m_bPlayerLabelsVisible;
	bool										m_bGameTimerVisible;
	bool										m_bOneShotCharacterInit;

	float										m_camHorizontalEdgeBuffer;
	float										m_camVerticalEdgeBuffer;

	GoidColl									m_DraggedObjects;
	GoidColl									m_SelectedItems;
	bool										m_bCameraScreenEdgeRotate;
	bool										m_bCameraKeyRotating;
	bool										m_bScreenHorizTrack;
	bool										m_bScreenVertTrack;	

	Goid										m_AreaOfEffectTarget;
	SFxSID										m_AreaOfEffectScriptID;

	float										m_rolloverTime;
	Goid										m_rolloverGoid;
	bool										m_bAutoInventoryUp;
	int											m_progressNodeCount;

	float										m_DefeatFadeOut;

	eBackendDialog								m_BackendDialog;
	bool										m_BackendDialogJustShown;

	bool										m_bResizeLabels;

	bool										m_bJipLoad;

	bool										m_bEscNisToFrontEnd;		// if true, hitting esc during nis aborts and goes to front end

	// player labels
	UIText *									m_pPlayerLabelText;

	struct PlayerLabel
	{
		eLifeState	m_LifeState;
		float		m_HeightOffset;
	};

	typedef std::map< Goid, PlayerLabel > PlayerLabelMap;

	PlayerLabelMap								m_PlayerLabels;
	float										m_MapShowBorderMeters;
	bool										m_bFadedOut;
	UIWindow *									m_pDefeatBg;	

	// debug
#	if !GP_RETAIL

	LodToPolyMap								m_LodToPolyMap;
	UIText *									m_pPolyText;
	bool										m_bPolyAlert;
	gpstring									m_sLastProbeOutput;
#	endif // !GP_RETAIL
	
	FUBI_SINGLETON_CLASS( UIGame, "Owner of all user interface components for the game." );
	SET_NO_COPYING( UIGame );
};

#define gUIGame UIGame::GetSingleton()


#endif
