// Dialog_Sfx_Editor.cpp : implementation file
//


#include "PrecompEditor.h"
#include "siegeeditor.h"
#include "Dialog_Sfx_Editor.h"
#include "gridctrl\\gridctrl.h"
#include "gridctrl\\gridbtncellcombo.h"
#include "Dialog_Sfx_New_Script.h"
#include "EditorSfx.h"
#include "fueldb.h"
#include "Flamethrower.h"
#include "Flamethrower_interpreter.h"
#include "WorldMessage.h"
#include "Dialog_Sfx_Parameters.h"
#include "Dialog_Sfx_Load_Script.h"
#include "WorldFx.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Editor dialog


Dialog_Sfx_Editor::Dialog_Sfx_Editor(CWnd* pParent /*=NULL*/)
	: CResizableDialog(Dialog_Sfx_Editor::IDD, pParent)
	, m_bSelectPrimary( false )
	, m_bSelectSecondary( false )
{
	//{{AFX_DATA_INIT(Dialog_Sfx_Editor)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void Dialog_Sfx_Editor::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(Dialog_Sfx_Editor)
	DDX_Control(pDX, IDC_EDIT_DEST, m_edit_dest);
	DDX_Control(pDX, IDC_EDIT_SOURCE, m_edit_source);
	DDX_Control(pDX, IDC_STATIC_DELETE, m_static_delete);
	DDX_Control(pDX, IDC_STATIC_TESTING, m_static_testing);
	DDX_Control(pDX, IDC_STATIC_SOURCE, m_static_source);
	DDX_Control(pDX, IDC_STATIC_DEST, m_static_dest);
	DDX_Control(pDX, IDC_STATIC_SCRIPT, m_static_script);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(Dialog_Sfx_Editor, CResizableDialog)
	//{{AFX_MSG_MAP(Dialog_Sfx_Editor)
	ON_BN_CLICKED(IDC_BUTTON_NEW_SCRIPT, OnButtonNewScript)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_SCRIPT, OnButtonLoadScript)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_SCRIPT, OnButtonSaveScript)
	ON_BN_CLICKED(IDC_BUTTON_STOP_SCRIPTS, OnButtonStopScripts)
	ON_BN_CLICKED(IDC_BUTTON_RUN_SCRIPT, OnButtonRunScript)
	ON_BN_CLICKED(IDC_BUTTON_ADD_COMMAND, OnButtonAddCommand)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_PARAMETERS, OnButtonEditParameters)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_COMMAND, OnButtonRemoveCommand)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_PRIMARY, OnButtonSelectPrimary)
	ON_BN_CLICKED(IDC_BUTTON_SELECT_SECONDARY, OnButtonSelectSecondary)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Editor message handlers

