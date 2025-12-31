//////////////////////////////////////////////////////////////////////////////
//
// File     :  GUIMasterShell.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __GUIMASTERSHELL_H
#define __GUIMASTERSHELL_H


// Forward Declarations
class UIShell;
class FuelSys;
class RapiOwner;
class Rapi;
class NamingKey;
class MenuCommands;
class LocMgr;

namespace FileSys {
	class MasterFileMgr;
	class PathFileMgr;
};


// Include Files
#include "gpstring.h"
#include "vector_3.h"
#include "GUIMasterTypes.h"
#include "ui_types.h"
#include <set>


class GUIMasterShell : public Singleton <GUIMasterShell>
{
public:

	GUIMasterShell();
	~GUIMasterShell();

	void Init();
	void Draw();
	void Update( double seconds );

	// Window Information

		HWND GetHWnd()					{ return m_hWnd; }
		void SetHWnd( HWND hWnd )		{ m_hWnd = hWnd; }
		HWND GetRenderHWnd()			{ return m_hWndRender; }
		void SetRenderHWnd( HWND hWnd ) { m_hWndRender = hWnd; }

	// Interface Information
		gpstring	GetInterface()			{ return m_sInterface; }
		void		SetInterface( gpstring sInterface ) { m_sInterface = sInterface; }
		void		GetAvailableInterfaces( StringVec & interfaces );
		void		CreateInterface( gpstring sInterface );
		void		LoadInterface( gpstring sInterface );
		void		GetActiveInterfaces( StringVec & interfaces );
		void		CloseInterface( gpstring sInterface );
		void		ShowInterface( gpstring sInterface, bool bShow );
	
	// Input Handling
		
		void OnLButtonDown();
		void OnLButtonUp();
		void OnMouseMove();
		void SetLButtonDown( bool bDown ) { m_mouseData.bLButtonDown = bDown; }

	// Rendering
		
		DWORD GetCursorColor() { return m_dwCursorColor; }
		void  SetCursorColor( DWORD dwColor ) { m_dwCursorColor = dwColor; }
		DWORD & GetClearColor() { return m_dwColor; }
		void  SetClearColor( DWORD color ) { m_dwColor = color; }

		void DrawRect( RECT rect, DWORD dwColor );
		void DrawSelectionRect();	
		void DrawCursor( int x, int y );
		void DrawGUIWindow( UIRect rect, UINormalizedRect uvrect, float alpha );
		void SetHideBlankWindows( bool bHide )	{ m_bHideBlank = bHide; }
		bool GetHideBlankWindows()				{ return m_bHideBlank; }

	// Modal Information
		MODE_TYPE	GetCurrentMode() { return m_currentMode; }
		void		SetCurrentMode( MODE_TYPE mode ) { m_currentMode = mode; }

		STRUCTURE_TYPE	GetStructureMode() { return m_structureMode; }
		void			SetStructureMode( STRUCTURE_TYPE type ) { m_structureMode = type; }

	// Creation
		gpstring	GetControlType()					{ return m_sControlType; }
		void		SetControlType( gpstring sType )	{ m_sControlType = sType; }
		gpstring	GetUniqueName( gpstring sName = "" );
		bool		DoesNameExist( gpstring sName );

