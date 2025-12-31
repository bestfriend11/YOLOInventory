//==========================================================================;
//
//		Hyper cheesy Rapi test application
//
//		Author:  James Loe
//
//**************************************************************************/

#include "ignored_warnings.h"

#include <windows.h>

#include <objbase.h>
#include <initguid.h>

#include <vector>
#include <list>
#include <map>
#include <set>
#include "smart_pointer.h"

#include "GPGlobal.h"
#include "TNK.h"
#include "GAS.h"
#include "GASHandle.h"
#include "GASDefaultTypes.h"
#include "GASFileParser.h"
#include "OCTANE.h"
#include "OCTANE_Default_Algorithms.h"
#include "IMG.h"
#include "TNKHandle.h"
#include "GPString.h"
#include "vector_3.h"
#include "space_3.h"

#include "RapiOwner.h"


HWND hwndMain;
HINSTANCE hInstance;
RapiOwner *pOwner  = NULL;
bool bFullScreen		= false;

matrix_3x3	mat;
float angle				= 0.0f;
RapiLight *pLight;

sVertex *pVerts;

RECT	rWindowRect = {0, 0, 640, 480};
int		BPP			= 16;

class App
{
public:

	smart_pointer< GAS > mGAS;
	smart_pointer< GAS_Default_Types > mGASDefaultTypes;
	smart_pointer< OCTANE > mOCTANE;
	smart_pointer< OCTANE_Default_Algorithms > mOCTANE_Default_Algorithms;
	smart_pointer< TNK > mFilesystem;
};

App m_app;
RapiStaticObject *pObject;