BOOL Dialog_Sfx_Editor::OnInitDialog() 
{
	CResizableDialog::OnInitDialog();
	
	RECT dlgRect;
	this->GetWindowRect( &dlgRect );
	
	RECT replaceRect;
	m_static_delete.GetWindowRect( &replaceRect );	

	RECT gridRect;
	gridRect.left		= replaceRect.left - dlgRect.left;
	gridRect.right		= replaceRect.right - dlgRect.left;
	gridRect.top		= replaceRect.top - dlgRect.top;
	gridRect.bottom		= replaceRect.bottom - dlgRect.top;

	m_grid.Create( gridRect, this, IDC_SFX_GRIDCONTROL, WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VISIBLE | WS_HSCROLL );	
	ShowSizeGrip( TRUE );

	WINDOWPLACEMENT wp;
	m_static_delete.GetWindowPlacement( &wp );
	m_grid.SetWindowPlacement( &wp );

	AddAnchor( IDC_SFX_GRIDCONTROL, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_EDIT_DEST, BOTTOM_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_EDIT_SOURCE, BOTTOM_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_STATIC_TESTING, BOTTOM_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_STATIC_SOURCE, BOTTOM_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_STATIC_DEST, BOTTOM_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDC_STATIC_SCRIPT, TOP_LEFT, BOTTOM_RIGHT );
	AddAnchor( IDOK, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_NEW_SCRIPT, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_LOAD_SCRIPT, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_SAVE_SCRIPT, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_HELP, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_STOP_SCRIPTS, BOTTOM_RIGHT );
	AddAnchor( IDC_BUTTON_RUN_SCRIPT, BOTTOM_RIGHT );
	AddAnchor( IDC_BUTTON_ADD_COMMAND, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_EDIT_PARAMETERS, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_REMOVE_COMMAND, TOP_RIGHT );
	AddAnchor( IDC_BUTTON_SELECT_PRIMARY, BOTTOM_RIGHT );
	AddAnchor( IDC_BUTTON_SELECT_SECONDARY, BOTTOM_RIGHT );

	BuildGrid();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void Dialog_Sfx_Editor::BuildGrid()
{
	m_grid.DeleteAllItems();	
	m_grid.SetEditable( TRUE );
	m_grid.SetRowResize( TRUE );
	m_grid.SetColumnResize( TRUE );
	m_grid.SetFixedColumnSelection(TRUE);
    m_grid.SetFixedRowSelection(TRUE);
	m_grid.SetFixedRowCount( 1 );	
	m_BtnDataBase.SetGrid( &m_grid);
	m_grid.InsertColumn( "Command" );
	m_grid.InsertColumn( "Sub-commands" );	
	m_grid.InsertColumn( "Type" );
	m_grid.InsertColumn( "Value" );
	m_grid.InsertColumn( "Documentation" );
}

void Dialog_Sfx_Editor::OnButtonNewScript() 
{
	Dialog_Sfx_New_Script dlg;
	if ( dlg.DoModal() != IDOK )
	{
		return;
	}

	BuildGrid();
	gpstring sNew = "Script - ";
	sNew += gEditorSfx.GetSelectedScript();
	m_static_script.SetWindowText( sNew.c_str() );		
}

void Dialog_Sfx_Editor::OnButtonLoadScript() 
{	
	Dialog_Sfx_Load_Script dlg;
	if ( dlg.DoModal() == IDOK )
	{
		gpstring sNew = "Script - ";
		sNew += gEditorSfx.GetSelectedScript();
		m_static_script.SetWindowText( sNew.c_str() );	
		BuildGrid();
		BuildGridFromScript( gEditorSfx.GetSelectedScriptHandle() );	
	}
}

void Dialog_Sfx_Editor::OnButtonSaveScript() 
{
	FuelHandle hScript = gEditorSfx.GetSelectedScriptHandle();
	if ( hScript.IsValid() )
	{		
		FuelHandle hNew = hScript->GetParentBlock();
		hNew->DestroyChildBlock( hScript );
		gpstring sFile = gEditorSfx.GetSelectedScript();
		sFile += ".gas";
		hNew = hNew->CreateChildBlock( "effect_script*", sFile.c_str() );		
		hNew->Set( "name", gEditorSfx.GetSelectedScript() );
		hNew->Set( "editor_generated", true );
		BuildScriptFromGrid( hNew );
		hNew->GetDB()->SaveChanges();
	}	
}

void Dialog_Sfx_Editor::OnButtonStopScripts() 
{
	gWorldFx.StopAllScripts();	
}

void Dialog_Sfx_Editor::OnButtonRunScript() 
{
	if ( gEditorSfx.GetSelectedScript().empty() )
	{
		return;
	}

	OnButtonSaveScript();

	CString rSource;
	CString rDest;
	m_edit_source.GetWindowText( rSource );
	m_edit_dest.GetWindowText( rDest );

	int goid = 0;
	if ( rSource.IsEmpty() )
	{
		return;
	}

	stringtool::Get( rSource.GetBuffer( 0 ), goid );
	Goid source = MakeGoid( goid );

	Goid dest = GOID_INVALID;
	if ( !rDest.IsEmpty() )
	{
		stringtool::Get( rDest.GetBuffer( 0 ), goid );
		dest = MakeGoid( goid );
	}

	gWorldFx.StopAllScripts();	
	gWorldFx.RebuildScriptVocabulary();

	gpstring sParams;
	
	FuelHandle hScript = gEditorSfx.GetSelectedScriptHandle();
	hScript->Get( "script", sParams );

	if ( !rSource.IsEmpty() && !rDest.IsEmpty() )
	{
		gWorldFx.RunScript( gEditorSfx.GetSelectedScript(), dest, source, sParams, GOID_INVALID, WE_UNKNOWN );
	}
	else
	{
		gWorldFx.RunScript( gEditorSfx.GetSelectedScript(), source, source, sParams, GOID_INVALID, WE_UNKNOWN );
	}
}


void Dialog_Sfx_Editor::OnButtonAddCommand() 
{	
	if ( gEditorSfx.GetSelectedScript().empty() )
	{
		MessageBox( "Please Load or create a New script.", "Error", MB_OK );
		return;
	}

	int row = m_grid.InsertRow( "" );
	m_grid.SetCellType( row, COMMAND_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
	
	CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, COMMAND_COL );
	pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
	pGridBtnCell->SetComboStyle( CBS_DROPDOWN );

	m_grid.SetItemData( row, COMMAND_COL, row+1 );

	CStringArray sa;

	CommandVec cmdVec;
	CommandVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetCommands( cmdVec );
	for ( i = cmdVec.begin(); i != cmdVec.end(); ++i ) 
	{
		sa.Add( (*i).sName.c_str() );	
	}	
	pGridBtnCell->SetComboString( sa );
	m_grid.SetItemBkColour( row, COMMAND_COL, 0x00d2d255 );

	m_grid.Refresh();
	
}

void Dialog_Sfx_Editor::OnButtonEditParameters() 
{
	CCellRange range = m_grid.GetSelectedCellRange();
	if (( range.GetMaxRow() == -1 ) || ( range.GetMaxCol() == -1 ))
	{
		return;
	}

	if ( range.Count() == 1 )
	{
		int row = range.GetMinRow();
		int col = range.GetMinCol();
		
		CString rText	= m_grid.GetItemText( row, col-1 );
		CString rEffect = m_grid.GetItemText( row-1, col );
		int search = 2;
		while ( !gEditorSfx.IsEffect( rEffect.GetBuffer( 0 ) ) )
		{
			rEffect = m_grid.GetItemText( row-search, col );
			search++;

			if ( GetPreviousCommandRow( row ) >= row-search )
			{
				return;
			}
		}

		if ( rText.CompareNoCase( "effect_parameters" ) == 0 && !rEffect.IsEmpty() )
		{
			gEditorSfx.SetCurrentEffectParams( m_grid.GetItemText( row, col ).GetBuffer( 0 ) );			
			gEditorSfx.SetCurrentEffect( rEffect.GetBuffer( 0 ) );

			Dialog_Sfx_Parameters dlg;
			if ( dlg.DoModal() == IDOK )
			{
				m_grid.SetItemText( row, col, gEditorSfx.GetCurrentEffectParams() );
			}
		}		
	}
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
}

void Dialog_Sfx_Editor::OnButtonRemoveCommand() 
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
			int maxRow = GetNextCommandRow( row );
			
			for ( int i = 0; i != (maxRow-row); ++i )
			{
				m_grid.DeleteRow( row );				
			}
		}
	}
	
	m_grid.AutoSizeColumns();

	m_grid.Refresh();
}


