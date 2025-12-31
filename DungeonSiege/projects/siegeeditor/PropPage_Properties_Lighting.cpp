// PropPage_Properties_Lighting.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Lighting.h"
#include "RapiOwner.h"
#include "ColorPickerDlg.h"
#include "EditorLights.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Lighting property page

IMPLEMENT_DYNCREATE(PropPage_Properties_Lighting, CResizablePage)

PropPage_Properties_Lighting::PropPage_Properties_Lighting() : CResizablePage(PropPage_Properties_Lighting::IDD)
{
	m_bApply = false;
	//{{AFX_DATA_INIT(PropPage_Properties_Lighting)
	//}}AFX_DATA_INIT
}

PropPage_Properties_Lighting::~PropPage_Properties_Lighting()
{
}

void PropPage_Properties_Lighting::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Lighting)
	DDX_Control(pDX, IDC_EDIT_GUID, m_edit_guid);
	DDX_Control(pDX, IDC_EDIT_ORADIUS, m_edit_oradius);
	DDX_Control(pDX, IDC_EDIT_IRADIUS, m_edit_iradius);
	DDX_Control(pDX, IDC_EDIT_INTENSITY, m_edit_intensity);
	DDX_Control(pDX, IDC_EDIT_DIR_Z, m_edit_z);
	DDX_Control(pDX, IDC_EDIT_DIR_Y, m_edit_y);
	DDX_Control(pDX, IDC_EDIT_DIR_X, m_edit_x);
	DDX_Control(pDX, IDC_EDIT_COLOR_RED, m_edit_red);
	DDX_Control(pDX, IDC_EDIT_COLOR_GREEN, m_edit_green);
	DDX_Control(pDX, IDC_EDIT_COLOR_BLUE, m_edit_blue);
	DDX_Control(pDX, IDC_EDIT_COLOR, m_edit_color);
	DDX_Control(pDX, IDC_CHECK_ONTIMER, m_on_timer);
	DDX_Control(pDX, IDC_CHECK_OCCLUDE, m_occlude);
	DDX_Control(pDX, IDC_CHECK_ACTIVE, m_active);
	DDX_Control(pDX, IDC_CHECK_DRAWSHADOWS, m_draw_shadows);
	DDX_Control(pDX, IDC_CHECK_AFFECT_TERRAIN, m_affect_terrain);
	DDX_Control(pDX, IDC_CHECK_AFFECT_ITEMS, m_affect_items);
	DDX_Control(pDX, IDC_CHECK_AFFECT_ACTORS, m_affect_actors);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Lighting, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Lighting)
	ON_BN_CLICKED(IDC_BUTTON_COLOR_STANDARD, OnButtonColorStandard)
	ON_BN_CLICKED(IDC_BUTTON_COLOR_ADVANCED, OnButtonColorAdvanced)
	ON_EN_CHANGE(IDC_EDIT_COLOR_RED, OnChangeEditColorRed)
	ON_EN_CHANGE(IDC_EDIT_COLOR_GREEN, OnChangeEditColorGreen)
	ON_EN_CHANGE(IDC_EDIT_COLOR_BLUE, OnChangeEditColorBlue)
	ON_EN_CHANGE(IDC_EDIT_DIR_X, OnChangeEditDirX)
	ON_EN_CHANGE(IDC_EDIT_DIR_Y, OnChangeEditDirY)
	ON_EN_CHANGE(IDC_EDIT_DIR_Z, OnChangeEditDirZ)
	ON_EN_CHANGE(IDC_EDIT_IRADIUS, OnChangeEditIradius)
	ON_EN_CHANGE(IDC_EDIT_ORADIUS, OnChangeEditOradius)
	ON_EN_CHANGE(IDC_EDIT_INTENSITY, OnChangeEditIntensity)
	ON_BN_CLICKED(IDC_CHECK_OCCLUDE, OnCheckOcclude)
	ON_BN_CLICKED(IDC_CHECK_DRAWSHADOWS, OnCheckDrawshadows)
	ON_BN_CLICKED(IDC_CHECK_AFFECT_ITEMS, OnCheckAffectItems)
	ON_BN_CLICKED(IDC_CHECK_ONTIMER, OnCheckOntimer)
	ON_BN_CLICKED(IDC_CHECK_ACTIVE, OnCheckActive)
	ON_BN_CLICKED(IDC_CHECK_AFFECT_ACTORS, OnCheckAffectActors)
	ON_BN_CLICKED(IDC_CHECK_AFFECT_TERRAIN, OnCheckAffectTerrain)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Lighting message handlers

