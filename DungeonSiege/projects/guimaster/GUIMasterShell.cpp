//////////////////////////////////////////////////////////////////////////////
//
// File     :  GUIMaster.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "MULTIMON.H"
#include "GUIMasterShell.h"
#include "IgnoredWarnings.h"
#include "RapiOwner.h"
#include "Fuel.h"
#include "FuelDB.h"
#include "stringtool.h"
#include "NamingKey.h"
#include "ui_shell.h"
#include "filesys.h"
#include "filesysutils.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "gpcore.h"
#include "menu.h"
#include "winx.h"
#include "MainFrm.h"
#include "LocHelp.h"


using namespace FileSys;


GUIMasterShell::GUIMasterShell()
{
	m_pMasterFileMgr	= NULL;
	m_pPathFileMgr		= NULL;
	m_pFuelSys			= NULL;
	m_pRapiOwner		= NULL;
	m_pRapi				= NULL;
	m_hWndRender		= NULL;
	m_pNamingKey		= NULL;
	m_pMenuCommands		= NULL;
	m_pLeadWindow		= NULL;	
	m_pLocMgr			= NULL;
	m_dwColor			= 0xFF000000;
	m_textureBg			= 0;

	m_mouseData.x1		= 0;
	m_mouseData.y1		= 0;
	m_mouseData.x2		= 0;
	m_mouseData.y2		= 0;
	m_mouseData.bLButtonDown	= false;
	m_mouseData.bDragging		= false;
	
	m_bSnapToGuides		= false;
	m_bParentMove		= false;
	m_bGroupMove		= false;
	m_bDockGroupMove	= false;
	m_bRadioGroupMove	= false;
	m_bShowRef			= true;
	m_bHideBlank		= false;
	m_bResizeMode		= true;
	m_bMoveMode			= true;

	m_screenWidth		= 800;
	m_screenHeight		= 600;

	m_dwCursorColor		= 0xFF0055FF;
	m_structureMode		= S_NONE;
	m_currentMode		= MODE_CREATE;
	m_draw_order		= 0;

	m_SelectedLine		= -1;

}


GUIMasterShell::~GUIMasterShell()
{		
	delete m_pRapiOwner;
	delete m_pNamingKey;	
	delete m_pMenuCommands;
	delete m_pFuelSys;
	delete m_pMasterFileMgr;
	delete m_pLocMgr;			
}


void GUIMasterShell::Init()
{
	// Get the current working directory and initialize the file manager for fuel
	char szDirectory[1024];
	GetCurrentDirectory( sizeof( szDirectory ), szDirectory );

	m_pMasterFileMgr	= new MasterFileMgr();
	m_pPathFileMgr		= new PathFileMgr( szDirectory );
	m_pMasterFileMgr->AddFileMgr( m_pPathFileMgr, true );

	// Create our fuel database
	m_pFuelSys = new FuelSys;
	BinFuel::SetDictionaryPath( "" );
	FuelDB * pFuelDB = m_pFuelSys->AddTextDb( "default" );
	FuelDB * pBinDB = m_pFuelSys->AddBinaryDb( "default_binary" );
	pFuelDB->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ | FuelDB::OPTION_WRITE, szDirectory, szDirectory );	
	pBinDB->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ | FuelDB::OPTION_WRITE, szDirectory, szDirectory );

	// Set the tank root directory
	m_sDir.assign( szDirectory );
	gpstring sDir = m_sDir;
	stringtool::AppendTrailingBackslash( sDir );
	//Tank::GSetRootDirectory( sDir );

	// Create Rapi owner
	m_pRapiOwner = new RapiOwner();

	// Set up the default video card
	m_pRapiOwner->Initialize();	
	RECT rect;
	GetClientRect( GetRenderHWnd(), &rect );
	m_pRapiOwner->PrepareToDraw( GetRenderHWnd(), 0, false, rect.right, rect.bottom, 16 );		
	m_pRapi = m_pRapiOwner->GetDevice( 0 );

	// Create the naming key
	m_pNamingKey = new NamingKey( "art\\NamingKey.NNK" );

	m_pLocMgr = new LocMgr();

	// Create the GameGUI
	m_pGameGUI = new UIShell( *m_pRapi, 640, 480 );

	m_pMenuCommands = new MenuCommands();	
}


void GUIMasterShell::Draw()
{
	m_pRapi->Begin3D();

		// Set the clear color
		m_pRapi->SetClearColor( m_dwColor );

		DrawBackgroundTexture();

		// Draw the GUI elements
		{
			GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
			GUInterfaceVec::iterator j;
			UIWindowDrawMap::iterator i;
			for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
				if ( (*j)->bVisible == true ) {
					for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i ) {

						if (( (*i).second->HasTexture() == false ) && ( !GetHideBlankWindows() ) && ( !(*i).second->GetIsCommonControl() ))
						{
							DrawGUIWindow( (*i).second->GetRect(), (*i).second->GetUVRect(), (*i).second->GetAlpha() );
						}

						(*i).second->Draw();						

						if ( IsWindowSelected( (*i).second ) )
						{
							DrawRect( (*i).second->GetRect(), 0xFFFF00FF );

							if ( GetCurrentMode() == MODE_UVEDIT )
							{
								DrawUVEdge( GetUXVYRect( (*i).second ) );
							}
						}
					}
				}
			}
		}
		
		POINT pt;
		GetCursorPos( &pt );
		ScreenToClient( m_hWndRender, &pt );
		DrawCursor( pt.x, pt.y );

		// Draw the selection indicator
		if ( m_mouseData.bDragging )
		{
			DrawSelectionRect();
		}

		if ( GetReferenceLines() )
		{
			DrawRefLines();
		}

	m_pRapi->End3D();
}


void GUIMasterShell::DrawBackgroundTexture()
{
	if ( GetBackgroundTexture() )
	{
		RECT rect;
		GetClientRect( m_hWndRender, &rect );

		gUIShell.GetRenderer().SetTextureStageState(	0,
														GetBackgroundTexture() != 0 ? D3DTOP_MODULATE : D3DTOP_DISABLE,
														D3DTOP_MODULATE,
														D3DTADDRESS_CLAMP,	// WRAP
														D3DTADDRESS_CLAMP,
														D3DTFG_POINT,		// LINEAR
														D3DTFN_POINT,
														D3DTFP_POINT,
														D3DBLEND_SRCALPHA,
														D3DBLEND_INVSRCALPHA,
														true );

		tVertex Verts[4];
		memset( Verts, 0, sizeof( tVertex ) * 4 );

		vector_3 default_color( 1.0f, 1.0f, 1.0f );
		DWORD color	= MAKEDWORDCOLOR( default_color );
		color		&= 0xFFFFFFFF;

		Verts[0].x			= rect.left-0.5f;
		Verts[0].y			= rect.top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].rhw		= 1.0f;
		Verts[0].uv.u		= (float)0.0f;
		Verts[0].uv.v		= (float)1.0f;
		Verts[0].color		= color;

		Verts[1].x			= rect.left-0.5f;
		Verts[1].y			= rect.bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].rhw		= 1.0f;
		Verts[1].uv.u		= (float)0.0f;
		Verts[1].uv.v		= (float)0.0f;
		Verts[1].color		= color;

		Verts[2].x			= rect.right-0.5f;
		Verts[2].y			= rect.top-0.5f;
		Verts[2].z			= 0.0f;
		Verts[2].rhw		= 1.0f;
		Verts[2].uv.u		= (float)1.0f;
		Verts[2].uv.v		= (float)1.0f;
		Verts[2].color		= color;

		Verts[3].x			= rect.right-0.5f;
		Verts[3].y			= rect.bottom-0.5f;
		Verts[3].z			= 0.0f;
		Verts[3].rhw		= 1.0f;
		Verts[3].uv.u		= (float)1.0f;
		Verts[3].uv.v		= (float)0.0f;
		Verts[3].color		= color;

		gUIShell.GetRenderer().DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, &m_textureBg, 1 );
	}
}


