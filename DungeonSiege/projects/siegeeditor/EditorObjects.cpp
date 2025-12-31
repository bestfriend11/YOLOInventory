//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorObjects.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



// Include Files
#include "PrecompEditor.h"
#include "EditorObjects.h"
#include "ScidManager.h"
#include "EditorTerrain.h"
#include "EditorRegion.h"
#include "ComponentList.h"
#include "GoGizmo.h"
#include "Server.h"
#include "GoData.h"
#include "MenuCommands.h"
#include "Preferences.h"
#include "GoEdit.h"
#include "Services.h"
#include "Player.h"
#include "EditorCamera.h"
#include "resource.h"
#include "Dialog_Portraits.h"
#include "WorldMap.h"
#include "RapiPrimitive.h"


using namespace siege;


EditorObjects::EditorObjects()	
{
	m_found_iterator	= m_found_objects.begin();
	m_bCutting			= false;
	m_bSequenceEditing	= false;
	m_sequenceFirst		= GOID_INVALID;
	m_sequenceLast		= GOID_INVALID;
	m_ViewerFilter.bSpell	= false;
	m_ViewerFilter.slot		= ES_NONE;

	m_nextObject.bRandomScale = false;
	m_nextObject.bRandPitch = false;
	m_nextObject.bRandRoll = false;
	m_nextObject.bRandYaw = false;
	m_nextObject.maxMultiplier = 1.5f;
	m_nextObject.minMultiplier = 0.85f;
}


bool EditorObjects::AddObject( gpstring sTemplate, SiegePos spos, Goid & newObject )
{	
	GoCloneReq createGo( sTemplate, gServer.GetPlayer( ToUnicode( "computer" ) )->GetId() );
	createGo.SetStartingPos( spos );	
	createGo.m_ForcePosition = true;
	newObject = gGoDb.CloneNonSpawnedGo( createGo, gSCIDManager.GetScid() );
	GoHandle hObject( newObject );
	if ( !hObject.IsValid() )
	{
		return false;
	}
	
	if ( hObject->HasEdit() )
	{
	//	hObject->GetEdit()->BeginEdit();
		if ( m_nextObject.bRandomScale && hObject->HasAspect() )
		{
			hObject->GetAspect()->SetRenderScaleMultiplier( ::Random( m_nextObject.minMultiplier, m_nextObject.maxMultiplier ) );
		}

		Quat orient = hObject->GetPlacement()->GetOrientation();		
		
		if ( m_nextObject.bRandPitch )
		{
			orient.RotateX( DegreesToRadians( ::Random( 0, 359 ) ) );			
		}
		if ( m_nextObject.bRandRoll )
		{
			orient.RotateZ( DegreesToRadians( ::Random( 0, 359 ) ) );			
		}
		if ( m_nextObject.bRandYaw )
		{
			orient.RotateY( DegreesToRadians( ::Random( 0, 359 ) ) );			
		}		

		hObject->GetPlacement()->SetOrientation( orient );
	//	hObject->GetEdit()->EndEdit();
	}		
	
	MenuFunctions mf;
	gpstring sGroup = hObject->GetDataTemplate()->GetGroupName();

	if ( sGroup.same_no_case( "trigger" ) || sGroup.same_no_case( "generator" ) || 
		 sGroup.same_no_case( "party" ) || sGroup.same_no_case( "humanoid_party" ) ) {		
		if ( gPreferences.GetShowGizmos() == false )
		{
			mf.ShowGizmos();	
		}
	}
	else
	{
		if ( hObject->HasGizmo() )
		{	
			if ( gPreferences.GetShowGizmos() == false )
			{
				mf.ShowGizmos();
			}
		}
		else
		{
			if ( gPreferences.GetShowObjects() == false )
			{
				mf.ShowObjects();
			}
		}
	}

	gGizmoManager.InsertGizmo( hObject );
	return true;
}


Goid EditorObjects::AddObject( Goid newObject, SiegePos spos )
{
	GoCloneReq createGo( newObject, gServer.GetPlayer( ToUnicode( "human" ) )->GetId() );
	createGo.SetStartingPos( spos );
	createGo.m_ForcePosition = true;
	Goid object = gGoDb.CloneNonSpawnedGo( createGo, gSCIDManager.GetScid() );
	GoHandle hObject( object );		
	gGizmoManager.InsertGizmo( hObject );

	return object;
}


bool EditorObjects::AddSelectedObject()
{
	if ( m_sSelectedTemplate.empty() ) 
	{
		return false;
	}

	SiegePos spos;
	spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();

	Goid object = GOID_INVALID;

	return( AddObject( m_sSelectedTemplate, spos, object ) );	
}


void EditorObjects::DeleteObject( Goid object )
{
	{
		GoHandle hObject( object );
		if ( hObject.IsValid() ) 
		{	
			gGizmoManager.RemoveGizmo( MakeInt( object ) );
	
			GoidColl::iterator i;
			for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
			{
				if ( object == *i )
				{
					m_clipboard.erase( i );
					break;
				}
			}

			gGoDb.SMarkGoAndChildrenForDeletion( object );
		}	
	}
	gGoDb.CommitAllRequests();
}


