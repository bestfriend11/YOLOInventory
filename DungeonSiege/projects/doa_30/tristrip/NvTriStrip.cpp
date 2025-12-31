
#include "NvTriStripObjects.h"
#include "NvTriStrip.h"

////////////////////////////////////////////////////////////////////////////////////////
//private data
static unsigned int cacheSize    = CACHESIZE_GEFORCE1_2;
static bool bStitchStrips        = true;
static unsigned int minStripSize = 0;
static bool bListsOnly           = false;

////////////////////////////////////////////////////////////////////////////////////////
// SetListsOnly()
//
// If set to true, will return an optimized list, with no strips at all.
//
// Default value: false
//
void SetListsOnly(const bool _bListsOnly)
{
	bListsOnly = _bListsOnly;
}

////////////////////////////////////////////////////////////////////////////////////////
// SetCacheSize()
//
// Sets the cache size which the stripfier uses to optimize the data.
// Controls the length of the generated individual strips.
// This is the "actual" cache size, so 24 for GeForce3 and 16 for GeForce1/2
// You may want to play around with this number to tweak performance.
//
// Default value: 16
//
void SetCacheSize(const unsigned int _cacheSize)
{
	cacheSize = _cacheSize;
}


////////////////////////////////////////////////////////////////////////////////////////
// SetStitchStrips()
//
// bool to indicate whether to stitch together strips into one huge strip or not.
// If set to true, you'll get back one huge strip stitched together using degenerate
//  triangles.
// If set to false, you'll get back a large number of separate strips.
//
// Default value: true
//
void SetStitchStrips(const bool _bStitchStrips)
{
	bStitchStrips = _bStitchStrips;
}


////////////////////////////////////////////////////////////////////////////////////////
// SetMinStripSize()
//
// Sets the minimum acceptable size for a strip, in triangles.
// All strips generated which are shorter than this will be thrown into one big, separate list.
//
// Default value: 0
//
void SetMinStripSize(const unsigned int _minStripSize)
{
	minStripSize = _minStripSize;
}

