/**********************************************************************
 *<
	FILE: ASPExp.cpp

	DESCRIPTION: Dungeon Siege Animation Exporter

	CREATED BY: biddle

	HISTORY: 

	-- Version 4.0 : Ported to gmax/3dsmax 4.2
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

#include <gpcore.h>
#include <gpmath.h>
#include <algorithm>
#include "ASPExp.h"
#include "MAXhelpers.h"
#include "MAXscrpt.h"
//#include <CustAttrib.h>
//#include <ICustAttribContainer.h>
#include <config.h>
#include <vector_3.h>
#include <notetrck.h>
#include "DSmxs.h"
#include <matrix_3x3.h>
#include <decomp.h>
#include "vertnormals.h"
#include "tristrip\NvTriStrip.h"
#include "phyexp.h"

#include <nema_iostructs.h>

#define UVMAXREZ 1024.0f

//////////////////////////////////////////////////////////////////////////////
// enum eSubMeshType implementation

static const char* s_SubMeshTypeStrings[] =
{
	"SUBMESH_BODY",
	"SUBMESH_HEAD",
	"SUBMESH_HAND",
	"SUBMESH_FEET"
};

COMPILER_ASSERT( ELEMENT_COUNT( s_SubMeshTypeStrings ) == SUBMESH_COUNT );

static stringtool::EnumStringConverter eSubMeshTypeConverter( s_SubMeshTypeStrings, SUBMESH_BEGIN, SUBMESH_END );

const char* ToString( eSubMeshType e )
	{  return ( eSubMeshTypeConverter.ToString( e ) );  }

bool FromString( const char* str, eSubMeshType& e )
	{  return ( eSubMeshTypeConverter.FromString( str, rcast <int&> ( e ) ) );  }

//-----------------------------------------------------------------
//-----------------------------------------------------------------
TriangleCorner::TriangleCorner(DWORD vi, const Point3& nm, Point3& tx, VertColor& vc)
{
	m_VertIndex = vi;
	m_TexCoord = tx;
	m_Normal = nm.Normalize();	// Make sure it's a unit normal
	m_VertColor = vc;
}

//-----------------------------------------------------------------
bool TriangleCorner::operator== (const TriangleCorner& rhs)
{
	return  (m_VertIndex == rhs.m_VertIndex)
		&& IsEqual(m_Normal,rhs.m_Normal,0.001f)
		&& IsEqual(m_TexCoord ,rhs.m_TexCoord, 1/UVMAXREZ)
		&& IsEqual(m_VertColor,rhs.m_VertColor,1/256.0f);
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------
ASPExporter::ASPExporter(const gpstring& maxfname, Interface* iface)
	: ExpBase(maxfname,iface)
{
}

ASPExporter::~ASPExporter()
{
}

//-----------------------------------------------------------------
int ASPExporter::FetchIntsIntoBitArray(IParamBlock2* pBlock,ParamID pid,BitArray& barray)
{
	int count = 0;
	if (pBlock)
	{
		count = pBlock->Count(pid);
		for ( int i = 0; i < count; ++i)
		{
			int j = pBlock->GetInt(pid,TimeValue(0),i)-1;
			if ( j < barray.GetSize() )
			{
				barray.Set(j);
			}
		}
	}
	return count;
}

//-----------------------------------------------------------------
int ASPExporter::FetchIntsIntoArray(IParamBlock2* pBlock,ParamID pid,std::vector<int>& array)
{
	array.clear();
	int count = 0;
	if (pBlock)
	{
		count = pBlock->Count(pid);
		for ( int i = 0; i < count; ++i)
		{
			int j = pBlock->GetInt(pid,TimeValue(0),i);
			array.push_back(j);
		}
	}
	return count;
}

//-----------------------------------------------------------------
int ASPExporter::FetchStringsIntoArray(IParamBlock2* pBlock,ParamID pid,std::vector<gpstring>& array)
{
	array.clear();
	int count = 0;
	if (pBlock)
	{
		count = pBlock->Count(pid);
		for ( int i = 0; i < count; ++i)
		{
			gpstring j = pBlock->GetStr(pid,TimeValue(0),i);
			array.push_back(j);
		}
	}
	return count;
}


//-----------------------------------------------------------------
bool ASPExporter::VerifyRequiredGlobals(void)
{

	int numfaces = m_ExportTriObj->GetMesh().getNumFaces();

	for2 (int i = 0; i != SUBMESH_COUNT; i++)
	{
		m_TriSet[i].SetSize(numfaces);
	}

	// Keep a bit array of the faces we want to 'draw last'
	m_DrawLastSet.SetSize(numfaces);
	m_DrawLastSet.ClearAll();

	MSPlugin* aspmod = FetchSiegeMaxInterface(m_ExportNode,"ASPMODDEF");

	if(!aspmod)
	{
		for (int i = 1; i != SUBMESH_COUNT; i++)
		{
			m_TriSet[i].ClearAll();
		}
	}
	else
	{
		// Locate all the ASP parameters in the custom attributes

		IParamBlock2* aspPBlock = 0;

		std::vector<int> StitchVerts;
		std::vector<gpstring> StitchTags;

		for (int pbi = 0; aspmod && (pbi < aspmod->pblocks.Count()); pbi++)
		{

			aspPBlock = aspmod->pblocks[pbi];

			if (aspPBlock && !stricmp(aspPBlock->GetLocalName(),"ASPDATA"))
			{

				// Locate all of the parameter IDs 
				for (int idx = 0; idx < aspPBlock->NumParams() ; idx++ )
				{
					ParamID pid = aspPBlock->IndextoID(idx);
					ParamType2 ptype = aspPBlock->GetParameterType(pid);
					TSTR pname = aspPBlock->GetLocalName(idx);

					if (pname)
					{
						if (!stricmp(pname,"RenderFlags") && (ptype == TYPE_INT))
						{
							m_RenderFlags = (DWORD)aspPBlock->GetInt(pid,TimeValue(0));
						}
						else if (!stricmp(pname,"HeadFaceList") && (ptype == TYPE_INT_TAB))
						{
							FetchIntsIntoBitArray(aspPBlock,pid,m_TriSet[SUBMESH_HEAD]);
						}
						else if (!stricmp(pname,"HandFaceList") && (ptype == TYPE_INT_TAB))
						{
							FetchIntsIntoBitArray(aspPBlock,pid,m_TriSet[SUBMESH_HAND]);
						}
						else if (!stricmp(pname,"FeetFaceList") && (ptype == TYPE_INT_TAB))
						{
							FetchIntsIntoBitArray(aspPBlock,pid,m_TriSet[SUBMESH_FEET]);
						}
						else if (!stricmp(pname,"StitchVertList") && (ptype == TYPE_INT_TAB))
						{
							FetchIntsIntoArray(aspPBlock,pid,StitchVerts);
						}
						else if (!stricmp(pname,"StitchTagList") && (ptype == TYPE_STRING_TAB))
						{
							FetchStringsIntoArray(aspPBlock,pid,StitchTags);
						}
					}
				}
			}
		}

		if (!StitchTags.empty())
		{
			
			DWORD currvert = 0;
			DWORD nextvert = 0;

			for (DWORD t = 0; t!= StitchTags.size(); ++t)
			{
				DWORD tag= FourCCToDWORD(StitchTags[t].c_str(),false);
				if ((currvert+1) <= StitchVerts.size())
				{
					DWORD sz = StitchVerts[currvert];
					currvert++;
					nextvert = currvert+sz;
					if ((nextvert) <= StitchVerts.size())
					{
						std::vector<int>::iterator beg = StitchVerts.begin() + currvert;
						std::vector<int>::iterator end = StitchVerts.begin() + nextvert;
						std::vector<int> templist;
						for (std::vector<int>::iterator it = beg; it!=end; ++it)
						{
							templist.push_back((*it)-1);
						}
						m_StitchSets[tag] = templist;
					}
				}
				else
				{
					return false;
				}

				currvert = nextvert;
			}
		}


	}

	BitArray allothers;
	allothers.SetSize(numfaces);
	allothers.ClearAll();

	for2 (int i = 1; i != SUBMESH_COUNT; i++)
	{
		allothers |= m_TriSet[i];
	}

	m_TriSet[0].SetAll();
	m_TriSet[0] &= ~allothers;

	m_HasSubMeshes = !allothers.IsEmpty();
	m_NumberOfSubMeshes = m_HasSubMeshes ? SUBMESH_COUNT : 1;

	m_HasVertexAlpha = (m_RenderFlags & (1<<4)) != 0 ;
	
	return true;
}


//-----------------------------------------------------------------
bool ASPExporter::BuildMaterialList(Mtl* mat,int index)
{

	bool ret = true;

	if (!mat || mat->ClassID() == Class_ID(DMTL_CLASS_ID,0))
	{
		
		if (!m_MaxMatID)
		{
			m_MaxMatID = 1;
		}
		
		gpstring diffusemapname;
		
		Texmap* diffusemap = mat ? mat->GetSubTexmap(ID_DI) : NULL;
		
		if ( diffusemap )
		{
			if ( diffusemap->ClassID() == Class_ID(BMTEX_CLASS_ID,0) )
			{
				Bitmap* bm = ((BitmapTex *)diffusemap)->GetBitmap(0);
				
				if (bm && bm->Storage())
				{
					diffusemapname = bm->Storage()->bi.Name();
					diffusemapname.to_lower();
				}
				else
				{
					gpstring buff;
					buff.assignf("Found material [%s] with a missing diffuse texture data, using placeholder texture",mat->GetName());
					DWORD icon = MB_OK | MB_ICONEXCLAMATION;
					MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"Exporter Error",icon );
					diffusemapname = "b_i_glb_placeholder.psd";
				}
			}
			else
			{
				diffusemapname.clear();
			}
		} 
		else
		{
			gpstring buff;
			if (mat)
			{
				buff.assignf("Found material [%s] without a diffuse texture, using placeholder texture",mat->GetName());
			}
			else
			{
				buff.assign("There is no material applied to the object, using placeholder texture");
			}
			DWORD icon = MB_OK | MB_ICONEXCLAMATION;
			MessageBox(GetCOREInterface()->GetMAXHWnd(),buff.c_str(),"Exporter Error",icon );
			diffusemapname = "b_i_glb_placeholder.psd";
		}

		if ( !diffusemapname.empty() )
		{
			TexListing::const_iterator beg = m_TextureList.begin();
			TexListing::const_iterator end = m_TextureList.end();
			TexListing::const_iterator it;

			for (it = beg; it != end; ++it)
			{
				if ( same_no_case((*it).texName,diffusemapname) )
				{
					break;
				}
			}

			int mapped_offset = (it==end) ? m_TextureList.size() : std::distance(beg,it);

			for (int submesh = 0 ; submesh != m_NumberOfSubMeshes; submesh++)
			{
				m_TextureLookup[submesh].insert( std::make_pair( index,LookupEntry(mapped_offset,0) ) );
			}

			if ( it == end )
			{
				m_TextureList.push_back(TextureEntry(diffusemapname,0));
			}
		}
	}
	else if (mat->ClassID() == Class_ID(MULTI_CLASS_ID,0))
	{
		// Multi-Sub material!
		m_MaxMatID =  mat->NumSubMtls();

		for2 (MtlID i = 0; ret && (i != m_MaxMatID) ; ++i)
		{
			ret = BuildMaterialList(  mat->GetSubMtl(i), index+i );
		}
	}
	return ret;
}

//-----------------------------------------------------------------
bool ASPExporter::OptimizeMaterialList()
{
	TexLookup matidlookup;

	Mesh& expmesh = m_ExportTriObj->GetMesh();

	int submesh;

	for2 (submesh = 0 ; submesh != m_NumberOfSubMeshes; submesh++)
	{
		for (int f = 0; f < expmesh.numFaces; ++f)
		{
			if (!m_TriSet[submesh][f])
			{
				continue;
			}
			
			MtlID facematid = expmesh.faces[f].getMatID();

			if ( facematid >= m_MaxMatID ) 
			{
				facematid = facematid % m_MaxMatID;
			}

			int actmatid;
			TexLookup::iterator texlookit = m_TextureLookup[submesh].find(facematid);
			
			if ( texlookit == m_TextureLookup[submesh].end() )
			{
				// The face is mapped with an unknown id, we'll need to mod the value
				gpwarningf(("Found unknown material id [%d] on face [%d] of submesh [%d]",facematid,f,submesh));
				actmatid = 0;
			}
			else
			{
				(*texlookit).second.numFaces++;
			}

		}	

		for (TexLookup::iterator it = m_TextureLookup[submesh].begin(); it != m_TextureLookup[submesh].end(); ++it)
		{
			// Total all the faces that map to each texture
			TexListing::iterator tl = m_TextureList.begin();
			std::advance(tl,(*it).second.mappedTo);
			(*tl).totFaces += (*it).second.numFaces;
		}
	}

	int mapped_offset = 0;
	for2 (TexListing::iterator it = m_TextureList.begin(); it != m_TextureList.end(); )
	{
		// Remove all the textures that have no faces mapped to them
		if ((*it).totFaces == 0)
		{
			for (submesh = 0 ; submesh != m_NumberOfSubMeshes; submesh++)
			{
				for (TexLookup::iterator tlit = m_TextureLookup[submesh].begin(); tlit != m_TextureLookup[submesh].end(); ++tlit)
				{
					if ((*tlit).second.mappedTo == mapped_offset)
					{
						(*tlit).second.mappedTo = -1;
					}
					else if ((*tlit).second.mappedTo > mapped_offset)
					{
						(*tlit).second.mappedTo--;
					}
				}
			}
			// Remove the texture from the list
			it = m_TextureList.erase(it);
		}
		else
		{ 
			mapped_offset++;
			++it;
		}
	}

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::FetchTextureFiles(std::vector<gpstring>& tn)
{
	tn.clear();
	for2 (TexListing::iterator it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
	{	
		tn.push_back((*it).texName);
	}
	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteMeshHeaderChunk(FileSys::File& outfile)
{	
	nema::sNeMaMesh_Chunk chunk;

	chunk.ChunkName					= REVERSE_FOURCC('BMSH');
	chunk.MajorVersion				= ASP_MAJOR_VERSION;			
	chunk.MinorVersion				= ASP_MINOR_VERSION;			
	chunk.ExtraVersion				= 0;	

	chunk.StringTableSize			= m_StringTableSize;

	chunk.NumberOfBones				= m_Bones.size();
	chunk.MaximumMaterials			= m_TextureList.size();
	chunk.MaximumVerts				= m_ExportTriObj->GetMesh().getNumVerts();
	chunk.NumberOfSubMeshes			= m_NumberOfSubMeshes;
	chunk.MeshAttrFlags				= m_RenderFlags;

	outfile.WriteStruct( chunk );

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteBoneInfo(FileSys::File& outfile)
{
	nema::sBoneHeader_Chunk chunk;

	chunk.ChunkName					= REVERSE_FOURCC('BONH');
	chunk.MajorVersion				= ASP_MAJOR_VERSION;			
	chunk.MinorVersion				= ASP_MINOR_VERSION;			
	chunk.ExtraVersion				= 0;

	outfile.WriteStruct( chunk );

	for2 (int i = 0; i < m_Bones.size(); ++i)
	{
		nema::sBoneInfo_Chunk ichunk;
		ichunk.BoneIndex		= i;		
		ichunk.ParentBoneIndex	= m_Bones[i].m_ParentIndex;	
		ichunk.NameOffset		= m_StringTablePos[i+m_TextureList.size()];
		outfile.WriteStruct( ichunk );
	}

	return true;
}

//-----------------------------------------------------------------
void BuildTriangleFaceCB::proc(int n) 
{

	Mesh& msh = m_Exporter->m_ExportTriObj->GetMesh();

	std::vector<int>& vlook = m_Exporter->m_VertLookup[m_SubMesh];
	std::vector<int>::iterator vit;

	Face& f = msh.faces[n];

	// Determine which partition this face falls into
	// We first partition everything based on the 'draw last' setting, then by material

	MtlID facematid = f.getMatID();

	if ( facematid >= m_Exporter->m_MaxMatID ) 
	{
		facematid = facematid % m_Exporter->m_MaxMatID;
	}

	ASPExporter::TexLookup::iterator it = m_Exporter->m_TextureLookup[m_SubMesh].find(facematid);

	if (it==m_Exporter->m_TextureLookup[m_SubMesh].end())
	{
		gpfatal(
			"it==m_Exporter->m_TextureLookup[m_SubMesh].end()\n"
			);
		return;
	}

	int partition = 2 * (*it).second.mappedTo;

	if (m_Exporter->m_DrawLastSet[n])
	{
		partition += 1;
	}

	if (m_Exporter->m_PartitionedTriCorners[m_SubMesh].size() < partition)
	{
		gpfatalf(
			("m_Exporter->m_PartitionedTriCorners[m_SubMesh].size() < partition\n"
			"(%d < %d)\n",
			m_Exporter->m_PartitionedTriCorners[m_SubMesh].size(),
			partition)
			);
		return;
	}

	DWORD vertA = f.getVert(0);
	DWORD vertB = f.getVert(1);
	DWORD vertC = f.getVert(2);
	
	//dbgwart gpgenericf(("Face %d: <%d,%d,%d>\n",n,vertA,vertB,vertC));

	DWORD viA,viB,viC;
	
	vit = std::find(vlook.begin(),vlook.end(),vertA);
	if (vit == vlook.end())
	{
		viA = vlook.size();
		vlook.push_back(vertA);
	}
	else
	{
		viA = vit-vlook.begin();
	}

	vit = std::find(vlook.begin(),vlook.end(),vertB);
	if (vit == vlook.end())
	{
		viB = vlook.size();
		vlook.push_back(vertB);
	}
	else
	{
		viB = vit-vlook.begin();
	}

	vit = std::find(vlook.begin(),vlook.end(),vertC);
	if (vit == vlook.end())
	{
		viC = vlook.size();
		vlook.push_back(vertC);
	}
	else
	{
		viC = vit-vlook.begin();
	}

	//dbgwart gpgenericf(("  indices <%d,%d,%d>\n",avi,bvi,cvi));

	DWORD smth = f.getSmGroup();

	Point3 nA,nB,nC;

	std::vector<VNormal>& vnorm = m_Exporter->m_VertNormals;

	nA = (vertA) < vnorm.size() ? vnorm[vertA].GetNormal(smth) : Point3(1,0,0);
	nB = (vertB) < vnorm.size() ? vnorm[vertB].GetNormal(smth) : Point3(1,0,0);
	nC = (vertC) < vnorm.size() ? vnorm[vertC].GetNormal(smth) : Point3(1,0,0);

	//dbgwart gpgenericf(("  NormA [%f,%f,%f]\n",an.x,an.y,an.z));
	//dbgwart gpgenericf(("  NormB [%f,%f,%f]\n",bn.x,bn.y,bn.z));
	//dbgwart gpgenericf(("  NormC [%f,%f,%f]\n",cn.x,cn.y,cn.z));

	UVVert uvA,uvB,uvC;
	bool uvok = false;

	if (msh.numTVerts>0)
	{
		TVFace& tvface = msh.tvFace[n];

		if (   (tvface.getTVert(0) < msh.numTVerts) 
			&& (tvface.getTVert(1) < msh.numTVerts)
			&& (tvface.getTVert(2) < msh.numTVerts)
			)
		{
			uvA = msh.tVerts[tvface.getTVert(0)];
			uvB = msh.tVerts[tvface.getTVert(1)];
			uvC	= msh.tVerts[tvface.getTVert(2)];
			uvok = true;
		}
	}

	if (!uvok)
	{
		// This object is missing a UVW map! 
		// ...map the UV to the XY
		uvA = msh.getVert(vertA)/5000.0f; uvA.z = 0.0f;
		uvB = msh.getVert(vertB)/5000.0f; uvA.z = 0.0f;
		uvC = msh.getVert(vertC)/5000.0f; uvA.z = 0.0f;
	}

	Point3 texA,texB,texC;

	texA.x = RoundOff(uvA.x,1.0f/UVMAXREZ);
	texA.y = RoundOff(uvA.y,1.0f/UVMAXREZ);
	texA.z = RoundOff(uvA.z,1.0f/UVMAXREZ);

	texB.x = RoundOff(uvB.x,1.0f/UVMAXREZ);
	texB.y = RoundOff(uvB.y,1.0f/UVMAXREZ);
	texB.z = RoundOff(uvB.z,1.0f/UVMAXREZ);

	texC.x = RoundOff(uvC.x,1.0f/UVMAXREZ);
	texC.y = RoundOff(uvC.y,1.0f/UVMAXREZ);
	texC.z = RoundOff(uvC.z,1.0f/UVMAXREZ);

/*	texA.x = floor((uvA.x*UVMAXREZ) + 0.5f)/UVMAXREZ;
	texA.y = floor((uvA.y*UVMAXREZ) + 0.5f)/UVMAXREZ;
	texB.x = floor((uvB.x*UVMAXREZ) + 0.5f)/UVMAXREZ;
	texB.y = floor((uvB.y*UVMAXREZ) + 0.5f)/UVMAXREZ;
	texC.x = floor((uvC.x*UVMAXREZ) + 0.5f)/UVMAXREZ;
	texC.y = floor((uvC.y*UVMAXREZ) + 0.5f)/UVMAXREZ;
*/
	//dbgwart gpgenericf(("  texA [%f,%f]\n",texA.x,texA.y));
	//dbgwart gpgenericf(("  texB [%f,%f]\n",texB.x,texB.y));
	//dbgwart gpgenericf(("  texC [%f,%f]\n",texC.x,texC.y));

	VertColor vcA,vcB,vcC;

	bool vcok = false;

	if ( msh.numCVerts>0 )
	{
		TVFace& tvface = msh.vcFace[n];
		if (   (tvface.getTVert(0) < msh.getNumVertCol()) 
			&& (tvface.getTVert(1) < msh.getNumVertCol())
			&& (tvface.getTVert(2) < msh.getNumVertCol())
			)
		{
			vcA = msh.vertCol[tvface.getTVert(0)];
			vcB = msh.vertCol[tvface.getTVert(1)];
			vcC = msh.vertCol[tvface.getTVert(2)];
			uvok = true;
		}
	}

	if (!vcok)
	{
		vcA = vcB = vcC = VertColor(1,1,1);
	}

	//dbgwart gpgenericf(("  vcA [%f,%f,%f]\n",vcA.x,vcA.y,vcA.z));
	//dbgwart gpgenericf(("  vcB [%f,%f,%f]\n",vcB.x,vcB.y,vcB.z));
	//dbgwart gpgenericf(("  vcC [%f,%f,%f]\n",vcC.x,vcC.y,vcC.z));
	 
	TriangleCorner tA(viA,nA,texA,vcA);
	TriangleCorner tB(viB,nB,texB,vcB);
	TriangleCorner tC(viC,nC,texC,vcC);

	WORD triA,triB,triC;
	
	std::vector<TriangleCorner>& tcorns = m_Exporter->m_PartitionedTriCorners[m_SubMesh][partition];
	std::vector<TriangleCorner>::iterator trit;

	trit = std::find(tcorns.begin(),tcorns.end(),tA);
	if (trit == tcorns.end())
	{
		triA = tcorns.size();
		tcorns.push_back(tA);
		//gpgeneric("A missed\n");
	}
	else
	{
		triA = trit-tcorns.begin();
		//gpgeneric("A hit\n");
	}

	trit = std::find(tcorns.begin(),tcorns.end(),tB);
	if (trit == tcorns.end())
	{
		triB = tcorns.size();
		tcorns.push_back(tB);
		//gpgeneric("B missed\n");
	}
	else
	{
		triB = trit-tcorns.begin();
		//gpgeneric("B hit\n");
	}

	trit = std::find(tcorns.begin(),tcorns.end(),tC);
	if (trit == tcorns.end())
	{
		triC = tcorns.size();
		tcorns.push_back(tC);
		//gpgeneric("C missed\n");
	}
	else
	{
		triC = trit-tcorns.begin();
		//gpgeneric("C hit\n");	
	}

	// Fetch the correct list of triangle indices for this material

	ASPExporter::TriIndexArray& tris = m_Exporter->m_PartitionedTriIndices[m_SubMesh][partition];
	tris.push_back(triA);
	tris.push_back(triB);
	tris.push_back(triC);

	//dbgwart gpgenericf(("  tri [%d,%d,%d]\n",triA,triB,triC));

}