void GUIMasterShell::Update( double seconds )
{
	OnMouseMove();		
	m_pGameGUI->Update( seconds, 0, 0 );
}


void GUIMasterShell::OnLButtonDown()
{
	POINT pt;
	GetCursorPos( &pt );
	ScreenToClient( m_hWndRender, &pt );
	
	bool bHit = HitDetect( pt.x, pt.y, false );
	
	m_mouseData.x1 = pt.x;
	m_mouseData.y1 = pt.y;
	m_mouseData.bLButtonDown = true;

	bool bSelected = false;
	if ( m_pLeadWindow )
	{
		CalculateStructureState( m_pLeadWindow->GetRect(), pt.x, pt.y );
		CalculateUVState( GetUXVYRect( m_pLeadWindow ), pt.x, pt.y );

		UIWindowVec::iterator i;		
		for ( i = m_selectedWindows.begin(); i != m_selectedWindows.end(); ++i )
		{
			if ( (*i) == m_pLeadWindow )
			{
				bSelected = true;
			}			
		}		
	}
	else
	{
		m_UVState = UV_NONE;
		m_structureMode = S_NONE;
	}
	
	switch ( GetCurrentMode() )
	{
	case MODE_STRUCTURE:
		{	
			if ( bHit )
			{
				if ( m_structureMode == S_MOVE )
				{
					Move();
				}
				else
				{
					Resize( pt.x, pt.y, false );
					bHit = true;
				}
			}
		}
		break;
	case MODE_UVEDIT:	
		{
			if ( bHit )
			{
				if ( m_UVState == UV_MOVE )
				{
					MoveUV();
				}
				else
				{
					ResizeUV( pt.x, pt.y );
					bHit = true;
				}
			}
		}
		break;
	case MODE_GUIDEEDIT:
		{
			FindReferenceLine( pt.x, pt.y );
		}
		break;
	case MODE_UVSTRUCTURE:
		{
			if ( bHit )
			{
				if ( m_structureMode == S_MOVE )
				{
					Move();
				}
				else
				{
					Resize( pt.x, pt.y, true );
					bHit = true;
				}
			}
		}
		break;		
	}

	if (( bHit == false ) && ( m_structureMode == S_NONE ))
	{
		m_mouseData.bDragging = true;
		m_selectedWindows.clear();
	}
	else if ( GetCurrentMode() == MODE_CREATE )
	{
		m_mouseData.bDragging = true;
		m_selectedWindows.clear();
	}

	if ( bHit == false )
	{
		m_selectedWindows.clear();
	}
}


void GUIMasterShell::OnLButtonUp()
{	
	POINT pt;
	GetCursorPos( &pt );
	ScreenToClient( m_hWndRender, &pt );
	m_mouseData.x2 = pt.x;
	m_mouseData.y2 = pt.y;

	HitDetect( pt.x, pt.y, false );
	
	m_structureMode = S_NONE;
	m_UVState = UV_NONE;
	switch ( GetCurrentMode() )
	{
	case MODE_CREATE:
		{
			if ( m_mouseData.bDragging ) 
			{
				UIWindow * pWindow = 0;
				if ( m_sControlType.empty() )
				{
					pWindow = gUIShell.CreateDefaultWindowOfType( "window" );
				}
				else 
				{
					pWindow = gUIShell.CreateDefaultWindowOfType( m_sControlType );
				}

				if ( pWindow )
				{
					int x1 = 0;
					int x2 = 0;
					int y1 = 0;
					int y2 = 0;
					// Calibrate the mouse position
					if ( m_mouseData.x1 > m_mouseData.x2 )
					{
						x1 = m_mouseData.x2;
						x2 = m_mouseData.x1;
					}
					else
					{
						x1 = m_mouseData.x1;
						x2 = m_mouseData.x2;
					}
					if ( m_mouseData.y1 > m_mouseData.y2 )
					{
						y1 = m_mouseData.y2;
						y2 = m_mouseData.y1;
					}
					else 
					{
						y1 = m_mouseData.y1;
						y2 = m_mouseData.y2;
					}

					pWindow->SetRect( x1, x2, y1, y2 );
					pWindow->SetUVRect( 0, 1, 0, 1 );
					pWindow->SetName( GetUniqueName() );
					pWindow->SetDrawOrder( ++m_draw_order );
					gUIShell.AddWindowToInterface( pWindow, m_sInterface );						
				}
			}
		}
		break;
	case MODE_STRUCTURE:
		{
			HitDetect( pt.x, pt.y, true );
		}
		break;
	case MODE_UVEDIT:	
		{
			HitDetect( pt.x, pt.y, true );
		}
		break;
	case MODE_GUIDEEDIT:
		{			
		}
		break;
	case MODE_UVSTRUCTURE:
		{
			HitDetect( pt.x, pt.y, true );
		}
		break;		
	}

	if (( GetCurrentMode() != MODE_CREATE ) && ( m_mouseData.bDragging ))
	{
		DragSelect();
	}

	m_mouseData.bLButtonDown	= false;
	m_mouseData.bDragging		= false;
}


void GUIMasterShell::OnMouseMove()
{
	POINT pt;
	GetCursorPos( &pt );
	ScreenToClient( m_hWndRender, &pt );

	GRect rect = winx::GetClientRect( GetRenderHWnd() );

	if ( pt.x < 0 )
	{
		pt.x = 0;
	}
	else if ( pt.x > rect.right )
	{
		pt.x = rect.right;
	}
	if ( pt.y < 0 )
	{
		pt.y = 0;
	}
	else if ( pt.y > rect.bottom )
	{
		pt.y = rect.bottom;
	}

	m_mouseData.x2 = pt.x;
	m_mouseData.y2 = pt.y;
	gpstring sPos;
	sPos.assignf( "%d, %d", pt.x, pt.y );

	gMainFrm.GetStatusBar().SetWindowText( sPos );

	switch ( GetCurrentMode() )
	{
	case MODE_CREATE:
		{
		}
		break;
	case MODE_STRUCTURE:
		{
			if ( m_pLeadWindow )
			{
				CalculateStructureCursor( m_pLeadWindow->GetRect(), pt.x, pt.y );
			}
			
			if ( m_structureMode == S_MOVE )
			{
				Move();
			}
			else
			{
				Resize( pt.x, pt.y );		
			}		
		}
		break;
	case MODE_UVEDIT:	
		{			
			if ( m_UVState == UV_MOVE )
			{
				MoveUV();
			}
			else
			{
				ResizeUV( pt.x, pt.y );		
			}
		}
		break;
	case MODE_GUIDEEDIT:
		{
			if ( m_mouseData.bLButtonDown )
			{
				MoveReferenceLine( pt.x, pt.y );
			}
		}
		break;
	case MODE_UVSTRUCTURE:
		{
			if ( m_pLeadWindow )
			{
				CalculateStructureCursor( m_pLeadWindow->GetRect(), pt.x, pt.y );
			}
			
			if ( m_structureMode == S_MOVE )
			{
				Move();
			}
			else
			{
				Resize( pt.x, pt.y, true );		
			}	
		}
		break;		
	}
}


