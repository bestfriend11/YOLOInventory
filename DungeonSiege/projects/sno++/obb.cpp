// **********************************************************************
//
//	FILE: OBB.cpp
//
// **********************************************************************

#include "precompiled_siegenodeobject.h"

// Include the requisite GPG math foundations
#include "IgnoredWarnings.h"
//#include "math_3.h"
#include "oriented_bounding_box_3.h"

#include "mnmath.h"

#include "OBBTree.h"

#include "qhull/QHull_A.h"

//#include "win32_window.h"

#include <string>
#include <list>
#include <fstream>

const UINT MAX_RECURSE_LEVEL = 16;


//static OBB theOBB;

//--- OBB -------------------------------------------------------
Point3 vector_3_to_Point3(vector_3 v) {return Point3(v.GetX(),v.GetY(),v.GetZ());}
vector_3 Point3_to_vector_3(Point3 p) {return vector_3(p[0],p[1],p[2]);}

OBB::OBB()
//	: iu(NULL)
//	, ip(NULL)
//	, hPanel(NULL)
	: RecursionLevel(0)
	, Tree(0)
//	, OutFile(0)
	, RootColourSwatch(0)
	, LeftColourSwatch(0)
	, RightColourSwatch(0)
	, CheckForSeparateMeshes(false)
{

}

OBB::~OBB()
{

}
/*
void OBB::BeginEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = iu;
	this->ip = ip;
	hPanel = ip->AddRollupPage(
		hInstance,
		MAKEINTRESOURCE(IDD_PANEL),
		OBBDlgProc,
		GetString(IDS_PARAMS),
		0);

}
	
void OBB::EndEditParams(Interface *ip,IUtil *iu) 
{
	this->iu = NULL;
	this->ip = NULL;
	ip->DeleteRollupPage(hPanel);
	hPanel = NULL;
}
*/

void OBB::Init(HWND hWnd)
{

	SendDlgItemMessage(hWnd, IDC_SLIDER1 , TBM_SETRANGE , (WPARAM)true, MAKELONG(0, 16));

	SendDlgItemMessage(hWnd, IDC_FIND_SEPARATE_MESHES , BM_SETCHECK, CheckForSeparateMeshes,0);

	// Set it up baby!
	SendMessage(hWnd, WM_VSCROLL, 0, 0);

	RootColourSwatch  = GetIColorSwatch(GetDlgItem(hWnd, IDC_CROOT),  RGB(0,255,0), _T("Set ROOT OBB Tree"));
	LeftColourSwatch  = GetIColorSwatch(GetDlgItem(hWnd, IDC_CLEFT),  RGB(255,0,0), _T("Set LEFT OBB Tree"));
	RightColourSwatch = GetIColorSwatch(GetDlgItem(hWnd, IDC_CRIGHT), RGB(0,0,255), _T("Set RIGHT OBB Tree"));
}

void OBB::Destroy(HWND hWnd)
{
	ReleaseIColorSwatch(RootColourSwatch);
	ReleaseIColorSwatch(LeftColourSwatch);
	ReleaseIColorSwatch(RightColourSwatch);

	if (Tree) {
		delete Tree;
		Tree = 0;
	}

}

//--- Start of stuff that was NOT generated -------------------------------------------------------

UINT OBB::SetRecursionLevel(const UINT& NewLevel)
{
	RecursionLevel = Minimum(NewLevel,MAX_RECURSE_LEVEL);
	return RecursionLevel;
}

/*
bool OBB::BrowseFileName(HWND hWnd)
{
	if (OutFileName.length() == 0) {
		OutFileName = "DEFAULT.OBB";
	}

	if (!QueryForWritableFile(hWnd,OutFileName.c_str(),"Select OBB file",OutFileName)) {
		OutFileName = "";
	};
	
	SendDlgItemMessage(hWnd, IDC_OBBFILENAME , WM_SETTEXT, 0,(long)OutFileName.c_str());

	return (OutFileName.length() != 0);
}
*/

void DBPrintMatrix(char *name,Matrix3& m) {

	Point3 p; 
	DebugPrint(_T("\n%s: ="),name);
	
	p = m.GetRow(0); 
	DebugPrint(_T("\n%12.6f,%12.6f,%12.6f"), p.x,p.y,p.z);
	
	p = m.GetRow(1); 
	DebugPrint(_T("\n%12.6f,%12.6f,%12.6f"), p.x,p.y,p.z);

	p = m.GetRow(2); 
	DebugPrint(_T("\n%12.6f,%12.6f,%12.6f"), p.x,p.y,p.z);

	p = m.GetRow(3); 
	DebugPrint(_T("\n%12.6f,%12.6f,%12.6f\n"), p.x,p.y,p.z);

}

void DBPrintPoint3(char *name,Point3& p) {

	DebugPrint(_T("\n%s: ="),name);

	DebugPrint(_T("\n%12.6f,%12.6f,%12.6f\n"), p.x,p.y,p.z);
	
}


void DBPrintValue(char *name,float& v) {

	DebugPrint(_T("\n%s: ="),name);

	DebugPrint(_T("\n%12.6f\n"), v);
	
}