//-----------------------------------------------------------------
bool ASPExporter::BuildTriangleCorners(int submesh)
{

	m_PartitionedTriIndices[submesh].clear();
	m_PartitionedTriCorners[submesh].clear();

	DWORD maxpartitions = m_TextureList.size()*2;

	for2 (int t = 0 ; t != maxpartitions ; ++t)
	{
		m_PartitionedTriIndices[submesh].push_back(TriIndexArray());
		m_PartitionedTriCorners[submesh].push_back(TriCornerArray());
	}

	m_VertLookup[submesh].clear();

	BuildTriangleFaceCB cb(this,submesh);

	m_TriSet[submesh].EnumSet(cb);

	// Optimize the tri strips

	// Prepare to stripify the geometry
	SetCacheSize( 16 );
	SetStitchStrips( false );
	SetMinStripSize( 0 );
	SetListsOnly( true );

	DWORD partition_start_index = 0; 
	
	for (DWORD p = 0; p!= maxpartitions; ++p)
	{
		// Make a copy of the corners for remapping
		TriCornerArray tempCorners = m_PartitionedTriCorners[submesh][p];

		if (m_PartitionedTriCorners[submesh][p].empty())
		{
			continue;
		}

		// in_indices: input index list, the indices you would use to render
		// in_numIndices: number of entries in in_indices
		// primGroups: array of optimized/stripified PrimitiveGroups
		// numGroups: number of groups returned

		// Generate strip information for this stage, numGroups will always be 1
		PrimitiveGroup* pGroup			= NULL;
		WORD numGroups					= 0;

		GenerateStrips( m_PartitionedTriIndices[submesh][p].begin(), m_PartitionedTriIndices[submesh][p].size(), &pGroup, &numGroups );
		gpassert(numGroups == 1);

		// Remap indices
		PrimitiveGroup* pRemappedGroups	= NULL;
		int* pIndexCache				= NULL;

		RemapIndices( pGroup, numGroups, m_PartitionedTriCorners[submesh][p].size(), &pRemappedGroups, pIndexCache );

		// Remap this section of vertices
		for2 ( DWORD i = 0; i < tempCorners.size(); ++i )
		{
			// Copy the vertex into it's new happy home
			DWORD ti = pIndexCache[i];

			m_PartitionedTriCorners[submesh][p][ti] = tempCorners[i];

			DWORD vi = tempCorners[i].m_VertIndex ;

			CornerListMap::iterator vmit = m_CornerMapping[submesh].find(vi);
			if (vmit == m_CornerMapping[submesh].end())
			{
				CornerList newbie;
				newbie.push_back(ti + partition_start_index);
				m_CornerMapping[submesh].insert(std::make_pair(vi,newbie));
			}
			else
			{
				(*vmit).second.push_back(ti + partition_start_index);
			}
		}

		// Copy the remapped indices
		memcpy( m_PartitionedTriIndices[submesh][p].begin(), pRemappedGroups[0].indices, sizeof( WORD ) * m_PartitionedTriIndices[submesh][p].size() );

		// Cleanup
		delete[] pIndexCache;
		delete[] pGroup;
		delete[] pRemappedGroups;

		partition_start_index += m_PartitionedTriCorners[submesh][p].size();
		
	}

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::BuildVertexWeights(void)
{
	m_VertexBoneWeights.clear();

	ISkin* skin = FetchSkinInterface(m_ExportNode);
	if ( skin )
	{
		BuildVertexWeightsFromSkin(skin);
	}
	
	Modifier* physmod = FetchPhysiqueModifier(m_ExportNode);
	if ( physmod )
	{
		BuildVertexWeightsFromPhysique(physmod);
	}
	
	if ( !m_VertexBoneWeights.empty() )
	{
		for2 (int v=0; v!=m_VertexBoneWeights.size(); ++v)
		{
			// Make sure we only allow a maximum of four bones per vertex.
			if (m_VertexBoneWeights[v].size() > 4)
			{
				BoneWeightList::iterator chop =  m_VertexBoneWeights[v].begin();
				std::advance(chop,4);
				m_VertexBoneWeights[v].erase(chop,m_VertexBoneWeights[v].end());
			}

			// Normalize all the weights
			BoneWeightList::iterator beg = m_VertexBoneWeights[v].begin();
			BoneWeightList::iterator end = m_VertexBoneWeights[v].end();
			BoneWeightList::iterator it;

			float sum = 0;
			for ( it=beg; it!=end; ++it)
			{
				sum += (*it).m_Weight;
			}
			for ( it=beg; it!=end; ++it)
			{
				(*it).m_Weight = (sum<0.001f) ? 0 : (*it).m_Weight/sum ;
			}
		}
		return true;
	}


	return false;
}

//-----------------------------------------------------------------
bool ASPExporter::BuildVertexWeightsFromPhysique(Modifier* physmod)
{
	if (physmod == NULL)
	{
		return false;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)physmod->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(m_ExportNode) ) 
	{
		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		for (int vert = 0; vert < numverts; vert++)
		{
			BoneWeightList newbies;

			if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) )
			{
				int vtype = phyVertExp->GetVertexType();

				if (vtype & BLENDED_TYPE)
				{
					IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

					for (int b = 0; b < phyBlendVert->GetNumberNodes(); b++)
					{
						INode* bone = phyBlendVert->GetNode(b);

						for (int i = 0; i != m_Bones.size(); ++i)
						{
							if (m_Bones[i].m_INode == bone)
							{

								float weight = RoundOff(phyBlendVert->GetWeight(b),0.001f);
								
								BoneWeightList::iterator it = newbies.begin();
								for (; it != newbies.end(); ++it)
								{
									if ((*it).m_Weight<= weight)
									{
										break;
									}
								}
								newbies.insert(it,BoneWeight(i,weight));
							}
						}
					}

				}
				else
				{
					// There is only one bone attachede to this vert

					IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;

					INode* bone = phyRigidVert->GetNode();

					for (int i = 0; i != m_Bones.size(); ++i)
					{
						if (m_Bones[i].m_INode == bone)
						{
							newbies.insert(newbies.begin(),BoneWeight(i,1));
							break;
						}
					}
				}

				phyContextExp->ReleaseVertexInterface(phyVertExp);

			} 
			else
			{
				// This deserves an ERROR!
				phyExp->ReleaseContextInterface(phyContextExp);
				m_VertexBoneWeights.clear();
			}

			m_VertexBoneWeights.push_back(newbies);
		}

		phyExp->ReleaseContextInterface(phyContextExp);

	}

	return false;
}

