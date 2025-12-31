//////////////////////////////////////////////////////////////////////////////
//
// File     :  Preferences.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __PREFERENCES_H
#define __PREFERENCES_H



enum eMoveLockMode
{
	MOVE_LOCK_NONE,
	MOVE_LOCK_X,
	MOVE_LOCK_Y,
	MOVE_LOCK_Z,	
};


class Preferences : public Singleton <Preferences>
{
public:

	Preferences();
	~Preferences()	{};	

	void	SetWireframe	( bool bWireframe	);
	bool	GetWireframe()						{ return m_bWireframe;	}
	void	SetDrawDoors	( bool bDrawDoors	);
	bool	GetDrawDoors()						{ return m_bDrawDoors;	}
	void	SetDrawFloors	( bool bDrawFloors	);
	bool	GetDrawFloors()						{ return m_bDrawFloors; }
	void	SetDrawBoxes	( bool bDrawBoxes	);
	bool	GetDrawBoxes()						{ return m_bDrawBoxes;	}
	void	SetSLights		( bool bSLights		);
	bool	GetSLights()						{ return m_bSLights;	}
	void	SetDLights		( bool bDLights		);
	bool	GetDLights()						{ return m_bDLights;	}
	void	SetSShadows		( bool bSShadows	);
	bool	GetSShadows()						{ return m_bSShadows;	}
	void	SetDShadows		( bool bDShadows	);
	bool	GetDShadows()						{ return m_bDShadows;	}
	void	SetSShading		( bool bSShading	);
	bool	GetSShading()						{ return m_bSShading;	}
	void	SetDShading		( bool bDShading	);
	bool	GetDShading()						{ return m_bDShading;	}
	void	SetPolyStats	( bool bPolystats	);
	bool	GetPolyStats()						{ return m_bPolystats;	}
	void	SetFPS			( bool bFPS			);
	bool	GetFPS()							{ return m_bFPS;		}
	void	SetDrawWater	( bool bDrawWater	);
	bool	GetDrawWater()						{ return m_bDrawWater;	}
	void	SetDrawLNodes	( bool bDrawLNodes	);
	bool	GetDrawLNodes()						{ return m_bDrawLNodes; }
	void	SetDrawNormals	( bool bDrawNormals );
	bool	GetDrawNormals()					{ return m_bDrawNormals; }
	void	SetUpdateWorld	( bool bUpdate		);
	bool	GetUpdateWorld()					{ return m_bUpdateWorld; }
	void	SetUpdateSiegeEngine( bool bUpdate	);
	bool	GetUpdateSiegeEngine()				{ return m_bUpdateSiegeEngine; }

	void	SetUpdateWeather( bool bUpdate )	{ m_bUpdateWeather = bUpdate; }
	bool	GetUpdateWeather()					{ return m_bUpdateWeather; }

	void	SetUpdateSound( bool bUpdate );
	bool	GetUpdateSound()					{ return m_bUpdateSound; }
			
	void	SetUpdateEffects( bool bUpdateEffects )		{ m_bUpdateEffects = bUpdateEffects; };
	bool	GetUpdateEffects()							{ return m_bUpdateEffects; };
	
	void	SetIgnoreVertexColor ( bool bIgnoreVertexColor );
	bool	GetIgnoreVertexColor()				{ return m_bIgnoreVertexColor; };

	void	SetPreferenceDisable ( bool bDisablePref );
	bool	GetPreferenceDisable()				{ return m_bDisablePref; };

	void	SetDrawDoorNumbers( bool bDrawDoor )	{ m_bDrawDoorNums = bDrawDoor;	}
	bool	GetDrawDoorNumbers()					{ return m_bDrawDoorNums;		}

	void	SetDrawStitchDoors( bool bDrawStitch )	{ m_bDrawStitchDoors = bDrawStitch; }
	bool	GetDrawStitchDoors()					{ return m_bDrawStitchDoors; }

	void	SetHitFloorsOnly( bool bHitFloor )		{ m_bHitFloorOnly = bHitFloor; }
	bool	GetHitFloorsOnly()						{ return m_bHitFloorOnly; }

	// Show Preferences
	void	SetShowObjects( bool bShow );
	bool	GetShowObjects()						{ return m_bShowObjects; }

	void	SetShowLights( bool bShow );
	bool	GetShowLights()							{ return m_bShowLights; }

