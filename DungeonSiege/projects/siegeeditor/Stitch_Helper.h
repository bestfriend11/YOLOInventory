/**************************************************************************

Filename		: Stitch_Helper.h

Description		: This class makes a list of nodes and node ids that allows
				  level designers to stitch together regions easier

Creation Date	: 4/3/00

Author			: Chad Queen

**************************************************************************/


#pragma once
#ifndef _STITCH_HELPER_IO_H_
#define _STITCH_HELPER_IO_H_


// Include Files
#include "siege_database_guid.h"
#include <string>
#include <vector>
#include "fuel.h"


// Forward Declarations
class SiegeEditorShell;
class FuelHandle;


// A node in a stitch unit
struct StitchUnit
{
	gpstring				sRegion;
	int						nodeID;
	int						doorID;
	siege::database_guid	nodeGUID;
};

typedef std::vector< StitchUnit > StitchVector;
typedef std::map< gpstring, StitchVector, istring_less > MapStitchMap;

class Stitch_Helper : public Singleton <Stitch_Helper>
{

public:

	// Constructor
	Stitch_Helper() {};

	// Destructor
	~Stitch_Helper() {};

	// Load the intial list of the stitched nodes
	void Load();	

	// Save the list of the stitched nodes
	void Save();

	// Remove a NodeID from the stitch helper list
	void RemoveNodeIDFromStitchHelper( int nodeID );
	void AddNodeToStitchHelper( siege::database_guid nodeGUID, int nodeID, int doorID, gpstring sRegion );
	bool FindNodeDoorInStitchHelper( siege::database_guid nodeGUID, int doorID, int & nodeID, gpstring & sRegion );
	void SetNodeDoorStitchHelperInfo( siege::database_guid nodeGUID, int nodeID, int doorID, gpstring sRegion );

	StitchVector & GetStitchVector()	{ return m_stitch_vector;		}
	void GetMapStitchVector( gpstring sRegionName, StitchVector & region_stitches );

	// Build a stitch index for a map
	void BuildStitchIndex( gpstring sMap );

	// Delete a node references in the stitch helper
	void DeleteNodeFromStitchIndex( siege::database_guid nodeGUID );


private:

	bool FindMatchingNodeIDInRegion( int nodeID, gpstring sSourceRegion, gpstring sDestRegion, FuelHandleList hlStitchRegions, gpstring & sNode, gpstring & sDoorID );

	void CalculateStitchBoundary( FuelHandle& hMap, FuelHandle& hStitchIndex,
								  gpstring& sRegionName, gpstring& sNode, gpstring& sDoor,
								  gpstring& dRegionName, gpstring& dNode, gpstring& dDoor );

	StitchVector		m_stitch_vector;
	MapStitchMap		m_MapStitchMap;

};



#define gStitchHelper Stitch_Helper::GetSingleton()

#endif