void EditorObjects::DeleteObject( Scid object )
{
	Goid found = gGoDb.FindGoidByScid( object );

	if ( found != GOID_INVALID )
	{
		GoHandle hObject( found );
		if ( hObject.IsValid() ) 
		{	
			gGizmoManager.RemoveGizmo( MakeInt( found ) );
			FuelHandle hFuel( hObject->DevGetFuelAddress() );		
			if ( hFuel.IsValid() )
			{
				FuelHandle hParent = hFuel->GetParentBlock();
				hParent->DestroyChildBlock( hFuel );			
			}
			gGoDb.SMarkGoAndChildrenForDeletion( found );
		}	
	}
	gGoDb.CommitAllRequests();
}


void EditorObjects::DeleteSelectedObjects()
{
	std::vector< DWORD > object_ids;
	gGizmoManager.GetSelectedObjectIDs( object_ids );
	std::vector< DWORD >::iterator i;
	for ( i = object_ids.begin(); i != object_ids.end(); ++i ) {
		DeleteObject( MakeGoid( *i ) );
	}
}


bool EditorObjects::Find( FIND_COMPONENT & fc )
{
	bool bFound = false;
	m_found_objects.clear();
	//m_found_iterator = NULL;
	

	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );
	for ( i = objects.begin(); i != objects.end(); ++i )
	{		
		GoHandle hObject( *i );
		gpstring b_name	= hObject->GetTemplateName();
		gpstring s_name	= ToAnsi( hObject->GetCommon()->GetScreenName() );		
		gpstring d		= hObject->GetDataTemplate()->GetDocs();
		gpstring t		= hObject->GetDataTemplate()->GetBaseDataTemplate()->GetName();
		DWORD dwScid	= MakeInt(hObject->GetScid());

		bool bMatch1 = false;
		bool bMatch2 = false;
		bool bMatch3 = false;
		bool bMatch4 = false;
		bool bMatch5 = false;
		
		if ( ( fc.sFuelName.empty() ) || ( b_name.find( fc.sFuelName ) != -1 ) ) {
			bMatch1 = true;
		}	
		
		if ( ( fc.sScreenName.empty() ) || ( s_name.find( fc.sScreenName ) != -1 ) ) {
			bMatch2 = true;
		}
		
		if ( ( fc.sDevName.empty() ) || ( d.find( fc.sDevName ) != -1 ) ) {
			bMatch3 = true;
		}
		
		if ( ( fc.sType.empty() ) || ( fc.sType.same_no_case( t ) ) ) {
			bMatch4 = true;
		}

		if ( ( fc.dwScid == MakeInt(SCID_INVALID) ) || ( fc.dwScid == dwScid ) ) {
			bMatch5 = true;
		}
		
		if ( bMatch1 && bMatch2 && bMatch3 && bMatch4 && bMatch5 ) {
			m_found_objects.push_back( hObject->GetGoid() );
			bFound = true;
		}
	}

	if ( m_found_objects.size() != 0 ) 
	{
		m_found_iterator = m_found_objects.begin();		
		gGizmoManager.SelectGizmo( MakeInt((*m_found_iterator)) );
		GoHandle hObject( *m_found_iterator );
		if ( hObject.IsValid() )
		{
			SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ));
			SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());						
			vector_3 worldPos = node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
			gEditorCamera.SetPosition( worldPos.x, worldPos.y, worldPos.z );			
		}

	}

	return bFound;
}


void EditorObjects::FindNext()
{	
	if ( m_found_objects.size() != 0 ) {
		++m_found_iterator;		
		if ( m_found_iterator == m_found_objects.end() ) {		
			m_found_iterator = m_found_objects.begin();
		}

		if ( m_found_iterator != m_found_objects.end() ) {
			gGizmoManager.SelectGizmo( MakeInt((*m_found_iterator)) );
			GoHandle hObject( *m_found_iterator );
			if ( hObject.IsValid() )
			{
				SiegeNodeHandle handle(gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ));
				SiegeNode& node = handle.RequestObject(gSiegeEngine.NodeCache());						
				vector_3 worldPos = node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
				gEditorCamera.SetPosition( worldPos.x, worldPos.y, worldPos.z );			
			}
		}
	}
}


void EditorObjects::FindSelectAll()
{	
	GoidColl::iterator i;
	for ( i = m_found_objects.begin(); i != m_found_objects.end(); ++i ) {
		gGizmoManager.SelectGizmo( MakeInt(*i) );
	}
}


