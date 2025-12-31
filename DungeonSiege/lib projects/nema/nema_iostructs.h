#pragma once
#ifndef NEMA_IOSTRUCTS
#define NEMA_IOSTRUCTS

/*========================================================================

	IOSTRUCTS

	This file contains all the structures needed to read in the data
	I have moved it out of the nema_aspect and nema_prskeys header file in
	hopes of thinning those files out

	
  ========================================================================*/

namespace nema {

	enum ChunkIDs {

		IDMeshHeader			= 0x48534D42,	// BMSH
		IDSubMeshHeader			= 0x42555342,	// BSUB
		IDVertList				= 0x58545642,	// BVTX
		IDVertMap				= 0x504D5642,	// BVMP
		IDBoneHeader			= 0x484E4F42,	// BONH
		IDBoneNameList			= 0x4C4D4E42,	// BNML
		IDBoneVertWeightList	= 0x4C575642,	// BVWL
		IDCornerList			= 0x4E524342,	// BCRN
		IDWCornerList			= 0x4E524357,	// WCRN
		IDTriangleList			= 0x49525442,	// BTRI
		IDSubMeshMaterial		= 0x4D4D5342,	// BSMM
		IDStitchSet				= 'HCTS',		// STCH
		IDBoundingBoxes			= 'XOBB',		// BBOX
		IDMeshEndMarker			= 0x444E4542,	// BEND

		IDRestPose				= 0x534f5052,	// RPOS
	};

	struct sNeMaMesh_Chunk_Version_1_0 {

		DWORD ChunkName;			// "BMSH"	// Boned Mesh
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		
		DWORD StringTableSize;	
		DWORD NumberOfBones;	
		DWORD MaximumMaterials;	
		DWORD MaximumVerts;
		DWORD NumberOfSubMeshes;
		DWORD MeshAttrFlags;
		
	};

	struct sNeMaSubMesh_Chunk {

		DWORD ChunkName;			// "BSUB"	// Sub Mesh variation
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		
		DWORD SubMeshID;	
		DWORD NumberOfSubMeshMaterials;	
		DWORD NumberOfSubMeshVerts;
		DWORD NumberOfSubMeshCorners;
		DWORD NumberOfSubMeshTriangles;
		
	};


	typedef sNeMaMesh_Chunk_Version_1_0 sNeMaMesh_Chunk;

	//** VERTS ***********************************************

	struct sVertList_Chunk {
		DWORD ChunkName;					// "BVTX"
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD NumberOfVerts;
	};

	//** VERT MAPPINGS ***************************************

	typedef DWORD sVertMappingIndex_Chunk;
	typedef DWORD sVertMappingSize_Chunk;

	struct sVertMapData_Chunk {
		DWORD ChunkName;					// "BVMP"
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
	};


	//** BONES ***********************************************

	struct sBoneHeader_Chunk {
		DWORD ChunkName;			// "BONH"	// Bone Header
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
	};

	struct sBoneInfo_Chunk {
		DWORD BoneIndex;		
		DWORD ParentBoneIndex;	
		DWORD NameOffset;
	};

	// Vertex Weights indexed by Bone

	struct sBoneVertData_Chunk {
		DWORD ChunkName;			// "BVWL"	// Bone Vert Weight Data
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
	};

	struct sBoneVertWeightList_Chunk {
		int		NumberOfPairs;
	};

	// Bone Weights indexed by Vertex

	struct sVertBoneData_Chunk {
		DWORD ChunkName;			// "VBWL"	// Vert Bone Weight Data
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		int   NumberOfVerts;		// If NEGATIVE, indicates the primary bone (the one with 100% of the verts)
	};

	struct sVertBoneWeightList_Chunk {
		int		NumberOfPairs;
	};

	//** CORNERS *************************************************

	struct sTexCoord {
		float u;
		float v;
	};

	struct sCorner_Chunk {
		DWORD     i;						// Index into vertex table
		float     nx, ny, nz;				// Normal
		DWORD     color;					// Color
		DWORD     specular;					// Specular color, not currently used
		sTexCoord uv;						// Tex coordinate set
	};

	struct sCornerList_Chunk {
		DWORD ChunkName;					// "BCRN"	// Bone Header
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD NumberOfCorners;
	};


	//** WEIGHTED CORNERS ****************************************

	struct sWeightedCorner_Chunk {
		vector_3 v;							// Position
		float b0, b1, b2, b3;				// Bone weights
		DWORD indices;
		vector_3 n;							// Normal
		DWORD color;						// Color
		sTexCoord uv;						// Tex coordinate set
	};

	struct sWeightedCornerList_Chunk {
		DWORD ChunkName;					// "WCRN"	// Bone Header
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD NumberOfCorners;
	};

	//** TRIANGLES ***********************************************

	struct sTriangleList_Chunk {
		DWORD ChunkName;			// "BTRI"	// Bone Header
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD NumberOfTriangles;
	};

	struct sTriangle_Chunk {
		DWORD CornerAIndex;
		DWORD CornerBIndex;
		DWORD CornerCIndex;
	};

	//** BONE REST POSITIONS ***********************************************

	struct sRestPoseList_Chunk {
		DWORD ChunkName;					// "RPOS"
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD NumberOfBones;
	};

	struct sRestPoseBoneData {
		Quat InvRotation;
		vector_3 InvPosition;
		Quat Rotation;
		vector_3 Position;
	};