void PropPage_Properties_Lighting::OnButtonColorStandard() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	{		
		vector_3 color;
		CString rTemp;
		GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
		color.x = atof( rTemp );
		GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
		color.y = atof( rTemp );
		GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
		color.z = atof( rTemp );

		cc.rgbResult = MAKEDWORDCOLOR(color) & 0x00FFFFFF;
	}

	cc.lStructSize		= sizeof( CHOOSECOLOR );
	cc.hwndOwner		= GetSafeHwnd();
	cc.hInstance		= 0;
	cc.lpCustColors		= crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0L;
	cc.lpfnHook			= 0;
	cc.lpTemplateName	= 0;

	if ( !ChooseColor( &cc ) ) {
		return;
	}

	float blue	= (float)((cc.rgbResult & 0x00FF0000) >> 16 ) / 255.0f;
	float green	= (float)((cc.rgbResult & 0x0000FF00) >> 8 ) / 255.0f;
	float red	= (float)( cc.rgbResult & 0x000000FF ) / 255.0f; 
	vector_3 color( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dcolor );				
	
	gpstring sTemp;
	sTemp.assignf( "%f", color.x );
	m_edit_red.SetWindowText( sTemp.c_str() );
	sTemp.assignf( "%f", color.y );
	m_edit_green.SetWindowText( sTemp.c_str() );
	sTemp.assignf( "%f", color.z );
	m_edit_blue.SetWindowText( sTemp.c_str() );	 
	
	SetCustomColors( crCustColors );	
	m_edit_color.Invalidate( TRUE );	
}

void PropPage_Properties_Lighting::OnButtonColorAdvanced() 
{
	DWORD dwColor = 0;
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.z = atof( rTemp );
	dwColor = MAKEDWORDCOLOR(color);

	CColorPickerDlg colorDlg( dwColor );
	if ( colorDlg.DoModal() == IDOK ) {
		COLORREF rgbResult = colorDlg.GetColor();

		float blue	= (float)((rgbResult & 0x00FF0000) >> 16 ) / 255.0f;
		float green	= (float)((rgbResult & 0x0000FF00) >> 8 ) / 255.0f;
		float red	= (float)( rgbResult & 0x000000FF ) / 255.0f; 
		vector_3 color( red, green, blue );
		DWORD dcolor = MAKEDWORDCOLOR(color);
		gEditorLights.SetColor( dcolor );				
		
		gpstring sTemp;
		sTemp.assignf( "%f", color.x );
		m_edit_red.SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.y );
		m_edit_green.SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.z );
		m_edit_blue.SetWindowText( sTemp.c_str() );	 		
	}
	m_edit_color.Invalidate( TRUE );
}