////////////////////////////////////////////////////////////////////////////////////////
// GenerateStrips()
//
// in_indices: input index list, the indices you would use to render
// in_numIndices: number of entries in in_indices
// primGroups: array of optimized/stripified PrimitiveGroups
// numGroups: number of groups returned
//
// Be sure to call delete[] on the returned primGroups to avoid leaking mem
//
//void GenerateStrips(const unsigned short* in_indices, const unsigned int in_numIndices,
//					PrimitiveGroup** primGroups, unsigned short* numGroups)
void GenerateStrips(const unsigned short* in_indices,
					const unsigned int in_numIndices,
					std::vector<PrimitiveGroup>& primGroups)
{
	//put data in format that the stripifier likes
	WordVec tempIndices;
	tempIndices.resize(in_numIndices);
	unsigned short maxIndex = 0;
	for(int i = 0; i < in_numIndices; i++)
	{
		tempIndices[i] = in_indices[i];
		if(in_indices[i] > maxIndex)
			maxIndex = in_indices[i];
	}
	NvStripInfoVec tempStrips;
	NvFaceInfoVec tempFaces;

	NvStripifier stripifier;
	
	//do actual stripification
	stripifier.Stripify(tempIndices, cacheSize, minStripSize, maxIndex, tempStrips, tempFaces);

	//stitch strips together
	IntVec stripIndices;
	unsigned int numSeparateStrips = 0;

	if(bListsOnly)
	{
		//if we're outputting only lists, we're done

		PrimitiveGroup currGroup;

		//count the total number of indices
		unsigned int numIndices = 0;
		for(int i = 0; i < tempStrips.size(); i++)
		{
			numIndices += tempStrips[i]->m_faces.size() * 3;
		}

		currGroup.type       = PT_LIST;

		//do strips
		unsigned int indexCtr = 0;
		for(i = 0; i < tempStrips.size(); i++)
		{
			for(int j = 0; j < tempStrips[i]->m_faces.size(); j++)
			{
				currGroup.indices.push_back(tempStrips[i]->m_faces[j]->m_v0);
				currGroup.indices.push_back(tempStrips[i]->m_faces[j]->m_v1);
				currGroup.indices.push_back(tempStrips[i]->m_faces[j]->m_v2);
			}
		}

		//do lists
		for(i = 0; i < tempFaces.size(); i++)
		{
			currGroup.indices.push_back(tempFaces[i]->m_v0);
			currGroup.indices.push_back(tempFaces[i]->m_v1);
			currGroup.indices.push_back(tempFaces[i]->m_v2);
		}

		primGroups.push_back(currGroup);

	}
	else
	{
		stripifier.CreateStrips(tempStrips, stripIndices, bStitchStrips, numSeparateStrips);

		//if we're stitching strips together, we better get back only one strip from CreateStrips()
		assert( (bStitchStrips && (numSeparateStrips == 1)) || !bStitchStrips);
		
		//convert to output format
		
		//first, the strips
		int startingLoc = 0;
		for(int stripCtr = 0; stripCtr < numSeparateStrips; stripCtr++)
		{

			PrimitiveGroup currGroup;

			int stripLength = 0;
			if(numSeparateStrips != 1)
			{
				//if we've got multiple strips, we need to figure out the correct length
				for(int i = startingLoc; i < stripIndices.size(); i++)
				{
					if(stripIndices[i] == -1)
						break;
				}
				
				stripLength = i - startingLoc;
			}
			else
				stripLength = stripIndices.size();
			
			currGroup.type       = PT_STRIP;
			
			for(int i = startingLoc; i < stripLength + startingLoc; i++)
				currGroup.indices.push_back(stripIndices[i]);
			
			startingLoc += stripLength + 1; //we add 1 to account for the -1 separating strips

			primGroups.push_back(currGroup);
		}
		
		//next, the list
		if(tempFaces.size() != 0)
		{
			PrimitiveGroup currGroup;

			currGroup.type       = PT_LIST;
			for(int i = 0; i < tempFaces.size(); i++)
			{
				currGroup.indices.push_back(tempFaces[i]->m_v0);
				currGroup.indices.push_back(tempFaces[i]->m_v1);
				currGroup.indices.push_back(tempFaces[i]->m_v2);
			}

			primGroups.push_back(currGroup);
		}
	}

	//clean up everything

	//delete strips
	for(i = 0; i < tempStrips.size(); i++)
	{
		for(int j = 0; j < tempStrips[i]->m_faces.size(); j++)
		{
			delete tempStrips[i]->m_faces[j];
			tempStrips[i]->m_faces[j] = NULL;
		}
		delete tempStrips[i];
		tempStrips[i] = NULL;
	}

	//delete faces
	for(i = 0; i < tempFaces.size(); i++)
	{
		delete tempFaces[i];
		tempFaces[i] = NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// RemapIndices()
//
// Function to remap your indices to improve spatial locality in your vertex buffer.
//
// in_primGroups: array of PrimitiveGroups you want remapped
// numGroups: number of entries in in_primGroups
// numVerts: number of vertices in your vertex buffer, also can be thought of as the range
//  of acceptable values for indices in your primitive groups.
// remappedGroups: array of remapped PrimitiveGroups
//
// Note that, according to the remapping handed back to you, you must reorder your 
//  vertex buffer.
//
// Modified 06/27/01 by James Loe
//	- Added ability to return the indexCache to the client.  This makes remapping
//	  your vertex buffer a simple matter of looking up the old index and finding the
//	  new position in the buffer instead of having to go through the newly remapped
//	  indices and essentially do the same work again.

void RemapIndices(const std::vector<PrimitiveGroup>& in_primGroups, 
				  const unsigned short numVerts,
				  std::vector<PrimitiveGroup>& remappedGroups,
				  std::vector<int>& indexCache)
{

	//caches oldIndex --> newIndex conversion
	indexCache.resize(numVerts,-1);
	
	DWORD numGroups = in_primGroups.size();

	//loop over primitive groups
	unsigned int indexCtr = 0;
	for(int i = 0; i < numGroups; i++)
	{

		PrimitiveGroup remapGroup;

		//init remapped group
		remapGroup.type       = in_primGroups[i].type;
		remapGroup.indices.resize(in_primGroups[i].numIndices());

		for(int j = 0; j < in_primGroups[i].numIndices() ; j++)
		{
			int cachedIndex = indexCache[in_primGroups[i].indices[j]];
			if(cachedIndex == -1) //we haven't seen this index before
			{
				//point to "last" vertex in VB
				remapGroup.indices[j] = indexCtr;

				//add to index cache, increment
				indexCache[in_primGroups[i].indices[j]] = indexCtr++;
			}
			else
			{
				//we've seen this index before
				remapGroup.indices[j] = cachedIndex;
			}
		}

		remappedGroups.push_back(remapGroup);
	}
}