void EditorObjects::ReplaceObject( gpstring sReplaceName ) 
{		
	if ( sReplaceName.same_no_case( "" ) ) {
		return;
	}

	m_found_objects.clear();
	//m_found_iterator = 0;
		
	ComponentVec objects;
	ComponentVec::iterator i;
	gComponentList.RetrieveObjectList( objects );
	for ( i = objects.begin(); i != objects.end(); ++i ) 
	{
		if ( sReplaceName.same_no_case( (*i).sFuelName ) ) 
		{

			std::vector< DWORD > selected_objects;
			std::vector< DWORD >::iterator j;
			gGizmoManager.GetSelectedObjectIDs( selected_objects );
			for ( j = selected_objects.begin(); j != selected_objects.end(); ++j ) {
				GoHandle hObject( MakeGoid(*j) );
				if ( hObject.IsValid() ) {
					Goid newObject = GOID_INVALID;
					AddObject( (*i).sFuelName, hObject->GetPlacement()->GetPosition(), newObject );
					DeleteObject( MakeGoid(*j) );
					gGizmoManager.RemoveGizmo( *j );
				}
			}
		}		
	}	
}


void EditorObjects::Copy()
{
	m_bCutting = false;
	m_clipboard.clear();
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objects );
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		m_clipboard.push_back( MakeGoid( *i ) );
	}
}


void EditorObjects::Cut()
{	
	Copy();
	m_bCutting = true;
}


void EditorObjects::Paste()
{
	SiegePos spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
	
	int x_diff = 0;
	int y_diff = 0;
	int z_diff = 0;

	GoidColl newObjects;

	GoidColl::iterator i;
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
	{	
		GoHandle hObject( *i );
		if ( i == m_clipboard.begin() )
		{
			SiegeNodeHandle Srchandle( gSiegeEngine.NodeCache().UseObject( spos.node ) );
			SiegeNode& Srcnode = Srchandle.RequestObject( gSiegeEngine.NodeCache() );	
			spos.pos = Srcnode.NodeToWorldSpace( spos.pos );

			SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ) );
			SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );	

			x_diff = spos.pos.x - node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos ).x;
			y_diff = node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos ).y;
			z_diff = spos.pos.z - node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos ).z;
		}

		SiegeNodeHandle objHandle( gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ) );
		SiegeNode& objNode = objHandle.RequestObject( gSiegeEngine.NodeCache() );

		SiegePos newPos;
		vector_3 worldPos;
				
		if ( m_clipboard.size() != 1 )
		{
			worldPos.x = objNode.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos ).x + x_diff;
			worldPos.y = objNode.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos ).y + y_diff;
			worldPos.z = objNode.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos ).z + z_diff;					
						
			newPos.pos = objNode.WorldToNodeSpace( worldPos );
			newPos.node = hObject->GetPlacement()->GetPosition().node;
		}
		else
		{
			newPos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
			newPos.pos.y = hObject->GetPlacement()->GetPosition().pos.y;
		}
		
		gGizmoManager.AccurateAdjustPositionToTerrain( newPos );
		
		Goid newObj = AddObject( *i, newPos );		
		newObjects.push_back( newObj );
		gGizmoManager.SelectGizmo( MakeInt(newObj) );		
	}

	if ( m_bCutting )
	{
		GoidColl objects = m_clipboard;
		for ( i = objects.begin(); i != objects.end(); ++i )
		{	
			DeleteObject( *i );
		}

		m_clipboard.clear();
		m_clipboard = newObjects;
		m_bCutting = false;
	}	
}


void EditorObjects::LinkSelectedObjectsToDest( Goid object )
{
	GoHandle hDest( object );
	if ( hDest.IsValid() )
	{
		std::vector< DWORD > objects;
		std::vector< DWORD >::iterator i;
		gGizmoManager.GetSelectedObjectIDs( objects );
		for ( i = objects.begin(); i != objects.end(); ++i )
		{
			GoHandle hSource( MakeGoid( *i ) );
			if ( hSource->SetComponentInt( NULL, "next_scid", MakeInt( hDest->GetScid() ) ) == false )
			{
				gpwarning( "Failed to link objects\n" );
			}
		}
	}

	gEditorRegion.SetRegionDirty();
}


