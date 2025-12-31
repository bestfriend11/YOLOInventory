// Window_Mesh_Previewer.cpp : implementation file
//

#include "PrecompEditor.h"
#include "SiegeEditor.h"
#include "Window_Mesh_Previewer.h"
#include "Services.h"
#include "RapiOwner.h"
#include "siege_light_database.h"
#include "MeshPreviewer.h"
#include "Preferences.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Window_Mesh_Previewer

Window_Mesh_Previewer::Window_Mesh_Previewer()
	: m_bDrag( false )
	, m_x( 0 ) 
	, m_y( 0 )
{
}

Window_Mesh_Previewer::~Window_Mesh_Previewer()
{
}


BEGIN_MESSAGE_MAP(Window_Mesh_Previewer, CWnd)
	//{{AFX_MSG_MAP(Window_Mesh_Previewer)
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_RESETCAMERA, OnViewResetcamera)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_VIEW_BACKGROUNDCOLOR, OnViewBackgroundcolor)
	ON_COMMAND(ID_PREFERENCES_WIREFRAME, OnPreferencesWireframe)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Window_Mesh_Previewer message handlers

void Window_Mesh_Previewer::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);	
		
	if ( !Services::DoesSingletonExist() ) 
	{
		return;
	}
	
	if ( RapiOwner::DoesSingletonExist() )
	{
		if ( gPreviewer.IsInitialized() )
		{
			RECT r;
			GetClientRect( &r );
			if ( SIZE_MINIMIZED == nType || r.top == r.bottom || r.left == r.right )
			{
				gRapiOwner.Pause( true );
				return;
			}
			gRapiOwner.InvalidateTextures( 1 );
			gRapiOwner.ForceResume();
			gRapiOwner.StopDrawing( 1 );
			gRapiOwner.PrepareToDraw(	this->m_hWnd, 1,
										false,
										r.right, r.bottom );
			gRapiOwner.RestoreTexturesAndLights( 1 );
		}
	}

	if ( m_wndStatusBar.GetSafeHwnd() ) 
	{
		int status_parts[2];
		RECT rcClient;
		GetClientRect( &rcClient );
		status_parts[0] = rcClient.left;
		status_parts[1] = rcClient.left+200;
		
		m_wndStatusBar.GetStatusBarCtrl().SetParts( 2, status_parts );
		m_wndStatusBar.GetStatusBarCtrl().SetText( "Ready", 0, SBT_NOBORDERS );
	}	
}

void Window_Mesh_Previewer::OnMouseMove(UINT nFlags, CPoint point) 
{
	switch ( nFlags ) {
	case MK_MBUTTON:
		{
			if ( m_bDrag == true ) {
				gPreviewer.RotateCamera( point.x-m_x, point.y-m_y );
			}
		}
		break;
	case MK_RBUTTON:
		{
			if ( m_bDrag == true ) 
			{
				gPreviewer.ZoomCamera( point.y-m_y );				
			}
		}
		break;
	case MK_LBUTTON:
		{
			/*
			if ( m_bDrag == true ) 
			{
				if ( nFlags == (MK_RBUTTON | MK_SHIFT) )
				{
					gPreviewer.TranslateCamera( vector_3( 0, (point.y-m_y)/2, 0 ) );
				}
				else
				{
					gPreviewer.TranslateCamera( vector_3( (point.x-m_x)/2, 0, (point.y-m_y)/2 ) );
				}
			}
			*/
		}
		break;
	}

	m_x = point.x;
	m_y = point.y;
	
	CWnd::OnMouseMove(nFlags, point);
}

void Window_Mesh_Previewer::OnMButtonDown(UINT nFlags, CPoint point) 
{
	SetCapture();
	m_bDrag = true;
	
	CWnd::OnMButtonDown(nFlags, point);
}

void Window_Mesh_Previewer::OnMButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	m_bDrag = false;
	
	CWnd::OnMButtonUp(nFlags, point);
}

void Window_Mesh_Previewer::OnRButtonDown(UINT nFlags, CPoint point) 
{
	SetCapture();
	m_bDrag = true;
	
	CWnd::OnRButtonDown(nFlags, point);
}

void Window_Mesh_Previewer::OnRButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	m_bDrag = false;
	
	CWnd::OnRButtonUp(nFlags, point);
}

BOOL Window_Mesh_Previewer::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if ( zDelta < 0 ) {
		gPreviewer.ZoomOut();
	}
	else {
		gPreviewer.ZoomIn();				
	}
	
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

int Window_Mesh_Previewer::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	/*
	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	int status_parts[2];
	RECT rcClient;
	GetClientRect( &rcClient );
	status_parts[0] = rcClient.right-100;
	status_parts[1] = rcClient.left+100;
	
	m_wndStatusBar.GetStatusBarCtrl().SetParts( 2, status_parts );
	m_wndStatusBar.GetStatusBarCtrl().SetText( "Ready", 0, SBT_NOBORDERS );

	gPreviewer.SetStatusBar( &m_wndStatusBar );
	*/

	return 0;
}

void Window_Mesh_Previewer::OnViewResetcamera() 
{
	gPreviewer.ResetCamera();	
}

void Window_Mesh_Previewer::OnLButtonDown(UINT nFlags, CPoint point) 
{
	SetCapture();
	m_bDrag = true;
	
	CWnd::OnLButtonDown(nFlags, point);
}

void Window_Mesh_Previewer::OnLButtonUp(UINT nFlags, CPoint point) 
{
	ReleaseCapture();
	m_bDrag = false;
	
	CWnd::OnLButtonUp(nFlags, point);
}

void Window_Mesh_Previewer::OnViewBackgroundcolor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	cc.rgbResult = 0x00FFFFFF;

	cc.lStructSize		= sizeof( CHOOSECOLOR );
	cc.hwndOwner		= GetSafeHwnd();
	cc.hInstance		= 0;
	cc.lpCustColors		= crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0L;
	cc.lpfnHook			= 0;
	cc.lpTemplateName	= 0;

	if ( !ChooseColor( &cc ) ) 
	{
		return;
	}

	float blue	= (float)((cc.rgbResult & 0x00FF0000) >> 16 ) / 255.0f;
	float green	= (float)((cc.rgbResult & 0x0000FF00) >> 8 ) / 255.0f;
	float red	= (float)( cc.rgbResult & 0x000000FF ) / 255.0f; 
	vector_3 color( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(color);	

	gPreviewer.GetRenderer()->SetClearColor( dcolor );
	gPreferences.SetPreviewColor( dcolor );
}

void Window_Mesh_Previewer::OnPreferencesWireframe() 
{
	gPreferences.SetPreviewWireframe( !gPreferences.GetPreviewWireframe() );
	gPreviewer.GetRenderer()->SetWireframe( gPreferences.GetPreviewWireframe() );
	CheckMenuItem( ::GetMenu( this->GetSafeHwnd() ), ID_PREFERENCES_WIREFRAME, gPreferences.GetPreviewWireframe() ? MF_CHECKED : MF_UNCHECKED );
}

void Window_Mesh_Previewer::OnClose() 
{
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement( &wp );		
	gPreferences.SetPreviewWindowPlacement( wp );
	
	gPreviewer.SetPreviewerWindow( NULL );
	gPreviewer.Deinit();	
	
	ShowWindow( SW_HIDE );

//	CWnd::OnClose();
}

void Window_Mesh_Previewer::CloseForLoad()
{
	OnClose();
}