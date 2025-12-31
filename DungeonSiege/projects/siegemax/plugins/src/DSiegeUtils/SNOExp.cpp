/**********************************************************************
 *<
	FILE: SNOExp.cpp

	DESCRIPTION: Dungeon Siege Exporter

	CREATED BY: biddle

	HISTORY: 

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#include <gpcore.h>

#include "SNOExp.h"
#include "stdmat.h"
#include "modstack.h"
#include "CustAttrib.h"
#include "ICustAttribContainer.h"
#include "VertNormals.h"
#include "PostProcess.h"
#include <Winsock2.h>

#include <config.h>

//******************************************************
inline void SwizzleToYisUp(const Matrix3& in_mat, Matrix3& out_mat)
{
	if (in_mat == out_mat)
	{
		Matrix3 temp(false);
		temp.SetColumn(0, -in_mat.GetColumn(0) );
		temp.SetColumn(1,  in_mat.GetColumn(2) );
		temp.SetColumn(2,  in_mat.GetColumn(1) );
		out_mat = temp;
	}
	else
	{
		out_mat.SetColumn(0, -in_mat.GetColumn(0) );
		out_mat.SetColumn(1,  in_mat.GetColumn(2) );
		out_mat.SetColumn(2,  in_mat.GetColumn(1) );
	}
}

//********************************************************************
// Create a matrix that will transform verts from 3DSMax/Artist space
// where Z is up, characters shit down Y and the millimeter is the
// unit of measure to world space, where Y is up, they shit down Z and
// the unit of choice is the metre.

Matrix3 BuildVertTransformMatrix(INode *sourceMesh) {

	Matrix3 MaxTransform = sourceMesh->GetNodeTM(0); // Assuming T=0;

	SwizzleToYisUp(MaxTransform,MaxTransform);

	MaxTransform.PreScale(Point3(.001f, .001f, .001f), true); 

	/* Let's ignore the pivot point location for now -- biddle

	Matrix3 pivotTM(1);

	pivotTM.SetTrans(sourceMesh->GetObjOffsetPos());
	PreRotateMatrix(pivotTM,sourceMesh->GetObjOffsetRot());
	ApplyScaling(pivotTM,sourceMesh->GetObjOffsetScale());

	Matrix3 vertTM = MaxTransform * pivotTM;

	*/


	return (MaxTransform);

}

//********************************************************************
// Same as the VertTransform, except that we dont apply scale
Matrix3 BuildNormTransformMatrix(INode *sourceMesh) {

	Matrix3 MaxTransform = sourceMesh->GetNodeTM(0); // Assuming T=0;
	
	SwizzleToYisUp(MaxTransform,MaxTransform);

	return (MaxTransform);
	
}


//******************************************************
SNOExporter::SNOExporter(const gpstring& fn, Interface* iface)
	: ExpBase(fn,iface)
{
}

SNOExporter::~SNOExporter()
{
}

inline bool FindInParamTable(IParamBlock2* PBlock,
							 ParamID PID,
							 TimeValue CurrT,
							 int checkIndex)
{
	if (PBlock)
	{
		int totalcheck = PBlock->Count(PID);
		for ( int i = 0; i < totalcheck; ++i)
		{
			if (checkIndex == PBlock->GetInt(PID,CurrT,i)-1)
			{
				return true;
			}
		}
	}
	return false;
}

void SNOExporter::AppendData(const char* buf, int count)
{
	for (int i = 0; i < count; ++i)
	{
		m_outbuff.push_back(buf[i]);
	}
}

bool SNOExporter::FetchTextureFiles(std::vector<gpstring>& tn)
{
	tn.clear();

	int j = m_materialDiffuseColours.size() - m_materialTextures.size();
	for2 (std::list<Texmap *>::iterator i = m_materialTextures.begin();
		i != m_materialTextures.end();
		++i,++j)
	{

		if (m_matUsed[j])
		{
			Texmap *ActiveTexture = (*i); 
			if((ActiveTexture->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00)))
			{
				// It is a bitmap texture - try to get the bitmap
				BitmapTex *ActiveTextureCast = (BitmapTex *)ActiveTexture;

				Bitmap* bm = ActiveTextureCast->GetBitmap(0);

				BitmapInfo* origBI = &bm->Storage()->bi;
				
				const TCHAR* nm = origBI->Name();

				tn.push_back( nm );
			}
		}	
	}

	return true;
}

