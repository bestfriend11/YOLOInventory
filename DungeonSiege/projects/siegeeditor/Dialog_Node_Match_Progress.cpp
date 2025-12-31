// Dialog_Node_Match_Progress.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Node_Match_Progress.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Match_Progress dialog


Dialog_Node_Match_Progress::Dialog_Node_Match_Progress(CWnd* pParent /*=NULL*/)
	: CDialog(Dialog_Node_Match_Progress::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Node_Match_Progress)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Node_Match_Progress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Node_Match_Progress)
	DDX_Control(pDX, IDC_STATIC_NODE, m_Node);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Node_Match_Progress, CDialog)
	//{{AFX_MSG_MAP(Dialog_Node_Match_Progress)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Node_Match_Progress message handlers

void Dialog_Node_Match_Progress::SetCurrentNode( const gpstring & sNode )
{
	m_Node.SetWindowText( sNode.c_str() );
}