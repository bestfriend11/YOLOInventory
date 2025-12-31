// Dialog_Object_Template.cpp : implementation file
//

#include "PrecompEditor.h"
#include "stdafx.h"
#include "siegeeditor.h"
#include "Dialog_Object_Template.h"
#include "Go.h"
#include "GizmoManager.h"
#include "GoEdit.h"
#include "FuBiSchema.h"
#include "gridctrl\\gridctrl.h"
#include "gridctrl\\gridbtncellcombo.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Template dialog


Dialog_Object_Template::Dialog_Object_Template(CWnd* pParent /*=NULL*/)
	: CResizableDialog(Dialog_Object_Template::IDD, pParent)
{
	//{{AFX_DATA_INIT(Dialog_Object_Template)
	//}}AFX_DATA_INIT
}


void Dialog_Object_Template::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Object_Template)
	DDX_Control(pDX, IDC_EDIT_TEMPLATE_NAME, m_template);
	DDX_Control(pDX, IDC_EDIT_SCID, m_scid);
	DDX_Control(pDX, IDC_STATIC_FIELDS, m_fields);
	DDX_Control(pDX, IDC_TREE_COMPONENTS, m_components);	
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Object_Template, CResizableDialog)
	//{{AFX_MSG_MAP(Dialog_Object_Template)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_COMPONENTS, OnSelchangedTreeComponents)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Object_Template message handlers

BOOL Dialog_Object_Template::OnInitDialog() 
{
	CResizableDialog::OnInitDialog();
	
	RECT fieldRect;
	RECT gridRect;
	RECT dlgRect;

	this->GetWindowRect( &dlgRect );
	m_fields.GetWindowRect( &fieldRect );

	gridRect.left	= fieldRect.left - dlgRect.left + 5;
	gridRect.right	= fieldRect.right - dlgRect.left - 10;
	gridRect.top	= fieldRect.top - dlgRect.top;
	gridRect.bottom = fieldRect.bottom- dlgRect.top - 25;
	m_grid.Create( gridRect, this, IDC_GRIDCONTROL, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	

	ShowSizeGrip( TRUE );

	// Resizability Anchors
	AddAnchor( IDC_TREE_COMPONENTS, TOP_LEFT, BOTTOM_LEFT );
	AddAnchor( IDC_STATIC_COMPONENTS, TOP_LEFT, BOTTOM_LEFT );

	AddAnchor( IDC_STATIC_FIELDS, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_GRIDCONTROL, TOP_LEFT, BOTTOM_RIGHT );

	AddAnchor( IDOK, TOP_RIGHT );
	AddAnchor( IDCANCEL, TOP_RIGHT );

	AddAnchor( IDC_EDIT_SCID, BOTTOM_LEFT );
	AddAnchor( IDC_EDIT_TEMPLATE_NAME, BOTTOM_LEFT );

	AddAnchor( IDC_STATIC_TEMPLATE, BOTTOM_LEFT );
	AddAnchor( IDC_STATIC_SCID, BOTTOM_LEFT );

	m_grid.SetEditable();
	m_grid.SetRowResize();
	m_grid.SetColumnResize();
	m_grid.SetFixedColumnSelection(TRUE);
    m_grid.SetFixedRowSelection(TRUE);
	m_grid.SetFixedRowCount();	

	std::vector< DWORD > objectIds;
	std::vector< DWORD >::iterator j;
	gGizmoManager.GetSelectedObjectIDs( objectIds );
	for ( j = objectIds.begin(); j != objectIds.end(); ++j )
	{
		GoHandle hObject( MakeGoid(*j) );
		if ( hObject.IsValid() )
		{
			m_Selected.push_back( MakeGoid(*j) );
		}

		if ( objectIds.size() == 1 )
		{
			gpstring sTemp; 
			sTemp.assignf( "0x%08X", MakeInt(hObject->GetScid()) );
			m_scid.SetWindowText( sTemp.c_str() );
			
			m_template.SetWindowText( hObject->GetTemplateName() );
		}		
	}
	
	if ( m_Selected.size() == 0 )
	{
		return TRUE;
	}

	ComponentRefMap componentMap;	
	GoidColl::iterator k;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		GoHandle hObject( *k );
		hObject->GetEdit()->BeginEdit();
		Go::ComponentColl::iterator i;

		for ( i = hObject->GetComponentBegin(); i != hObject->GetComponentEnd(); ++i )
		{
			gpstring sComponent = (*i)->GetName();
			if ( sComponent.same_no_case( "edit" ) == false )
			{
				ComponentRefMap::iterator iFound = componentMap.find( sComponent );
				if ( iFound != componentMap.end() )
				{
					(*iFound).second++;
				}
				else
				{
					componentMap.insert( ComponentRefPair( sComponent, 1 ) );
				}				
			}
		}		
	}

	ComponentRefMap::iterator l;
	for ( l = componentMap.begin(); l != componentMap.end(); ++l )
	{
		if ( (*l).second == m_Selected.size() )
		{
			m_components.InsertItem( (*l).first.c_str(), 0, 0 );
		}
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void Dialog_Object_Template::OnOK() 
{
	GoidColl::iterator k;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		GoHandle hObject( *k );
		hObject->GetEdit()->EndEdit();
	}
	
	CDialog::OnOK();
}

void Dialog_Object_Template::OnCancel() 
{
	GoidColl::iterator k;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		GoHandle hObject( *k );
		hObject->GetEdit()->CancelEdit();
	}
	
	CDialog::OnCancel();
}

