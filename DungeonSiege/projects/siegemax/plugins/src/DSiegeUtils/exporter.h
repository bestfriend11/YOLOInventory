/**********************************************************************
 *<
	FILE: exporter.h

	DESCRIPTION:	Template Utility

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef __EXPORTER__H

#define __EXPORTER__H
#include "IgnoredWarnings.h"

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "smart_pointer.h"

#ifndef _UINT32
#define _UINT32
typedef unsigned int UINT32;
typedef unsigned short UINT16;
#endif

#include <fstream>

#include <list>
#include <vector>

extern TCHAR *GetString(int id);

extern HINSTANCE hInstance;

// The unique ClassID
#define SIEGE_NODE_EXPORTER_CLASS_ID	Class_ID(0x30805d77, 0x52302c66)

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

//========================================================================

class Spot {

public:
	Spot(const std::string& n, const Matrix3& tm) 
		: d_name(n)
		, d_transMatrix(tm)
	{
	};

	Matrix3 d_transMatrix;
	std::string d_name;
};

typedef smart_pointer<Spot> SmartSpot;
typedef std::vector<SmartSpot> SmartSpotArray;
typedef SmartSpotArray::iterator SmartSpotArrayIter;

//========================================================================

class SiegeNodeExportEnum : public ITreeEnumProc {
	
private:

	smart_pointer<std::ofstream> d_outstream;
	Interface *d_interface;

//	std::vector<int> d_materialMapping;

	std::list<Texmap *> d_materialTextures;
	std::list<Color> d_materialDiffuseColours;

	std::vector<int> d_materialMapping;

	std::vector<bool> d_matUsed;

	std::vector<Point2> d_uvValues;
	std::vector<UINT32> d_uvLookup;

	std::vector<UINT32>	d_vertValues;
	std::vector<int>	d_vertLookup;
	int d_actualvertcount;

	std::vector<UINT32>	d_faceValues;
	std::vector<int>	d_faceLookup;
	int d_actualfacecount;
	
	int d_matsActuallyUsed;
	int d_texturesActuallyUsed;

	std::vector<UINT32> d_materialFaceCount;
	   
	std::string d_exportfilename;

	SmartSpotArray d_spotArray;

public:

	int d_exported_count;

	void SiegeNodeExportEnum::SpotEnum(const Matrix3& vertTM, INode* node);

	explicit SiegeNodeExportEnum(TSTR fname,Interface *maxIFace);
	~SiegeNodeExportEnum();

	void EnumerateMaterial(Mtl *Material );
	void EnumerateMaterialTree(Mtl *RootMaterial);
	void ExportMaterials(void);
	bool DumpOutTheTexture(Texmap *ActiveTexture);
	void WriteTexture(Bitmap &MAXBitmap,
					  bool const TextureMapHasAlpha);

	int callback(INode *node);
	
};

class SiegeNodeExporter : public SceneExport {
public:

	//Constructor/Destructor
	SiegeNodeExporter();
	~SiegeNodeExporter();

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);	// Export file

};



// This is the Class Descriptor for the SiegeNodeExporter plug-in
class SiegeNodeExporterClassDesc:public ClassDesc {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading = FALSE) {return new SiegeNodeExporter();}
	const TCHAR *	ClassName() {return GetString(IDS_CLASS_NAME);}
	SClass_ID		SuperClassID() {return SCENE_EXPORT_CLASS_ID;}
	Class_ID		ClassID() {return SIEGE_NODE_EXPORTER_CLASS_ID;}
	const TCHAR* 	Category() {return GetString(IDS_CATEGORY);}
	void			ResetClassParams (BOOL fileReset);
};

#endif // __EXPORTER__H