	void	SetShowGizmos( bool bShow );
	bool	GetShowGizmos()							{ return m_bShowGizmos; }

	void	SetShowTuningPoints( bool bShow );
	bool	GetShowTuningPoints()					{ return m_bShowTuningPoints; }
	
	// Mode Preferences

	void	SetModeObjects( bool bMode );
	bool	GetModeObjects()						{ return m_bModeObjects; }

	void	SetModeLights( bool bMode );
	bool	GetModeLights()							{ return m_bModeLights; }

	void	SetModeGizmos( bool bMode );
	bool	GetModeGizmos()							{ return m_bModeGizmos; }

	void	SetModeNodes( bool bMode );
	bool	GetModeNodes()							{ return m_bModeNodes; }

	void	SetModeDecals( bool bMode );
	bool	GetModeDecals()							{ return m_bModeDecals; }

	void	SetModeLogicalNodes( bool bMode );
	bool	GetModeLogicalNodes()					{ return m_bModeLogicalNodes; }

	void	SetModeTuningPoints( bool bMode );
	bool	GetModeTuningPoints()					{ return m_bModeTuningPoints; }

	void	SetModeNodePlacement( bool bMode );
	bool	GetModeNodePlacement()					{ return m_bModeNodePlacement; }
	
	void	SetPaintSelectNodes( bool bSelect );
	bool	GetPaintSelectNodes()					{ return m_bPaintSelectNodes; }

	void	SetSnapToGround( bool bSelect );
	bool	GetSnapToGround()						{ return m_bSnapToGround;		}

	void			SetContentDisplay( eDisplayType type )	{ m_content_display = type;		}
	eDisplayType	GetContentDisplay()						{ return m_content_display;		}

	void	SetPlacementMode( bool bSelect );
	bool	GetPlacementMode()						{ return m_bPlacementMode;		}

	void	SetMovementMode( bool bSelect );
	bool	GetMovementMode()						{ return m_bMovementMode;		}

	void	SetSelectionLock( bool bSelect );
	bool	GetSelectionLock()						{ return m_bSelectionLock;		}

	void	SetPlacementLock( bool bSelect );
	bool	GetPlacementLock()						{ return m_bPlacementLock;		}

	void	SetDrawDirectionalLights( bool bSelect );
	bool	GetDrawDirectionalLights()					{ return m_bDrawDirectional; }

	void	SetShowToolBar( bool bSelect )	{ m_bShowToolBar = bSelect; }
	bool	GetShowToolBar()				{ return m_bShowToolBar; }

	void	SetShowDlgBar( bool bSelect )	{ m_bShowDlgBar = bSelect;	}
	bool	GetShowDlgBar()					{ return m_bShowDlgBar;		}

	void	SetShowViewBar( bool bSelect )	{ m_bShowViewBar = bSelect; }
	bool	GetShowViewBar()				{ return m_bShowViewBar;	}

	void	SetShowNodeBar( bool bSelect )	{ m_bShowNodeBar = bSelect; }
	bool	GetShowNodeBar()				{ return m_bShowNodeBar; }

	void	SetShowObjectBar( bool bSelect ){ m_bShowObjectBar = bSelect; }
	bool	GetShowObjectBar()				{ return m_bShowObjectBar; }

	void	SetShowLightBar( bool bSelect )	{ m_bShowLightBar = bSelect; }
	bool	GetShowLightBar()				{ return m_bShowLightBar;	}

	void	SetShowCameraBar( bool bSelect )	{ m_bShowCameraBar = bSelect; }
	bool	GetShowCameraBar()					{ return m_bShowCameraBar; }

	void	SetShowModeBar( bool bSelect )		{ m_bShowModeBar = bSelect; }
	bool	GetShowModeBar()					{ return m_bShowModeBar; }

	void	SetShowPreferenceBar( bool bSelect ) { m_bShowPreferenceBar = bSelect; }
	bool	GetShowPreferenceBar()				 { return m_bShowPreferenceBar; }

	void	SetZoomRate( float zoomRate )	{ m_zoomRate = zoomRate; }
	float	GetZoomRate()					{ return m_zoomRate; }

	void	SetLinkMode( bool bLink )		{ m_bLinkMode = bLink; }
	bool	GetLinkMode()					{ return m_bLinkMode; }

	// SiegeEditor Load/Save Settings

	void	LoadSettings();
	bool	Load();

