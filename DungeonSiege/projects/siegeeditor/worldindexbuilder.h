#pragma once
#ifndef __WORLDINDEXBUILDER_H
#define __WORLDINDEXBUILDER_H

#include "Fuel.h"
#include "gpstring.h"
#include "SiegeEditorTypes.h"
#include "GoDefs.h"


namespace FileSys 
{
	class AutoFindHandle;
}


class WorldIndexBuilder
{


public:

	WorldIndexBuilder();
	~WorldIndexBuilder(){};

	bool BuildMapNodeMeshIndex( FuelHandle const & hRegionDir );
		
	bool BuildRegionStreamerIndexes( FuelHandle const & hRegionDir, bool bNodeContent = true );

	bool BuildRegionStreamerNodeIndex( FuelHandle const & hRegionDir, FuelHandle & hIndexDir );

	bool BuildRegionStreamerNodeContentIndex( FuelHandle const & hRegionDir, FuelHandle & hIndexDir );
};




#endif