int	Dialog_Sfx_Editor::GetNextCommandRow( int row )
{
	for ( int i = row+1; i != m_grid.GetRowCount(); ++i )
	{
		CString rCommand = m_grid.GetItemText( i, COMMAND_COL );
		if ( !rCommand.IsEmpty() )
		{
			return i;
		}
	}

	return m_grid.GetRowCount();
}


void Dialog_Sfx_Editor::BuildScriptFromGrid( FuelHandle hScript )
{
	if ( hScript.IsValid() )
	{		
		gpstring sLine;
		sLine.append( "[[\n" );
		bool bFirst = true;
		for ( int row = 1; row != m_grid.GetRowCount(); ++row )
		{			
			CString rCommand = m_grid.GetItemText( row, COMMAND_COL );
			if ( !rCommand.IsEmpty() )
			{
				if ( !sLine.empty() && !bFirst )
				{
					sLine += ";\n";										
				}	

				bFirst = false;
				
				sLine += rCommand.GetBuffer( 0 );
				continue;
			}
			
			CString rSubCommand1 = m_grid.GetItemText( row, SUBCOMMAND1_COL );
			if ( !rSubCommand1.IsEmpty() )
			{
				sLine += " ";
				sLine += rSubCommand1.GetBuffer( 0 );
				continue;
			}
	
			CString rValue = m_grid.GetItemText( row, VALUE_COL );
			if ( !rValue.IsEmpty() )
			{
				sLine += " ";

				CString rType = m_grid.GetItemText( row, TYPE_COL );
				bool bEffectParameter = false;
				if ( rType.CompareNoCase( "effect_parameters" ) == 0 )
				{
					bEffectParameter = true;
				}

				if ( bEffectParameter )
				{
					sLine += "\"";
				}
				sLine += rValue.GetBuffer( 0 );
				if ( bEffectParameter )
				{
					sLine += "\"";
				}
				continue;
			}			
		}

		if ( !sLine.empty() )
		{
			sLine += ";\n";			
		}	

		sLine.append( "]]" );
		
		hScript->Set( "script", sLine.c_str() );
	}
}


void Dialog_Sfx_Editor::BuildGridFromScript( FuelHandle hScript )
{
	if ( hScript.IsValid() )
	{
		gpstring sScript;
		hScript->Get( "script", sScript );
		stringtool::ReplaceAllChars( sScript, '\n', ' ' );
		stringtool::ReplaceAllChars( sScript, '\r', ' ' );

		int newPos = sScript.find_first_of( "\r" );
		while ( newPos != gpstring::npos )
		{
			sScript.erase( newPos );
			newPos = sScript.find_first_of( "\r" );
		}

		int newRow = 0;		
		
		gpstring sCurrentCommand;
		int pos = sScript.find_first_of( " " );
		while ( pos != gpstring::npos )
		{		
			gpstring sNew = sScript.substr( 0, pos );			
			sScript = sScript.substr( pos+1, sScript.size() );
			pos = sScript.find_first_of( " " );	
			
			if ( sNew.same_no_case( "[[" ) || sNew.same_no_case( "]]" ) )
			{
				continue;
			}

			if ( sNew[sNew.size()-1] == ';' )
			{
				sNew = sNew.substr( 0, sNew.size() - 1 );
			}

			bool bContinue = false;
			
			// Search for the string in the commands
			{
				CommandVec commands;
				CommandVec::iterator i;
				gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );
				for ( i = commands.begin(); i != commands.end(); ++i )
				{
					if ( (*i).sName.same_no_case( sNew ) )
					{
						sCurrentCommand = sNew;

						newRow = m_grid.InsertRow( sNew.c_str() );
						m_grid.SetItemData( newRow, COMMAND_COL, newRow+1 );
						m_grid.SetItemState( newRow, COMMAND_COL, GVIS_READONLY );
						m_grid.SetItemBkColour( newRow, COMMAND_COL, 0x00d2d255 );
						m_grid.SetItemText( newRow, DOC_COL, (*i).sDoc.c_str() );
						bContinue = true;
						BuildArgumentsForCommand( sNew, newRow );

						m_rowTypes.clear();
						break;
					}
				}
			}

			if ( bContinue )
			{
				continue;
			}

			// Search for the string in the subcommands
			{
				CommandVec commands;
				CommandVec::iterator i;
				gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );
				for ( i = commands.begin(); i != commands.end(); ++i )
				{
					if ( (*i).sName.same_no_case( sCurrentCommand ) )
					{
						if ( RecursiveBuildGridSubCommands( sNew, (*i).subCommands ) )
						{
							bContinue = true;
							break;
						}					
					}
				}
			}

			if ( bContinue )
			{
				continue;
			}

			// The string must be an argument
			if ( sNew.size() != 0 )
			{
				int emptyRow = FindEmptyArgumentRow( newRow );

				RowTypeMap::iterator iType = m_rowTypes.find( emptyRow );
				if ( iType != m_rowTypes.end() )
				{
					while ( !IsValueValidType( sNew, (*iType).second, sCurrentCommand ) )
					{
						emptyRow++;	
						iType = m_rowTypes.find( emptyRow );
						
						if ( ( emptyRow >= m_grid.GetRowCount() ) || iType == m_rowTypes.end() )
						{
							break;
						}
					}
				}

				if ( emptyRow < m_grid.GetRowCount() )
				{
					if ( sNew[0] == '\"' )
					{
						sNew = sNew.substr( 1, sNew.size() );
					}
					if ( sNew[sNew.size()-1] == '\"' )
					{
						sNew = sNew.substr( 0, sNew.size()-1 );
					}
					m_grid.SetItemText( emptyRow, VALUE_COL, sNew.c_str() );
				}
			}
		}		
	}

	m_grid.AutoSizeColumns();
}