LRESULT WINAPI
WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT r;
    switch (msg)
    {
        case WM_CREATE:
			if( pOwner ){
				pOwner->PrepareToDraw( hwnd, bFullScreen, rWindowRect.right, rWindowRect.bottom, BPP );
				Rapi *pDevice = pOwner->GetDevice( 0 );
				if( !pDevice )
					break;

				unsigned int tex = pDevice->CreateTexture( *m_app.mFilesystem, "art\\terrain.bmp", true, 0, TEX_LOCKED );
				smart_pointer< sVertex > pVertices;
				pVerts	= new sVertex[4];
				pVertices.TransferAndOwn( pVerts );

				{
					// Create two lights, one green one on the left, and one red one on the right
					LCOLOR color;
					color.r	= 0.0f;
					color.g = 1.0f;
					color.b = 0.0f;
					color.a = 1.0f;

					pLight = pDevice->CreateLight(	D3DLIGHT_POINT, color, 
											vector_3( -32.0f, 128.0f, 0.0f ),
											vector_3( 0.0f, 0.0f, 0.0f ),
											384.0f );

					color.r = 1.0f;
					color.g = 0.0f;
					color.b = 0.0f;
					color.a = 1.0f;

					pDevice->CreateLight(	D3DLIGHT_POINT, color, 
											vector_3( 288.0f, 128.0f, 0.0f ),
											vector_3( 0.0f, 0.0f, 0.0f ),
											384.0f );

					pDevice->SetAmbientLight( 0x40404040 );
				}

				pVerts[0].x			= 0.0f;
				pVerts[0].y			= 0.0f;
				pVerts[0].z			= 0.0f;
				pVerts[0].nx		= 0.0f;
				pVerts[0].ny		= 0.0f;
				pVerts[0].nz		= -1.0f;
				pVerts[0].color		= 0x00FF0000;
				pVerts[0].uv[0].u	= 0.0f;
				pVerts[0].uv[0].v	= 0.0f;

				pVerts[1].x			= 0.0f;
				pVerts[1].y			= 256.0f;
				pVerts[1].z			= 0.0f;
				pVerts[1].nx		= 0.0f;
				pVerts[1].ny		= 0.0f;
				pVerts[1].nz		= -1.0f;
				pVerts[1].color		= 0xFFFFFFFF;
				pVerts[1].uv[0].u	= 0.0f;
				pVerts[1].uv[0].v	= 1.0f;

				pVerts[2].x			= 256.0f;
				pVerts[2].y			= 256.0f;
				pVerts[2].z			= 0.0f;
				pVerts[2].nx		= 0.0f;
				pVerts[2].ny		= 0.0f;
				pVerts[2].nz		= -1.0f;
				pVerts[2].color		= 0xFFFFFFFF;
				pVerts[2].uv[0].u	= 1.0f;
				pVerts[2].uv[0].v	= 1.0f;

				pVerts[3].x			= 256.0f;
				pVerts[3].y			= 0.0f;
				pVerts[3].z			= 0.0f;
				pVerts[3].nx		= 0.0f;
				pVerts[3].ny		= 0.0f;
				pVerts[3].nz		= -1.0f;
				pVerts[3].color		= 0xFFFFFFFF;
				pVerts[3].uv[0].u	= 1.0f;
				pVerts[3].uv[0].v	= 0.0f;

				smart_pointer< sTriangle > pTriangles;
				sTriangle *pTris = new sTriangle[2];
				pTriangles.TransferAndOwn( pTris );
				pTris[0].tIndex[0] = 0;
				pTris[0].vIndex[0] = 0;
				pTris[0].vIndex[1] = 1;
				pTris[0].vIndex[2] = 2;
				pTris[1].tIndex[0] = 0;
				pTris[1].vIndex[0] = 0;
				pTris[1].vIndex[1] = 2;
				pTris[1].vIndex[2] = 3;

				std::vector< unsigned int > texlist;
				texlist.push_back( tex );

				pObject = new RapiStaticObject(	pDevice, 
												pVertices, 4,
												pTriangles, 2,
												texlist );

				pObject->Optimize();
			}
            return 0;
        case WM_DESTROY:
            if ( pOwner )
                delete pOwner;
            return 0;
        case WM_SETCURSOR:
            if ( pOwner && pOwner->IsFullScreen() )
            {
                SetCursor( NULL );
                return 1;
            }
            break;
		case WM_SYSKEYDOWN:
			if( wparam == VK_RETURN ){
				if( GetKeyState( VK_MENU ) & 0x8000 ){
					if( pOwner->IsFullScreen() ){
						pOwner->StopDrawing();
						pOwner->PrepareToDraw( hwnd, false );
						SetWindowPos(	hwnd, 
										HWND_NOTOPMOST,
										rWindowRect.left,
										rWindowRect.top,
										rWindowRect.right-rWindowRect.left,
										rWindowRect.bottom-rWindowRect.top,
										SWP_NOACTIVATE );
					}
					else{
						GetWindowRect( hwnd, &rWindowRect );
						pOwner->StopDrawing();
						pOwner->PrepareToDraw( hwnd, true, rWindowRect.right, rWindowRect.bottom, BPP );
					}
				}
			}
			break;
        case WM_SIZE:
            if ( pOwner && !pOwner->IsFullScreen() )
            {
                GetClientRect( hwnd, &r );
                if ( SIZE_MINIMIZED == wparam || r.top == r.bottom )
                {
                    pOwner->Pause( true );
                    return 0;
                }
                pOwner->Pause( false );
                pOwner->StopDrawing();
                pOwner->PrepareToDraw( hwnd, false );
                break;
            }
        case WM_MOVE:
        case WM_DISPLAYCHANGE:
            if (pOwner && pOwner->IsFullScreen() && !IsIconic(hwnd))
            {
                RECT r;
                SetRect(&r, 0, GetSystemMetrics(SM_CYCAPTION), GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                AdjustWindowRectEx(&r, WS_POPUP | WS_CAPTION, FALSE, 0);
                SetWindowPos(hwnd, NULL, r.left, r.top, r.right-r.left, r.bottom-r.top, SWP_NOACTIVATE | SWP_NOZORDER);
            }
            break;
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

static BOOL
InitApp(HINSTANCE hInstance)
{
    WNDCLASS wndclass;
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = (WNDPROC) WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(void *);
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = (HICON) NULL;
    wndclass.hCursor       = (HCURSOR) NULL;
    wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = "TestApp";
    if (! RegisterClass(&wndclass) )
    {
        return FALSE;
    }

	//----- GAS
	m_app.mGAS.TransferAndOwn( new GAS );
	m_app.mGASDefaultTypes.TransferAndOwn( new GAS_Default_Types( *(m_app.mGAS.GetPointer()) ) );

	//----- OCTANE
	m_app.mOCTANE.TransferAndOwn( new OCTANE );
	m_app.mOCTANE_Default_Algorithms.TransferAndOwn( new OCTANE_Default_Algorithms( *(m_app.mOCTANE.GetPointer()) ) );

	//----- TNK
	m_app.mFilesystem.TransferAndOwn( new TNK( *(m_app.mGAS.GetPointer()), *(m_app.mOCTANE_Default_Algorithms.GetPointer()) ));
	GPString Execution_Home = "."; // Get the home directory for the app
	char dir[256];
	GetCurrentDirectory( 256, dir );
	m_app.mFilesystem->AddFiles( dir );

	pOwner = new RapiOwner();
	if( pOwner->Initialize() != S_OK )
		return FALSE;

	return TRUE;
}

int APIENTRY
WinMain( HINSTANCE hinst, 
         HINSTANCE hPrevInstance,
         LPSTR lpCmdLine,
         int nCmdShow )
{
    HRESULT hr;

    // initialize COM
    if (FAILED(hr = CoInitialize(NULL)))
        return FALSE;


    /* Register the window class for the main window. */ 
 
    if (!hPrevInstance) { 
        if (! InitApp(hinst) )
            return FALSE; 
    } 
 
    hInstance = hinst;

    /* Create the main window. */ 
    hwndMain = CreateWindow("TestApp", "3D Test App", 
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT, (HWND) NULL, 
        (HMENU) NULL, hInstance, (LPVOID) NULL); 
 
    /* 
     * If the main window cannot be created, terminate 
     * the application. 
     */ 
 
    if (!hwndMain) 
        return FALSE; 

        /* Show the window and paint its contents. */ 
 
    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain); 

        /* Start the message loop. */ 

	MSG msg;
	while( 1 ){
		if( PeekMessage( &msg, hwndMain, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
			
			if( msg.message == WM_QUIT )
				break;
		}

        if ( pOwner && !pOwner->IsPaused() )
        {
			Rapi *pDevice = pOwner->GetDevice( 0 );
			if( !pDevice )
				break;

			pDevice->SetPerspectiveMatrix( RadiansFrom( 90 ) );
			pDevice->SetViewMatrix( vector_3( 128.0f, 128.0f, -200.0f ),
									vector_3( 128.0f, 128.0f, 0.0f ),
									vector_3( 0.0f, 1.0f, 0.0f ) );

			pDevice->SetWorldMatrix( mat, vector_3( 0, 0, 0 ) );
			pDevice->TranslateWorldMatrix( vector_3( 128.0f, 128.0f, 0.0f ) );
			pDevice->RotateWorldMatrix( YRotationColumns( angle )*XRotationColumns( angle ) );
			pDevice->ScaleWorldMatrix( vector_3( 0.5f, 0.5f, 0.5f ) );
			pDevice->TranslateWorldMatrix( vector_3( -128.0f, -128.0f, 0.0f ) );

			pDevice->Begin3D( );

			pDevice->SetTextureStageState(	0,
											D3DTOP_MODULATE,
											D3DTOP_SELECTARG1,
											D3DTADDRESS_CLAMP,
											D3DTADDRESS_CLAMP,
											D3DTFG_LINEAR,
											D3DTFN_LINEAR,
											D3DTFP_LINEAR,
											D3DBLEND_SRCALPHA,
											D3DBLEND_INVSRCALPHA,
											false );
			pObject->Render();

			pDevice->End3D( );

			angle += 0.01f;

        }
	}
 
    DestroyWindow( hwndMain );
    /* Return the exit code to Windows. */ 

    // uninitialize COM
    CoUninitialize();
 
    return msg.wParam; 
}