bool OBB::WriteOBBFile(HWND hWnd)
{
/*
	if (!theOBBTree) {
		// First need to construct it
		MessageBox(hWnd, _T("Construct the OBB tree before you attempt to write it"), 
			_T("Error"), MB_ICONERROR);
	
		return false;
	}

	if (!BrowseFileName(hWnd)) {
		return false;
	}

	std::ofstream OBBStreamOut(OutFileName.c_str(),std::ios::binary);

	if (!OBBStreamOut.is_open()) {
		MessageBox(hWnd, _T("Cannot open OBB file"), _T("Error"), MB_ICONERROR);
		return false;
	}

	if (!theOBBTree->Write(OBBStreamOut,0)) {
		MessageBox(hWnd, _T("Error writing OBB file"), _T("Error"), MB_ICONERROR);
		return false;
	};

	OBBStreamOut.close();

// Test to make sure that the file is 'ok'

	std::ifstream OBBStreamIn(OutFileName.c_str(),std::ios::binary);

	if (!OBBStreamIn.is_open()) {
		MessageBox(hWnd, _T("Cannot open OBB file"), _T("Error"), MB_ICONERROR);
		return false;
	}

	OBBNode inOBBTree;

	bool BadInput = inOBBTree.Read(OBBStreamIn,0);
	assert(!BadInput);

	OBBStreamIn.close();

	MessageBox(hWnd, _T("OBB file written"), _T("Success"), MB_ICONINFORMATION);
*/
	return true;
}

void OBB::Create(IObjParam *iObjParams,BoundaryMesh& bmesh, INode *sourcenode)
{

/*

	MNMesh unionmesh; 

	int numpoints=0;
	
	
	if (ip->GetSelNodeCount() != 1) {

		// Should tell the user to glue stuff together!
		return;

	}

	TimeValue t = ip->GetTime();
		
	INode *node = ip->GetSelNode(0);

	
	assert(node);

	Object *obj = node->EvalWorldState(t).obj;
	TriObject *tri;

	if (obj->CanConvertToType(triObjectClassID)) { 
		tri = (TriObject *) obj->ConvertToType(0,triObjectClassID);
		numpoints = tri->mesh.getNumVerts();
	}

	if (!(numpoints > 0)) {
	return;	//BAIL EARLY
	}
*/

	Matrix3 RootTM(1);

	// Compute the pivot TM
	Matrix3 pivotTM(1);
	pivotTM.SetTrans(sourcenode->GetObjOffsetPos());
	PreRotateMatrix(pivotTM,sourcenode->GetObjOffsetRot());
	ApplyScaling(pivotTM,sourcenode->GetObjOffsetScale());

	if (sourcenode->GetObjTMAfterWSM(0).IsIdentity()) {
		// It's in world space, so put it back into object
		// space.  We can do this by computing the inverse
		// of the matrix returned before any world space
		// modifiers were applied.
		RootTM = Inverse(sourcenode->GetObjTMBeforeWSM(0));

	} else {
		// It's in object space, get the Object TM.
		RootTM = sourcenode->GetObjectTM(0);
	}


	MNMesh unionmesh; 
	unionmesh = MNMesh(bmesh);

	unionmesh.FillInMesh();

	{for (int k = 0 ; k < unionmesh.VNum() ; k++ ) {
		unionmesh.P(k) = unionmesh.P(k) * pivotTM;
	}}


	if (Tree) {
		delete Tree;
		Tree = 0;
	}

	Tree = BuildOBBTree(unionmesh,"",0,false,false); 

	if (!theHold.Holding()) {
		theHold.Begin();
	}

	DrawOBBNode(*Tree,
		iObjParams,sourcenode,true,"",
		RootColourSwatch->GetColor(),
		LeftColourSwatch->GetColor(),
		RightColourSwatch->GetColor());

	theHold.Accept("Construct OBBTree");

	iObjParams->RedrawViews(0);


}