bool GUIMasterShell::HitDetect( int x, int y, bool bHighlight )
{
	bool bHit = false;
	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::reverse_iterator j;
	UIWindowDrawMap::reverse_iterator i;
	for ( j = interfaces.rbegin(); j != interfaces.rend(); ++j ) {
		if (( (*j)->bVisible == true ) && ( (*j)->name.same_no_case( GetInterface() ) )) {
			for ( i = (*j)->ui_draw_windows.rbegin(); i != (*j)->ui_draw_windows.rend(); ++i ) {
				bHit = (*i).second->HitDetect( x, y );								
				if ( bHit )
				{
					SetLeadWindow( (*i).second );														

					if ( bHighlight ) 
					{						
						m_selectedWindows.clear();
						m_selectedWindows.push_back( (*i).second );						
					}
					return bHit; 
				}
			}
		}
	}

	return bHit;
}


void GUIMasterShell::DragSelect()
{
	int x1 = 0;
	int x2 = 0;
	int y1 = 0;
	int y2 = 0;
	
	// Calibrate the mouse position
	if ( m_mouseData.x1 > m_mouseData.x2 )
	{
		x1 = m_mouseData.x2;
		x2 = m_mouseData.x1;
	}
	else
	{
		x1 = m_mouseData.x1;
		x2 = m_mouseData.x2;
	}
	if ( m_mouseData.y1 > m_mouseData.y2 )
	{
		y1 = m_mouseData.y2;
		y2 = m_mouseData.y1;
	}
	else 
	{
		y1 = m_mouseData.y1;
		y2 = m_mouseData.y2;
	}

	m_selectedWindows.clear();
	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::reverse_iterator j;
	UIWindowDrawMap::reverse_iterator i;
	for ( j = interfaces.rbegin(); j != interfaces.rend(); ++j ) {
		if (( (*j)->bVisible == true ) && ( (*j)->name.same_no_case( GetInterface() ) )) {
			for ( i = (*j)->ui_draw_windows.rbegin(); i != (*j)->ui_draw_windows.rend(); ++i ) {				
				UIWindow * pWindow = (*i).second;
				if (( pWindow->GetRect().left >= x1 ) &&
					( pWindow->GetRect().right <= x2 ) &&
					( pWindow->GetRect().top >= y1 ) &&
					( pWindow->GetRect().bottom <= y2 ))
				{
					m_selectedWindows.push_back( pWindow );						
				}
			}
		}
	}
}


void GUIMasterShell::DrawRect( RECT rect, DWORD dwColor )
{
	tVertex Verts[5];
	memset( Verts, 0, sizeof( sVertex ) * 5 );

	Verts[0].x			= rect.left-0.5f+1;
	Verts[0].y			= rect.top-0.5f+1;
	Verts[0].z			= 0.0f;
	Verts[0].color		= dwColor;
	Verts[0].rhw		= 1.0f;

	Verts[1].x			= rect.left-0.5f+1;
	Verts[1].y			= rect.bottom-0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].color		= dwColor;
	Verts[1].rhw		= 1.0f;

	Verts[2].x			= rect.right-0.5f;
	Verts[2].y			= rect.bottom-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].color		= dwColor;
	Verts[2].rhw		= 1.0f;

	Verts[3].x			= rect.right-0.5f;
	Verts[3].y			= rect.top-0.5f+1;
	Verts[3].z			= 0.0f;
	Verts[3].color		= dwColor;
	Verts[3].rhw		= 1.0f;

	Verts[4].x			= rect.left-0.5f+1;
	Verts[4].y			= rect.top-0.5f+1;
	Verts[4].z			= 0.0f;
	Verts[4].color		= dwColor;
	Verts[4].rhw		= 1.0f;

	m_pRapi->DrawPrimitive( D3DPT_LINESTRIP, Verts, 5, TVERTEX, 0, 1 );
}


void GUIMasterShell::DrawSelectionRect()
{
	RECT rect;
	rect.left	= m_mouseData.x1;
	rect.top	= m_mouseData.y1;
	rect.right	= m_mouseData.x2;
	rect.bottom = m_mouseData.y2;
	DrawRect( rect, 0xFF00FF00 );
}


void GUIMasterShell::DrawCursor( int x,  int y )
{
	tVertex Verts[4];
	memset( Verts, 0, sizeof( sVertex ) * 4 );
	
	RECT rect;
	GetClientRect( GetRenderHWnd(), &rect );

	Verts[0].x			= rect.left-0.5f;
	Verts[0].y			= y-0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].color		= GetCursorColor();
	Verts[0].rhw		= 1.0f;

	Verts[1].x			= rect.right-0.5f;
	Verts[1].y			= y-0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].color		= GetCursorColor();
	Verts[1].rhw		= 1.0f;

	Verts[2].x			= x-0.5f;
	Verts[2].y			= rect.top-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].color		= GetCursorColor();
	Verts[2].rhw		= 1.0f;

	Verts[3].x			= x-0.5f;
	Verts[3].y			= rect.bottom-0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].color		= GetCursorColor();
	Verts[3].rhw		= 1.0f;

	m_pRapi->SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_CLAMP,
									D3DTADDRESS_CLAMP,
									D3DTFG_POINT,		
									D3DTFN_POINT,
									D3DTFP_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									true );
	
	m_pRapi->DrawPrimitive( D3DPT_LINELIST, Verts, 4, TVERTEX, 0, 1 );
}


void GUIMasterShell::DrawGUIWindow( UIRect rect, UINormalizedRect uvrect, float alpha )
{
	tVertex Verts[4];
	memset( Verts, 0, sizeof( sVertex ) * 4 );

	vector_3 default_color( 1.0f, 1.0f, 1.0f );
	DWORD color	= MAKEDWORDCOLOR( default_color );
	color		&= 0x00FFFFFF;

	DWORD dwColor = 0xFF002266;

	Verts[0].x			= rect.left-0.5f;
	Verts[0].y			= rect.top-0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].uv.u		= uvrect.left;
	Verts[0].uv.v		= uvrect.bottom;
	Verts[0].color		= (dwColor | (((BYTE)((alpha)*255.0f))<<24));
	Verts[0].rhw		= 1.0f;

	Verts[1].x			= rect.left-0.5f;
	Verts[1].y			= rect.bottom-0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].uv.u		= uvrect.left;
	Verts[1].uv.v		= uvrect.top;
	Verts[1].color		= (dwColor | (((BYTE)((alpha)*255.0f))<<24));
	Verts[1].rhw		= 1.0f;

	Verts[2].x			= rect.right-0.5f;
	Verts[2].y			= rect.top-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].uv.u		= uvrect.right;
	Verts[2].uv.v		= uvrect.bottom;
	Verts[2].color		= (dwColor | (((BYTE)((alpha)*255.0f))<<24));
	Verts[2].rhw		= 1.0f;

	Verts[3].x			= rect.right-0.5f;
	Verts[3].y			= rect.bottom-0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].uv.u		= uvrect.right;
	Verts[3].uv.v		= uvrect.top;
	Verts[3].color		= (dwColor | (((BYTE)((alpha)*255.0f))<<24));
	Verts[3].rhw		= 1.0f;

	m_pRapi->SetTextureStageState(	0,
								D3DTOP_SELECTARG2,
								D3DTOP_SELECTARG2,
								D3DTADDRESS_WRAP,
								D3DTADDRESS_CLAMP,
								D3DTFG_POINT,		
								D3DTFN_POINT,
								D3DTFP_POINT,
								D3DBLEND_SRCALPHA,
								D3DBLEND_INVSRCALPHA,
								true );
	
	m_pRapi->DrawPrimitive( D3DPT_TRIANGLESTRIP, Verts, 4, TVERTEX, 0, 1 );		
	
	
	m_pRapi->SetTextureStageState(	0,
									D3DTOP_SELECTARG2,
									D3DTOP_SELECTARG2,
									D3DTADDRESS_WRAP,
									D3DTADDRESS_CLAMP,
									D3DTFG_POINT,		
									D3DTFN_POINT,
									D3DTFP_POINT,
									D3DBLEND_SRCALPHA,
									D3DBLEND_INVSRCALPHA,
									true );	
}


