// Dialog_Grid_Settings.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Grid_Settings.h"
#include "Preferences.h"
#include "EditorTerrain.h"
#include "siege_node.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace siege;

/////////////////////////////////////////////////////////////////////////////
// Dialog_Grid_Settings dialog


Dialog_Grid_Settings::Dialog_Grid_Settings(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Grid_Settings::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Grid_Settings)
	//}}AFX_DATA_INIT
}


void Dialog_Grid_Settings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Grid_Settings)
	DDX_Control(pDX, IDC_CHECK_LABELS_INTERVAL, m_LabelInterval);
	DDX_Control(pDX, IDC_STATIC_CONVERSION, m_Conversion);
	DDX_Control(pDX, IDC_STATIC_COLOR, m_Color);
	DDX_Control(pDX, IDC_EDIT_SIZE, m_Size);
	DDX_Control(pDX, IDC_EDIT_ORIGIN_Z, m_OriginZ);
	DDX_Control(pDX, IDC_EDIT_ORIGIN_Y, m_OriginY);
	DDX_Control(pDX, IDC_EDIT_ORIGIN_X, m_OriginX);
	DDX_Control(pDX, IDC_EDIT_INTERVAL, m_Interval);
	DDX_Control(pDX, IDC_CHECK_ENABLE_GRID, m_EnableGrid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Grid_Settings, CDialog)
	//{{AFX_MSG_MAP(Dialog_Grid_Settings)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_GRID, OnCheckEnableGrid)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_BUTTON_COLOR, OnButtonColor)
	ON_BN_CLICKED(IDC_BUTTON_USE_SELECTED_NODE_CENTER, OnButtonUseSelectedNodeCenter)
	ON_BN_CLICKED(IDC_BUTTON_USE_DEFAULT_CENTER, OnButtonUseDefaultCenter)
	ON_EN_CHANGE(IDC_EDIT_SIZE, OnChangeEditSize)
	ON_EN_CHANGE(IDC_EDIT_INTERVAL, OnChangeEditInterval)
	ON_EN_CHANGE(IDC_EDIT_ORIGIN_X, OnChangeEditOriginX)
	ON_EN_CHANGE(IDC_EDIT_ORIGIN_Y, OnChangeEditOriginY)
	ON_EN_CHANGE(IDC_EDIT_ORIGIN_Z, OnChangeEditOriginZ)
	ON_BN_CLICKED(IDC_CHECK_LABELS_INTERVAL, OnCheckLabelsInterval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Grid_Settings message handlers

BOOL Dialog_Grid_Settings::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_EnableGrid.SetCheck( gPreferences.GetEnableGrid() );

	gpstring sTemp;
	sTemp.assignf( "%d", gPreferences.GetGridSize() );
	m_Size.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%d", gPreferences.GetGridInterval() );
	m_Interval.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetGridOrigin().x );
	m_OriginX.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetGridOrigin().y );
	m_OriginY.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gPreferences.GetGridOrigin().z );
	m_OriginZ.SetWindowText( sTemp.c_str() );

	m_LabelInterval.SetCheck( gPreferences.GetGridDrawLabelIntervals() );

	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Grid_Settings::OnCheckEnableGrid() 
{
	gPreferences.SetEnableGrid( m_EnableGrid.GetCheck() ? true : false );	
}

void Dialog_Grid_Settings::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);				
}

void Dialog_Grid_Settings::OnButtonColor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

	// Convert dword color to colorref	
	BYTE rb, gb, bb = 0;
	rb = (unsigned char)( (gPreferences.GetGridColor() & 0x00ff0000) >> 16 );
	gb = (unsigned char)( (gPreferences.GetGridColor() & 0x0000ff00) >> 8 );
	bb = (unsigned char)( (gPreferences.GetGridColor() & 0x000000ff));
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
		return;
	}
	
	float blue	= (float)(GetBValue(cc.rgbResult) ) / 255.0f;
	float green	= (float)(GetGValue(cc.rgbResult) ) / 255.0f;
	float red	= (float)(GetRValue(cc.rgbResult) ) / 255.0f; 
	vector_3 vcolor( red, green, blue );
	DWORD dcolor = MAKEDWORDCOLOR(vcolor);
	gPreferences.SetGridColor( dcolor );	
	
	m_Color.Invalidate( TRUE );
	
	SetCustomColors( crCustColors );		
}

void Dialog_Grid_Settings::OnButtonUseSelectedNodeCenter() 
{
	SiegeNode * pNode = gSiegeEngine.IsNodeValid( gEditorTerrain.GetSelectedNode() );
	if ( pNode )
	{
		SiegeMesh& bmesh = pNode->GetMeshHandle().RequestObject(gSiegeEngine.MeshCache());
		vector_3 center = bmesh.Centroid();
		center = pNode->NodeToWorldSpace( center );
		gPreferences.SetGridOrigin( center );

		gpstring sTemp;
		sTemp.assignf( "%f", center.x );
		m_OriginX.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%f", center.y );
		m_OriginY.SetWindowText( sTemp.c_str() );

		sTemp.assignf( "%f", center.z );
		m_OriginZ.SetWindowText( sTemp.c_str() );
	}
}