	void	SaveSettings();
	void	Save();	

	// Helper Functions
	bool	CanPlace();
	bool	CanSelect();

	bool		GetCustomTabActive()			{ return m_bCustomViewActive; }
	void		SetCustomTabActive( bool bSet );

	void	SetSaveStitch		( bool bStitch )	{ m_bSaveStitch = bStitch; }
	bool	GetSaveStitch		()					{ return m_bSaveStitch; }

	void	SetSaveNodes		( bool bNodes )		{ m_bSaveNodes = bNodes; }
	bool	GetSaveNodes		()					{ return m_bSaveNodes; }

	void	SetSaveLights		( bool bLights )	{ m_bSaveLights = bLights; }
	bool	GetSaveLights		()					{ return m_bSaveLights; }

	void	SetSaveHotpoints	( bool bHotpoints ) { m_bSaveHotpoints = bHotpoints; }
	bool	GetSaveHotpoints	()					{ return m_bSaveHotpoints; }

	void	SetSaveGameObjects	( bool bGameObjects )	{ m_bSaveGameObjects = bGameObjects; }
	bool	GetSaveGameObjects	()						{ return m_bSaveGameObjects; }

	void	SetSaveStartingPos	( bool bStartingPos )	{ m_bSaveStartingPos = bStartingPos; }
	bool	GetSaveStartingPos	()						{ return m_bSaveStartingPos; }

	void	SetRightClickScroll	( bool bRight )		{ m_bRightScroll = bRight; }
	bool	GetRightClickScroll	()					{ return m_bRightScroll; }

	void	SetDisplayHexadecimal( bool bHex )		{ m_bDisplayHex = bHex; }
	bool	GetDisplayHexadecimal()					{ return m_bDisplayHex; }

	void	SetDisplayFourCC( bool bFourCC )		{ m_bDisplayFourCC = bFourCC; }
	bool	GetDisplayFourCC()						{ return m_bDisplayFourCC; }

	void	SetDecalHorizontal( float horiz )		{ m_decalHoriz = horiz; }
	float	GetDecalHorizontal()					{ return m_decalHoriz; }

	void	SetDecalVertical( float vert )			{ m_decalVert = vert; }
	float	GetDecalVertical()						{ return m_decalVert; }

	void	SetObjectRotateMode( bool bRotate );
	bool	GetObjectRotateMode()					{ return m_bRotateMode;		}

	void	SetCameraTranslateStep( float step )	{ m_cameraTranslateStep = step; }
	float	GetCameraTranslateStep()				{ return m_cameraTranslateStep; }

	void	SetCameraRotateStep( float step )		{ m_cameraRotateStep = step; }
	float	GetCameraRotateStep()					{ return m_cameraRotateStep; }

	void	SetCameraZoomStep( float step )			{ m_cameraZoomStep = step; }
	float	GetCameraZoomStep()						{ return m_cameraZoomStep; }

	void	SetDrawCommands( bool bDraw )			{ m_bDrawCommands = bDraw;	}
	bool	GetDrawCommands()						{ return m_bDrawCommands;	}	

	void	SetSelectNoninteractive( bool bSet )	{ m_bSelectNoninteractive = bSet; }
	bool	GetSelectNoninteractive()				{ return m_bSelectNoninteractive; }

	void	SetSelectInteractive( bool bSet )		{ m_bSelectInteractive = bSet; }
	bool	GetSelectInteractive()					{ return m_bSelectInteractive; }

	void	SetSelectGizmos( bool bSet )			{ m_bSelectGizmos = bSet; }
	bool	GetSelectGizmos()						{ return m_bSelectGizmos; }	
	
	void	SetSelectNonGizmos( bool bSet )			{ m_bSelectNonGizmos = bSet; }
	bool	GetSelectNonGizmos()					{ return m_bSelectNonGizmos; }

	void	SetSelectAll( bool bSet )				{ m_bSelectAll = bSet; }
	bool	GetSelectAll()							{ return m_bSelectAll; }

	void			SetMoveLockMode( eMoveLockMode mode );
	eMoveLockMode	GetMoveLockMode()						{ return m_MoveLockMode; }

	void	SetRotationIncrement( float increment )	{ m_rotationIncrement = increment;	}
	float	GetRotationIncrement()					{ return m_rotationIncrement;		}
		
