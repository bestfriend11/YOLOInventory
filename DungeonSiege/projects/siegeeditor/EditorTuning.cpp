//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorTuning.cpp
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#include "PrecompEditor.h"
#include "EditorTuning.h"
#include "EditorRegion.h"
#include "EditorTerrain.h"
#include "EditorObjects.h"
#include "FileSysUtils.h"
#include "AiQuery.h"
#include "Config.h"
#include "GoData.h"
#include "GoInventory.h"
#include "GoAttack.h"
#include "GoDefend.h"
#include "GoActor.h"
#include "GoDb.h"
#include "GoEdit.h"
#include "GoGizmo.h"
#include "Preferences.h"
#include "GameAuditor.h"
#include "Server.h"
#include "Player.h"

using namespace siege;


EditorTuning::EditorTuning()
{	
	m_bAutoSize		= false;	
	m_radius		= 15.0f;
	m_tolerance		= 0.5f;
	m_bMakerEnabled	= false;
	m_bPlacePoints	= false;	
	m_bMasterReport	= false;
	m_hMasterFile	= NULL;
	m_totalEnemies	= 0;
	m_totalExperience = 0;

	m_reportReq.bSummary = false;
	m_reportReq.bSummaryPoint = false;
	m_reportReq.bSummaryRegion = false;
	m_reportReq.bPlacedItem = false;
	m_reportReq.bPlacedItemPoint = false;
	m_reportReq.bPlacedItemRegion = false;
	m_reportReq.bPartyMember = false;
	m_reportReq.bPartyMemberPoint = false;
	m_reportReq.bPartyMemberRegion = false;
	m_reportReq.bEnemyItem = false;
	m_reportReq.bEnemyItemPoint = false;
	m_reportReq.bEnemyItemRegion = false;
	m_reportReq.bEnemy = false;
	m_reportReq.bEnemyPoint = false;
	m_reportReq.bEnemyRegion = false;
	m_reportReq.bContainerItem = false;
	m_reportReq.bContainerItemPoint = false;
	m_reportReq.bContainerItemRegion = false;
	m_reportReq.bAllItem = false;
	m_reportReq.bAllItemPoint = false;
	m_reportReq.bAllItemRegion = false;
}


void EditorTuning::Draw()
{
	if ( GetMakerEnabled() )
	{
		GoidColl objects;
		GoidColl::iterator i;
		gGoDb.GetAllGlobalGoids( objects );	
		for ( i = objects.begin(); i != objects.end(); ++i )
		{		
			GoHandle hObject( *i );
			if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) && hObject->HasEdit() )
			{
				FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
				if ( pRecord )
				{				
					int nextScid = MakeInt(SCID_INVALID);
					pRecord->Get( "next_scid", nextScid );

					float radius = 0.0f;
					pRecord->Get( "radius", radius );

					gpstring sName;
					pRecord->Get( "path_name", sName );

					if ( GetSelectedPath().empty() || GetSelectedPath().same_no_case( sName ) )
					{
						Goid next = gGoDb.FindGoidByScid( MakeScid(nextScid) );
						GoHandle hNext( next );
						if ( hNext.IsValid() )
						{
							gWorld.DrawDebugLinkedTerrainPoints( hObject->GetPlacement()->GetPosition(), 
																 hNext->GetPlacement()->GetPosition(), 
																 0xFFFF00FF, "" );				
						}			

						gWorld.DrawDebugSphere( hObject->GetPlacement()->GetPosition(), radius, 0xFFFFFF00, "" );
					}
				}
			}
		}
	}
}


gpstring EditorTuning::GetPathName( Goid object )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() && hObject->HasEdit() )
	{
		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
		if ( pRecord )
		{
			gpstring sName;
			pRecord->Get( "path_name", sName );
			return sName;
		}
	}
	
	return "";
}


void EditorTuning::GetTuningPathNames( StringVec & paths )
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			gpstring sPath = GetPathName( *i );

			StringVec::iterator j;
			bool bFound = false;
			for ( j = paths.begin(); j != paths.end(); ++j )
			{
				if ( (*j).same_no_case( sPath ) )
				{		
					bFound = true;
					break;					
				}
			}

			if ( !bFound )
			{
				paths.push_back( sPath );
			}
		}
	}
}


void EditorTuning::GetPointsInPath( gpstring sPath, GoidColl & points )
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			gpstring sObjectPath = GetPathName( *i );
			if ( sPath.same_no_case( sObjectPath ) )
			{
				points.push_back( *i );
			}
		}
	}
}


void EditorTuning::GetObjectsInPath( gpstring sPath, GoidColl & objects )
{
	GoidColl points;
	GoidColl::iterator i;
	GetPointsInPath( sPath, points );

	UsedGoidSet usedGoids;
	for ( i = points.begin(); i != points.end(); ++i )
	{
		GoHandle hPoint( *i );
		if ( hPoint.IsValid() )
		{
			float radius = GetRadius( hPoint->GetGoid() );
			GopColl occupants;
			GopColl::iterator j;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), radius, occupants );				
			for ( j = occupants.begin(); j != occupants.end(); ++j )
			{
				if ( usedGoids.find( (*j)->GetGoid() ) == usedGoids.end() )
				{
					objects.push_back( (*j)->GetGoid() );
					usedGoids.insert( (*j)->GetGoid() );
				}
			}
		}
	}
}


void EditorTuning::RemovePath( gpstring sName )
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			gpstring sObjectPath = GetPathName( *i );
			if ( sName.same_no_case( sObjectPath ) )
			{
				gEditorObjects.DeleteObject( *i );
			}
		}
	}
}


Goid EditorTuning::GetFirstPoint( gpstring sPath )
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			gpstring sObjectPath = GetPathName( *i );
			if ( sPath.same_no_case( sObjectPath ) )
			{			
				if ( GetPreviousPoint( *i ) == GOID_INVALID )
				{
					return *i;
				}
			}
		}
	}

	return GOID_INVALID;
}


Goid EditorTuning::GetPreviousPoint( Goid point )
{
	GoidColl objects;
	GoidColl::iterator i;
	gGoDb.GetAllGlobalGoids( objects );	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{		
		GoHandle hObject( *i );
		if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
		{
			FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
			if ( pRecord )
			{				
				int nextScid = MakeInt(SCID_INVALID);
				pRecord->Get( "next_scid", nextScid );

				GoHandle hPoint( point );
				if ( MakeScid(nextScid) == hPoint->GetScid() )
				{
					return hObject->GetGoid();
				}
			}
		}
	}

	return GOID_INVALID;
}


Goid EditorTuning::GetNextPoint( Goid point )
{
	GoHandle hObject( point );
	if ( hObject.IsValid() && hObject->HasComponent( "dev_path_point" ) )
	{
		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
		if ( pRecord )
		{				
			int nextScid = MakeInt(SCID_INVALID);
			pRecord->Get( "next_scid", nextScid );
			
			return gGoDb.FindGoidByScid( MakeScid(nextScid) );
		}
	}

	return GOID_INVALID;
}


void EditorTuning::SetRadius( Goid object, float radius )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
		if ( pRecord )
		{
			hObject->GetEdit()->BeginEdit();

			pRecord->Set( "radius", radius );

			hObject->GetEdit()->EndEdit();
		}
	}	

	gEditorRegion.SetRegionDirty();
}


float EditorTuning::GetRadius( Goid object )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
		if ( pRecord )
		{
			float radius = 0.0f;
			pRecord->Get( "radius", radius );
			return radius;
		}
	}

	return 0;
}


gpstring EditorTuning::GetPointName( Goid object )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );
		if ( pRecord )
		{
			gpstring sName;
			pRecord->Get( "point_name", sName );
			return sName;
		}
	}

	return gpstring();
}



