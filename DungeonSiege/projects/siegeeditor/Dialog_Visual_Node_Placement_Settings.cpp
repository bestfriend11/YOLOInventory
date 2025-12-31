// Dialog_Visual_Node_Placement_Settings.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Visual_Node_Placement_Settings.h"
#include "EditorTerrain.h"
#include "Preferences.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Visual_Node_Placement_Settings dialog


Dialog_Visual_Node_Placement_Settings::Dialog_Visual_Node_Placement_Settings(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Visual_Node_Placement_Settings::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Visual_Node_Placement_Settings)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Visual_Node_Placement_Settings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Visual_Node_Placement_Settings)
	DDX_Control(pDX, IDC_CHECK_DRAW_DOORS, m_DrawDoors);
	DDX_Control(pDX, IDC_CHECK_DOOR_GUESS, m_DoorGuess);
	DDX_Control(pDX, IDC_EDIT_SNAP_DELAY, m_SnapDelay);
	DDX_Control(pDX, IDC_CHECK_ROTATE_ORIENT, m_RotateOrient);
	DDX_Control(pDX, IDC_STATIC_VALID_COLOR, m_ValidColor);
	DDX_Control(pDX, IDC_STATIC_INVALID_COLOR, m_InvalidColor);
	DDX_Control(pDX, IDC_STATIC_DESTINATION_DOOR_COLOR, m_DestinationDoorColor);
	DDX_Control(pDX, IDC_EDIT_SNAP_SENSITIVITY, m_SnapSensitivity);
	DDX_Control(pDX, IDC_CHECK_DRAW_MESH_BOX, m_DrawMeshBox);
	DDX_Control(pDX, IDC_CHECK_DRAW_DOOR_NUMBERS, m_DrawDoorNumbers);
	DDX_Control(pDX, IDC_CHECK_DRAW_DEST_ID, m_DrawDestId);
	DDX_Control(pDX, IDC_CHECK_ALLOW_INVALID_DOOR_MATCH, m_AllowInvalidMatch);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Visual_Node_Placement_Settings, CDialog)
	//{{AFX_MSG_MAP(Dialog_Visual_Node_Placement_Settings)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_CHECK_DRAW_DEST_ID, OnCheckDrawDestId)
	ON_BN_CLICKED(IDC_CHECK_DRAW_DOOR_NUMBERS, OnCheckDrawDoorNumbers)
	ON_BN_CLICKED(IDC_CHECK_DRAW_MESH_BOX, OnCheckDrawMeshBox)
	ON_BN_CLICKED(IDC_BUTTON_VALID_COLOR, OnButtonValidColor)
	ON_BN_CLICKED(IDC_BUTTON_INVALID_COLOR, OnButtonInvalidColor)
	ON_BN_CLICKED(IDC_BUTTON_DESTINATION_ID_COLOR, OnButtonDestinationIdColor)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_SNAP, OnButtonDefaultSnap)
	ON_BN_CLICKED(IDC_CHECK_ALLOW_INVALID_DOOR_MATCH, OnCheckAllowInvalidDoorMatch)
	ON_BN_CLICKED(IDC_CHECK_ROTATE_ORIENT, OnCheckRotateOrient)
	ON_EN_CHANGE(IDC_EDIT_SNAP_SENSITIVITY, OnChangeEditSnapSensitivity)
	ON_EN_CHANGE(IDC_EDIT_SNAP_DELAY, OnChangeEditSnapDelay)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_SNAP_DELAY, OnButtonDefaultSnapDelay)
	ON_BN_CLICKED(IDC_CHECK_DRAW_DOORS, OnCheckDrawDoors)
	ON_BN_CLICKED(IDC_CHECK_DOOR_GUESS, OnCheckDoorGuess)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Visual_Node_Placement_Settings message handlers

