// PropPage_Properties_Template.cpp : implementation file
//

#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Object_Property_Sheet.h"
#include "PropPage_Properties_Template.h"
#include "GoEdit.h"
#include "FuBiSchema.h"
#include "gridctrl\\gridctrl.h"
#include "gridctrl\\gridbtncellcombo.h"
#include "GoData.h"
#include "GoGizmo.h"
#include "Preferences.h"
#include "EditorObjects.h"
#include "Mmsystem.h"


#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Template property page

IMPLEMENT_DYNCREATE(PropPage_Properties_Template, CResizablePage)

PropPage_Properties_Template::PropPage_Properties_Template() : CResizablePage(PropPage_Properties_Template::IDD)
{
	m_bApply = false;

	//{{AFX_DATA_INIT(PropPage_Properties_Template)
	//}}AFX_DATA_INIT
}

PropPage_Properties_Template::~PropPage_Properties_Template()
{
}

void PropPage_Properties_Template::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PropPage_Properties_Template)
	DDX_Control(pDX, IDC_CHECK_FOURCC, m_fourcc);
	DDX_Control(pDX, IDC_COMBO_TEMPLATE, m_combo_template);
	DDX_Control(pDX, IDC_CHECK_HEX, m_hex);
	DDX_Control(pDX, IDC_EDIT_SCREENNAME, m_screen_name);
	DDX_Control(pDX, IDC_EDIT_DOC, m_doc);
	DDX_Control(pDX, IDC_STATIC_REPLACE, m_replace);
	DDX_Control(pDX, IDC_STATIC_FIELDS, m_fields);	
	DDX_Control(pDX, IDC_EDIT_SCID, m_scid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(PropPage_Properties_Template, CResizablePage)
	//{{AFX_MSG_MAP(PropPage_Properties_Template)
	ON_BN_CLICKED(IDC_CHECK_HEX, OnCheckHex)
	ON_CBN_SELCHANGE(IDC_COMBO_TEMPLATE, OnEditchangeComboTemplate)
	ON_BN_CLICKED(IDC_CHECK_FOURCC, OnCheckFourcc)
	ON_WM_KILLFOCUS()
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PropPage_Properties_Template message handlers

BOOL PropPage_Properties_Template::OnInitDialog() 
{
	CResizablePage::OnInitDialog();

	RECT gridRect;
	RECT dlgRect;

	this->GetWindowRect( &dlgRect );
	
	RECT replaceRect;
	m_replace.GetWindowRect( &replaceRect );	

	RECT tabRect;
	((CPropertySheet *)this->GetParent())->GetTabControl()->GetClientRect( &tabRect );	

	gridRect.left		= replaceRect.left - dlgRect.left;
	gridRect.right		= replaceRect.right - dlgRect.left;
	gridRect.top		= replaceRect.top - dlgRect.top;
	gridRect.bottom		= replaceRect.bottom - dlgRect.top;
	
	m_grid.Create( gridRect, this, IDC_GRIDCONTROL, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	

	// Resizability Anchors	
	AddAnchor( IDC_STATIC_FIELDS, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_GRIDCONTROL, TOP_LEFT, BOTTOM_RIGHT );
	
	AddAnchor( IDC_EDIT_SCID, TOP_LEFT );
	AddAnchor( IDC_COMBO_TEMPLATE, TOP_LEFT );

	AddAnchor( IDC_STATIC_TEMPLATE, TOP_LEFT );
	AddAnchor( IDC_STATIC_SCID, TOP_LEFT );

	AddAnchor( IDC_STATIC_SCREENNAME, TOP_LEFT	);
	AddAnchor( IDC_EDIT_SCREENNAME, TOP_LEFT	);	

	AddAnchor( IDC_STATIC_DOC, TOP_LEFT );
	AddAnchor( IDC_EDIT_DOC, TOP_LEFT );

	m_grid.SetEditable();
	m_grid.SetRowResize();
	m_grid.SetColumnResize();
	m_grid.SetFixedColumnSelection(TRUE);
    m_grid.SetFixedRowSelection(TRUE);
	m_grid.SetFixedRowCount();

	m_replace.ShowWindow( SW_HIDE );

	m_hex.SetCheck( (int)gPreferences.GetDisplayHexadecimal() );
	m_fourcc.SetCheck( (int)gPreferences.GetDisplayFourCC() );

	const ComponentTemplateMap& templates = gComponentList.GetTemplateMap();
	ComponentTemplateMap::const_iterator i;
	for ( i = templates.begin(); i != templates.end(); ++i )
	{
		m_combo_template.AddString( (*i).first.c_str() );
	}
	
	Reset();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void PropPage_Properties_Template::Reset()
{
	if ( !GetSafeHwnd() )
	{
		return;
	}

	m_componentMap.clear();
	m_bApply = false;
	gObjectProperties.Evaluate();

	GoidColl::iterator iSel;
	for ( iSel = m_Selected.begin(); iSel != m_Selected.end(); ++iSel )
	{
		GoHandle hObject( *iSel );
		if ( hObject.IsValid() )
		{
			if ( hObject->HasEdit() && hObject->GetEdit()->IsEditing() )
			{
				hObject->GetEdit()->CancelEdit();
			}
		}
	}

	m_Selected.clear();

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
		else
		{
			return;
		}

		if ( objectIds.size() == 1 )
		{
			gpstring sTemp; 
			sTemp.assignf( "0x%08X", MakeInt(hObject->GetScid()) );
			m_scid.SetWindowText( sTemp.c_str() );		
			
			m_combo_template.SetCurSel( m_combo_template.FindString( 0, hObject->GetTemplateName() ) );
			m_screen_name.SetWindowText( ToAnsi( hObject->GetCommon()->GetScreenName().c_str() ) );
			m_doc.SetWindowText( hObject->GetDataTemplate()->GetExtraDocs().c_str() );
		}		
	}
	
	if ( m_Selected.size() == 0 )
	{
		return;
	}
	
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
				ComponentRefMap::iterator iFound = m_componentMap.find( sComponent );
				if ( iFound != m_componentMap.end() )
				{
					(*iFound).second++;
				}
				else
				{
					m_componentMap.insert( ComponentRefPair( sComponent, 1 ) );
				}				
			}
		}		
	}

	m_grid.DeleteAllItems();

	m_grid.SetEditable( TRUE );
	m_grid.SetRowResize( TRUE );
	m_grid.SetColumnResize( TRUE );
	m_grid.SetFixedColumnSelection(TRUE);
    m_grid.SetFixedRowSelection(TRUE);
	m_grid.SetFixedRowCount( 1 );	
	m_BtnDataBase.SetGrid( &m_grid);

	m_grid.InsertColumn( "Field Name" );
	m_grid.InsertColumn( "Override" );
	m_grid.InsertColumn( "Value" );
	m_grid.InsertColumn( "Documentation" );	
	
	// Get the gos that we are working with
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

	ComponentRefMap::iterator l;
	for ( l = m_componentMap.begin(); l != m_componentMap.end(); ++l )
	{
		if ( (*l).second == m_Selected.size() )
		{
			FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( (*l).first.c_str() );
			
			if ( gEditorObjects.IsHiddenComponent( pRecord->GetHeaderSpec()->GetName() ) )
			{
				continue;
			}

			int row = m_grid.InsertRow( pRecord->GetHeaderSpec()->GetName() );
			LOGFONT newFont = (LOGFONT)(*(m_grid.GetItemFont( row, 0 )));
			newFont.lfWeight = FW_BOLD;
			m_grid.SetItemFont( row, 0, &newFont );
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
				pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );
				pGridBtnCell->SetupBtns( 0, DFC_BUTTON, DFCS_BUTTONCHECK, CGridBtnCellBase::CTL_ALIGN_CENTER, 0, FALSE, "" );

				if ( hObject->GetEdit()->GetOverride( (*l).first.c_str(), (*iCol).GetName() ) )
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

				if ( gPreferences.GetDisplayHexadecimal() )
				{
					if ( (*iCol).m_Type == FuBi::VAR_INT32 || (*iCol).m_Type == FuBi::VAR_UINT32 )
					{
						int number = 0;
						stringtool::Get( sValue, number );
						sValue.assignf( "0x%08X", number );
					}
				}
				else if ( gPreferences.GetDisplayFourCC() )
				{
					if ( (*iCol).m_Type == FuBi::VAR_INT32 || (*iCol).m_Type == FuBi::VAR_UINT32 )
					{
						int number = 0;
						stringtool::Get( sValue, number );
						sValue.assignf( "%c%c%c%c", (char)(number >> 24), (char)(number >> 16), (char)(number >> 8), (char)number );
					}
				}

				if( (*iCol).GetConstraint() && ((*iCol).GetConstraint()->m_Type == FuBi::ConstrainStrings::TYPE_STRINGS) )
				{
					m_grid.SetCellType( newRow, 2, RUNTIME_CLASS(CGridBtnCellCombo) );
					CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( newRow, 2 );
					pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
					pGridBtnCell->SetComboStyle( CBS_DROPDOWN );

					CStringArray sa;
					FuBi::ConstrainStrings::StringColl strings;
					FuBi::ConstrainStrings::StringColl::iterator iString;
					((FuBi::ConstrainStrings *)(*iCol).GetConstraint())->OnGetStrings( strings );
					for ( iString = strings.begin(); iString != strings.end(); ++iString )
					{
						sa.Add( (*iString).m_String.c_str() );
					}
					pGridBtnCell->SetComboString( sa );
				}
				
				m_grid.SetItemText( newRow, 2, sValue );			
				m_grid.SetItemText( newRow, 3, (*iCol).m_Documentation );
				m_grid.SetItemState( newRow, 3, GVIS_READONLY );
			}

			m_grid.InsertRow( "" );
		}
	}	
	
	m_grid.AutoSize();
	m_grid.AutoSizeColumns();
}


