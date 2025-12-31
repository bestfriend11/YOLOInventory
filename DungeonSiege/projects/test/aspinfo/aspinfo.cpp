// aspinfo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"

typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char  UBYTE;
typedef struct { float u; float v; } sUV ;
typedef	struct { DWORD V; float	W;} sBoneVertWeightPair ;
typedef struct { float x; float y; float z;} vector_3;
typedef struct { float x; float y; float z; float w;} Quat;
typedef vector_3 sVert;

#include "nema_iostructs.h"
//#include "GpMem.h"
//#include "FileSys.h"

using namespace nema;

int main(int argc, char* argv[])
{

	// Open the file

	char* fname = 0;
	FILE* f = 0;

	bool DUMP_TRIS = true;
	bool DUMP_VERTS = true;
	bool DUMP_CORNERS = true;

	if (argc == 2)
	{
		fname = argv[1];
	}
	else
	{
		printf("\nERROR: Please supply the name of an ASP file to check\n",fname);
	}

	DWORD tricount = 0;
	DWORD corncount = 0;

	try {

	f = fopen(fname,"rb");
	if (!f)
	{
		printf("\nERROR: Could not locate %s\n",fname);
		return 0;
	}

	fseek(f,0,SEEK_END);
	DWORD fs = ftell(f);
	fseek(f,0,SEEK_SET);

	char* data = new char[fs];
	DWORD numread = fread(data,sizeof(char),fs,f);
	if (fs != numread)
	{
		printf("\nERROR: Could not read %s\n",fname);
		return 0;
	}

	const char* runner = data;

	// Parse the mesh
	sNeMaMesh_Chunk* header = (sNeMaMesh_Chunk*)runner;
	runner += sizeof(sNeMaMesh_Chunk);

	DWORD MajorVersion = header->MajorVersion;
	DWORD MinorVersion = header->MinorVersion;

	DWORD vertcount = 0;

	// **************** Read in the STRING data

	runner += header->StringTableSize;

	// **************** Read in the BONE hierarchy info
	const sBoneHeader_Chunk *tmpBoneHeader = (const sBoneHeader_Chunk*)runner;
	runner += sizeof(sBoneHeader_Chunk);

	{for (DWORD i=0; i < header->NumberOfBones;i++) {
		const sBoneInfo_Chunk* d = (const sBoneInfo_Chunk*)runner;
		runner += sizeof(sBoneInfo_Chunk);
	}}

	//*************************************************
	//********** Begin SUB MESH loop ******************
	//*************************************************

	DWORD current_submesh = 0;

	for (current_submesh = 0 ; current_submesh < header->NumberOfSubMeshes; current_submesh++) {

		// **************** Read in the sub texture sets (if there are any)
		const sNeMaSubMesh_Chunk* d = (const sNeMaSubMesh_Chunk*)runner;
		runner += sizeof(sNeMaSubMesh_Chunk);

		const sSubMeshMaterial_Chunk *tmpSubMeshMatChunk = (const sSubMeshMaterial_Chunk*)runner;
		runner += sizeof(sSubMeshMaterial_Chunk);

		for (DWORD i=0; i < tmpSubMeshMatChunk->NumberOfMaterials; i++) {
			const sSubMeshMaterialData *tmpSubMeshMaterialData = (const sSubMeshMaterialData*)runner;
			runner += sizeof(sSubMeshMaterialData);
			tricount += tmpSubMeshMaterialData->NumberOfFaces;
		}

		// **************** Read in the VERTEX data

		const sVertList_Chunk *tmpVertList = (const sVertList_Chunk*)runner;
		runner += sizeof(sVertList_Chunk);

		vertcount += tmpVertList->NumberOfVerts;
		runner += tmpVertList->NumberOfVerts * sizeof(sVert);

		// **************** Read in the CORNER data

		const sCornerList_Chunk *tmpCornList = (const sCornerList_Chunk*)runner;
		runner += sizeof(sCornerList_Chunk);

		corncount += tmpCornList->NumberOfCorners;

		runner += sizeof(sCorner_Chunk) * tmpCornList->NumberOfCorners;

		if ((header->MajorVersion > 1) || (header->MajorVersion == 1 && header->MinorVersion > 2)) {
			const sWeightedCornerList_Chunk *tmpWCornList = (const sWeightedCornerList_Chunk*)runner;
			runner += sizeof(sWeightedCornerList_Chunk);
			runner += tmpWCornList->NumberOfCorners * sizeof(sWeightedCorner_Chunk); 
		}

		const sVertMapData_Chunk *tmpVertMapData = (const sVertMapData_Chunk*)runner;
		runner += sizeof(sVertMapData_Chunk);

		{for (DWORD i=0; i < tmpVertList->NumberOfVerts; i++) {

			const sVertMappingSize_Chunk *tmpSize = (const sVertMappingSize_Chunk*)runner;
			runner += sizeof(sVertMappingSize_Chunk);

			runner += *tmpSize * sizeof(sVertMappingIndex_Chunk);

		}}

		const sTriangleList_Chunk *tmpTriList = (const sTriangleList_Chunk*)runner;
		runner += sizeof(sTriangleList_Chunk);

		if ((header->MajorVersion > 2) || (header->MajorVersion == 2 && header->MinorVersion >= 2))
		{
			runner += sizeof(DWORD) * tmpSubMeshMatChunk->NumberOfMaterials;
		}

		runner += tmpTriList->NumberOfTriangles * sizeof(sTriangle_Chunk);

		const sBoneVertData_Chunk *tmpVertBoneData = (const sBoneVertData_Chunk*)runner;
		runner += sizeof(sBoneVertData_Chunk);

		{for (DWORD i=0; i < header->NumberOfBones;i++) {

			const sBoneVertWeightList_Chunk *tmpBWVL = (const sBoneVertWeightList_Chunk*)runner;
			runner += sizeof(sBoneVertWeightList_Chunk);

			if (tmpBWVL->NumberOfPairs != -1)
			{
				runner += tmpBWVL->NumberOfPairs * sizeof(sBoneVertWeightPair);
			}
		}}

		if (!((header->MajorVersion < 1) || (header->MajorVersion == 1 && header->MinorVersion < 4))) {

			const sStitchSetHeader_Chunk *tmpStitchHeader = (const sStitchSetHeader_Chunk*)runner;
			runner += sizeof(sStitchSetHeader_Chunk);

			for (DWORD s = 0; s<tmpStitchHeader->NumberOfSets; s++) {
				const sStitchSet_Chunk *tmpStitchSet = (const sStitchSet_Chunk*)runner;
				runner += sizeof(sStitchSet_Chunk);
				runner += sizeof(DWORD)*tmpStitchSet->m_nverts;
			}

		}

	}

	//*************************************************
	//********** End of SUB MESH loop *****************
	//*************************************************

	if (DUMP_TRIS)
	{
		printf("TRIANGLES: %d",tricount);
	}
	if (DUMP_VERTS)
	{
		printf(" VERTICES: %d",vertcount);
	}
	if (DUMP_CORNERS)
	{
		printf(" CORNERS: %d\n",corncount);
	}

	delete [] data;

	}

	catch(...)
	{
		printf("INTERNAL ERROR: Not an ASP file??\n");
	}

	if (!f)
	{
		fclose(f);
	}

	return tricount;
}