BOOL Dialog_Visual_Node_Placement_Settings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gpstring sTemp;
	sTemp.assignf( "%f", gPreferences.GetVisualSnapSensitivity() );
	m_SnapSensitivity.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetVisualSnapDelay() );
	m_SnapDelay.SetWindowText( sTemp.c_str() );

	m_DrawMeshBox.SetCheck( gPreferences.GetVisualDrawMeshBox() ? TRUE : FALSE );
	m_DrawDoorNumbers.SetCheck( gPreferences.GetVisualDrawDoorNumbers() ? TRUE : FALSE );
	m_DrawDestId.SetCheck( gPreferences.GetVisualDrawDestId() ? TRUE : FALSE );
	m_AllowInvalidMatch.SetCheck( gPreferences.GetVisualAllowInvalidMatch() ? TRUE : FALSE );
	m_RotateOrient.SetCheck( gPreferences.GetVisualRotateOrient() ? TRUE : FALSE );
	m_DrawDoors.SetCheck( gPreferences.GetVisualDrawDoors() ? TRUE : FALSE );
	m_DoorGuess.SetCheck( gPreferences.GetVisualAllowDoorGuess() ? TRUE : FALSE );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT Dialog_Visual_Node_Placement_Settings::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( message == WM_CTLCOLORSTATIC ) 
	{		
		int id = GetWindowLong( (HWND)lParam, GWL_ID );
		if ( id == IDC_STATIC_VALID_COLOR ) 
		{						
			if ( m_hBrushValid ) 
			{
				DeleteObject( m_hBrushValid );
			}

			BYTE rb, gb, bb = 0;
			rb = (unsigned char)( (gPreferences.GetVisualValidColor() & 0x00ff0000) >> 16 );
			gb = (unsigned char)( (gPreferences.GetVisualValidColor() & 0x0000ff00) >> 8 );
			bb = (unsigned char)( (gPreferences.GetVisualValidColor() & 0x000000ff));
			COLORREF colorref = RGB( rb, gb, bb );	

			m_hBrushValid = CreateSolidBrush( colorref );

			return (LRESULT)m_hBrushValid;
		}
		else if ( id == IDC_STATIC_INVALID_COLOR )
		{
			if ( m_hBrushInvalid ) 
			{
				DeleteObject( m_hBrushInvalid );
			}

			BYTE rb, gb, bb = 0;
			rb = (unsigned char)( (gPreferences.GetVisualInvalidColor() & 0x00ff0000) >> 16 );
			gb = (unsigned char)( (gPreferences.GetVisualInvalidColor() & 0x0000ff00) >> 8 );
			bb = (unsigned char)( (gPreferences.GetVisualInvalidColor() & 0x000000ff));
			COLORREF colorref = RGB( rb, gb, bb );	

			m_hBrushInvalid = CreateSolidBrush( colorref );

			return (LRESULT)m_hBrushInvalid;
		}
		else if ( id == IDC_STATIC_DESTINATION_DOOR_COLOR )
		{
			if ( m_hBrushDest ) 
			{
				DeleteObject( m_hBrushDest );
			}

			BYTE rb, gb, bb = 0;
			rb = (unsigned char)( (gPreferences.GetVisualDestDoorColor() & 0x00ff0000) >> 16 );
			gb = (unsigned char)( (gPreferences.GetVisualDestDoorColor() & 0x0000ff00) >> 8 );
			bb = (unsigned char)( (gPreferences.GetVisualDestDoorColor() & 0x000000ff));
			COLORREF colorref = RGB( rb, gb, bb );	

			m_hBrushDest = CreateSolidBrush( colorref );

			return (LRESULT)m_hBrushDest;
		}
	}
	
	return CDialog::WindowProc(message, wParam, lParam);
}

void Dialog_Visual_Node_Placement_Settings::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);				
}

void Dialog_Visual_Node_Placement_Settings::OnCheckDrawDestId() 
{
	gPreferences.SetVisualDrawDestId( m_DrawDestId.GetCheck() ? true : false );	
}

void Dialog_Visual_Node_Placement_Settings::OnCheckDrawDoorNumbers() 
{
	gPreferences.SetVisualDrawDoorNumbers( m_DrawDoorNumbers.GetCheck() ? true : false );
}

void Dialog_Visual_Node_Placement_Settings::OnCheckDrawMeshBox() 
{
	gPreferences.SetVisualDrawMeshBox( m_DrawMeshBox.GetCheck() ? true : false );	
}

