#pragma once
/**********************************************************************
 *<
	FILE: ASPExp.h

	DESCRIPTION: Dungeon Siege Animation Exporter

	CREATED BY: biddle

	HISTORY: 

	-- Version 4.1 : Ported to gmax/3dsmax 4.2
	-- Version 4.0 : Removed obsolete BCRN section
	== (MeshWriter.ms)
	-- Version 2.5 added convert NECK tools to full STITCH tools
	-- Version 2.4 added tests for dupes and degenerates
	-- Version 2.3 added RunLengths for sub-groups
	-- Version 2.2 added StartIndexes for sub-groups
	-- Version 2.1 added BBoxes
	-- Version 2.0 converted to Y is UP, Z is FACING FORWARD
	-- Version 1.5 added Bone Offsets
	-- Version 1.4 added Vert Stitching
	-- Version 1.3 added WeightedCorners
	-- Version 1.2 added VertBoneWeights
	-- Version 1.1
	-- Version 1.0 added sub mesh support
	-- Version 0.4 added multi texture support
	-- Version 0.3 added selection set support
	-- Version 0.2 added mesh attributes

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/
#ifndef _ASPEXP_H
#define _ASPEXP_H

#define ASP_MAJOR_VERSION 4
#define ASP_MINOR_VERSION 1

#include "ExpBase.h"
#include "VertNormals.h"

class ASPExporter;

//-----------------------------------------------------------------
enum eSubMeshType
{
	SET_BEGIN_ENUM( SUBMESH_, 0 ),
	SUBMESH_BODY,
	SUBMESH_HEAD,
	SUBMESH_HAND,
	SUBMESH_FEET,
	SET_END_ENUM( SUBMESH_ ),
};

const char* ToString   ( eSubMeshType e );
const char* ToPathVar  ( eSubMeshType e );
const char* ToExtension( eSubMeshType e );
bool        FromString ( const char* str, eSubMeshType& e );

//-----------------------------------------------------------------
class TriangleCorner
{
public:

	DWORD		m_VertIndex;
	Point3		m_Normal;
	Point3		m_TexCoord;
	VertColor	m_VertColor;

	TriangleCorner(DWORD vi, const Point3& nm, Point3& tx, VertColor& vc);

	bool operator== (const TriangleCorner& rhs);
};

//-----------------------------------------------------------------
class BuildTriangleFaceCB : public BitArrayCallback 
{

public:
	
	BuildTriangleFaceCB(ASPExporter* aspexp,int sm)
		: m_Exporter(aspexp)
		, m_SubMesh(sm)
		{};
	
	void proc(int n);

private:

	ASPExporter*	m_Exporter;
	int				m_SubMesh;
	
};


//-----------------------------------------------------------------
class ASPExporter : public ExpBase
{
	
	friend BuildTriangleFaceCB;
	
public:

	int m_ExportedCount;

	explicit ASPExporter(const gpstring& maxfname, Interface* iface);
	~ASPExporter();

	bool FetchTextureFiles(std::vector<gpstring>& filenames);

	DSiegeUtils::eExpErrType ExportINode(INode *node, const gpstring& expfname, FileSys::File& outfile);

private:

	TriObject* m_ExportTriObj;


	DWORD						m_RenderFlags;

	std::vector<VNormal>		m_VertNormals;
	
	typedef std::vector<WORD>	TriIndexArray;

	std::vector<TriIndexArray>	m_PartitionedTriIndices[SUBMESH_COUNT];

	typedef std::vector<TriangleCorner> TriCornerArray;
	
	std::vector<TriCornerArray>	m_PartitionedTriCorners[SUBMESH_COUNT];

	std::vector<int>			m_VertLookup[SUBMESH_COUNT];
	BitArray					m_TriSet[SUBMESH_COUNT];

	BitArray					m_DrawLastSet;

	typedef std::map< DWORD,std::vector<int> >	StitchSetMap;
	StitchSetMap					m_StitchSets;

	typedef std::vector< DWORD >		CornerList;
	typedef std::map<DWORD,CornerList>	CornerListMap;

	CornerListMap	m_CornerMapping[SUBMESH_COUNT];		

	struct BoneWeight
	{
		BoneWeight(int b, float w) 
			: m_BoneIndex(b)
			, m_Weight(w)
			{};

		int m_BoneIndex;
		float m_Weight;
	};

	typedef std::list< BoneWeight >	BoneWeightList;

	std::vector< BoneWeightList >	m_VertexBoneWeights;

	struct VertWeight
	{
		VertWeight(int b, float w) 
			: m_VertIndex(b)
			, m_Weight(w)
			{};

		int m_VertIndex;
		float m_Weight;
	};

	typedef std::list< VertWeight >	VertWeightList;

	struct LookupEntry
	{
		int mappedTo;
		int numFaces;
		LookupEntry(int m, int n) 
			: mappedTo(m)
			, numFaces(n) 
			{};
		LookupEntry(const LookupEntry& le) 
			: mappedTo(le.mappedTo)
			, numFaces(le.numFaces) 
			{};
	};

	typedef std::map< int,LookupEntry >	TexLookup;

	struct TextureEntry
	{
		gpstring texName;
		int totFaces;
		TextureEntry(const gpstring& t, int n) 
			: texName(t)
			, totFaces(n) 
			{};
	};

	typedef std::list< TextureEntry > TexListing;

	TexListing	m_TextureList;
	TexLookup	m_TextureLookup[SUBMESH_COUNT];
	MtlID		m_MaxMatID;

	bool		m_HasNegativeParity;

	bool		m_HasSubMeshes;
	bool		m_HasRootBone;
	bool		m_HasVertexAlpha;

	int			m_NumberOfSubMeshes;

	int FetchIntsIntoBitArray(IParamBlock2* pBlock,ParamID pid,BitArray& barray);
	int FetchIntsIntoArray(IParamBlock2* pBlock,ParamID pid,std::vector<int>& array);
	int FetchStringsIntoArray(IParamBlock2* pBlock,ParamID pid,std::vector<gpstring>& array);

	bool VerifyRequiredGlobals(void);
	bool BuildMaterialList(Mtl* mat,int index);
	bool OptimizeMaterialList(void);

	bool BuildTriangleCorners(int submesh);
	bool BuildVertexWeights(void);

	bool BuildVertexWeightsFromPhysique(Modifier* physmod);
	bool BuildVertexWeightsFromSkin(ISkin* skin);

	bool WriteMeshHeaderChunk(FileSys::File& outfile);
	bool WriteBoneInfo(FileSys::File& outfile);
	
	bool WriteTriangleSet(FileSys::File& outfile, int submesh);
	bool WriteSubMeshHeader(FileSys::File& outfile,int submesh);
	bool WriteSubMeshMaterial(FileSys::File& outfile,int submesh);
	bool WriteVertexData(FileSys::File& outfile,int submesh);
	bool WriteWeightedCornerData(FileSys::File& outfile,int submesh);
	bool WriteCornerMapping(FileSys::File& outfile,int submesh);
	DWORD WriteTriangleData(FileSys::File& outfile,int submesh);
	bool WriteBoneWeightList(FileSys::File& outfile,int submesh);
	bool WriteStitchSets(FileSys::File& outfile,int submesh);

	bool WriteRestPose(FileSys::File& outfile);
	bool WriteBoundingBoxes(FileSys::File& outfile);
	bool WriteEndOfMesh(FileSys::File& outfile);

	void Dump(void);
	
};

#endif