void Dialog_Sfx_Editor::BuildArgumentsForCommand( gpstring sCommand, int row )
{
	CommandVec commands;
	CommandVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );

	m_grid.SetItemState( row, SUBCOMMAND1_COL, GVIS_READONLY );
	m_grid.SetItemState( row, DOC_COL, GVIS_READONLY );
	m_grid.SetItemState( row, TYPE_COL, GVIS_READONLY );

	for ( i = commands.begin(); i != commands.end(); ++i ) 
	{
		if ( (*i).sName.same_no_case( sCommand ) )
		{	
			m_grid.SetItemText( row, DOC_COL, (*i).sDoc );

			ArgumentVec arguments = (*i).arguments;
			ArgumentVec::iterator j = arguments.begin();
			int order = 1;
			while ( arguments.size() != 0 )
			{
				if ( (*j).order == order )
				{
					// Setup argument
					int newRow = m_grid.InsertRow( "" );
					m_grid.SetItemBkColour( newRow, VALUE_COL, 0x00d2d255 );
					m_grid.SetItemData( newRow, COMMAND_COL, row+1 );
					SetupArgument( (*j), newRow );
					m_rowTypes.insert( RowTypePair( newRow, (*j).sType ) );

					j = arguments.erase( j );
					++order;
				}
				else
				{
					++j;
					if ( j == arguments.end() )
					{
						j = arguments.begin();
					}
				}
			}
		}
	}	
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
}


int Dialog_Sfx_Editor::FindEmptyArgumentRow( int newRow )
{
	int maxRow = m_grid.GetRowCount();
	for ( int i = newRow; i != maxRow; ++i )
	{
		CString rType	= m_grid.GetItemText( i, TYPE_COL );
		CString rValue	= m_grid.GetItemText( i, VALUE_COL );

		DWORD data = m_grid.GetItemData( i, VALUE_COL );

		if ( !rType.IsEmpty() && rValue.IsEmpty() && data != ARGUMENT_USED_ROW )
		{
			m_grid.SetItemData( i, VALUE_COL, ARGUMENT_USED_ROW );
			return i;
		}
	}

	return 0;
}