void Dialog_Object_Template::OnSelchangedTreeComponents(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	m_grid.DeleteAllItems();

	m_grid.SetEditable( TRUE );
	m_grid.SetRowResize( TRUE );
	m_grid.SetColumnResize( TRUE );
	m_grid.SetFixedColumnSelection(TRUE);
    m_grid.SetFixedRowSelection(TRUE);
	m_grid.SetFixedRowCount( 1 );	
	m_BtnDataBase.SetGrid( &m_grid);

	// Get the gos that we are working with
	GoidColl::iterator k;
	GoHandle hObject;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		hObject = *k;
		break;
	}
	if ( hObject.IsValid() == false )
	{
		return;
	}


	CString rComponent = m_components.GetItemText( m_components.GetSelectedItem() );
	FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( rComponent.GetBuffer( 0 ) );			
	
	m_grid.InsertColumn( "Field Name" );
	m_grid.InsertColumn( "Override" );
	m_grid.InsertColumn( "Value" );
	m_grid.InsertColumn( "Documentation" );	
	
	int row = m_grid.InsertRow( pRecord->GetHeaderSpec()->GetName() );
	m_grid.SetItemText( row, 3, pRecord->GetHeaderSpec()->GetDocs() );

	FuBi::HeaderSpec::ColumnSpecColl::const_iterator iCol;
	for ( iCol = pRecord->GetHeaderSpec()->GetColumnBegin(); iCol != pRecord->GetHeaderSpec()->GetColumnEnd(); ++iCol )
	{
		int newRow = m_grid.InsertRow( (*iCol).GetName() );
		m_grid.SetItemState( newRow, 0, GVIS_READONLY );

		gpstring sValue;
		pRecord->GetAsString( (*iCol).GetName(), sValue );		


		m_grid.SetCellType( newRow, 1, RUNTIME_CLASS(CGridBtnCell) );            
        CGridBtnCell* pGridBtnCell = (CGridBtnCell*)m_grid.GetCell( newRow, 1 );
	    pGridBtnCell->SetBtnDataBase( &m_BtnDataBase);
		pGridBtnCell->SetupBtns( 0, DFC_BUTTON, DFCS_BUTTONCHECK, CGridBtnCellBase::CTL_ALIGN_CENTER, 0, FALSE, "" );

		if ( hObject->GetEdit()->GetOverride( rComponent.GetBuffer( 0 ), (*iCol).GetName() ) )
		{
			CGridBtnCell* pGridBtnCell = (CGridBtnCell*)m_grid.GetCell( newRow, 1 );
			pGridBtnCell->SetBtnDataBase( &m_BtnDataBase);												
			pGridBtnCell->SetDrawCtlState( 0, DFCS_CHECKED );			
		}
		else
		{
			CGridBtnCell* pGridBtnCell = (CGridBtnCell*)m_grid.GetCell( newRow, 1 );
			pGridBtnCell->SetBtnDataBase( &m_BtnDataBase);												
			pGridBtnCell->SetDrawCtlState( 0, DFCS_BUTTONCHECK );				
		}

		m_grid.SetItemText( newRow, 2, sValue );			
		m_grid.SetItemText( newRow, 3, (*iCol).m_Documentation );
		m_grid.SetItemState( newRow, 3, GVIS_READONLY );
	}
	
	m_grid.AutoSize();
	m_grid.AutoSizeColumns();

	*pResult = 0;
}


