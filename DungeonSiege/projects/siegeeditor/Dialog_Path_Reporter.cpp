// Dialog_Path_Reporter.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Path_Reporter.h"
#include "EditorTuning.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Reporter dialog


Dialog_Path_Reporter::Dialog_Path_Reporter(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Path_Reporter::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Path_Reporter)
	//}}AFX_DATA_INIT
}


void Dialog_Path_Reporter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Path_Reporter)
	DDX_Control(pDX, IDC_CHECK_REGION, m_region);
	DDX_Control(pDX, IDC_CHECK_PLACEDITEM_POINT, m_placeditem_point);
	DDX_Control(pDX, IDC_CHECK_PLACEDITEM, m_placeditem);
	DDX_Control(pDX, IDC_CHECK_PATH_SUMMARY_POINT, m_path_point);
	DDX_Control(pDX, IDC_CHECK_PARTYMEMBER_POINT, m_partymember_point);
	DDX_Control(pDX, IDC_CHECK_PARTYMEMBER, m_partymember);
	DDX_Control(pDX, IDC_CHECK_ENEMYITEM_POINT, m_enemyitem_point);
	DDX_Control(pDX, IDC_CHECK_ENEMYITEM, m_enemyitem);
	DDX_Control(pDX, IDC_CHECK_ENEMY_POINT, m_enemy_point);
	DDX_Control(pDX, IDC_CHECK_ENEMY, m_enemy);
	DDX_Control(pDX, IDC_CHECK_CONTAINERITEM_POINT, m_containeritem_point);
	DDX_Control(pDX, IDC_CHECK_CONTAINERITEM, m_containeritem);
	DDX_Control(pDX, IDC_CHECK_ALLITEM_POINT, m_allitem_point);
	DDX_Control(pDX, IDC_CHECK_ALLITEM, m_allitem);
	DDX_Control(pDX, IDC_CHECK_PATH_SUMMARY, m_pathSummary);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Path_Reporter, CDialog)
	//{{AFX_MSG_MAP(Dialog_Path_Reporter)
	ON_BN_CLICKED(IDC_BUTTON_GENERATE_REPORTS, OnButtonGenerateReports)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_Reporter message handlers

void Dialog_Path_Reporter::OnButtonGenerateReports() 
{
	ReportReq requests;
	requests.bSummary = false;
	requests.bSummaryPoint = false;
	requests.bSummaryRegion = false;
	requests.bPlacedItem = false;
	requests.bPlacedItemPoint = false;
	requests.bPlacedItemRegion = false;
	requests.bPartyMember = false;
	requests.bPartyMemberPoint = false;
	requests.bPartyMemberRegion = false;
	requests.bEnemyItem = false;
	requests.bEnemyItemPoint = false;
	requests.bEnemyItemRegion = false;
	requests.bEnemy = false;
	requests.bEnemyPoint = false;
	requests.bEnemyRegion = false;
	requests.bContainerItem = false;
	requests.bContainerItemPoint = false;
	requests.bContainerItemRegion = false;
	requests.bAllItem = false;
	requests.bAllItemPoint = false;
	requests.bAllItemRegion = false;

	if ( m_region.GetCheck() )
	{
		requests.bSummaryRegion = true;
	}
	if ( m_pathSummary.GetCheck() )
	{
		requests.bSummary = true;
	}
	if ( m_path_point.GetCheck() )
	{
		requests.bSummaryPoint = true;
	}
	if ( m_placeditem.GetCheck() )
	{
		if ( requests.bSummary )
		{
			requests.bPlacedItem = true;
		}
		if ( requests.bSummaryRegion )
		{
			requests.bPlacedItemRegion = true;
		}
	}
	if ( m_placeditem_point.GetCheck() )
	{
		requests.bPlacedItemPoint = true; 
	}
	if ( m_partymember.GetCheck() )
	{
		if ( requests.bSummary )
		{
			requests.bPartyMember = true;
		}
		if ( requests.bSummaryRegion )
		{
			requests.bPartyMemberRegion = true;
		}
	}
	if ( m_partymember_point.GetCheck() )
	{		
		requests.bPartyMemberPoint = true;
	}
	if ( m_enemyitem.GetCheck() )
	{
		if ( requests.bSummary )
		{
			requests.bEnemyItem = true; 
		}
		if ( requests.bSummaryRegion )
		{
			requests.bEnemyItemRegion = true;
		}
	}
	if ( m_enemyitem_point.GetCheck() )
	{
		requests.bEnemyItemPoint = true;
	}
	if ( m_enemy.GetCheck() )
	{
		if ( requests.bSummary )
		{
			requests.bEnemy = true; 
		}
		if ( requests.bSummaryRegion )
		{
			requests.bEnemyRegion = true;
		}
	}
	if ( m_containeritem.GetCheck() )
	{
		if ( requests.bSummary )
		{
			requests.bContainerItem = true;
		}
		if ( requests.bSummaryRegion )
		{
			requests.bContainerItemRegion = true;
		}
	}
	if ( m_containeritem_point.GetCheck() )
	{
		requests.bContainerItemPoint = true;
	}
	if ( m_allitem.GetCheck() )
	{
		if ( requests.bSummary )
		{
			requests.bAllItem = true;
		}
		if ( requests.bSummaryRegion )
		{
			requests.bAllItemRegion = true;
		}
	}
	if ( m_allitem_point.GetCheck() )
	{
		requests.bAllItemPoint = true;
	}

	gEditorTuning.SetReportRequest( requests );	

	CDialog::OnOK();
}

BOOL Dialog_Path_Reporter::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_region.SetCheck( 1 );
	m_placeditem_point.SetCheck( 1 );
	m_placeditem.SetCheck( 1 );
	m_path_point.SetCheck( 1 );
	m_partymember_point.SetCheck( 1 );
	m_partymember.SetCheck( 1 );
	m_enemyitem_point.SetCheck( 1 );
	m_enemyitem.SetCheck( 1 );
	m_enemy_point.SetCheck( 1 );
	m_enemy.SetCheck( 1 );
	m_containeritem_point.SetCheck( 1 );
	m_containeritem.SetCheck( 1 );
	m_allitem_point.SetCheck( 1 );
	m_allitem.SetCheck( 1 );
	m_pathSummary.SetCheck( 1 );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