void EditorTuning::SetPosition( Goid object, SiegePos spos )
{
	GoHandle hObject( object );
	if ( hObject.IsValid() )
	{
		FuBi::Record * pRecord = hObject->GetEdit()->GetRecord( "dev_path_point" );

		float radius = 0.0f;
		if ( pRecord )
		{
			pRecord->Get( "radius", radius );
		}
		
		hObject->GetPlacement()->SetPosition( spos, gPreferences.GetSnapToGround() );	
			
		if ( GetAutoSize() )
		{
			Goid previous = GetPreviousPoint( object );
			Goid next = GetNextPoint( object );
			GoHandle hPrevious( previous );
			GoHandle hNext( next );		
			
			if ( hPrevious.IsValid() )
			{
				SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ) );
				SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

				SiegeNodeHandle prev_handle( gSiegeEngine.NodeCache().UseObject( hPrevious->GetPlacement()->GetPosition().node ) );
				SiegeNode& prev_node = prev_handle.RequestObject( gSiegeEngine.NodeCache() );
				
				vector_3 originPt;
				vector_3 prevPt;
				
				originPt	= node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
				prevPt		= prev_node.NodeToWorldSpace( hPrevious->GetPlacement()->GetPosition().pos );
				
				float distance = originPt.Distance( prevPt );	
				radius = distance - GetRadius( hPrevious->GetGoid() ) + GetRadiusTolerance();				
			}			
			else if ( hNext.IsValid() )
			{
				SiegeNodeHandle handle( gSiegeEngine.NodeCache().UseObject( hObject->GetPlacement()->GetPosition().node ) );
				SiegeNode& node = handle.RequestObject( gSiegeEngine.NodeCache() );

				SiegeNodeHandle prev_handle( gSiegeEngine.NodeCache().UseObject( hNext->GetPlacement()->GetPosition().node ) );
				SiegeNode& prev_node = prev_handle.RequestObject( gSiegeEngine.NodeCache() );
				
				vector_3 originPt;
				vector_3 prevPt;
				
				originPt	= node.NodeToWorldSpace( hObject->GetPlacement()->GetPosition().pos );
				prevPt		= prev_node.NodeToWorldSpace( hNext->GetPlacement()->GetPosition().pos );
				
				float distance = originPt.Distance( prevPt );	
				radius = distance - GetRadius( hNext->GetGoid() ) + GetRadiusTolerance();
			}

			SetRadius( object, radius );
		}			
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTuning::AddPoint()
{
	Goid object = GOID_INVALID;
	SiegePos spos;
	spos = gSiegeEngine.GetMouseShadow().GetFloorHitPos();

	if ( gEditorObjects.AddObject( "dev_path_point", spos, object ) )
	{
		GoHandle hObject( object );
		if ( !hObject )
		{
			gperrorf( ( "hObject is invalid! 0x%x", MakeInt(object) ) );
		}

		GoHandle hSource( MakeGoid( gGizmoManager.GetLastSelectedGizmoID() ) );
		if ( !hSource )
		{
			gperrorf( ( "hSource is invalid! 0x%x", gGizmoManager.GetLastSelectedGizmoID() ) );
		}

		if ( hObject.IsValid() )
		{			
			SetRadius( object, GetRadius() );
			
			if ( hSource.IsValid() && hSource->HasComponent( "dev_path_point" ) )
			{				
				FuBi::Record * pRecord			= hObject->GetEdit()->GetRecord( "dev_path_point" );			
				FuBi::Record * pSourceRecord	= hSource->GetEdit()->GetRecord( "dev_path_point" );
				
				gpstring sPath;
				pSourceRecord->Get( "path_name", sPath );

				hSource->GetEdit()->BeginEdit();
				pSourceRecord->Set( "next_scid", MakeInt(hObject->GetScid()) );
				hSource->GetEdit()->EndEdit();

				hObject->GetEdit()->BeginEdit();
				pRecord->Set( "path_name", sPath );
				hObject->GetEdit()->EndEdit();

				SetPosition( hObject->GetGoid(), hObject->GetPlacement()->GetPosition() );				
			}			

			hObject->GetEdit()->SetIsVisible( true );
			gGizmoManager.GetGizmo( MakeInt(hObject->GetGoid()) )->bVisible = true;		
		}
	}

	gEditorRegion.SetRegionDirty();
}


void EditorTuning::GenerateReports()
{
	SetCurrentDirectory( gConfig.ResolvePathVars( "%se_user_path%" ) );
	
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "Tuning";
	CreateDirectory( sDir, 0 );
	sDir += "\\";
	sDir += gEditorRegion.GetRegionName();	
	CreateDirectory( sDir, 0 );

	if ( m_reportReq.bSummary || m_reportReq.bSummaryRegion )
	{
		GenerateSummary( m_reportReq.bSummaryRegion, m_reportReq.bSummary );
	}
	if ( m_reportReq.bSummaryPoint )
	{
		GeneratePointSummary();	
	}
	if ( m_reportReq.bAllItem || m_reportReq.bAllItemRegion )
	{
		ReportAllItem( m_reportReq.bAllItemRegion, m_reportReq.bAllItem );
	}
	if ( m_reportReq.bAllItemPoint )
	{
		ReportAllItemPoint();
	}
	if ( m_reportReq.bContainerItem || m_reportReq.bContainerItemRegion )
	{
		ReportContainerItem( m_reportReq.bContainerItemRegion, m_reportReq.bContainerItem );
	}
	if ( m_reportReq.bContainerItemPoint )
	{
		ReportContainerItemPoint();
	}
	if ( m_reportReq.bEnemy || m_reportReq.bEnemyRegion )
	{
		ReportEnemy( m_reportReq.bEnemyRegion, m_reportReq.bEnemy );
	}
	if ( m_reportReq.bEnemyPoint )
	{
		ReportEnemyPoint();
	}
	if ( m_reportReq.bEnemyItem || m_reportReq.bEnemyItemRegion )
	{
		ReportEnemyItem( m_reportReq.bEnemyItemRegion, m_reportReq.bEnemyItem );
	}
	if ( m_reportReq.bEnemyItemPoint )
	{
		ReportEnemyItemPoint();
	}
	if ( m_reportReq.bPartyMember || m_reportReq.bPartyMemberRegion )
	{
		ReportPartyMember( m_reportReq.bPartyMemberRegion, m_reportReq.bPartyMember );
	}
	if ( m_reportReq.bPartyMemberPoint )
	{
		ReportPartyMemberPoint();
	}	
}


void EditorTuning::GenerateMasterPointSummary()
{
	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );	
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		GoHandle hPoint( GetFirstPoint( *k ) );
		float distance = 0;	
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}
			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );
				if ( hPrevious.IsValid() )
				{			
					distance += hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		gpstring sPathPoint;
		DWORD dwBytesWritten = 0;
		sPathPoint.assignf( "Path: %s - Distance: %f\r\n", (*k).c_str(), distance );
		WriteFile( m_hMasterFile, (void *)sPathPoint.c_str(), sPathPoint.size(), &dwBytesWritten, NULL );	
	}

	gpstring sPathPoint;
	DWORD dwBytesWritten = 0;
	sPathPoint.assign( "\r\n" );
	WriteFile( m_hMasterFile, (void *)sPathPoint.c_str(), sPathPoint.size(), &dwBytesWritten, NULL );	
}