void EditorObjects::PlaceSequenceObject()
{	
	if ( gGizmoManager.GetNumObjectsSelected() != 1 )
	{
		return;
	}

	SiegePos spos;
	spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();

	Goid newObject;
	
	gpstring sTemplate;
	switch ( GetSequenceType() )
	{
	case SEQUENCE_SELECT:
	case SEQUENCE_LINK:
		return;		
	case SEQUENCE_MOVE:
		sTemplate = "cmd_ai_c_move";		
		break;
	case SEQUENCE_PATROL:		
		sTemplate = "cmd_ai_c_patrol";
		break;
	case SEQUENCE_STOP:
		sTemplate = "cmd_ai_t_stop";
		break;
	case SEQUENCE_FOLLOW:
		sTemplate = "cmd_ai_t_follow";
		break;
	case SEQUENCE_GUARD:
		sTemplate = "cmd_ai_t_get";
		break;
	case SEQUENCE_ATTACK:
		sTemplate = "cmd_ai_c_attack_object";
		break;
	case SEQUENCE_GET:
		sTemplate = "cmd_ai_t_get";
		break;
	case SEQUENCE_DROP:
		sTemplate = "cmd_ai_t_drop";
		break;
	case SEQUENCE_USE:
		sTemplate = "cmd_ai_t_use";
		break;
	case SEQUENCE_EQUIP:
		sTemplate = "cmd_ai_t_equip";
		break;
	case SEQUENCE_GIVE:
		sTemplate = "cmd_ai_t_give";
		break;
	}

	if ( AddObject( sTemplate, spos, newObject ) )
	{
		GoHandle hSource( MakeGoid( gGizmoManager.GetSelectedGizmoID() ) );
		GoHandle hDest( newObject );

		if ( hSource.IsValid() && hDest.IsValid() )
		{
			if (( !hSource->HasGizmo() ) && ( hDest->HasGizmo() ))
			{
				hSource->GetEdit()->BeginEdit();
				FuBi::Record * pRecord = hSource->GetEdit()->GetRecord( "mind" );											
				gpstring sScid;
				sScid.assignf( "0x%08X", MakeInt( hDest->GetScid() ) );
				pRecord->SetAsString( "initial_command", sScid.c_str() );				
				hSource->GetEdit()->EndEdit();

				m_sequenceFirst = GOID_INVALID;
				m_sequenceLast = GOID_INVALID;
			}
			else if ( hSource->HasGizmo() && hDest->HasGizmo() )
			{
				hSource->SetComponentInt( NULL, "next_scid", MakeInt( hDest->GetScid() ) );
			}

			if ( GetSequenceType() == SEQUENCE_PATROL )
			{
				if (( !hSource->HasGizmo() ) && ( hDest->HasGizmo() ))
				{
					m_sequenceFirst = hDest->GetGoid();	
				}	
				
				if ( m_sequenceFirst == GOID_INVALID )
				{
					m_sequenceFirst = hDest->GetGoid();
				}
								
				if ( m_sequenceFirst != hDest->GetGoid() )
				{	
					m_sequenceLast = hDest->GetGoid();
					GoHandle hLast( m_sequenceLast );
					GoHandle hFirst( m_sequenceFirst );

					if ( hLast.IsValid() && hFirst.IsValid() )
					{							
						hLast->SetComponentInt( NULL, "next_scid", MakeInt( hFirst->GetScid() ) );						
					}
				}
			}
		}
	
		gGizmoManager.DeselectAll();
		gGizmoManager.ForceSelectGizmo( MakeInt( hDest->GetGoid() ) );
	}
}


void EditorObjects::PlaceMeterPoint()
{
	SiegePos spos;
	spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();
	Goid newObject = GOID_INVALID;

	if ( AddObject( "dev_meter_point", spos, newObject ) )
	{
		GoHandle hSource( MakeGoid( gGizmoManager.GetSelectedGizmoID() ) );
		GoHandle hDest( newObject );		

		if ( hSource.IsValid() && hDest.IsValid() )
		{
			if ( hSource->HasComponent( "dev_meter_point" ) && hDest->HasComponent( "dev_meter_point" ) )
			{				
				FuBi::Record * pRecord			= hDest->GetEdit()->GetRecord( "dev_meter_point" );			
				FuBi::Record * pSourceRecord	= hSource->GetEdit()->GetRecord( "dev_meter_point" );
								
				hSource->GetEdit()->BeginEdit();
				pSourceRecord->Set( "next_scid", MakeInt(hDest->GetScid()) );
				hSource->GetEdit()->EndEdit();
			}					
		}
	
		gGizmoManager.DeselectAll();
		gGizmoManager.ForceSelectGizmo( MakeInt( hDest->GetGoid() ) );
	}
	else
	{
		MessageBoxEx( gEditor.GetHWnd(), "Could not find the meter point template.  You will not be able to place meter points this session.  Make sure you have your content configured correctly.", "Place Meter Point", MB_OK | MB_ICONEXCLAMATION, LANG_ENGLISH );
	}
}


void EditorObjects::GrabSelectedScid()
{
	std::vector< DWORD > objects;
	std::vector< DWORD >::iterator i;
	gGizmoManager.GetSelectedObjectIDs( objects );
	if ( objects.size() != 0 )
	{
		i = objects.begin();
		GoHandle hObject( MakeGoid(*i) );
		if ( hObject.IsValid() )
		{
			gpstring sScid;
			sScid.assignf( "0x%08X", MakeInt(hObject->GetScid()) );

			// Open the clipboard, and empty it. 
			if ( !OpenClipboard( gEditor.GetHWnd() ) ) 
			{
				return; 
			}
			EmptyClipboard();

			LPTSTR  lptstrCopy; 
			HGLOBAL hglbCopy; 
    
			hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (sScid.size() + 1) * sizeof(TCHAR) ); 
			if (hglbCopy == NULL) 
			{ 
				CloseClipboard(); 
				return; 
			} 
 
			// Lock the handle and copy the text to the buffer. 
			lptstrCopy = (char *)GlobalLock(hglbCopy); 
			memcpy(lptstrCopy, sScid.c_str(), sScid.size() * sizeof(TCHAR)); 
			lptstrCopy[sScid.size()] = (TCHAR) 0;    // null character 
			GlobalUnlock( hglbCopy) ; 
 
			// Place the handle on the clipboard. 
 
	        SetClipboardData(CF_TEXT, hglbCopy); 

			CloseClipboard();
		}
    } 					
}