void PropPage_Properties_Template::Apply()
{
	GoidColl::iterator k;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		GoHandle hObject( *k );
		if ( hObject.IsValid() )
		{
			hObject->GetEdit()->EndEdit();
			if (hObject->HasAspect())
			{
				hObject->GetAspect()->GetAspectHandle()->Reconstitute();
			}
			else if (hObject->HasGizmo())
			{
				hObject->GetGizmo()->GetAspectHandle()->Reconstitute();
			}
		}
	}
	m_bApply = false;
}


void PropPage_Properties_Template::Cancel()
{
	GoidColl::iterator k;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		GoHandle hObject( *k );
		if ( hObject.IsValid() )
		{
			if ( hObject->GetEdit()->IsEditing() )
			{
				hObject->GetEdit()->CancelEdit();
			}
		}
	}
}

BOOL PropPage_Properties_Template::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
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
					ComponentRefMap::iterator l;
					for ( l = m_componentMap.begin(); l != m_componentMap.end(); ++l )
					{
						if ( (*l).second == m_Selected.size() )
						{
							FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( (*l).first.c_str() );							
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
										if ( gPreferences.GetDisplayHexadecimal() )
										{
											if ( (*iCol).m_Type == FuBi::VAR_INT32 || (*iCol).m_Type == FuBi::VAR_UINT32 )
											{
												int number = 0;
												stringtool::Get( sNewValue, number );
												sNewValue.assignf( "0x%08X", number );
												m_grid.SetItemText( pDispInfo->item.row, pDispInfo->item.col, sNewValue.c_str() );
											}
										}
									
										bool bSuccess = pRecord->SetAsString( (*iCol).GetName(), sNewValue.c_str() );
										if ( !bSuccess )
										{
											// Convert to FourCC if possible											
											if ( (*iCol).m_Type.IsSimpleInteger() && sNewValue.size() == 4 )
											{												
												char one	= sNewValue[3];
												char two	= sNewValue[2];
												char three	= sNewValue[1];
												char four	= sNewValue[0];
												int number = 0;
												number |= four << 24;
												number |= three << 16;
												number |= two << 8;
												number |= one;												
																								
												//fourCC = mmioStringToFOURCC( sNewValue.c_str(), 0 );
												bSuccess = pRecord->Set( (*iCol).GetName(), number );
											}	
											
											if ( !bSuccess )
											{
												MessageBox( "Invalid value passed in.  Please select a valid value.", "Data Entry Error", MB_OK );
												m_grid.SetItemText( pDispInfo->item.row, pDispInfo->item.col, sOldValue.c_str() );
											}
										}
										else
										{
											hObject->GetEdit()->SetOverride( (*l).first.c_str(), (*iCol).GetName(), true );								
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

				m_bApply = true;
				gObjectProperties.Evaluate();
			}
        }
		else if ( pDispInfo->item.col == 1 )
		{				
			// Get the gos that we are working with
			GoidColl::iterator k;
			for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
			{
				GoHandle hObject( *k );
			
			
				ComponentRefMap::iterator l;
				for ( l = m_componentMap.begin(); l != m_componentMap.end(); ++l )
				{
					if ( (*l).second == m_Selected.size() )
					{
						FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( (*l).first.c_str() );							
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
									hObject->GetEdit()->SetOverride( (*l).first.c_str(), (*iCol).GetName(), false );
									gpstring sOldValue;
									pRecord->GetAsString( (*iCol).GetName(), sOldValue );
									m_grid.SetItemText( pDispInfo->item.row, 2, sOldValue.c_str() );
									m_grid.Refresh();
								}
								else
								{
									hObject->GetEdit()->SetOverride( (*l).first.c_str(), (*iCol).GetName(), true );
								}					
							}					
						}				    
					}
				}
			}

			m_bApply = true;
			gObjectProperties.Evaluate();
		}
    }
	
	return CResizablePage::OnNotify(wParam, lParam, pResult);
}