void EditorTuning::BatchGenerateTuningReports( MapToRegionMap & regionMap, bool bMasterReport, bool bTuningLoad )
{
	ReportSys::EnableContext( &gPerfContext, false );

	m_bMasterReport = bMasterReport;
	m_hMasterFile	= NULL;
	m_totalEnemies		= 0;
	m_totalExperience	= 0;
	m_totalGold			= 0;
	
	if ( m_bMasterReport )
	{
		SetCurrentDirectory( gConfig.ResolvePathVars( "%se_user_path%" ) );
		gpstring sDir = FileSys::GetCurrentDirectory( true );
		sDir += "Tuning";
		CreateDirectory( sDir, 0 );
		sDir += "\\";
		sDir += gEditorRegion.GetRegionName();	
		CreateDirectory( sDir, 0 );

		sDir = FileSys::GetCurrentDirectory( true );
		sDir += "tuning\\";
		gpstring sFile;
		sFile.assignf( "%s%s", sDir.c_str(), "master_summary.txt" );

		m_hMasterFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( m_hMasterFile == NULL )
		{
			return;
		}		
	}

	MapToRegionMap::iterator i;
	for ( i = regionMap.begin(); i != regionMap.end(); ++i ) 
	{	
		gpstring sMap		= (*i).first;
		gpstring sRegion	= (*i).second; 				
		gEditorRegion.LoadRegion( sRegion, sMap, false, false, false, bTuningLoad );	
	
		gGoDb.CommitAllRequests();
		
		GenerateReports();	
		
		GenerateMasterPointSummary();
	}	

	if ( m_bMasterReport )
	{
		gpstring sTemp;
		DWORD dwBytesWritten = 0;
		sTemp.assignf( "\r\nTotal Enemies for Selected Regions: %d\r\n", m_totalEnemies );
		WriteFile( m_hMasterFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			
		sTemp.assignf( "Total Experience for Selected Regions: %d\r\n", m_totalExperience );
		WriteFile( m_hMasterFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	
		sTemp.assignf( "Total Gold (including item value) for Selected Regions: %d\r\n", m_totalGold );
		WriteFile( m_hMasterFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			
	}

	m_bMasterReport = false;
	CloseHandle ( m_hMasterFile );
	m_hMasterFile = NULL;

	ReportSys::EnableContext( &gPerfContext, true );
}


void EditorTuning::GenerateSummary( bool bRegion, bool bPath )
{	
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";	
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";	

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_summary.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			GoidColl objects;
			GetObjectsInPath( *k, objects );
			GenerateSummaryHelper( hFile, objects );
		}
	}

	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_summary.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoidColl objects;
		gGoDb.GetAllGlobalGoids( objects );
		GenerateSummaryHelper( hFile, objects, true );
	}
}


void EditorTuning::GenerateSummaryHelper( HANDLE hFile, GoidColl objects, bool bMasterReport )
{
	int totalOccupants		= 0;
	float totalExperience	= 0;
	float monsterExperience = 0;
	int totalGold			= 0;
	int monsterGold			= 0;
	
	int totalMonsters		= 0;

	int numSpells			= 0;
	int numPotions			= 0;
	int numMelee			= 0;
	int numRanged			= 0;
	int numArmor			= 0;
	int numShields			= 0;
	int itemGold			= 0;
	
	int numMonsterSpells	= 0;
	int numMonsterPotions	= 0;
	int numMonsterMelee		= 0;
	int numMonsterRanged	= 0;	
	int numMonsterArmor		= 0;
	int numMonsterShields	= 0;
	int monsterItemGold		= 0;

	std::map< gpstring, int, istring_less > monsterTypes;	
	
	GoidColl generatedObjects;
	GoidColl deletionList;

	GoidColl::iterator i = objects.begin();
	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( gAIQuery.Is( NULL, hObject, QT_EVIL ) )
		{			
			gGameAuditor.Set( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName(), 0 );			
		}
	}
	i = objects.begin();

	while( i != objects.end() )
	{
		GoHandle hObject( *i );

		if ( hObject->HasAspect() )
		{
			totalExperience += hObject->GetAspect()->GetExperienceValue();					
		}

		if ( hObject->HasComponent( "generator_basic" ) )			 
		{
			int numChildren = hObject->GetComponentInt( "generator_basic", "num_children_incubating", 0 );
			gpstring sTemplate = hObject->GetComponentString( "generator_basic", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}			
		}		
		else if ( hObject->HasComponent( "generator_advanced_a1" ) )
		{
			int numChildren = hObject->GetComponentInt( "generator_advanced_a1", "num_children_incubating", 0 );			
			gpstring sTemplate = hObject->GetComponentString( "generator_advanced_a1", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}
		}
		else if ( hObject->HasComponent( "generator_advanced_a2" ) )
		{
			int numChildren = hObject->GetComponentInt( "generator_advanced_a2", "num_children_incubating", 0 );
			gpstring sTemplate = hObject->GetComponentString( "generator_advanced_a2", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}			
		}				 
		else if ( hObject->HasComponent( "generator_breakable" ) )
		{
			int numChildren = hObject->GetComponentInt( "generator_breakable", "num_children_incubating", 0 );
			gpstring sTemplate = hObject->GetComponentString( "generator_breakable", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}			
		}				 
		else if ( hObject->HasComponent( "generator_auto_object_exploding" ) )
		{
			int numChildren = 1;
			gpstring sTemplate = hObject->GetComponentString( "generator_auto_object_exploding", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}			
		}
		else if ( hObject->HasComponent( "generator_auto_advanced_a1" ) )
		{
			int numChildren = hObject->GetComponentInt( "generator_auto_advanced_a1", "num_children_incubating", 0 );
			gpstring sTemplate = hObject->GetComponentString( "generator_auto_advanced_a1", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}			
		}
		else if ( hObject->HasComponent( "generator_auto_advanced_a2" ) )
		{
			int numChildren = hObject->GetComponentInt( "generator_auto_advanced_a2", "num_children_incubating", 0 );
			gpstring sTemplate = hObject->GetComponentString( "generator_auto_advanced_a2", "child_template_name" );
			GoCloneReq cloneReq( sTemplate );
			cloneReq.m_OmniGo = true;
			
			for ( int iChild = 0; iChild != numChildren; ++iChild )
			{
				Goid object = gGoDb.SCloneGo( cloneReq );
				if ( object != GOID_INVALID )
				{
					generatedObjects.push_back( object );
					deletionList.push_back( object );
				}
			}			
		}

		bool bDontCountGold = false;
		if ( hObject->HasAspect() && gAIQuery.Is( NULL, hObject, QT_EVIL ) )
		{
			monsterExperience += hObject->GetAspect()->GetExperienceValue();
			if ( hObject->HasInventory() )
			{
				monsterGold += hObject->GetInventory()->GetGold();

				GopColl items;
				GopColl::iterator iItem;
				hObject->GetInventory()->ListItems( IL_ALL, items );
				for ( iItem = items.begin(); iItem != items.end(); ++iItem )
				{	
					if ( (*iItem)->HasGui() && !(*iItem)->GetGui()->IsDroppable() )
					{						
						continue;
					}

					if ( (*iItem)->IsEquipped() && hObject->GetInventory()->GetEquippedItemsSpewType( gServer.GetLocalHumanPlayer()->GetId() ) != GoInventory::SPEW_GROUND )
					{						
						continue;
					}

					if ( (*iItem)->IsPotion() )
					{
						numMonsterPotions += 1;
					}
					if ( (*iItem)->IsSpell() )
					{
						numMonsterSpells += 1;
					}
					if ( (*iItem)->IsMeleeWeapon() )
					{
						numMonsterMelee += 1;
					}
					if ( (*iItem)->IsRangedWeapon() )
					{
						numMonsterRanged += 1;
					}
					if ( (*iItem)->IsShield() )
					{
						numMonsterShields += 1;
					}
					else if ( (*iItem)->IsArmor() )
					{
						numMonsterArmor += 1;
					}							
					if ( (*iItem)->IsItem() )
					{
						monsterItemGold += (*iItem)->GetAspect()->GetGoldValue();
					}
				}

				gGameAuditor.Increment( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName() );
			}

			totalMonsters += 1;
			gpstring sTemplate = hObject->GetTemplateName();
			std::map< gpstring, int, istring_less >::iterator iMonsters = monsterTypes.find( sTemplate );
			if ( iMonsters == monsterTypes.end() )
			{
				monsterTypes.insert( std::pair< gpstring, int >( sTemplate, 1 ) );
			}
			else
			{
				(*iMonsters).second++;
			}					
		}		
		if ( hObject->HasInventory() )
		{
			totalGold += hObject->GetInventory()->GetGold();
		}	
				
		if ( !hObject->HasParent() || !(hObject->HasParent() && gAIQuery.Is( NULL, hObject->GetParent(), QT_EVIL )) )
		{
			if ( hObject->IsPotion() )
			{
				numPotions += 1;
			}
			if ( hObject->IsSpell() )
			{
				numSpells += 1;
			}
			if ( hObject->IsMeleeWeapon() )
			{
				numMelee += 1;
			}
			if ( hObject->IsRangedWeapon() )
			{
				numRanged += 1;
			}
			if ( hObject->IsShield() )
			{
				numShields += 1;
			}
			else if ( hObject->IsArmor() )
			{
				numArmor += 1;
			}

			if ( hObject->IsItem() )
			{
				itemGold += hObject->GetAspect()->GetGoldValue();
			}
		}				
		
		i++;

		if ( i == objects.end() && generatedObjects.size() != 0 )
		{
			objects.clear();
			GoidColl::iterator iChild;
			for ( iChild = generatedObjects.begin(); iChild != generatedObjects.end(); ++iChild )
			{
				objects.push_back( *iChild );
			}

			i = objects.begin();
			generatedObjects.clear();
		}		
	}

	GoidColl::iterator iDelete;
	for ( iDelete = deletionList.begin(); iDelete != deletionList.end(); ++iDelete )
	{
		gEditorObjects.DeleteObject( *iDelete );
	}

	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	if ( m_bMasterReport && bMasterReport )
	{
		sTemp.assignf( "%s,%d,%f,%d\r\n", gEditorRegion.GetRegionName().c_str(), totalMonsters, totalExperience, totalGold+itemGold );
		WriteFile( m_hMasterFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			
		m_totalEnemies += totalMonsters;
		m_totalExperience += totalExperience;
		m_totalGold += (totalGold + itemGold + monsterItemGold);
	}

	sTemp.assign( "GENERAL INFORMATION\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assign( "-------------------\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Number of objects in path: %d\r\n", totalOccupants );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Number of enemies in path: %d\r\n", totalMonsters );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Total Experience from all objects: %f\r\n", totalExperience );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Total Experience from enemies: %f\r\n\r\n", monsterExperience );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Total Gold: %d\r\n", totalGold );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Total Gold from enemies: %d\r\n", monsterGold );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	
	
	sTemp.assign( "PLACED ITEM INFORMATION\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assign( "-----------------------\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Spells: %d\r\n", numSpells );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Potions: %d\r\n", numPotions );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Melee Weapons: %d\r\n", numMelee );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Ranged Weapons: %d\r\n", numRanged );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Armor: %d\r\n", numArmor );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Shields: %d\r\n", numShields );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Gold Worth of Items: %d\r\n\r\n", itemGold );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assign( "MONSTER INFORMATION\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assign( "------------------------\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );			

	sTemp.assignf( "Monster-owned Spells: %d\r\n", numMonsterSpells );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Monster-owned Potions: %d\r\n", numMonsterPotions );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Monster-owned Melee Weapons: %d\r\n", numMonsterMelee );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Monster-owned Ranged Weapons: %d\r\n", numMonsterRanged );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Monster-owned Armor: %d\r\n", numMonsterArmor );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Monster-owned Shields: %d\r\n", numMonsterShields );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assignf( "Monster-owned Gold Worth of Items: %d\r\n\r\n", monsterItemGold );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	

	sTemp.assign( "Enemies Used: \r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	
	std::map< gpstring, int, istring_less >::iterator j;
	for ( j = monsterTypes.begin(); j != monsterTypes.end(); ++j )
	{
		sTemp.assignf( "%s - %d\r\n", (*j).first.c_str(), (*j).second );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	
	}		

	CloseHandle( hFile );
}


