/**********************************************************************
 *<
	FILE: exporter.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: 

	HISTORY: 
		Version 5.3 has multiple vertex normals and the NextVert button for doors
		Version 5.4 has been ported to MAX 3.0
		Version 5.6	includes the correct name in the INFO block and support modifying the original mesh


 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "precompiled_siegenodeobject.h"

#include "matrix_3x3.h"

#include "math\calcbsp.h"
#include "math\plane_3.h"
#include "postprocess.h"



// Make the exporter available to the DLL loader
static SiegeNodeExporterClassDesc SiegeNodeExporterDesc;
ClassDesc* GetSiegeNodeExporterDesc() {return &SiegeNodeExporterDesc;}

//TODO: Should implement this method to reset the plugin params when Max is reset
void SiegeNodeExporterClassDesc::ResetClassParams (BOOL fileReset) 
{

}

BOOL CALLBACK SiegeNodeExporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static SiegeNodeExporter *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (SiegeNodeExporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
	}
	return FALSE;
}

//--- fwd decl of utility functions
Matrix3 BuildVertTransformMatrix(INode *sourceMesh);
Matrix3 BuildNormTransformMatrix(INode *sourceMesh);

matrix_3x3 ConvertToMatrix3x3(const Matrix3& mat);

//****************
inline void SwizzleToYisUp(const Matrix3& in_mat, Matrix3& out_mat)
{
	if (in_mat == out_mat)
	{
		Matrix3 temp(false);
		temp.SetColumn(0,-in_mat.GetColumn(0) );
		temp.SetColumn(1, in_mat.GetColumn(2) );
		temp.SetColumn(2, in_mat.GetColumn(1) );
		out_mat = temp;
	}
	else
	{
		out_mat.SetColumn(0,-in_mat.GetColumn(0) );
		out_mat.SetColumn(1, in_mat.GetColumn(2) );
		out_mat.SetColumn(2, in_mat.GetColumn(1) );
	}
}

//--- SiegeNodeExporter -------------------------------------------------------
SiegeNodeExporter::SiegeNodeExporter()
{

}

SiegeNodeExporter::~SiegeNodeExporter() 
{

}

int SiegeNodeExporter::ExtCount()
{
	//TODO: Returns the number of file name extensions supported by the plug-in.
	return 1;
}

const TCHAR *SiegeNodeExporter::Ext(int n)
{		
	//TODO: Return the 'i-th' file name extension (i.e. "3DS").
	switch (n) {
		case 0: 
		default:
			return _T("SNO");
	}
}

const TCHAR *SiegeNodeExporter::LongDesc()
{
	//TODO: Return long ASCII description (i.e. "Targa 2.0 Image File")
	return _T("Siege Node Data File");
}
	
const TCHAR *SiegeNodeExporter::ShortDesc() 
{			
	//TODO: Return short ASCII description (i.e. "Targa")
	return _T("SiegeNode");
}

const TCHAR *SiegeNodeExporter::AuthorName()
{			
	//TODO: Return ASCII Author name
	return _T("biddle");
}

const TCHAR *SiegeNodeExporter::CopyrightMessage() 
{	
	// Return ASCII Copyright message
	return _T("Copyright Gas Powered Games 1999");
}

const TCHAR *SiegeNodeExporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *SiegeNodeExporter::OtherMessage2() 
{		
	//TODO: Return other message #2 if any
	return _T("");
}

UINT32 SiegeNodeExporter::Version()
{				
	//TODO: Return Version number * 100 (i.e. v3.01 = 301)
	return (SNOD_VERSION * 100) + SNOD_MINOR_VERSION;
}

void SiegeNodeExporter::ShowAbout(HWND hWnd)
{			
	// Optional
}

int SiegeNodeExporter::DoExport(const TCHAR *filename,ExpInterface *expIFace,
						Interface *maxIFace, BOOL suppressPrompts, DWORD options) 
{
	//TODO: Implement the actual file export here and 
	//		return TRUE If the file is exported properly

	/* We aren't prompting right now

	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				SiegeNodeExporterOptionsDlgProc, (LPARAM)this);

	 	// Assert the pointers that MAX should've passed as references
	*/

	assert(filename);
	assert(expIFace);
	assert(maxIFace);

	int ExportSuccessful = TRUE;

	IScene* scene = expIFace->theScene;

	
// Set the enumerator with a handle to the stream
	//Open a handle to the export stream

	SiegeNodeExportEnum* xport = new SiegeNodeExportEnum(filename,maxIFace);

	if( xport )
	{
		xport->d_exported_count = 0;

		scene->EnumTree(xport);

		char buff[128];
		bool retvalue	= false;

		if( xport->d_exported_count == 0 )
		{
//			sprintf (buff, "Nothing was exported\n\nDid you forget to select a node to export?");
//			MessageBox(NULL,buff,"Hey, Sailor!",MB_OK| MB_ICONSTOP );
		}
		else
		{
			if( xport->d_exported_count == 1 )
			{
				sprintf (buff, "1 node exported");
			}
			else
			{
				sprintf (buff, "%d nodes exported",xport->d_exported_count);
			}
//			MessageBox(NULL,buff,"Hey, Sailor!",MB_OK);
			retvalue	= true;
		}

		delete xport;
		return retvalue;
	}

	return false;
}

//******************************************************
//******************************************************
SiegeNodeExportEnum::SiegeNodeExportEnum(TSTR fname,Interface *maxIFace)
	 :	d_exportfilename(std::string(fname))
	 ,	d_interface(maxIFace)
{

	d_interface = maxIFace;
/*
	TSTR path,file,ext;
	SplitFilename(fname,&path,&file,&ext);
 
	d_outstream = smart_pointer<std::ofstream>(new std::ofstream(fname,std::ios::out|std::ios::binary));

	assert(d_outstream->good());
	
	if (!d_outstream) {
		delete this; // Is this the 'right way' to handle a bad file open
	}
*/

}

