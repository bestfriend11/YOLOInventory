/**************************************************************************

Filename		: Main.cpp

Description		: Entry point for editor

Creation Date	: 6/30/99

**************************************************************************/


// Include Files
#include "IgnoredWarnings.h"
#include <windows.h>
#include "Main.h"
#include "InitWin.h"
#include "FontExporter.h"
#include "resource.h"
#include <commdlg.h>
#include <assert.h>


// Const Globals
const int font_size = 20;
const unsigned int char_start = 32;
const unsigned int char_end	 = 126;


/**************************************************************************

Function		: WinMain()

Purpose			: Create the main game object and begin execution.

Returns			: Integer value of wParam in received message.

**************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    HWND			hWnd;
    MSG             msg;
	FontExporter	*pFontExporter = 0;	
	
	// Create a new test shell object	
	pFontExporter = new FontExporter();

	// Get the handle to the window
	hWnd	= InitWindow( hInstance, iCmdShow, pFontExporter );
	
	// Initialize the font exporter
	pFontExporter->SetHWnd( hWnd );
	pFontExporter->SetHInstance( hInstance );
	
	// Exporter dialog
	CreateDialogParam( hInstance, MAKEINTRESOURCE(IDD_FONTEXPORT), hWnd, ExporterProc, (LPARAM)pFontExporter );
	
	// Main Message Loop
    while ( TRUE ) {
		if ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ){
			if ( !GetMessage( &msg, NULL, 0, 0 ) )
				return msg.wParam;			   	
			TranslateMessage( &msg ); 
			DispatchMessage ( &msg ); 			
		}
		else {            								
		}
    }
    return msg.wParam;	
}


/**************************************************************************

Function		: WindProc()

Purpose			: Handles all window messages.

Returns			: LRESULT CALLBACK received from DefWindowProc();

**************************************************************************/
LRESULT CALLBACK WindProc( HWND hWnd, UINT Msg, WPARAM wParam,
						   LPARAM  lParam )
{
	static	HINSTANCE		hInstance;
	FontExporter *pFontExporter;
    pFontExporter	= (FontExporter *)GetWindowLong( hWnd, GWL_USERDATA );
	PAINTSTRUCT ps;
						
	switch( Msg ) {
    case    WM_CREATE:
        // This gets us our pTest pointer
		SetWindowLong( hWnd, GWL_USERDATA, 
					   (LONG)((CREATESTRUCT *) lParam )->lpCreateParams );
		hInstance = (HINSTANCE)((CREATESTRUCT *) lParam )->hInstance;
		break;	
	case	WM_PAINT:
		{			
			BeginPaint( hWnd, &ps );
			if ( pFontExporter ) {
				pFontExporter->Draw();
			}	
			EndPaint( hWnd, &ps );
		}
		break;
	case	WM_COMMAND:		
		break;
	case	WM_KEYDOWN:		
		break;
	case    WM_DESTROY:
		PostQuitMessage( 0 );
		break;		
	}
	return DefWindowProc( hWnd, Msg, wParam, lParam );
}