void GUIMasterShell::CalculateStructureState( UIRect rect, int x, int y )
{
	if (( m_mouseData.bLButtonDown ) &&
		( x >= rect.left ) && ( x <= rect.right ) && ( y >= rect.top ) && ( y <= rect.bottom ))
	{
		m_structureMode = S_MOVE;
	}
	else 
	{
		m_structureMode = S_NONE;
	}

	if (( rect.top <= y ) && ( rect.bottom >= y ))
	{
		if ( InRange( rect.left, x, 3 ) ) {
			m_structureMode = S_RESIZE_X1;
		}
		if ( InRange( rect.right, x, 3 ) ) {
			m_structureMode = S_RESIZE_X2;
		}
	}

	if (( rect.left <= x ) && ( rect.right >= x ))
	{
		if ( InRange( rect.top, y, 3 ) ) {
			m_structureMode = S_RESIZE_Y1;
		}
		if ( InRange( rect.bottom, y, 3 ) ) {
			m_structureMode = S_RESIZE_Y2;
		}
	}
}

void GUIMasterShell::CalculateStructureCursor( UIRect rect, int x, int y )
{
	SetCursorColor( DEFAULT_CURSOR_COLOR );

	if (( rect.top <= y ) && ( rect.bottom >= y ))
	{
		if ( InRange( rect.left, x, 3 ) ) {		
			SetCursorColor( ACTION_CURSOR_COLOR );
		}
		if ( InRange( rect.right, x, 3 ) ) {		
			SetCursorColor( ACTION_CURSOR_COLOR );
		}
	}

	if (( rect.left <= x ) && ( rect.right >= x ))
	{
		if ( InRange( rect.top, y, 3 ) ) {		
			SetCursorColor( ACTION_CURSOR_COLOR );
		}
		if ( InRange( rect.bottom, y, 3 ) ) {		
			SetCursorColor( ACTION_CURSOR_COLOR );
		}
	}
}


bool GUIMasterShell::InRange( int p1, int p2, int range )
{
	if (( p2 >= (p1-range)) && ( p2 <= (p1+range) ))
	{
		return true;
	}
	return false;
}


void GUIMasterShell::Move( eDirection dir )
{
	if ( !GetMoveMode() )
	{
		return;
	}

	std::set< UIWindow * > window_set;
	RetreiveWindowsToMove( window_set );
	std::set< UIWindow * >::iterator i;
	for ( i = window_set.begin(); i != window_set.end(); ++i )
	{
		switch ( dir )
		{
		case LEFT:
			{
				(*i)->GetRect().left	-= 1;
				(*i)->GetRect().right	-= 1;
			}
			break;
		case RIGHT:
			{
				(*i)->GetRect().left	+= 1;
				(*i)->GetRect().right	+= 1;
			}
			break;
		case UP:
			{
				(*i)->GetRect().top		-= 1;
				(*i)->GetRect().bottom	-= 1;
			}
			break;
		case DOWN:
			{
				(*i)->GetRect().top		+= 1;
				(*i)->GetRect().bottom	+= 1;
			}
			break;
		}
	}
}


void GUIMasterShell::Move()
{
	if ( !GetMoveMode() )
	{
		return;
	}

	std::set< UIWindow * > window_set;
	RetreiveWindowsToMove( window_set );
	std::set< UIWindow * >::iterator i;
	for ( i = window_set.begin(); i != window_set.end(); ++i )
	{	
		// Move the main window frame and UV frame
		(*i)->GetRect().left	+= m_mouseData.x2 - m_mouseData.x1;		
		(*i)->GetRect().right	+= m_mouseData.x2 - m_mouseData.x1;		
		(*i)->GetRect().top		+= m_mouseData.y2 - m_mouseData.y1;		
		(*i)->GetRect().bottom	+= m_mouseData.y2 - m_mouseData.y1;		
		

		// Lock the position to an edge if close enough		
		if ( GetSnapToGuides() == true ) {
			int distance = 0;
			int tolerance = 5;

			if ( InRange( (*i)->GetRect().left, 0, tolerance )) {
				distance = (*i)->GetRect().left;
				(*i)->GetRect().left		-= distance;
				(*i)->GetRect().right		= 0;
			}
			if ( InRange( (*i)->GetRect().top, 0, tolerance )) {
				distance = (*i)->GetRect().top;
				(*i)->GetRect().top			-= distance;
				(*i)->GetRect().bottom		= 0;
			}
			if ( InRange( (*i)->GetRect().right, m_screenWidth, tolerance )) {
				distance = m_screenWidth - (*i)->GetRect().right;
				(*i)->GetRect().left		+= distance;
				(*i)->GetRect().right		=  m_screenWidth;
			}
			if ( InRange( (*i)->GetRect().bottom, m_screenHeight, tolerance )) {
				distance = m_screenHeight - (*i)->GetRect().bottom;
				(*i)->GetRect().top			+= distance;
				(*i)->GetRect().bottom		=  m_screenHeight;
			}
		}
	
		/*
		if ( GetDrawGrid() && (*i).bSnapToGrid ) {
			int x = 0;
			int y = 0;
			int distance = 0;
			int tolerance = 5;

			while ( x <= m_screenWidth ) {				
				if ( InRange( m_pLeadWindow->GetRect().left, x, -tolerance, tolerance ) ) {
					distance = m_pLeadWindow->GetRect().left - x;
					m_pLeadWindow->GetRect().right		-= distance;
					m_pLeadWindow->GetRect().left		= x;
					uxvy_rect.left	-= distance;
					uxvy_rect.right	-= distance;						
				}
				else if ( InRange( m_pLeadWindow->GetRect().right, x, -tolerance, tolerance )) {
						distance = x - m_pLeadWindow->GetRect().right;
						m_pLeadWindow->GetRect().left		+= distance;
						m_pLeadWindow->GetRect().right		= x;
						uxvy_rect.left	+= distance;
						uxvy_rect.right	+= distance;
				} 
				
				x += GetGridWidth();
			}

			while ( y <= m_screenHeight ) {				
				if ( InRange( m_pLeadWindow->GetRect().top, y, -tolerance, tolerance )) {
					distance = m_pLeadWindow->GetRect().top - y;
					m_pLeadWindow->GetRect().bottom		-= distance;
					m_pLeadWindow->GetRect().top		= y;
					uxvy_rect.top	-= distance;
					uxvy_rect.bottom	-= distance;
				}
				
				else if ( InRange( m_pLeadWindow->GetRect().bottom, y, -tolerance, tolerance )) {
					distance = y - m_pLeadWindow->GetRect().bottom;
					m_pLeadWindow->GetRect().top		+= distance;
					m_pLeadWindow->GetRect().bottom		= y;
					uxvy_rect.top	+= distance;
					uxvy_rect.bottom	+= distance;
				}
				y += GetGridHeight();
			}
		}
		*/

		
		if ( GetReferenceLines() ) {
			int distance = 0;
			int tolerance = 5;

			for ( int j = 0; j < m_RefLines.size(); j++ ) {
				if ( m_RefLines[j].rAxis == Y_AXIS ) {
					if ( InRange( m_pLeadWindow->GetRect().left, m_RefLines[j].Coord, tolerance ) ) {
						distance = m_pLeadWindow->GetRect().left - m_RefLines[j].Coord;
						m_pLeadWindow->GetRect().right		-= distance;
						m_pLeadWindow->GetRect().left		= m_RefLines[j].Coord;
					}
					else if ( InRange( m_pLeadWindow->GetRect().right, m_RefLines[j].Coord, tolerance )) {
						distance = m_RefLines[j].Coord - m_pLeadWindow->GetRect().right;
						m_pLeadWindow->GetRect().left		+= distance;
						m_pLeadWindow->GetRect().right		= m_RefLines[j].Coord;
					}
				}

				if ( m_RefLines[j].rAxis == X_AXIS ) {
					if ( InRange( m_pLeadWindow->GetRect().top, m_RefLines[j].Coord, tolerance )) {
						distance = m_pLeadWindow->GetRect().top - m_RefLines[j].Coord;
						m_pLeadWindow->GetRect().bottom		-= distance;
						m_pLeadWindow->GetRect().top		= m_RefLines[j].Coord;
					}
					
					else if ( InRange( m_pLeadWindow->GetRect().bottom, m_RefLines[j].Coord, tolerance )) {
						distance = m_RefLines[j].Coord - m_pLeadWindow->GetRect().bottom;
						m_pLeadWindow->GetRect().top		+= distance;
						m_pLeadWindow->GetRect().bottom		= m_RefLines[j].Coord;
					}
				}
			}
		}		
	}
	
	m_mouseData.x1 = m_mouseData.x2;
	m_mouseData.y1 = m_mouseData.y2;	
}