DSiegeUtils::eExpErrType SNOExporter::ExportINode(INode *thisnode, const gpstring& expfname, FileSys::File& outfile )
{

	TimeValue CurrT(0);
	Interval  CurrInterv = FOREVER;

	m_outbuff.clear();

	// Locate all the SNO parameters in the custom attributes

	IParamBlock2* snoPBlock = 0;
	ParamID snoDoorEdgeListPID = -1;
	ParamID snoDoorLastEdgesPID = -1;
	ParamID snoDoorVertListPID = -1;
	ParamID snoDoorLastVertsPID = -1;
	ParamID snoDoorDirectionListPID = -1;
	ParamID snoFloorFaceListPID = -1;
	ParamID snoWaterFaceListPID = -1;
	ParamID snoIgnoreFaceListPID = -1;
	ParamID snoLockedNormListPID = -1;

	MSPlugin* snomod = FetchSiegeMaxInterface(thisnode,"SNOMODDEF");

	if ( snomod )
	{
		for (int pbi = 0; snomod && (pbi < snomod->pblocks.Count()); pbi++)
		{
			snoPBlock = snomod->pblocks[pbi];
			if (snoPBlock && !stricmp(snoPBlock->GetLocalName(),"SNODATA"))
			{

				// Locate all of the parameter IDs 
				for (int idx = 0; idx < snoPBlock->NumParams() ; idx++ )
				{
					ParamID pid = snoPBlock->IndextoID(idx);
					ParamType2 ptype = snoPBlock->GetParameterType(pid);
					TSTR pname = snoPBlock->GetLocalName(idx);

					if ( pname ) 
					{
						if ( ptype == TYPE_INT_TAB )
						{
							if (!stricmp(pname,"DoorEdgeList"))
							{
								snoDoorEdgeListPID = pid;
							}
							else if (!stricmp(pname,"DoorLastEdges"))
							{
								snoDoorLastEdgesPID = pid;
							}
							else if (!stricmp(pname,"DoorVertList"))
							{
								snoDoorVertListPID = pid;
							}
							else if (!stricmp(pname,"DoorLastVerts"))
							{
								snoDoorLastVertsPID = pid;
							}
							else if (!stricmp(pname,"FloorFaceList"))
							{
								snoFloorFaceListPID = pid;
							}
							else if (!stricmp(pname,"WaterFaceList"))
							{
								snoWaterFaceListPID = pid;
							}
							else if (!stricmp(pname,"IgnoredFaceList"))
							{
								snoIgnoreFaceListPID = pid;
							}
							else if (!stricmp(pname,"LockedNormVertList"))
							{
								snoLockedNormListPID = pid;
							}					
						}
						else if ( (ptype == TYPE_POINT3_TAB) )
						{
							if (!stricmp(pname,"DoorDirectionList"))
							{
								snoDoorDirectionListPID = pid;
							}
						}
					}
				}

				// Could put us a nice error message to let them know 
				// that the Custom Attributes are busted
				if ( snoDoorEdgeListPID == -1 )			return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoDoorLastEdgesPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoDoorVertListPID == -1 )			return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoDoorLastVertsPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoDoorDirectionListPID == -1 )	return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoFloorFaceListPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoDoorLastVertsPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoWaterFaceListPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoIgnoreFaceListPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;
				if ( snoLockedNormListPID == -1 )		return DSiegeUtils::EXP_ERROR_MANGLED_CUSTOM_MODIFIER;

			}
		}
	}
	else
	{
		
	}

	TCHAR* thisnodeactualname = thisnode->GetName();

	Object *o =thisnode->EvalWorldState(CurrT).obj;
	TriObject *tri = (TriObject *)o->ConvertToType(CurrT, triObjectClassID);
	Mesh& realmesh = tri->GetMesh();

	Matrix3 vertTM = BuildVertTransformMatrix( thisnode );
	Matrix3 normTM = BuildNormTransformMatrix( thisnode );

	m_matUsed.clear();
	m_uvValues.clear();
	m_uvLookup.clear();

	m_matsActuallyUsed = 0;
	m_texturesActuallyUsed = 0;
	m_materialFaceCount.clear();


 	//== Setup =============================

	Mtl *MaxMaterial = thisnode->GetMtl();


	// Recursively enumerate the materials list for the selected node
	if(MaxMaterial)
	{
		EnumerateMaterialTree(MaxMaterial);
	}

	// Now determine just which materials are used by the object
	m_matUsed.clear();
	m_matsActuallyUsed = 0;
	m_texturesActuallyUsed = 0;

	// curr_mat runs from -NUM_OF_NONTEXTURED to NUM_OF_TEXTURED-1

	int mtsize = m_materialTextures.size();
	int dcsize = m_materialDiffuseColours.size();

	{for (int curr_mat = mtsize - dcsize; 
			curr_mat < mtsize; ++curr_mat) {

		bool curr_mat_used = false;

		{for (int i = 0; i < realmesh.getNumFaces() ; i++) {

			// Skip this face if it is ignored...
			if ( FindInParamTable(snoPBlock,snoIgnoreFaceListPID,CurrT,i) )
			{
				continue;
			}

			Face& face = realmesh.faces[i];
			
			// Does this face use the current material
			if ( curr_mat == m_materialMapping[face.getMatID()%dcsize] )
			{
				curr_mat_used = true;
				m_matsActuallyUsed++;
				if (curr_mat >= 0) {
					m_texturesActuallyUsed++;
				}
				break;
			}

		}}

		m_matUsed.push_back(curr_mat_used);
	}}

	if (m_materialTextures.size() && !m_matsActuallyUsed)
	{
		char buf[256];
		sprintf(buf,"Multi/Sub material has %d items, of which %d are used",m_materialTextures.size(),m_matsActuallyUsed);
		MessageBox(NULL,buf,"",MB_OK|MB_ICONWARNING);
	}

	if( m_matsActuallyUsed != m_texturesActuallyUsed )
	{
		char textbuf[ 512 ];
		sprintf( textbuf, "SNO [%s] has polygons without texture!", thisnode->GetName() );
		MessageBox( NULL, textbuf, "Ooops!", MB_OK|MB_ICONWARNING );
	}
	
	//== Create a reduce precision version of the UV coords ==

	m_uvLookup.clear();
	m_uvLookup.reserve(realmesh.getNumTVerts());
	m_uvValues.clear();
	m_uvValues.reserve(realmesh.getNumTVerts());

	int total_unique = 0;
	
	{for (int i = 0; i < realmesh.getNumTVerts(); i++) {

		// A texture transformation matrix would be anice thing in here...
		// Point3 pt = VectorTransform(vertTM,realmesh.P(i));
		
		UVVert uv = realmesh.tVerts[i];

		// round the UV coords to the nearest 1/4096th

		Point2 newuv;
		newuv.x = floor((uv.x*4096.0f) + 0.5f)/4096.0f;
		newuv.y = floor((uv.y*4096.0f) + 0.5f)/4096.0f ;

		int pos = 0;
		bool found = false;

		{ for ( std::vector<Point2>::iterator it=m_uvValues.begin();
				it != m_uvValues.end(); ++it) {

			if (*it == newuv) {
				found = true;
				break;
			}
			++pos;
						
		}}

		if (!found) {
			pos = m_uvValues.size();
			m_uvValues.push_back(newuv);
		}

		m_uvLookup.push_back(pos);


	}}

	// Build the vert and face lookup tables
	// so we can remove the ignored faces

	m_vertLookup.clear();
	m_vertValues.clear();

	m_faceLookup.clear();
	m_faceValues.clear();

	// Initialize the vert lookup to 'all ignored'
	{for (int i = 0; i < realmesh.numVerts; i++) {
		m_vertLookup.push_back(-1);
	}}

	m_actualfacecount = 0;
	m_actualvertcount = 0;


	{for (int f = 0; f < realmesh.getNumFaces() ; f++) {

		// Skip this face if it the next to be ignored...
		if (FindInParamTable(snoPBlock,snoIgnoreFaceListPID,CurrT,f))
		{
			m_faceLookup.push_back(-1);
			continue;
		}

		m_faceLookup.push_back(m_actualfacecount);
		m_actualfacecount++;
		m_faceValues.push_back(f);

		const Face& face = realmesh.faces[f];
		
		for (int j = 0; j < 3; j++) 
		{
			int mappedv = face.v[j];

			if (m_vertLookup[mappedv] < 0) {
				m_vertLookup[mappedv] = m_actualvertcount;
				m_actualvertcount++;
				m_vertValues.push_back(mappedv);
			}
		}

	}}

	std::vector<VNormal*> vnorms;
	std::vector<int> firstNormIndex;
	int totalNorms=0;

	{
		Point3 pt[3];
		float  av[3];
		Point3 fnorm;

		// Compute the vertex surface normals for normals we are NOT ignoring
		
		{for (int i=0; i < m_actualvertcount; i++) {
			vnorms.push_back(new VNormal());
		}}

		{for (int i = 0; i < realmesh.getNumFaces(); i++) {

			Face& face = realmesh.faces[i];

			pt[0] = realmesh.verts[face.v[0]];
			pt[1] = realmesh.verts[face.v[1]];
			pt[2] = realmesh.verts[face.v[2]];

			av[0] = ComputeAngleBetweenVectors((pt[1] - pt[0]),(pt[2] - pt[0]));
			av[1] = ComputeAngleBetweenVectors((pt[2] - pt[1]),(pt[0] - pt[1]));
			av[2] = ComputeAngleBetweenVectors((pt[0] - pt[2]),(pt[1] - pt[2]));

			// Calculate the face normal

			fnorm = CrossProd((pt[1] - pt[0]),(pt[2] - pt[0])).Normalize();

			for (int j=0; j<3; j++) {

				int mv = face.v[j];
				int mappedvert = m_vertLookup[mv];
				if (mappedvert < 0)
				{
					continue;
				}
				if ((snoPBlock,snoLockedNormListPID,CurrT,mv))
				{
					// This locked normal points up in Z and matches ANY smoothing group
					vnorms[mappedvert]->AddNormal(Point3(0.0f,0.0f,1.0f),av[j],-1,m_faceLookup[i]>=0);
				} 
				else
				{
					vnorms[mappedvert]->AddNormal(fnorm,av[j],face.getSmGroup(),m_faceLookup[i]>=0);
				}
			}

		}}

		// Now we have to re-normalize everything
		{for (unsigned int i=0; i < vnorms.size(); i++) {
			vnorms[i]->Normalize();
		}}

		// Remove all the normals that are associated only with ignored faces
		{for (unsigned int i=0; i < vnorms.size(); i++) {
			vnorms[i] = Prune(vnorms[i]);
		}}

		// Count all of the norms, storing the position of the first normal used by each vert
		{for (unsigned int i=0; i < vnorms.size(); i++) {

			if (vnorms[i] == NULL) continue;

			firstNormIndex.push_back(totalNorms);

			totalNorms++;

			VNormal* vn = vnorms[i]->next;

			while (vn) {
				totalNorms++;
				vn = vn->next;
			}
		}}

	}

	//=====================================================================

	// Fetch door verts
	if (snoPBlock)
	{
		int numdoors = snoPBlock->Count(snoDoorLastVertsPID);
		int vi = 0;
		int vmax = 0;
		int vmin;
		int vert = -1;
		
		int vactmax = snoPBlock->Count(snoDoorVertListPID);

		for (int di = 0 ; di < numdoors; di++)
		{

			std::list<int> doorverts;

			vmin = vmax;
			vmax = snoPBlock->GetInt(snoDoorLastVertsPID,CurrT,di);

			if (vactmax<vmax)
			{
				OutputDebugString("Your door verts are scrambled, you are pooched\n");
			}

			for (vi = vmin ; vi < vmax; vi++)
			{
				vert = snoPBlock->GetInt(snoDoorVertListPID,CurrT,vi) - 1;
				doorverts.push_back( vert );
			}

			//doors.push_back( doorverts );
		}
	}

	try {

		//== Header Info =============================
		SiegeNodeIO_header Header;

		Header.MagicValue = SNOD_MAGIC; // "SNOD"
		Header.Version = SNOD_VERSION;
		Header.MinorVersion = SNOD_MINOR_VERSION;

		Header.NumVertices = m_actualvertcount; // realmesh.getNumVerts();
		Header.NumTextureVerts = m_uvValues.size(); //realmesh.TVNum(); 

		if (realmesh.getNumVertCol() == 0) {
			Header.NumColourVerts = 1;
		} else {
			Header.NumColourVerts = realmesh.getNumVertCol();
		}

		Header.NumNormals = totalNorms;
		Header.NumMaterials = m_matsActuallyUsed;
		Header.NumTextureMaps = m_texturesActuallyUsed;
		Header.NumFaces = m_actualfacecount; //realmesh.getNumFaces();
		Header.NumDoors	= snoPBlock ? snoPBlock->Count(snoDoorLastVertsPID) : 0;
		Header.NumFloorFaces = snoPBlock ? snoPBlock->Count(snoFloorFaceListPID) : 0;
		Header.NumWaterFaces = snoPBlock ? snoPBlock->Count(snoWaterFaceListPID) : 0;

		// The the wireframe colour of the object we ar using as a boundary
		Header.WireframeColour = thisnode->GetWireColor();

		Header.NumSpots = 0;

		AppendData((const char *)&Header,sizeof(Header));

		//== Doors ==================================
		
		int FirstDoorVert = 0;
		int FirstDoorI = 0;
		int NextDoorI = 0;

		{for (UINT32 i = 0 ;  snoPBlock && (i < Header.NumDoors) ; i++) {

			UINT32 DoorID = i+1;
			AppendData((const char *)&DoorID,sizeof(DoorID));

			FirstDoorI = NextDoorI;
			NextDoorI = snoPBlock->GetInt(snoDoorLastVertsPID,CurrT,i);

			int DoorMin = snoPBlock->GetInt(snoDoorVertListPID,CurrT,FirstDoorI)-1;
			int DoorMax = snoPBlock->GetInt(snoDoorVertListPID,CurrT,NextDoorI-1)-1;

			Point3 MinDoorPt = realmesh.verts[DoorMin];
			Point3 MaxDoorPt = realmesh.verts[DoorMax];		

			// Get the center of the door
			Point3 doorCenter = VectorTransform(vertTM,(MinDoorPt+MaxDoorPt)*0.5f);		

			// Get the orientation
			Point3 doorOut  = Normalize(snoPBlock->GetPoint3(snoDoorDirectionListPID,CurrT,i));
			Point3 doorUp   = Point3(0,0,1);		
			Point3 doorBase = CrossProd(doorUp,doorOut);

			Matrix3 doorOrient(doorBase,doorUp,doorOut,Point3(0,0,0));

			// I think that this swizzle is unecessary, but it was done in v1.0 
			Matrix3 swizOrient(false);
			SwizzleToYisUp(doorOrient,swizOrient);

			Point3 x_axis = swizOrient.GetRow(0);
			Point3 y_axis = swizOrient.GetRow(1);
			Point3 z_axis = swizOrient.GetRow(2);

			AppendData((const char *)&doorCenter,sizeof(doorCenter));				

			AppendData((const char *)&x_axis,	sizeof(x_axis));
			AppendData((const char *)&y_axis,	sizeof(y_axis));
			AppendData((const char *)&z_axis,	sizeof(z_axis));

			// Dump out the edges used by this Door

			int numverts = NextDoorI - FirstDoorI;

			AppendData((const char *)&numverts,sizeof(int));

			for (int v=FirstDoorI ; v < NextDoorI; ++v)
			{
				int vi = snoPBlock->GetInt(snoDoorVertListPID,CurrT,v)-1;
				int vert = m_vertLookup[vi];
				AppendData((const char *)&vert,sizeof(int));
			}

			
		}}

		//==================================================
		// The rest of the data concerns the bounding mesh
		// future version of this exporter MAY write 
		// separate files out
		//==================================================
		
		//== Vertices ================================

		{for (UINT32 i = 0; i < Header.NumVertices; i++) {

			UINT32 id = m_vertValues[i];

			Point3 v = realmesh.getVert(id);

			Point3 pt = VectorTransform(vertTM,v);

			// Might have to convert to  Foundation/Math vector_3 format
			AppendData((char *)&pt, sizeof(Point3));
			
		}}

		//== Texture Verts ===========================
		
		{ for ( std::vector<Point2>::iterator it=m_uvValues.begin();
			it != m_uvValues.end(); ++it) {

			float xx = (*it).x;
			float yy = (*it).y;

			// Here we are assuming the the artist is NOT using W
			AppendData((char *)&xx, sizeof(float));
			AppendData((char *)&yy, sizeof(float));


		}}

		/*
		{for (UINT32 i = 0; i < Header.NumTextureVerts; i++) {

			// A texture transformation matrix would be anice thing in here...
			// Point3 pt = VectorTransform(vertTM,realmesh.P(i));
			
			UVVert uv = realmesh.TV(i);

			// Here we a assuming the the artist is NOT using W
			AppendData((char *)&uv.x, sizeof(float));
			AppendData((char *)&uv.y, sizeof(float));
			
		}}
		*/

		//== Vertex Colours ==========================
		
		if (realmesh.getNumVertCol() == 0) {

			VertColor vc(1.0,1.0,1.0);
			AppendData((char *)&vc, sizeof(VertColor));

		} else {

			for (UINT32 i = 0; i < realmesh.getNumVertCol(); i++) {

			
				VertColor vc = realmesh.vertCol[i];

				// Here we a assuming the the artist is NOT using W
				AppendData((char *)&vc, sizeof(VertColor));
				
			}
		}

		
		//== Vertex Normals ==========================

		{ for (unsigned int i=0; i < vnorms.size(); i++) {

			Point3 nrm = vnorms[i]->norm;
			Point3 pt = VectorTransform(normTM,nrm);
			AppendData((char *)&pt, sizeof(Point3));

			VNormal *vn = vnorms[i]->next;

			while (vn) {

				nrm = vn->norm;
				pt = VectorTransform(normTM,nrm);
				AppendData((char *)&pt, sizeof(Point3));


				vn = vn->next;
			}
		}}
		
		//== Face Index Table ========================

		
		std::vector<UINT16> tempFaceMapping(Header.NumFaces);

		unsigned int total_face_count = 0;

		{for (UINT32 i = 0; i < Header.NumFaces ; i++) {
			tempFaceMapping[i] = -1;
		}}

		//**********************************************************************************************
		//**********************************************************************************************
		// Code for exporting materials that have no texture map seems to be missing from this location
		//**********************************************************************************************
		//**********************************************************************************************

		{for (int curr_mat = mtsize - dcsize,j = 0; 
				curr_mat < mtsize; ++curr_mat, ++j) {

			if (m_matUsed[j]) {

				UINT32 curr_face_count = 0;

				{for (UINT32 fc = 0; fc < Header.NumFaces ; fc++) {

					UINT32 i = m_faceValues[fc];

					Face& face = realmesh.faces[i];
				
					// Every face must be a triangle
					// TODO: fix it so I use the triangulation
					// gpassert(face.deg == 3);
					
					// Does this face use the current material

					int fid = face.getMatID();

					if (fid >= dcsize) {
						fid = fid%dcsize;
					}

					if (curr_mat != m_materialMapping[fid]) {
						continue;
					}

					curr_face_count++;
					
					gpassert (tempFaceMapping[fc] == (UINT16)-1);
					tempFaceMapping[fc] = total_face_count++;
					
					{for (int k = 0; k <3; k++) {
					
						UINT32 vindex;
						UINT32 index;

						// Vert index
						vindex = m_vertLookup[face.v[k]];
						
						gpassert (vindex>= 0);
						gpassert (vindex<m_actualvertcount);

						AppendData((char *)&vindex, sizeof(index));
					
						// UVVert index
						if (Header.NumTextureVerts) {
							TVFace& tvface = realmesh.tvFace[i];
							index = m_uvLookup[tvface.getTVert(k)];		// Index the rounded values
							gpassert (index < Header.NumTextureVerts);
						} else {
							index = -1;
						}
						AppendData((char *)&index, sizeof(index));
					
						// VertColor index
						if (realmesh.getNumVertCol()) {
							TVFace& vcface = realmesh.vcFace[i];
							index = vcface.getTVert(k);
							gpassert (index < realmesh.getNumVertCol());
						} else {
							index = 0;
						}

						AppendData((char *)&index, sizeof(index));

						int normindex = -1;
						if (Header.NumNormals) {

							normindex = firstNormIndex[vindex];

							if (!(vnorms[vindex]->smooth & face.smGroup)) {

								// Search for the right smoothing group
								VNormal *vn = vnorms[vindex]->next;
								normindex++;
								while (vn && !(vn->smooth & face.smGroup))
								{
									vn = vn->next;
									normindex++;
								}
							}

						}

						AppendData((char *)&normindex, sizeof(normindex));

				
					}}
				
				}}

				m_materialFaceCount.push_back(curr_face_count);
			}

		}}

		{for (UINT32 i = 0; i < Header.NumFaces ; i++) {
			if (tempFaceMapping[i] == (UINT16)-1) {
				gpassert(tempFaceMapping[i] != (UINT16)-1);
			}
		}}

		gpassert(total_face_count == Header.NumFaces);

		//== Floor Faces  ============================
		
		UINT32 floor_count = 0;
		{for (UINT32 i = 0; snoPBlock && (i < Header.NumFloorFaces) ; i++) {

			int fi = snoPBlock->GetInt(snoFloorFaceListPID,CurrT,i)-1;

			UINT16 index = tempFaceMapping[fi];
			gpassert(index < Header.NumFaces);
			AppendData((char *)&index, sizeof(index));
		}}

		//== Water Faces  ============================
		
		{for (UINT32 i = 0; snoPBlock && (i < Header.NumWaterFaces) ; i++) {

			int fi = snoPBlock->GetInt(snoWaterFaceListPID,CurrT,i)-1;

			UINT16 index = tempFaceMapping[fi];
			gpassert(index < Header.NumFaces);
			AppendData((char *)&index, sizeof(index));

		}}

		//== Materials ==============================

		int count = m_materialFaceCount.size();
		AppendData((char *)&count, sizeof(count));

		// Dump out the number of faces used by each material
		{for (int i = 0; i < count; i++) {
			int count = m_materialFaceCount[i];
			AppendData((char *)&count, sizeof(count));
		}}


		ExportMaterials();

		// Tack on history/debugging information

		AppendData("INFO",4);

		int fieldcount = 7;		// Number of strings of info we are about to write
		AppendData((char *)&fieldcount, sizeof(fieldcount));

		char tempbuf[256];
		char nbuf[200];
		DWORD nbufsize = 200;

		WNetGetUser(NULL,&nbuf[0],&nbufsize);
		sprintf( tempbuf, "USER=%s", nbuf );
		AppendData(tempbuf,strlen(tempbuf)+1);

		gethostname(nbuf,nbufsize);
		sprintf( tempbuf, "HOST=%s", nbuf );
		AppendData(tempbuf,strlen(tempbuf)+1);

		sprintf( tempbuf, "MAXFILE=%s", m_MaxFName );
		AppendData(tempbuf,strlen(tempbuf)+1);

		sprintf( tempbuf, "SNOFILE=%s", expfname );
		AppendData(tempbuf,strlen(tempbuf)+1);

		sprintf( tempbuf, "VERSION=%d.%d", SNOD_VERSION , SNOD_MINOR_VERSION );
		AppendData(tempbuf,strlen(tempbuf)+1);

		char dbuffer [9];
		_strdate( dbuffer );

		sprintf( tempbuf, "DATE=%s", dbuffer );
		AppendData(tempbuf,strlen(tempbuf)+1);

		char tbuffer [9];
		_strtime( tbuffer );

		sprintf( tempbuf, "TIME=%s", tbuffer );
		AppendData(tempbuf,strlen(tempbuf)+1);

		// delete the norms list
		{for (unsigned int i = 0; i < vnorms.size(); ++i) {
			delete vnorms[i];
		}}


		// With the file written successfully, we run the post processing step.  This will
		// reorganize the triangle data into a form that is friendly to the game at load time
		// and will calculate redundant pathfinding information.
		PostProcess processor;

		if (!processor.OrgAndCalcInfo( outfile, m_outbuff ))
		{
			return DSiegeUtils::EXP_ERROR_IN_POSTPROCESS;
		}

	}

	catch (...) {
		return DSiegeUtils::EXP_ERROR_EXCEPTION;
	}

	return DSiegeUtils::EXP_SUCCESS;
}