void EditorTuning::ReportAllItem( bool bRegion, bool bPath )
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{		
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_all_items.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "All Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			GoidColl objects;
			GetObjectsInPath( *k, objects );
			ReportAllItemHelper( hFile, objects );
		}
	}

	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_all_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "All Items Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
	
		GoidColl objects;
		gGoDb.GetAllGlobalGoids( objects );
		ReportAllItemHelper( hFile, objects );		
	}		
}


void EditorTuning::ReportAllItemHelper( HANDLE hFile, GoidColl objects )
{
	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	sTemp.assign( "Template Name,Scid,Level,Class,Damage,Defense,Gold Value,Magical\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	GoidColl::iterator i;		

	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( gAIQuery.Is( NULL, hObject, QT_EVIL ) )
		{			
			gGameAuditor.Set( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName(), 0 );			
		}
	}

	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );

		if ( hObject->IsContainer() || ( hObject->HasActor() && gAIQuery.Is( NULL, hObject, QT_EVIL ) ) )
		{
			GopColl items;
			GopColl::iterator iItem;
			hObject->GetInventory()->ListItems( IL_ALL, items );
			for ( iItem = items.begin(); iItem != items.end(); ++iItem )
			{
				float	level	= 0;						
				float	defense = 0.0f;
				gpstring sMagical = "No";
				gpstring sClass;
				gpstring sDamage = "0";

				if ( (*iItem)->HasGui() && !(*iItem)->GetGui()->IsDroppable() )
				{
					continue;
				}

				if ( (*iItem)->IsEquipped() && hObject->GetInventory()->GetEquippedItemsSpewType( gServer.GetLocalHumanPlayer()->GetId() ) != GoInventory::SPEW_GROUND )
				{
					continue;
				}

				if ( (*iItem)->HasMagic() )
				{
					sMagical = "Yes";
				}

				if ( (*iItem)->HasAttack() )
				{
					sDamage.assignf( "%f-%f", (*iItem)->GetAttack()->GetDamageMin(), (*iItem)->GetAttack()->GetDamageMax() );	
					
					float avg = ((*iItem)->GetAttack()->GetDamageMax() + (*iItem)->GetAttack()->GetDamageMin())/2;
					if ( avg != 0 )
					{
						level = avg - ( avg * ((( (*iItem)->GetAttack()->GetDamageMax() - (*iItem)->GetAttack()->GetDamageMin())/2) / avg ) / 10 );
					}
				}
				if ( (*iItem)->HasDefend() )
				{
					defense = (*iItem)->GetDefend()->GetDefense();
					level = defense;
				}

				if ( (*iItem)->IsMeleeWeapon() )
				{
					sClass = "Melee Weapon";
				}
				else if ( (*iItem)->IsRangedWeapon() )
				{
					sClass = "Ranged Weapon";
				}
				else if ( (*iItem)->IsSpell() )
				{
					sClass = "Spell";
				}
				else if ( (*iItem)->IsShield() )
				{
					sClass = "Shield";
				}
				else if ( (*iItem)->IsArmor() )
				{
					sClass = "Armor";
				}
				else if ( (*iItem)->IsPotion() )
				{
					sClass = "Potion";
				}
				else if ( (*iItem)->IsGold() )
				{
					sClass = "Gold";
				}

				sTemp.assignf( "%s,0x%08X,%f,%s,%s,%f,%d,%s\r\n", (*iItem)->GetTemplateName(),
															MakeInt((*iItem)->GetScid()),
															level, sClass.c_str(), sDamage.c_str(),
															defense, (*iItem)->GetAspect()->GetGoldValue(),
															sMagical.c_str() );
				WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
			}

			if ( gAIQuery.Is( NULL, hObject, QT_EVIL ) )
			{
				gGameAuditor.Increment( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName() );
			}
		}
		else if ( hObject->IsItem() && !hObject->GetParent() )
		{
			float	level	= 0;						
			float	defense = 0.0f;
			gpstring sMagical = "No";
			gpstring sClass;
			gpstring sDamage = "0";

			if ( hObject->HasMagic() )
			{
				sMagical = "Yes";
			}

			if ( hObject->HasAttack() )
			{
				sDamage.assignf( "%f-%f", hObject->GetAttack()->GetDamageMin(), hObject->GetAttack()->GetDamageMax() );	
				
				float avg = (hObject->GetAttack()->GetDamageMax() + hObject->GetAttack()->GetDamageMin())/2;
				if ( avg != 0 )
				{
					level = avg - ( avg * ((( hObject->GetAttack()->GetDamageMax() - hObject->GetAttack()->GetDamageMin())/2) / avg ) / 10 );
				}
			}
			if ( hObject->HasDefend() )
			{
				defense = hObject->GetDefend()->GetDefense();
				level = defense;
			}

			if ( hObject->IsMeleeWeapon() )
			{
				sClass = "Melee Weapon";
			}
			else if ( hObject->IsRangedWeapon() )
			{
				sClass = "Ranged Weapon";
			}
			else if ( hObject->IsSpell() )
			{
				sClass = "Spell";
			}
			else if ( hObject->IsShield() )
			{
				sClass = "Shield";
			}
			else if ( hObject->IsArmor() )
			{
				sClass = "Armor";
			}
			else if ( hObject->IsPotion() )
			{
				sClass = "Potion";
			}
			else if ( hObject->IsGold() )
			{
				sClass = "Gold";
			}
			else
			{
				continue;
			}

			sTemp.assignf( "%s,0x%08X,%f,%s,%s,%f,%d,%s\r\n", hObject->GetTemplateName(),
														MakeInt(hObject->GetScid()),
														level, sClass.c_str(), sDamage.c_str(),
														defense, hObject->GetAspect()->GetGoldValue(),
														sMagical.c_str() );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
		}
	}

	CloseHandle( hFile );
}
 

