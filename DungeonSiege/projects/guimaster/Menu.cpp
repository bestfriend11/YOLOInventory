//////////////////////////////////////////////////////////////////////////////
//
// File     :  Menu.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


// Include Files
#include "stdafx.h"
#include "gpcore.h"
#include "Dialog_Controls.h"
#include "Dialog_Preferences.h"
#include "Dialog_Load_Interface.h"
#include "Dialog_Create_Interface.h"
#include "Dialog_Show_Interfaces.h"
#include "Dialog_Properties.h"
#include "Dialog_Common_Settings.h"
#include "Dialog_Group.h"
#include "Dialog_Text.h"
#include "Menu.h"
#include "Resource.h"
#include "GUIMasterShell.h"
#include "ui_shell.h"
#include "MainFrm.h"
#include "Dialog_Create_Background.h"

#ifdef _DEBUG
#include "gpmem_new_off.h"
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


MenuCommands::MenuCommands()
{
	m_pControls		= NULL;
	m_pPreferences	= NULL;
	m_pInterfaces	= NULL;
}


MenuCommands::~MenuCommands()
{
	delete m_pControls;
	delete m_pPreferences;
	delete m_pInterfaces;
}


void MenuCommands::Controls()
{
	if ( m_pControls )
	{
		delete m_pControls;
		m_pControls = 0;
	}
	
	m_pControls = new CDialog_Controls;
	m_pControls->Create( IDD_DIALOG_CONTROLS );
}


void MenuCommands::Preferences()
{
	if ( m_pPreferences )
	{
		delete m_pPreferences;
		m_pPreferences = 0;
	}

	m_pPreferences = new CDialog_Preferences;
	m_pPreferences->Create( IDD_DIALOG_PREFERENCES );
}


void MenuCommands::ShowInterfaces()
{
	if ( m_pInterfaces )
	{
		delete m_pInterfaces;
		m_pInterfaces = 0;
	}

	m_pInterfaces = new CDialog_Show_Interfaces;
	m_pInterfaces->Create( IDD_DIALOG_SHOWINTERFACES );
}



// Menu Functions


void MenuFunctions::Controls()
{
	gMenuCommands.Controls();
}


void MenuFunctions::CreationMode()
{
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_CREATION, MF_CHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_STRUCTURE, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVEDIT, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVSMARTMAP, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_GUIDEEDIT, MF_UNCHECKED );

	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_CREATION, TRUE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_STRUCTURE, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVSMARTMAP, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVEDIT, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_GUIDEEDIT, FALSE );

	gGUIMaster.SetCurrentMode( MODE_CREATE );
}


void MenuFunctions::StructureMode()
{
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_CREATION, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_STRUCTURE, MF_CHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVEDIT, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVSMARTMAP, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_GUIDEEDIT, MF_UNCHECKED );

	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_CREATION, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_STRUCTURE, TRUE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVSMARTMAP, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVEDIT, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_GUIDEEDIT, FALSE );

	gGUIMaster.SetCurrentMode( MODE_STRUCTURE );
}


void MenuFunctions::UVSmartMapMode()
{
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_CREATION, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_STRUCTURE, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVEDIT, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVSMARTMAP, MF_CHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_GUIDEEDIT, MF_UNCHECKED );

	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_CREATION, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_STRUCTURE, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVSMARTMAP, TRUE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVEDIT, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_GUIDEEDIT, FALSE );

	gGUIMaster.SetCurrentMode( MODE_UVSTRUCTURE );	
}


void MenuFunctions::UVEditMode()
{
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_CREATION, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_STRUCTURE, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVEDIT, MF_CHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVSMARTMAP, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_GUIDEEDIT, MF_UNCHECKED );

	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_CREATION, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_STRUCTURE, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVSMARTMAP, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVEDIT, TRUE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_GUIDEEDIT, FALSE );

	gGUIMaster.SetCurrentMode( MODE_UVEDIT );
}


void MenuFunctions::GuideEditMode()
{
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_CREATION, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_STRUCTURE, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVEDIT, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_UVSMARTMAP, MF_UNCHECKED );
	CheckMenuItem( GetMenu( gGUIMaster.GetHWnd() ), ID_MODE_GUIDEEDIT, MF_CHECKED );

	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_CREATION, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_STRUCTURE, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVSMARTMAP, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_UVEDIT, FALSE );
	gMainFrm.GetModeBar().GetToolBarCtrl().CheckButton( ID_MODE_GUIDEEDIT, TRUE );

	gGUIMaster.SetCurrentMode( MODE_GUIDEEDIT );
}


void MenuFunctions::CreateInterface()
{
	CDialog_Create_Interface dlgCreate;
	dlgCreate.DoModal();
}


void MenuFunctions::LoadInterface()
{
	CDialog_Load_Interface dlgLoad;
	dlgLoad.DoModal();
}


void MenuFunctions::BringToFront()
{
	gGUIMaster.BringSelectedWindowsToFront();
}


void MenuFunctions::Preferences()
{
	gMenuCommands.Preferences();
}


void MenuFunctions::ShowInterfaces()
{
	gMenuCommands.ShowInterfaces();
}


void MenuFunctions::Properties()
{
	CDialog_Properties dlgProp;
	dlgProp.DoModal();
}


void MenuFunctions::CommonSettings()
{
	CDialog_Common_Settings dlgCommon;
	dlgCommon.DoModal();
}


void MenuFunctions::TextSettings()
{
	CDialog_Text dlgText;
	dlgText.DoModal();
}


void MenuFunctions::Messages()
{
}


void MenuFunctions::CloseInterfaces()
{
	StringVec interfaces;
	StringVec::iterator i;
	gGUIMaster.GetActiveInterfaces( interfaces );
	for ( i = interfaces.begin(); i != interfaces.end(); ++i )
	{
		gUIShell.DeactivateInterface( (*i) );
	}
}


void MenuFunctions::SaveInterface()
{
	gGUIMaster.Save();
}


void MenuFunctions::Cut()
{
	gGUIMaster.Cut();
}


void MenuFunctions::Copy()
{
	gGUIMaster.Copy();
}


void MenuFunctions::Paste()
{
	gGUIMaster.Paste();
}


void MenuFunctions::Delete()
{
	switch ( gGUIMaster.GetCurrentMode() )
	{
	case MODE_GUIDEEDIT:
		{
			gGUIMaster.DeleteReferenceLine();
		}
		break;
	default:
		{
			gGUIMaster.DeleteSelectedWindows();
		}
		break;
	}
}


void MenuFunctions::Up()
{
	gGUIMaster.Move( UP );
}


void MenuFunctions::Down()
{
	gGUIMaster.Move( DOWN );
}


void MenuFunctions::Right()
{
	gGUIMaster.Move( RIGHT );
}


void MenuFunctions::Left()
{
	gGUIMaster.Move( LEFT );
}


void MenuFunctions::GroupSettings()
{
	CDialog_Group dlgGroup;
	dlgGroup.DoModal();
}


void MenuFunctions::CreateGuiIdMap()
{
	gGUIMaster.CreateGuiIdMap();
}


void MenuFunctions::SetBackground()
{
	Dialog_Create_Background dlg;
	dlg.DoModal();
}