	// Structure
		bool		HitDetect( int x, int y, bool bHighlight = false );
		void		CalculateStructureState( UIRect rect, int x, int y );
		void		CalculateStructureCursor( UIRect rect, int x, int y );
		bool		InRange( int p1, int p2, int range );
		void		Move();	
		void		Move( eDirection dir );
		void		Resize( int x, int y, bool bUVStructure = false );
		void		ResizeRectToTexture( UIRect & rect, unsigned int texture ); 
		void		ResizeUVRectToTexture( UINormalizedRect & uvrect, UIRect & rect, unsigned int texture );
		void		BringSelectedWindowsToFront();
		UIWindow *	GetLeadWindow() { return m_pLeadWindow; }
		void		SetLeadWindow( UIWindow * pWindow ) { m_pLeadWindow = pWindow; }
		bool		GetSnapToGuides() { return m_bSnapToGuides; }
		void		SetSnapToGuides( bool bSnap ) { m_bSnapToGuides = bSnap; }
		bool		GetParentMove() { return m_bParentMove; }
		void		SetParentMove( bool bMove ) { m_bParentMove = bMove; }
		bool		GetGroupMove() { return m_bGroupMove; }
		void		SetGroupMove( bool bMove ) { m_bGroupMove = bMove; }
		bool		GetDockGroupMove() { return m_bDockGroupMove; }
		void		SetDockGroupMove( bool bMove ) { m_bDockGroupMove = bMove; }
		bool		GetRadioGroupMove() { return m_bRadioGroupMove; }
		void		SetRadioGroupMove( bool bMove ) { m_bRadioGroupMove = bMove; }
		void		DragSelect();
		void		RetreiveWindowsToMove( std::set< UIWindow * > & window_set );		
		void		DeleteSelectedWindows();
		bool		IsWindowSelected( UIWindow * pWindow );
		void		GetPossibleParents( UIWindow * pWindow, UIWindowVec & windows );
		void		Copy();
		void		Cut();
		void		Paste();
		void		SetResizeMode( bool bSet )	{ m_bResizeMode = bSet; }
		bool		GetResizeMode()				{ return m_bResizeMode; }
		void		SetMoveMode( bool bSet )	{ m_bMoveMode = bSet; }
		bool		GetMoveMode()				{ return m_bMoveMode; }

	// UVStructure
		void		MoveUV();
		void		ResizeUV( int x, int y );
		void		CalculateUVState( UIRect rect, int x, int y );
		UIRect		GetUXVYRect( UIWindow * pWindow );
		void		DrawUVEdge( UIRect rect );


	// Persistence
		gpstring	GetTextureNamingKeyName( gpstring sFilename );
		void		Save();
		void		CreateGuiIdMap();


	// Guides
			
		void DrawLine( int Coord, REF_AXIS rAxis, DWORD color );
		void DeleteReferenceLine();		
		void DrawRefLines();		
		void SetReferenceLines( bool bShow ) { m_bShowRef = bShow; }		
		bool GetReferenceLines() { return m_bShowRef; }
		void CreateReferenceLine( REF_AXIS axis );
		void MoveReferenceLine( int x, int y );
		void FindReferenceLine( int x, int y );


	// Background

		void SetBackgroundTexture( unsigned int texture ) { m_textureBg = texture; }
		unsigned int GetBackgroundTexture() { return m_textureBg; }
		void DrawBackgroundTexture();

private:

	FileSys::MasterFileMgr	*	m_pMasterFileMgr;
	FileSys::PathFileMgr	*	m_pPathFileMgr;
	NamingKey				*	m_pNamingKey;
	UIShell					*	m_pGameGUI;	
	FuelSys					*	m_pFuelSys;
	RapiOwner				*	m_pRapiOwner;
	Rapi					*	m_pRapi;
	MenuCommands			*	m_pMenuCommands;
	LocMgr					*	m_pLocMgr;
	gpstring					m_sDir;
	HWND						m_hWndRender;
	DWORD						m_dwColor;
	gpstring					m_sControlType;
	MouseData					m_mouseData;
	DWORD						m_dwCursorColor;
	MODE_TYPE					m_currentMode;
	HWND						m_hWnd;
	STRUCTURE_TYPE				m_structureMode;
	UIWindowVec					m_selectedWindows;
	UIWindow *					m_pLeadWindow;
	gpstring					m_sInterface;
	bool						m_bSnapToGuides;
	bool						m_bParentMove;
	bool						m_bGroupMove;
	bool						m_bDockGroupMove;
	bool						m_bRadioGroupMove;
	bool						m_bHideBlank;
	int							m_screenWidth;
	int							m_screenHeight;
	RefLineVec					m_RefLines;
	bool						m_bShowRef;
	int							m_SelectedLine;
	UV_STATE					m_UVState;
	int							m_draw_order;
	UIWindowVec					m_clipboard;
	unsigned int				m_textureBg;

	bool						m_bResizeMode;
	bool						m_bMoveMode;
};


#define gGUIMaster GUIMasterShell::GetSingleton()


#endif