void EditorTuning::ReportContainerItem( bool bRegion, bool bPath )
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{		
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_container_items.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "Container Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	
			
			GoidColl objects;
			GetObjectsInPath( *k, objects );
			ReportContainerItemHelper( hFile, objects );			
		}
	}
	
	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_container_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Container Items Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );	
				
		GoidColl objects;
		gGoDb.GetAllGlobalGoids( objects );	
		ReportContainerItemHelper( hFile, objects );
	}	
}


void EditorTuning::ReportContainerItemHelper( HANDLE hFile, GoidColl objects )
{
	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	sTemp.assign( "Template Name,Scid,Level,Class,Damage,Defense,Gold Value,Magical\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	GoidColl::iterator i;	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );

		if ( hObject->IsContainer() )
		{
			GopColl items;
			GopColl::iterator iItem;
			hObject->GetInventory()->ListItems( IL_ALL, items );
			for ( iItem = items.begin(); iItem != items.end(); ++iItem )
			{
				float	level	= 0;						
				float	defense = 0.0f;
				gpstring sMagical = "No";
				gpstring sClass;
				gpstring sDamage = "0";

				if ( (*iItem)->HasMagic() )
				{
					sMagical = "Yes";
				}

				if ( (*iItem)->HasAttack() )
				{
					sDamage.assignf( "%f-%f", (*iItem)->GetAttack()->GetDamageMin(), (*iItem)->GetAttack()->GetDamageMax() );	
					
					float avg = ((*iItem)->GetAttack()->GetDamageMax() + (*iItem)->GetAttack()->GetDamageMin())/2;
					if ( avg != 0 )
					{
						level = avg - ( avg * ((( (*iItem)->GetAttack()->GetDamageMax() - (*iItem)->GetAttack()->GetDamageMin())/2) / avg ) / 10 );
					}
				}
				if ( (*iItem)->HasDefend() )
				{
					defense = (*iItem)->GetDefend()->GetDefense();
					level = defense;
				}

				if ( (*iItem)->IsMeleeWeapon() )
				{
					sClass = "Melee Weapon";
				}
				else if ( (*iItem)->IsRangedWeapon() )
				{
					sClass = "Ranged Weapon";
				}
				else if ( (*iItem)->IsSpell() )
				{
					sClass = "Spell";
				}
				else if ( (*iItem)->IsShield() )
				{
					sClass = "Shield";
				}
				else if ( (*iItem)->IsArmor() )
				{
					sClass = "Armor";
				}
				else if ( (*iItem)->IsPotion() )
				{
					sClass = "Potion";
				}
				else if ( (*iItem)->IsGold() )
				{
					sClass = "Gold";
				}

				sTemp.assignf( "%s,0x%08X,%f,%s,%s,%f,%d,%s\r\n", (*iItem)->GetTemplateName(),
															MakeInt((*iItem)->GetScid()),
															level, sClass.c_str(), sDamage.c_str(),
															defense, (*iItem)->GetAspect()->GetGoldValue(),
															sMagical.c_str() );
				WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
			}
		}	
	}

	CloseHandle( hFile );
}


void EditorTuning::ReportEnemyItem( bool bRegion, bool bPath )
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{		
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_enemy_items.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "Enemy Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			GoidColl objects;
			GetObjectsInPath( *k, objects );
			ReportEnemyItemHelper( hFile, objects );					
		}
	}
	
	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_enemy_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Enemy Items Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
	
		GoidColl objects;		
		gGoDb.GetAllGlobalGoids( objects );
		ReportEnemyItemHelper( hFile, objects );
	}
}


void EditorTuning::ReportEnemyItemHelper( HANDLE hFile, GoidColl objects )
{
	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	sTemp.assign( "Template Name,Scid,Level,Class,Damage,Defense,Gold Value,Magical\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	GoidColl::iterator i;
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( gAIQuery.Is( NULL, hObject, QT_EVIL ) )
		{			
			gGameAuditor.Set( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName(), 0 );			
		}
	}

	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( hObject->HasActor() && gAIQuery.Is( NULL, hObject, QT_EVIL ) )
		{
			GopColl items;
			GopColl::iterator iItem;
			hObject->GetInventory()->ListItems( IL_ALL, items );
			for ( iItem = items.begin(); iItem != items.end(); ++iItem )
			{
				float	level	= 0;						
				float	defense = 0.0f;
				gpstring sMagical = "No";
				gpstring sClass;
				gpstring sDamage = "0";

				if ( (*iItem)->HasGui() && !(*iItem)->GetGui()->IsDroppable() )
				{
					continue;
				}

				if ( (*iItem)->IsEquipped() && hObject->GetInventory()->GetEquippedItemsSpewType( gServer.GetLocalHumanPlayer()->GetId() ) != GoInventory::SPEW_GROUND )
				{
					continue;
				}

				if ( (*iItem)->HasMagic() )
				{
					sMagical = "Yes";
				}

				if ( (*iItem)->HasAttack() )
				{
					sDamage.assignf( "%f-%f", (*iItem)->GetAttack()->GetDamageMin(), (*iItem)->GetAttack()->GetDamageMax() );	
					
					float avg = ((*iItem)->GetAttack()->GetDamageMax() + (*iItem)->GetAttack()->GetDamageMin())/2;
					if ( avg != 0 )
					{
						level = avg - ( avg * ((( (*iItem)->GetAttack()->GetDamageMax() - (*iItem)->GetAttack()->GetDamageMin())/2) / avg ) / 10 );
					}
				}
				if ( (*iItem)->HasDefend() )
				{
					defense = (*iItem)->GetDefend()->GetDefense();
					level = defense;
				}

				if ( (*iItem)->IsMeleeWeapon() )
				{
					sClass = "Melee Weapon";
				}
				else if ( (*iItem)->IsRangedWeapon() )
				{
					sClass = "Ranged Weapon";
				}
				else if ( (*iItem)->IsSpell() )
				{
					sClass = "Spell";
				}
				else if ( (*iItem)->IsShield() )
				{
					sClass = "Shield";
				}
				else if ( (*iItem)->IsArmor() )
				{
					sClass = "Armor";
				}
				else if ( (*iItem)->IsPotion() )
				{
					sClass = "Potion";
				}
				else if ( (*iItem)->IsGold() )
				{
					sClass = "Gold";
				}

				sTemp.assignf( "%s,0x%08X,%f,%s,%s,%f,%d,%s\r\n", (*iItem)->GetTemplateName(),
															MakeInt((*iItem)->GetScid()),
															level, sClass.c_str(), sDamage.c_str(),
															defense, (*iItem)->GetAspect()->GetGoldValue(),
															sMagical.c_str() );
				WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
			}

			gGameAuditor.Increment( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName() );
		}
	}

	CloseHandle( hFile );
}


void EditorTuning::ReportPlacedItem( bool bRegion, bool bPath )
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{		
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_placed_items.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "Placed Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			GoidColl objects;
			GetObjectsInPath( *k, objects );
			ReportPlacedItemHelper( hFile, objects );								
		}
	}

	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_placed_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Placed Items Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
		
		GoidColl objects;		
		gGoDb.GetAllGlobalGoids( objects );
		ReportPlacedItemHelper( hFile, objects );		
	}
}