	void	SetCameraSnapIncrement( float increment )	{ m_cameraSnapIncrement = increment; }
	float	GetCameraSnapIncrement()					{ return m_cameraSnapIncrement; }

	void	SetLockCustomView( bool bLock )				{ m_bLockCustomView = bLock; }
	bool	GetLockCustomView()							{ return m_bLockCustomView; }

	void	SetHudSelected( bool bSet )					{ m_bHudSelected = bSet; }
	bool	GetHudSelected()							{ return m_bHudSelected; }

	void	SetDrawLogicalFlags( bool bSet )			{ m_bDrawLogicalFlags = bSet; }
	bool	GetDrawLogicalFlags()						{ return m_bDrawLogicalFlags; }

	void	SetDrawLightHuds( bool bSet )				{ m_bDrawLightHuds = bSet; }
	bool	GetDrawLightHuds()							{ return m_bDrawLightHuds; }

	void	SetDrawDebugHuds( bool bSet );
	bool	GetDrawDebugHuds();

	void			SetLastDebugHudGroup( eDebugHudGroup group )	{ m_lastHudGroup = group; }
	eDebugHudGroup	GetLastDebugHudGroup()							{ return m_lastHudGroup; }	

	void	SetCustomViewSaveName( gpstring sName ) { m_sCustomView = sName; }
	gpstring GetCustomViewSaveName()				{ return m_sCustomView; }

	void	SetPreviewWireframe( bool bSet )	{ m_bPreviewWireframe = bSet; }
	bool	GetPreviewWireframe()				{ return m_bPreviewWireframe; }
		
	void	SetPreviewColor( DWORD dwColor )	{ m_dwPreviewColor = dwColor;	}
	DWORD	GetPreviewColor()					{ return m_dwPreviewColor;		}

	void	SetPreviewWindowPlacement( WINDOWPLACEMENT wp ) { m_PreviewWindowPlacement = wp; }	
	WINDOWPLACEMENT GetPreviewWindowPlacement() { return m_PreviewWindowPlacement; }

	void	SetClearColor( DWORD dwColor );
	DWORD	GetClearColor()						{ return m_dwClearColor; }

	void	SetRestrictCameraToGame( bool bSet )	{ m_bRestrictCameraToGame = bSet; }
	bool	GetRestrictCameraToGame()				{ return m_bRestrictCameraToGame; }

	void	SetDrawCameraTarget( bool bSet )	{ m_bDrawCameraTarget = bSet; }
	bool	GetDrawCameraTarget()				{ return m_bDrawCameraTarget; }

	void		SetResourcePath( gpstring sPath )	{ m_sResourcePath = sPath;	}
	gpstring	GetResourcePath()					{ return m_sResourcePath;	}
	void		SetSelectedResource( gpstring sFile )	{ m_sSelectedResource = sFile; }
	gpstring	GetSelectedResource()					{ return m_sSelectedResource; }

	void		SetEnableGrid( bool bSet )			{ m_bEnableGrid = bSet; }
	bool		GetEnableGrid()						{ return m_bEnableGrid; }

	void		SetGridSize( int size )				{ m_GridSize = size; }
	int			GetGridSize()						{ return m_GridSize; }

	void		SetGridInterval( int interval )		{ m_GridInterval = interval; }
	int			GetGridInterval()					{ return m_GridInterval; }
	
	void		SetGridColor( DWORD dwColor )		{ m_GridColor = dwColor; }
	DWORD		GetGridColor()						{ return m_GridColor; }

	void		SetGridOrigin( vector_3 origin )	{ m_GridOrigin = origin; }
	vector_3	GetGridOrigin()						{ return m_GridOrigin; }

	void		SetGridDrawLabelIntervals( bool bSet )	{ m_bDrawLabelIntervals = bSet; }
	bool		GetGridDrawLabelIntervals()				{ return m_bDrawLabelIntervals; }

	void		SetGridDrawLabelAmount( bool bSet )		{ m_bDrawLabelAmount = bSet; }
	bool		GetGridDrawLabelAmount()				{ return m_bDrawLabelAmount; }

	void		SetGridLabelAmount( int labelInterval )	{ m_DrawLabelAmount = labelInterval; }
	int			GetGridLabelAmount()					{ return m_DrawLabelAmount; }

	void		SetPlaceMeterPoints( bool bSet )		{ m_bPlaceMeterPoints = bSet; }
	bool		GetPlaceMeterPoints()					{ return m_bPlaceMeterPoints; }

