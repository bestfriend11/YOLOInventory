#pragma once
#ifndef _SIEGE_DECAL_DATABASE_
#define _SIEGE_DECAL_DATABASE_

/************************************************************************************
**
**								SiegeDecalDatabase
**
**		This class is responsible for maintaining a central collection
**		location for decal information.
**
**		Author:  James Loe
**		Date:	 11/09/00
**
************************************************************************************/

namespace siege
{
	// Forward dec of decal class
	class SiegeDecal;

	// Container definitions
	typedef UnsafeBucketVector< SiegeDecal* >				DecalCollection;
	typedef std::list< unsigned int >						DecalIndexList;
	typedef std::map< database_guid, DecalIndexList >		DecalMap;
	typedef std::map< database_guid, SiegeDecal* >			DecalGuidMap;

	class SiegeDecalDatabase : public Singleton< SiegeDecalDatabase >
	{
	public:

		// Construction and destruction
		SiegeDecalDatabase();
		~SiegeDecalDatabase();

		// Takes a position and uses the face normal at that position to setup a
		// standard default projection.  Uses horizontal and vertical measurements
		// (in meters) to setup the initial scaling.
		unsigned int			CreateDecal( const SiegePos& pos, const vector_3& normal,
											 const float horizMeters, const float vertMeters,
											 const gpstring& decal_tex, bool bMipMapping = true,
											 bool bLight = true, bool bPerspCorrect = false );

		// Detailed construction.  Takes a projection origin and direction,
		// as well as a horizontal and vertical projection size to setup scale,
		// and both the near plane and far plane to build a proper frustum.
		unsigned int			CreateDecal( const SiegePos& origin, const matrix_3x3& orient,
											 const float horizMeters, const float vertMeters,
											 const float nearPlane, const float farPlane,
											 const gpstring& decal_tex, bool bMipMapping = true,
											 bool bLight = true, bool bPerspCorrect = false );

		// Load a decal
		unsigned int			LoadDecal( const char*& pData );
		void					UpgradeLoadDecal( unsigned int decal_index );
		void					DowngradeLoadDecal( unsigned int decal_index );

		// Destroy a decal
		void					DestroyDecal( unsigned int decal_index, bool bRemoveIndex = true );

		// Render the decals for this node.  Returns number of triangles rendered.
		unsigned int			RenderNodeDecals( const SiegeNode& node );

		// Update decal lighting for this node
		void					UpdateDecalLighting( const SiegeNode& node );

		// Update decal projection
		void					UpdateDecalProjection( const database_guid nodeGuid );

		// Get the decal pointer
		SiegeDecal*				GetDecalPointer( unsigned int decal_index );
		SiegeDecal*				GetDecalPointer( database_guid decal_guid );

		// Decal selection by ray test
		bool					GetHitDecal( const vector_3& ray_orig, const vector_3& ray_dir, unsigned int& hit_decal );

		// Get the decal collection
		const DecalCollection&	GetDecalCollection()						{ return m_DecalCollection; }

		// Generate a random GUID
		database_guid			GenerateRandomDecalGUID();

	private:

		// Insert a new decal into the proper collections
		unsigned int			InsertDecal( SiegeDecal* pDecal, bool bUpdateLighting = true );

		// Collection of currently active decals
		DecalCollection			m_DecalCollection;

		// Mapping of nodes to a list of indices into the collection
		DecalMap				m_DecalMap;

		// Mapping of decal GUID's to decal pointers
		DecalGuidMap			m_DecalGuidMap;
	};
}

#define gSiegeDecalDatabase siege::SiegeDecalDatabase::GetSingleton()

#endif