void EditorObjects::SwitchObject( Goid object, gpstring sNewTemplate )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		// Retrieve the old object's data and delete it
		SiegePos spos = hObject->GetPlacement()->GetPosition();
		Quat orient = hObject->GetPlacement()->GetOrientation();
		Scid scid = hObject->GetScid();
	
		int intScid = 0;
		gpstring sScid;
		Scid initialCommand;
		Scid nextScid = SCID_INVALID;

		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "mind" );													
		if ( pRecord )
		{
			pRecord->GetAsString( "initial_command", sScid );				
			stringtool::Get( sScid, intScid );
			initialCommand = MakeScid( intScid );			
		}
		
		FuBi::Record * pCmdRecord = hObject->GetEdit()->GetRecord( "cmd_ai_dojob" );
		if ( pCmdRecord )
		{
			intScid = hObject->GetComponentInt( NULL, "next_scid" );						
			nextScid = MakeScid( intScid );
		}

		// done with the handle, release it before deleting the object
		hObject = GOID_INVALID;

		bool bSelected = gGizmoManager.GetGizmo( MakeInt( object ) )->bSelected;
		gGizmoManager.DeleteGizmo( MakeInt( object ) );
		gGoDb.CommitAllRequests();
		gWorldMap.RemoveScidIndex( scid );

		// Insert the new object in its place
		GoCloneReq createGo( sNewTemplate, gServer.GetPlayer( ToUnicode( "human" ) )->GetId() );
		createGo.SetStartingPos( spos );	
		Goid newObject = gGoDb.CloneNonSpawnedGo( createGo, scid );
		GoHandle hNewObject( newObject );			

		// Copy some of the old object's information into the new object.	
		hNewObject->GetPlacement()->SetOrientation( orient );
		FuBi::Record * pNewRecord = hNewObject->GetEdit()->GetRecord( "mind" );
		if ( pNewRecord )
		{
			gpstring sTemp;
			sTemp.assignf( "0x%08X", MakeInt( initialCommand ) );
			pNewRecord->SetAsString( "initial_command", sTemp.c_str() );			
		}
		
		pCmdRecord = hNewObject->GetEdit()->GetRecord( "cmd_ai_dojob" );
		if ( pCmdRecord )
		{
			hNewObject->SetComponentScid( NULL, "next_scid", nextScid );			
		}

		MenuFunctions mf;
		gpstring sGroup = hNewObject->GetDataTemplate()->GetGroupName();

		if ( sGroup.same_no_case( "trigger" ) || sGroup.same_no_case( "generator" ) || 
			 sGroup.same_no_case( "party" ) || sGroup.same_no_case( "humanoid_party" ) ) {		
			if ( gPreferences.GetShowGizmos() == false )
			{
				mf.ShowGizmos();	
			}
		}
		else
		{
			if ( gPreferences.GetShowObjects() == false )
			{
				mf.ShowObjects();
			}
		}

		gGizmoManager.InsertGizmo( hNewObject );
		if ( bSelected )
		{
			gGizmoManager.SelectGizmo( MakeInt( newObject ) );
		}
	}
}


void EditorObjects::Draw()
{
	GoidColl::iterator i;
	for ( i = m_clipboard.begin(); i != m_clipboard.end(); ++i )
	{
		gpstring sLabel;
		if ( m_bCutting )
		{
			sLabel = "X";
		}
		else
		{
			sLabel = "C";
		}

		GoHandle hObject( *i );
		SiegePos spos = hObject->GetPlacement()->GetPosition();
		spos.pos.y += 2.0f;
		spos.pos.x += 0.3f;		
		
		gSiegeEngine.GetLabels().DrawLabel( &gServices.GetConsoleFont(), spos, sLabel );
	}

	DrawCommands();
	DrawMeterPoints();

	if ( Dialog_Portraits::DoesSingletonExist() )
	{
		gDialogPortraits.Draw();
	}
}