void PropPage_Properties_Template::OnCheckHex() 
{
	gPreferences.SetDisplayHexadecimal( m_hex.GetCheck() ? true : false );
	if ( m_hex.GetCheck() )
	{
		m_fourcc.SetCheck( 0 );	
		gPreferences.SetDisplayFourCC( false );
	}

	gObjectProperties.Reset();	
}

void PropPage_Properties_Template::OnEditchangeComboTemplate() 
{
	if ( m_Selected.size() == 0 )
	{
		return;
	}

	CString rTemplate;
	m_combo_template.GetLBText( m_combo_template.GetCurSel(), rTemplate );

	GoidColl::iterator k;
	for ( k = m_Selected.begin(); k != m_Selected.end(); ++k )
	{
		gEditorObjects.SwitchObject( *k, rTemplate.GetBuffer( 0 ) );
	}	
}

void PropPage_Properties_Template::OnCheckFourcc() 
{
	gPreferences.SetDisplayFourCC( m_fourcc.GetCheck() ? true : false );
	if ( m_fourcc.GetCheck() )
	{
		m_hex.SetCheck( 0 );
		gPreferences.SetDisplayHexadecimal( false );
	}	

	gObjectProperties.Reset();
}

void PropPage_Properties_Template::OnKillFocus(CWnd* pNewWnd) 
{	
	CResizablePage::OnKillFocus(pNewWnd);		
}

void PropPage_Properties_Template::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CResizablePage::OnShowWindow(bShow, nStatus);	
	ArrangeLayout();
	Invalidate( TRUE );	
	UpdateWindow();		
}
