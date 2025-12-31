//////////////////////////////////////////////////////////////////////////////
//
// File     :  SiegeEditorShell.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __SIEGEEDITORSHELL_H
#define __SIEGEEDITORSHELL_H



// Include Files
#include "PrecompEditor.h"
#include "RapiOwner.h"
#include "RapiAppModule.h"

// Class Forward Declarations
class EditorTerrain;
class EditorObjects;
class EditorLights;
class EditorRegion;
class EditorCamera;
class EditorTriggers;
class EditorGenerators;
class EditorMap;
class LogicalNodeCreator;
class WorldIndexBuilder;
class Stitch_Helper;
class ComponentList;
class Preferences;
class CSiegeEditorDoc;
class KeyMapper;
class CustomViewer;
class MeshPreviewer;
class GizmoManager;
class SCIDManager;
class EditorLights;
class World;
class Services;
class GpConsole;
class gpprofiler;
class GameState;
class CommandManager;
class EditorHotpoints;
class EditorGizmos;
class EditorTuning;
class EditorSfx;


namespace ReportSys {
	class EditCtlSink;
}


// This is the main control class for the editor; there can be only one!
class SiegeEditorShell : public RapiAppModule, public Singleton <SiegeEditorShell>
{
public:

	SiegeEditorShell();
	virtual ~SiegeEditorShell();

	// Window Information
	HWND GetHWnd()					{ return GetMainWnd(); }
	void SetMainHWnd( HWND hWnd )	{ m_hWndMain = hWnd; }
	HWND GetMainHWnd()				{ return m_hWndMain; }

	// Access
	WorldIndexBuilder *			GetWorldIndexBuilder()	{ return m_pWorldIndexBuilder;	}
	Stitch_Helper *				GetStitchHelper()		{ return m_pStitchHelper;		}
	LogicalNodeCreator *		GetLogicalNodeCreator()	{ return m_pLogicalNodeCreator; }
	CImageList *				GetImageList()			{ return m_pImageList;			}
	void						SetImageList( CImageList * pImageList );

	void						SetDocument( CSiegeEditorDoc * pDoc ) { m_pDocument = pDoc; }
	CSiegeEditorDoc *			GetDocument()			{ return m_pDocument;			}

	void						SetStatusBar( CStatusBar * pBar ) { m_pStatusBar = pBar; }
	CStatusBar *				GetStatusBar()			{ return m_pStatusBar; }

	// Accelerator
	void SetAccelerator( HACCEL hAccel );
	HACCEL & GetAccelerator() { return m_hAccel; }

	void GetBuildTimeString( gpstring & sBuild );

	virtual bool OnFrame              ( float deltaTime, float actualDeltaTime );

	void SetShuttingDown( bool bSet )	{ m_bShutdown = bSet; }
	bool IsShuttingDown()				{ return m_bShutdown; }

	void DrawMainFocus();

private:

	virtual bool OnPaint( PAINTSTRUCT& ps );
	virtual bool OnSize ( const GSize& newSize );

	// pipeline
	virtual bool OnInitCore    ( void );
	virtual bool OnInitWindow  ( void );
	virtual bool OnInitServices( void );
	virtual bool OnShutdown    ( void );

	// special pipeline
	virtual void OnRestoreSurfaces ( bool force );

	// execution
	virtual bool OnCheckFocus         ( void );
	
	virtual bool OnSysFrame           ( float deltaTime, float actualDeltaTime );

	// utility
	void InitShell();
	void ConstructImageList();
	void ShutdownShell();

	bool							m_bShutdown;

	// Editor Component Pointers
	my EditorTerrain *				m_pEditorTerrain;
	my EditorCamera *				m_pEditorCamera;
	my EditorRegion *				m_pEditorRegion;
	my EditorMap *					m_pEditorMap;
	my EditorObjects *				m_pEditorObjects;
	my EditorLights *				m_pEditorLights;
	my EditorTriggers *				m_pEditorTriggers;
	my EditorHotpoints *			m_pEditorHotpoints;
	my EditorGizmos *				m_pEditorGizmo;
	my EditorTuning *				m_pEditorTuning;
	my EditorSfx *					m_pEditorSfx;

	HWND							m_hWndMain;
	CSiegeEditorDoc	*				m_pDocument;
	CStatusBar *					m_pStatusBar;
	HACCEL							m_hAccel;
	gpstring						m_LastProfileOutput;		// misc storage $$$ switch to a context as a buffer, move gpprofiler into AppModule

	// Editor Sub-Component Pointers
	my WorldIndexBuilder *			m_pWorldIndexBuilder;		// Handles all world index io
	my LogicalNodeCreator *			m_pLogicalNodeCreator;		// Creates and maintains logical nodes
	my Stitch_Helper *				m_pStitchHelper;
	my CImageList *					m_pImageList;
	my ComponentList *				m_pComponentList;
	my Preferences *				m_pPreferences;
	my KeyMapper *					m_pKeyMapper;
	my CustomViewer *				m_pCustomViewer;
	my MeshPreviewer *				m_pPreviewer;
	my GizmoManager *				m_pGizmoManager;
	my SCIDManager *				m_pSCIDManager;
	my ReportSys::EditCtlSink *		m_pOutputSink;
	my CommandManager *				m_pCommandManager;

	// core
	my gpprofiler*					m_GpProfiler;

	// window
	my RapiOwner*					m_RapiOwner;

	// services
	my GpConsole*					m_GpConsole;
	my Services*					m_Services;
	my World*						m_World;
	my GameState*					m_GameState;

};

#define gEditor Singleton <SiegeEditorShell>::GetSingleton()

#endif