	//** MATERIAL SUPPORT **************************************************

	struct sSubMeshMaterial_Chunk {
		DWORD ChunkName;					// "BSMM"
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD NumberOfMaterials;
	};

	struct sSubMeshMaterialData {
		DWORD MaterialID;					
		DWORD NumberOfFaces;					
	};

	//** ANIMATION SUPPORT *************************************************

	enum ControllerChunkIDs {

		IDAnimHeader			= 0x4D494E41,	// ANIM
		IDAnimKeyList			= 0x54534C4B,	// KLST
		IDAnimNotes				= 'NOTE',  
		IDAnimTracer			= 'TRCR',  
		IDAnimEndMarker			= 0x444E4541	// AEND

	};

	struct sNeMaAnim_Chunk_1_0 {

		DWORD ChunkName;					// "ANIM"
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD StringTableSize;
		DWORD NumberOfKeyLists;
		float Duration;
		float DeltaX;
		float DeltaY;
		float DeltaZ;
		DWORD LoopAtEnd;
	};

	struct sNeMaAnim_Chunk_1_1 {

		DWORD ChunkName;					// "ANIM"
		UBYTE MajorVersion;			
		UBYTE MinorVersion;			
		WORD  ExtraVersion;	
		DWORD StringTableSize;
		DWORD NumberOfKeyLists;
		float Duration;
		float LinearDisplacementX;
		float LinearDisplacementY;
		float LinearDisplacementZ;
		float AngularDisplacementX;
		float AngularDisplacementY;
		float AngularDisplacementZ;
		float AngularDisplacementW;
		float HalfAngularDisplacementX;
		float HalfAngularDisplacementY;
		float HalfAngularDisplacementZ;
		float HalfAngularDisplacementW;
		DWORD LoopAtEnd;
	};

	typedef sNeMaAnim_Chunk_1_1 sNeMaAnim_Chunk;


	//** STITCH LISTS *********************************************

	struct sStitchSetHeader_Chunk {
		DWORD	ChunkName;					// "STCH"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	NumberOfSets;
	};

	struct sStitchSet_Chunk {
		DWORD	m_tag;
		DWORD	m_nverts;
	};

	//** BOUNDING BOX LISTS ***************************************

	struct sBBoxList_Chunk {
		DWORD	ChunkName;					// "STCH"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	NumberOfBoxes;
	};

	struct sBBoxData {
		DWORD Tag;
		float OffsetX;
		float OffsetY;
		float OffsetZ;
		float OrientX;
		float OrientY;
		float OrientZ;
		float OrientW;
		float HalfDiagX;
		float HalfDiagY;
		float HalfDiagZ;
	};

	//** KEY LISTS ***********************************************

	struct sAnimKeyList_Chunk_2_x {
		DWORD	ChunkName;					// "KLST"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	BoneIndex;		
		DWORD	BoneNameOffset;
		DWORD	NumberOfKeys;
	};

	struct sAnimKeyList_Chunk_3_0 {
		DWORD	ChunkName;					// "KLST"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	BoneIndex;		
		DWORD	BoneNameOffset;
		DWORD	NumberOfRotKeys;
		DWORD	NumberOfPosKeys;
	};

	typedef sAnimKeyList_Chunk_3_0 sAnimKeyList_Chunk;

	struct sAnimRootKeyList_Chunk_2_x {
		DWORD	ChunkName;					// "RKEY"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	NumberOfKeys;
	};

	struct sAnimRootKeyList_Chunk_3_0 {
		DWORD	ChunkName;					// "RKEY"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	NumberOfRotKeys;
		DWORD	NumberOfPosKeys;
	};

	typedef sAnimRootKeyList_Chunk_3_0 sAnimRootKeyList_Chunk;

	//** KEY DATA (***********************************************

	struct sAnimRotPosKey_Chunk {
		float	Time;
		float	RotX;
		float	RotY;
		float	RotZ;
		float	RotW;
		float	PosX;
		float	PosY;
		float	PosZ;
	};

	struct sAnimRotKey_Chunk {
		float	Time;
		float	RotX;
		float	RotY;
		float	RotZ;
		float	RotW;
	};

	struct sAnimPosKey_Chunk {
		float	Time;
		float	PosX;
		float	PosY;
		float	PosZ;
	};


	//** NOTE LIST *********************************************

	struct sAnimNoteList_Chunk {
		DWORD	ChunkName;					// "NOTE"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	NumberOfNotes;
	};

	//** TRACK EVENT *********************************************
	struct sAnimNote_Chunk {
		float	Time;
		DWORD	FourCC;
	};

	//** TRACER LIST *********************************************

	struct sAnimTracerList_Chunk {
		DWORD	ChunkName;					// "TRCR"
		UBYTE	MajorVersion;			
		UBYTE	MinorVersion;			
		WORD	ExtraVersion;	
		DWORD	NumberOfTracers;
		float	StartTime;
		float	EndTime;
	};

	//** TRACER ELEMENT*******************************************
	struct sAnimTracerKey_Chunk {
		float	PosX;
		float	PosY;
		float	PosZ;
		float	Row0X;
		float	Row0Y;
		float	Row0Z;
		float	Row1X;
		float	Row1Y;
		float	Row1Z;
		float	Row2X;
		float	Row2Y;
		float	Row2Z;
	};

};


#endif	// NEMA_IOSTRUCTS