//*************************
OBBNode *OBB::
BuildOBBTree(MNMesh& treemesh,
		const char const *tag,
		const int level,
		const bool& DrawMesh, const bool& DrawBox) {

	if (level < 0) return NULL;

	OBBNode* CurrNode = new OBBNode;

	Point3 MeshMeanPoint;
	double HullVolume = 0;
	int TotalHullPoints = 0;

	assert (treemesh.VNum() > 2);
	
	if (treemesh.VNum() < 4) {
		PlanarOBB(CurrNode->mutableOBB(), treemesh, MeshMeanPoint);
	} else {
		ConvexHullOBB(CurrNode->mutableOBB(), treemesh, MeshMeanPoint, HullVolume ,TotalHullPoints);
	}

	vector_3 bboxsize = CurrNode->OBB().GetHalfDiagonal();
	matrix_3x3 bboxorient = CurrNode->OBB().GetOrientation();
	vector_3 bboxcenter = CurrNode->OBB().GetCenter();

	// Should we split this OBB ?

	float VolumeDifference = fabs(HullVolume - (2*bboxsize.GetX() * 2*bboxsize.GetY() * 2*bboxsize.GetZ()));

	#define THRESH_RATIO 0.005

	if (((unsigned int)level < RecursionLevel) && ((treemesh.VNum() != TotalHullPoints) || (VolumeDifference > (THRESH_RATIO*HullVolume))) ) {

		Matrix3 LocalTM(1);

		LocalTM.SetRow(0,vector_3_to_Point3(bboxorient.GetColumn_0()));
		LocalTM.SetRow(1,vector_3_to_Point3(bboxorient.GetColumn_1()));
		LocalTM.SetRow(2,vector_3_to_Point3(bboxorient.GetColumn_2()));
		LocalTM.SetRow(3,vector_3_to_Point3(bboxcenter));

		Matrix3 InverseLocalTM = Inverse(LocalTM);

		MNMesh lowermesh;
		MNMesh uppermesh;

		{for (int v = 0; v < treemesh.VNum(); ++v ) {
			treemesh.V(v)->SetFlag(MN_DEAD|MN_USER,false);
		}}

		{for (int f = 0; f < treemesh.FNum(); ++f ) {
			treemesh.F(f)->SetFlag(MN_DEAD|MN_USER,false);
		}}
		{for (int e = 0; e < treemesh.ENum(); ++e ) {
			treemesh.E(e)->SetFlag(MN_DEAD|MN_USER,false);
		}}

		// Can we identify two separate meshes in the data?

		int edges_visited = 0;

		if (CheckForSeparateMeshes) {

#if 0  /// This is QUITE busted in Max3.0 -- biddle
			list<int> VertSet;

			VertSet.push_back(0);
			treemesh.V(0)->SetFlag(MN_USER,true);

			while (VertSet.size() > 0) {

				int curr_vert_index = *VertSet.begin();
				VertSet.pop_front();

				MNVert *curr_vert = treemesh.V(curr_vert_index);

				for (int i = 0; i < curr_vert->edg.Count(); ++i) {

					int curr_edge = curr_vert->edg[i];

					if (!treemesh.E(curr_edge)->GetFlag(MN_USER)) {

						treemesh.E(curr_edge)->SetFlag(MN_USER,true);

						int next_vert_index;

						if (curr_vert_index == treemesh.E(curr_edge)->v1) {
							// forward edge
							next_vert_index = treemesh.E(curr_edge)->v2;
						} else {
							// backward edge
							next_vert_index = treemesh.E(curr_edge)->v1;
						}

						edges_visited++;

						if (!treemesh.V(next_vert_index)->GetFlag(MN_USER)) {

							treemesh.V(next_vert_index)->SetFlag(MN_USER,true);
							VertSet.push_back(next_vert_index);

						}

					}
				}

			}
#endif   /// This is QUITE busted in Max3.0 -- biddle
		}

		assert(0);


		if (0 && CheckForSeparateMeshes  && (edges_visited != treemesh.ENum())) {

#if 0  /// This is QUITE busted in Max3.0 -- biddle

			lowermesh = treemesh;
			lowermesh.FillInMesh();

			uppermesh = treemesh;
			uppermesh.FillInMesh();

			//Set all the verts and faces of the lowermesh to DEAD,
			// while the uppermesh is NOT DEAD

			{for (int f = 0; f < treemesh.FNum(); ++f ) {
				lowermesh.F(f)->SetFlag(MN_DEAD,true);
				uppermesh.F(f)->SetFlag(MN_DEAD,false);

			}}

			{for (int v = 0; v < treemesh.VNum(); ++v ) {

				lowermesh.V(v)->SetFlag(MN_DEAD,true);
				uppermesh.V(v)->SetFlag(MN_DEAD,false);

				lowermesh.P(v) = treemesh.P(v) * InverseLocalTM;
				uppermesh.P(v) = treemesh.P(v) * InverseLocalTM;

			}}

			{for (int curr_vert=0; curr_vert < treemesh.VNum(); ++curr_vert) {

				bool IsInLower = treemesh.V(curr_vert)->GetFlag(MN_USER);

				lowermesh.V(curr_vert)->SetFlag(MN_DEAD,!IsInLower);
				uppermesh.V(curr_vert)->SetFlag(MN_DEAD,IsInLower);

				{for (int i = 0; i < treemesh.V(curr_vert)->fac.Count(); ++i) {

					int curr_face = treemesh.V(curr_vert)->fac[i];

					if (!treemesh.F(curr_face)->GetFlag(MN_USER)) {

						treemesh.F(curr_face)->SetFlag(MN_USER,true);

						lowermesh.F(curr_face)->SetFlag(MN_DEAD,!IsInLower);
						uppermesh.F(curr_face)->SetFlag(MN_DEAD,IsInLower);


					}
				}}

			}}

#endif
		} else {

			//*******************
			// Partition the treemesh into Upper and Lower halves

			// Grab the axes in order from longest (axisV0) to shortest (axisV2)

			vector_3 axisV0,axisV1,axisV2;

			if (bboxsize.GetX() >= bboxsize.GetY()) {
				if (bboxsize.GetX() >= bboxsize.GetZ()) {
					axisV0 = bboxorient.GetColumn_0();
					if (bboxsize.GetY() >= bboxsize.GetZ()) {
						axisV1 = bboxorient.GetColumn_1();
						axisV2 = bboxorient.GetColumn_2();
					} else {
						axisV1 = bboxorient.GetColumn_2();
						axisV2 = bboxorient.GetColumn_1();
					}
				} else {
					axisV0 = bboxorient.GetColumn_2();
					axisV1 = bboxorient.GetColumn_0();
					axisV2 = bboxorient.GetColumn_1();
				}
			} else {
				if (bboxsize.GetY() >= bboxsize.GetZ()) {
					axisV0 = bboxorient.GetColumn_1();
					if (bboxsize.GetX() >= bboxsize.GetZ()) {
						axisV1 = bboxorient.GetColumn_0();
						axisV2 = bboxorient.GetColumn_2();
					} else {
						axisV1 = bboxorient.GetColumn_2();
						axisV2 = bboxorient.GetColumn_0();
					}
				} else {
					axisV0 = bboxorient.GetColumn_2();
					axisV1 = bboxorient.GetColumn_1();
					axisV2 = bboxorient.GetColumn_0();
				}
			}

			Point3 UpNormal;

			Point3 facecentroid;

			bool LowerEmpty = true;
			bool UpperEmpty = true;

			UpNormal = vector_3_to_Point3(axisV0);

			{for (int f = 0; f < treemesh.FNum() && (UpperEmpty || LowerEmpty); f++ ) {

				treemesh.ComputeCenter(f,facecentroid);
				facecentroid -=  MeshMeanPoint;

				float dp = DotProd(UpNormal,facecentroid);
				if ( dp > 0 ) {
					UpperEmpty = false;
				} else {
					LowerEmpty = false;
				}
			}}
			
			if (UpperEmpty || LowerEmpty) {

				LowerEmpty = true;
				UpperEmpty = true;

				UpNormal = vector_3_to_Point3(axisV1);

				for (int f = 0; f < treemesh.FNum() && (UpperEmpty || LowerEmpty); f++ ) {

					treemesh.ComputeCenter(f,facecentroid);
					facecentroid -=  MeshMeanPoint;

					float dp = DotProd(UpNormal,facecentroid);
					if ( dp > 0 ) {
						UpperEmpty = false;
					} else {
						LowerEmpty = false;
					}
				}

			} 

			if (UpperEmpty || LowerEmpty) {

				LowerEmpty = true;
				UpperEmpty = true;

				UpNormal = vector_3_to_Point3(axisV2);

				for (int f = 0; f < treemesh.FNum() && (UpperEmpty || LowerEmpty); f++ ) {

					treemesh.ComputeCenter(f,facecentroid);
					facecentroid -=  MeshMeanPoint;

					float dp = DotProd(UpNormal,facecentroid);
					if ( dp > 0 ) {
						UpperEmpty = false;
					} else {
						LowerEmpty = false;
					}
				}
			}

			if (UpperEmpty || LowerEmpty) {
//				treemesh.Clear();
				return 0;
			}

			{for (int v = 0; v < treemesh.VNum(); v++ ) {

				lowermesh.V(lowermesh.NewVert(treemesh.P(v) * InverseLocalTM))->SetFlag(MN_DEAD,true);
				uppermesh.V(uppermesh.NewVert(treemesh.P(v) * InverseLocalTM))->SetFlag(MN_DEAD,true);

			}}


			{for (int f = 0; f < treemesh.FNum(); f++ ) {

				treemesh.ComputeCenter(f,facecentroid);
				facecentroid -=  MeshMeanPoint;

				int A = treemesh.F(f)->vtx[0];
				int B = treemesh.F(f)->vtx[1];
				int C = treemesh.F(f)->vtx[2];

				float dp = DotProd(UpNormal,facecentroid);
				if ( dp > 0 ) {

					uppermesh.NewTri(A,B,C);
					uppermesh.V(A)->SetFlag(MN_DEAD,false);
					uppermesh.V(B)->SetFlag(MN_DEAD,false);
					uppermesh.V(C)->SetFlag(MN_DEAD,false);

				} else {

					lowermesh.NewTri(A,B,C);
					lowermesh.V(A)->SetFlag(MN_DEAD,false);
					lowermesh.V(B)->SetFlag(MN_DEAD,false);
					lowermesh.V(C)->SetFlag(MN_DEAD,false);

				}

			}}

		}

		lowermesh.CollapseDeadStructs();
		lowermesh.FillInMesh();

		uppermesh.CollapseDeadStructs();
		uppermesh.FillInMesh();

		int ucount = uppermesh.FNum();
		int lcount = lowermesh.FNum();
		int tcount = treemesh.FNum();

		if ((ucount + lcount) != tcount) {
//			MessageBox(hDlg, _T("Lost some faces in the process!"), _T("Error"), MB_ICONERROR);
		}

//#define SWEET_BABY_JESUS_GIVE_ME_STRENGTH
#ifdef SWEET_BABY_JESUS_GIVE_ME_STRENGTH

		if (level+2 >= RecursionLevel) {

			TriObject* lowerobj = (TriObject *)ip->CreateInstance(GEOMOBJECT_CLASS_ID,triObjectClassID);
			INode* lowernode = ip->CreateObjectNode(lowerobj);
			lowernode->SetWireColor(RootColourSwatch->GetColor());

			char sname[32];
			sprintf(sname,"LOWER_HALF_%s",tag);
			lowernode->SetName(sname);

			lowermesh.OutToTri(lowerobj->mesh);

			TriObject* upperobj = (TriObject *)ip->CreateInstance(GEOMOBJECT_CLASS_ID,triObjectClassID);
			INode* uppernode = ip->CreateObjectNode(upperobj);
			uppernode->SetWireColor(RootColourSwatch->GetColor()/2);
			uppermesh.OutToTri(upperobj->mesh);

			sprintf(sname,"UPPER_HALF_%s",tag);
			uppernode->SetName(sname);

			TriObject* treeobj = (TriObject *)ip->CreateInstance(GEOMOBJECT_CLASS_ID,triObjectClassID);
			INode* treenode = ip->CreateObjectNode(treeobj);
			treenode->SetWireColor(RootColourSwatch->GetColor()/4);

			sprintf(sname,"TREE_%s",tag);
			treenode->SetName(sname);

			treemesh.OutToTri(treeobj->mesh);

		}
#endif

		treemesh.Clear();

		char newtag[17];

		if (lcount == 0) {
			CurrNode->SetLeftKid(0);
		} else {
			sprintf(newtag,"%sL",tag);
			CurrNode->SetLeftKid(BuildOBBTree(lowermesh,newtag,level+1,false,false));
		}

		if (ucount == 0) {
			CurrNode->SetRightKid(0);
		} else {
			sprintf(newtag,"%sU",tag);
			CurrNode->SetRightKid(BuildOBBTree(uppermesh,newtag,level+1,false,false));
		}

	} else {

		CurrNode->SetLeftKid(0);
		CurrNode->SetRightKid(0);

	}

	return (CurrNode);

}

