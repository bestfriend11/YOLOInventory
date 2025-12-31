//////////////////////////////////////////////////////////////////////////////
//
// File     :  Menu.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __MENU_H
#define __MENU_H

// Includes
#include "gpcore.h"

// Forward Declarations
class CDialog_Controls;
class CDialog_Preferences;
class CDialog_Show_Interfaces;


class MenuCommands : public Singleton <MenuCommands>
{
public:

	MenuCommands();
	~MenuCommands();

	void Controls();
	void Preferences();
	void ShowInterfaces();

private:

	CDialog_Controls	* m_pControls;
	CDialog_Preferences	* m_pPreferences;
	CDialog_Show_Interfaces * m_pInterfaces;
};


#define gMenuCommands MenuCommands::GetSingleton()


// Menu Functions

class MenuFunctions
{
public:

	void Controls();
	void CreationMode();
	void StructureMode();
	void UVSmartMapMode();
	void UVEditMode();
	void GuideEditMode();
	void Preferences();
	void LoadInterface();
	void CreateInterface();
	void ShowInterfaces();
	void Properties();
	void BringToFront();
	void Messages();
	void CloseInterfaces();
	void SaveInterface();
	void Cut();
	void Copy();
	void Paste();
	void Delete();
	void Left();
	void Right();
	void Up();
	void Down();
	void CommonSettings();
	void GroupSettings();
	void TextSettings();
	void CreateGuiIdMap();
	void SetBackground();
};


#endif