void Dialog_Grid_Settings::OnButtonUseDefaultCenter() 
{
	m_OriginX.SetWindowText( "0.0" );
	m_OriginY.SetWindowText( "0.0" );
	m_OriginZ.SetWindowText( "0.0" );	
	gPreferences.SetGridOrigin( vector_3( 0.0f, 0.0f, 0.0f ) );
}

void Dialog_Grid_Settings::OnChangeEditSize() 
{
	CString rSize;
	m_Size.GetWindowText( rSize );
	if ( rSize.IsEmpty() )
	{
		return;
	}
	int size = 0;
	stringtool::Get( rSize.GetBuffer( 0 ), size );
	gPreferences.SetGridSize( size );

	Refresh();		
}

void Dialog_Grid_Settings::OnChangeEditInterval() 
{
	CString rInterval;
	m_Interval.GetWindowText( rInterval );
	if ( rInterval.IsEmpty() )
	{
		return;
	}
	int interval = 0;
	stringtool::Get( rInterval.GetBuffer( 0 ), interval );
	gPreferences.SetGridInterval( interval );

	Refresh();			
}

void Dialog_Grid_Settings::OnChangeEditOriginX() 
{
	vector_3 origin = gPreferences.GetGridOrigin();
	CString rOrigin;
	m_OriginX.GetWindowText( rOrigin );
	if ( rOrigin.IsEmpty() )
	{
		return;
	}
	
	stringtool::Get( rOrigin.GetBuffer( 0 ), origin.x );

	gPreferences.SetGridOrigin( origin );
}

void Dialog_Grid_Settings::OnChangeEditOriginY() 
{
	vector_3 origin = gPreferences.GetGridOrigin();
	CString rOrigin;
	m_OriginY.GetWindowText( rOrigin );
	if ( rOrigin.IsEmpty() )
	{
		return;
	}
	
	stringtool::Get( rOrigin.GetBuffer( 0 ), origin.y );

	gPreferences.SetGridOrigin( origin );	
}

void Dialog_Grid_Settings::OnChangeEditOriginZ() 
{
	vector_3 origin = gPreferences.GetGridOrigin();
	CString rOrigin;
	m_OriginZ.GetWindowText( rOrigin );
	if ( rOrigin.IsEmpty() )
	{
		return;
	}
	
	stringtool::Get( rOrigin.GetBuffer( 0 ), origin.z );

	gPreferences.SetGridOrigin( origin );	
}

LRESULT Dialog_Grid_Settings::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if ( message == WM_CTLCOLORSTATIC ) 
	{		
		int id = GetWindowLong( (HWND)lParam, GWL_ID );
		if ( id == IDC_STATIC_COLOR ) 
		{						
			if ( m_hBrush ) 
			{
				DeleteObject( m_hBrush );
			}

			BYTE rb, gb, bb = 0;
			rb = (unsigned char)( (gPreferences.GetGridColor() & 0x00ff0000) >> 16 );
			gb = (unsigned char)( (gPreferences.GetGridColor() & 0x0000ff00) >> 8 );
			bb = (unsigned char)( (gPreferences.GetGridColor() & 0x000000ff));
			COLORREF colorref = RGB( rb, gb, bb );	

			m_hBrush = CreateSolidBrush( colorref );

			return (LRESULT)m_hBrush;
		}
	}
	
	return CDialog::WindowProc(message, wParam, lParam);
}


void Dialog_Grid_Settings::Refresh()
{
	gpstring sTemp;
	sTemp.assignf( "1 box equals %f meters", (float)gPreferences.GetGridSize()/(float)gPreferences.GetGridInterval() );
	m_Conversion.SetWindowText( sTemp.c_str() );
}

void Dialog_Grid_Settings::OnCheckLabelsInterval() 
{
	gPreferences.SetGridDrawLabelIntervals( m_LabelInterval.GetCheck() ? true : false );
	
}
/*
void Dialog_Grid_Settings::OnCheckLabelSet() 
{
	gPreferences.SetGridDrawLabelAmount( m_LabelSet.GetCheck() ? true : false );
	
}

void Dialog_Grid_Settings::OnChangeEditLabelInterval() 
{
	CString rAmount;
	m_LabelIntervalAmount.GetWindowText( rAmount );
	if ( rAmount.IsEmpty() )
	{
		return;
	}

	int interval = 0;
	stringtool::Get( rAmount.GetBuffer( 0 ), interval );
	gPreferences.SetGridLabelAmount( interval );
}
*/