bool Dialog_Sfx_Editor::IsValueValidType( gpstring sValue, gpstring sType, gpstring sCommand )
{
	if ( sType.same_no_case( "values" ) )
	{
		CommandVec commands;
		CommandVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );
		for ( i = commands.begin(); i != commands.end(); ++i )
		{
			if ( (*i).sName.same_no_case( sCommand ) )
			{
				ArgumentVec::iterator iArg;
				for ( iArg = (*i).arguments.begin(); iArg != (*i).arguments.end(); ++iArg )
				{
					if ( (*iArg).sType.same_no_case( sType ) )
					{
						StringVec::iterator iValue;
						for ( iValue = (*iArg).values.begin(); iValue != (*iArg).values.end(); ++iValue )
						{
							if ( (*iValue).same_no_case( sValue ) )
							{
								return true;
							}
						}
					}				
				}

				return RecursiveIsValueValidType( sValue, sType, (*i).subCommands );
			}
		}			
	}
	
	if ( sType.same_no_case( "macro" ) || sType.same_no_case( "macro_variable" ) || sType.same_no_case( "macro_variable_all" ) )
	{
		StringVec macros;
		StringVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetMacros( macros );
		for ( i = macros.begin(); i != macros.end(); ++i )
		{
			if ( sValue.same_no_case( *i ) )
			{
				return true;
			}
		}		
	}	
	if ( sType.same_no_case( "macro_variable" ) || sType.same_no_case( "macro_variable_all" ) )
	{
		StringVec variables;
		StringVec::iterator i;
		GetScriptVariables( variables );
		for ( i = variables.begin(); i != variables.end(); ++i )
		{
			if ( sValue.same_no_case( *i ) )
			{
				return true;
			}
		}		
	}		
	if ( sType.same_no_case( "macro_variable_all" ) )
	{
		if ( sValue.same_no_case( "all" ) )
		{
			return true;
		}		
	}
	if ( sType.same_no_case( "effects" )  )
	{			
		EffectParametersVec effectParams;
		EffectParametersVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetEffectParameters( effectParams );
		for ( i = effectParams.begin(); i != effectParams.end(); ++i )
		{
			if ( (*i).sEffectName.same_no_case( sValue ) )
			{
				return true;
			}
		}		
	}
	if ( sType.same_no_case( "bone_names" ) )
	{
		for ( int bone = 0; bone <= (int)BoneTranslator::HANDLE; ++bone )
		{
			gpstring sBone = "@";
			sBone += (BoneTranslator::ToString( BoneTranslator::eBone(bone) ));
			
			if ( sBone.same_no_case( sValue ) )
			{
				return true;
			}
		}		
	}
	if ( sType.same_no_case( "script_names" ) )
	{
		FuelHandle hScripts( "world:global:effects" );
		if ( hScripts.IsValid() )
		{
			FuelHandleList hlScripts = hScripts->ListChildBlocks( -1, "", "effect_script*" );
			FuelHandleList::iterator i;
			for ( i = hlScripts.begin(); i != hlScripts.end(); ++i )
			{
				gpstring sName;
				(*i)->Get( "name", sName );				
				if ( sName.same_no_case( sValue ) )
				{
					return true;
				}				
			}
		}	
	}
	if ( sType.same_no_case( "worldmessages" ) )
	{
		for ( int wm = 0; wm <= (int)WE_REQ_JOB_END; ++wm )
		{
			gpstring sMessage = ::ToString( (eWorldEvent)wm );
			if ( sMessage.same_no_case( sValue ) )
			{
				return true;
			}
		}		
	}
	if ( sType.same_no_case( "edit" ) )
	{
		return true;
	}
	if ( sType.same_no_case( "effect_parameters" ) )
	{
		if ( sValue[0] == '\"' )
		{
			return true;
		}
	}

	return false;
}


bool Dialog_Sfx_Editor::RecursiveIsValueValidType( gpstring sValue, gpstring sType, SubCommandVec & subCommands )
{
	SubCommandVec::iterator j;
	for ( j = subCommands.begin(); j != subCommands.end(); ++j )
	{
		ArgumentVec::iterator iArg;
		for ( iArg = (*j).arguments.begin(); iArg != (*j).arguments.end(); ++iArg )
		{
			if ( (*iArg).sType.same_no_case( sType ) )
			{
				StringVec::iterator iValue;
				for ( iValue = (*iArg).values.begin(); iValue != (*iArg).values.end(); ++iValue )
				{
					if ( (*iValue).same_no_case( sValue ) )
					{
						return true;
					}
				}
			}				
		}

		return RecursiveIsValueValidType( sValue, sType, (*j).subCommands );		
	}

	return false;
}

bool Dialog_Sfx_Editor::RecursiveBuildGridSubCommands( gpstring sName, SubCommandVec & subCommands )
{
	SubCommandVec::iterator j;
	for ( j = subCommands.begin(); j != subCommands.end(); ++j )
	{
		if ( (*j).sName.same_no_case( sName.c_str() ) )
		{
			int row = m_grid.InsertRow( "" );
			m_grid.SetItemText( row, DOC_COL, (*j).sDoc );
			m_grid.SetItemText( row, SUBCOMMAND1_COL, (*j).sName );
			m_grid.SetItemBkColour( row, SUBCOMMAND1_COL, 0x00d2d255 );	
			m_grid.SetItemState( row, SUBCOMMAND1_COL, GVIS_READONLY );

			DWORD data = GetPreviousCommandData( row );

			ArgumentVec arguments = (*j).arguments;
			ArgumentVec::iterator k = arguments.begin();
			int order = 1;
			while ( arguments.size() != 0 )
			{
				if ( (*k).order == order )
				{
					// Setup argument							
					int newRow = m_grid.InsertRow( "" );					
					m_grid.SetItemData( newRow, COMMAND_COL, data );
					SetupArgument( (*k), newRow );
					m_grid.SetItemBkColour( newRow, VALUE_COL, 0x00d2d255 );
					m_rowTypes.insert( RowTypePair( newRow, (*k).sType ) );
					k = arguments.erase( k );									

					++order;
				}
				else
				{
					++k;
					if ( k == arguments.end() )
					{
						k = arguments.begin();
					}
				}
			}						

			return true;
		}

		if ( RecursiveBuildGridSubCommands( sName, (*j).subCommands ) )
		{
			return true;
		}
	}

	return false;
}
	

void Dialog_Sfx_Editor::OnButtonSelectPrimary() 
{
	m_bSelectSecondary = false;
	m_bSelectPrimary = true;	
}

void Dialog_Sfx_Editor::OnButtonSelectSecondary() 
{
	m_bSelectPrimary = false;
	m_bSelectSecondary = true; 	
}


void Dialog_Sfx_Editor::SetPrimaryTarget( int id )
{
	m_bSelectPrimary = false;
	m_bSelectSecondary = false;

	gpstring sTemp;
	sTemp.assignf( "0x%08X", id );
	m_edit_source.SetWindowText( sTemp.c_str() );
}