//*************************
void
OBB::
ConvexHullOBB(
	oriented_bounding_box_3& BBox,
	const MNMesh& unionmesh,Point3& MeanMeshPoint, double &HullVolume, int &TotalHullPoints) {

	// covariance matrix
	matrix_3x3 CovMat;

	coordT* PointBuffer = new coordT[3*(unionmesh.VNum()+1)];
	coordT* p = PointBuffer;
	coordT* DummyPointBuffer = 0;

	MeanMeshPoint = Point3(0,0,0);

	{for (int k = 0 ; k<unionmesh.VNum() ; k++ ) {

		Point3 worldpoint = unionmesh.P(k);
		*p++ = worldpoint.x;
		*p++ = worldpoint.y;
		*p++ = worldpoint.z;
		MeanMeshPoint += worldpoint;
	}}

	// Calculate the mean of all points in the set, we will need it later when
	// we go to partition the OBBTree

	MeanMeshPoint /= unionmesh.VNum();
	int exitcode;             /* 0 if no error from qhull */
	int curlong, totlong;	    /* memory remaining after qh_memfreeshort */

	int dim = 3;

	exitcode= qh_new_qhull(dim, unionmesh.VNum(), PointBuffer, false,
              "qhull Tv Qs", NULL, NULL); 

	if (exitcode == 2) {

		qh_freeqhull(!qh_ALL);
		qh_memfreeshort (&curlong, &totlong);
		assert(!(curlong || totlong));

#if 1
		// Add in a dummy point so we can guarantee that the
		// all the points in the PointBuffer are not co-planar

		Point3 A = unionmesh.P(0);
		Point3 B = unionmesh.P(1);
		Point3 C = unionmesh.P(2);

		Point3 DummyPoint = CrossProd(B-A,C-A) + A;

		DummyPointBuffer = p;

		*p++ = DummyPoint.x;
		*p++ = DummyPoint.y;
		*p++ = DummyPoint.z;

		exitcode= qh_new_qhull(dim, unionmesh.VNum()+1, PointBuffer, false,
              "qhull Tv Qs", NULL, NULL); 

#else
		qh_getarea(qh facet_list);
		

		TotalHullPoints = qh num_vertices;

		qh_freeqhull(!qh_ALL);
		qh_memfreeshort (&curlong, &totlong);
		assert(!(curlong || totlong));

		TotalHullPoints--;
		HullVolume = 0;


		return PlanarOBB(unionmesh);
#endif

	}

	assert(exitcode==0);

	qh_getarea(qh facet_list);

	HullVolume = qh totvol;
	TotalHullPoints = qh num_vertices;

	Point3 hullcentroid(0.0f,0.0f,0.0f);
	float hullarea=0.0;

	vertexT *vertex;
	int vertex_i,vertex_n;

	Point3 HullCenterPoint(0,0,0); 
	FORALLvertices {
		HullCenterPoint.x += vertex->point[0];
		HullCenterPoint.y += vertex->point[1];
		HullCenterPoint.z += vertex->point[2];
	}

	HullCenterPoint /= TotalHullPoints;

	facetT *facet;	    /* set by FORALLfacets */

	FORALLfacets {


		float facearea = qh_facetarea(facet);

		// Sorts the vertex into qh_ORIENTclock order (set for counterclock)
		setT *vertices= qh_facet3vertex( facet );
//		vertexT *vertex;
//		int vertex_i,vertex_n;

		matrix_3x3 VertMat = ZeroMatrix();
		Point3 facecentroid(0.0f,0.0f,0.0f);
		
		bool SkipFacet = false;

		FOREACHvertex_i_(vertices) {

			// Is this the dummy point?

			if (DummyPointBuffer == vertex->point) {

				HullVolume = 0;
				SkipFacet = true;
				break;

			} else {

				facecentroid.x += vertex->point[0];
				facecentroid.y += vertex->point[1];
				facecentroid.z += vertex->point[2];

//				{for (int i = 0; i < 3 ; i++ ) {
//					for (int j = i; j < 3 ; j++ ) {
//						VertMat(i,j) += vertex->point[i] * vertex->point[j];
//					}
//				}}


//				VertMat._ij += vertex->point[i] * vertex->point[j];

				VertMat.SetElement_00(VertMat.GetElement_00() + (vertex->point[0] * vertex->point[0]));
				VertMat.SetElement_01(VertMat.GetElement_01() + (vertex->point[0] * vertex->point[1]));
				VertMat.SetElement_02(VertMat.GetElement_02() + (vertex->point[0] * vertex->point[2]));

				VertMat.SetElement_10(VertMat.GetElement_10() + (vertex->point[1] * vertex->point[0]));
				VertMat.SetElement_11(VertMat.GetElement_11() + (vertex->point[1] * vertex->point[1]));
				VertMat.SetElement_12(VertMat.GetElement_12() + (vertex->point[1] * vertex->point[2]));

				VertMat.SetElement_20(VertMat.GetElement_20() + (vertex->point[2] * vertex->point[0]));
				VertMat.SetElement_21(VertMat.GetElement_21() + (vertex->point[2] * vertex->point[1]));
				VertMat.SetElement_22(VertMat.GetElement_22() + (vertex->point[2] * vertex->point[2]));

			}


		}

		if (SkipFacet) continue;

		hullarea += qh_facetarea(facet);

		hullcentroid += (facecentroid/vertex_n)*facearea;

//		{for (int i = 0; i < 3 ; i++ ) {
//			for (int j = i; j < 3 ; j++ ) {
//
//				VertMat(i,j) += facecentroid[i] * facecentroid[j];
//				VertMat(i,j) *= facearea / (vertex_n*(vertex_n+1));
//
//				// Add the value for this vertex to the CovMat
//				CovMat(i,j) += VertMat(i,j);
//
//			}
//		}}


		float tmp = facearea / (vertex_n*(vertex_n+1));

		VertMat.SetElement_00((VertMat.GetElement_00() + (facecentroid[0] * facecentroid[0])) * tmp);
		VertMat.SetElement_01((VertMat.GetElement_01() + (facecentroid[0] * facecentroid[1])) * tmp);
		VertMat.SetElement_02((VertMat.GetElement_02() + (facecentroid[0] * facecentroid[2])) * tmp);

		VertMat.SetElement_10((VertMat.GetElement_10() + (facecentroid[1] * facecentroid[0])) * tmp);
		VertMat.SetElement_11((VertMat.GetElement_11() + (facecentroid[1] * facecentroid[1])) * tmp);
		VertMat.SetElement_12((VertMat.GetElement_12() + (facecentroid[1] * facecentroid[2])) * tmp);

		VertMat.SetElement_20((VertMat.GetElement_20() + (facecentroid[2] * facecentroid[0])) * tmp);
		VertMat.SetElement_21((VertMat.GetElement_21() + (facecentroid[2] * facecentroid[1])) * tmp);
		VertMat.SetElement_22((VertMat.GetElement_22() + (facecentroid[2] * facecentroid[2])) * tmp);


		CovMat.SetElement_00(CovMat.GetElement_00() + VertMat.GetElement_00());
		CovMat.SetElement_01(CovMat.GetElement_01() + VertMat.GetElement_01());
		CovMat.SetElement_02(CovMat.GetElement_02() + VertMat.GetElement_02());

		CovMat.SetElement_10(CovMat.GetElement_10() + VertMat.GetElement_10());
		CovMat.SetElement_11(CovMat.GetElement_11() + VertMat.GetElement_11());
		CovMat.SetElement_12(CovMat.GetElement_12() + VertMat.GetElement_12());

		CovMat.SetElement_20(CovMat.GetElement_20() + VertMat.GetElement_20());
		CovMat.SetElement_21(CovMat.GetElement_21() + VertMat.GetElement_21());
		CovMat.SetElement_22(CovMat.GetElement_22() + VertMat.GetElement_22());

		qh_settempfree(&vertices);

	}

	hullcentroid /= hullarea;

//	{for (int i = 0; i < 3 ; i++ ) {
//		for (int j = i; j < 3 ; j++ ) {
//
//			CovMat(i,j) -= hullcentroid[i] * hullcentroid[j] * hullarea;
//
//			CovMat(j,i) = CovMat(i,j);	 // Symmetric
//
//		}
//	}}

	CovMat.SetElement_00(CovMat.GetElement_00() - (hullcentroid[0] * hullcentroid[0] * hullarea));
	CovMat.SetElement_01(CovMat.GetElement_01() - (hullcentroid[0] * hullcentroid[1] * hullarea));
	CovMat.SetElement_02(CovMat.GetElement_02() - (hullcentroid[0] * hullcentroid[2] * hullarea));

	CovMat.SetElement_10(CovMat.GetElement_01());
	CovMat.SetElement_11(CovMat.GetElement_11() - (hullcentroid[1] * hullcentroid[1] * hullarea));
	CovMat.SetElement_12(CovMat.GetElement_12() - (hullcentroid[1] * hullcentroid[2] * hullarea));

	CovMat.SetElement_20(CovMat.GetElement_02());
	CovMat.SetElement_21(CovMat.GetElement_12());
	CovMat.SetElement_22(CovMat.GetElement_22() - (hullcentroid[2] * hullcentroid[2] * hullarea));


	qh_freeqhull(!qh_ALL);
	qh_memfreeshort (&curlong, &totlong);
	assert(!(curlong || totlong));

	delete [] PointBuffer;

	ExtractBBoxFromCovMat(BBox,unionmesh,CovMat,hullcentroid,HullCenterPoint);

}  // end of subroutine