BOOL PropPage_Properties_Lighting::OnInitDialog() 
{
	CResizablePage::OnInitDialog();
	
	/*
	AddAnchor( IDC_EDIT_ORADIUS, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_IRADIUS, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_INTENSITY, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_DIR_X, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_DIR_Y, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_DIR_Z, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_COLOR_RED, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_COLOR_GREEN, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_COLOR_BLUE, MIDDLE_CENTER );
	AddAnchor( IDC_EDIT_COLOR, MIDDLE_CENTER );	
	AddAnchor( IDC_CHECK_ACTIVE, MIDDLE_CENTER );
	AddAnchor( IDC_CHECK_DRAWSHADOWS, MIDDLE_CENTER );
	
	AddAnchor( IDC_CHECK_OCCLUDE, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_DIRECTION, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_SETTINGS, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_IRADIUS, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_GREEN, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_ORADIUS, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_INTENSITY, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_RED, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_GREEN, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_BLUE, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_VALUES, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_FINAL, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_OPTIONS, MIDDLE_CENTER );
	AddAnchor( IDC_BUTTON_COLOR_STANDARD, MIDDLE_CENTER );
	AddAnchor( IDC_BUTTON_COLOR_ADVANCED, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_X, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_Y, MIDDLE_CENTER );
	AddAnchor( IDC_STATIC_Z, MIDDLE_CENTER );

	AddAnchor( IDC_CHECK_ONTIMER, MIDDLE_CENTER );	
	AddAnchor( IDC_CHECK_AFFECT_TERRAIN, MIDDLE_CENTER );	
	AddAnchor( IDC_CHECK_AFFECT_ITEMS, MIDDLE_CENTER );
	AddAnchor( IDC_CHECK_AFFECT_ACTORS, MIDDLE_CENTER );
	*/

	Reset();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void SetCustomColors( COLORREF *colors )
{
	char szWindows[256] = "";
	GetWindowsDirectory( szWindows, sizeof( szWindows ) );
	char szFile[256] = "siege_colors.bin";
	char szFullFile[256] = "";
	sprintf( szFullFile, "%s\\%s", szWindows, szFile );
	FILE *f = NULL;
	f = fopen( szFullFile, "wb" );
	if ( !f ) {
		return;
	}
	fwrite( colors, sizeof( colors ), 16, f );
	fclose( f );
}


void GetCustomColors( COLORREF *colors )
{
	char szWindows[256] = "";
	GetWindowsDirectory( szWindows, sizeof( szWindows ) );
	char szFile[256] = "siege_colors.bin";
	char szFullFile[256] = "";
	sprintf( szFullFile, "%s\\%s", szWindows, szFile );
	FILE *f = NULL;
	f = fopen( szFullFile, "rb" );
	if ( !f ) {
		return;
	}
	fread( colors, sizeof( colors ), 16, f );
	fclose( f );	
}

LRESULT PropPage_Properties_Lighting::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( message == WM_CTLCOLORSTATIC ) 
	{		
		int id = GetWindowLong( (HWND)lParam, GWL_ID );
		if ( id == IDC_EDIT_COLOR ) 
		{
			double r = 0;
			double g = 0;
			double b = 0;
			CString rTemp;
			GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
			r = atof( rTemp );
			GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );
			g = atof( rTemp );
			GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );				
			b = atof( rTemp );
			BYTE rb, gb, bb = 0;
			rb = (unsigned char)(r * 255);
			gb = (unsigned char)(g * 255);
			bb = (unsigned char)(b * 255);
			COLORREF colorref = RGB( rb, gb, bb );				
			if ( m_hBrush ) {
				DeleteObject( m_hBrush );
			}
			m_hBrush = CreateSolidBrush( colorref );
			return (LRESULT)m_hBrush;
		}
	}	
	
	return CResizablePage::WindowProc(message, wParam, lParam);
}


void PropPage_Properties_Lighting::Reset()
{
	if ( !GetSafeHwnd() )
	{
		return;
	}

	m_edit_guid.SetWindowText( "" );
	std::vector< DWORD > light_ids;	
	gGizmoManager.GetSelectedLightIDs( light_ids );
	if ( light_ids.size() == 1 )
	{
		gpstring sGuid;
		sGuid.assignf( "0x%08X", *(light_ids.begin()) );
		m_edit_guid.SetWindowText( sGuid.c_str() );
	}
	
	DWORD dwColor = MAKEDWORDCOLOR( vector_3(1.0f, 1.0f, 1.0f ) );
	if ( gEditorLights.GetColor( dwColor ) ) {

		float blue	= (float)((dwColor & 0x00FF0000) >> 16 ) / 255.0f;
		float green	= (float)((dwColor & 0x0000FF00) >> 8 ) / 255.0f;
		float red	= (float)( dwColor & 0x000000FF ) / 255.0f; 
		vector_3 color( blue, green, red );

		gpstring sTemp;
		sTemp.assignf( "%f", color.x );
		m_edit_red.SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.y );
		m_edit_green.SetWindowText( sTemp.c_str() );
		sTemp.assignf( "%f", color.z );
		m_edit_blue.SetWindowText( sTemp.c_str() );	
	}
	m_edit_color.Invalidate( TRUE );
	
	bool bActive		= false;
	bool bDrawShadows	= false;
	bool bOcclude		= false;
	bool bAffectActors	= true;
	bool bAffectItems	= true;
	bool bAffectTerrain	= true;
	bool bOnTimer		= false;

	vector_3 vDir;
	float intensity		= 1.0f;
	float iradius		= 0.0f;
	float oradius		= 20.0f;

	gEditorLights.GetLightProperties(	vDir, iradius, oradius, intensity, bOcclude, bDrawShadows, bActive, 
										bAffectActors, bAffectItems, bAffectTerrain, bOnTimer );
	
	m_active.SetCheck( (int)bActive );
	m_draw_shadows.SetCheck( (int)bDrawShadows );
	m_occlude.SetCheck( (int)bOcclude );
	m_affect_actors.SetCheck( (int)bAffectActors );
	m_affect_items.SetCheck( (int)bAffectItems );
	m_affect_terrain.SetCheck( (int)bAffectTerrain );
	m_on_timer.SetCheck( (int)bOnTimer );

	gpstring sTemp;
	sTemp.assignf( "%f", vDir.x );
	m_edit_x.SetWindowText( sTemp );
	sTemp.assignf( "%f", vDir.y );
	m_edit_y.SetWindowText( sTemp );
	sTemp.assignf( "%f", vDir.z );
	m_edit_z.SetWindowText( sTemp );

	sTemp.assignf( "%f", intensity );
	m_edit_intensity.SetWindowText( sTemp );

	sTemp.assignf( "%f", iradius );
	m_edit_iradius.SetWindowText( sTemp );

	sTemp.assignf( "%f", oradius );
	m_edit_oradius.SetWindowText( sTemp );

	m_bApply = false;
	gObjectProperties.Evaluate();
}