void Dialog_Sfx_Editor::SetSecondaryTarget( int id )
{
	m_bSelectPrimary = false;
	m_bSelectSecondary = false;

	gpstring sTemp;
	sTemp.assignf( "0x%08X", id );
	m_edit_dest.SetWindowText( sTemp.c_str() );
}

BOOL Dialog_Sfx_Editor::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if ( m_grid.GetSafeHwnd() && (wParam == (WPARAM)m_grid.GetDlgCtrlID()))
    {
        *pResult = 1;
        GV_DISPINFO *pDispInfo = (GV_DISPINFO*)lParam;
        if (GVN_ENDLABELEDIT == pDispInfo->hdr.code)
        { 
			// Read command information
			if ( pDispInfo->item.col == COMMAND_COL ) 
			{
				int data = m_grid.GetItemData( pDispInfo->item.row, pDispInfo->item.col );				
				if ( data == pDispInfo->item.row+1 )
				{
					CString rText = m_grid.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
					if ( !rText.IsEmpty() )
					{
						m_grid.SetItemState( pDispInfo->item.row, pDispInfo->item.col, GVIS_READONLY );

						if ( !SetupSubCommand1ForCommand( rText.GetBuffer( 0 ), pDispInfo->item.row ) )
						{
							SetupArgumentsForCommand( rText.GetBuffer( 0 ), pDispInfo->item.row );
						}
					}
				}
			}

			// Read sub-command 1 information
			if ( pDispInfo->item.col == SUBCOMMAND1_COL )
			{
				int data = m_grid.GetItemData( pDispInfo->item.row, pDispInfo->item.col );
				if ( data == pDispInfo->item.row+1 )
				{					
					CString rText = m_grid.GetItemText( pDispInfo->item.row, pDispInfo->item.col );
					if ( !rText.IsEmpty() )
					{
						m_grid.SetItemState( pDispInfo->item.row, pDispInfo->item.col, GVIS_READONLY );

						if ( !SetupSubCommand1ForSubCommand1( rText.GetBuffer( 0 ), pDispInfo->item.row ) )
						{
							SetupArgumentsForSubCommand1( rText.GetBuffer( 0 ), pDispInfo->item.row );
						}
					}
				}
			}
	
			// Value column handling
			if ( pDispInfo->item.col == VALUE_COL )
			{
				m_grid.AutoSizeColumns();
				m_grid.Refresh();
			}
		}
	}
	
	return CResizableDialog::OnNotify(wParam, lParam, pResult);
}


void Dialog_Sfx_Editor::SetupArgumentsForCommand( gpstring sCommand, int row )
{
	CommandVec commands;
	CommandVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );

	m_grid.SetItemState( row, SUBCOMMAND1_COL, GVIS_READONLY );
	m_grid.SetItemState( row, DOC_COL, GVIS_READONLY );
	m_grid.SetItemState( row, TYPE_COL, GVIS_READONLY );

	for ( i = commands.begin(); i != commands.end(); ++i ) 
	{
		if ( (*i).sName.same_no_case( sCommand ) )
		{	
			m_grid.SetItemText( row, DOC_COL, (*i).sDoc );

			ArgumentVec arguments = (*i).arguments;
			ArgumentVec::iterator j = arguments.begin();
			int order = 1;
			while ( arguments.size() != 0 )
			{
				if ( (*j).order == order )
				{
					// Setup argument
					int newRow = m_grid.InsertRow( "" );
					m_grid.SetItemBkColour( newRow, VALUE_COL, 0x00d2d255 );
					m_grid.SetItemData( newRow, COMMAND_COL, row+1 );
					SetupArgument( (*j), newRow );

					j = arguments.erase( j );
					++order;
				}
				else
				{
					++j;
					if ( j == arguments.end() )
					{
						j = arguments.begin();
					}
				}
			}
		}
	}	
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
}


bool Dialog_Sfx_Editor::SetupSubCommand1ForCommand( gpstring sCommand, int row )
{
	CommandVec commands;
	CommandVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );

	for ( i = commands.begin(); i != commands.end(); ++i ) 
	{
		if ( (*i).sName.same_no_case( sCommand ) )
		{	
			m_grid.SetItemText( row, DOC_COL, (*i).sDoc );

			if ( (*i).subCommands.size() == 0 )
			{
				return false;
			}
			
			int newRow = m_grid.InsertRow( "" );
			m_grid.SetItemBkColour( newRow, SUBCOMMAND1_COL, 0x00d2d255 );
			m_grid.SetItemData( newRow, COMMAND_COL, row+1 );

			m_grid.SetCellType( newRow, SUBCOMMAND1_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
			CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( newRow, SUBCOMMAND1_COL );
			pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
			pGridBtnCell->SetComboStyle( CBS_DROPDOWN );				
			CStringArray sa;				
			
			SubCommandVec::iterator j;
			for ( j = (*i).subCommands.begin(); j != (*i).subCommands.end(); ++j )
			{				
				sa.Add( (*j).sName.c_str() );							
			}

			pGridBtnCell->SetComboString( sa );

			m_grid.SetItemData( newRow, SUBCOMMAND1_COL, newRow+1 );
			
			m_grid.AutoSizeColumns();
			m_grid.Refresh();			

			if ( (*i).subCommands.size() != 0 )
			{
				return true;
			}
		}
	}
		
	m_grid.AutoSizeColumns();
	m_grid.Refresh();

	return false;
}