//*************************
void 
OBB::
ExtractBBoxFromCovMat(
	oriented_bounding_box_3& BBox,
	const MNMesh& unionmesh,matrix_3x3& CovMat,const Point3& centroid,const Point3& centerpoint) {

	matrix_3x3 EigVecs;
	Eigenvectors(CovMat,EigVecs);

	vector_3 V0 = Normalize(EigVecs.GetColumn_0());
	vector_3 V1 = Normalize(EigVecs.GetColumn_1());
	vector_3 V2 = Normalize(EigVecs.GetColumn_2());

	matrix_3x3 BoxOrientation = MatrixColumns(V0,V1,V2);
	
	Point3 P0 = Normalize(vector_3_to_Point3(EigVecs.GetColumn_0()));
	Point3 P1 = Normalize(vector_3_to_Point3(EigVecs.GetColumn_1()));
	Point3 P2 = Normalize(vector_3_to_Point3(EigVecs.GetColumn_2()));

	float Min0 = FLOAT_INFINITE;
	float Min1 = FLOAT_INFINITE;
	float Min2 = FLOAT_INFINITE;
	
	float Max0 = -FLOAT_INFINITE;
	float Max1 = -FLOAT_INFINITE;
	float Max2 = -FLOAT_INFINITE;

	// Only need to check points on the hull here....
	{for (int k = 0; k < unionmesh.VNum(); k++ ) {

		Point3 PT = unionmesh.P(k);

		float dp;

		dp = DotProd(PT,P0);
		Min0 = Minimum(Min0,dp);
		Max0 = Maximum(Max0,dp);

		dp = DotProd(PT,P1);
		Min1 = Minimum(Min1,dp);
		Max1 = Maximum(Max1,dp);

		dp = DotProd(PT,P2);
		Min2 = Minimum(Min2,dp);
		Max2 = Maximum(Max2,dp);

	}}
		
	vector_3 BoundingSize((Max0-Min0)/2,(Max1-Min1)/2,(Max2-Min2)/2);
	vector_3 BoundingCenter((Max0+Min0)/2,(Max1+Min1)/2,(Max2+Min2)/2);
	BoundingCenter = BoxOrientation*BoundingCenter;

	BBox.SetCenter(BoundingCenter);
	BBox.SetHalfDiagonal(BoundingSize);
	BBox.SetOrientation(BoxOrientation);

}