void EditorObjects::DrawCommands()
{
	if ( gPreferences.GetDrawCommands() )
	{
		GoidColl objects;
		GoidColl::iterator i;

		if ( !gGizmoManager.IsObjectSelected() )
		{
			gGoDb.GetAllGlobalGoids( objects );	
		}
		else
		{
			std::vector< DWORD > ids;
			std::vector< DWORD >::iterator j;
			gGizmoManager.GetSelectedObjectIDs( ids );
			for ( j = ids.begin(); j != ids.end(); ++j )
			{
				objects.push_back( MakeGoid( *j ) );
			}			
		}

		for ( i = objects.begin(); i != objects.end(); ++i ) 
		{
			GoHandle hObject( *i );
			if ( !hObject->IsCommand() )
			{			
				int intScid = 0;
				gpstring sScid;
				Scid command = SCID_INVALID;				

				FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "mind" );													
				if ( pRecord )
				{
					pRecord->GetAsString( "initial_command", sScid );				
					stringtool::Get( sScid, intScid );
					command = MakeScid( intScid );			
				}
				
				Go * pFirst = hObject;
				Go * pLast	= 0;

				DWORD dwColor = MakeInt( hObject->GetGoid() );				
				dwColor |= 0xff000000;
				Scid initialCommand = command;
				bool bFirst = false;
				std::set< int > cmdSet;

				bool bBreakTime = false;
				while ( command != SCID_INVALID && command != 0 )
				{
					Goid dest = gGoDb.FindGoidByScid( command );
					pLast = GoHandle( dest );					
					
					if ( pFirst && pLast )
					{
						gWorld.DrawDebugLinkedTerrainPoints( pFirst->GetPlacement()->GetPosition(), pLast->GetPlacement()->GetPosition(), dwColor, "" );
					}
					else if ( !pLast && dest != 0 )
					{
						gSiegeEngine.GetLabels().DrawLabel( &gServices.GetConsoleFont(), pFirst->GetPlacement()->GetPosition(), "Pointing to Bad Command!" );
						break;
					}
					else if ( !pLast )
					{
						break;
					}

					if ( bBreakTime )
					{
						break;
					}

					if ( command == initialCommand )
					{
						if ( bFirst )
						{
							break;
						}
						else
						{
							bFirst = true;
						}
					}

					if ( pLast->IsCommand() )
					{
						int intCommand = pLast->GetComponentInt( NULL, "next_scid" );						
						command = MakeScid( intCommand );
					}				

					if ( cmdSet.find( MakeInt( command ) ) == cmdSet.end() )
					{
						cmdSet.insert( MakeInt( command ) );
					}
					else
					{
						bBreakTime = true;
					}

					pFirst = pLast;
				}				
			}
		}		
	}
}


void EditorObjects::DrawMeterPoints()
{
	if ( gPreferences.GetDrawMeterPoints() )
	{
		GoidColl objects;
		GoidColl::iterator i;
		std::set< Goid > linked;

		gGoDb.GetAllGlobalGoids( objects );	
			
		for ( i = objects.begin(); i != objects.end(); ++i ) 
		{
			GoHandle hObject( *i );
			if ( hObject->HasComponent( "dev_meter_point" ) )
			{			
				int intScid = 0;
				gpstring sScid;
				Scid command = SCID_INVALID;
				
				FuBi::Record * pRecord			= hObject->GetEdit()->GetRecord( "dev_meter_point" );						
				if ( pRecord )
				{					
					pRecord->GetAsString( "next_scid", sScid );
					stringtool::Get( sScid, intScid );
					command = MakeScid( intScid );			
				}								
				
				Go * pFirst = hObject;
				Go * pLast	= 0;

				DWORD dwColor = gPreferences.GetMeterPointColor();				
				Scid initialCommand = command;
				bool bFirst = false;
				std::set< int > cmdSet;

				bool bBreakTime = false;
				float totalMeters = 0.0f;				

				while ( command != SCID_INVALID && command != 0 )
				{
					Goid dest = gGoDb.FindGoidByScid( command );
					pLast = GoHandle( dest );					
					
					if ( (pFirst && pLast) && (pFirst != pLast) )
					{	
						std::set< Goid >::iterator iFindLinked = linked.find( pLast->GetGoid() );
						if ( iFindLinked != linked.end() )
						{
							break;
						}

						SiegePos spos1 = pFirst->GetPlacement()->GetPosition();												
						SiegePos spos2 = pLast->GetPlacement()->GetPosition();												
					
						gWorld.DrawDebugLinkedTerrainPoints( spos1, spos2, dwColor, "" );
						
						gpstring sDistance;

						totalMeters += spos2.WorldPos().Distance( spos1.WorldPos() );
						if ( gPreferences.GetShowMeterPointDistance() )
						{
							sDistance.assignf( "%f", spos2.WorldPos().Distance( spos1.WorldPos() ) );
						}
						else
						{
							sDistance.assignf( "%f", totalMeters );
						}						
						
						spos2.pos.y += 0.5f;
						gSiegeEngine.GetLabels().DrawLabel( gEditorTerrain.GetFont(), spos2, sDistance, 0.0f, 0.8f );

						linked.insert( pLast->GetGoid() );
					}
					else if ( !pLast && dest != 0 )
					{						
						break;
					}
					else if ( !pLast )
					{
						break;
					}

					if ( bBreakTime )
					{
						break;
					}

					if ( command == initialCommand )
					{
						if ( bFirst )
						{
							break;
						}
						else
						{
							bFirst = true;
						}
					}

					if ( pLast->HasComponent( "dev_meter_point" ) )
					{
						int intCommand = pLast->GetComponentInt( NULL, "next_scid" );						
						command = MakeScid( intCommand );
					}				

					if ( cmdSet.find( MakeInt( command ) ) == cmdSet.end() )
					{
						cmdSet.insert( MakeInt( command ) );
					}
					else
					{
						bBreakTime = true;
					}

					pFirst = pLast;
				}					
			}
		}	
	}	
}


