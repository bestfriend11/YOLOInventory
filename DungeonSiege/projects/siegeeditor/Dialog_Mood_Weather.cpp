// Dialog_Mood_Weather.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Mood_Weather.h"
#include "Dialog_Mood_Editor.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Weather dialog


Dialog_Mood_Weather::Dialog_Mood_Weather(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Mood_Weather::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Mood_Weather)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Mood_Weather::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Mood_Weather)
	DDX_Control(pDX, IDC_EDIT_WIND_VELOCITY, m_WindVelocity);
	DDX_Control(pDX, IDC_EDIT_WIND_DIRECTION, m_WindDirection);
	DDX_Control(pDX, IDC_EDIT_SNOW_DENSITY, m_SnowDensity);
	DDX_Control(pDX, IDC_EDIT_RAIN_DENSITY, m_RainDensity);
	DDX_Control(pDX, IDC_CHECK_ENABLE_WIND, m_EnableWind);
	DDX_Control(pDX, IDC_CHECK_RAIN_ENABLE, m_RainEnable);
	DDX_Control(pDX, IDC_CHECK_LIGHTNING, m_Lightning);
	DDX_Control(pDX, IDC_CHECK_ENABLE_SNOW, m_EnableSnow);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Mood_Weather, CDialog)
	//{{AFX_MSG_MAP(Dialog_Mood_Weather)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_BN_CLICKED(IDC_CHECK_RAIN_ENABLE, OnCheckRainEnable)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_SNOW, OnCheckEnableSnow)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_WIND, OnCheckEnableWind)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Mood_Weather message handlers

BOOL Dialog_Mood_Weather::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		FuelHandle hRain = hMood->GetChildBlock( "rain" );
		if ( hRain )
		{
			m_RainEnable.SetCheck( TRUE );
			float rainDensity = 0.0f;
			hRain->Get( "rain_density", rainDensity );
			gpstring sTemp;
			sTemp.assignf( "%f", rainDensity );
			m_RainDensity.SetWindowText( sTemp.c_str() );

			bool bLightning = false;
			hRain->Get( "lightning", bLightning );
			m_Lightning.SetCheck( bLightning ? TRUE : FALSE );
		}
		else
		{
			m_RainDensity.EnableWindow( FALSE );
			m_Lightning.EnableWindow( FALSE );
		}

		FuelHandle hSnow = hMood->GetChildBlock( "snow" );
		if ( hSnow )
		{
			m_EnableSnow.SetCheck( TRUE );
			float snowDensity = 0.0f;
			hSnow->Get( "snow_density", snowDensity );
			gpstring sTemp;
			sTemp.assignf( "%f", snowDensity );
			m_SnowDensity.SetWindowText( sTemp.c_str() );
		}
		else
		{
			m_SnowDensity.EnableWindow( FALSE );
		}

		FuelHandle hWind = hMood->GetChildBlock( "wind" );
		if ( hWind )
		{
			m_EnableWind.SetCheck( TRUE );
			float windVelocity = 0.0f;
			float windDirection = 0.0f;
			hWind->Get( "wind_velocity", windVelocity );
			hWind->Get( "wind_direction", windDirection );
			gpstring sTemp;
			sTemp.assignf( "%f", windVelocity );
			m_WindVelocity.SetWindowText( sTemp.c_str() );
			sTemp.assignf( "%f", windDirection );
			m_WindDirection.SetWindowText( sTemp.c_str() );			
		}
		else
		{
			m_WindVelocity.EnableWindow( FALSE );
			m_WindDirection.EnableWindow( FALSE );
		}		
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Mood_Weather::OnOK() 
{
	FuelHandle hMood = gDialogMoodEditor.GetSelectedMood();
	if ( hMood )
	{
		FuelHandle hRain = hMood->GetChildBlock( "rain" );
		if ( m_RainEnable.GetCheck() )
		{			
			if ( !hRain )
			{
				hRain = hMood->CreateChildBlock( "rain" );
			}

			float rainDensity = 0.0f;
			CString rRainDensity;
			m_RainDensity.GetWindowText( rRainDensity );
			if ( rRainDensity.IsEmpty() )
			{
				MessageBoxEx( this->GetSafeHwnd(), "Please enter a rain density.", "Rain Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
			stringtool::Get( rRainDensity.GetBuffer( 0 ), rainDensity );
			hRain->Set( "rain_density", rainDensity );

			hRain->Set( "lightning", m_Lightning.GetCheck() ? true : false );
		}
		else
		{
			if ( hRain )
			{
				hMood->DestroyChildBlock( hRain );
			}
		}

		FuelHandle hSnow = hMood->GetChildBlock( "snow" );
		if ( m_EnableSnow.GetCheck() )
		{		
			if ( !hSnow )
			{
				hSnow = hMood->CreateChildBlock( "snow" );
			}

			float snowDensity = 0.0f;
			CString rSnowDensity;
			m_SnowDensity.GetWindowText( rSnowDensity );
			if ( rSnowDensity.IsEmpty() )
			{
				MessageBoxEx( this->GetSafeHwnd(), "Please enter a snow density.", "Snow Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
			stringtool::Get( rSnowDensity.GetBuffer( 0 ), snowDensity );
			hSnow->Set( "snow_density", snowDensity );
		}
		else 
		{
			if ( hSnow )
			{
				hMood->DestroyChildBlock( hSnow );
			}
		}

		FuelHandle hWind = hMood->GetChildBlock( "wind" );
		if ( m_EnableWind.GetCheck() )
		{
			if ( !hWind )
			{
				hWind = hMood->CreateChildBlock( "wind" );
			}

			float windVelocity = 0.0f;
			CString rWindVelocity;
			m_WindVelocity.GetWindowText( rWindVelocity );
			if ( rWindVelocity.IsEmpty() )
			{
				MessageBoxEx( this->GetSafeHwnd(), "Please enter a wind velocity.", "Wind Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
			stringtool::Get( rWindVelocity.GetBuffer( 0 ), windVelocity );
			hWind->Set( "wind_velocity", windVelocity );

			float windDirection = 0.0f;
			CString rWindDirection;
			m_WindDirection.GetWindowText( rWindDirection );
			if ( rWindDirection.IsEmpty() )
			{
				MessageBoxEx( this->GetSafeHwnd(), "Please enter a wind direction.", "Wind Properties", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				return;
			}
			stringtool::Get( rWindDirection, windDirection );
			hWind->Set( "wind_direction", windDirection );			
		}
		else 
		{
			if ( hWind )
			{
				hMood->DestroyChildBlock( hWind );
			}
		}

		hMood->GetDB()->SaveChanges();
	}
	
	CDialog::OnOK();
}

void Dialog_Mood_Weather::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void Dialog_Mood_Weather::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}

void Dialog_Mood_Weather::OnCheckRainEnable() 
{
	m_RainDensity.EnableWindow( m_RainEnable.GetCheck() );
	m_Lightning.EnableWindow( m_RainEnable.GetCheck() );
}

void Dialog_Mood_Weather::OnCheckEnableSnow() 
{
	m_SnowDensity.EnableWindow( m_EnableSnow.GetCheck() );	
}

void Dialog_Mood_Weather::OnCheckEnableWind() 
{
	m_WindVelocity.EnableWindow( m_EnableWind.GetCheck() );
	m_WindDirection.EnableWindow( m_EnableWind.GetCheck() );	
}