//*************************
void 
OBB::
PlanarOBB(
  	oriented_bounding_box_3& BBox,
	const MNMesh& unionmesh, Point3& MeanMeshPoint)
{
	
	Point3 A,B,C;

	double LenA = 0;
	double LenB = 0;
	double LenC = 0;

	MeanMeshPoint = Point3(0,0,0);
	A = MeanMeshPoint;

	// Nice little N^2 loop to find the longest (A) and second longest (B) edges in the set

	{for (int p = 0 ; p<unionmesh.VNum()-1; p++) {

		MeanMeshPoint += unionmesh.P(p);

		{for (int q = p+1 ; q<unionmesh.VNum(); q++) {

			Point3 C = unionmesh.P(p)-unionmesh.P(q);

			LenC = Length(C);

			if (LenC > LenA) {

				B = A;
				A = C;
				
				LenB = LenA;
				LenA = LenC ;

			} else if (LenC > LenB) {

				B = C;

				LenB = LenC;
			}

		}}
	}}


	if (fabs(LenA-LenB) < 0.05 * LenA) {	// Within 5%
		A += B;	
	}


	MeanMeshPoint += unionmesh.P(unionmesh.VNum()-1);
	MeanMeshPoint /= unionmesh.VNum();

	vector_3 V0 = Point3_to_vector_3(A);
	vector_3 V1 = Point3_to_vector_3(B);
	vector_3 V2;

	// Not terribly stable here! --biddle

	V1 = CrossProduct(V1,V0);
	V1 = CrossProduct(V0,V1);
	V2 = CrossProduct(V1,V0);

	V0 = Normalize(V0);
	V1 = Normalize(V1);
	V2 = Normalize(V2);

	float Min0 = FLOAT_INFINITE;
	float Min1 = FLOAT_INFINITE;
	
	float Max0 = -FLOAT_INFINITE;
	float Max1 = -FLOAT_INFINITE;

	{for (int k = 0; k < 3; k++ ) {

		Point3 test = unionmesh.P(k);

		vector_3 VT(test.x,test.y,test.z);

		float dp;

		dp = InnerProduct(VT,V0);
		Min0 = Minimum(Min0,dp);
		Max0 = Maximum(Max0,dp);

		dp = InnerProduct(VT,V1);
		Min1 = Minimum(Min1,dp);
		Max1 = Maximum(Max1,dp);


	}}

	matrix_3x3 BoxOrientation = MatrixColumns(V0,V1,V2);
	vector_3 BoundingSize((Max0-Min0)/2,(Max1-Min1)/2,0);
	vector_3 BoundingCenter = vector_3((Max0+Min0)/2,(Max1+Min1)/2,0);

	BBox.SetCenter(BoundingCenter);
	BBox.SetHalfDiagonal(BoundingSize);
	BBox.SetOrientation(BoxOrientation);

}