//-----------------------------------------------------------------
bool ASPExporter::BuildVertexWeightsFromSkin(ISkin* skin)
{
	//-- For each vertex we need to find the weights of the bones that influences it
	//-- We create two lists, one indexed by bone with vert/weight pairs, the other indexed by vert with bone/weight pairs

	if (skin == NULL)
	{
		return false;
	}

	ISkinContextData* skincontext = skin->GetContextInterface(m_ExportNode);

	Mesh& mesh = m_ExportTriObj->GetMesh();

	for2 (int vi = 0; vi != mesh.getNumVerts(); vi++)
	{
		BoneWeightList newbies;
		int numbones = skincontext->GetNumAssignedBones(vi);
		for (int lbi = 0; lbi != numbones; lbi++)
		{
			INode* bone = skin->GetBone(skincontext->GetAssignedBone(vi,lbi));
			if (IsValidBone(bone))
			{
				for (int b = 0; b != m_Bones.size(); ++b)
				{
					if (m_Bones[b].m_INode == bone)
					{
						float weight = RoundOff(skincontext->GetBoneWeight(vi,lbi),0.001f);
						for (BoneWeightList::iterator it = newbies.begin(); it != newbies.end(); ++it)
						{
							if ((*it).m_Weight<= weight)
							{
								break;
							}
						}
						newbies.insert(it,BoneWeight(b,weight));
					}
				}
			}
		}
		m_VertexBoneWeights.push_back(newbies);
	}

	return true;

}

