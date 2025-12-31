// Dialog_Sfx_Parameters.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Sfx_Parameters.h"
#include "gridctrl\\gridctrl.h"
#include "gridctrl\\gridbtncellcombo.h"
#include "EditorSfx.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Parameters dialog


Dialog_Sfx_Parameters::Dialog_Sfx_Parameters(CWnd* pParent /*=NULL*/)
	: CResizableDialog(Dialog_Sfx_Parameters::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Sfx_Parameters)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Sfx_Parameters::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Sfx_Parameters)
	DDX_Control(pDX, IDC_STATIC_REMOVE, m_remove);
	DDX_Control(pDX, IDC_STATIC_EFFECT_NAME, m_effectName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Sfx_Parameters, CResizableDialog)
	//{{AFX_MSG_MAP(Dialog_Sfx_Parameters)
	ON_BN_CLICKED(IDC_BUTTON_ADD_PARAMETER, OnButtonAddParameter)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_PARAMETER, OnButtonRemoveParameter)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Parameters message handlers

BOOL Dialog_Sfx_Parameters::OnInitDialog() 
{
	CResizableDialog::OnInitDialog();
	
	RECT dlgRect;
	this->GetWindowRect( &dlgRect );
	
	RECT replaceRect;
	m_remove.GetWindowRect( &replaceRect );	

	RECT gridRect;
	gridRect.left		= replaceRect.left - dlgRect.left - 5;
	gridRect.right		= replaceRect.right - dlgRect.left - 5;
	gridRect.top		= replaceRect.top - dlgRect.top + GetSystemMetrics( SM_CYCAPTION ) - 40;
	gridRect.bottom		= replaceRect.bottom - dlgRect.top + GetSystemMetrics( SM_CYCAPTION ) - 40;

	m_grid.Create( gridRect, this, IDC_SFX_PARAM_GRIDCONTROL, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	
	ShowSizeGrip( TRUE );

	AddAnchor( IDC_SFX_PARAM_GRIDCONTROL, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_STATIC_EFFECT_NAME, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDOK, TOP_RIGHT );
	AddAnchor( IDCANCEL, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_ADD_PARAMETER, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_REMOVE_PARAMETER, TOP_RIGHT );

	m_grid.DeleteAllItems();	
	m_grid.SetEditable( TRUE );
	m_grid.SetRowResize( TRUE );
	m_grid.SetColumnResize( TRUE );
	m_grid.SetFixedColumnSelection(TRUE);
    m_grid.SetFixedRowSelection(TRUE);
	m_grid.SetFixedRowCount( 1 );	
	m_BtnDataBase.SetGrid( &m_grid);

	m_grid.InsertColumn( "Parameter Name" );
	m_grid.InsertColumn( "Type" );
	m_grid.InsertColumn( "Value" );
	m_grid.InsertColumn( "Documentation" );

	m_effectName.SetWindowText( gEditorSfx.GetCurrentEffect().c_str() );

	BuildGridFromParameterString();
		
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Sfx_Parameters::OnOK() 
{
	gEditorSfx.SetCurrentEffectParams( BuildParameterStringFromGrid() );
	
	CResizableDialog::OnOK();
}

void Dialog_Sfx_Parameters::OnCancel() 
{	
	CResizableDialog::OnCancel();
}

void Dialog_Sfx_Parameters::OnButtonAddParameter() 
{
	int newRow = m_grid.InsertRow( "" );
	
	m_grid.SetCellType( newRow, PARAMETER_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
	CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( newRow, PARAMETER_COL );
	pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
	pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
	m_grid.SetItemBkColour( newRow, PARAMETER_COL, 0x00d2d255 );
	
	CStringArray sa;

	EffectParametersVec effectParameters;
	EffectParametersVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetEffectParameters( effectParameters );
	for ( i = effectParameters.begin(); i != effectParameters.end(); ++i )
	{
		if ( gEditorSfx.GetCurrentEffect().same_no_case( (*i).sEffectName ) || (*i).sEffectName.same_no_case( "global" ) )
		{
			EffectParameterVec::iterator j;
			for ( j = (*i).parameters.begin(); j != (*i).parameters.end(); ++j )
			{
				sa.Add( (*j).sParameter.c_str() );
			}		
		}
	}
	
	pGridBtnCell->SetComboString( sa );
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();		
}

void Dialog_Sfx_Parameters::OnButtonRemoveParameter() 
{
	CCellRange range = m_grid.GetSelectedCellRange();
	if (( range.GetMaxRow() == -1 ) || ( range.GetMaxCol() == -1 ))
	{
		return;
	}

	if ( range.Count() == 1 )
	{
		CString rText = m_grid.GetItemText( range.GetMinRow(), range.GetMinCol() );
		if ( !rText.IsEmpty() )
		{
			int row = range.GetMinRow();
			int col = range.GetMinCol();

			m_grid.DeleteRow( row );
		}
	}
			
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
}


gpstring Dialog_Sfx_Parameters::BuildParameterStringFromGrid()
{	
	gpstring sParameters;

	for ( int row = 1; row != m_grid.GetRowCount(); ++row )
	{
		CString rParameter = m_grid.GetItemText( row, PARAMETER_COL );
		if ( rParameter.IsEmpty() )
		{
			continue;
		}

		CString rValue = m_grid.GetItemText( row, PARAM_VALUE_COL );
		gpstring sValue;

		CString rType = m_grid.GetItemText( row, PARAM_TYPE_COL );
		if ( rType.CompareNoCase( "integer" ) == 0 )
		{
			int value = 0;
			stringtool::Get( rValue.GetBuffer( 0 ), value );
			sValue.assignf( "%d", value );
		}
		else if ( rType.CompareNoCase( "vector" ) == 0 )
		{
			vector_3 value;
			int numStrings = stringtool::GetNumDelimitedStrings( rValue.GetBuffer( 0 ), ',' );
			if ( numStrings != 3 )
			{
				gpstring sMessage;
				sMessage.assignf( "Vector type format for parameter: %s is incorrect.  ( For example, try: float,float,float ).", rParameter.GetBuffer( 0 ) );
				MessageBoxEx( this->GetSafeHwnd(), sMessage.c_str(), "SiegeFX Effect Parameters", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
				continue;
			}

			stringtool::GetDelimitedValue( rValue.GetBuffer( 0 ), ',', 0, value.x );
			stringtool::GetDelimitedValue( rValue.GetBuffer( 0 ), ',', 1, value.y );
			stringtool::GetDelimitedValue( rValue.GetBuffer( 0 ), ',', 2, value.z );

			sValue.assignf( "%f,%f,%f", value.x, value.y, value.z );
		}
		else if ( rType.CompareNoCase( "float" ) == 0 )
		{
			float value = 0;
			stringtool::Get( rValue.GetBuffer( 0 ), value );
			sValue.assignf( "%f", value );
		}
		else
		{
			sValue = rValue.GetBuffer( 0 );
		}
		
		sParameters.appendf( "%s(%s)", rParameter.GetBuffer( 0 ), sValue.c_str() );
	}
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
	return sParameters;
}


void Dialog_Sfx_Parameters::BuildGridFromParameterString()
{
	gpstring sParam;
	gpstring sCurrentParams = gEditorSfx.GetCurrentEffectParams();
	int newRow = 0;	
	for ( int i = 0; i != sCurrentParams.size(); ++i )
	{
		if ( sCurrentParams[i] == '(' )
		{			
			newRow = m_grid.InsertRow( sParam.c_str() );

			EffectParametersVec effectParameters;
			EffectParametersVec::iterator k;
			gEditorSfx.GetEffectSchemaDb()->GetEffectParameters( effectParameters );
			for ( k = effectParameters.begin(); k != effectParameters.end(); ++k )
			{				
				if ( gEditorSfx.GetCurrentEffect().same_no_case( (*k).sEffectName ) )
				{
					EffectParameterVec::iterator j;
					for ( j = (*k).parameters.begin(); j != (*k).parameters.end(); ++j )
					{
						if ( sParam.same_no_case( (*j).sParameter ) )
						{
							m_grid.SetItemText( newRow, PARAM_TYPE_COL, (*j).sType.c_str() );
							m_grid.SetItemText( newRow, PARAM_DOC_COL, (*j).sDoc.c_str() );
							
							m_grid.SetItemState( newRow, PARAM_TYPE_COL, GVIS_READONLY );
							m_grid.SetItemState( newRow, PARAM_DOC_COL, GVIS_READONLY );
							m_grid.SetItemState( newRow, PARAMETER_COL, GVIS_READONLY );

							m_grid.SetItemBkColour( newRow, PARAM_VALUE_COL, 0x00d2d255 );
						}
					}
				}
			}

			sParam.clear();			
		}
		else if ( sCurrentParams[i] == ')' )
		{
			m_grid.SetItemText( newRow, PARAM_VALUE_COL, sParam.c_str() );
			sParam.clear();			
		}
		else
		{		
			sParam.appendf( "%c", sCurrentParams[i] );
		}
	}
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
}

BOOL Dialog_Sfx_Parameters::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if ( m_grid.GetSafeHwnd() && (wParam == (WPARAM)m_grid.GetDlgCtrlID()))
    {
        *pResult = 1;
        GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;
        if (GVN_ENDLABELEDIT == pDispInfo->hdr.code)
        { 
			// Read command information
			if ( pDispInfo->item.col == PARAMETER_COL ) 
			{
				CString rText = m_grid.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
				if ( !rText.IsEmpty() )
				{
					m_grid.SetItemState( pDispInfo->item.row, PARAMETER_COL, GVIS_READONLY );
					
					EffectParametersVec effectParameters;
					EffectParametersVec::iterator i;
					gEditorSfx.GetEffectSchemaDb()->GetEffectParameters( effectParameters );
					for ( i = effectParameters.begin(); i != effectParameters.end(); ++i )
					{
						if ( gEditorSfx.GetCurrentEffect().same_no_case( (*i).sEffectName ) )
						{
							EffectParameterVec::iterator j;
							for ( j = (*i).parameters.begin(); j != (*i).parameters.end(); ++j )
							{
								if ( (*j).sParameter.same_no_case( rText.GetBuffer( 0 ) ) )
								{
									m_grid.SetItemText( pDispInfo->item.row, PARAM_TYPE_COL, (*j).sType.c_str() );
									m_grid.SetItemText( pDispInfo->item.row, PARAM_DOC_COL, (*j).sDoc.c_str() );
								
									m_grid.SetItemState( pDispInfo->item.row, PARAM_TYPE_COL, GVIS_READONLY );
									m_grid.SetItemState( pDispInfo->item.row, PARAM_DOC_COL, GVIS_READONLY );
								}
							}
						}
					}

					m_grid.AutoSizeColumns();
					m_grid.Refresh();
				}
			}
		}
	}
	
	
	return CResizableDialog::OnNotify(wParam, lParam, pResult);
}

void Dialog_Sfx_Parameters::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
