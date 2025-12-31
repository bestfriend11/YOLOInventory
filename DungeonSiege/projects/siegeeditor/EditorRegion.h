//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorRegion.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#pragma once
#ifndef __EDITORREGION_H
#define __EDITORREGION_H

// Include Files
#include "gpcore.h"
#include "SiegeEditorTypes.h"
#include "Fuel.h"
#include "vector_3.h"
#include "GoDefs.h"


// Forward Declarations
class FuelHandle;


// Structures
struct REGION_DATA 
{
	FastFuelHandle	fh_region;	
	gpstring		map_name;
};

struct RegionCreateInfo
{	
	gpstring sMapName;
	gpstring sRegionName;
	gpstring sSnoFile;
	int scidRange;
	int meshRange;
	int regionId;
};

typedef std::map< unsigned int, unsigned int >	OffsetTableMap;


class EditorRegion : public Singleton <EditorRegion>
{
public:

	EditorRegion();	

	// Save all changes made to the gas tree
	void SaveRegion( bool bGameObjects, bool bLights, bool bNodes, bool bStitch, bool bStartingPos );

	// Load in region 
	bool LoadRegion( gpstring szName, gpstring szMap, 
					 bool bForceRecalc = false, bool bForceLoadLights = false, bool bForceLoadDecals = false, 
					 bool bTuningLoad = false, bool bForceLoadLogicalFlags = false, bool bFullRecalc = false  );

	// Creates a new blank region & quest
	void CreateRegion( RegionCreateInfo & regionCreation );

	// Setup the appropriate blank files and directories for manipulation
	void SetupBlankRegion( RegionCreateInfo & regionCreation );

	int GetDefaultScidRange( gpstring sMap );
	int GetDefaultRegionId( gpstring sMap );
	int	GetDefaultMeshRange( gpstring sMap, gpstring sRegion = gpstring::EMPTY );

	bool IsValidScidRange( gpstring sMap, int scidRange );
	bool IsValidRegionId( gpstring sMap, int regionId );
	bool IsValidMeshRange( gpstring sMap, int meshRange );
	
	// Get a list of the valid maps
	void GetMapList( StringVec & map_list );

	// Get a list of the valid regions
	void GetRegionList(	gpstring map_name, StringVec & region_list );
	
	// LNC Calculation
	void RecalculateLNC( FuelHandle hNodes, CProgressCtrl * pProgress );
	void SaveLNC();

	// loads siege nodes and builds their LNOs if needed
	bool BuildRegionLNOs( FuelHandle const & hRegionDir, bool bForceRecalc );

	// Fill in the passed map with parsed offset table data from the LNC file
	void BuildFileOffsetTable( HANDLE hFile, OffsetTableMap* pMap );
		
	// Recursive function that lists all files from a given root
	void GetFileList( StringVec & files, const char *szCurrentDirectory );
	
	// Backup gas files	
	void BackupGASFiles();
	void SetSaveBackups( bool bSave )	{ m_bSaveBackups = bSave;	}
	bool GetSaveBackups()				{ return m_bSaveBackups;	}	

	gpstring GetMapName()		{ return m_sMapName;	}
	
	gpstring	GetRegionName()	{ return m_sRegionName; }
	void		SetRegionName( gpstring sNewName );
	void		RenameRegion( FuelHandle hBlock, gpstring sRegionName, gpstring sNewName ); 

	gpstring	GetRegionDescription();
	void		SetRegionDescription( gpstring sDescription );

	gpstring	GetRegionNotes();
	void		SetRegionNotes( gpstring sNotes );
		
	// Set/Get Region Directory Fuel Handle
	void		SetRegionHandle( FuelHandle fhRegion )	{ m_fhRegion = fhRegion;	};
	FuelHandle	& GetRegionHandle()						{ return m_fhRegion;		};

	// Set/Get Map Directory Fuel Handle
	void		SetMapHandle( FuelHandle fhMap )		{ m_fhMap = fhMap;			};
	FuelHandle	& GetMapHandle()						{ return m_fhMap;			};

	void SetRegionGUID( RegionId id )	{ m_region_guid = id; }
	RegionId GetRegionGUID()			{ return m_region_guid; }	

	void	SetFrustumId( DWORD id )	{ m_frustumId = id;		}
	DWORD	GetFrustumId()				{ return m_frustumId;	}

	void	SetLoading( bool bLoading )	{ m_bLoading = bLoading; }
	bool	GetLoading()				{ return m_bLoading; }

	// Is there a region loaded/created that we can perform work on accessor functions
	void SetRegionEnable( bool bEnable )	{ m_bRegionEnable = bEnable; };
	bool GetRegionEnable()					{ return m_bRegionEnable; };	

	gpstring GetSelectedConversation()						{ return m_sSelectedConversation; }
	void SetSelectedConversation( gpstring sConversation )	{ m_sSelectedConversation = sConversation; }
	
	void SetRegionDirty( bool bSet = true )	{ m_bRegionDirty = bSet; }
	bool GetRegionDirty()					{ return m_bRegionDirty; }

	void SetMeshRange( int range )			{ m_MeshRange = range; }
	int  GetMeshRange()						{ return m_MeshRange;  }
	
private:	
	
	// Private Member Variables	
	bool									m_bRegionEnable;
	std::list< gpstring >					m_maps;	
	std::list< REGION_DATA >				m_regions;	
	FuelHandle								m_fhRegion;
	FuelHandle								m_fhMap;
	bool									m_bSaveBackups;
	gpstring								m_sRegionName;
	gpstring								m_sMapName;
	RegionId								m_region_guid;
	DWORD									m_frustumId;
	bool									m_bLoading;
	gpstring								m_sSelectedConversation;
	bool									m_bRegionDirty;
	int										m_MeshRange;

	// Name of the map and region we current have loaded
	gpstring								m_RegionDirectory;
};


#define gEditorRegion EditorRegion::GetSingleton()


gpstring GetUserInformation();

#endif
