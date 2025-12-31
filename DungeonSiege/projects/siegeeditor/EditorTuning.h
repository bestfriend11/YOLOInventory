//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorTuning.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __EDITORTUNING_H
#define __EDITORTUNING_H

#include "EditorMap.h"

struct ReportReq
{
	bool bSummary;
	bool bSummaryRegion;
	bool bSummaryPoint;
	bool bPlacedItem;
	bool bPlacedItemRegion;
	bool bPlacedItemPoint;
	bool bPartyMember;
	bool bPartyMemberRegion;
	bool bPartyMemberPoint;
	bool bEnemyItem;
	bool bEnemyItemRegion;
	bool bEnemyItemPoint;
	bool bEnemy;
	bool bEnemyRegion;
	bool bEnemyPoint;
	bool bContainerItem;
	bool bContainerItemRegion;
	bool bContainerItemPoint;
	bool bAllItem;
	bool bAllItemRegion;
	bool bAllItemPoint;
};

struct tuningPContent
{
	int gold;
	StringVec objectTemplates;
};


typedef std::set< Goid > UsedGoidSet;


class EditorTuning : public Singleton <EditorTuning>
{
public:

	EditorTuning();
	~EditorTuning() {}

	void Draw();	
	
	gpstring GetPathName( Goid object );

	void SetMakerEnabled( bool bEnabled )	{ m_bMakerEnabled = bEnabled;	}
	bool GetMakerEnabled()					{ return m_bMakerEnabled;		}

	void GetPointsInPath( gpstring sPath, GoidColl & points );

	void GetTuningPathNames( StringVec & paths );
	
	bool GetAutoSize()					{ return m_bAutoSize; }
	void SetAutoSize( bool bSet )		{ m_bAutoSize = bSet; }
	
	gpstring GetSelectedPath()					{ return m_selectedPath; }
	void	 SetSelectedPath( gpstring sPath )	{ m_selectedPath = sPath; }

	void  SetPosition( Goid object, SiegePos spos );

	Goid  GetPreviousPoint( Goid point );
	Goid  GetNextPoint( Goid point );
	Goid  GetFirstPoint( gpstring sPath );
		
	void  SetRadius( Goid object, float radius );
	void  SetRadius( float radius )	{ m_radius = radius;	}
	float GetRadius()				{ return m_radius;		}
	float GetRadius( Goid object );

	float GetRadiusTolerance()					{ return m_tolerance; }
	void  SetRadiusTolerance( float tolerance ) { m_tolerance = tolerance; }

	gpstring GetPointName( Goid object );

	void AddPoint();

	void SetPlacePoints( bool bPlace )	{ m_bPlacePoints = bPlace; }
	bool GetPlacePoints()				{ return m_bPlacePoints; }

	void RemovePath( gpstring sName );

	void GetObjectsInPath( gpstring sPath, GoidColl & objects );

	// Reporting-related methods

	void BatchGenerateTuningReports( MapToRegionMap & regionMap, bool bMasterReport, bool bTuningLoad );

	void SetReportRequest( ReportReq request )			{ m_reportReq = request; }
	ReportReq	GetReportRequest()						{ return m_reportReq; }

	void GenerateReports();

	void GenerateMasterPointSummary();

	void GenerateSummary( bool bRegion, bool bPath );
	void GenerateSummaryHelper( HANDLE hFile, GoidColl objects, bool bMasterReport = false );

	void GeneratePointSummary();

	void ReportPlacedItem( bool bRegion, bool bPath );
	void ReportPlacedItemHelper( HANDLE hFile, GoidColl objects );
	void ReportPlacedItemPoint();

	void ReportPartyMember( bool bRegion, bool bPath );
	void ReportPartyMemberHelper( HANDLE hFile, GoidColl objects );
	void ReportPartyMemberPoint();

	void ReportEnemyItem( bool bRegion, bool bPath );
	void ReportEnemyItemHelper( HANDLE hFile, GoidColl objects );
	void ReportEnemyItemPoint();

	void ReportEnemy( bool bRegion, bool bPath );
	void ReportEnemyHelper( HANDLE hFile, GoidColl objects );
	void ReportEnemyPoint();

	void ReportContainerItem( bool bRegion, bool bPath );
	void ReportContainerItemHelper( HANDLE hFile, GoidColl objects );
	void ReportContainerItemPoint();

	void ReportAllItem( bool bRegion, bool bPath );
	void ReportAllItemHelper( HANDLE hFile, GoidColl objects );
	void ReportAllItemPoint();	
	
private:
	
	ReportReq		m_reportReq;
	bool			m_bPlacePoints;
	bool			m_bAutoSize;
	float			m_radius;
	gpstring		m_selectedPath;
	float			m_tolerance;
	bool			m_bMakerEnabled;
	
	// Master Report Data
	bool			m_bMasterReport;
	HANDLE			m_hMasterFile;

	DWORD			m_totalEnemies;
	DWORD			m_totalExperience;
	DWORD			m_totalGold;
	
};


#define gEditorTuning EditorTuning::GetSingleton()


#endif