// Dialog_Path_Maker.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Path_Maker.h"
#include "Dialog_Path_Reporter.h"
#include "EditorTuning.h"
#include "GoEdit.h"
#include "GoGizmo.h"
#include "Preferences.h"
#include "MenuCommands.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Maker dialog


Dialog_Path_Maker::Dialog_Path_Maker(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Path_Maker::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Path_Maker)
	//}}AFX_DATA_INIT
}


void Dialog_Path_Maker::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Path_Maker)
	DDX_Control(pDX, IDC_SPIN_RADIUS, m_spinRadius);
	DDX_Control(pDX, IDC_EDIT_TOLERANCE, m_tolerance);
	DDX_Control(pDX, IDC_EDIT_RADIUS, m_radius);
	DDX_Control(pDX, IDC_CHECK_AUTOSIZE, m_autosize);
	DDX_Control(pDX, IDC_COMBO_PATHS, m_comboPaths);
	DDX_Control(pDX, IDC_CHECK_PLACE_POINTS, m_placePoints);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Path_Maker, CDialog)
	//{{AFX_MSG_MAP(Dialog_Path_Maker)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_PATH, OnButtonRemovePath)
	ON_BN_CLICKED(IDC_BUTTON_REPORTS, OnButtonReports)
	ON_BN_CLICKED(IDC_CHECK_PLACE_POINTS, OnCheckPlacePoints)
	ON_WM_CLOSE()
	ON_CBN_SELCHANGE(IDC_COMBO_PATHS, OnSelchangeComboPaths)
	ON_BN_CLICKED(IDC_CHECK_AUTOSIZE, OnCheckAutosize)
	ON_EN_CHANGE(IDC_EDIT_TOLERANCE, OnChangeEditTolerance)
	ON_EN_CHANGE(IDC_EDIT_RADIUS, OnChangeEditRadius)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_RADIUS, OnDeltaposSpinRadius)
	ON_BN_CLICKED(IDC_BUTTON_DUPLICATE, OnButtonDuplicate)
	ON_BN_CLICKED(IDC_BUTTON_ELEV, OnButtonElev)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Maker message handlers

