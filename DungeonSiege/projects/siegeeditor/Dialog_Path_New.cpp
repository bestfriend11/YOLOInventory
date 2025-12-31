// Dialog_Path_New.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Path_New.h"
#include "EditorTuning.h"
#include "PropPage_Properties_Lighting.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_New dialog


Dialog_Path_New::Dialog_Path_New(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Path_New::IDD, pParent)
	, m_color( 0xFFFF00FF )
{
	//{{AFX_DATA_INIT(Dialog_Path_New)
	//}}AFX_DATA_INIT
}


void Dialog_Path_New::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Path_New)
	DDX_Control(pDX, IDC_EDIT_PATH, m_path);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Path_New, CDialog)
	//{{AFX_MSG_MAP(Dialog_Path_New)
	ON_BN_CLICKED(IDC_BUTTON_COLOR, OnButtonColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Path_New message handlers

void Dialog_Path_New::OnOK() 
{
	CString rPath;
	m_path.GetWindowText( rPath );	

	if ( gEditorTuning.AddPath( rPath.GetBuffer( 0 ), m_color ) )
	{
		CDialog::OnOK();
	}
	else
	{
		MessageBox( "This path already exists.  Please select another name.", "Path Exists", MB_OK );
	}
}

BOOL Dialog_Path_New::OnInitDialog() 
{
	CDialog::OnInitDialog();
			
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Path_New::OnButtonColor() 
{
	CHOOSECOLOR cc;
	COLORREF crCustColors[16];
	GetCustomColors( crCustColors );

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
	m_color = MAKEDWORDCOLOR(color);
}
