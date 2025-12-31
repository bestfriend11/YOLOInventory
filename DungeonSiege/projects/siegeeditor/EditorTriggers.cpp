//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorTriggers.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "PrecompEditor.h"
#include "EditorTriggers.h"
#include "ScidManager.h"
#include "EditorRegion.h"
#include "EditorObjects.h"
#include "Trigger_Sys.h"
#include "WorldTime.h"


EditorTriggers::EditorTriggers()
{
	m_current_object	= GOID_INVALID;
	m_trigger_index		= INVALID_INDEX;
	m_condition_index	= INVALID_INDEX;
	m_action_index		= INVALID_INDEX;

}


EditorTriggers::~EditorTriggers()
{
}


void EditorTriggers::ExecuteSelectedTriggers()
{
	std::vector< DWORD > objectIds;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objectIds );
	for ( i = objectIds.begin(); i != objectIds.end(); ++i )
	{
		GoHandle hObject( MakeGoid( *i  ) );
		if ( hObject.IsValid() )
		{
			trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
			for ( int j = 0; j != pTriggerStorage->Size(); ++j )
			{
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( j, &pTrigger );
				
				if ( pTrigger )
				{					
					float seconds = ::GetGlobalSeconds() - gWorldTime.GetSecondsElapsed();
					while ( !pTrigger->ExecuteActions( seconds ) )
					{
						seconds = ::GetGlobalSeconds() - seconds;
					}					
				}
			}
		}
	}
}


void EditorTriggers::ResetSelectedTriggers()
{
	std::vector< DWORD > objectIds;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objectIds );
	for ( i = objectIds.begin(); i != objectIds.end(); ++i )
	{
		GoHandle hObject( MakeGoid( *i  ) );
		if ( hObject.IsValid() )
		{
			trigger::Storage *pTriggerStorage = &(hObject->GetCommon()->GetValidInstanceTriggers());
			for ( int j = 0; j != pTriggerStorage->Size(); ++j )
			{
				trigger::Trigger *pTrigger = 0;
				pTriggerStorage->Get( j, &pTrigger );
				
				if ( pTrigger )
				{
					pTrigger->Reset(gWorldTime.GetTime());
				}
			}
		}
	}
}