BOOL Dialog_Path_Maker::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	gpstring sTemp;

	gEditorTuning.SetMakerEnabled( true );

	sTemp.assignf( "%f", gEditorTuning.GetRadius() );
	m_radius.SetWindowText( sTemp.c_str() );

	sTemp.assignf( "%f", gEditorTuning.GetRadiusTolerance() );
	m_tolerance.SetWindowText( sTemp.c_str() );

	if ( gEditorTuning.GetAutoSize() )
	{
		m_autosize.SetCheck( true );
	}

	m_comboPaths.ResetContent();

	StringVec paths;
	StringVec::iterator i;
	gEditorTuning.GetTuningPathNames( paths );
	for ( i = paths.begin(); i != paths.end(); ++i )
	{
		m_comboPaths.AddString( (*i).c_str() );
	}

	int index = m_comboPaths.FindString( 0, gEditorTuning.GetSelectedPath().c_str() );
	if ( index != CB_ERR )
	{
		m_comboPaths.SetCurSel( index );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Path_Maker::OnButtonRemovePath() 
{
	int sel = m_comboPaths.GetCurSel();
	if ( sel != CB_ERR )
	{
		int result = MessageBoxEx( GetSafeHwnd(), "Remove the current path?", "Tuning Paths", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
		if ( result == IDYES )
		{
			result = MessageBoxEx( GetSafeHwnd(), "Do you REALLY want to remove this path?", "Tuning Paths", MB_YESNO | MB_ICONQUESTION, LANG_ENGLISH );
			if ( result == IDYES )
			{
				CString rSel;
				m_comboPaths.GetLBText( sel, rSel );
				gEditorTuning.RemovePath( rSel.GetBuffer( 0 ) );
			}
		}	
	}

	m_comboPaths.ResetContent();
	StringVec paths;
	StringVec::iterator i;
	gEditorTuning.GetTuningPathNames( paths );	
	for ( i = paths.begin(); i != paths.end(); ++i )
	{
		m_comboPaths.AddString( (*i).c_str() );
	}

	int index = m_comboPaths.FindString( 0, gEditorTuning.GetSelectedPath().c_str() );
	if ( index != CB_ERR )
	{
		m_comboPaths.SetCurSel( index );
	}
}

void Dialog_Path_Maker::OnButtonReports() 
{
	Dialog_Path_Reporter dlg;
	if ( dlg.DoModal() == IDOK )
	{
		gEditorTuning.GenerateReports();
	}
}

void Dialog_Path_Maker::OnCheckPlacePoints() 
{
	if ( m_placePoints.GetCheck() )
	{
		gEditorTuning.SetPlacePoints( true );
	}
	else
	{
		gEditorTuning.SetPlacePoints( false );
	}	
}

void Dialog_Path_Maker::OnClose() 
{
	gEditorTuning.SetPlacePoints( false );
	gEditorTuning.SetMakerEnabled( false );
	
	CDialog::OnClose();
}

void Dialog_Path_Maker::OnSelchangeComboPaths() 
{
	int sel = m_comboPaths.GetCurSel();
	CString rSel;
	if ( sel != CB_ERR )
	{
		m_comboPaths.GetLBText( sel, rSel );
		gEditorTuning.SetSelectedPath( rSel.GetBuffer( 0 ) );
	}	
}

void Dialog_Path_Maker::OnCheckAutosize() 
{
	if ( m_autosize.GetCheck() )
	{
		gEditorTuning.SetAutoSize( true );
	}
	else
	{
		gEditorTuning.SetAutoSize( false );
	}
}

void Dialog_Path_Maker::OnChangeEditTolerance() 
{
	CString rTolerance;
	m_tolerance.GetWindowText( rTolerance );
	gEditorTuning.SetRadiusTolerance( (float)atof( rTolerance.GetBuffer( 0 ) ) );
}

void Dialog_Path_Maker::OnChangeEditRadius() 
{
	CString rRadius;
	m_radius.GetWindowText( rRadius );
	gEditorTuning.SetRadius( (float)atof( rRadius.GetBuffer( 0 ) ) );

	std::vector< DWORD > points;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( points );
	for ( i = points.begin(); i != points.end(); ++i )
	{	
		GoHandle hObject( MakeGoid( *i ) );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			gEditorTuning.SetRadius( hObject->GetGoid(), (float)atof( rRadius.GetBuffer( 0 ) ) );
		}		
	}
}

void Dialog_Path_Maker::OnDeltaposSpinRadius(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	
	gEditorTuning.SetRadius( gEditorTuning.GetRadius() + (float)pNMUpDown->iDelta / 10.0f );

	std::vector< DWORD > points;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( points );
	for ( i = points.begin(); i != points.end(); ++i )
	{	
		GoHandle hObject( MakeGoid( *i ) );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			gEditorTuning.SetRadius( hObject->GetGoid(), gEditorTuning.GetRadius() );
		}		
	}
	
	gpstring sRadius;
	sRadius.assignf( "%f", gEditorTuning.GetRadius() );
	m_radius.SetWindowText( sRadius.c_str() );
	
	*pResult = 0;
}

void Dialog_Path_Maker::OnButtonDuplicate() 
{
	std::vector< DWORD > points;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( points );
	for ( i = points.begin(); i != points.end(); ++i )
	{	
		GoHandle hObject( MakeGoid( *i ) );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			hObject->GetEdit()->BeginEdit();
			FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
			if ( pRecord )
			{					
				pRecord->Set( "point_name", "dup" );
			}			
			
			FuBi::Record * pGizmoRecord = hObject->GetEdit()->GetRecord( "gizmo" );
			if (  pGizmoRecord )
			{
				pGizmoRecord->Set( "diffuse_color", "0.0,1.0,0.0" );
			}

			hObject->GetEdit()->EndEdit();
		}
	}	
}

void Dialog_Path_Maker::OnButtonElev() 
{
	std::vector< DWORD > points;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( points );
	for ( i = points.begin(); i != points.end(); ++i )
	{	
		GoHandle hObject( MakeGoid( *i ) );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			hObject->GetEdit()->BeginEdit();
			FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
			if ( pRecord )
			{					
				pRecord->Set( "point_name", "elev" );
			}
			
			FuBi::Record * pGizmoRecord = hObject->GetEdit()->GetRecord( "gizmo" );
			if (  pGizmoRecord )
			{
				pGizmoRecord->Set( "diffuse_color", "0.0,0.0,1.0" );
			}

			hObject->GetEdit()->EndEdit();
		}
	}	
}

void Dialog_Path_Maker::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