void EditorTuning::ReportPlacedItemHelper( HANDLE hFile, GoidColl objects )
{
	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	sTemp.assign( "Template Name,Scid,Level,Class,Damage,Defense,Gold Value,Magical\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	GoidColl::iterator i;
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );

		if ( hObject->IsItem() && !hObject->GetParent() )
		{
			float	level	= 0;						
			float	defense = 0.0f;
			gpstring sMagical = "No";
			gpstring sClass;
			gpstring sDamage = "0";

			if ( hObject->HasMagic() )
			{
				sMagical = "Yes";
			}

			if ( hObject->HasAttack() )
			{
				sDamage.assignf( "%f-%f", hObject->GetAttack()->GetDamageMin(), hObject->GetAttack()->GetDamageMax() );	
				
				float avg = (hObject->GetAttack()->GetDamageMax() + hObject->GetAttack()->GetDamageMin())/2;
				if ( avg != 0 )
				{
					level = avg - ( avg * ((( hObject->GetAttack()->GetDamageMax() - hObject->GetAttack()->GetDamageMin())/2) / avg ) / 10 );
				}
			}
			if ( hObject->HasDefend() )
			{
				defense = hObject->GetDefend()->GetDefense();
				level = defense;
			}

			if ( hObject->IsMeleeWeapon() )
			{
				sClass = "Melee Weapon";
			}
			else if ( hObject->IsRangedWeapon() )
			{
				sClass = "Ranged Weapon";
			}
			else if ( hObject->IsSpell() )
			{
				sClass = "Spell";
			}
			else if ( hObject->IsShield() )
			{
				sClass = "Shield";
			}
			else if ( hObject->IsArmor() )
			{
				sClass = "Armor";
			}
			else if ( hObject->IsPotion() )
			{
				sClass = "Potion";
			}
			else if ( hObject->IsGold() )
			{
				sClass = "Gold";
			}

			sTemp.assignf( "%s,0x%08X,%f,%s,%s,%f,%d,%s\r\n", hObject->GetTemplateName(),
														MakeInt(hObject->GetScid()),
														level, sClass.c_str(), sDamage.c_str(),
														defense, hObject->GetAspect()->GetGoldValue(),
														sMagical.c_str() );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
		}
	}

	CloseHandle( hFile );
}


void EditorTuning::ReportEnemy( bool bRegion, bool bPath )
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_enemies.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "Enemies Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			GoidColl objects;
			GetObjectsInPath( *k, objects );
			ReportEnemyHelper( hFile, objects );											
		}
	}

	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_enemies.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Enemies Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
		
		GoidColl objects;
		gGoDb.GetAllGlobalGoids( objects );
		ReportEnemyHelper( hFile, objects );		
	}
}


void EditorTuning::ReportEnemyHelper( HANDLE hFile, GoidColl objects )
{
	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	float	totalExp	= 0;
	float	totalLife	= 0;
	int		totalGold	= 0;
	int		totalEnemies = 0;

	sTemp.assign( "Enemies Breakdown:\r\n\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Name,Life,Experience,Total Gold Value,Actual Gold\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	GoidColl generatedObjects;
	GoidColl deletionList;
	
	GoidColl::iterator i = objects.begin();
	
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );
		if ( gAIQuery.Is( NULL, hObject, QT_EVIL ) )
		{			
			gGameAuditor.Set( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName(), 0 );			
		}
	}
	i = objects.begin();

	while ( i != objects.end() )
	{
		GoHandle hObject( *i );
		if ( hObject->HasActor() && gAIQuery.Is( NULL, hObject, QT_EVIL ) )				
		{								
			totalExp += hObject->GetAspect()->GetExperienceValue();
			totalLife += hObject->GetAspect()->GetMaxLife();
			
			int gold = 0;
			int actualGold = 0;
			GopColl items;
			GopColl::iterator iItem;
			hObject->GetInventory()->ListItems( IL_ALL, items );
			for ( iItem = items.begin(); iItem != items.end(); ++iItem )
			{
				if ( (*iItem)->HasGui() && !(*iItem)->GetGui()->IsDroppable() )
				{
					continue;
				}

				if ( (*iItem)->IsEquipped() && hObject->GetInventory()->GetEquippedItemsSpewType( gServer.GetLocalHumanPlayer()->GetId() ) != GoInventory::SPEW_GROUND )
				{
					continue;
				}

				gold		+= (*iItem)->GetAspect()->GetGoldValue();
				totalGold	+= (*iItem)->GetAspect()->GetGoldValue();
			}
			actualGold = hObject->GetInventory()->GetGold();
			gold += hObject->GetInventory()->GetGold();
			totalGold += hObject->GetInventory()->GetGold();					
			
			sTemp.assignf( "%s,%f,%f,%d,%d\r\n", hObject->GetTemplateName(), hObject->GetAspect()->GetMaxLife(), hObject->GetAspect()->GetExperienceValue(), gold, actualGold );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			totalEnemies++;

			gGameAuditor.Increment( GE_PLAYER_KILL, MakeInt( gServer.GetLocalHumanPlayer()->GetId() ), hObject->GetTemplateName() );
		}
		else
		{
			if ( hObject->HasComponent( "generator_basic" ) )			 
			{
				int numChildren = hObject->GetComponentInt( "generator_basic", "num_children_incubating", 0 );
				gpstring sTemplate = hObject->GetComponentString( "generator_basic", "child_template_name" );
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
					}
				}			
			}		
			else if ( hObject->HasComponent( "generator_advanced_a1" ) )
			{
				int numChildren = hObject->GetComponentInt( "generator_advanced_a1", "num_children_incubating", 0 );			
				gpstring sTemplate = hObject->GetComponentString( "generator_advanced_a1", "child_template_name" );				
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
					}
				}
			}
			else if ( hObject->HasComponent( "generator_advanced_a2" ) )
			{
				int numChildren = hObject->GetComponentInt( "generator_advanced_a2", "num_children_incubating", 0 );
				gpstring sTemplate = hObject->GetComponentString( "generator_advanced_a2", "child_template_name" );
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
						deletionList.push_back( object );
					}
				}			
			}	
			else if ( hObject->HasComponent( "generator_breakable" ) )
			{
				int numChildren = hObject->GetComponentInt( "generator_breakable", "num_children_incubating", 0 );
				gpstring sTemplate = hObject->GetComponentString( "generator_breakable", "child_template_name" );
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
						deletionList.push_back( object );
					}
				}			
			}
			else if ( hObject->HasComponent( "generator_auto_object_exploding" ) )
			{
				int numChildren = 1;
				gpstring sTemplate = hObject->GetComponentString( "generator_auto_object_exploding", "child_template_name" );
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
						deletionList.push_back( object );
					}
				}			
			}
			else if ( hObject->HasComponent( "generator_auto_advanced_a1" ) )
			{
				int numChildren = hObject->GetComponentInt( "generator_auto_advanced_a1", "num_children_incubating", 0 );
				gpstring sTemplate = hObject->GetComponentString( "generator_auto_advanced_a1", "child_template_name" );
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
						deletionList.push_back( object );
					}
				}			
			}
			else if ( hObject->HasComponent( "generator_auto_advanced_a2" ) )
			{
				int numChildren = hObject->GetComponentInt( "generator_auto_advanced_a2", "num_children_incubating", 0 );
				gpstring sTemplate = hObject->GetComponentString( "generator_auto_advanced_a2", "child_template_name" );
				GoCloneReq cloneReq( sTemplate );
				cloneReq.m_OmniGo = true;
				
				for ( int iChild = 0; iChild != numChildren; ++iChild )
				{
					Goid object = gGoDb.SCloneGo( cloneReq );
					if ( object != GOID_INVALID )
					{
						generatedObjects.push_back( object );
						deletionList.push_back( object );
					}
				}			
			}
		}

		i++;

		if ( i == objects.end() && generatedObjects.size() != 0 )
		{
			objects.clear();
			GoidColl::iterator iChild;
			for ( iChild = generatedObjects.begin(); iChild != generatedObjects.end(); ++iChild )
			{
				objects.push_back( *iChild );
			}

			i = objects.begin();
			generatedObjects.clear();
		}
	}

	GoidColl::iterator iDelete;
	for ( iDelete = deletionList.begin(); iDelete != deletionList.end(); ++iDelete )
	{
		gEditorObjects.DeleteObject( *iDelete );
	}

	sTemp.assignf( "\r\nTotal Enemies: %d\r\n", totalEnemies );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Experience: %f\r\n", totalExp );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Life: %f\r\n", totalLife );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Gold: %d\r\n", totalGold );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	CloseHandle( hFile );
}