void GUIMasterShell::Resize( int x, int y, bool bUVStructure )
{
	if ( !GetResizeMode() )
	{
		return;
	}

	if ( m_pLeadWindow )
	{
		UIRect uxvy_rect = GetUXVYRect( m_pLeadWindow );

		switch ( m_structureMode ) {
		case S_RESIZE_X1:
			{				
				float test = x - m_pLeadWindow->GetRect().left;				
				float correction =  (test/(m_pLeadWindow->GetRect().right-m_pLeadWindow->GetRect().left))*(m_pLeadWindow->GetUVRect().right-m_pLeadWindow->GetUVRect().left);				
				m_pLeadWindow->GetRect().left = x;
		
				if ( bUVStructure ) 
				{											
					UINormalizedRect rect = m_pLeadWindow->GetUVRect();
					rect.left += correction;						
					m_pLeadWindow->SetUVRect( rect.left, rect.right, rect.top, rect.bottom );
				}
			}
			break;
		case S_RESIZE_X2:
			{
				float test = x - m_pLeadWindow->GetRect().right;				
				float correction =  (test/(m_pLeadWindow->GetRect().right-m_pLeadWindow->GetRect().left))*(m_pLeadWindow->GetUVRect().right-m_pLeadWindow->GetUVRect().left);
						
				m_pLeadWindow->GetRect().right = x;
							
				if ( bUVStructure ) 
				{	
					UINormalizedRect rect = m_pLeadWindow->GetUVRect();
					rect.right += correction;						
					m_pLeadWindow->SetUVRect( rect.left, rect.right, rect.top, rect.bottom );
				}
			}
			break;
		case S_RESIZE_Y1:
			{
				float test = y - m_pLeadWindow->GetRect().top;				
				float correction =  (test/(m_pLeadWindow->GetRect().bottom-m_pLeadWindow->GetRect().top))*(m_pLeadWindow->GetUVRect().bottom-m_pLeadWindow->GetUVRect().top);
						
				m_pLeadWindow->GetRect().top = y;
						
				if ( bUVStructure ) 
				{						
					UINormalizedRect rect = m_pLeadWindow->GetUVRect();
					rect.bottom -= correction;						
					m_pLeadWindow->SetUVRect( rect.left, rect.right, rect.top, rect.bottom );					
				}
			}
			break;
		case S_RESIZE_Y2:
			{
				float test = y - m_pLeadWindow->GetRect().bottom;				
				float correction =  (test/(m_pLeadWindow->GetRect().bottom-m_pLeadWindow->GetRect().top))*((m_pLeadWindow->GetUVRect().bottom-m_pLeadWindow->GetUVRect().top));
								
				m_pLeadWindow->GetRect().bottom = y;
						
				if ( bUVStructure ) 
				{						
					UINormalizedRect rect = m_pLeadWindow->GetUVRect();
					rect.top -= correction;						
					m_pLeadWindow->SetUVRect( rect.left, rect.right, rect.top, rect.bottom );					
				}
			}
			break;	
		}
	}
}


void GUIMasterShell::GetAvailableInterfaces( StringVec & interfaces )
{
	FuelHandle hUI( "ui:interfaces" );
	if ( hUI.IsValid() )
	{
		FuelHandleList hlInterfaces = hUI->ListChildBlocks( 3 );
		FuelHandleList::iterator i;
		for ( i = hlInterfaces.begin(); i != hlInterfaces.end(); ++i )
		{
			gpstring sParent = (*i)->GetParentBlock()->GetName();
			if ( sParent.same_no_case( (*i)->GetName() ) || (*i)->GetBool( "interface", false ) )
			{
				interfaces.push_back( (*i)->GetName() );
			}
		}
	}
}


void GUIMasterShell::CreateInterface( gpstring sInterface )
{
	gUIShell.AddBlankInterface( sInterface );
	SetInterface( sInterface );
	m_draw_order = 0;
}


void GUIMasterShell::LoadInterface( gpstring sInterface )
{
	FastFuelHandle hUI( "ui:interfaces" );
	if ( hUI.IsValid() )
	{
		FastFuelHandleColl hlInterfaces;
		hUI.ListChildren( hlInterfaces, 3 );
		FastFuelHandleColl::iterator i;
		for ( i = hlInterfaces.begin(); i != hlInterfaces.end(); ++i )
		{
			gpstring sName = (*i).GetName();
			if ( sName.same_no_case( sInterface ) )
			{
				gUIShell.ActivateInterface( *i );
				SetInterface( sInterface );
			}
		}
	}

	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::iterator j;
	for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
		if ( (*j)->name.same_no_case( m_sInterface ) ) {
			
			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				UIWindowDrawMap::iterator i = (*j)->ui_draw_windows.end();
				--i;					
				m_draw_order = (*i).first + 1;
			}				
		}
	}
}


gpstring GUIMasterShell::GetUniqueName( gpstring sCopyName )
{
	gpstring sName;
	if ( sCopyName.empty() )
	{		
		if ( GetControlType().empty() )
		{
			sName = "window";
		}
		else
		{
			sName = GetControlType();
		}
	}
	else
	{
		sName = sCopyName;
	}

	bool bExists = true;
	int counter = 0;
	gpstring sNewName = sName;
	while ( bExists )
	{	
		sNewName.appendf( "_%d", counter++ );
		bExists = DoesNameExist( sNewName );
		if ( bExists )
		{
			sNewName = sName;
		}
	}	
	return sNewName;
}