void SNOExporter::EnumerateMaterial( Mtl *MaxMaterial ) {


	Texmap *CurrTexture = MaxMaterial->GetSubTexmap(ID_DI);

	if (CurrTexture) {

		m_materialDiffuseColours.push_back(MaxMaterial->GetDiffuse());	

		m_materialMapping.push_back(m_materialTextures.size());
		m_materialTextures.push_back(CurrTexture);	

	} else {

		m_materialDiffuseColours.push_front(MaxMaterial->GetDiffuse());	
		m_materialMapping.push_back(m_materialTextures.size()-m_materialDiffuseColours.size());

	}

}

void SNOExporter::EnumerateMaterialTree( Mtl *RootMaterial ) {

	m_materialDiffuseColours.clear();
	m_materialTextures.clear();
	
	if (RootMaterial->IsMultiMtl()) {

		{for (int i=0; i<RootMaterial->NumSubMtls(); i++) {

			Mtl *sub_mat = RootMaterial->GetSubMtl(i);

			EnumerateMaterial(sub_mat);

		}}

	} else {

		EnumerateMaterial(RootMaterial);
	}	
}

void SNOExporter::ExportMaterials(void) {

	int j = 0;
	{ for (std::list<Color>::iterator i = m_materialDiffuseColours.begin();
		i != m_materialDiffuseColours.end();
		++i,++j) {

		if (m_matUsed[j]) {

			float red	= (*i).r;
			float green = (*i).g;
			float blue	= (*i).b;

			AppendData((char *)&red,sizeof(float));
			AppendData((char *)&green,sizeof(float));
			AppendData((char *)&blue,sizeof(float));

		}
		
	}}
 
	j= m_materialDiffuseColours.size() - m_materialTextures.size();
	{ for (std::list<Texmap *>::iterator i = m_materialTextures.begin();
		i != m_materialTextures.end();
		++i,++j) {

		if (m_matUsed[j]) {
			Texmap *ActiveTexture = (*i); 
			DumpOutTheTexture(ActiveTexture);
		}
		
	}}

}