void EditorTuning::ReportPartyMember( bool bRegion, bool bPath )
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	if ( bPath )
	{
		StringVec paths;
		StringVec::iterator k;		
		GetTuningPathNames( paths );
		for ( k = paths.begin(); k != paths.end(); ++k )
		{		
			gpstring sFile;
			sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_npcs.txt" );

			HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( hFile == NULL )
			{
				return;
			}

			gpstring sTemp;
			sTemp.assignf( "NPC Summary for Path: %s\r\n\r\n", (*k).c_str() );
			
			DWORD dwBytesWritten = 0;
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			GoidColl objects;
			GetObjectsInPath( *k, objects );
			ReportPartyMemberHelper( hFile, objects );														
		}
	}

	if ( bRegion )
	{
		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), gEditorRegion.GetRegionName().c_str(), "_region_npcs.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "NPC Summary for Region: %s\r\n\r\n", gEditorRegion.GetRegionName().c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoidColl objects;
		gGoDb.GetAllGlobalGoids( objects );
		ReportPartyMemberHelper( hFile, objects );		
	}
}


void EditorTuning::ReportPartyMemberHelper( HANDLE hFile, GoidColl objects )
{
	gpstring sTemp;
	DWORD dwBytesWritten = 0;

	sTemp.assignf( "Name,Life,Experience,Gold,Store,Party Member\r\n" );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
	
	int totalNpcs			= 0;
	int totalStores			= 0;
	int totalPartyMembers	= 0;
	int totalExp			= 0;
	int totalLife			= 0;
	int totalGold			= 0;
	
	GoidColl::iterator i;		
	for ( i = objects.begin(); i != objects.end(); ++i )
	{
		GoHandle hObject( *i );

		if ( hObject->HasActor() && (gAIQuery.Is( NULL, hObject, QT_GOOD ) || gAIQuery.Is( NULL, hObject, QT_NEUTRAL )) )				
		{
			gpstring sStore = "false";
			gpstring sParty = "false";

			if ( hObject->GetParent() && hObject->GetParent()->HasParty() )
			{
				totalPartyMembers++;
				sParty = "true";
			}

			if ( hObject->HasStore() )
			{
				totalStores++;
				sStore = "true";
			}

			totalNpcs++;

			int gold = 0;
			GopColl items;
			GopColl::iterator iItem;
			hObject->GetInventory()->ListItems( IL_ALL, items );
			for ( iItem = items.begin(); iItem != items.end(); ++iItem )
			{
				gold		+= (*iItem)->GetAspect()->GetGoldValue();						
			}
			gold += hObject->GetInventory()->GetGold();

			totalGold += gold;
			
			sTemp.assignf( "%s,%f,%f,%d,%s,%s\r\n", hObject->GetTemplateName(), hObject->GetAspect()->GetMaxLife(), hObject->GetAspect()->GetExperienceValue(), gold, sStore.c_str(), sParty.c_str() );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			totalNpcs++;
		}
	}

	sTemp.assignf( "\r\nTotal NPCs: %d\r\n", totalNpcs );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Experience: %f\r\n", totalExp );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Life: %f\r\n", totalLife );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Gold: %d\r\n", totalGold );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Stores: %d\r\n", totalStores );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	sTemp.assignf( "Total Party Members: %d\r\n", totalPartyMembers );
	WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

	CloseHandle( hFile );
}

void EditorTuning::GeneratePointSummary()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_summary.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		sTemp.assignf( "Point,Name,Distance,Total Objects,Enemies,NPCs,Items,Gold,Gold Item Value\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int goodActors		= 0;
			int evilActors		= 0;
			int totalGold		= 0;
			int items			= 0;
			int totalObjects	= 0;
			int goldValue		= 0;			
			
			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );
			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( (*i)->HasActor() && gAIQuery.Is( NULL, *i, QT_GOOD ) )				
				{
					goodActors++;
				}				
				else if ( (*i)->HasActor() && gAIQuery.Is( NULL, *i, QT_EVIL ) )		
				{
					evilActors++;
				}
				else if ( (*i)->IsItem() && ( (*i)->IsArmor() || (*i)->IsSpell() || (*i)->IsWeapon() || (*i)->IsShield() || (*i)->IsGold() || (*i)->IsPotion() ) )
				{
					items++;
				}

				totalObjects++;

				if ( (*i)->IsItem() && (*i)->IsGold() )
				{
					totalGold += (*i)->GetAspect()->GetGoldValue();						
					goldValue += (*i)->GetAspect()->GetGoldValue();
				}				
				else if ( (*i)->HasActor() )
				{
					totalGold += (*i)->GetInventory()->GetGold();
					goldValue += (*i)->GetAspect()->GetGoldValue();
				}
				else if ( (*i)->HasAspect() )
				{
					goldValue += (*i)->GetAspect()->GetGoldValue();
				}
				
				totalObjects++;
			}

			float distance = 0;	
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%d,%d,%d\r\n", point++, GetPointName(hPoint->GetGoid()).c_str(),distance, totalObjects, evilActors, goodActors, items, totalGold, goldValue );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			
			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}


void EditorTuning::ReportAllItemPoint()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";	

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_all_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point All Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );
		
		sTemp.assignf( "Point,Name,Distance,Total Objects,Gold,Total Gold Value,Spells,Armor,Weapon,Potion\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )				 
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int totalItems = 0;
			int totalGold = 0;
			int goldValue = 0;
			int spells = 0;
			int weapons = 0;			
			int armor = 0;
			int potion = 0;

			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );

			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( (*i)->IsItem() && ( (*i)->IsArmor() || (*i)->IsSpell() || (*i)->IsWeapon() || (*i)->IsShield() || (*i)->IsGold() || (*i)->IsPotion() ) )
				{
					totalItems++;
					if ( (*i)->IsGold() )
					{
						totalGold += (*i)->GetAspect()->GetGoldValue();						
					}
					goldValue += (*i)->GetAspect()->GetGoldValue();
					
					if ( (*i)->IsArmor() )
					{
						armor++;
					}
					if ( (*i)->IsSpell() )
					{
						spells++;
					}
					if ( (*i)->IsWeapon() )
					{
						weapons++;
					}
					if ( (*i)->IsPotion() )
					{
						potion++;
					}
				}
			}

			float distance = 0;			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%d,%d,%d,%d\r\n", point++, GetPointName(hPoint->GetGoid()).c_str(), distance, totalItems, totalGold, goldValue, spells, armor, weapons, potion );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}


void EditorTuning::ReportContainerItemPoint()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_container_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point Container Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		sTemp.assignf( "Point,Name,Distance,Total Containers,Gold,Total Gold Value,Spells,Armor,Weapon,Potion\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )				 
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int totalItems = 0;
			int totalGold = 0;
			int goldValue = 0;
			int spells = 0;
			int weapons = 0;			
			int armor = 0;
			int potion = 0;

			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );

			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( (*i)->IsItem() && (*i)->IsContainer() )
				{
					totalItems++;

					GopColl items;
					GopColl::iterator iItem;
					(*i)->GetInventory()->ListItems( IL_ALL, items );
					for ( iItem = items.begin(); iItem != items.end(); ++iItem )
					{
						if ( (*iItem)->IsGold() )
						{
							totalGold += (*iItem)->GetAspect()->GetGoldValue();						
						}
						goldValue += (*iItem)->GetAspect()->GetGoldValue();
						
						if ( (*iItem)->IsArmor() )
						{
							armor++;
						}
						if ( (*iItem)->IsSpell() )
						{
							spells++;
						}
						if ( (*iItem)->IsWeapon() )
						{
							weapons++;
						}
						if ( (*iItem)->IsPotion() )
						{
							potion++;
						}										
					}					
				}
			}

			float distance = 0;			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%d,%d,%d,%d\r\n", point++, GetPointName(hPoint->GetGoid()).c_str(), distance, totalItems, totalGold, goldValue, spells, armor, weapons, potion );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}