SiegeNodeExportEnum::~SiegeNodeExportEnum() {

}

inline const Point3 reorder(const Point3& v) {

	Point3 nv(-v[0],v[2],v[1]);

	return nv;

}

void SiegeNodeExportEnum::SpotEnum(const Matrix3& RootTM, INode* node) {

	Matrix3 spotTM = node->GetNodeTM(0) * Inverse(RootTM);

//	spotTM.RotateX(-HALFPI);
//	spotTM.RotateY(PI);

//	spotTM.PreRotateY(PI);					/// WHY DO I HAVE TO DO THIS!!!!! WHERE DID I SCREW IT UP?

//	Point3 center = spotTM.GetRow(3);
//	center *= 0.001f;

	spotTM = node->GetNodeTM(0) * Inverse(RootTM);

	spotTM.SetRow(0,reorder(spotTM.GetRow(0)));
	spotTM.SetRow(1,reorder(spotTM.GetRow(1)));
	spotTM.SetRow(2,reorder(spotTM.GetRow(2)));
	spotTM.SetRow(3,reorder(spotTM.GetRow(3))*0.001f);

	SmartSpot newspot(new Spot(node->GetName(),spotTM));

	d_spotArray.push_back(newspot);
	
}


int SiegeNodeExportEnum::callback(INode *node) {


	// Is this a SiegeNodeObject?
	if (node->GetObjectRef()->CanConvertToType(SIEGENODEOBJECT_CLASS_ID)) {

		TSTR nodefilename;
		try {

			// Are there any selected meshes?

			// Is the mesh attach to this siege node selected

			INode *Rootnode = d_interface->GetRootNode();

			SiegeNodeObject* sno = (SiegeNodeObject*)node->GetObjectRef()->ConvertToType(0,SIEGENODEOBJECT_CLASS_ID);

			// This isn't the 'best' way to run through the list of verts
			// (and everything else in the boundary MNMesh for that matter)
			// but I need to get something going and I WILL be coming back
			// to revise it once I get a better feel for the process
			
			// For now we'll write DEADBEEF as a dummy entry for everything			
			int beef = 0xEFBEADDE;

			BoundaryMesh* bmesh = sno->GetBMesh();

			TimeValue t(0);
			INode*  thisnode = (INode *)sno->GetReference(SiegeNodeObject::REF_MESH);

			if (!thisnode->Selected()) {
				return false;
			}

			bmesh->UpdateBoundaryMesh(thisnode);

			Matrix3 vertTM = BuildVertTransformMatrix((INode *)sno->GetReference(SiegeNodeObject::REF_MESH));
			Matrix3 normTM = BuildNormTransformMatrix((INode *)sno->GetReference(SiegeNodeObject::REF_MESH));

			d_matUsed.clear();
			d_uvValues.clear();
			d_uvLookup.clear();
			d_spotArray.clear();

			d_matsActuallyUsed = 0;
			d_texturesActuallyUsed = 0;
			d_materialFaceCount.clear();

	        Object *o =thisnode->EvalWorldState(t).obj;
	        TriObject *tri = (TriObject *)o->ConvertToType(t, triObjectClassID);
		    Mesh& realmesh = tri->GetMesh();

			bool VertsAreOK = false;

			if (realmesh.numVerts != bmesh->VNum()) {

				for (int badvert = realmesh.numVerts; !VertsAreOK && (badvert <= bmesh->VNum()); badvert++) {
					int goodvert;
					for (goodvert = 0; goodvert < realmesh.numVerts ; goodvert++) {
						if (realmesh.verts[goodvert] == bmesh->P(badvert)) {
							// Should verify that we have the correct UV coords too!
							break;
						}
					}
					VertsAreOK = goodvert < realmesh.numVerts;
				}

				if (!VertsAreOK) {
					char buff[512];
					sprintf(buff,"The vert count for %s differs between the Mesh and MNMesh\n",thisnode->GetName(),realmesh.numVerts,bmesh->VNum());
					MessageBox(NULL,buff,"Hey, Sailor!",MB_OK|MB_ICONWARNING );
				}

			} else {

				VertsAreOK = true;

			}

			if (realmesh.numFaces != bmesh->FNum()) {
				char buff[512];
				sprintf(buff,"The face count for %s  differs between the Mesh and MNMesh\n\n%d != %d",thisnode->GetName(),realmesh.numFaces,bmesh->FNum());
				MessageBox(NULL,buff,"Hey, Sailor!",MB_OK|MB_ICONWARNING );
			}
			
			if (!VertsAreOK || (realmesh.numFaces != bmesh->FNum())) {
				char buff[512];
				sprintf(buff,"Please use STL check to look for errors in %s\n\nThis node will not be exported",thisnode->GetName());
				MessageBox(NULL,buff,"Hey, Sailor!",MB_OK|MB_ICONSTOP );
				return TREE_IGNORECHILDREN;
			}


			// Can we write to the file?

			{
				TSTR path,file,ext;

				TSTR fname = d_exportfilename.c_str();

				TSTR nname = thisnode->GetName();

				int pos;


				pos = _tcsspn(nname," \t");
				nodefilename = _tcsninc(nname,pos);
				_tcsrev(nodefilename);

				pos = _tcsspn(nodefilename," \t");
				nname = _tcsninc(nodefilename,pos);
				_tcsrev(nname);

				
				char* underscore= _tcschr(nname,' ');
				while (underscore) {
					*underscore = '_';
					underscore= _tcschr(nname,' ');
				}

				underscore= _tcschr(nname,'\t');
				while (underscore) {
					*underscore = '_';
					underscore= _tcschr(nname,'\t');
				}

				SplitFilename(fname,&path,&file,&ext);

				nodefilename = path +"\\"+ nname + ext;


				d_outstream = smart_pointer<std::ofstream>(new std::ofstream(nodefilename,std::ios::out|std::ios::binary));


				if (!d_outstream->good()) {
					nname = "Couldn't open file\n\n";
					nname = nname + nodefilename;
					MessageBox(NULL,nname,"Hey, Sailor!",MB_OK| MB_ICONSTOP );
					return TREE_IGNORECHILDREN;
				}

			}


 			//== Setup =============================
			Mtl *MaxMaterial = (*(INode*)(sno->GetReference(SiegeNodeObject::REF_MESH))).GetMtl();


			// Recursively enumerate the materials list for the selected node
			if(MaxMaterial)
			{
				EnumerateMaterialTree(MaxMaterial);
			}


			// Build a list of all the 'SPOT' objects in the scene
			for (int idx = 0; idx < thisnode->NumberOfChildren(); idx++) {
				SpotEnum(thisnode->GetNodeTM(0),thisnode->GetChildNode(idx));
			}

			// Now determine just which materials are used by the object
			d_matUsed.clear();
			d_matsActuallyUsed = 0;
			d_texturesActuallyUsed = 0;

			// curr_mat runs from -NUM_OF_NONTEXTURED to NUM_OF_TEXTURED-1

			int mtsize = d_materialTextures.size();
			int dcsize = d_materialDiffuseColours.size();

			{for (int curr_mat = mtsize - dcsize; 
					curr_mat < mtsize; ++curr_mat) {

				bool curr_mat_used = false;

				{for (int i = 0; i < realmesh.getNumFaces() ; i++) {

					// Skip this face if it's marked as ignored...
					if (bmesh->F(i)->GetFlag(IGNORED_FLAG)) {
						continue;
					}

					Face& face = realmesh.faces[i];
					
					// Does this face use the current material
					if (curr_mat == d_materialMapping[face.getMatID()%dcsize]) {
						curr_mat_used = true;
						d_matsActuallyUsed++;
						if (curr_mat >= 0) {
							d_texturesActuallyUsed++;
						}
						break;
					}

				}}

				d_matUsed.push_back(curr_mat_used);
			}}
/*
			{for (UINT32 curr_mat = d_materialTextures.size() - d_materialDiffuseColours.size(); 
					curr_mat < d_materialTextures.size(); ++curr_mat) {

				bool curr_mat_used = false;

				{for (int i = 0; i < realmesh.FNum() ; i++) {

					MNFace* face = realmesh.F(i);
					
					// Does this face use the current material
					if (curr_mat == d_materialMapping[face->material]) {
						curr_mat_used = true;
						d_matsActuallyUsed++;
						d_texturesActuallyUsed++;
						break;
					}

				}}

				d_matUsed.push_back(curr_mat_used);
			}}
*/
			if (d_materialTextures.size() && !d_matsActuallyUsed) {
				char buf[256];
				sprintf(buf,"Multi/Sub material has %d items, of which %d are used",d_materialTextures.size(),d_matsActuallyUsed);
				MessageBox(NULL,buf,"",MB_OK|MB_ICONWARNING);
			}

			//== Create a reduce precision version of the UV coords ==

			d_uvLookup.clear();
			d_uvLookup.reserve(realmesh.getNumTVerts());
			d_uvValues.clear();
			d_uvValues.reserve(realmesh.getNumTVerts());

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

				{ for ( std::vector<Point2>::iterator it=d_uvValues.begin();
						it != d_uvValues.end(); ++it) {

					if (*it == newuv) {
						found = true;
						break;
					}
					++pos;
								
				}}

				if (!found) {
					pos = d_uvValues.size();
					d_uvValues.push_back(newuv);
				}

				d_uvLookup.push_back(pos);


			}}

			// Build the vert and face lookup tables
			// so we can remove the ignored faces

			d_vertLookup.clear();
			d_vertValues.clear();

			d_faceLookup.clear();
			d_faceValues.clear();


			// Initialize the vert lookup to 'all ignored'
			{for (int i = 0; i < realmesh.numVerts; i++) {
				d_vertLookup.push_back(-1);
			}}

			d_actualfacecount = 0;
			d_actualvertcount = 0;

			{for (int f = 0; f < realmesh.getNumFaces() ; f++) {

				// Skip this face if it's marked as ignored...
				if (bmesh->F(f)->GetFlag(IGNORED_FLAG)) {
					d_faceLookup.push_back(-1);
					continue;
				} 

				d_faceLookup.push_back(d_actualfacecount);
				d_actualfacecount++;
				d_faceValues.push_back(f);

				MNFace* face = bmesh->F(f);;
				
				for (int j = 0; j < 3; j++) {

					int mappedv = face->vtx[j];

					if (mappedv >= realmesh.numVerts) {

						// This is an extra vert added by the MN mesh
						for (int goodvert = 0; goodvert < realmesh.numVerts ; goodvert++) {
							if (realmesh.verts[goodvert] == bmesh->P(mappedv)) {
								mappedv = goodvert;
								break;
							}
						}
					}

					if (d_vertLookup[mappedv] < 0) {
						d_vertLookup[mappedv] = d_actualvertcount;
						d_actualvertcount++;
						d_vertValues.push_back(mappedv);
					}
				}
				

			}}


//*********************


			std::vector<VNormal*> vnorms;
			std::vector<int> firstNormIndex;
			int totalNorms=0;

			{
				MNVert *v0, *v1, *v2;
				Point3 fnorm;

//				vnorms.SetCount(bmesh->VNum());

				// Compute the vertex surface normals for normals we are NOT ignoring
				
				{for (int i=0; i < d_actualvertcount; i++) {
					vnorms.push_back(new VNormal());
				}}

				{for (int i = 0; i < realmesh.getNumFaces(); i++) {

					MNFace* face = bmesh->F(i);

if (!face) MessageBox(NULL,"Internal exporter error: face is NULL","Snooping",MB_OK|MB_ICONWARNING );

					// Calculate the surface normal

					int mv0 = face->vtx[0];
					if (mv0 >= realmesh.numVerts) {

						// This is an extra vert added by the MN mesh
						for (int goodvert = 0; goodvert < realmesh.numVerts ; goodvert++) {
							if (realmesh.verts[goodvert] == bmesh->P(mv0)) {
								mv0 = goodvert;
								break;
							}
						}
					}
					v0 = bmesh->V(mv0);

					int mv1 = face->vtx[1];
					if (mv1 >= realmesh.numVerts) {

						// This is an extra vert added by the MN mesh
						for (int goodvert = 0; goodvert < realmesh.numVerts ; goodvert++) {
							if (realmesh.verts[goodvert] == bmesh->P(mv1)) {
								mv1 = goodvert;
								break;
							}
						}
					}
					v1 = bmesh->V(mv1);

					int mv2 = face->vtx[2];
					if (mv2 >= realmesh.numVerts) {

						// This is an extra vert added by the MN mesh
						for (int goodvert = 0; goodvert < realmesh.numVerts ; goodvert++) {
							if (realmesh.verts[goodvert] == bmesh->P(mv2)) {
								mv2 = goodvert;
								break;
							}
						}
					}
					v2 = bmesh->V(mv2);

if (!v0) MessageBox(NULL,"Internal exporter error: v0 is NULL","Snooping",MB_OK|MB_ICONWARNING );
if (!v1) MessageBox(NULL,"Internal exporter error: v1 is NULL","Snooping",MB_OK|MB_ICONWARNING );
if (!v2) MessageBox(NULL,"Internal exporter error: v2 is NULL","Snooping",MB_OK|MB_ICONWARNING );

					fnorm = (v1->p - v0->p)^(v2->p - v1->p);

					for (int j=0; j<3; j++) {

						int mv = face->vtx[j];
						if (mv >= realmesh.numVerts) {

							// This is an extra vert added by the MN mesh
							for (int goodvert = 0; goodvert < realmesh.numVerts ; goodvert++) {
								if (realmesh.verts[goodvert] == bmesh->P(mv)) {
									mv = goodvert;
									break;
								}
							}
						}

if (mv >= d_vertLookup.size()) {
	char buff[512];
	sprintf(buff,"Internal Exporter error, bmesh->F(%d)->vtx[%d] is out of bounds\n\nArray Size:  %d\nArray Index: %d",i,j,d_vertLookup.size(),mv);
	MessageBox(NULL,buff,"Snooping",MB_OK|MB_ICONWARNING );
}
						int mappedvert = d_vertLookup[mv];
						if (mappedvert < 0) {
							continue;
						}

if (mappedvert >= vnorms.size()) {
	char buff[512];
	sprintf(buff,"Internal Exporter error, mappedvert is out of bounds\n\nArray Size:  %d\nArray Index: %d",vnorms.size(),mappedvert);
	MessageBox(NULL,buff,"Snooping",MB_OK|MB_ICONWARNING );
}

if (!vnorms[mappedvert]) MessageBox(NULL,"Internal Exporter error, vnorms[mappedvert] is NULL","Snooping",MB_OK|MB_ICONWARNING );

						if (bmesh->V(face->vtx[j])->GetFlag(LOCKED_NORM_FLAG)) {
							// This normal points up in Z and matches ANY smoothing group
							vnorms[mappedvert]->AddNormal(Point3(0.0f,0.0f,1.0f),-1,d_faceLookup[i]>=0);
						} else {
							// If its NOT spoofed OR is is spoofed but the face is ignored, then use the normal
							if (!bmesh->V(face->vtx[j])->GetFlag(SPOOFED_FLAG)) {
								vnorms[mappedvert]->AddNormal(fnorm,face->smGroup,d_faceLookup[i]>=0);
							} else if (face->GetFlag(IGNORED_FLAG)) {
								// Spoofed verts match ANY smoothing group
								vnorms[mappedvert]->AddNormal(fnorm,-1,d_faceLookup[i]>=0);
							}
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

//*******************

			if( d_matsActuallyUsed != d_texturesActuallyUsed )
			{
				char textbuf[ 512 ];
				sprintf( textbuf, "SNO [%s] has polygons without texture!", thisnode->GetName() );
				MessageBox( NULL, textbuf, "Ooops!", MB_OK|MB_ICONWARNING );
			}

 			//== Header Info =============================
			SiegeNodeIO_header Header;

			Header.MagicValue = SNOD_MAGIC; // "SNOD"
			Header.Version = SNOD_VERSION;
			Header.MinorVersion = SNOD_MINOR_VERSION;

			Header.NumVertices = d_actualvertcount; // realmesh.getNumVerts();
			Header.NumTextureVerts = d_uvValues.size(); //realmesh.TVNum(); 

			if (realmesh.getNumVertCol() == 0) {
				Header.NumColourVerts = 1;
			} else {
				Header.NumColourVerts = realmesh.getNumVertCol();
			}

			Header.NumNormals = totalNorms;
			Header.NumMaterials = d_matsActuallyUsed;
			Header.NumTextureMaps = d_texturesActuallyUsed;
			Header.NumFaces = d_actualfacecount; //realmesh.getNumFaces();

			Header.NumDoors = bmesh->GetNumDoorLoops()+bmesh->NumIDoors();
			Header.NumFloorFaces = bmesh->GetFloorFaceCount();
			Header.NumWaterFaces = bmesh->GetWaterFaceCount();

			// The the wireframe colour of the object we ar using as a boundary
			Header.WireframeColour = (*(INode *)sno->GetReference(SiegeNodeObject::REF_MESH)).GetWireColor();

			Header.NumSpots = d_spotArray.size();

			d_outstream->write((const char *)&Header,sizeof(Header));


			//== Doors ==================================
			
			{for (UINT32 i = 0 ; i < bmesh->GetNumDoorLoops(); i++) {

//				*d_outstream << "DOOR";
//				d_outstream->write((const char *)&i,sizeof(i));

				Door* door = bmesh->GetDoor(i);
				
				UINT32 door_id = door->GetID();
				d_outstream->write((const char *)&door_id,sizeof(door_id));

				// Get the center of the door
				Point3 door_center = VectorTransform(vertTM,door->GetCenterPoint());

				d_outstream->write((const char *)&door_center,sizeof(door_center));
				
				// Get the orientation
				
				//w Matrix3 door_orient = door->GetOrientation();
				//w door_orient.RotateZ(PI);
				//w door_orient.RotateX(-HALFPI);

				Matrix3 door_orient(false);
				SwizzleToYisUp(door->GetOrientation(),door_orient);
				
				Point3 x_axis = door_orient.GetRow(0);
				Point3 y_axis = door_orient.GetRow(1);
				Point3 z_axis = door_orient.GetRow(2);

				d_outstream->write((const char *)&x_axis,sizeof(x_axis));
				d_outstream->write((const char *)&y_axis,sizeof(y_axis));
				d_outstream->write((const char *)&z_axis,sizeof(z_axis));

				// Dump out the edges used by this Door

				// TODO:biddle- once I move to writing wedges, be sure
				// to account for them here (may have repeated verts)

				std::list<int> vlist;

				int numverts = bmesh->GetDoorVerts(i,vlist);

				d_outstream->write((const char *)&numverts,sizeof(int));

				{for (std::list<int>::iterator v = vlist.begin(); v != vlist.end(); ++v) {
					int vert = d_vertLookup[*v];
					d_outstream->write((const char *)&vert,sizeof(int));
				}}
				
			}}

			//== Internal Doors =========================
			
			{for (UINT32 i = 0 ; i < bmesh->NumIDoors(); i++) {

//				*d_outstream << "DOOR";
//				d_outstream->write((const char *)&i,sizeof(i));

				IDoor* door = bmesh->GetIDoor(i);
				
				UINT32 door_id = door->GetID();
				d_outstream->write((const char *)&door_id,sizeof(door_id));

				// Get the center of the door
				Point3 door_center = VectorTransform(vertTM,door->GetCenterPoint());

				d_outstream->write((const char *)&door_center,sizeof(door_center));
				
				// Get the orientation
				
				Matrix3 door_orient(false);
				SwizzleToYisUp(door->GetOrientation(),door_orient);

				//w door_orient.RotateZ(PI);
				//w door_orient.RotateX(-HALFPI);

				door_orient.Orthogonalize();
				
				Point3 x_axis = door_orient.GetRow(0);
				Point3 y_axis = door_orient.GetRow(1);
				Point3 z_axis = door_orient.GetRow(2);

				d_outstream->write((const char *)&x_axis,sizeof(x_axis));
				d_outstream->write((const char *)&y_axis,sizeof(y_axis));
				d_outstream->write((const char *)&z_axis,sizeof(z_axis));

				// Dump out the edges used by this Door

				// TODO:biddle- once I move to writing wedges, be sure
				// to account for them here (may have repeated verts)

				int numverts = door->NumVerts();

				d_outstream->write((const char *)&numverts,sizeof(int));


				{for (int v=0; v < numverts; ++v) {
					int vert = d_vertLookup[door->d_vertIndexArray[v]];
					d_outstream->write((const char *)&vert,sizeof(int));
				}}

				
			}}

				
			//== Spots ===============================

			{for (UINT32 i = 0 ; i < Header.NumSpots; i++) {
				
				const Matrix3& m = d_spotArray[i]->d_transMatrix;

				Point3 x_axis = m.GetRow(0);
				Point3 y_axis = m.GetRow(1);
				Point3 z_axis = m.GetRow(2);
				Point3 center = m.GetRow(3);

				d_outstream->write((const char *)&x_axis,sizeof(x_axis));
				d_outstream->write((const char *)&y_axis,sizeof(y_axis));
				d_outstream->write((const char *)&z_axis,sizeof(z_axis));
				d_outstream->write((const char *)&center,sizeof(center));

				*d_outstream << d_spotArray[i]->d_name.c_str() << '\0';


			}}

			//==================================================
			// The rest of the data concerns the bounding mesh
			// future version of this exporter MAY write 
			// separate files out
			//==================================================
			
			//== Vertices ================================

			{for (UINT32 i = 0; i < Header.NumVertices; i++) {

				UINT32 id = d_vertValues[i];

				Point3 v = realmesh.getVert(id);

				Point3 pt = VectorTransform(vertTM,v);

				// Might have to convert to  Foundation/Math vector_3 format
				d_outstream->write((char *)&pt, sizeof(Point3));
				
			}}

			//== Texture Verts ===========================
			
			{ for ( std::vector<Point2>::iterator it=d_uvValues.begin();
				it != d_uvValues.end(); ++it) {

				float xx = (*it).x;
				float yy = (*it).y;

				// Here we are assuming the the artist is NOT using W
				d_outstream->write((char *)&xx, sizeof(float));
				d_outstream->write((char *)&yy, sizeof(float));


			}}

			/*
			{for (UINT32 i = 0; i < Header.NumTextureVerts; i++) {

				// A texture transformation matrix would be anice thing in here...
				// Point3 pt = VectorTransform(vertTM,realmesh.P(i));
				
				UVVert uv = realmesh.TV(i);

				// Here we a assuming the the artist is NOT using W
				d_outstream->write((char *)&uv.x, sizeof(float));
				d_outstream->write((char *)&uv.y, sizeof(float));
				
			}}
			*/

			//== Vertex Colours ==========================
			
			if (realmesh.getNumVertCol() == 0) {

				VertColor vc(1.0,1.0,1.0);
				d_outstream->write((char *)&vc, sizeof(VertColor));

			} else {

				for (UINT32 i = 0; i < realmesh.getNumVertCol(); i++) {

				
					VertColor vc = realmesh.vertCol[i];

					// Here we a assuming the the artist is NOT using W
					d_outstream->write((char *)&vc, sizeof(VertColor));
					
				}
			}

			
			//== Vertex Normals ==========================

			{ for (unsigned int i=0; i < vnorms.size(); i++) {

				Point3 nrm = vnorms[i]->norm;
				Point3 pt = VectorTransform(normTM,nrm);
				d_outstream->write((char *)&pt, sizeof(Point3));

				VNormal *vn = vnorms[i]->next;

				while (vn) {

					nrm = vn->norm;
					pt = VectorTransform(normTM,nrm);
					d_outstream->write((char *)&pt, sizeof(Point3));


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

				if (d_matUsed[j]) {

					UINT32 curr_face_count = 0;

					{for (UINT32 fc = 0; fc < Header.NumFaces ; fc++) {

						UINT32 i = d_faceValues[fc];

						Face& face = realmesh.faces[i];
					
						// Every face must be a triangle
						// TODO: fix it so I use the triangulation
						// assert(face.deg == 3);
						
						// Does this face use the current material

						int fid = face.getMatID();

						if (fid >= dcsize) {
							fid = fid%dcsize;
						}

						if (curr_mat != d_materialMapping[fid]) {
							continue;
						}

						curr_face_count++;
						
						assert (tempFaceMapping[fc] == (UINT16)-1);
						tempFaceMapping[fc] = total_face_count++;
						
						{for (int k = 0; k <3; k++) {
						
							UINT32 vindex;
							UINT32 index;

							// Vert index
							vindex = d_vertLookup[face.v[k]];
							
							assert (vindex>= 0);
							assert (vindex<d_actualvertcount);

							d_outstream->write((char *)&vindex, sizeof(index));
						
							// UVVert index
							if (Header.NumTextureVerts) {
								TVFace& tvface = realmesh.tvFace[i];
								index = d_uvLookup[tvface.getTVert(k)];		// Index the rounded values
								assert (index < Header.NumTextureVerts);
							} else {
								index = -1;
							}
							d_outstream->write((char *)&index, sizeof(index));
						
							// VertColor index
							if (realmesh.getNumVertCol()) {
								TVFace& vcface = realmesh.vcFace[i];
								index = vcface.getTVert(k);
								assert (index < realmesh.getNumVertCol());
							} else {
								index = 0;
							}

							d_outstream->write((char *)&index, sizeof(index));

/*							// TODO::biddle - Where is the normal?????
							if (Header.NumNormals) {
							//	index = face.cvrt[k];
							} else {
								index = -1;
							}
							index = 0;
							d_outstream->write((char *)&index, sizeof(index));
*/
							int normindex = -1;
							if (Header.NumNormals) {

								normindex = firstNormIndex[vindex];

								if (!(vnorms[vindex]->smooth & face.smGroup)) {

									// Search for the right smoothing group
									VNormal *vn = vnorms[vindex]->next;
									normindex++;
									while (vn && !(vn->smooth & face.smGroup)) {
										vn = vn->next;
										normindex++;
									}
								}

							}

							d_outstream->write((char *)&normindex, sizeof(normindex));

					
						}}
					
					}}

					d_materialFaceCount.push_back(curr_face_count);
				}

			}}

			{for (UINT32 i = 0; i < Header.NumFaces ; i++) {
				if (tempFaceMapping[i] == (UINT16)-1) {
					assert (tempFaceMapping[i] != (UINT16)-1);
				}
			}}

			assert (total_face_count == Header.NumFaces);

			//== Floor Faces  ============================
			
			UINT32 floor_count = 0;
			{for (UINT32 i = 0; i < Header.NumFaces ; i++) {

				MNFace* face = bmesh->F(d_faceValues[i]);

				if (face->GetFlag(FLOOR_FACE_FLAG)) {
					UINT16 index = tempFaceMapping[i];
					assert(index < Header.NumFaces);
					d_outstream->write((char *)&index, sizeof(index));
					floor_count++;
				}
				
			}}

			assert(floor_count == Header.NumFloorFaces);

			//== Water Faces  ============================
			
			UINT32 water_count = 0;
			{for (UINT32 i = 0; i < Header.NumFaces ; i++) {

				MNFace* face = bmesh->F(d_faceValues[i]);

				if (face->GetFlag(WATER_FACE_FLAG)) {
					UINT16 index = tempFaceMapping[i];
					assert(index < Header.NumFaces);
					d_outstream->write((char *)&index, sizeof(index));
					water_count++;
				}
				
			}}

			assert(water_count == Header.NumWaterFaces);

			//== Materials ==============================

			int count = d_materialFaceCount.size();
			d_outstream->write((char *)&count, sizeof(count));

			// Dump out the number of faces used by each material
			{for (int i = 0; i < count; i++) {
				int count = d_materialFaceCount[i];
				d_outstream->write((char *)&count, sizeof(count));
			}}


			ExportMaterials();



			// Tack on history/debugging information

			*d_outstream << "INFO";

			int fieldcount = 7;		// Number of strings of info we are about to write
			d_outstream->write((char *)&fieldcount, sizeof(fieldcount));

			char nbuf[256];
			DWORD nbufsize = 256;

			WNetGetUser(NULL,&nbuf[0],&nbufsize);
			*d_outstream << "USER=" << nbuf << '\0';

			gethostname(nbuf,nbufsize);
			*d_outstream << "HOST=" << nbuf << '\0';

			char* maxfname = d_interface->GetCurFilePath();
			*d_outstream << "MAXFILE=" << maxfname << '\0';

			const char* snofname = thisnode->GetName();
			*d_outstream << "SNOFILE=" << snofname << '\0';

			*d_outstream << "VERSION=" << SNOD_VERSION << '.' << SNOD_MINOR_VERSION << '\0';

			char dbuffer [9];
			char tbuffer [9];
			_strdate( dbuffer );
			_strtime( tbuffer );

			*d_outstream << "DATE=" << dbuffer << '\0';
			*d_outstream << "TIME=" << tbuffer << '\0';


			// delete the norms list
			{for (unsigned int i = 0; i < vnorms.size(); ++i) {
				delete vnorms[i];
			}}

	
		}

		catch (...) {
			char buff[512];
			sprintf(buff,"Please use STL check to look for errors in your mesh\n\nExporter cannot continue");
			MessageBox(NULL,buff,"Hey, Sailor!",MB_OK|MB_ICONSTOP );
			return TREE_IGNORECHILDREN;
		}
		
		d_exported_count++;

		d_outstream->close();

		// With the file written successfully, we run the post processing step.  This will
		// reorganize the triangle data into a form that is friendly to the game at load time
		// and will calculate redundant pathfinding information.
		PostProcess processor;
		processor.OrgAndCalcInfo( nodefilename );

		return TREE_IGNORECHILDREN;
		
	}
	
	return TREE_CONTINUE;
	
}

void SiegeNodeExportEnum::EnumerateMaterial( Mtl *MaxMaterial ) {


	Texmap *CurrTexture = MaxMaterial->GetSubTexmap(ID_DI);

	if (CurrTexture) {

		d_materialDiffuseColours.push_back(MaxMaterial->GetDiffuse());	

		d_materialMapping.push_back(d_materialTextures.size());
		d_materialTextures.push_back(CurrTexture);	

	} else {

		d_materialDiffuseColours.push_front(MaxMaterial->GetDiffuse());	
		d_materialMapping.push_back(d_materialTextures.size()-d_materialDiffuseColours.size());

	}

}

void SiegeNodeExportEnum::EnumerateMaterialTree( Mtl *RootMaterial ) {

	d_materialDiffuseColours.clear();
	d_materialTextures.clear();
	
	if (RootMaterial->IsMultiMtl()) {

		{for (int i=0; i<RootMaterial->NumSubMtls(); i++) {

			Mtl *sub_mat = RootMaterial->GetSubMtl(i);

			EnumerateMaterial(sub_mat);

		}}

	} else {

		EnumerateMaterial(RootMaterial);
	}	
}

void SiegeNodeExportEnum::ExportMaterials(void) {

	int j = 0;
	{ for (std::list<Color>::iterator i = d_materialDiffuseColours.begin();
		i != d_materialDiffuseColours.end();
		++i,++j) {

		if (d_matUsed[j]) {

			float red	= (*i).r;
			float green = (*i).g;
			float blue	= (*i).b;

			d_outstream->write((char *)&red,sizeof(float));
			d_outstream->write((char *)&green,sizeof(float));
			d_outstream->write((char *)&blue,sizeof(float));

		}
		
	}}
 
	j= d_materialDiffuseColours.size() - d_materialTextures.size();
	{ for (std::list<Texmap *>::iterator i = d_materialTextures.begin();
		i != d_materialTextures.end();
		++i,++j) {

		if (d_matUsed[j]) {
			Texmap *ActiveTexture = (*i); 
			DumpOutTheTexture(ActiveTexture);
		}
		
	}}
}


bool SiegeNodeExportEnum::DumpOutTheTexture(Texmap *ActiveTexture) {

		std::string TextureName;
		
		// See if it's a bitmap texture
//		bool MapHasAlpha = false;
//		UINT32 TextureWidth = 64;
//		UINT32 TextureHeight = 64;
//		Bitmap *MaxBitmapTexture = 0;
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

//			MapHasAlpha = (ActiveTextureCast->GetAlphaSource() ==
//						   ALPHA_FILE);
//			MaxBitmapTexture = ActiveTextureCast->GetBitmap(0);
		}
		else
		{
			TextureName += "MAP";
			TextureName += rand();
		}

		// Put the name in the file (we'll probably want it later)
		*d_outstream << TextureName.c_str();
		*d_outstream << '\0';

		/* Version 4 and up don't write texture data

		if(MaxBitmapTexture)
		{
			// We got the bitmap texture - use the bitmap directly
			TextureWidth = MaxBitmapTexture->Width();
			TextureHeight = MaxBitmapTexture->Height();
		}

		if(!(TextureWidth & (TextureWidth - 1)) &&
		   !(TextureHeight & (TextureHeight - 1)))
		{
			// The bitmap's OK
			WriteTexture(*MaxBitmapTexture,MapHasAlpha);
		}
		else				
		{
			// TODO: Round to the nearest power of 2
			TextureWidth = 1 << (int)((log(TextureWidth) / log(2)) + .5);
			TextureHeight = 1 << (int)((log(TextureHeight) / log(2)) + .5);

			// Cap
			TextureWidth = Maximum(TextureWidth, 256);
			TextureHeight = Maximum(TextureHeight, 256);
			
			BitmapInfo MAXBitmapInfo;
			MAXBitmapInfo.SetType(BMM_TRUE_32);
			MAXBitmapInfo.SetWidth(TextureWidth);
			MAXBitmapInfo.SetHeight(TextureHeight);
			MAXBitmapInfo.SetFlags(MAP_HAS_ALPHA);
			MAXBitmapInfo.SetCustomFlag(0);

			Bitmap *DeleteBitmap = TheManager->Create(&MAXBitmapInfo);
			assert(DeleteBitmap);
			ActiveTexture->RenderBitmap(0, DeleteBitmap, 1.0f, TRUE);

			WriteTexture(*DeleteBitmap,MapHasAlpha);
			DeleteBitmap->DeleteThis();
		}
		*/
		
		return true;

}

void SiegeNodeExportEnum::WriteTexture(Bitmap &MAXBitmap,
					bool const TextureMapHasAlpha)
{



	int image_type;
	char *image_data;
	
	int alpha_type;
	char *alpha_data;
	
	image_data = (char *)MAXBitmap.Storage()->GetStoragePtr(&image_type);
	assert(image_data);
	
	alpha_data = (char *)MAXBitmap.Storage()->GetStoragePtr(&alpha_type);
	if (TextureMapHasAlpha) {
		assert(image_data);
	}

	int w = MAXBitmap.Width();
	int h = MAXBitmap.Height();
	int d;
	
	d_outstream->write((char *)&TextureMapHasAlpha,sizeof(TextureMapHasAlpha));

	d_outstream->write((char *)&w,sizeof(w));
	d_outstream->write((char *)&h,sizeof(w));

	int image_size = w*h;

	switch (image_type) {
		
		case BMM_TRUE_32 :
			d = 32;
			d_outstream->write((char*)&d,sizeof(d));			
			d_outstream->write(image_data,image_size*4);			
			break;
		case BMM_TRUE_24 :
			d = 24;
			d_outstream->write((char*)&d,sizeof(d));			
			d_outstream->write(image_data,image_size*3);
			break;
	}
	

}

//********************************************************************
// Create a matrix that will transform verts from 3DSMax/Artist space
// where Z is up, characters shit down Y and the millimeter is the
// unit of measure to world space, where Y is up, they shit down Z and
// the unit of choice is the metre.

#define SKIP_THE_NODE_TRANSFORM

Matrix3 BuildVertTransformMatrix(INode *sourceMesh) {

#ifdef SKIP_THE_NODE_TRANSFORM
	Matrix3 MaxTransform(1);		// Set to identity matrix
#else
	Matrix3 MaxTransform = sourceMesh->GetNodeTM(0); // Assuming T=0;
#endif

	//w MaxTransform.PreRotateX(-HALFPI);
	//w MaxTransform.PreRotateZ(PI);
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

#ifdef SKIP_THE_NODE_TRANSFORM
	Matrix3 MaxTransform(1);		// Set to identity matrix
#else
	Matrix3 MaxTransform = sourceMesh->GetNodeTM(0); // Assuming T=0;
#endif
	
	//w MaxTransform.PreRotateX(-HALFPI);
	//w MaxTransform.PreRotateZ(PI);
	SwizzleToYisUp(MaxTransform,MaxTransform);

	return (MaxTransform);
	
}

/*
matrix_3x3 ConvertToMatrix3x3(const Matrix3& mat) {

	Point3 xrow = mat.GetRow(0);
	Point3 yrow = mat.GetRow(1);
	Point3 zrow = mat.GetRow(2);

	vector_3 xcol(xrow.x,xrow.y,xrow.z);
	vector_3 ycol(yrow.x,yrow.y,yrow.z);
	vector_3 zcol(zrow.x,zrow.y,zrow.z);
	
	matrix_3x3 tmat(xcol,ycol,zcol);
	
}
*/
/*
  The following fence  methods are useful for algorithms such as SabinDoo wherein you don't
  want to mix faces with different characteristics.

******************************
Prototype:

void FenceMaterials();

Remarks:

Sets the MN_EDGE_NOCROSS flag on all edges that lie between faces with
different material IDs.

******************************
Prototype:

void FenceSmGroups();

Remarks:

Sets the MN_EDGE_NOCROSS flag on all edges that lie between faces with exclusive
smoothing groups.

******************************
Prototype:

void FenceFaceSel();

Remarks:

Sets the MN_EDGE_NOCROSS flag on all edges that lie between selected & non-selected
faces.  (This checks the MN_SEL, not the MN_TARG, flags on the faces.)

******************************
Prototype:

void FenceOneSidedEdges();

Remarks:

Sets the MN_EDGE_NOCROSS flag on all edges that are on the boundary.

******************************
Prototype:

void FenceNonPlanarEdges(float thresh=.9999f);

Remarks:

This method is available in release 2.5 and later only.
Sets MN_EDGE_NOCROSS flags and eliminates MN_EDGE_INVIS flags to prevent faces with
different normals from being merged in MakePolyMesh or MakeConvexPolyMesh. 

Parameters:

float thresh=.9999f
This is the threshold used to determine if two adjacent faces have the same normals,
i.e. lie in the same plane.  If the dot product between the normals is less than thresh,
they are considered different, otherwise they're considered the same.  The threshold
angle between faces is the arc cosine of this amount, so for instance to set a
threshold angle of .5 degrees, you would call FenceNonPlanarEdges with a thresh of
cos(.5*PI/180.).  The default value is equivalent to about .81 degrees. 

******************************
Prototype:

void SetTVSeamFlags();

Remarks:

Sets the MN_EDGE_TV_SEAM flag on all edges whose endpoints on one face have mapping
coordinates that are offset from the mapping coordinates of the same endpoints on the
other face by the same amount.  Operating on a standard cylinder with mapping
coordinates assigned, for instance, this will set the MN_EDGE_TV_SEAM flag on the
column of edges that forms the wrap boundary for the mapping coordinates.


 */