bool SNOExporter::DumpOutTheTexture(Texmap *ActiveTexture) {

		gpstring TextureName;
		
		// See if it's a bitmap texture
		if((ActiveTexture->ClassID() ==
			Class_ID(BMTEX_CLASS_ID, 0x00)))
		{
			// It is a bitmap texture - try to get the bitmap
			BitmapTex *ActiveTextureCast = (BitmapTex *)ActiveTexture;
			
			char DriveBuffer[_MAX_DRIVE];
			char PathBuffer[_MAX_DIR];
			char FilenameBuffer[_MAX_FNAME];
			char ExtensionBuffer[_MAX_EXT];
			_splitpath(ActiveTextureCast->GetMapName(),
					   DriveBuffer, PathBuffer,
					   FilenameBuffer, ExtensionBuffer);
			TextureName += FilenameBuffer;
			TextureName += ExtensionBuffer;
			// TODO: Replace the . with a _ in the extension buffer?

		}
		else
		{
			TextureName += "MAP";
			TextureName += rand();
		}

		// Put the name in the file (we'll probably want it later)
		AppendData(TextureName.c_str(),TextureName.size()+1);
		
		return true;

}

bool SNOExporter::VerifyRequiredGlobals(void)
{
	return true;
}

void SNOExporter::Dump()
{
	return;
}