void Dialog_Visual_Node_Placement_Settings::OnButtonValidColor() 
{
	gPreferences.SetVisualValidColor( GetColor( gPreferences.GetVisualValidColor() ) );		
	m_ValidColor.Invalidate( TRUE );
}

void Dialog_Visual_Node_Placement_Settings::OnButtonInvalidColor() 
{
	gPreferences.SetVisualInvalidColor( GetColor( gPreferences.GetVisualInvalidColor() ) );		
	m_InvalidColor.Invalidate( TRUE );	
}

void Dialog_Visual_Node_Placement_Settings::OnButtonDestinationIdColor() 
{
	gPreferences.SetVisualDestDoorColor( GetColor( gPreferences.GetVisualDestDoorColor() ) );		
	m_DestinationDoorColor.Invalidate( TRUE );	
}

void Dialog_Visual_Node_Placement_Settings::OnButtonDefaultSnap() 
{
	gPreferences.SetVisualSnapSensitivity( 1.0f );
	gpstring sTemp;
	sTemp.assignf( "%f", gPreferences.GetVisualSnapSensitivity() );
	m_SnapSensitivity.SetWindowText( sTemp.c_str() );		
}

void Dialog_Visual_Node_Placement_Settings::OnCheckAllowInvalidDoorMatch() 
{
	gPreferences.SetVisualAllowInvalidMatch( m_AllowInvalidMatch.GetCheck()	? true : false );
}

DWORD Dialog_Visual_Node_Placement_Settings::GetColor( DWORD oldColor )
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	// Convert dword color to colorref	
	BYTE rb, gb, bb = 0;
	rb = (unsigned char)( (oldColor & 0x00ff0000) >> 16 );
	gb = (unsigned char)( (oldColor & 0x0000ff00) >> 8 );
	bb = (unsigned char)( (oldColor & 0x000000ff));
	COLORREF colorref = RGB( rb, gb, bb );	

	cc.rgbResult		= colorref;
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
		return oldColor;
	}
	
	float blue	= (float)(GetBValue(cc.rgbResult) ) / 255.0f;
	float green	= (float)(GetGValue(cc.rgbResult) ) / 255.0f;
	float red	= (float)(GetRValue(cc.rgbResult) ) / 255.0f; 
	vector_3 vcolor( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(vcolor);	
	
	SetCustomColors( crCustColors );

	return dcolor;
}

void Dialog_Visual_Node_Placement_Settings::OnCheckRotateOrient() 
{
	gPreferences.SetVisualRotateOrient( m_RotateOrient.GetCheck() ? true : false );
}

void Dialog_Visual_Node_Placement_Settings::OnChangeEditSnapSensitivity() 
{
	CString rValue;
	m_SnapSensitivity.GetWindowText( rValue );
	if ( rValue.IsEmpty() )
	{
		return;
	}

	float value;
	stringtool::Get( rValue.GetBuffer( 0 ), value );

	gPreferences.SetVisualSnapSensitivity( value );	
}

void Dialog_Visual_Node_Placement_Settings::OnChangeEditSnapDelay() 
{
	CString rValue;
	m_SnapDelay.GetWindowText( rValue );
	if ( rValue.IsEmpty() )
	{
		return;
	}

	float value;
	stringtool::Get( rValue.GetBuffer( 0 ), value );

	gPreferences.SetVisualSnapDelay( value );		
}

void Dialog_Visual_Node_Placement_Settings::OnButtonDefaultSnapDelay() 
{
	gPreferences.SetVisualSnapDelay( 0.2f );
	gpstring sTemp;
	sTemp.assignf( "%f", gPreferences.GetVisualSnapDelay() );
	m_SnapDelay.SetWindowText( sTemp.c_str() );			
}

void Dialog_Visual_Node_Placement_Settings::OnCheckDrawDoors() 
{
	gPreferences.SetVisualDrawDoors( m_DrawDoors.GetCheck() ? true : false );	
}

void Dialog_Visual_Node_Placement_Settings::OnCheckDoorGuess() 
{
	gPreferences.SetVisualAllowDoorGuess( m_DoorGuess.GetCheck() ? true : false );	
}
