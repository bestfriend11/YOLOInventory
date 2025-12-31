//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorTriggers.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __EDITORTRIGGERS_H
#define __EDITORTRIGGERS_H


// Include Files
#include "gpcore.h"
#include "SiegeEditorTypes.h"
#include "ImageListDefines.h"
#include "siege_pos.h"
#include "GoDefs.h"


// structs
struct ParameterInfo
{
	gpstring	sType;
	gpstring	sName;
	gpstring	sValue;
	bool		bHasValues;
	StringVec	values;
};


#define INVALID_INDEX -1


class EditorTriggers : public Singleton <EditorTriggers>
{
public:

	EditorTriggers();
	~EditorTriggers();

	void	SetCurrentGoid( Goid object )		{ m_current_object = object; }
	Goid	GetCurrentGoid()					{ return m_current_object; }

	void	SetCurrentTriggerIndex( int index ) { m_trigger_index = index; }
	int		GetCurrentTriggerIndex()			{ return m_trigger_index; }

	void	SetCurrentConditionIndex( int index )	{ m_condition_index = index; }
	int		GetCurrentConditionIndex()				{ return m_condition_index;	}

	void	SetCurrentActionIndex( int index )		{ m_action_index = index; }
	int		GetCurrentActionIndex()					{ return m_action_index; }

	ParameterInfo * GetSelectedParameter()			{ return &m_selected_parameter; }

	void	ExecuteSelectedTriggers();
	void	ResetSelectedTriggers();

private:

	Goid		m_current_object;
	int				m_trigger_index;
	int				m_condition_index;
	int				m_action_index;
	ParameterInfo	m_selected_parameter;

};


#define gEditorTriggers EditorTriggers::GetSingleton()


#endif 