	void		SetDrawMeterPoints( bool bSet )			{ m_bDrawMeterPoints = bSet; }
	bool		GetDrawMeterPoints()					{ return m_bDrawMeterPoints; }

	void		SetMeterPointColor( DWORD dwColor )		{ m_MeterPointColor = dwColor; }
	DWORD		GetMeterPointColor()					{ return m_MeterPointColor; }
	
	void		SetShowMeterPointDistance( bool bSet )	{ m_bShowMeterPointDistance = bSet; }
	bool		GetShowMeterPointDistance()				{ return m_bShowMeterPointDistance; }	

	void		SetVisualDrawDestId( bool bSet )		{ m_bVisualDrawDestId = bSet; }
	bool		GetVisualDrawDestId()					{ return m_bVisualDrawDestId; }

	void		SetVisualDrawDoorNumbers( bool bSet )	{ m_bVisualDrawDoorNumbers = bSet; }
	bool		GetVisualDrawDoorNumbers()				{ return m_bVisualDrawDoorNumbers; }

	void		SetVisualDrawMeshBox( bool bSet )		{ m_bVisualDrawMeshBox = bSet; }
	bool		GetVisualDrawMeshBox()					{ return m_bVisualDrawMeshBox; }

	void		SetVisualAllowInvalidMatch( bool bSet )	{ m_bAllowInvalidMatch = bSet; }
	bool		GetVisualAllowInvalidMatch()			{ return m_bAllowInvalidMatch; }

	void		SetVisualSnapSensitivity( float value )	{ m_VisualSnapSensitivity = value;	}
	float		GetVisualSnapSensitivity()				{ return m_VisualSnapSensitivity;	}

	void		SetVisualValidColor( DWORD dwColor )	{ m_VisualValidColor = dwColor;		}
	DWORD		GetVisualValidColor()					{ return m_VisualValidColor;		}
		
	void		SetVisualInvalidColor( DWORD dwColor )	{ m_VisualInvalidColor = dwColor;	}
	DWORD		GetVisualInvalidColor()					{ return m_VisualInvalidColor;		}

	void		SetVisualDestDoorColor( DWORD dwColor )	{ m_VisualDestDoorColor = dwColor;	}
	DWORD		GetVisualDestDoorColor()				{ return m_VisualDestDoorColor;		}

	void		SetVisualRotateOrient( bool bSet )		{ m_bVisualRotateOrient = bSet;		}
	bool		GetVisualRotateOrient()					{ return m_bVisualRotateOrient;		}

	void		SetVisualSnapDelay( float value )		{ m_VisualSnapDelay = value;		}
	float		GetVisualSnapDelay()					{ return m_VisualSnapDelay;			}

	void		SetVisualAllowDoorGuess( bool bSet )	{ m_bVisualAllowDoorGuess = bSet;	}
	bool		GetVisualAllowDoorGuess()				{ return m_bVisualAllowDoorGuess;	}

	void		SetVisualDrawDoors( bool bSet )			{ m_bVisualDrawDoors = bSet;		}
	bool		GetVisualDrawDoors()					{ return m_bVisualDrawDoors;		}
	
private:
	
	bool	m_bWireframe;				// Wireframe on/off
	bool	m_bDrawDoors;				// Draw doors on/off
	bool	m_bDrawFloors;				// Draw floors on/off
	bool	m_bDrawBoxes;				// Draw node boxes on/off
	bool	m_bSLights;					// Show static lighting
	bool	m_bDLights;					// Show dynamic lighting
	bool	m_bSShadows;				// Show static shadows
	bool	m_bDShadows;				// Show dynamic shadows
	bool	m_bSShading;				// Show static shading
	bool	m_bDShading;				// Show dynamic shading
	bool	m_bIgnoreVertexColor;		// Ignore Vertex Color
	bool	m_bDisablePref;				// Preferences check disable
	bool	m_bPolystats;				// Polygon statistics
	bool	m_bUpdateEffects;			// Update lighting, effects
	bool	m_bDrawDoorNums;			// Draw the door numbers
	bool	m_bPlacementMode;			// Placement Mode
	bool	m_bMovementMode;			// Selection Mode
	bool	m_bFPS;						// Show frames per second information
	bool	m_bDrawNormals;
	bool	m_bDrawLNodes;
	bool	m_bDrawWater;
	bool	m_bUpdateWorld;
	bool	m_bUpdateSiegeEngine;
	bool	m_bDrawStitchDoors;
	bool	m_bPlacementLock;
	bool	m_bSelectionLock;
	bool	m_bUpdateWeather;
	bool	m_bUpdateSound;
	bool	m_bHitFloorOnly;
	bool	m_bLinkMode;
	bool	m_bSaveStitch;
	bool	m_bSaveNodes;
	bool	m_bSaveLights;
	bool	m_bSaveHotpoints;
	bool	m_bSaveGameObjects;
	bool	m_bSaveStartingPos;
	bool	m_bRightScroll;
	bool	m_bDisplayHex;
	bool	m_bDisplayFourCC;
	bool	m_bRotateMode;
	bool	m_bDrawCommands;
	bool	m_bSelectNoninteractive;
	bool	m_bSelectInteractive;
	bool	m_bSelectGizmos;
	bool	m_bSelectNonGizmos;
	bool	m_bSelectAll;	