void PropPage_Properties_Lighting::Apply()
{
	if ( GetSafeHwnd() ) 
	{
		bool bActive		= m_active.GetCheck() ? true : false;
		bool bDrawShadows	= m_draw_shadows.GetCheck() ? true : false;
		bool bOcclude		= m_occlude.GetCheck() ? true : false;
		bool bAffectActors	= m_affect_actors.GetCheck() ? true : false;
		bool bAffectItems	= m_affect_items.GetCheck() ? true : false;
		bool bAffectTerrain	= m_affect_terrain.GetCheck() ? true : false;
		bool bOnTimer		= m_on_timer.GetCheck() ? true : false;

		CString rTemp;

		vector_3 vDir;
		m_edit_x.GetWindowText( rTemp );
		vDir.x = atof( rTemp );
		m_edit_y.GetWindowText( rTemp );
		vDir.y = atof( rTemp );
		m_edit_z.GetWindowText( rTemp );
		vDir.z = atof( rTemp );

		m_edit_intensity.GetWindowText( rTemp );
		float intensity = atof( rTemp );

		m_edit_iradius.GetWindowText( rTemp );
		float iradius = atof( rTemp );

		m_edit_oradius.GetWindowText( rTemp );
		float oradius = atof( rTemp );

		gEditorLights.SetLightProperties(	vDir, iradius, oradius, intensity, bOcclude, bDrawShadows, bActive,
											bAffectActors, bAffectItems, bAffectTerrain, bOnTimer );
	}
	m_bApply = false;
}


void PropPage_Properties_Lighting::Cancel()
{
}


void PropPage_Properties_Lighting::OnChangeEditColorRed() 
{
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );

	DWORD dwColor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dwColor );
	m_edit_color.Invalidate( TRUE );	
}

void PropPage_Properties_Lighting::OnChangeEditColorGreen() 
{
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );

	DWORD dwColor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dwColor );
	m_edit_color.Invalidate( TRUE );		
}

void PropPage_Properties_Lighting::OnChangeEditColorBlue() 
{
	vector_3 color;
	CString rTemp;
	GetDlgItemText( IDC_EDIT_COLOR_RED, rTemp );
	color.x = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_GREEN, rTemp );		
	color.y = atof( rTemp );
	GetDlgItemText( IDC_EDIT_COLOR_BLUE, rTemp );
	color.z = atof( rTemp );

	DWORD dwColor = MAKEDWORDCOLOR(color);
	gEditorLights.SetColor( dwColor );
	m_edit_color.Invalidate( TRUE );	
}

void PropPage_Properties_Lighting::OnChangeEditDirX() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnChangeEditDirY() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnChangeEditDirZ() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnChangeEditIradius() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnChangeEditOradius() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnChangeEditIntensity() 
{
	m_bApply = true;
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckOcclude() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckDrawshadows() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckAffectItems() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckOntimer() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckActive() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckAffectActors() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

void PropPage_Properties_Lighting::OnCheckAffectTerrain() 
{	
	m_bApply = true;	
	gObjectProperties.Evaluate();
}

BOOL PropPage_Properties_Lighting::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR * pNm = (NMHDR *)lParam;
	if ( pNm->code == 0xffffff37 )
	{
		if ( gObjectProperties.Evaluate() )
		{
			int result = MessageBoxEx( this->GetSafeHwnd(), "You have changed the properties on this page.  Would you like to apply the changes?", "Apply Changes?", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
			if ( result == IDYES ) 
			{
				gObjectProperties.Apply();
			}	
		}
	}
	
	return CResizablePage::OnNotify(wParam, lParam, pResult);
}
