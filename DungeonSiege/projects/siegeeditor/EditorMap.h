//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorMap.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#pragma once
#ifndef __EDITORMAP_H
#define __EDITORMAP_H

// Include Files
#include "SiegeEditorTypes.h"
#include "gpcore.h"
#include "gpstring.h"
#include "siege_pos.h"

typedef std::multimap< gpstring, gpstring, istring_less > MapToRegionMap;
typedef std::pair< gpstring, gpstring > MapToRegionPair;

struct MapCreateInfo
{
	gpstring sMapName;
	gpstring sDescription;
	gpstring sScreenName;
	bool	 bDevOnly;
	float	 worldFrustumRadius;
	float	 worldInterestRadius;
	int		 startTimeHour;
	int		 startTimeMinute;
};


class EditorMap : public Singleton <EditorMap>
{
public:

	EditorMap();	
	~EditorMap() {};

	// Creation
	void CreateNewMap( MapCreateInfo & mapCreateInfo );

	// Advanced Settings
	void SetWorldFrustumRadius( float radius );
	float  GetWorldFrustumRadius();

	void SetWorldInterestRadius( float radius );
	float  GetWorldInterestRadius();

	// Camera
	void GetCameraSettings( float & y, float & z, float & x, int & nodeId,
							float & azimuth, float & orbit, 
							float & distance, float & minDistance, float & maxDistance,
							float & nearclip, float & farclip, float & fov,
							float & minAzimuth, float & maxAzimuth );

	void SetCameraSettings( float & y, float & z, float & x, int & nodeId,
							float & azimuth, float & orbit, 
							float & distance, float & minDistance, float & maxDistance,
							float & nearclip, float & farclip, float & fov,
							float & minAzimuth, float & maxAzimuth );	

	// For Batch LNC Generation
	void GenerateLNCs( MapToRegionMap & regions, bool bFullSave, bool bDeleteLNC, bool bLoadLights, bool bLoadDecals, 
					   bool bLoadLogical, bool bFullRecalc, bool bRemapMeshRange, bool bConvertGlobalNodes );

	// Teleportation ( Book Marks )
	void GetBookmarks( StringVec & bookmarks );
	
	void GetBookmarkParameters( gpstring sBookmark, SiegePos & spos, float & azimuth, float & orbit, float & distance, gpstring & sDescription );
	void SetBookmarkParameters( gpstring sOldName, gpstring sBookmark, SiegePos spos, float azimuth, float orbit, float distance, gpstring sDescription );
	
	bool AddBookmark( gpstring sBookmark, SiegePos spos, float azimuth, float orbit, float distance );
	void RemoveBookmark( gpstring sBookmark );

	void SetSelectedBookmark( gpstring sBookmark ) { m_sSelectedBookmark = sBookmark; }
	gpstring GetSelectedBookmark() { return m_sSelectedBookmark; }

	void ValidateBookmarks( gpstring sMap );
	void BuildMapNodeList( gpstring sMap, std::vector< int > nodeIds );

	// Multiplayer starting positions
	void GetStartPositions( StringVec & positions );

	void GetPositionParameters( int id, SiegePos & spos, SiegePos & cameraPos, float & azimuth, float & orbit, float & distance );
	void SetPositionParameters( int id, SiegePos spos, SiegePos cameraPos, float azimuth, float orbit, float distance );

	bool AddStartPosition( int id, SiegePos spos, SiegePos cameraPos, float azimuth, float orbit, float distance );
	void RemoveStartPosition( int id );	

	void SetSelectedStartID( int id ) { m_selectedStartID = id; }
	int GetSelectedStartID() { return m_selectedStartID; }

	void SetStartTime( int hour, int minutes );
	void GetStartTime( int & hour, int & minutes );
	
	void SetNewHotpointName( gpstring sName )	{ m_sNewHotpointName = sName;	}
	gpstring GetNewHotpointName()				{ return m_sNewHotpointName;	}
	void CreateNorthVector( gpstring sMapAddress );

	bool GetHasNodeMeshIndex() { return m_bHasNodeMeshMap; }
	void SetHasNodeMeshIndex( bool bSet ) { m_bHasNodeMeshMap = bSet; }

	void RemapNodeMeshes();
	void RemapMesh( FuelHandleList & hlNodes, gpstring sOldGuid, gpstring sNewGuid );

private:

	gpstring	m_sSelectedBookmark;
	int			m_selectedStartID;
	gpstring	m_sNewHotpointName;
	bool		m_bHasNodeMeshMap;

};


#define gEditorMap EditorMap::GetSingleton()



#endif
