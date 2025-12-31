#if !defined(AFX_DIALOG_SFX_EDITOR_H__48168BC2_0466_4D17_B80C_0335F6FD3B7D__INCLUDED_)
#define AFX_DIALOG_SFX_EDITOR_H__48168BC2_0466_4D17_B80C_0335F6FD3B7D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Dialog_Sfx_Editor.h : header file
//

#include "ResizableDialog.h"
#include "PropertyGrid.h"
#include "gridctrl\\BtnDataBase.h"
#include "EditorSfx.h"

struct Argument;


#define COMMAND_COL 0
#define SUBCOMMAND1_COL 1
#define TYPE_COL 2
#define VALUE_COL 3
#define DOC_COL 4

#define ARGUMENT_USED_ROW -1

/////////////////////////////////////////////////////////////////////////////
// Dialog_Sfx_Editor dialog

class Dialog_Sfx_Editor : public CResizableDialog, public Singleton <Dialog_Sfx_Editor>
{
// Construction
public:
	Dialog_Sfx_Editor(CWnd* pParent = NULL);   // standard constructor

	bool GetSelectPrimaryTarget() { return m_bSelectPrimary; }
	void SetPrimaryTarget( int id );

	bool GetSelectSecondaryTarget() { return m_bSelectSecondary; }
	void SetSecondaryTarget( int id );

// Dialog Data
	//{{AFX_DATA(Dialog_Sfx_Editor)
	enum { IDD = IDD_DIALOG_SFX };
	CEdit	m_edit_dest;
	CEdit	m_edit_source;
	CStatic	m_static_delete;
	CButton	m_static_testing;
	CStatic	m_static_source;
	CStatic	m_static_dest;
	CButton	m_static_script;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(Dialog_Sfx_Editor)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
	
	void BuildGrid();

// Implementation
protected:

	CPropertyGrid	m_grid;	
	CBtnDataBase	m_BtnDataBase;

	bool m_bSelectPrimary;
	bool m_bSelectSecondary;

	void BuildScriptFromGrid( FuelHandle hScript );
	void BuildGridFromScript( FuelHandle hScript );
	void GetScriptVariables( StringVec & variables );

	void SetupArgumentsForCommand( gpstring sCommand, int row );
	bool SetupSubCommand1ForCommand( gpstring sCommand, int row );
	void SetupArgumentsForSubCommand1( gpstring sSubCommand1, int row );
	bool SetupSubCommand1ForSubCommand1( gpstring sSubCommand1, int row );
	
	bool IsValueValidType( gpstring sValue, gpstring sType, gpstring sCommand );
	bool RecursiveIsValueValidType( gpstring sValue, gpstring sType, SubCommandVec & subCommands );
	
	bool RecursiveBuildGridSubCommands( gpstring sName, SubCommandVec & subCommands );
	bool RecursiveSetupArgumentsForSubCommand( gpstring sName, int row, SubCommandVec & subCommands );

	void BuildArgumentsForCommand( gpstring sCommand, int row );

	void SetupArgument( Argument arg, int row );

	int  FindEmptyArgumentRow( int newRow );

	gpstring GetPreviousCommandName( int row );
	int		 GetPreviousCommandData( int row );
	int		 GetPreviousCommandRow( int row );
	int		 GetNextCommandRow( int row );
	

	typedef std::map< int, gpstring > RowTypeMap;
	typedef std::pair< int, gpstring > RowTypePair;

	RowTypeMap m_rowTypes;

	// Generated message map functions
	//{{AFX_MSG(Dialog_Sfx_Editor)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonNewScript();
	afx_msg void OnButtonLoadScript();
	afx_msg void OnButtonSaveScript();
	afx_msg void OnButtonStopScripts();
	afx_msg void OnButtonRunScript();
	afx_msg void OnButtonAddCommand();
	afx_msg void OnButtonEditParameters();
	afx_msg void OnButtonRemoveCommand();
	afx_msg void OnButtonSelectPrimary();
	afx_msg void OnButtonSelectSecondary();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define gSfxDialog Dialog_Sfx_Editor::GetSingleton()

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOG_SFX_EDITOR_H__48168BC2_0466_4D17_B80C_0335F6FD3B7D__INCLUDED_)