	// View Preferences ( not located on the Preferences Dialog box )
	bool	m_bShowLights;
	bool	m_bShowGizmos;
	bool	m_bShowObjects;
	bool	m_bShowTuningPoints;
	
	bool	m_bShowToolBar;
	bool	m_bShowDlgBar;
	bool	m_bShowViewBar;
	bool	m_bShowNodeBar;
	bool	m_bShowObjectBar;
	bool	m_bShowLightBar;
	bool	m_bShowCameraBar;
	bool	m_bShowModeBar;	
	bool	m_bShowPreferenceBar;

	// Mode Preferences 
	bool	m_bModeLights;
	bool	m_bModeGizmos;
	bool	m_bModeObjects;
	bool	m_bModeNodes;
	bool	m_bModeDecals;
	bool	m_bModeLogicalNodes;
	bool	m_bModeTuningPoints;
	bool	m_bModeNodePlacement;
		
	// Misc
	bool	m_bPaintSelectNodes;
	bool	m_bSnapToGround;
	bool	m_bDrawDirectional;
	float	m_zoomRate;
	float	m_rotationIncrement;
	float	m_cameraSnapIncrement;
	bool	m_bLockCustomView;
	bool	m_bHudSelected;	
	bool	m_bCustomViewActive;	
	float	m_decalHoriz;
	float	m_decalVert;
	float	m_cameraTranslateStep;
	float	m_cameraRotateStep;
	float	m_cameraZoomStep;
	bool	m_bDrawLogicalFlags;
	bool	m_bDrawLightHuds;
	bool	m_bDrawCameraTarget;
	bool	m_bRestrictCameraToGame;
	bool	m_bPlaceMeterPoints;
	bool	m_bDrawMeterPoints;
	DWORD	m_MeterPointColor;
	bool	m_bShowMeterPointDistance;

	// Visual Node Placement
	bool	m_bVisualDrawDestId;
	bool	m_bVisualDrawDoorNumbers;
	bool	m_bVisualDrawMeshBox;
	bool	m_bAllowInvalidMatch;
	float	m_VisualSnapSensitivity;
	DWORD	m_VisualValidColor;
	DWORD	m_VisualInvalidColor;
	DWORD	m_VisualDestDoorColor;
	bool	m_bVisualRotateOrient;
	float	m_VisualSnapDelay;
	bool	m_bVisualDrawDoors;
	bool	m_bVisualAllowDoorGuess;

	bool		m_bEnableGrid;
	int			m_GridSize;
	int			m_GridInterval;
	DWORD		m_GridColor;
	vector_3	m_GridOrigin;	
	bool		m_bDrawLabelIntervals;
	bool		m_bDrawLabelAmount;
	int			m_DrawLabelAmount;

	eDisplayType m_content_display;
	eMoveLockMode m_MoveLockMode;
	gpstring	 m_sCustomView;
	eDebugHudGroup m_lastHudGroup;

	gpstring	 m_sResourcePath;
	gpstring	 m_sSelectedResource;

	// Mesh Previewer Preferences
	bool	m_bPreviewWireframe;	
	DWORD	m_dwPreviewColor;
	DWORD	m_dwClearColor;
	WINDOWPLACEMENT m_PreviewWindowPlacement;
};


#define gPreferences Preferences::GetSingleton()


#endif