//*************************
void 
OBB::
DrawOBBNode(
	const OBBNode& CurrNode,
	Interface* ip,
	INode *ParentNode,
	const bool& DrawBox,
	const char const *tag,	
	const COLORREF& BBoxColour,
	const COLORREF& LeftBBoxColour,
	const COLORREF& RightBBoxColour
	) {

	Matrix3 LocalTM(1);

	matrix_3x3 borient = CurrNode.OBB().GetOrientation();
	vector_3 bcenter =  CurrNode.OBB().GetCenter();
	vector_3 bsize = CurrNode.OBB().GetHalfDiagonal();

	Point3 x_basis = vector_3_to_Point3(borient.GetColumn_0());
	Point3 y_basis = vector_3_to_Point3(borient.GetColumn_1());
	Point3 z_basis = vector_3_to_Point3(borient.GetColumn_2());

	Point3 local_orig = vector_3_to_Point3(bcenter);

	LocalTM.SetRow(0,x_basis);
	LocalTM.SetRow(1,y_basis);
	LocalTM.SetRow(2,z_basis);
	LocalTM.SetRow(3,local_orig);

	INode* bboxnode = NULL;


	// Let's display the bounding box so we can make sure we are on the right track

	MNMesh bboxmesh;

	int A = bboxmesh.NewVert( Point3( -bsize.GetX(), -bsize.GetY(), -bsize.GetZ() ) );
	int B = bboxmesh.NewVert( Point3( -bsize.GetX(), +bsize.GetY(), -bsize.GetZ() ) );
	int C = bboxmesh.NewVert( Point3( +bsize.GetX(), +bsize.GetY(), -bsize.GetZ() ) );
	int D = bboxmesh.NewVert( Point3( +bsize.GetX(), -bsize.GetY(), -bsize.GetZ() ) );
	int E = bboxmesh.NewVert( Point3( -bsize.GetX(), -bsize.GetY(), +bsize.GetZ() ) );
	int F = bboxmesh.NewVert( Point3( -bsize.GetX(), +bsize.GetY(), +bsize.GetZ() ) );
	int G = bboxmesh.NewVert( Point3( +bsize.GetX(), +bsize.GetY(), +bsize.GetZ() ) );
	int H = bboxmesh.NewVert( Point3( +bsize.GetX(), -bsize.GetY(), +bsize.GetZ() ) );

	bboxmesh.NewQuad(A,B,C,D);
	bboxmesh.NewQuad(F,E,H,G);
	bboxmesh.NewQuad(E,A,D,H);
	bboxmesh.NewQuad(H,D,C,G);
	bboxmesh.NewQuad(G,C,B,F);
	bboxmesh.NewQuad(F,B,A,E);

	TriObject* bboxobj = (TriObject *)ip->CreateInstance(GEOMOBJECT_CLASS_ID,triObjectClassID);
	bboxnode = ip->CreateObjectNode(bboxobj);

	bboxnode->SetWireColor(BBoxColour);
	bboxnode->SetNodeTM(0,LocalTM);

	if (ParentNode) {
		ParentNode->AttachChild(bboxnode,0);
	}

	char sname[32];
	sprintf(sname,"OBB_%s",tag);
	bboxnode->SetName(sname);

	bboxmesh.OutToTri(bboxobj->mesh);

	if (CurrNode.ValidLeftKid() && CurrNode.ValidRightKid()) {
		bboxnode->Hide(true);
	}

	char newtag[17];


	if (CurrNode.ValidLeftKid()) {
		sprintf(newtag,"%sL",tag);
		DrawOBBNode(CurrNode.LeftKid(),ip,bboxnode,DrawBox,newtag,LeftBBoxColour,LeftBBoxColour,RightBBoxColour);
	} 

	if (CurrNode.ValidRightKid()) {
		sprintf(newtag,"%sR",tag);
		DrawOBBNode(CurrNode.RightKid(),ip,bboxnode,DrawBox,newtag,RightBBoxColour,LeftBBoxColour,RightBBoxColour);
	}


}


//*****************************************
// Needed for qhull error messages
//*****************************************

extern "C" {
void DBugPrint(va_list l);
}

void DBugPrint(va_list l) {
	DebugPrint(l);
}