BOOL Dialog_Object_Template::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if ( m_grid.GetSafeHwnd() && (wParam == (WPARAM)m_grid.GetDlgCtrlID()))
    {
        *pResult = 1;
        GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;
        if (GVN_ENDLABELEDIT == pDispInfo->hdr.code)
        {            
			if (( pDispInfo->item.col == 2 ) && ( pDispInfo->item.row > 0 ))
			{
				// Get the gos that we are working with
				GoidColl::iterator k;
				for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
				{
					GoHandle hObject( *k );
				
					CString rComponent = m_components.GetItemText( m_components.GetSelectedItem() );
					FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( rComponent.GetBuffer( 0 ) );							
					FuBi::HeaderSpec::ColumnSpecColl::const_iterator iCol;
					
					CString rRow = m_grid.GetItemText( pDispInfo->item.row, 0 );
					for ( iCol = pRecord->GetHeaderSpec()->GetColumnBegin(); iCol != pRecord->GetHeaderSpec()->GetColumnEnd(); ++iCol )
					{
						if ( rRow == (*iCol).GetName() )
						{
							gpstring sOldValue;
							pRecord->GetAsString( (*iCol).GetName(), sOldValue );
							gpstring sNewValue = m_grid.GetItemText( pDispInfo->item.row, pDispInfo->item.col ).GetBuffer( 0 );
							
							if ( sOldValue.same_no_case( sNewValue ) == false )
							{
								bool bSuccess = pRecord->SetAsString( (*iCol).GetName(), sNewValue.c_str() );
								if ( !bSuccess )
								{
									MessageBox( "Invalid value passed in.  Please select a valid value.", "Data Entry Error", MB_OK );
									m_grid.SetItemText( pDispInfo->item.row, pDispInfo->item.col, sOldValue.c_str() );
								}
								else
								{
									hObject->GetEdit()->SetOverride( rComponent.GetBuffer( 0 ), (*iCol).GetName(), true );								
									CGridBtnCell* pGridBtnCell = (CGridBtnCell*)m_grid.GetCell( pDispInfo->item.row, 1 );
									pGridBtnCell->SetBtnDataBase( &m_BtnDataBase);				
									pGridBtnCell->SetDrawCtlState( 0, DFCS_CHECKED );
									m_grid.Refresh();
								}
							}
						}					
					}		
				}
			}
        }
		else if ( pDispInfo->item.col == 1 )
		{				
			// Get the gos that we are working with
			GoidColl::iterator k;
			for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
			{
				GoHandle hObject( *k );
			
			
				CString rComponent = m_components.GetItemText( m_components.GetSelectedItem() );
				FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( rComponent.GetBuffer( 0 ) );							
				FuBi::HeaderSpec::ColumnSpecColl::const_iterator iCol;
				
				CString rRow = m_grid.GetItemText( pDispInfo->item.row, 0 );
				for ( iCol = pRecord->GetHeaderSpec()->GetColumnBegin(); iCol != pRecord->GetHeaderSpec()->GetColumnEnd(); ++iCol )
				{
					if ( rRow == (*iCol).GetName() )
					{
						CGridBtnCell* pGridBtnCell = (CGridBtnCell*)m_grid.GetCell( pDispInfo->item.row, 1 );
						pGridBtnCell->SetBtnDataBase( &m_BtnDataBase);
						int test = pGridBtnCell->GetDrawCtlState( 0 );
					
						if ( pGridBtnCell->GetDrawCtlState( 0 ) == 0 )
						{
							hObject->GetEdit()->SetOverride( rComponent.GetBuffer( 0 ), (*iCol).GetName(), false );
							gpstring sOldValue;
							pRecord->GetAsString( (*iCol).GetName(), sOldValue );
							m_grid.SetItemText( pDispInfo->item.row, 2, sOldValue.c_str() );
							m_grid.Refresh();
						}
						else
						{
							hObject->GetEdit()->SetOverride( rComponent.GetBuffer( 0 ), (*iCol).GetName(), true );
						}					
					}					
				}				    
			}
		}
    }
	
	return CDialog::OnNotify(wParam, lParam, pResult);
}