void Dialog_Sfx_Editor::SetupArgumentsForSubCommand1( gpstring sSubCommand1, int row )
{
	gpstring sCommand = GetPreviousCommandName( row );	
	DWORD data = GetPreviousCommandData( row );
	CommandVec commands;
	CommandVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );

	for ( i = commands.begin(); i != commands.end(); ++i ) 
	{
		if ( (*i).sName.same_no_case( sCommand.c_str() ) )
		{	
			RecursiveSetupArgumentsForSubCommand( sSubCommand1, row, (*i).subCommands );			
		}
	}

	m_grid.AutoSizeColumns();	
	m_grid.Refresh();
}


gpstring Dialog_Sfx_Editor::GetPreviousCommandName( int row )
{
	for ( int i = row; i != 0; --i )
	{
		CString rCommand = m_grid.GetItemText( i, COMMAND_COL );
		if ( !rCommand.IsEmpty() )
		{
			return rCommand.GetBuffer( 0 );
		}
	}

	return "";
}


int Dialog_Sfx_Editor::GetPreviousCommandData( int row )
{
	for ( int i = row; i != 0; --i )
	{
		int data = m_grid.GetItemData( i, COMMAND_COL );		
		if ( data != 0 )
		{
			return data;
		}
	}

	return 0;
}


int Dialog_Sfx_Editor::GetPreviousCommandRow( int row )
{
	for ( int i = row; i != 0; --i )
	{
		CString rCommand = m_grid.GetItemText( i, COMMAND_COL );
		if ( !rCommand.IsEmpty() )
		{
			return i;
		}
	}

	return 0;
}


bool Dialog_Sfx_Editor::RecursiveSetupArgumentsForSubCommand( gpstring sName, int row, SubCommandVec & subCommands )
{	
	DWORD data = GetPreviousCommandData( row );
	
	SubCommandVec::iterator j;
	for ( j = subCommands.begin(); j != subCommands.end(); ++j )
	{
		if ( (*j).sName.same_no_case( sName.c_str() ) )
		{
			m_grid.SetItemText( row, DOC_COL, (*j).sDoc );

			ArgumentVec arguments = (*j).arguments;
			ArgumentVec::iterator k = arguments.begin();
			int order = 1;
			while ( arguments.size() != 0 )
			{
				if ( (*k).order == order )
				{
					// Setup argument							
					int newRow = m_grid.InsertRow( "" );
					m_grid.SetItemBkColour( newRow, VALUE_COL, 0x00d2d255 );
					m_grid.SetItemData( newRow, COMMAND_COL, data );
					SetupArgument( (*k), newRow );

					k = arguments.erase( k );
					++order;
				}
				else
				{
					++k;
					if ( k == arguments.end() )
					{
						k = arguments.begin();
					}
				}
			}						

			return true;
		}

		RecursiveSetupArgumentsForSubCommand( sName, row, (*j).subCommands );
	}

	return false;
}