bool GUIMasterShell::DoesNameExist( gpstring sName )
{
	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::iterator j;
	UIWindowDrawMap::iterator i;
	for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
		if ( (*j)->bVisible == true ) {
			for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i ) 
			{
				if ( (*i).second->GetName().same_no_case( sName ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}


void GUIMasterShell::BringSelectedWindowsToFront()
{
	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::iterator j;
	for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
		if ( (*j)->name.same_no_case( m_sInterface ) ) {

			if ( (*j)->ui_draw_windows.size() != 0 )
			{
				UIWindowDrawMap::iterator i = (*j)->ui_draw_windows.end();
				--i;

				UIWindowVec::iterator k;
				for ( k = m_selectedWindows.begin(); k != m_selectedWindows.end(); ++k )
				{
										
					(*k)->SetDrawOrder( (*k)->GetDrawOrder() + (*i).first + 1 );

					UIWindowDrawMap::iterator o;
					for ( o = (*j)->ui_draw_windows.begin(); o != (*j)->ui_draw_windows.end(); ++o )
					{
						if ( (*o).second == (*k) )
						{							
							(*j)->ui_draw_windows.erase( o );
							(*j)->ui_draw_windows.insert( UIWindowDrawPair( (*k)->GetDrawOrder(), (*k) ) );
							break;
						}
					}								
				}			
			}
		}
	}
}


void GUIMasterShell::GetActiveInterfaces( StringVec & sv_interfaces )
{
	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::iterator j;
	for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
		sv_interfaces.push_back( (*j)->name );
	}
}


void GUIMasterShell::CloseInterface( gpstring sInterface )
{
	gUIShell.DeactivateInterface( sInterface );
}


void GUIMasterShell::ShowInterface( gpstring sInterface, bool bShow )
{
	if ( bShow )
	{
		gUIShell.ShowInterface( sInterface );
	}
	else
	{
		gUIShell.HideInterface( sInterface );
	}
}
	

gpstring GUIMasterShell::GetTextureNamingKeyName( gpstring sFilename )
{
	gpstring sNew = sFilename;
	int pos = sFilename.find_last_of( "\\", sFilename.size() );
	if ( pos != gpstring::npos )
	{
		sNew = sFilename.substr( pos+1, sFilename.size() );
		int pos = sNew.find_last_of( ".", sNew.size() );
		if ( pos != gpstring::npos )
		{
			sNew = sNew.substr( 0, pos );
		}
	}
	return sNew;
}


void GUIMasterShell::ResizeRectToTexture( UIRect & rect, unsigned int texture )
{
	int width	= 0;
	int height	= 0;

	// Make sure there is a texture on the polygon before starting
	if ( texture == 0 )
	{
		return;
	}
	
	width	= gUIShell.GetRenderer().GetTextureWidth( texture );
	height	= gUIShell.GetRenderer().GetTextureHeight( texture );
	
	// Resize the current polygon to the dimensions of the texture
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;
}


void GUIMasterShell::ResizeUVRectToTexture( UINormalizedRect & uvrect, UIRect & rect, unsigned int texture )
{
	int width	= 0;
	int height	= 0;

	// Make sure there is a texture on the polygon before starting
	if ( texture == 0 )
	{
		return;
	}
	
	width	= gUIShell.GetRenderer().GetTextureWidth( texture );
	height	= gUIShell.GetRenderer().GetTextureHeight( texture );

	UINormalizedRect new_uvrect;
	new_uvrect.right	= (float)(rect.right-rect.left)/(float)width;
	new_uvrect.top		= (float)(rect.bottom-rect.top)/(float)height;
	uvrect = new_uvrect;	
}


void GUIMasterShell::RetreiveWindowsToMove( std::set< UIWindow * > & window_set )
{
	UIWindowVec::iterator i;
	UIWindowVec::iterator j;
	for ( i = m_selectedWindows.begin(); i != m_selectedWindows.end(); ++i )
	{
		UIWindow * pWindow = (*i);
		window_set.insert( pWindow );

		UIWindowVec windows;
		if ( GetParentMove() )
		{
			windows.clear();
			windows = pWindow->GetChildren();
			for ( j = windows.begin(); j != windows.end(); ++j )
			{
				window_set.insert( *j );
			}
		}

		if ( GetGroupMove() )
		{
			windows.clear();
			windows = gUIShell.ListWindowsOfGroup( pWindow->GetGroup() );
			for ( j = windows.begin(); j != windows.end(); ++j )
			{
				window_set.insert( *j );
			}
		}

		if ( GetDockGroupMove() )
		{
			windows.clear();
			windows = gUIShell.ListWindowsOfDockGroup( pWindow->GetGroup() );
			for ( j = windows.begin(); j != windows.end(); ++j )
			{
				window_set.insert( *j );
			}
		}

		if ( GetRadioGroupMove() )
		{
			windows.clear();
			windows = gUIShell.ListWindowsOfRadioGroup( pWindow->GetGroup() );
			for ( j = windows.begin(); j != windows.end(); ++j )
			{
				window_set.insert( *j );
			}
		}
	}	
}


UIRect GUIMasterShell::GetUXVYRect( UIWindow * pWindow )
{
	UIRect rect;
	if ( pWindow->GetUVRect().left != 0 )
	{
		rect.left = pWindow->GetRect().left + (int)( (float)(1.0f / pWindow->GetUVRect().left) * (pWindow->GetRect().right-pWindow->GetRect().left) );
	}
	else
	{
		rect.left = pWindow->GetRect().left;
	}
	if ( pWindow->GetUVRect().right != 0 )
	{
		rect.right = pWindow->GetRect().left + (int)( (float)(1.0f / pWindow->GetUVRect().right) * (pWindow->GetRect().right-pWindow->GetRect().left) );
	}
	else
	{
		rect.right = pWindow->GetRect().right;
	}
	if ( pWindow->GetUVRect().top != 0 )
	{
		rect.top = pWindow->GetRect().top + (int)( (float)(1.0f / pWindow->GetUVRect().top)  * (pWindow->GetRect().bottom-pWindow->GetRect().top)  );
	}
	else
	{
		rect.top = pWindow->GetRect().top;
	}
	if ( pWindow->GetUVRect().bottom != 0 )
	{
		rect.bottom = pWindow->GetRect().top + (int)( (float)(1.0f / pWindow->GetUVRect().bottom) * (pWindow->GetRect().bottom-pWindow->GetRect().top));
	}
	else
	{
		rect.bottom = pWindow->GetRect().bottom;
	}

	return rect;
}


void GUIMasterShell::DeleteSelectedWindows()
{
	UIWindowVec::iterator i;
	for ( i = m_selectedWindows.begin(); i != m_selectedWindows.end(); ++i )
	{
		(*i)->SetMarkForDeletion( true );
	}
	m_selectedWindows.clear();
	m_pLeadWindow = 0;
	gUIShell.ProcessWindowDeletionList();
}


bool GUIMasterShell::IsWindowSelected( UIWindow * pWindow )
{
	UIWindowVec::iterator i;
	for ( i = m_selectedWindows.begin(); i != m_selectedWindows.end(); ++i )
	{
		if ( pWindow == (*i) )
		{
			return true;
		}
	}
	return false;
}


void GUIMasterShell::DrawLine( int Coord, REF_AXIS rAxis, DWORD color )
{
	tVertex Verts[4];
	memset( Verts, 0, sizeof( sVertex ) * 4 );
	
	RECT rect;
	GetClientRect( GetRenderHWnd(), &rect );

	if ( rAxis == X_AXIS ) {
		Verts[0].x			= rect.left-0.5f;
		Verts[0].y			= Coord-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].color		= color;
		Verts[0].rhw		= 1.0f;

		Verts[1].x			= rect.right-0.5f;
		Verts[1].y			= Coord-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].color		= color;
		Verts[1].rhw		= 1.0f;
	}
	else {
		Verts[0].x			= Coord-0.5f;
		Verts[0].y			= rect.top-0.5f;
		Verts[0].z			= 0.0f;
		Verts[0].color		= color;
		Verts[0].rhw		= 1.0f;

		Verts[1].x			= Coord-0.5f;
		Verts[1].y			= rect.bottom-0.5f;
		Verts[1].z			= 0.0f;
		Verts[1].color		= color;
		Verts[1].rhw		= 1.0f;
	}
	
	m_pRapi->DrawPrimitive( D3DPT_LINESTRIP, Verts, 2, TVERTEX, 0, 1 );
}


void GUIMasterShell::DrawRefLines()
{
	RefLineVec::iterator i;
	for ( i = m_RefLines.begin(); i != m_RefLines.end(); ++i ) {		
		if ( (*i).ID == m_SelectedLine )
		{
			DrawLine( (*i).Coord, (*i).rAxis, 0xFF00FF33 );
		}
		else
		{
			DrawLine( (*i).Coord, (*i).rAxis, 0xFF00FFFF );
		}
	}
}