void EditorTuning::ReportEnemyItemPoint()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_enemy_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point Enemy Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		sTemp.assignf( "Point,Name,Distance,Total Enemies,Gold,Total Gold Value,Spells,Armor,Weapon,Potion\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int totalEnemies = 0;
			int totalGold = 0;
			int goldValue = 0;
			int spells = 0;
			int weapons = 0;			
			int armor = 0;
			int potion = 0;

			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );

			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( (*i)->HasActor() && gAIQuery.Is( NULL, *i, QT_EVIL ) )		
				{
					totalEnemies++;

					GopColl items;
					GopColl::iterator iItem;
					(*i)->GetInventory()->ListItems( IL_ALL, items );
					for ( iItem = items.begin(); iItem != items.end(); ++iItem )
					{
						if ( (*iItem)->IsGold() )
						{
							totalGold += (*iItem)->GetAspect()->GetGoldValue();						
						}
						goldValue += (*iItem)->GetAspect()->GetGoldValue();
						
						if ( (*iItem)->IsArmor() )
						{
							armor++;
						}
						if ( (*iItem)->IsSpell() )
						{
							spells++;
						}
						if ( (*iItem)->IsWeapon() )
						{
							weapons++;
						}
						if ( (*iItem)->IsPotion() )
						{
							potion++;
						}										
					}					
				}
			}

			float distance = 0;			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%d,%d,%d,%d\r\n", point++, GetPointName(hPoint->GetGoid()).c_str(), distance, totalEnemies, totalGold, goldValue, spells, armor, weapons, potion );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}


void EditorTuning::ReportPlacedItemPoint()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_placed_items.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point Placed Items Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		sTemp.assignf( "Point,Name,Distance,Total Placed Items,Gold,Total Gold Value,Spells,Armor,Weapon,Potion\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int totalItems = 0;
			int totalGold = 0;
			int goldValue = 0;
			int spells = 0;
			int weapons = 0;			
			int armor = 0;
			int potion = 0;

			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );

			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( !(*i)->GetParent() && (*i)->IsItem() && ( (*i)->IsArmor() || (*i)->IsSpell() || (*i)->IsWeapon() || (*i)->IsShield() || (*i)->IsGold() || (*i)->IsPotion() ) )
				{
					totalItems++;

					if ( (*i)->IsGold() )
					{
						totalGold += (*i)->GetAspect()->GetGoldValue();						
					}
					goldValue += (*i)->GetAspect()->GetGoldValue();
					
					if ( (*i)->IsArmor() )
					{
						armor++;
					}
					if ( (*i)->IsSpell() )
					{
						spells++;
					}
					if ( (*i)->IsWeapon() )
					{
						weapons++;
					}
					if ( (*i)->IsPotion() )
					{
						potion++;
					}									
				}
			}

			float distance = 0;			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%d,%d,%d,%d\r\n", point, GetPointName(hPoint->GetGoid()).c_str(), distance, totalItems, totalGold, goldValue, spells, armor, weapons, potion );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}


void EditorTuning::ReportEnemyPoint()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_enemies.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point Enemies Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		sTemp.assignf( "Point,Name,Distance,Total Enemies,Gold,Total Gold Value,Total Life,Total Experience,Total Mana\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int totalEnemies = 0;
			int totalGold = 0;
			int goldValue = 0;
			float totalLife = 0;
			float totalExperience = 0;
			float totalMana = 0;
			
			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );

			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( (*i)->HasActor() && gAIQuery.Is( NULL, *i, QT_EVIL ) )		
				{
					totalEnemies++;

					GopColl items;
					GopColl::iterator iItem;
					(*i)->GetInventory()->ListItems( IL_ALL, items );
					for ( iItem = items.begin(); iItem != items.end(); ++iItem )
					{
						if ( (*iItem)->IsGold() )
						{
							totalGold += (*iItem)->GetAspect()->GetGoldValue();						
						}
						goldValue += (*iItem)->GetAspect()->GetGoldValue();																			
					}
					
					totalLife += (*i)->GetAspect()->GetMaxLife();
					totalExperience += (*i)->GetAspect()->GetExperienceValue();
					totalMana += (*i)->GetAspect()->GetMaxMana();
				}
			}

			float distance = 0;			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%f,%f,%f\r\n", point, GetPointName(hPoint->GetGoid()).c_str(), distance, totalEnemies, totalGold, goldValue, totalLife, totalExperience, totalMana );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}


void EditorTuning::ReportPartyMemberPoint()
{
	gpstring sDir = FileSys::GetCurrentDirectory( true );
	sDir += "tuning\\";
	sDir += gEditorRegion.GetRegionName();	
	sDir += "\\";

	StringVec paths;
	StringVec::iterator k;		
	GetTuningPathNames( paths );
	for ( k = paths.begin(); k != paths.end(); ++k )
	{	
		UsedGoidSet usedGoids;

		gpstring sFile;
		sFile.assignf( "%s%s%s", sDir.c_str(), (*k).c_str(), "_point_party_members.txt" );

		HANDLE hFile = CreateFile( sFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == NULL )
		{
			return;
		}

		gpstring sTemp;
		sTemp.assignf( "Point Party Member Summary for Path: %s\r\n\r\n", (*k).c_str() );
		
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		sTemp.assignf( "Point,Name,Distance,Total Enemies,Gold,Total Gold Value,Spells,Armor,Weapon,Potion\r\n" );
		WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

		GoHandle hPoint( GetFirstPoint( *k ) );
		int point = 0;
		while ( hPoint.IsValid() )
		{
			if ( GetPointName( hPoint->GetGoid() ).same_no_case( "dup" ) )
			{
				hPoint = GetNextPoint( hPoint->GetGoid() );
				continue;
			}

			int totalNpcs = 0;
			int totalGold = 0;
			int goldValue = 0;
			int spells = 0;
			int weapons = 0;			
			int armor = 0;
			int potion = 0;

			GopColl occupants;
			GopColl::iterator i;
			gAIQuery.GetOccupantsInSphere( hPoint->GetPlacement()->GetPosition(), GetRadius( hPoint->GetGoid() ), occupants );

			for ( i = occupants.begin(); i != occupants.end(); ++i )
			{
				if ( usedGoids.find( (*i)->GetGoid() ) != usedGoids.end() )
				{
					continue;
				}
				else
				{
					usedGoids.insert( (*i)->GetGoid() );
				}

				if ( (*i)->HasActor() && gAIQuery.Is( NULL, *i, QT_GOOD ) )		
				{
					totalNpcs++;

					GopColl items;
					GopColl::iterator iItem;
					(*i)->GetInventory()->ListItems( IL_ALL, items );
					for ( iItem = items.begin(); iItem != items.end(); ++iItem )
					{
						if ( (*iItem)->IsGold() )
						{
							totalGold += (*i)->GetAspect()->GetGoldValue();						
						}
						goldValue += (*i)->GetAspect()->GetGoldValue();
						
						if ( (*iItem)->IsArmor() )
						{
							armor++;
						}
						if ( (*iItem)->IsSpell() )
						{
							spells++;
						}
						if ( (*iItem)->IsWeapon() )
						{
							weapons++;
						}
						if ( (*iItem)->IsPotion() )
						{
							potion++;
						}												
					}					
				}
			}

			float distance = 0;			
			if ( !GetPointName( hPoint->GetGoid() ).same_no_case( "elev" ) )
			{
				GoHandle hPrevious( GetPreviousPoint( hPoint->GetGoid() ) );

				if ( hPrevious.IsValid() )
				{			
					distance = hPoint->GetPlacement()->GetPosition().WorldPos().Distance( hPrevious->GetPlacement()->GetPosition().WorldPos() );
				}
			}

			sTemp.assignf( "%d,%s,%f,%d,%d,%d,%d,%d,%d,%d\r\n", point, GetPointName(hPoint->GetGoid()).c_str(), distance, totalNpcs, totalGold, goldValue, spells, armor, weapons, potion );
			WriteFile( hFile, (void *)sTemp.c_str(), sTemp.size(), &dwBytesWritten, NULL );

			hPoint = GetNextPoint( hPoint->GetGoid() );
		}

		CloseHandle( hFile );
	}
}