//-----------------------------------------------------------------
bool ASPExporter::WriteSubMeshHeader(FileSys::File& outfile,int submesh)
{
	nema::sNeMaSubMesh_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('BSUB');
	chunk.MajorVersion = ASP_MAJOR_VERSION;			
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	
		
	chunk.SubMeshID = submesh;
	
	chunk.NumberOfSubMeshVerts = m_VertLookup[submesh].size();

	chunk.NumberOfSubMeshMaterials = 0;	
	chunk.NumberOfSubMeshCorners = 0;
	chunk.NumberOfSubMeshTriangles = 0;

	// Tally up all the tris & corners in each of the partitions
	DWORD numparts = 2*m_TextureList.size();
	for2 (int p = 0 ; p != numparts ; p+=2)
	{
		if (!m_PartitionedTriIndices[submesh][p].empty() || !m_PartitionedTriIndices[submesh][p+1].empty())
		{
			// Even numbered partitions store
			chunk.NumberOfSubMeshMaterials++;
		}
		
		chunk.NumberOfSubMeshCorners += m_PartitionedTriCorners[submesh][p].size();
		chunk.NumberOfSubMeshTriangles += m_PartitionedTriIndices[submesh][p].size()/3;

		// Remember to account for any 'draw last' partitions (that follow the regulars)
		chunk.NumberOfSubMeshCorners += m_PartitionedTriCorners[submesh][p+1].size();
		chunk.NumberOfSubMeshTriangles += m_PartitionedTriIndices[submesh][p+1].size()/3;
	}

	outfile.WriteStruct(chunk);

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteSubMeshMaterial(FileSys::File& outfile,int submesh)
{
	nema::sSubMeshMaterial_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('BSMM');
	chunk.MajorVersion = ASP_MAJOR_VERSION;			
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	
	
	chunk.NumberOfMaterials = 0;	

	DWORD numparts = 2*m_TextureList.size();
	for2 (int p = 0 ; p != numparts ; p +=2 )
	{
		if (!m_PartitionedTriIndices[submesh][p].empty() || !m_PartitionedTriIndices[submesh][p+1].empty())
		{
			chunk.NumberOfMaterials++;
		}
	}

	outfile.WriteStruct(chunk);

	nema::sSubMeshMaterialData data;
	data.MaterialID = 0;
	data.NumberOfFaces = 0;

	for2 (int p = 0 ; p != numparts ; p +=2 )
	{
		data.NumberOfFaces = m_PartitionedTriIndices[submesh][p].size() + m_PartitionedTriIndices[submesh][p+1].size();
		if (data.NumberOfFaces > 0)
		{
			data.NumberOfFaces /= 3;
			//gpgenericf(("%d: Number of faces: %d\n",data.MaterialID,data.NumberOfFaces));
			outfile.WriteStruct(data);
		}
		data.MaterialID++;
	}

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteVertexData(FileSys::File& outfile,int submesh)
{

	nema::sVertList_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('BVTX');					// "BVTX"
	chunk.MajorVersion = ASP_MAJOR_VERSION;	
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	
	chunk.NumberOfVerts = m_VertLookup[submesh].size();

	outfile.WriteStruct(chunk);

	Mesh& msh = m_ExportTriObj->GetMesh();

	Matrix3 objxfrm = m_ExportNode->GetObjectTM(TimeValue(0));

	//dbgwart gpgenericf(("Voitexans!\n"));

	if (m_IsDeformable) 
	{	
		objxfrm.Translate(-m_CurrRootPosition);
		objxfrm.RotateX(-HALFPI);
	}
	else
	{
		Matrix3 nodexfrm = m_ExportNode->GetNodeTM(TimeValue(0));
		AffineParts ap;
		decomp_affine(nodexfrm,&ap);
		Matrix3 posmat = TransMatrix(-ap.t);
		Matrix3 rotmat(true);
		ap.q.Inverse().MakeMatrix(rotmat);
		objxfrm = objxfrm * posmat * rotmat;
	}

	for (int v = 0; v != m_VertLookup[submesh].size(); ++v)
	{
		int vi = m_VertLookup[submesh][v];
		
		Point3 vert = msh.getVert(vi);

		//dbgwart gpgenericf(("%d\t[%f,%f,%f]\n",vi,vert.x,vert.y,vert.z));

		vert = objxfrm * vert;
		vert *= 1/1000.0f;

		//dbgwart gpgenericf(("\t[%f,%f,%f]\n",vert.x,vert.y,vert.z));
		
		outfile.Write(&vert.x,sizeof(float));
		outfile.Write(&vert.y,sizeof(float));
		outfile.Write(&vert.z,sizeof(float));

	}

	return true;
}
	
//-----------------------------------------------------------------
bool ASPExporter::WriteWeightedCornerData(FileSys::File& outfile,int submesh)
{

	nema::sWeightedCornerList_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('WCRN');
	chunk.MajorVersion = ASP_MAJOR_VERSION;	
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	

	chunk.NumberOfCorners = 0;
		
	DWORD numparts = 2*m_TextureList.size();
	for2 (int p = 0 ; p != numparts ; p++)
	{	
		chunk.NumberOfCorners += m_PartitionedTriCorners[submesh][p].size();
	}
	
	outfile.WriteStruct(chunk);
	
	nema::sWeightedCorner_Chunk corner;
	
	Mesh& msh = m_ExportTriObj->GetMesh();

	Matrix3 objxfrm = m_ExportNode->GetObjectTM(TimeValue(0));

	if (m_IsDeformable) 
	{	
		objxfrm.Translate(m_CurrRootPosition);
		objxfrm.RotateX(-HALFPI);
	}
	else
	{
		Matrix3 nodexfrm = m_ExportNode->GetNodeTM(TimeValue(0));
		objxfrm = Inverse(nodexfrm) * objxfrm ;
	}

	//dbgwart gpgenericf(("\nVCORNERS %d\n",chunk.NumberOfCorners));

	for2 (int p = 0 ; p != numparts ; p++)
	{	
		//dbgwart gpgenericf(("Partition: %d\n",p));

		for (int crn = 0; crn != m_PartitionedTriCorners[submesh][p].size(); ++crn)
		{
			TriangleCorner& tricorn = m_PartitionedTriCorners[submesh][p][crn];

			// VERT
			int lvi = tricorn.m_VertIndex;

			int vi = (lvi >= m_VertLookup[submesh].size()) ? 0 : m_VertLookup[submesh][lvi];

			//dbgwart gpgenericf(("v: %2d -> %2d -> %2d\n",crn,lvi,vi));
			
			Point3 vert = msh.getVert(vi);

			//dbgwart gpgenericf(("v: %2d(%2d):\n",crn,vi));
			//dbgwart gpgenericf(("\t\t[%f,%f,%f]\n",crn,vi,vert.x,vert.y,vert.z));

			vert = objxfrm * vert;
			vert *= 1/1000.0f;

			corner.v.x = vert.x;
			corner.v.y = vert.y;
			corner.v.z = vert.z;

			//dbgwart gpgenericf(("\t\t[%f,%f,%f]\n",corner.v.x,corner.v.y,corner.v.z));

			// BONE WEIGHTS

			corner.b0 = 0.0f;
			corner.b1 = 0.0f;
			corner.b2 = 0.0f;
			corner.b3 = 0.0f;
			corner.indices = 0;	// is this off-by-one?
			
			//dbgwart DWORD badindices = 0;

			if (!m_VertexBoneWeights.empty() && (m_VertexBoneWeights.size() > vi))
			{
				BoneWeightList& bwl = m_VertexBoneWeights[vi];
				
				BoneWeightList::iterator bwit = bwl.begin();
				
				if (bwit != bwl.end())
				{
					corner.b0 = (*bwit).m_Weight;
					corner.indices = (*bwit).m_BoneIndex & 0xff;
			//dbgwart 		badindices = (1+(*bwit).m_BoneIndex) & 0xff;
					++bwit;
				}
				if (bwit != bwl.end())
				{
					corner.b1 = (*bwit).m_Weight;
					corner.indices &= ((*bwit).m_BoneIndex & 0xff)<<8;
			//dbgwart 		badindices &= (1+((*bwit).m_BoneIndex) & 0xff)<<8;
					++bwit;
				}
				if (bwit != bwl.end())
				{
					corner.b2 = (*bwit).m_Weight;
					corner.indices &= ((*bwit).m_BoneIndex & 0xff)<<16;
			//dbgwart 		badindices &= (1+((*bwit).m_BoneIndex) & 0xff)<<16;
					++bwit;
				}
				if (bwit != bwl.end())
				{
					corner.b3 = (*bwit).m_Weight;
					corner.indices &= ((*bwit).m_BoneIndex & 0xff)<<24;
			//dbgwart 		badindices &= (1+((*bwit).m_BoneIndex) & 0xff)<<24;
					++bwit;
				}
			}

			//dbgwart gpgenericf(("BONE0: %f\n",corner.b0));
			//dbgwart gpgenericf(("BONE1: %f\n",corner.b1));
			//dbgwart gpgenericf(("BONE2: %f\n",corner.b2));
			//dbgwart gpgenericf(("BONE3: %f\n",corner.b3));
			//dbgwart gpgenericf(("INDICES:  0x%08x (0x%08x)\n",corner.indices,badindices));

			// NORMALS

			corner.n.x = tricorn.m_Normal.x;
			corner.n.y = tricorn.m_Normal.y;
			corner.n.z = tricorn.m_Normal.z;

			//dbgwart gpgenericf(("n:  [%f,%f,%f]\n",corner.n.x,corner.n.y,corner.n.z));

			// DIFFUSE COLOR

			UBYTE red = (UBYTE) (255 * min( 1.0f, max( 0.0f,tricorn.m_VertColor.x ) ) );
			UBYTE grn = (UBYTE) (255 * min( 1.0f, max( 0.0f,tricorn.m_VertColor.y ) ) );
			UBYTE blu = (UBYTE) (255 * min( 1.0f, max( 0.0f,tricorn.m_VertColor.z ) ) );
			
			UBYTE alf =  m_HasVertexAlpha ? (255 * ( 1.0f - min( 1.0f,max( 0.0f,tricorn.m_TexCoord.z ) ) )) : 255;

			corner.color = (alf<<24) + (red<<16) + (grn<<8) + blu;

			//dbgwart gpgenericf(("vc: [%f,%f,%f]\n",tricorn.m_VertColor.x,tricorn.m_VertColor.y,tricorn.m_VertColor.z));
			//dbgwart gpgenericf(("VALPHA: %s\n",m_HasVertexAlpha ? "TRUE" : "FALSE"));
			//dbgwart gpgenericf(("COLOR DWORD: 0x%08x\n",corner.color));

			// TEXCOORD

			corner.uv.u = tricorn.m_TexCoord.x;
			corner.uv.v = tricorn.m_TexCoord.y;

			//dbgwart gpgenericf(("uv: [%f,%f]\n",corner.uv.u,corner.uv.v));

			outfile.WriteStruct(corner);

		}
	}

	return true;

}

//-----------------------------------------------------------------
bool ASPExporter::WriteCornerMapping(FileSys::File& outfile,int submesh)
{
	nema::sVertMapData_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('BVMP');
	chunk.MajorVersion = ASP_MAJOR_VERSION;	
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	

	outfile.WriteStruct(chunk);

	DWORD count=0;

	for (CornerListMap::iterator it = m_CornerMapping[submesh].begin();
								 it != m_CornerMapping[submesh].end();
								 ++it)
	{
		DWORD sz = (*it).second.size();
		outfile.Write(&sz,sizeof(DWORD));

		//dbgwart gpgenericf(("%d: size %d\n\t",count++,sz));

		for (DWORD j = 0; j != sz; ++j)
		{
			outfile.Write(&((*it).second[j]),sizeof(DWORD));
			//dbgwart gpgenericf((j?",%d":"%d",(*it).second[j]));
		}
		//dbgwart gpgenericf(("\n"));
	}

	return true;
}

//-----------------------------------------------------------------
DWORD ASPExporter::WriteTriangleData(FileSys::File& outfile,int submesh)
{

	nema::sTriangleList_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('BTRI');
	chunk.MajorVersion = ASP_MAJOR_VERSION;	
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	
	chunk.NumberOfTriangles = 0;

	DWORD numparts = 2*m_TextureList.size();
	for2 (int p = 0 ; p != numparts ; ++p)
	{	
		chunk.NumberOfTriangles += m_PartitionedTriIndices[submesh][p].size()/3;
	}	
	
	outfile.WriteStruct(chunk);

	//dbgwart gpgenericf(("tricount: %d\n",chunk.NumberOfTriangles));

	if (chunk.NumberOfTriangles == 0)
	{
		return 0;
	}

	// Do each 'regular' partition followed by the 'draw last' 
	// partition for the same material

	std::list<DWORD> startlist;
	std::list<DWORD>::iterator startit;
	DWORD startindex = 0;

	for2 (int p = 0 ; p != numparts ; p+=2)
	{	
/*		DWORD startindex = 0xFFFFFFFF;
		DWORD finalindex = 0;

		DWORD sz_plain = m_PartitionedTriIndices[submesh][p].size();
		DWORD sz_funky = m_PartitionedTriIndices[submesh][p+1].size();
		
		if ((sz_funky + sz_plain) == 0)
		{
			continue;
		}

		for2 (int t = 0 ; t != sz_plain ; t++)
		{
			DWORD pt = m_PartitionedTriIndices[submesh][p][t];	
			if (pt < startindex) startindex = pt;
			if (pt > finalindex) finalindex = pt;
		}

		for2 (int t = 0 ; t != sz_funky ; t++)
		{
			DWORD pt = m_PartitionedTriIndices[submesh][p+1][t];
			if (pt < startindex) startindex = pt;
			if (pt > finalindex) finalindex = pt;
		}
		
		DWORD length= finalindex-startindex+1;
*/
		DWORD sz_plain = m_PartitionedTriCorners[submesh][p].size();
		DWORD sz_funky = m_PartitionedTriCorners[submesh][p+1].size();

		DWORD length = sz_plain + sz_funky;

		if (length == 0)
		{
			continue;
		}

		outfile.Write(&startindex,sizeof(DWORD));
		outfile.Write(&length,sizeof(DWORD));

		//dbgwart gpgenericf(("%d: %d for %d\n",p, startindex,length));

		startlist.push_back(startindex);
		startlist.push_back(sz_plain);

		startindex += length;
		
	}

	startit = startlist.begin();

	// count the number of tris we write
	int c=0;

	for2 (int p = 0 ; p != numparts ; p+=2)
	{	

		DWORD sz_plain = m_PartitionedTriIndices[submesh][p].size();
		DWORD sz_funky = m_PartitionedTriIndices[submesh][p+1].size();

		if ((sz_funky + sz_plain) == 0)
		{
			continue;
		}

		DWORD plainstart = (*startit);
		++startit;

		for2 (int t = 0 ; t != sz_plain ; t+=3)
		{
			DWORD ptA,ptB,ptC;

			// Write out the partitioned triindices
			ptA = m_PartitionedTriIndices[submesh][p][t+0];
			ptB = m_PartitionedTriIndices[submesh][p][t+1];
			ptC = m_PartitionedTriIndices[submesh][p][t+2];

			outfile.Write(&ptA,sizeof(DWORD));
			if (m_HasNegativeParity)
			{
				outfile.Write(&ptC,sizeof(DWORD));
				outfile.Write(&ptB,sizeof(DWORD));			
				//dbgwart gpgenericf(("%d:{%d:%d} [%d,%d,%d]\n",c++,p,t,ptA,ptC,ptB));	
			}
			else
			{
				outfile.Write(&ptB,sizeof(DWORD));			
				outfile.Write(&ptC,sizeof(DWORD));
				//dbgwart gpgenericf(("%d:{%d:%d} [%d,%d,%d]\n",c++,p,t,ptA,ptB,ptC));	
			}
		}
		
		DWORD funkystart = (*startit);
		++startit;

		for2 (int t = 0 ; t != sz_funky ; t+=3)
		{
			DWORD ptA,ptB,ptC;
			
			// Write out the partitioned triindices
			ptA = m_PartitionedTriIndices[submesh][p+1][t+0] + funkystart;
			ptB = m_PartitionedTriIndices[submesh][p+1][t+1] + funkystart;
			ptC = m_PartitionedTriIndices[submesh][p+1][t+2] + funkystart;

			outfile.Write(&ptA,sizeof(DWORD));
			if (m_HasNegativeParity)
			{
				outfile.Write(&ptC,sizeof(DWORD));
				outfile.Write(&ptB,sizeof(DWORD));			
				//dbgwart gpgenericf(("%d:{%d:%d} [%d,%d,%d]\n",c++,p+1,t,ptA,ptC,ptB));	
			}
			else
			{
				outfile.Write(&ptB,sizeof(DWORD));			
				outfile.Write(&ptC,sizeof(DWORD));
				//dbgwart gpgenericf(("%d:{%d:%d} [%d,%d,%d]\n",c++,p+1,t,ptA,ptB,ptC));	
			}
		}
		
	}	

	return chunk.NumberOfTriangles;

/*
	WriteChunkID "0x49525442" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BTRI
	
	WriteBinInt g_File g_Tris.count
	
	local startindices = #()
	local runlengths = #()
	
	for tlist in g_StrippedTris do 
	(
		if tlist.count > 0 then
		(
			-- Find the starting index for this set of tris
			local startindex = 10000000
			local finalindex = 0
			for tri in tlist do
			(
				if tri[1] < startindex then startindex = tri[1]
				if tri[2] < startindex then startindex = tri[2]
				if tri[3] < startindex then startindex = tri[3]
				
				if tri[1] > finalindex then finalindex = tri[1]
				if tri[2] > finalindex then finalindex = tri[2]
				if tri[3] > finalindex then finalindex = tri[3]
			)
			
			if (finalindex>g_Corners.count) then (
				g_BlastErrorCode = BLAST_ERROR_BAD_OPTIMIZE
				if g_Verbose then (
						MessageBox (
						"Failed to export properly!\n\n" +
						"The mesh optimizer has failed. Please remove all dupes and spikes")title:"Bad News"
				)
				format "ERROR: Failed to export! The optimizer failed. Please STL check and remove double faces\n"  tris_done g_Tris.count
				return false;
			)
		
			WriteBinInt g_File (startindex-1)
			WriteBinInt g_File (finalindex+1-startindex)
			append startindices startindex
			append runlengths (finalindex+1-startindex)
		)
		else
		(
			append startindices 0
			append runlengths 0
		)
	)
	
	
format "tricount %\n" g_Tris.count
format "starts % lengths %\n" startindices runlengths
	
	local tris_done = 0
	local drawn_last = 0
	local drawn_last_total = 0
	
	DrawLastBA = #{}
	if (g_Mesh.faces[#drawlast] != undefined) then 
	(
		for f in g_Mesh.faces[#drawlast] do 
		(
			append DrawLastBA f.index
			drawn_last_total +=1
		)
	)
	
	if (DrawLastBA.count == 0) then
	(
		for t = 1 to g_StrippedTris.count do 
		(	
			tlist = g_StrippedTris[t]
			startindex = startindices[t]
			
			for tri in tlist do
			(
				if (p_NegativeParity) then 
				(
					WriteBinInt g_File (tri[1]-startindex)
					WriteBinInt g_File (tri[3]-startindex)
					WriteBinInt g_File (tri[2]-startindex)
				)
				else
				(
					WriteBinInt g_File (tri[1]-startindex)
					WriteBinInt g_File (tri[2]-startindex)
					WriteBinInt g_File (tri[3]-startindex)
				)
				tris_done += 1
			)
		)
	) 
	else
	(
		
		if (p_NegativeParity) then (
		
			for sset in g_SubMeshMaterialSets do (
			
				WriteBinInt g_File startindex-1
			
				for t in sset do (
					if DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					tris_done += 1
				)
				for t in sset do (
					if not DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					tris_done += 1
					drawn_last += 1
				)
			)
		)
		else
		(

			for sset in g_SubMeshMaterialSets do (
		
				WriteBinInt g_File startindex-1
			
				for t in sset do (
					if DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					tris_done += 1
				)
				for t in sset do (
					if not DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					drawn_last += 1
					tris_done += 1
				)
			)
		)
	)
	
	
	if (g_Tris.count == tris_done and drawn_last == drawn_last_total) then (
		return true
	) else (
		g_BlastErrorCode = BLAST_ERROR_BAD_MATERIALS
		format "tris done = % tris expected = %\n" tris_done g_Tris.count
		format "drawn last = % drawn last expected = %\n" drawn_last drawn_last_total
		if g_Verbose then (
				MessageBox (
				"Failed to export properly!\n\n" +
				"Are you SURE all the materials you are using are valid AND MAPPED TO THE SHADOW?\n\n" +
				"You can be reasonably sure that your texures are OK if they pass the Verification Toolkit\n\n" +
				"Please check everything twice before you start blaming the exporter =)") title:"Bad News"
		)
		format "ERROR: Failed to export! wrote % tris, needed to write %. Does the object pass the material verifier? Are all materials mapped to the shadow?\n"  tris_done g_Tris.count
	)
		
	return false
	
)

*/
}
	
//-----------------------------------------------------------------
bool ASPExporter::WriteBoneWeightList(FileSys::File& outfile,int submesh)
{
	nema::sBoneVertData_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('BVWL');
	chunk.MajorVersion = ASP_MAJOR_VERSION;	
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	

	outfile.WriteStruct(chunk);

	if (m_IsDeformable)
	{
		//dbgwart gpgenericf(("BVWL for %d (%d verts)\n",submesh,m_VertLookup[submesh].size()));

		std::vector< VertWeightList > tempBVW;
		
		for2 (int b=0; b!=m_Bones.size(); ++b)
		{
			VertWeightList emptylist; 
			tempBVW.push_back(emptylist);
		}

		for2 (int v=0; v!=m_VertLookup[submesh].size(); ++v)
		{
			DWORD vi = m_VertLookup[submesh][v];
			
			BoneWeightList::iterator beg = m_VertexBoneWeights[vi].begin();
			BoneWeightList::iterator end = m_VertexBoneWeights[vi].end();
			BoneWeightList::iterator it;
			for ( it=beg; it!=end; ++it)
			{
				tempBVW[(*it).m_BoneIndex].push_back(VertWeight(v,(*it).m_Weight));
			}
		}

		for2 (DWORD b=0; b != m_Bones.size(); ++b)
		{
			
			DWORD sz = tempBVW[b].size();
			outfile.Write(&sz,sizeof(DWORD));
			//dbgwart gpgenericf(("%d: bvwl %d\n",b,sz));

			for (VertWeightList::iterator vwlit = tempBVW[b].begin();
										  vwlit != tempBVW[b].end();
										  ++vwlit)
			{

				outfile.Write(&(*vwlit).m_VertIndex,sizeof(DWORD));
				outfile.Write(&(*vwlit).m_Weight,sizeof(float));

				//dbgwart gpgenericf(("\t(%d,%f)\n",(*vwlit).m_VertIndex,(*vwlit).m_Weight));
			}
		}
	}
	else
	{
		// This is a rigid object that is entirely weighted to a single mesh
		for2 (DWORD b=0; b != m_Bones.size(); ++b)
		{
			//dbgwart gpgenericf(("%d: bvwl empty\n",b));
			static DWORD negone = -1;
			static DWORD zero = 0;
			if (m_RigidBoneID == b)
			{
				//dbgwart gpgenericf(("\t-1\n"));
				outfile.Write(&negone,sizeof(DWORD)); //This is the bone that all the verts are attached to
			}
			else
			{
				//dbgwart gpgenericf(("\t0\n"));
				outfile.Write(&zero,sizeof(DWORD));
			}
		}

	}


/*
	-- Write out the weights sorted by BONE and indexed to VERTEX
	
	WriteChunkID "0x4C575642" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BVWL
	
	-- If p_NumBoneWeights is negative, then it will be interpreted as a flag
	-- currently a value of -1 means that the bone will have all the verts rigidly linked to it
	
	for curr_i = 1 to g_BoneList.count do (
	
		if (g_BoneVertexWeights.count == 0) then (
			
			if g_RigidBoneID == curr_i then (
				WriteBinInt g_File -1		-- This is the bone that all the verts are attached to
			) else (
				WriteBinInt g_File 0
			)
			
		) else (
		
			WriteBinInt g_File g_BoneVertexWeights[curr_i].count
	
			for bvw in g_BoneVertexWeights[curr_i] do (
				WriteBinInt   g_File (bvw[1]-1)	-- vert index (accounting for the off-by-one)
				WriteBinFloat g_File bvw[2]		-- weight
			)
		)

	)

*/
	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteStitchSets(FileSys::File& outfile,int submesh)
{

	nema::sStitchSetHeader_Chunk chunk;

	chunk.ChunkName = REVERSE_FOURCC('STCH');
	chunk.MajorVersion = ASP_MAJOR_VERSION;	
	chunk.MinorVersion = ASP_MINOR_VERSION;			
	chunk.ExtraVersion = 0;	
	chunk.NumberOfSets = 0;

	StitchSetMap::iterator pbeg = m_StitchSets.begin();
	StitchSetMap::iterator pend = m_StitchSets.end();
	StitchSetMap::iterator pit;
		
	StitchSetMap remappedstitchverts;

	std::vector<int>& vlook = m_VertLookup[submesh];
	std::vector<int>::iterator vit;

	for (pit=pbeg; pit != pend; ++pit)
	{
		std::vector<int> templist;
		DWORD tag = (*pit).first;
		DWORD sz = (*pit).second.size();
		for (int v = 0; v != sz; ++v)
		{
			DWORD vert = (*pit).second[v];

			vit = std::find(vlook.begin(),vlook.end(),vert);
			if (vit != vlook.end())
			{
				templist.push_back(vit-vlook.begin());
			}
		}
		if (templist.size() == sz)
		{
			remappedstitchverts[tag] = templist;
			chunk.NumberOfSets++;
		}
	}

	outfile.WriteStruct(chunk);

	StitchSetMap::iterator rmbeg = remappedstitchverts.begin();
	StitchSetMap::iterator rmend = remappedstitchverts.end();
	StitchSetMap::iterator rmit;
	
	for (rmit=rmbeg; rmit != rmend; ++rmit)
	{
		DWORD rmtag = (*rmit).first;
		DWORD rmsz = (*rmit).second.size();
		outfile.Write(&rmtag,sizeof(rmtag));
		outfile.Write(&rmsz,sizeof(rmsz));
		for (int v = 0; v != rmsz; ++v)
		{
			DWORD vert = (*rmit).second[v];
			outfile.Write(&vert,sizeof(vert));
		}
	}

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteTriangleSet(FileSys::File& outfile,int submesh)
{
//	BuildSubMeshMaterialSets(submesh);

	BuildTriangleCorners(submesh);

	WriteSubMeshHeader(outfile,submesh);

	WriteSubMeshMaterial(outfile,submesh);

	WriteVertexData(outfile,submesh);

	WriteWeightedCornerData(outfile,submesh);

	WriteCornerMapping(outfile,submesh);

	DWORD triswritten = WriteTriangleData(outfile,submesh);

	WriteBoneWeightList(outfile,submesh);

	WriteStitchSets(outfile,submesh);

/*
	

	-- Collect info about vert stitch sets of this particular sub mesh
	g_StitchSets = #()
		
	for s = 1 to stitch_set_names.count do
	(
		ssn = stitch_set_names[s]
		ssf = stitch_set_fourcc[s]
		
		StitchVerts = FetchStitchVerts p_Mesh (ssn+"_VERTS")
		if StitchVerts.count > 0 then (
		
			remappedstitchverts = #()
			
			for v in StitchVerts do (
			
				pos = finditem g_VertLookup v
				if pos > 0 then (
					append remappedstitchverts (pos-1)
				)
				
			)
					
			if (remappedstitchverts.count > 0) then (
				copy remappedstitchverts
				append g_StitchSets #(ssf,remappedstitchverts)
			)
		)
	)

	WriteStitchSets()
*/	

/*
	if not (ValidSubMesh v) then (
		continue
	)
	
	SelectFacesInSubMesh p_Mesh v
	local subs_ok = BuildSubMeshMaterialSets p_Mesh		-- partition the faces of the sub mesh by materials
	
	if (not subs_ok) then (
	
		EnterShowSelectedFacesMode p_Mesh
		subObjectLevel = 0
		ExitShowSelectedFacesMode p_Mesh
		RestoreSelectedFaces p_Mesh
		setarrowcursor()

		slidertime = savedslidertime 
		if g_Verbose then (MessageBox "Failed to locate any faces with one of the materials\nAre you sure your materials are valid?" title:"Bad News")
		else (format "ERROR: Failed to export properly! Failed to locate any faces with one of the materials\n") 
		g_BlastErrorCode = BLAST_ERROR_BAD_MATERIALS
		return false
	)
	
	prog_scale = (v+1)*1.0/(MAX_MESH_VARIATIONS+1)

	BuildTriangleCorners p_ProgBar prog_scale
	
!	g_StrippedTris = #()
!	
!	for sset in g_SubMeshMaterialSets do 
!	(
!		local submesh = #()
!		for t in sset do
!		(
!			append submesh g_Tris[t]	
!		)
!		append g_StrippedTris submesh
!	)

!	stripped_ok = doastripify g_StrippedTris g_corners g_vertmapping
!	if not stripped_ok then (
!		if (g_Verbose) then
!		(
!			MessageBox "You need at least one bone attached to the object\n\nPerhaps you are missing a 'Dummy Root'?" title:"Bad News"
!		)
!		format "ERROR: There is no bone attached to the object. Perhaps you are missing a 'Dummy Root'?\n"
!		g_BlastErrorCode = BLAST_ERROR_MISSING_MESH	
!		return false
!	)
	
!	if (p_ProgBar != undefined) then p_ProgBar.value = 80 * prog_scale
	
	BuildVertexWeights()

	WriteSubMeshHeader v
	
	WriteSubMeshMaterialChunk()
	
	if (p_ProgBar != undefined) then p_ProgBar.value = 84 * prog_scale	
	
	WriteVertexData()
	
	if (p_ProgBar != undefined) then p_ProgBar.value = 90 * prog_scale
	
	WriteCornerData()
	
	NormalizeVertexWeights()
	
	WriteWeightedCornerData()
	
	if (p_ProgBar != undefined) then p_ProgBar.value = 94 * prog_scale
	
	WriteVertexMapping()
	
	if (p_ProgBar != undefined) then p_ProgBar.value = 96 * prog_scale	
	
	EnterShowSelectedFacesMode p_Mesh
	triswritten = WriteTriangleData (HasNegativeParity g_Mesh.transform)
	ExitShowSelectedFacesMode p_Mesh
	
	if (not triswritten) then (
		slidertime = savedslidertime 
		EnterShowSelectedFacesMode p_Mesh
		subObjectLevel = 0
		ExitShowSelectedFacesMode p_Mesh
		RestoreSelectedFaces p_Mesh
		setarrowcursor()
		return false
	)
	
	WriteBoneWeightList()
	-- WriteVertexBoneOffsetList()
	
	-- Collect info about vert stitch sets of this particular sub mesh
	g_StitchSets = #()
		
	for s = 1 to stitch_set_names.count do
	(
		ssn = stitch_set_names[s]
		ssf = stitch_set_fourcc[s]
		
		StitchVerts = FetchStitchVerts p_Mesh (ssn+"_VERTS")
		if StitchVerts.count > 0 then (
		
			remappedstitchverts = #()
			
			for v in StitchVerts do (
			
				pos = finditem g_VertLookup v
				if pos > 0 then (
					append remappedstitchverts (pos-1)
				)
				
			)
					
			if (remappedstitchverts.count > 0) then (
				copy remappedstitchverts
				append g_StitchSets #(ssf,remappedstitchverts)
			)
		)
	)
	
	WriteStitchSets()


*/
	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteRestPose(FileSys::File& outfile)
{
	
	nema::sRestPoseList_Chunk chunk;

	chunk.ChunkName					= REVERSE_FOURCC('RPOS');
	chunk.MajorVersion				= ASP_MAJOR_VERSION;			
	chunk.MinorVersion				= ASP_MINOR_VERSION;			
	chunk.ExtraVersion				= 0;	
	chunk.NumberOfBones				= m_Bones.size();	
	
	outfile.WriteStruct( chunk );

	TimeValue kt = GetCOREInterface()->GetTime();

	AffineParts ap;

	for2 (DWORD b = 0; b < m_Bones.size(); b++)
	{

		Matrix3 objxfrm = m_Bones[b].m_INode->GetNodeTM(kt);

		//dbgwart gpgenericf(("\nbone %d %s\n",b,m_Bones[b].m_INode->GetName()));

		decomp_affine(objxfrm, &ap);
		Point3 bonepos = ap.t;
		Quat bonerot = ap.q.Inverse();			

		//dbgwart gpgenericf(("brot(A):(quat %f %f %f %f)\n",bonerot.x,bonerot.y,bonerot.z,bonerot.w));
		//dbgwart gpgenericf(("bpos(A):[%f,%f,%f]\n",bonepos.x,bonepos.y,bonepos.z));

		bonepos = m_CurrRootRotMatrix * (bonepos - m_CurrRootPosition);
		bonerot = m_CurrRootRotation.Inverse() * bonerot;

		//dbgwart gpgenericf(("brot(B):(quat %f %f %f %f)\n",bonerot.x,bonerot.y,bonerot.z,bonerot.w));
		//dbgwart gpgenericf(("bpos(B):[%f,%f,%f]\n",bonepos.x,bonepos.y,bonepos.z));

		if (false)
		{
			Matrix3 YisUp(true);
			YisUp.RotateX(HALFPI);
			bonepos = YisUp * bonepos;
			bonerot = Quat(YisUp) * bonerot;
			//dbgwart gpgenericf(("brot(C):(quat %f %f %f %f)\n",bonerot.x,bonerot.y,bonerot.z,bonerot.w));
			//dbgwart gpgenericf(("bpos(C):[%f,%f,%f]\n",bonepos.x,bonepos.y,bonepos.z));
		}


		Quat invbonerot = bonerot.Inverse();

		Matrix3 bonerotmat;
		bonerot.MakeMatrix(bonerotmat);

		Point3 invbonepos = bonerotmat * (-bonepos);
						
		invbonepos = invbonepos / 1000.0f;
			
		//dbgwart gpgenericf(("brot:    (quat %f %f %f %f)\n",bonerot.x,bonerot.y,bonerot.z,bonerot.w));
		//dbgwart gpgenericf(("bpos:    [%f,%f,%f]\n",bonepos.x,bonepos.y,bonepos.z));
		//dbgwart gpgenericf(("invbrot: (quat %f %f %f %f)\n",invbonerot.x,invbonerot.y,invbonerot.z,invbonerot.w));
		//dbgwart gpgenericf(("invbpos: [%f,%f,%f]\n",invbonepos.x,invbonepos.y,invbonepos.z));	

		outfile.Write(&invbonerot.x,sizeof(float));
		outfile.Write(&invbonerot.y,sizeof(float));
		outfile.Write(&invbonerot.z,sizeof(float));
		outfile.Write(&invbonerot.w,sizeof(float));

		outfile.Write(&invbonepos.x,sizeof(float));
		outfile.Write(&invbonepos.y,sizeof(float));
		outfile.Write(&invbonepos.z,sizeof(float));

		Quat relrestrot;
		Point3 relrestpos;

		if (b == 0) 
		{
			Matrix3 kidxfrm =  m_Bones[b].m_INode->GetNodeTM(kt);
			Matrix3 relrestxfrm = kidxfrm * Inverse(m_CurrRootTransform);
			decomp_affine(relrestxfrm, &ap);
			relrestpos = ap.t;
			relrestrot = ap.q.Inverse();
		}
		else
		{
			Matrix3 parxfrm = m_Bones[m_Bones[b].m_ParentIndex].m_INode->GetNodeTM(kt);

			decomp_affine(parxfrm, &ap);
			Quat parentrot = ap.q.Inverse();

			Matrix3 parentrotmat;
			parentrot.MakeMatrix(parentrotmat);

			Point3 parentpos = parxfrm.GetTrans();

			Matrix3 kidxfrm = m_Bones[b].m_INode->GetNodeTM(kt);

			decomp_affine(kidxfrm, &ap);
			Quat kidrot = ap.q.Inverse();
	
			Point3 kidpos = kidxfrm.GetTrans();

			relrestpos =  parentrotmat * (kidpos-parentpos);
			relrestrot =  parentrot.Inverse() * kidrot;
		}

		relrestpos = relrestpos / 1000.0f;

		//dbgwart gpgenericf(("relrrot: (quat %f %f %f %f)\n",relrestrot.x,relrestrot.y,relrestrot.z,relrestrot.w));
		//dbgwart gpgenericf(("relrpos: [%f,%f,%f]\n",relrestpos.x,relrestpos.y,relrestpos.z));
		
		outfile.Write(&relrestrot.x,sizeof(float));
		outfile.Write(&relrestrot.y,sizeof(float));
		outfile.Write(&relrestrot.z,sizeof(float));
		outfile.Write(&relrestrot.w,sizeof(float));

		outfile.Write(&relrestpos.x,sizeof(float));
		outfile.Write(&relrestpos.y,sizeof(float));
		outfile.Write(&relrestpos.z,sizeof(float));
	}

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteBoundingBoxes(FileSys::File& outfile)
{
	bool bad_data = false;

	std::list< std::pair <INode*,DWORD> > bboxes;
	std::list< std::pair <INode*,DWORD> >::iterator it;
	
	gpstring cmd;
	cmd.assignf("dscb_CollectBBoxes $'%s'",m_ExportNode->GetName());
	Value* v = RunMaxScriptCommand(cmd.begin_split());

	if (!is_undefined(v))
	{
		if ( is_array(v) )
		{
			Array* a = (Array*)v;
			for (int i = 0; !bad_data && (i < (*a).size) ; i++)
			{
				INode* bbox = (*a)[i]->to_node();
				if ( bbox )
				{
					bboxes.push_back(std::make_pair(bbox,0));
				}
				else
				{
					bad_data = true;
					bboxes.clear();
				}
			}
		}
		v->make_collectable();
	}
	

	for (it=bboxes.begin(); !bad_data && (it != bboxes.end()) ; )
	{

		gpstring cmd;
		cmd.assignf("dscb_FetchBBoxFourCC $'%s'",(*it).first->GetName());
		Value* v = RunMaxScriptCommand(cmd.begin_split());

		if (!is_undefined(v))
		{
			TCHAR* fourCC = v->to_string();
			if (fourCC)
			{
				(*it).second = FourCCToDWORD(fourCC);	
				++it;
			}
			else
			{
				bad_data = true;
				bboxes.clear();
				it = bboxes.end();
			}
			v->make_collectable();
		}
	}
	
	nema::sBBoxList_Chunk chunk;

	chunk.ChunkName				= REVERSE_FOURCC('BBOX');
	chunk.MajorVersion			= ASP_MAJOR_VERSION;			
	chunk.MinorVersion			= ASP_MINOR_VERSION;			
	chunk.ExtraVersion			= 0;	
	chunk.NumberOfBoxes			= bboxes.size();

	outfile.WriteStruct( chunk );

	for (it=bboxes.begin(); !bad_data && (it != bboxes.end()) ; ++it)
	{

		outfile.Write(&(*it).second,sizeof(DWORD));

		Matrix3 kidxfrm =  (*it).first->GetNodeTM(TimeValue(0));
		Matrix3 relrestxfrm = kidxfrm * Inverse(m_CurrRootTransform);
		AffineParts ap;
		decomp_affine(relrestxfrm, &ap);
		Point3 boxpos = ap.t;
		Quat boxrot = ap.q.Inverse();
	
		//dbgwart gpgenericf(("boxrot: (quat %f %f %f %f)\n",boxrot.x,boxrot.y,boxrot.z,boxrot.w));
		//dbgwart gpgenericf(("boxpos: [%f,%f,%f]\n",boxpos.x,boxpos.y,boxpos.z));
		
		outfile.Write(&boxrot.x,sizeof(float));
		outfile.Write(&boxrot.y,sizeof(float));
		outfile.Write(&boxrot.z,sizeof(float));
		outfile.Write(&boxrot.w,sizeof(float));

		outfile.Write(&boxpos.x,sizeof(float));
		outfile.Write(&boxpos.y,sizeof(float));
		outfile.Write(&boxpos.z,sizeof(float));

		BaseObject *obj = (*it).first->GetObjectRef();

		// Lets grab the first modifier on the stack
		while (obj->SuperClassID() == GEN_DERIVOB_CLASS_ID )
		{
			IDerivedObject *pDerObj = (IDerivedObject *) obj;
			obj = pDerObj->GetObjRef();
		}

		Box3 localbox;
		obj->GetLocalBoundBox(TimeValue(0),(*it).first,0,localbox);
		
		Point3 halfdiag = localbox.Width() / (2.0f*1000.0f);

		outfile.Write(&halfdiag.x,sizeof(float));
		outfile.Write(&halfdiag.y,sizeof(float));
		outfile.Write(&halfdiag.z,sizeof(float));

	}

	return true;
}

//-----------------------------------------------------------------
bool ASPExporter::WriteEndOfMesh(FileSys::File& outfile)
{
	DWORD fourcc = REVERSE_FOURCC('BEND');
	outfile.Write(&fourcc,sizeof(fourcc));
	return true;
}

//-----------------------------------------------------------------
DSiegeUtils::eExpErrType 
ASPExporter::ExportINode(INode *node, const gpstring& expfname, FileSys::File& outfile)
{

	Object *obj =node->EvalWorldState(0).obj;

	bool mustDelete = false;
	
	m_ExportTriObj = NULL;
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
	{
		m_ExportTriObj = (TriObject *)obj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));
		mustDelete = ( obj != m_ExportTriObj );
	}

	if (!m_ExportTriObj)
	{
		return DSiegeUtils::EXP_ERROR_CANNOT_CONVERT_TO_TRIOBJECT;
	}

	m_ExportNode = node;

	if (!VerifyRequiredGlobals())
	{
		return DSiegeUtils::EXP_ERROR_BUSTED_MAXSCRIPT_GLOBALS;
	}
	
	InitializeStringTable();

	m_IsDeformable = FetchSkinInterface(m_ExportNode) || FetchPhysiqueModifier(m_ExportNode);

	// Build list of all the textures on the node
	Mtl* mat = m_ExportNode->GetMtl();

	m_MaxMatID = 0;
	m_TextureList.clear();
	BuildMaterialList(mat,0);
	OptimizeMaterialList();

	for (TexListing::iterator it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
	{
		AddStringToStringTable(FileSys::GetFileNameOnly((*it).texName));
	}

	BuildBoneList();

	BuildBoneStringTable();

	DetermineRootAdjustment(TimeValue(0));
	
	Matrix3 normXform(true);

	normXform =  m_ExportNode->GetObjectTM(TimeValue(0));
	AffineParts ap;
	decomp_affine(normXform, &ap);

	m_HasNegativeParity = ap.f < 0;
	ap.q.MakeMatrix(normXform);
	
	if (m_IsDeformable)
	{
		normXform.RotateX(-HALFPI);
	}
	else
	{
		Matrix3 bonexfrm =  m_ExportNode->GetNodeTM(TimeValue(0));
		AffineParts ap;
		decomp_affine(bonexfrm, &ap);
		Matrix3 rotmat;
		ap.q.Inverse().MakeMatrix(rotmat);
		normXform = normXform * rotmat;
	}
	
	m_VertNormals.clear();
	CalcVertNormals(m_ExportTriObj,normXform,m_VertNormals,m_HasNegativeParity);

	BuildVertexWeights();

	WriteMeshHeaderChunk(outfile);

	WriteStringTable(outfile);
	
	WriteBoneInfo(outfile);
	
	WriteTriangleSet(outfile,SUBMESH_BODY);

	if (m_HasSubMeshes)
	{
		for (DWORD i = 1; i != SUBMESH_COUNT; i++)
		{
			WriteTriangleSet(outfile,i);
		}
	}

	WriteRestPose(outfile);
	
	WriteBoundingBoxes(outfile);
		
	WriteEndOfMesh(outfile);
	
	WriteInfoChunk(outfile);

	// Dump();
	
	if (mustDelete) m_ExportTriObj->DeleteMe();

	return DSiegeUtils::EXP_SUCCESS;
}

void ASPExporter::Dump(void)
{
	int b;

	//dbgwart gpgenericf(("There are %d sorted bones\n",m_Bones.size()));
	
	for (b = 0; b < m_Bones.size(); ++b)
	{
		INode* bnode = m_Bones[b].m_INode;
		gpstring bname = (bnode) ? bnode->GetName() : "<NULL>";
		stringtool::RemoveBorderingWhiteSpace(bname);

		int p = m_Bones[b].m_ParentIndex;

		INode* pnode = (p>=0) ? m_Bones[p].m_INode : NULL;
		gpstring pname = (pnode) ? pnode->GetName() : "<NULL>";
		stringtool::RemoveBorderingWhiteSpace(pname);

		//dbgwart gpgenericf(("\tBone [%d:%s] -> Parent [%d:%s]\n",b,bname.c_str(),p,pname.c_str()));
	}

	//dbgwart gpgenericf(("There are %d optimized textures:\n",m_TextureList.size()));

	int count=0;
/*dbg
	for2 (TexListing::iterator it = m_TextureList.begin(); it != m_TextureList.end(); ++it)
	{
		gpgenericf(("\t[%d:%s] faces:%d\n",count++,(*it).texName.c_str(),(*it).totFaces));
	}
	gpgenericf(("There are %d texture mappings\n",m_TextureLookup[0.size()));

	count=0;
	for2 (TexLookup::iterator it = m_TextureLookup.begin(); it != m_TextureLookup.end(); ++it)
	{
		gpgenericf(("\t%d maps to %d (%d faces)\n",count++,(*it).second.mappedTo,(*it).second.numFaces));
	}
	gpgenericf(("There are %d tris\n",m_ExportTriObj->GetMesh().numFaces));
*/

	class DumpBitArrayCB : public BitArrayCallback 
	{
	public:
		gpstring outbuf;

		void proc(int n)
		{
			outbuf.appendf(outbuf.empty() ? "%d" : ",%d",n);
		}

		void clear()
		{
			outbuf.clear();
		}
	};

	DumpBitArrayCB dba;

	m_TriSet[SUBMESH_BODY].EnumSet(dba);
	gpgenericf(("Body tris (%s)\n", dba.outbuf.empty() ? "NONE" : dba.outbuf.c_str()))
	dba.clear();

	m_TriSet[SUBMESH_HEAD].EnumSet(dba);
	gpgenericf(("Head tris (%s)\n", dba.outbuf.empty() ? "NONE" : dba.outbuf.c_str()))
	dba.clear();

	m_TriSet[SUBMESH_HAND].EnumSet(dba);
	gpgenericf(("Hand tris (%s)\n", dba.outbuf.empty() ? "NONE" : dba.outbuf.c_str()))
	dba.clear();

	m_TriSet[SUBMESH_FEET].EnumSet(dba);
	gpgenericf(("Foot tris (%s)\n", dba.outbuf.empty() ? "NONE" : dba.outbuf.c_str()))
	dba.clear();

	for2 (int v=0; v!=m_VertexBoneWeights.size(); ++v)
	{
		BoneWeightList::iterator beg = m_VertexBoneWeights[v].begin();
		BoneWeightList::iterator end = m_VertexBoneWeights[v].end();
		BoneWeightList::iterator it;

		gpgenericf(("%d : ", v));
		for ( it=beg; it!=end; ++it)
		{
			if (it == beg)
			{
				gpgenericf(("[%2d,%5.3f]",(*it).m_BoneIndex,(*it).m_Weight));
			}
			else
			{
				gpgenericf((",[%2d,%5.3f]",(*it).m_BoneIndex,(*it).m_Weight));
			}
		}
		gpgenericf(("\n", v));
	}

}

/*
$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

fn WriteMesh p_Mesh p_FName p_TopBone p_MeshFlags p_ProgBar = (

	setWaitCursor () 
	
!	g_Mesh = p_Mesh
!	g_FName = p_FName

!	if p_Mesh.modifiers[#skin] != undefined then (
!		select p_Mesh
!		max modify mode
!	)
	
!	savedslidertime = slidertime
!	slidertime = animationrange.start

!	-- For ver 2.0 we will correct for YisUP at source
!	DetermineRootAdjustment animationrange.start
	
?	SaveSelectedFaces p_Mesh

?	g_MeshAttributeFlags = p_MeshFlags 	
	
!	if (p_ProgBar != undefined) then p_ProgBar.value = 0
	
!	BuildBoneList p_TopBone
	
!	if (g_BoneList.count == 0) then (
	
!		slidertime = savedslidertime 

!		if (g_Mesh.parent == $bip01) then (
!			if g_Verbose then (
!				MessageBox "The SKINMESH object is attached to the BIPO1 and not the BIP01_PELVIS" title:"Bad News"
!			)
!			format "ERROR: The SKINMESH object is attached to the BIPO1 and not the BIP01_PELVIS\n"
!		) else (
!			if g_Verbose then (
!				MessageBox "You need at least one bone attached to the object\n\nPerhaps you are missing a 'Dummy Root'?" title:"Bad News"
!			)
!			format "ERROR: There is no bone attached to the object. Perhaps you are missing a 'Dummy Root'?\n"
!		)
			
!		g_BlastErrorCode = BLAST_ERROR_BAD_BONES
!		return false
!	)
	
	
	-- g_NumberOfMeshVariations = CountMeshVariations p_Mesh
	
!	InitializeStringTable()
	
!	-- The texture file names are the first strings in the table
!	for t in g_TextureNameList do (
!		AddStringToStringTable t
!	)

!	BuildBoneStringTable()

!	g_NumberOfSubMeshes = CountMeshVariations p_Mesh

!	if (p_ProgBar != undefined) then p_ProgBar.value = 0	
	
	WriteMeshHeaderChunk()

	WriteStringTable()
	
	WriteBoneInfo()
	
	BuildVertNormals()

	-- For each mesh variation
	
	for v = 0 to MAX_MESH_VARIATIONS do (
	
		if not (ValidSubMesh v) then (
			continue
		)
		
		SelectFacesInSubMesh p_Mesh v
		local subs_ok = BuildSubMeshMaterialSets p_Mesh		-- partition the faces of the sub mesh by materials
		
		if (not subs_ok) then (
		
			EnterShowSelectedFacesMode p_Mesh
			subObjectLevel = 0
			ExitShowSelectedFacesMode p_Mesh
			RestoreSelectedFaces p_Mesh
			setarrowcursor()
	
			slidertime = savedslidertime 
			if g_Verbose then (MessageBox "Failed to locate any faces with one of the materials\nAre you sure your materials are valid?" title:"Bad News")
			else (format "ERROR: Failed to export properly! Failed to locate any faces with one of the materials\n") 
			g_BlastErrorCode = BLAST_ERROR_BAD_MATERIALS
			return false
		)
		
		prog_scale = (v+1)*1.0/(MAX_MESH_VARIATIONS+1)
	
		BuildTriangleCorners p_ProgBar prog_scale
		
		g_StrippedTris = #()
		
		for sset in g_SubMeshMaterialSets do 
		(
			local submesh = #()
			for t in sset do
			(
				append submesh g_Tris[t]	
			)
			append g_StrippedTris submesh
		)

		stripped_ok = doastripify g_StrippedTris g_corners g_vertmapping
		if not stripped_ok then (
			if (g_Verbose) then
			(
				MessageBox "You need at least one bone attached to the object\n\nPerhaps you are missing a 'Dummy Root'?" title:"Bad News"
			)
			format "ERROR: There is no bone attached to the object. Perhaps you are missing a 'Dummy Root'?\n"
			g_BlastErrorCode = BLAST_ERROR_MISSING_MESH	
			return false
		)
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 80 * prog_scale
		
		BuildVertexWeights()
	
		WriteSubMeshHeader v
		
		WriteSubMeshMaterialChunk()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 84 * prog_scale	
		
		WriteVertexData()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 90 * prog_scale
		
		WriteCornerData()
		
		NormalizeVertexWeights()
		
		WriteWeightedCornerData()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 94 * prog_scale
		
		WriteVertexMapping()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 96 * prog_scale	
		
		EnterShowSelectedFacesMode p_Mesh
		triswritten = WriteTriangleData (HasNegativeParity g_Mesh.transform)
		ExitShowSelectedFacesMode p_Mesh
		
		if (not triswritten) then (
			slidertime = savedslidertime 
			EnterShowSelectedFacesMode p_Mesh
			subObjectLevel = 0
			ExitShowSelectedFacesMode p_Mesh
			RestoreSelectedFaces p_Mesh
			setarrowcursor()
			return false
		)
		
		WriteBoneWeightList()
		-- WriteVertexBoneOffsetList()
		
		-- Collect info about vert stitch sets of this particular sub mesh
		g_StitchSets = #()
			
		for s = 1 to stitch_set_names.count do
		(
			ssn = stitch_set_names[s]
			ssf = stitch_set_fourcc[s]
			
			StitchVerts = FetchStitchVerts p_Mesh (ssn+"_VERTS")
			if StitchVerts.count > 0 then (
			
				remappedstitchverts = #()
				
				for v in StitchVerts do (
				
					pos = finditem g_VertLookup v
					if pos > 0 then (
						append remappedstitchverts (pos-1)
					)
					
				)
						
				if (remappedstitchverts.count > 0) then (
					copy remappedstitchverts
					append g_StitchSets #(ssf,remappedstitchverts)
				)
			)
		)
		
		WriteStitchSets()

	)
		
	if (p_ProgBar != undefined) then p_ProgBar.value = 100
	
	WriteRestPose()
	
	WriteBoundingBoxes()
		
	WriteEndOfMesh()
	
	WriteMeshInfoChunk()

	EnterShowSelectedFacesMode p_Mesh
	subObjectLevel = 0
	ExitShowSelectedFacesMode p_Mesh
	
	RestoreSelectedFaces p_Mesh
	
	setarrowcursor()
	
	slidertime = savedslidertime 
	
	g_BlastErrorCode = BLAST_NO_ERROR
	
	return true
	
)

*/

/*
GLOBAL g_File
GLOBAL g_FName
GLOBAL g_Mesh
GLOBAL g_Corners
GLOBAL g_Tris
GLOBAL g_StrippedTris
GLOBAL g_Verts
GLOBAL g_VertLookup
GLOBAL g_VertMapping
GLOBAL g_VertexAlpha

GLOBAL g_BlastErrorCode

GLOBAL g_BoneCornerWeights
GLOBAL g_CornerBoneWeights
GLOBAL g_BoneVertexWeights 
GLOBAL g_VertexBoneWeights 
GLOBAL g_VertexBoneOffsets

GLOBAL g_BoneList
GLOBAL g_BoneParentList
GLOBAL g_RigidBoneID 		-- Bone ID that is to be used as the only transform for RIGID objects

GLOBAL g_StringTable
GLOBAL g_StringTableSize
GLOBAL g_StringTablePos
GLOBAL g_VertNorms
GLOBAL g_MeshAttributeFlags

GLOBAL g_NumberOfSubMeshes

GLOBAL g_StitchSets

GLOBAL g_RootAdjustedForYIsUp 

GLOBAL MESH_MAJOR_VERSION
GLOBAL MESH_MINOR_VERSION

-- Revision history
-- version 2.5 added convert NECK tools to full STITCH tools
-- version 2.4 added tests for dupes and degenerates
-- version 2.3 added RunLengths for sub-groups
-- version 2.2 added StartIndexes for sub-groups
-- version 2.1 added BBoxes
-- version 2.0 converted to Y is UP, Z is FACING FORWARD
-- version 1.5 added Bone Offsets
-- version 1.4 added Vert Stitching
-- version 1.3 added WeightedCorners
-- version 1.2 added VertBoneWeights
-- version 1.1
-- version 1.0 added sub mesh support
-- version 0.4 added multi texture support
-- version 0.3 added selection set support
-- version 0.2 added mesh attributes
MESH_MAJOR_VERSION	= 2 			
MESH_MINOR_VERSION	= 5
	
MAX_MESH_VARIATIONS = 3		-- HEAD/HANDS/FEET

filein "nema\\stitchtools.ms"
filein "nema\\nematools.ms"
filein "nema\\cornerstuff.ms"
filein "nema\\normstuff.ms"
filein "nema\\selectsettools.ms"
filein "nema\\bboxtools.ms"

--*************************************************************
fn WriteMeshHeaderChunk = (

	WriteChunkID "0x48534D42" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BMSH
	
	WriteBinInt g_File g_StringTableSize
	WriteBinInt g_File g_BoneList.count
	WriteBinInt g_File g_NumberOfMaterials
	WriteBinInt g_File g_Mesh.numverts
	WriteBinInt g_File g_NumberOfSubMeshes 
	WriteBinInt g_File g_MeshAttributeFlags
	
)

--*************************************************************
fn WriteSubMeshHeader variation = (
	
	-- here we can encode some info in the variationID		
	SubMeshID = variation-1
	
	WriteChunkID "0x42555342" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BSUB
	WriteBinInt g_File SubMeshID
	
	WriteBinInt g_File g_NumberOfMaterials  -- ???
	WriteBinInt g_File g_VertLookup.count
	WriteBinInt g_File g_Corners.count
	WriteBinInt g_File g_Tris.count

)

--*************************************************************
fn WriteSubMeshMaterialChunk = (
			
	WriteChunkID "0x4D4D5342" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BSMM
	
	local totalused = 0
	
	for tset = 1 to g_NumberOfMaterials do (
		if (g_SubMeshMaterialSets[tset].count > 0) then (
			totalused += 1
		)
	)
	
	WriteBinInt g_File totalused 	-- Write out the number of materials used by this sub-mesh
	
	for tset = 1 to g_NumberOfMaterials do (
		if g_SubMeshMaterialSets[tset].count == 0 then continue -- Skip the material we don't care for
		WriteBinInt g_File (tset-1) -- watch out for the Off-By-0ne
		WriteBinInt g_File g_SubMeshMaterialSets[tset].count
	)
	
)
		
--*************************************************************
fn WriteMeshInfoChunk = (
			
		WriteBinInt  g_File (doaStringToInt "0x4f464e49" "%x")	-- INFO
			
		WriteBinInt g_File 6

		WriteString ("USER="+(doaGetUserName() as string))
		WriteString ("HOST="+(doaGetHostName() as string))
		WriteString ("MAXFILE="+maxFilePath+maxFileName)
		WriteString ("PRSFILE="+g_FName)
		WriteString ("VERSION="+(MESH_MAJOR_VERSION as string) + "." + (MESH_MINOR_VERSION as string))
		WriteString ("CREATED="+localTime)
		
)

--*************************************************************
fn WriteBoneInfo = (

	WriteChunkID "0x484E4F42" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BONH
	
	for curr_i = 1 to g_BoneList.count do (
	
		b = g_BoneList[curr_i]	
		bp = FetchValidParent b
		
		parent_i = finditem g_BoneList bp
		
		if (parent_i == 0) then (
			if g_Verbose then (MessageBox ("Invalid parent index p:" + (parent_i) as string + " k:" + (bp as string) + "\nExport will not be valid"))
		)
		
		WriteBinInt g_File (curr_i-1)			-- Don't get all off-by-one	
		
		WriteBinInt g_File (parent_i-1)
		WriteBinInt g_File g_StringTablePos[curr_i+g_NumberOfMaterials]
	)
)

--*************************************************************
fn WriteBoneWeightList = (

	-- Write out the weights sorted by BONE and indexed to VERTEX
	
	WriteChunkID "0x4C575642" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BVWL
	
	-- If p_NumBoneWeights is negative, then it will be interpreted as a flag
	-- currently a value of -1 means that the bone will have all the verts rigidly linked to it
	
	for curr_i = 1 to g_BoneList.count do (
	
		if (g_BoneVertexWeights.count == 0) then (
			
			if g_RigidBoneID == curr_i then (
				WriteBinInt g_File -1		-- This is the bone that all the verts are attached to
			) else (
				WriteBinInt g_File 0
			)
			
		) else (
		
			WriteBinInt g_File g_BoneVertexWeights[curr_i].count
	
			for bvw in g_BoneVertexWeights[curr_i] do (
				WriteBinInt   g_File (bvw[1]-1)	-- vert index (accounting for the off-by-one)
				WriteBinFloat g_File bvw[2]		-- weight
			)
		)

	)
	
)

	
--*************************************************************
fn WriteVertexWeightList = (

	-- Write out the weights sorted by VERTEX and indexed to BONE
	
	WriteChunkID "0x4C574256" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- VBWL

	if g_VertexBoneWeights.count == 0 then (
	
		WriteBinInt g_File -(g_RigidBoneID-1)	-- A zero or negative value flags a rigid object with the 
												-- actual bone index used derived as (-val)
	) else (
	
		WriteBinInt g_File g_VertList.count
	
		for curr_i = 1 to g_VertList.count do (
		
			WriteBinInt g_File g_VertexBoneWeights[curr_i].count
			
			for vbw in g_VertexBoneWeights[curr_i] do (
				WriteBinFloat g_File vbw[2]		-- weight
				WriteBinInt   g_File (vbw[1]-1)	-- bone index (accounting for the off-by-one)
			)
		
		)
	)
	
)

--*************************************************************
fn WriteVertexData = (

	WriteChunkID "0x58545642" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BVTX
	WriteBinInt g_File g_VertLookup.count
	
	for v in g_VertLookup do (

		-- Are we deformable?
		if (g_mesh.modifiers[#Physique] != undefined) then (
			vert = (getvert g_Mesh v) - g_RootPosition 
			vert = vert * (RotateXMatrix -90)
		) else (
			vert = (getvert g_Mesh v) - g_Mesh.transform.translation 
			vert = vert * (inverse g_Mesh.transform.rotationpart)
		)

		WriteBinFloat g_File (vert.x / 1000.0)
		WriteBinFloat g_File (vert.y / 1000.0)
		WriteBinFloat g_File (vert.z / 1000.0)
		
	)

)

--*************************************************************
fn WriteVertexMapping = (

	WriteChunkID "0x504D5642" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BVMP
	
	for vmaplist in g_VertMapping do (
		WriteBinInt g_File vmaplist.count
		for vmapindex in vmaplist do (
			WriteBinInt g_File (vmapindex-1)
		)
	)
)


--*************************************************************
fn WriteWeightedCornerData = (

	WriteChunkID "0x4e524357" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- WCRN
	
	WriteBinInt g_File g_Corners.count
	
	for i = 1 to g_Corners.count do (
	
		corn = g_Corners[i]
		
		vi = corn[1]
		nm = corn[2]
		uv = corn[3]
		cl = corn[4]
		
		mv = g_VertLookup[vi]
		
		-- Are we deformable?
		if g_mesh.modifiers[#Physique] != undefined then (
			if Not g_RootAdjustedForYIsUp then (
				in coordsys (g_RootTransform * (RotateXMatrix 90)) vert = getvert g_Mesh mv
			) else (
				in coordsys (g_RootTransform ) vert = getvert g_Mesh mv
			)
		) else (
			in coordsys local vert = getvert g_Mesh mv
		)
		
		WriteBinFloat g_File (vert.x / 1000.0)
		WriteBinFloat g_File (vert.y / 1000.0)
		WriteBinFloat g_File (vert.z / 1000.0)
		
		if (g_VertexBoneWeights.count == 0) then (
		
			WriteBinFloat g_File 1.0
			WriteBinFloat g_File 0.0
			WriteBinFloat g_File 0.0
			WriteBinFloat g_File 0.0
			WriteBinByte g_File 1
			WriteBinByte g_File 0
			WriteBinByte g_File 0
			WriteBinByte g_File 0
			
		) else (

			if g_VertexBoneWeights[vi].count < 5 then (
				sum_count = g_VertexBoneWeights[vi].count
			) else (
				sum_count = 4
			)
			
			for t = 1 to sum_count do (
				WriteBinFloat g_File g_VertexBoneWeights[vi][t][2]
			)
			for t = (sum_count+1) to 4 do (
				WriteBinFloat g_File 0.0
			)
			
			for t = 1 to sum_count do (
				WriteBinByte g_File g_VertexBoneWeights[vi][t][1]
			)
			for t = (sum_count+1) to 4 do (
				WriteBinByte g_File 0
			)
		
		)
					
		WriteBinFloat g_File nm.x
		WriteBinFloat g_File nm.y
		WriteBinFloat g_File nm.z
		
		if g_VertexAlpha then (
			WriteBinByte g_File (cl.blue  as integer)
			WriteBinByte g_File (cl.green as integer)
			WriteBinByte g_File (cl.red   as integer)
			if (uv.z < 0)      then (alpha = 255)
			else if (uv.z >= 1) then (alpha = 0)
			else 				    (alpha = (1-uv.z) * 255)
			WriteBinByte g_File (alpha as integer) 
		) else (
			WriteBinByte g_File (cl.blue  as integer)
			WriteBinByte g_File (cl.green as integer)
			WriteBinByte g_File (cl.red   as integer)
			WriteBinByte g_File (doaStringToInt "0xff" "%x")
		)
		
		u = uv.x		
		v = uv.y
		 
		WriteBinFloat g_File u
		WriteBinFloat g_File v

	)
	
)

--*************************************************************
fn WriteCornerData = (

	WriteChunkID "0x4e524342" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BCRN
	
	WriteBinInt g_File g_Corners.count
	
	for i = 1 to g_Corners.count do (
	
		corn = g_Corners[i]
		
		vi = corn[1]
		nm = corn[2]
		uv = corn[3]
		cl = corn[4]
		
		WriteBinInt   g_File (vi-1)		-- watch out for the Off-By-0ne
		
		WriteBinFloat g_File nm.x
		WriteBinFloat g_File nm.y
		WriteBinFloat g_File nm.z
		
		if g_VertexAlpha then (
			WriteBinByte g_File (doaStringToInt "0xff" "%x")
			WriteBinByte g_File (doaStringToInt "0xff" "%x")
			WriteBinByte g_File (doaStringToInt "0xff" "%x")
			WriteBinByte g_File (cl.red   as integer) 
		) else (
			WriteBinByte g_File (cl.blue  as integer)
			WriteBinByte g_File (cl.green as integer)
			WriteBinByte g_File (cl.red   as integer)
			WriteBinByte g_File (doaStringToInt "0xff" "%x")
		)
		
		WriteBinInt   g_file 0		-- no specular color written just yet
		
		u = uv.x		
		v = uv.y
		 
		WriteBinFloat g_File u
		WriteBinFloat g_File v

	)
	
)

--*************************************************************
fn WriteVertexBoneOffsetList = (

	-- Write out the weights AND OFFSETS sorted by VERTEX and indexed to BONE
	
	WriteBinInt g_File g_Corners.count
	
	for i = 1 to g_Corners.count do (
	
		corn = g_Corners[i]
		
		vi = corn[1]
		nm = corn[2]
		uv = corn[3]
		cl = corn[4]
		
		mv = g_VertLookup[vi]
		
		if (g_VertexBoneWeights.count == 0) then (
		
			WriteBinByte g_File 0
			
		) else (

			if g_VertexBoneWeights[vi].count < 5 then (
				sum_count = g_VertexBoneWeights[vi].count
			) else (
				sum_count = 4
			)
			
			WriteBinInt g_File sum_count
			
			for t = 1 to sum_count do (
				WriteBinFloat g_File g_VertexBoneOffset[vi][t][2]
				WriteBinFloat g_File g_VertexBoneOffset[vi][t][3].x
				WriteBinFloat g_File g_VertexBoneOffset[vi][t][3].y
				WriteBinFloat g_File g_VertexBoneOffset[vi][t][3].z
			)
						
			for t = 1 to sum_count do (
				WriteBinInt g_File g_VertexBoneWeights[vi][t][1]
			)
		
		)
			
	)
		
)

--*************************************************************
fn WriteTriangleData p_NegativeParity = (

	WriteChunkID "0x49525442" MESH_MAJOR_VERSION MESH_MINOR_VERSION -- BTRI
	
	WriteBinInt g_File g_Tris.count
	
	local startindices = #()
	local runlengths = #()
	
	for tlist in g_StrippedTris do 
	(
		if tlist.count > 0 then
		(
			-- Find the starting index for this set of tris
			local startindex = 10000000
			local finalindex = 0
			for tri in tlist do
			(
				if tri[1] < startindex then startindex = tri[1]
				if tri[2] < startindex then startindex = tri[2]
				if tri[3] < startindex then startindex = tri[3]
				
				if tri[1] > finalindex then finalindex = tri[1]
				if tri[2] > finalindex then finalindex = tri[2]
				if tri[3] > finalindex then finalindex = tri[3]
			)
			
			if (finalindex>g_Corners.count) then (
				g_BlastErrorCode = BLAST_ERROR_BAD_OPTIMIZE
				if g_Verbose then (
						MessageBox (
						"Failed to export properly!\n\n" +
						"The mesh optimizer has failed. Please remove all dupes and spikes")title:"Bad News"
				)
				format "ERROR: Failed to export! The optimizer failed. Please STL check and remove double faces\n"  tris_done g_Tris.count
				return false;
			)
		
			WriteBinInt g_File (startindex-1)
			WriteBinInt g_File (finalindex+1-startindex)
			append startindices startindex
			append runlengths (finalindex+1-startindex)
		)
		else
		(
			append startindices 0
			append runlengths 0
		)
	)
	
	
format "tricount %\n" g_Tris.count
format "starts % lengths %\n" startindices runlengths
	
	local tris_done = 0
	local drawn_last = 0
	local drawn_last_total = 0
	
	DrawLastBA = #{}
	if (g_Mesh.faces[#drawlast] != undefined) then 
	(
		for f in g_Mesh.faces[#drawlast] do 
		(
			append DrawLastBA f.index
			drawn_last_total +=1
		)
	)
	
	if (DrawLastBA.count == 0) then
	(
		for t = 1 to g_StrippedTris.count do 
		(	
			tlist = g_StrippedTris[t]
			startindex = startindices[t]
			
			for tri in tlist do
			(
				if (p_NegativeParity) then 
				(
					WriteBinInt g_File (tri[1]-startindex)
					WriteBinInt g_File (tri[3]-startindex)
					WriteBinInt g_File (tri[2]-startindex)
				)
				else
				(
					WriteBinInt g_File (tri[1]-startindex)
					WriteBinInt g_File (tri[2]-startindex)
					WriteBinInt g_File (tri[3]-startindex)
				)
				tris_done += 1
			)
		)
	) 
	else
	(
		
		if (p_NegativeParity) then (
		
			for sset in g_SubMeshMaterialSets do (
			
				WriteBinInt g_File startindex-1
			
				for t in sset do (
					if DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					tris_done += 1
				)
				for t in sset do (
					if not DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					tris_done += 1
					drawn_last += 1
				)
			)
		)
		else
		(

			for sset in g_SubMeshMaterialSets do (
		
				WriteBinInt g_File startindex-1
			
				for t in sset do (
					if DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					tris_done += 1
				)
				for t in sset do (
					if not DrawLastBA[t] then continue
					WriteBinInt g_File ((g_Tris[t][1])-1)
					WriteBinInt g_File ((g_Tris[t][2])-1)
					WriteBinInt g_File ((g_Tris[t][3])-1)
					drawn_last += 1
					tris_done += 1
				)
			)
		)
	)
	
	
	if (g_Tris.count == tris_done and drawn_last == drawn_last_total) then (
		return true
	) else (
		g_BlastErrorCode = BLAST_ERROR_BAD_MATERIALS
		format "tris done = % tris expected = %\n" tris_done g_Tris.count
		format "drawn last = % drawn last expected = %\n" drawn_last drawn_last_total
		if g_Verbose then (
				MessageBox (
				"Failed to export properly!\n\n" +
				"Are you SURE all the materials you are using are valid AND MAPPED TO THE SHADOW?\n\n" +
				"You can be reasonably sure that your texures are OK if they pass the Verification Toolkit\n\n" +
				"Please check everything twice before you start blaming the exporter =)") title:"Bad News"
		)
		format "ERROR: Failed to export! wrote % tris, needed to write %. Does the object pass the material verifier? Are all materials mapped to the shadow?\n"  tris_done g_Tris.count
	)
		
	return false
	
)

--*************************************************************
fn FetchObjectTransform b t = (

	at time t xfrm = b.transform

	return xfrm
)

--*************************************************************
fn WriteRestPose  = (

	WriteChunkID "0x534f5052" MESH_MAJOR_VERSION MESH_MINOR_VERSION  -- RPOS
	WriteBinInt g_File g_BoneList.count
	
	for curr_i = 1 to g_BoneList.count do ( -- at time 0 (
	
		b = g_BoneList[curr_i]	
	
		objxfrm = FetchObjectTransform b slidertime 
		
		bonerot = objxfrm.rotationpart
		bonepos = objxfrm.translation
		
		bonepos = (bonepos-g_RootPosition) * (inverse g_RootRotation)
		bonerot =  (g_RootRotation) * (inverse bonerot)
		
		if not g_RootAdjustedForYIsUp then (
			bonepos = bonepos * ((eulerangles -90 0 0) as quat)
			bonerot = ((eulerangles 90 0 0) as quat) * bonerot 
		)
						
		invbonerot = (inverse bonerot)		
		invbonepos = ((-bonepos) * (bonerot))
		
		-- const vector_3 pos = posepos + poserot.RotateVector(invrestpos);
		testrot = bonerot * invbonerot 
		testpos = bonepos + (invbonepos * testrot)
				
		invbonepos = invbonepos / 1000
	
		WriteBinFloat g_File invbonerot.x
		WriteBinFloat g_File invbonerot.y
		WriteBinFloat g_File invbonerot.z
		WriteBinFloat g_File invbonerot.w
		
		WriteBinFloat g_File invbonepos.x
		WriteBinFloat g_File invbonepos.y
		WriteBinFloat g_File invbonepos.z

		if (curr_i == 1) then (
			
			objxfrm = FetchObjectTransform b slidertime 
			kidrot = objxfrm.rotationpart
			kidpos = objxfrm.translation
			
			relrestpos = (kidpos-g_RootPosition) * (inverse g_RootRotation)
			relrestrot =  (g_RootRotation) * (inverse kidrot)

			if not g_RootAdjustedForYIsUp then (
				-- At the root, we need to account for 'Y is UP, Z is forward'
				relrestpos = relrestpos * ((eulerangles -90 0 0) as quat)
				relrestrot = ((eulerangles 90 0 0) as quat) * relrestrot 
			)

		) else (
			
			parxfrm = FetchObjectTransform (FetchValidParent b) slidertime 
			parentrot = parxfrm.rotationpart
			parentpos = parxfrm.translation
			
			kidxfrm = FetchObjectTransform b slidertime 
			kidrot = kidxfrm.rotationpart
			kidpos = kidxfrm.translation
			
			relrestpos =  (kidpos-parentpos) * inverse parentrot 
			relrestrot =  (parentrot) * inverse kidrot
			
		)
			
		relrestpos = relrestpos / 1000.0
		
		WriteBinFloat g_File relrestrot.x
		WriteBinFloat g_File relrestrot.y
		WriteBinFloat g_File relrestrot.z
		WriteBinFloat g_File relrestrot.w
		
		WriteBinFloat g_File relrestpos.x
		WriteBinFloat g_File relrestpos.y
		WriteBinFloat g_File relrestpos.z
		
	)
		
)

--*************************************************************
fn WriteStitchSets = (

	WriteFourCCChunkID "STCH" MESH_MAJOR_VERSION MESH_MINOR_VERSION  
	
	WriteBinInt  g_File g_StitchSets.count
	for sset in g_StitchSets do (
		WriteBinInt g_File (doaFourCCToInt sset[1])
		WriteBinInt g_File (sset[2].count)
		for v = 1 to sset[2].count do (
			WriteBinInt g_File sset[2][v]
		)
	) 
	
)

--*************************************************************
fn WriteBoundingBoxes = (

	local boxes = CollectBBoxes g_Mesh
	
	WriteFourCCChunkID "BBOX" MESH_MAJOR_VERSION MESH_MINOR_VERSION  
	
	WriteBinInt g_File boxes.count
	
	for b in boxes do (
	
		i = findItem g_BoxNames b.name
		
		WriteBinInt g_File (doaFourCCToInt g_BoxFourCC[i])
		
		objxfrm = FetchObjectTransform b 0
		
		boxpos = objxfrm.translation
		boxrot = objxfrm.rotationpart
		
		boxpos = (boxpos-g_RootPosition) * (inverse g_RootRotation)
		boxrot = (g_RootRotation) * (inverse boxrot)
	
		if not g_RootAdjustedForYIsUp then (
			boxpos = boxpos * ((eulerangles -90 0 0) as quat)
			boxrot = ((eulerangles 90 0 0) as quat) * boxrot
		)
		
		boxpos = boxpos / 1000
		halfdiag = [b.width,b.length,b.height] / (2*1000)

		WriteBinFloat g_File boxpos.x
		WriteBinFloat g_File boxpos.y
		WriteBinFloat g_File boxpos.z
		
		WriteBinFloat g_File boxrot.x
		WriteBinFloat g_File boxrot.y
		WriteBinFloat g_File boxrot.z
		WriteBinFloat g_File boxrot.w
		
		WriteBinFloat g_File halfdiag.x
		WriteBinFloat g_File halfdiag.y
		WriteBinFloat g_File halfdiag.z
		
	) 
	
)

--*************************************************************
fn WriteEndOfMesh = (

	WriteBinInt  g_File (doaStringToInt "0x444E4542" "%x")	-- BEND
	
)

--*************************************************************

fn WriteMesh p_Mesh p_FName p_TopBone p_MeshFlags p_ProgBar = (

	setWaitCursor () 
	
	g_Mesh = p_Mesh
	g_FName = p_FName

	if p_Mesh.modifiers[#skin] != undefined then (
		select p_Mesh
		max modify mode
	)
	
	savedslidertime = slidertime
	slidertime = animationrange.start

	-- For ver 2.0 we will correct for YisUP at source
	DetermineRootAdjustment animationrange.start
	
	SaveSelectedFaces p_Mesh

	g_MeshAttributeFlags = p_MeshFlags 	
	
	if (p_ProgBar != undefined) then p_ProgBar.value = 0
	
	BuildBoneList p_TopBone
	
	if (g_BoneList.count == 0) then (
	
		slidertime = savedslidertime 
	
		if (g_Mesh.parent == $bip01) then (
			if g_Verbose then (
				MessageBox "The SKINMESH object is attached to the BIPO1 and not the BIP01_PELVIS" title:"Bad News"
			)
			format "ERROR: The SKINMESH object is attached to the BIPO1 and not the BIP01_PELVIS\n"
		) else (
			if g_Verbose then (
				MessageBox "You need at least one bone attached to the object\n\nPerhaps you are missing a 'Dummy Root'?" title:"Bad News"
			)
			format "ERROR: There is no bone attached to the object. Perhaps you are missing a 'Dummy Root'?\n"
		)
			
		g_BlastErrorCode = BLAST_ERROR_BAD_BONES
		return false
	)
	
	
	-- g_NumberOfMeshVariations = CountMeshVariations p_Mesh
	
	InitializeStringTable()
	
	-- The texture file names are the first strings in the table
	for t in g_TextureNameList do (
		AddStringToStringTable t
	)

	BuildBoneStringTable()

	g_NumberOfSubMeshes = CountMeshVariations p_Mesh

	if (p_ProgBar != undefined) then p_ProgBar.value = 0	
	
	WriteMeshHeaderChunk()

	WriteStringTable()
	
	WriteBoneInfo()
	
	BuildVertNormals()

	-- For each mesh variation
	
	for v = 0 to MAX_MESH_VARIATIONS do (
	
		if not (ValidSubMesh v) then (
			continue
		)
		
		SelectFacesInSubMesh p_Mesh v
		local subs_ok = BuildSubMeshMaterialSets p_Mesh		-- partition the faces of the sub mesh by materials
		
		if (not subs_ok) then (
		
			EnterShowSelectedFacesMode p_Mesh
			subObjectLevel = 0
			ExitShowSelectedFacesMode p_Mesh
			RestoreSelectedFaces p_Mesh
			setarrowcursor()
	
			slidertime = savedslidertime 
			if g_Verbose then (MessageBox "Failed to locate any faces with one of the materials\nAre you sure your materials are valid?" title:"Bad News")
			else (format "ERROR: Failed to export properly! Failed to locate any faces with one of the materials\n") 
			g_BlastErrorCode = BLAST_ERROR_BAD_MATERIALS
			return false
		)
		
		prog_scale = (v+1)*1.0/(MAX_MESH_VARIATIONS+1)
	
		BuildTriangleCorners p_ProgBar prog_scale
		
		g_StrippedTris = #()
		
		for sset in g_SubMeshMaterialSets do 
		(
			local submesh = #()
			for t in sset do
			(
				append submesh g_Tris[t]	
			)
			append g_StrippedTris submesh
		)

		stripped_ok = doastripify g_StrippedTris g_corners g_vertmapping
		if not stripped_ok then (
			if (g_Verbose) then
			(
				MessageBox "You need at least one bone attached to the object\n\nPerhaps you are missing a 'Dummy Root'?" title:"Bad News"
			)
			format "ERROR: There is no bone attached to the object. Perhaps you are missing a 'Dummy Root'?\n"
			g_BlastErrorCode = BLAST_ERROR_MISSING_MESH	
			return false
		)
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 80 * prog_scale
		
		BuildVertexWeights()
	
		WriteSubMeshHeader v
		
		WriteSubMeshMaterialChunk()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 84 * prog_scale	
		
		WriteVertexData()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 90 * prog_scale
		
		WriteCornerData()
		
		NormalizeVertexWeights()
		
		WriteWeightedCornerData()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 94 * prog_scale
		
		WriteVertexMapping()
		
		if (p_ProgBar != undefined) then p_ProgBar.value = 96 * prog_scale	
		
		EnterShowSelectedFacesMode p_Mesh
		triswritten = WriteTriangleData (HasNegativeParity g_Mesh.transform)
		ExitShowSelectedFacesMode p_Mesh
		
		if (not triswritten) then (
			slidertime = savedslidertime 
			EnterShowSelectedFacesMode p_Mesh
			subObjectLevel = 0
			ExitShowSelectedFacesMode p_Mesh
			RestoreSelectedFaces p_Mesh
			setarrowcursor()
			return false
		)
		
		WriteBoneWeightList()
		-- WriteVertexBoneOffsetList()
		
		-- Collect info about vert stitch sets of this particular sub mesh
		g_StitchSets = #()
			
		for s = 1 to stitch_set_names.count do
		(
			ssn = stitch_set_names[s]
			ssf = stitch_set_fourcc[s]
			
			StitchVerts = FetchStitchVerts p_Mesh (ssn+"_VERTS")
			if StitchVerts.count > 0 then (
			
				remappedstitchverts = #()
				
				for v in StitchVerts do (
				
					pos = finditem g_VertLookup v
					if pos > 0 then (
						append remappedstitchverts (pos-1)
					)
					
				)
						
				if (remappedstitchverts.count > 0) then (
					copy remappedstitchverts
					append g_StitchSets #(ssf,remappedstitchverts)
				)
			)
		)
		
		WriteStitchSets()

	)
		
	if (p_ProgBar != undefined) then p_ProgBar.value = 100
	
	WriteRestPose()
	
	WriteBoundingBoxes()
		
	WriteEndOfMesh()
	
	WriteMeshInfoChunk()

	EnterShowSelectedFacesMode p_Mesh
	subObjectLevel = 0
	ExitShowSelectedFacesMode p_Mesh
	
	RestoreSelectedFaces p_Mesh
	
	setarrowcursor()
	
	slidertime = savedslidertime 
	
	g_BlastErrorCode = BLAST_NO_ERROR
	
	return true
	
)

*/