bool EditorObjects::IsHiddenComponent( gpstring sComponent )
{
	StringVec::iterator i;
	for ( i = m_hiddenComponents.begin(); i != m_hiddenComponents.end(); ++i )
	{
		if ( (*i).same_no_case( sComponent ) )
		{
			return true;
		}
	}

	return false;
}


void EditorObjects::AddHiddenComponent( gpstring sComponent )
{
	m_hiddenComponents.push_back( sComponent );
}


void EditorObjects::RemoveHiddenComponent( gpstring sComponent )
{
	StringVec::iterator i;
	for ( i = m_hiddenComponents.begin(); i != m_hiddenComponents.end(); ++i )
	{
		if ( (*i).same_no_case( sComponent ) )
		{
			m_hiddenComponents.erase( i );
			return;
		}
	}
}


bool EditorObjects::CreateMacroFromSelection( gpstring sMacro )
{
	std::vector< DWORD > objects;
	gGizmoManager.GetSelectedObjectIDs( objects );
	return CreateMacro( sMacro, objects );
}


bool EditorObjects::CreateMacro( gpstring sMacro, std::vector< DWORD > objects )
{
	FuelHandle hMacro( "config:editor" );
	if ( hMacro.IsValid() )
	{
		if ( hMacro->GetChildBlock( "se_macros" ) )
		{
			hMacro = hMacro->GetChildBlock( "se_macros" );
		}
		else 
		{
			hMacro = hMacro->CreateChildBlock( "se_macros", "se_macros.gas" );
		}

		if ( !hMacro.IsValid() )
		{
			return false;
		}

		if ( hMacro->GetChildBlock( sMacro.c_str() ) )
		{
			return false; 
		}

		hMacro = hMacro->CreateChildBlock( sMacro.c_str() );
		if ( hMacro.IsValid() )
		{
			vector_3 leadPos;
			std::vector< DWORD >::iterator i;
			for ( i = objects.begin(); i != objects.end(); ++i )
			{
				GoHandle hObject( MakeGoid(*i) );
				if ( hObject.IsValid() )
				{
					gpstring sScid;
					sScid.assignf( "mc_0x%08X", hObject->GetScid() );
					FuelHandle hChild = hMacro->CreateChildBlock( sScid );
					if ( hChild.IsValid() )
					{
						if ( i == objects.begin() )
						{					
							hChild->Set( "lead_object", true );	

							gpstring sLeadPos;
							sLeadPos.assignf( "%f,%f,%f",	hObject->GetPlacement()->GetPosition().pos.x,
															hObject->GetPlacement()->GetPosition().pos.y,
															hObject->GetPlacement()->GetPosition().pos.z );
							hChild->Set( "lead_pos", sLeadPos.c_str() );

							SiegeNode* pNode = gSiegeEngine.IsNodeValid( hObject->GetPlacement()->GetPosition().node );
							if ( pNode )
							{
								leadPos = pNode->NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
							}
						}
						else
						{
							SiegeNode* pNode = gSiegeEngine.IsNodeValid( hObject->GetPlacement()->GetPosition().node );
							if ( pNode )
							{
								vector_3 pos = pNode->NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
								gpstring sPos;

								sPos.assignf( "%f,%f,%f", pos.x-leadPos.x, pos.y-leadPos.y, pos.z-leadPos.z );
								hChild->Set( "relative_position", sPos );
							}							
						}
					
						gGoDb.SaveGo( hObject->GetGoid(), hChild );
					}
				}
			}
		}
	}

	gEditorRegion.SetRegionDirty();

	return true;
}


void EditorObjects::DeleteMacro( gpstring sMacro )
{
	FuelHandle hMacro( "config:editor" );
	if ( hMacro.IsValid() )
	{
		hMacro = hMacro->GetChildBlock( "se_macros" );
		if ( !hMacro.IsValid() )
		{
			return;
		}

		FuelHandle hChild = hMacro->GetChildBlock( sMacro.c_str() );
		if ( hChild.IsValid() )
		{
			hMacro->DestroyChildBlock( hChild );
		}
	}

	gEditorRegion.SetRegionDirty();
}


void EditorObjects::PlaceMacroInSelectedNode( gpstring sMacro )
{
	SiegePos spos;
	spos.node = gEditorTerrain.GetSelectedNode();
	if ( spos.node.IsValid() )
	{
		PlaceMacro( sMacro, spos );
	}
}


