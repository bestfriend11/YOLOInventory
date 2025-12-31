/**********************************************************************
 **********************************************************************/

#ifndef OBB_H
#define OBB_H

#include "IgnoredWarnings.h"
#include "obbtree\OBBTree.h"

class BoundaryMesh;	// from boundary.h

using namespace std;

//#include "utilapi.h"
//class OBB : public UtilityObj {

class OBB /*: public Object*/ {

	public:

//		IUtil *iu;
//		Interface *ip;
//		HWND hPanel;
		
		//Constructor/Destructor
		OBB();
		~OBB();

//		void BeginEditParams(Interface *ip,IUtil *iu);
//		void EndEditParams(Interface *ip,IUtil *iu);
//		void DeleteThis() {}

		void Init(HWND hWnd);
		void Destroy(HWND hWnd);

		void Create(IObjParam *iObjParams,BoundaryMesh& bmesh, INode *sourcenode);

		OBBNode *BuildOBBTree(MNMesh& treemesh,
			const char const *tag,
			const int level,
			const bool& DrawMesh, const bool& DrawBox
			);

		void ConvexHullOBB(
			oriented_bounding_box_3& BBox,
			const MNMesh& unionmesh,
			Point3& MeanMeshPoint,
			double &HullVolume,
			int &TotalHullPoints
			);

		void PlanarOBB(
			oriented_bounding_box_3& BBox,
			const MNMesh& mesh, Point3& MeanMeshPoint);

		void ExtractBBoxFromCovMat(
			oriented_bounding_box_3& BBox,
			const MNMesh& unionmesh,
			matrix_3x3& CovMat,
			const Point3& centroid,
			const Point3& meanmeshpoint
			);

		void DrawOBBNode(
			const OBBNode& CurrNode,
			Interface* ip,
			INode *ParentNode,
			const bool& DrawBox,
			const char const *tag,
			const COLORREF& BBoxColour,
			const COLORREF& LeftBBoxColour,
			const COLORREF& RightBBoxColour
			);

		OBBNode *Tree;

		unsigned int SetRecursionLevel(const unsigned int& NewLevel);				
		unsigned int RecursionLevel;

		bool BrowseFileName(HWND hWnd);

		bool WriteOBBFile(HWND hWnd);

//		std::string OutFileName;
//		FILE *OutFile;

		IColorSwatch *RootColourSwatch;
		IColorSwatch *LeftColourSwatch;
		IColorSwatch *RightColourSwatch;

		bool CheckForSeparateMeshes;
};

#endif // OBB_H