/**************************************************************************

Function		: ExporterProc()

Purpose			: Handles all dialog messages.

Returns			: BOOL CALLBACK

**************************************************************************/
BOOL CALLBACK ExporterProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static	FontExporter *	pFontExporter = 0;
	static	OPENFILENAME	ofn;
	char					szTemp[32] = "";
	
	switch ( msg ) {
	case WM_INITDIALOG:
		{			
			// Get a list of all the fonts and publish them into a combo box
			pFontExporter = (FontExporter *)lParam;	
			std::vector< std::string > font_list = pFontExporter->GetAvailableFonts();
			std::vector< std::string >::iterator i;
			for ( i = font_list.begin(); i != font_list.end(); ++i ) {
				SendMessage( GetDlgItem( hDlg, IDC_FONT_NAMES ), CB_ADDSTRING, 0, (LPARAM)(*i).c_str() );
			}
			ofn = FileInitialize( pFontExporter->GetHWnd(), pFontExporter->GetHInstance() );

			// Set the default values
			pFontExporter->SetFontSize( font_size );
			SetDlgItemInt( hDlg, IDC_FONTSIZE, font_size, true );
			SetDlgItemInt( hDlg, IDC_ASCII_LOW, char_start, false );
			SetDlgItemInt( hDlg, IDC_ASCII_HIGH, char_end, false );
			
			for ( int j = 128; j <= 1024; j *= 2 ) {
				sprintf( szTemp, "%d", j );
				SendMessage( GetDlgItem( hDlg, IDC_WIDTH ), CB_ADDSTRING, 0, (LPARAM)szTemp );
				SendMessage( GetDlgItem( hDlg, IDC_HEIGHT ), CB_ADDSTRING, 0, (LPARAM)szTemp );
			}
			int index = SendMessage( GetDlgItem( hDlg, IDC_WIDTH ), CB_FINDSTRING, 0, (LPARAM)"256" );
			SendMessage( GetDlgItem( hDlg, IDC_WIDTH ), CB_SETCURSEL, index, (LPARAM)0 );
			SendMessage( GetDlgItem( hDlg, IDC_HEIGHT ), CB_SETCURSEL, index, (LPARAM)0 );

			pFontExporter->SetASCIIRange( char_start, char_end );
		}
		return TRUE;
	case WM_COMMAND:
		{				
			switch ( LOWORD( wParam )) {	
			case IDC_RESIZE:
				{
					int width	= 0;
					int height	= 0;
					int index = SendMessage( GetDlgItem( hDlg, IDC_WIDTH ), CB_GETCURSEL, 0, 0 );
					SendMessage( GetDlgItem( hDlg, IDC_WIDTH ), CB_GETLBTEXT, index, (LPARAM)szTemp );
					width = atoi( szTemp );
					index = SendMessage( GetDlgItem( hDlg, IDC_HEIGHT ), CB_GETCURSEL, 0, 0 );
					SendMessage( GetDlgItem( hDlg, IDC_HEIGHT ), CB_GETLBTEXT, index, (LPARAM)szTemp );
					height = atoi( szTemp );	
					BOOL bSuccess = MoveWindow( pFontExporter->GetHWnd(), 0, 0, width+6, height+25, true );
					if ( !bSuccess ) {
						assert( 0 );
					}
					pFontExporter->SetScreenWidth( width );
					pFontExporter->SetScreenHeight( height );

					RECT rect;
					GetClientRect( pFontExporter->GetHWnd(), &rect );
				}
				// Do not return here, instead re-export the font
			case IDOK:
				{
					char szTemp[256] = "";
					GetDlgItemText( hDlg, IDC_SPACING, szTemp, sizeof( szTemp ) );
					pFontExporter->SetSpacing( atoi( szTemp ) );

					char szSize[32] = "";
					GetDlgItemText( hDlg, IDC_FONTSIZE, szSize, sizeof( szSize ) );
					pFontExporter->SetFontSize( atoi( szSize ) );

					char szTypeface[256] = "";
					int select = SendMessage( GetDlgItem( hDlg, IDC_FONT_NAMES ), CB_GETCURSEL, 0, 0 );
					SendMessage( GetDlgItem( hDlg, IDC_FONT_NAMES ), CB_GETLBTEXT, select, (LPARAM)szTypeface );

					bool	bBold		= false;
					bool	bItalic		= false;
					bool	bUnderline	= false;
					int		check		= 0;

					check		= SendMessage( GetDlgItem( hDlg, IDC_BOLD	), BM_GETCHECK, 0, 0 );
					if ( check == BST_CHECKED ) {
						bBold = true;
					}
					check		= SendMessage( GetDlgItem( hDlg, IDC_ITALIC ), BM_GETCHECK, 0, 0 );					
					if ( check == BST_CHECKED ) {
						bItalic = true;
					}
					check		= SendMessage( GetDlgItem( hDlg, IDC_UNDERLINE	), BM_GETCHECK, 0, 0 );
					if ( check == BST_CHECKED ) {
						bUnderline = true;
					}


					BOOL bSuccessLow	= false;
					BOOL bSuccessHigh	= false;
					unsigned int low		= GetDlgItemInt( hDlg, IDC_ASCII_LOW, &bSuccessLow, false );
					unsigned int high		= GetDlgItemInt( hDlg, IDC_ASCII_HIGH, &bSuccessHigh, false );

					if ( low < 1 ) {
						low = 1;
					}
					else if ( low > 255 ) {
						low = 255;
					}
					
					if ( high > 255 ) {
						high = 255;
					}
					else if ( high < 1 ) {
						high = 1;
					}

					pFontExporter->SetASCIIRange( low, high );
					SetDlgItemInt( hDlg, IDC_ASCII_LOW, low, false );
					SetDlgItemInt( hDlg, IDC_ASCII_HIGH, high, false );
					
					if ( select != CB_ERR ) {
						pFontExporter->ExportFont( szTypeface, bBold, bItalic, bUnderline );
					}					
				}
				return TRUE;
			case IDC_SELECT_BACKGROUND_COLOR:
				{
					CHOOSECOLOR cc;
					COLORREF crCustColors[16];
					cc.lStructSize		= sizeof( CHOOSECOLOR );
					cc.hwndOwner		= hDlg;
					cc.hInstance		= 0;
					cc.rgbResult		= RGB( 0x80, 0x80, 0x80 );
					cc.lpCustColors		= crCustColors;
					cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
					cc.lCustData		= 0L;
					cc.lpfnHook			= 0;
					cc.lpTemplateName	= 0;

					ChooseColor( &cc );
					pFontExporter->SetBackgroundColor( cc.rgbResult );
				}
				return TRUE;
			case IDC_SELECT_TEXT_COLOR:
				{
					CHOOSECOLOR cc;
					COLORREF crCustColors[16];
					cc.lStructSize		= sizeof( CHOOSECOLOR );
					cc.hwndOwner		= hDlg;
					cc.hInstance		= 0;
					cc.rgbResult		= RGB( 0x80, 0x80, 0x80 );
					cc.lpCustColors		= crCustColors;
					cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
					cc.lCustData		= 0L;
					cc.lpfnHook			= 0;
					cc.lpTemplateName	= 0;

					ChooseColor( &cc );
					pFontExporter->SetFontColor( cc.rgbResult );
				}
				return TRUE;			
			case IDCANCEL:				
				SendMessage( pFontExporter->GetHWnd(), WM_CLOSE, 0, 0 );
				EndDialog( hDlg, 0 );
				return TRUE;
			case IDC_SAVE_FONT:
				{
					int	check = 0;
					bool bCheck = false;
					check		= SendMessage( GetDlgItem( hDlg, IDC_CHECK_SMOOTHING ), BM_GETCHECK, 0, 0 );
					if ( check == BST_CHECKED ) {
						pFontExporter->SetSmooth( true );
					}
					else
					{
						pFontExporter->SetSmooth( false );
					}

					bool bContinue = true;
					while ( bContinue ) {
						char szFile[256] = "";
						char szFull[256] = "";
						if ( FileSaveDlg( hDlg, szFile, szFull, &ofn ) == 0 ) {							
							break;
						}
						bContinue = pFontExporter->SaveFont( szFile );
					}

				}
				return TRUE;			
			}			
		}
		return TRUE;
	}
	return FALSE;
}


