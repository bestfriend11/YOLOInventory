#pragma once
/**********************************************************************
 *<
	FILE: SNOExp.cpp

	DESCRIPTION: SiegeNode Exporter

	CREATED BY: biddle

	HISTORY: 

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#ifndef _SNOEXP_H
#define _SNOEXP_H

#include <gpcore.h>
#include <list>
#include <vector>
#include "DSiegeUtils.h"
#include "ExpBase.h"

//========================================================================
class SNOExporter : public ExpBase
{
	
public:

	explicit SNOExporter(const gpstring& maxfname, Interface* iface);
	~SNOExporter();

	void EnumerateMaterial(Mtl *Material );
	void EnumerateMaterialTree(Mtl *RootMaterial);
	void ExportMaterials(void);
	bool DumpOutTheTexture(Texmap *ActiveTexture);

	void AppendData(const char* buf, int count);

	bool FetchTextureFiles(std::vector<gpstring>& filenames);
	
	DSiegeUtils::eExpErrType ExportINode(INode *node, const gpstring& expfname, FileSys::File& outfile);

	bool VerifyRequiredGlobals(void);
	void Dump(void);

private:

	std::vector<char>	m_outbuff;

	std::list<Texmap *> m_materialTextures;
	std::list<Color>	m_materialDiffuseColours;

	std::vector<int>	m_materialMapping;

	std::vector<bool>	m_matUsed;

	std::vector<Point2> m_uvValues;
	std::vector<UINT32> m_uvLookup;

	std::vector<UINT32>	m_vertValues;
	std::vector<int>	m_vertLookup;
	int m_actualvertcount;

	std::vector<UINT32>	m_faceValues;
	std::vector<int>	m_faceLookup;
	int m_actualfacecount;
	
	int m_matsActuallyUsed;
	int m_texturesActuallyUsed;

	std::vector<UINT32> m_materialFaceCount;
	
};

//========================================================================

class SiegeNodeIO_header
{
public:

	UINT32 MagicValue;			// 0
	UINT32 Version;				// 1

	// Boundary Mesh info
	UINT32 NumVertices;			// 2
	UINT32 NumTextureVerts;		// 3
	UINT32 NumColourVerts;		// 4
	UINT32 NumNormals;			// 5
	UINT32 NumMaterials;		// 7
	UINT32 NumFaces;			// 6
	UINT32 NumDoors;			// 8
	UINT32 NumFloorFaces;		// 9
	UINT32 NumWaterFaces;		// 10
	UINT32 WireframeColour;		// 11
	UINT32 NumTextureMaps;		// 12
	UINT32 MinorVersion;		// 13
	UINT32 NumSpots;			// 14
	
	UINT32 Reserved[16-14];		// 15

};

#endif