void GUIMasterShell::CreateReferenceLine( REF_AXIS axis )
{
	static int	guide_ID = 0;
	Ref_Line	rLine;

	rLine.Coord		= 0;
	rLine.rAxis		= axis;
	rLine.ID		= ++guide_ID;

	m_RefLines.push_back( rLine );
	m_SelectedLine = rLine.ID;
}


void GUIMasterShell::MoveReferenceLine( int x, int y )
{
	RECT rect;
	GetClientRect( m_hWndRender, &rect );
	RefLineVec::iterator i;
	for ( i = m_RefLines.begin(); i != m_RefLines.end(); ++i ) {
		if ( (*i).ID == m_SelectedLine ) {
			if ( (*i).rAxis == X_AXIS )
				(*i).Coord = y;
				
			if ( (*i).rAxis == Y_AXIS )
				(*i).Coord = x;
			return;
		}
	}
}


void GUIMasterShell::FindReferenceLine( int x, int y )
{	
	RefLineVec::iterator i;
	for ( i = m_RefLines.begin(); i != m_RefLines.end(); ++i ) {
		if ( (*i).rAxis == X_AXIS ) {
			if ( InRange( y, (*i).Coord, 3 ) ) {
				m_SelectedLine = (*i).ID;
				return;
			}
		}
		if ( (*i).rAxis == Y_AXIS ) {
			if ( InRange( x, (*i).Coord, 3 ) ) {
				m_SelectedLine = (*i).ID;
				return;
			}
		}		
	}
	m_SelectedLine = -1;	
}


void GUIMasterShell::DeleteReferenceLine()
{
	for( RefLineVec::iterator i = m_RefLines.begin(); 
		 i != m_RefLines.end(); ++i	) {
		if ( (*i).ID == m_SelectedLine ) {
			m_RefLines.erase( i );
			return;
		}
	}
}


void GUIMasterShell::ResizeUV( int x, int y )
{
	UIWindowVec::iterator i;
	for ( i = m_selectedWindows.begin(); i != m_selectedWindows.end(); ++i )
	{
		UIRect uxvy_rect = GetUXVYRect( *i );

		RECT rect;
		GetClientRect( GetRenderHWnd(), &rect );

		float ux1 = (float)uxvy_rect.left/(float)rect.right;
		float ux2 = (float)uxvy_rect.right/(float)rect.right;
		float vy1 = (float)uxvy_rect.top/(float)rect.bottom;
		float vy2 = (float)uxvy_rect.bottom/(float)rect.bottom;			
		
		float xf = (float)x / (float)rect.right;
		float yf = (float)y / (float)rect.bottom;

		float fNumber;

		switch ( m_UVState ) {
		
		case UV_RESIZE_X2:
			{
				fNumber = ( ux2 - ux1 ) / ( xf - ux1 );				
				UINormalizedRect rect = (*i)->GetUVRect();
				rect.left *= fNumber;
				rect.right *= fNumber;
				(*i)->SetUVRect( rect.left, rect.right, rect.top, rect.bottom );								
			}
			break;	
		case UV_RESIZE_Y2:
			{
				fNumber = ( vy2 - vy1 ) / ( yf - vy1 );			
				UINormalizedRect rect = (*i)->GetUVRect();
				rect.top *= fNumber;
				rect.bottom *= fNumber;			
			}
			break;						
		}
	}
}


void GUIMasterShell::CalculateUVState( UIRect rect, int x, int y )
{
	if ( m_mouseData.bLButtonDown )
	{
		m_UVState = UV_MOVE;
	}
	else 
	{
		m_UVState = UV_NONE;
	}

	if (( rect.top <= y ) && ( rect.bottom >= y ))
	{		
		if ( InRange( rect.right, x, 3 ) ) {
			m_UVState = UV_RESIZE_X2;
		}
	}

	if (( rect.left <= x ) && ( rect.right >= x ))
	{
		if ( InRange( rect.bottom, y, 3 ) ) {
			m_UVState = UV_RESIZE_Y2;
		}		
	}
}


void GUIMasterShell::DrawUVEdge( UIRect rect )
{
	tVertex Verts[5];
	memset( Verts, 0, sizeof( sVertex ) * 5 );

	vector_3 default_color( 1.0f, 1.0f, 1.0f );
	DWORD color	= MAKEDWORDCOLOR( default_color );
	color		&= 0xFFFFFF00;

	Verts[0].x			= rect.left-0.5f;
	Verts[0].y			= rect.top-0.5f;
	Verts[0].z			= 0.0f;
	Verts[0].color		= color;
	Verts[0].rhw		= 1.0f;

	Verts[1].x			= rect.right-0.5f;
	Verts[1].y			= rect.top-0.5f;
	Verts[1].z			= 0.0f;
	Verts[1].color		= color;
	Verts[1].rhw		= 1.0f;

	Verts[2].x			= rect.right-0.5f;
	Verts[2].y			= rect.bottom-0.5f;
	Verts[2].z			= 0.0f;
	Verts[2].color		= color;
	Verts[2].rhw		= 1.0f;

	Verts[3].x			= rect.left-0.5f;
	Verts[3].y			= rect.bottom-0.5f;
	Verts[3].z			= 0.0f;
	Verts[3].color		= 0xFFFF0000;
	Verts[3].rhw		= 1.0f;

	Verts[4].x			= rect.left-0.5f;
	Verts[4].y			= rect.top-0.5f;
	Verts[4].z			= 0.0f;
	Verts[4].color		= 0xFFFF0000;
	Verts[4].rhw		= 1.0f;
	
	m_pRapi->DrawPrimitive( D3DPT_LINESTRIP, Verts, 5, TVERTEX, 0, 1 );
}


void GUIMasterShell::MoveUV()
{
}


void GUIMasterShell::GetPossibleParents( UIWindow * pWindow, UIWindowVec & windows )
{
	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::iterator j;
	UIWindowMap::iterator i;
	for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
		if (( (*j)->bVisible == true ) && ( (*j)->name.same_no_case( GetInterface() ) )) {
			for ( i = (*j)->ui_windows.begin(); i != (*j)->ui_windows.end(); ++i ) 
			{
				if ( (*i).second != pWindow )
				{
					windows.push_back( (*i).second );
				}
			}
		}
	}
}