/**************************************************************************

Function		: FileInitialize()

Purpose			: Initializes the Common Dialog File boxes

Returns			: void

**************************************************************************/
OPENFILENAME FileInitialize( HWND hWnd, HINSTANCE hInstance )
{
	static char szFilter[]	=	"BMP Files (*.bmp)\0*.bmp\0"  \
								"All Files (*.*)\0*.*\0\0";
	OPENFILENAME ofn;

	ofn.lStructSize			= sizeof( OPENFILENAME );
	ofn.hwndOwner			= hWnd;
	ofn.hInstance			= hInstance;
	ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= (char *)malloc( 256 );
	ofn.nMaxFile			= _MAX_PATH;
	ofn.lpstrFileTitle		= (char *)malloc( 256 );
	ofn.nMaxFileTitle		= _MAX_FNAME + _MAX_EXT;
	ofn.lpstrInitialDir		= NULL;
	ofn.lpstrTitle			= NULL;
	ofn.Flags				= 0;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= "bmp";
	ofn.lCustData			= 0L;
	ofn.lpfnHook			= NULL;
	ofn.lpTemplateName		= NULL;

	return ofn;
}


/**************************************************************************

Function		: FileSaveDlg()

Purpose			: Initializes the File Save dialog box

Returns			: void

**************************************************************************/
BOOL FileSaveDlg( HWND hWnd, PSTR pstrFilename, PSTR pstrTitlename,
				  OPENFILENAME *ofn )
{
	ofn->hwndOwner		= hWnd;
	ofn->lpstrFile		= pstrFilename;
	ofn->lpstrFileTitle	= pstrTitlename;
	ofn->Flags			= OFN_OVERWRITEPROMPT;

	return GetSaveFileName( ofn );
}