bool Dialog_Sfx_Editor::SetupSubCommand1ForSubCommand1( gpstring sSubCommand1, int row )
{
	CString rCommand = GetPreviousCommandName( row ).c_str();
	DWORD data = GetPreviousCommandData( row );
	CommandVec commands;
	CommandVec::iterator i;
	gEditorSfx.GetEffectSchemaDb()->GetCommands( commands );

	for ( i = commands.begin(); i != commands.end(); ++i ) 
	{
		if ( (*i).sName.same_no_case( rCommand.GetBuffer( 0 ) ) )
		{	
			SubCommandVec::iterator j;
			for ( j = (*i).subCommands.begin(); j != (*i).subCommands.end(); ++j )
			{
				if ( (*j).sName.same_no_case( sSubCommand1 ) )
				{
					if ( (*j).subCommands.size() == 0 )
					{
						return false;
					}
										
					int newRow = m_grid.InsertRow( "" );
					m_grid.SetItemBkColour( newRow, SUBCOMMAND1_COL, 0x00d2d255 );
					m_grid.SetItemData( newRow, COMMAND_COL, data );
					m_grid.SetCellType( newRow, SUBCOMMAND1_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
					CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( newRow, SUBCOMMAND1_COL );
					pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
					pGridBtnCell->SetComboStyle( CBS_DROPDOWN );				
					CStringArray sa;				

					SubCommandVec::iterator k;
					for ( k = (*j).subCommands.begin(); k != (*j).subCommands.end(); ++k )
					{				
						sa.Add( (*k).sName.c_str() );							
					}

					pGridBtnCell->SetComboString( sa );

					m_grid.SetItemData( newRow, SUBCOMMAND1_COL, newRow+1 );
					
					m_grid.AutoSizeColumns();
					m_grid.Refresh();

					if ( (*j).subCommands.size() != 0 )
					{
						return true;
					}
				}
			}			
		}
	}	
	
	m_grid.AutoSizeColumns();
	m_grid.Refresh();

	return false;
}


void Dialog_Sfx_Editor::SetupArgument( Argument arg, int row )
{
	m_grid.SetItemText( row, DOC_COL, arg.sDoc );
	m_grid.SetItemText( row, TYPE_COL, arg.sType );
	
	m_grid.SetItemState( row, DOC_COL, GVIS_READONLY );
	m_grid.SetItemState( row, TYPE_COL, GVIS_READONLY );

	if ( arg.sType.same_no_case( "values" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;
		StringVec::iterator i;
		for ( i = arg.values.begin(); i != arg.values.end(); ++i )
		{
			sa.Add( (*i).c_str() );
		}

		pGridBtnCell->SetComboString( sa );
	}
	else if ( arg.sType.same_no_case( "macro" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;
		StringVec macros;
		StringVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetMacros( macros );
		for ( i = macros.begin(); i != macros.end(); ++i )
		{
			sa.Add( (*i).c_str() );
		}

		pGridBtnCell->SetComboString( sa );
	}
	else if ( arg.sType.same_no_case( "macro_variable" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;
		StringVec macros;
		StringVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetMacros( macros );
		for ( i = macros.begin(); i != macros.end(); ++i )
		{
			sa.Add( (*i).c_str() );
		}

		StringVec variables;
		GetScriptVariables( variables );
		for ( i = variables.begin(); i != variables.end(); ++i )
		{
			sa.Add( (*i).c_str() );
		}

		pGridBtnCell->SetComboString( sa );		
	}	
	else if ( arg.sType.same_no_case( "macro_variable_all" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;
		StringVec macros;
		StringVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetMacros( macros );
		for ( i = macros.begin(); i != macros.end(); ++i )
		{
			sa.Add( (*i).c_str() );
		}

		StringVec variables;
		GetScriptVariables( variables );
		for ( i = variables.begin(); i != variables.end(); ++i )
		{
			sa.Add( (*i).c_str() );
		}

		sa.Add( "all" );

		pGridBtnCell->SetComboString( sa );
	}
	else if ( arg.sType.same_no_case( "effects" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;
		
		EffectParametersVec effectParams;
		EffectParametersVec::iterator i;
		gEditorSfx.GetEffectSchemaDb()->GetEffectParameters( effectParams );
		for ( i = effectParams.begin(); i != effectParams.end(); ++i )
		{
			sa.Add( (*i).sEffectName.c_str() );
		}

		pGridBtnCell->SetComboString( sa );
	}
	else if ( arg.sType.same_no_case( "bone_names" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;

		for ( int bone = 0; bone <= (int)BoneTranslator::HANDLE; ++bone )
		{
			gpstring sBone = "@";
			sBone += (BoneTranslator::ToString( BoneTranslator::eBone(bone) ));
			sa.Add( sBone.c_str() );
		}

		pGridBtnCell->SetComboString( sa );
	}
	else if ( arg.sType.same_no_case( "script_names" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );
		
		CStringArray sa;

		FuelHandle hScripts( "world:global:effects" );
		if ( hScripts.IsValid() )
		{
			FuelHandleList hlScripts = hScripts->ListChildBlocks( -1, "", "effect_script*" );
			FuelHandleList::iterator i;
			for ( i = hlScripts.begin(); i != hlScripts.end(); ++i )
			{
				gpstring sName;
				(*i)->Get( "name", sName );

				sa.Add( sName.c_str() );
			}
		}

		pGridBtnCell->SetComboString( sa );
	}
	else if ( arg.sType.same_no_case( "worldmessages" ) )
	{
		m_grid.SetCellType( row, VALUE_COL, RUNTIME_CLASS(CGridBtnCellCombo) );
		CGridBtnCellCombo * pGridBtnCell = (CGridBtnCellCombo*)m_grid.GetCell( row, VALUE_COL );
		pGridBtnCell->SetBtnDataBase( &m_BtnDataBase );												
		pGridBtnCell->SetComboStyle( CBS_DROPDOWN );		
		
		CStringArray sa;

		for ( int wm = 0; wm <= (int)WE_COUNT; ++wm )
		{
			gpstring sMessage = ::ToString( (eWorldEvent)wm );
			sa.Add( sMessage.c_str() );
		}

		pGridBtnCell->SetComboString( sa );
	}
	


	m_grid.SetItemText( row, TYPE_COL, arg.sType );
	m_grid.SetItemData( row, VALUE_COL, row+1 );
		
	m_grid.AutoSizeColumns();
	m_grid.Refresh();
}



void Dialog_Sfx_Editor::GetScriptVariables( StringVec & variables )
{	
	for ( int row = 0; row != m_grid.GetRowCount(); ++row )
	{
		CString rCommand = m_grid.GetItemText( row, COMMAND_COL );
		if ( rCommand.CompareNoCase( "set" ) == 0 )
		{			
			if ( row+1 < m_grid.GetRowCount() )
			{
				CString rSet = m_grid.GetItemText( row+1, VALUE_COL );
				if ( !rSet.IsEmpty() )
				{
					variables.push_back( rSet.GetBuffer( 0 ) );
				}
			}
		}
	}
}

void Dialog_Sfx_Editor::OnButtonHelp() 
{
	HELPINFO lhelpInfo;
	CDialog::OnHelpInfo(&lhelpInfo);			
}