void EditorObjects::PlaceMacro( gpstring sMacro, SiegePos spos )
{
	gpstring sAddress;
	sAddress.assignf( "config:editor:se_macros:%s", sMacro.c_str() );
	FuelHandle hMacro( sAddress );
	if ( hMacro.IsValid() )
	{
		FuelHandleList hlObjects = hMacro->ListChildBlocks( 1 );
		FuelHandleList::iterator i;

		std::vector< MacroComponent > components;
		std::vector< MacroComponent >::iterator iMc;
		MacroComponent lead;

		OldScidToNewScidMap scidMap;

		for ( i = hlObjects.begin(); i != hlObjects.end(); ++i )
		{
			MacroComponent mc;
			bool bLead = false;			
			(*i)->Get( "lead_object", bLead );
			if ( !bLead )
			{			
				gpstring sPos;
				(*i)->Get( "relative_position", sPos );
				stringtool::GetDelimitedValue( sPos, ',', 0, mc.pos.x );
				stringtool::GetDelimitedValue( sPos, ',', 1, mc.pos.y );
				stringtool::GetDelimitedValue( sPos, ',', 2, mc.pos.z );
			}
			else
			{			
				gpstring sPos;
				(*i)->Get( "lead_pos", sPos );
				stringtool::GetDelimitedValue( sPos, ',', 0, spos.pos.x );
				stringtool::GetDelimitedValue( sPos, ',', 1, spos.pos.y );
				stringtool::GetDelimitedValue( sPos, ',', 2, spos.pos.z );
			}
			
			FuelHandleList hlChildren = (*i)->ListChildBlocks( 1 );
			if ( hlChildren.size() == 0 )
			{
				continue;
			}
			FuelHandle hChild = *(hlChildren.begin());
			if ( hChild.IsValid() )
			{
				mc.sAddress = hChild->GetAddress();

				int intScid = 0;
				gpstring sScid = hChild->GetName();
				stringtool::Get( sScid.c_str(), intScid );
				scidMap.insert( std::make_pair( MakeScid(intScid), SCID_INVALID ) );
				mc.oldScid = MakeScid(intScid);
			}
			
			if ( !bLead )
			{
				components.push_back( mc );
			}
			else
			{
				lead = mc;
			}
		}

		SiegeNode* pNode = gSiegeEngine.IsNodeValid( spos.node );
		if ( !pNode )
		{
			return;
		}

		GoidColl newObjects;
		
		
		Goid leadObj = AddObjectFromInstance( lead.sAddress, spos );		
		GoHandle hLead( leadObj );
		vector_3 leadPos = spos.pos;
		if ( hLead.IsValid() )
		{	
			newObjects.push_back( hLead->GetGoid() );

			{
				OldScidToNewScidMap::iterator iScid = scidMap.find( lead.oldScid );
				if ( iScid != scidMap.end() )
				{
					(*iScid).second = hLead->GetScid();
				}
			}

			for ( iMc = components.begin(); iMc != components.end(); ++iMc )
			{
				SiegePos newPos;
				newPos.node = spos.node;
				newPos.pos.x = leadPos.x + (*iMc).pos.x;
				newPos.pos.y = leadPos.y + (*iMc).pos.y;
				newPos.pos.z = leadPos.z + (*iMc).pos.z;
				gGizmoManager.AccurateAdjustPositionToTerrain( newPos );
				Goid newObj = AddObjectFromInstance( (*iMc).sAddress, newPos );
				GoHandle hObject( newObj );
				if ( hObject.IsValid() )
				{			
					hObject->GetPlacement()->SetPosition( newPos );
					OldScidToNewScidMap::iterator iScid = scidMap.find( (*iMc).oldScid );
					if ( iScid != scidMap.end() )
					{
						(*iScid).second = hObject->GetScid();
					}			

					newObjects.push_back( hObject->GetGoid() );
				}
			}		
		}
	
		GoidColl::iterator iGoid;
		for ( iGoid = newObjects.begin(); iGoid != newObjects.end(); ++iGoid )
		{
			GoHandle hObject( *iGoid );
			Scid nextScid = hObject->GetComponentScid( NULL, "next_scid" );			

			OldScidToNewScidMap::iterator iScid;
			iScid = scidMap.find( nextScid );
			if ( iScid != scidMap.end() )
			{
				hObject->SetComponentScid( NULL, "next_scid", (*iScid).second );
			}
		}
	}	
}


Goid EditorObjects::AddObjectFromInstance( gpstring sFuelAddress, SiegePos spos )
{
	FastFuelHandle hFuel( sFuelAddress );

	if ( hFuel.IsValid() )
	{
		GoCloneReq cloneReq( hFuel.GetType(), gServer.GetPlayer( ToUnicode( "computer" ) )->GetId() );
		cloneReq.SetStartingPos( spos );
		Goid newObject = gGoDb.CloneNonSpawnedGo( cloneReq, gSCIDManager.GetScid(), hFuel );
		GoHandle hObject( newObject );	
		return hObject->GetGoid();
	}

	return GOID_INVALID;
}


void EditorObjects::GetMacroNames( StringVec & macros )
{
	FuelHandle hMacro( "config:editor:se_macros" );
	if ( hMacro.IsValid() )
	{
		FuelHandleList hlMacros = hMacro->ListChildBlocks( 1 );
		FuelHandleList::iterator i;
		for ( i = hlMacros.begin(); i != hlMacros.end(); ++i )
		{
			macros.push_back( (*i)->GetName() );
		}
	}
}