void GUIMasterShell::Save()
{
	FuelHandle hSave;
	
	{
		FuelHandle hInterface( "ui:interfaces" );
		FuelHandleList hlInterfaces = hInterface->ListChildBlocks( 3 );
		FuelHandleList::iterator i;
		for ( i = hlInterfaces.begin(); i != hlInterfaces.end(); ++i )
		{		
			if ( GetInterface().same_no_case( (*i)->GetName() ) )
			{
				hSave = *i;
				break;
			}
		}

		if ( hSave.IsValid() == false )
		{
			hSave = hInterface->CreateChildDirBlock( GetInterface() );
			gpstring sInterface = GetInterface();
			sInterface += ".gas";
			hSave = hSave->CreateChildBlock( GetInterface(), sInterface );
			hSave->Set( "interface", true );
		}
		else
		{
			bool bNotDone = true;
			FuelHandle hOld;
			while ( bNotDone )
			{
				hOld = hSave->GetChildBlock( GetInterface() );
				if ( !hOld.IsValid() )
				{
					hOld = hSave;
				}
				FuelHandleList hlWindows = hOld->ListChildBlocks( -1 );
				FuelHandleList::iterator k;
				bNotDone = false;
				for ( k = hlWindows.begin(); k != hlWindows.end(); ++k )
				{
					gpstring sType = (*k)->GetType();
					if ( !sType.same_no_case( "" ) && !sType.same_no_case( "selection_texture" ) )
					{
						GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
						GUInterfaceVec::iterator j;
						UIWindowDrawMap::iterator i;
						for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
							if (( (*j)->bVisible == true ) && ( (*j)->name.same_no_case( GetInterface() ) )) {
								
								bool bFound = false;
								for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i ) 
								{
									if ( (*i).second->GetName().same_no_case( (*k)->GetName() ))
									{
										bFound = true;
									}
								}

								if ( !bFound && hOld.GetPointer() == (*k)->GetParent() )
								{
									//FuelHandle hParent = (*k)->GetParent();
									//hParent->DestroyChildBlock( *k );
									hOld->DestroyChildBlock( *k );
									bNotDone = true;
									break;
								}
							}
						}					
					}
					if ( bNotDone )
					{
						break;
					}
				}
			}
			
			hSave = hOld;
			
			/*
			if ( hOld.IsValid() )
			{
				hSave->DestroyChildBlock( hOld );
			}
			gpstring sInterface = GetInterface();
			sInterface += ".gas";
			
			hSave = hSave->CreateChildBlock( GetInterface(), sInterface );
			hSave->Set( "interface", true );
			*/
		}
	}

	GUInterfaceVec interfaces = m_pGameGUI->GetInterfaces();
	GUInterfaceVec::iterator j;
	UIWindowDrawMap::iterator i;
	for ( j = interfaces.begin(); j != interfaces.end(); ++j ) {
		if (( (*j)->bVisible == true ) && ( (*j)->name.same_no_case( GetInterface() ) )) {
			for ( i = (*j)->ui_draw_windows.begin(); i != (*j)->ui_draw_windows.end(); ++i ) 
			{
				(*i).second->Save( hSave, false );
			}
		}
	}

	hSave->GetDB()->SaveChanges();
}


void GUIMasterShell::Copy()
{
	m_clipboard.clear();
	UIWindowVec::iterator i;
	for ( i = m_selectedWindows.begin(); i != m_selectedWindows.end(); ++i )
	{
		m_clipboard.push_back( *i );
	}
}


void GUIMasterShell::Cut()
{
	Copy();
	DeleteSelectedWindows();
}


void GUIMasterShell::Paste()
{
	UIWindowVec::iterator i;
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
	{
		UIWindow * pWindow = gUIShell.CreateDefaultWindowOfType( (*i)->GetType() );
		*pWindow = *(*i);
		pWindow->SetDrawOrder( (*i)->GetDrawOrder() + (++m_draw_order) );
		pWindow->SetTextureIndex( (*i)->GetTextureIndex() );
		pWindow->GetRect() = (*i)->GetRect();
		pWindow->SetName( GetUniqueName( (*i)->GetName() ) );
		pWindow->SetUVRect( (*i)->GetUVRect().left, (*i)->GetUVRect().right, (*i)->GetUVRect().top, (*i)->GetUVRect().bottom );
		pWindow->SetAlpha( (*i)->GetAlpha() );
		pWindow->SetHasTexture( (*i)->HasTexture() );		
		
		if ( pWindow->GetType() == UI_TYPE_TEXT )
		{
			UIText * pText = (UIText *)pWindow;			
			pText->SetFont( ((UIText *)(*i))->GetFont() );
			pText->SetJustification( ((UIText *)(*i))->GetJustification() );
			pText->SetText( ((UIText *)(*i))->GetText() );				
			pWindow->SetDrawOrder( pWindow->GetDrawOrder() + (++m_draw_order) );
		}

		gUIShell.AddWindowToInterface( pWindow, m_sInterface );
		
		POINT pt;
		GetCursorPos( &pt );
		ScreenToClient( m_hWndRender, &pt );		
	}
}


void GUIMasterShell::CreateGuiIdMap()
{
	int interfaceId = 0;

	typedef std::map< int, int > InterfaceToWindowMap;
	typedef std::pair< int, int > InterfaceToWindowPair;
	InterfaceToWindowMap iwmap;

	FuelHandle hIdMap( "ui:localization_ui_map" );
	if ( !hIdMap.IsValid() )
	{
		FuelHandle hBase( "ui" );
		hIdMap = hBase->CreateChildBlock( "localization_ui_map", "localization_ui_map.gas" );			
		FuelHandleList hlInterfaces = hIdMap->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlInterfaces.begin(); i != hlInterfaces.end(); ++i )
		{
			int windowId	= 0;
			int id = 0;
			(*i)->Get( "interface_id", id );
			if ( id > interfaceId )
			{
				interfaceId = id;
			}

			FuelHandleList hlWindows = (*i)->ListChildBlocks( 1 );
			FuelHandleList::iterator j;
			for ( j = hlWindows.begin(); j != hlWindows.end(); ++j )
			{
				if ( (*j)->FindFirstKeyAndValue() )
				{
					gpstring sKey, sValue;
					while ( (*j)->GetNextKeyAndValue( sKey, sValue ) )
					{
						int winId = 0;
						stringtool::Get( sKey, winId );
						if ( winId > windowId )
						{
							windowId = winId;
						}

						if ( windowId >= 256 )
						{
							MessageBox( m_hWnd, "You have more than 255 Windows, this is bad for localization.", "Error!", MB_OK );
						}
					}
				}
			}
			iwmap.insert( InterfaceToWindowPair( id, windowId ) );
		}
	}

	if ( interfaceId >= 256 )
	{
		MessageBox( m_hWnd, "You have more than 255 Interfaces, this is bad for localization.", "Error!", MB_OK );
	}

	FuelHandle hUi( "ui:interfaces" );
	if ( hUi.IsValid() )
	{
		FuelHandleList hlInterfaces = hUi->ListChildBlocks( 3 );
		FuelHandleList::iterator i;
		for ( i = hlInterfaces.begin(); i != hlInterfaces.end(); ++i )
		{
			bool bInterface = false;
			(*i)->Get( "interface", bInterface );			

			if ( bInterface )
			{
				FuelHandle hInterface = hIdMap->GetChildBlock( (*i)->GetName() );
				int currInterface = 0;
				if ( !hInterface.IsValid() )
				{					
					hInterface = hIdMap->CreateChildBlock( (*i)->GetName() );				
					++interfaceId;
					hInterface->Set( "interface_id", interfaceId );
					hInterface->Set( "address", (*i)->GetAddress().c_str() );
					currInterface = interfaceId;
				}
				else
				{
					hInterface->Get( "interface_id", currInterface );
				}

				if ( currInterface >= 256 )
				{
					MessageBox( m_hWnd, "You have more than 255 Interfaces, this is bad for localization.", "Error!", MB_OK );
				}

				int windowId = 0;
				InterfaceToWindowMap::iterator found = iwmap.find( currInterface );
				if ( found != iwmap.end() )
				{
					windowId = (*found).second;
				}
				
				FuelHandleList hlWindows = (*i)->ListChildBlocks( -1 );
				FuelHandleList::iterator j;
				for ( j = hlWindows.begin(); j != hlWindows.end(); ++j )
				{
					FuelHandle hWindow = hInterface->GetChildBlock( (*j)->GetName() );
					if ( !hWindow.IsValid() )
					{
						hWindow = hInterface->CreateChildBlock( (*j)->GetName() );
						++windowId;						
						if ( windowId >= 256 )
						{
							MessageBox( m_hWnd, "You have more than 255 Windows, this is bad for localization.", "Error!", MB_OK );
						}
						hWindow->Set( "window_id", windowId );
						hWindow->Set( "address", (*j)->GetAddress() );
					}
				}				
			}
		}
	}

	hUi->GetDB()->SaveChanges();
}

	
			
					