#if !defined(BOUNDARY_H)
/* ========================================================================
   Header File: boundary.h
   Declares: 
   ======================================================================== */

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */

/* ========================================================================
   Forward Declarations
   ======================================================================== */
#include "IgnoredWarnings.h"

#include "max.h"
#include "mnmath.h"
#include "resource.h"

#include "smart_pointer.h"

#include "indexlists.h"

#include <list>
#include <vector>

#ifndef _UINT32
#define _UINT32
typedef unsigned int UINT32;
typedef unsigned short UINT16;
#endif

//========================================================================

class Door {

private:

	UINT32 d_id;

	int d_loopUsed;
	int d_firstEdge;
	int d_lastEdge;
	int d_maxEdge;
	bool d_flipped;
	bool d_verticalAlignment;

	Point3 d_centerPoint;
	Matrix3 d_orientation;
		
	Door(void);

public:
	
	Door(int loop, int first, int last, bool flip, bool align, UINT32 id)
		: d_loopUsed(loop)
		, d_firstEdge(first)
		, d_lastEdge(last)
		, d_maxEdge(-1)
		, d_flipped(flip)
		, d_verticalAlignment(align)
		, d_id(id)
		{};
	
	int GetID(void) {return (d_id);}

	int GetLoopUsed(void) {return (d_loopUsed);}
	int GetFirstEdge(void) {return (d_firstEdge);}
	int GetLastEdge(void) {return (d_lastEdge);}
	int GetRange(void) {return ((d_lastEdge-d_firstEdge)%d_maxEdge);}
	int GetMaxEdge(void) {return (d_maxEdge);}
	int NextEdge(int i) {return (i+1)%d_maxEdge;}

	void SetMaxEdge(int m) {d_maxEdge = m;}
	
	Point3& GetCenterPoint(void) {return (d_centerPoint);}
	void SetCenterPoint(Point3& p) {d_centerPoint = p;}
	
	Matrix3& GetOrientation(void) {return (d_orientation);}
	void SetOrientation(Matrix3& m) {d_orientation = m;}
	
	bool GetFlipped(void) {return (d_flipped);}
	void SetFlipped(bool f) {d_flipped = f;}

	bool GetVerticalAlignment(void) {return (d_verticalAlignment);}
	void SetVerticalAlignment(bool f) {d_verticalAlignment = f;}

	void AdvanceTheEdges(void ) {
		if (d_maxEdge > 0) {
			d_firstEdge = NextEdge(d_firstEdge);
			d_lastEdge  = NextEdge(d_lastEdge);
		}
	}

	
};

typedef smart_pointer<Door> SmartDoor;
typedef std::list<SmartDoor> SmartDoorList;
typedef SmartDoorList::iterator SmartDoorListIter;

class IDoor {

private:

	UINT32 d_id;

	bool d_flipped;
	bool d_verticalAlignment;

	Point3 d_centerPoint;
	Matrix3 d_orientation;
		
	IDoor(void);

public:

	std::vector<int> d_vertIndexArray;
	
	IDoor(bool flip, bool align, UINT32 id)
		: d_flipped(flip)
		, d_verticalAlignment(align)
		, d_id(id)
		{};
	
	int GetID(void) {return (d_id);}

	Point3& GetCenterPoint(void) {return (d_centerPoint);}
	void SetCenterPoint(Point3& p) {d_centerPoint = p;}
	
	Matrix3& GetOrientation(void) {return (d_orientation);}
	void SetOrientation(Matrix3& m) {d_orientation = m;}
	
	bool GetFlipped(void) {return (d_flipped);}
	void SetFlipped(bool f) {d_flipped = f;}

	bool GetVerticalAlignment(void) {return (d_verticalAlignment);}
	void SetVerticalAlignment(bool f) {d_verticalAlignment = f;}

//	Point3* GetVerts(void) { return &d_vertList[0]; }
	int NumVerts(void) { return d_vertIndexArray.size(); }

};

typedef smart_pointer<IDoor> SmartIDoor;
typedef std::list<SmartIDoor> SmartIDoorList;
typedef SmartIDoorList::iterator SmartIDoorListIter;

// Common flags
const DWORD HIT_FLAG			= MN_USER;
const DWORD SELECTED_FLAG		= MN_USER<<1;

// MNVert user flags
const DWORD DOOR_VERT_FLAG		= MN_USER<<2;
const DWORD NORM_HIT_FLAG		= MN_USER<<3;
const DWORD NORM_SEL_FLAG		= MN_USER<<4;
const DWORD LOCKED_NORM_FLAG	= MN_USER<<5;
const DWORD SPOOFED_FLAG		= MN_USER<<6;
const DWORD SPOOFED_SEL_FLAG	= MN_USER<<7;

// MNEdge user flags
const DWORD DOOR_EDGE_FLAG		= MN_USER<<2;
const DWORD IDOOR_HIT_FLAG		= MN_USER<<3;
const DWORD IDOOR_SEL_FLAG		= MN_USER<<4;

// MNFace user flags
const DWORD FLOOR_FACE_FLAG		= MN_USER<<2;
const DWORD IGNORED_HIT_FLAG	= MN_USER<<3;
const DWORD IGNORED_SEL_FLAG	= MN_USER<<4;
const DWORD IGNORED_FLAG		= MN_USER<<5;
const DWORD WATER_FACE_FLAG		= MN_USER<<6;


// Hit Test return code flags
const DWORD RET_HIT_NOTHING			= 0;
const DWORD RET_HIT_DOOR			= 1<<0;
const DWORD RET_HIT_NON_DOOR		= 1<<1;
const DWORD RET_HIT_FLOOR			= 1<<2;
const DWORD RET_HIT_IDOOR			= 1<<3;
const DWORD RET_HIT_IDOOR_ENDPOINT	= 1<<5;
const DWORD RET_HIT_IGNORED			= 1<<4;
const DWORD RET_HIT_NORM			= 1<<6;
const DWORD RET_HIT_WATER			= 1<<7;

// Packed Hit Test return code
//  F  LLLLLLLLLLL  VVVVVVVVVVVVVVVVVVVV

const int PACKED_VERT_SIZE = 20;
const int PACKED_LOOP_SIZE = 11;
const int PACKED_FLAG_SIZE =  1;

const int PACKED_VERT_MASK = (1 << PACKED_VERT_SIZE) - 1;
const int PACKED_LOOP_MASK = (1 << PACKED_LOOP_SIZE) - 1;
const int PACKED_FLAG_MASK = (1 << PACKED_FLAG_SIZE) - 1;

const int PACKED_VERT_SHIFT = 0;
const int PACKED_LOOP_SHIFT = PACKED_VERT_SHIFT+PACKED_VERT_SIZE;
const int PACKED_FLAG_SHIFT = PACKED_LOOP_SHIFT+PACKED_LOOP_SIZE;


//========================================================================
	
class BoundaryMesh :  public MNMesh {

private:

	// ********* Door data 

	// Keep a reference to the INode holding the mesh we are
	// fetching our loops from
	INode* d_loopSource;
	 
	// Keep a list of all the open loops in the mesh
	MNMeshBorder d_borderLoops;
	int d_borderLoopCount;
	int d_doorLoopCount;

	// Keep the index of the loop we will use as a portal into the mesh
	int d_loopUsedIndex;
	
	// Need to supprt sub object hit testing/selection
	UINT32 d_hitID;
	int d_hitLoop;	
	int d_hitVert;
	int d_hitEdge;
	int d_flippedDoor;
	int d_verticalDoor;
	
	int d_firstEdge;
	int d_lastEdge;

	void DrawLoops(ViewExp *vpt);
	int SelectLoops(ViewExp *vpt, ModContext* mc,int flags);

	SmartDoorList d_doorList;
	SmartDoorList::iterator d_selectedDoor;

	// ********* Internal Door data 

	SmartIDoorList d_iDoorList;
	SmartIDoorList::iterator d_selectedIDoor;

	void DrawIDoors(ViewExp *vpt);

//	std::vector<int> d_selectedIDoorEdgeArray;
	std::list<int> d_selectedIDoorVertList;

	// ********* Floor data 
 
	int d_floorFaceCount;
	int d_selectedFloorFaceCount;
	
	void DrawFloor(ViewExp *vpt);

	// ********* Water data 
 
	int d_waterFaceCount;
	int d_selectedWaterFaceCount;
	
	void DrawWater(ViewExp *vpt);

	// ********* Ignored (face) data 
 
	int d_ignoredCount;
	int d_selectedIgnoredCount;
	
	void DrawIgnored(ViewExp *vpt);

	// ********* Locked Normal data 
 
	int d_lockedNormCount;
	int d_selectedNormCount;
	
	void DrawNorms(ViewExp *vpt);

	// ********* Spoofed Normal data 
 
	int d_spoofedNormCount;
	int d_selectedSpoofedNormCount;
	
	void DrawSpoofed(ViewExp *vpt);

public:
	
	BoundaryMesh(void);
	~BoundaryMesh(void);
	

	// Display
	void Render(ViewExp *vpt);
	void DrawDoorArrow(ViewExp *vpt,Matrix3& tmn);


	//-----------------------

	// Update the boundary mesh to reflect the state of the underlying mesh
	bool UpdateBoundaryMesh(INode *source);

	//-----------------------

	// Access
	int GetNumDoorLoops(void) {return d_doorLoopCount;}
	int GetNumBorderLoops(void) {return d_borderLoopCount;}

	int GetBorderLoopCount(int loop);
	
	int GetHitLoop(void) {return d_hitLoop;}
	void SetHitLoop(int h) {d_hitLoop = h;}
	
	int GetHitVert(void) {return d_hitVert;}
	void SetHitVert(int h) {d_hitVert = h;}
	
	int FindLoops(INode *source,bool loading = false);

	// Selection/Hit Testing
	int DoorHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void DoorSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	int IDoorHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void IDoorSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	int FloorHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void FloorSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	int WaterHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void WaterSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	int IgnoredHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void IgnoredSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	int NormsHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void NormsSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	int SpoofedHitTest(ViewExp *vpt, ModContext* mc, int flags);
	void SpoofedSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly);

	void ClearHit(void);

	//-----------------------
	// Regular doors 

	Door* GetSelectedDoor(void) {
		if (d_selectedDoor != d_doorList.end()) {
			return (*d_selectedDoor).GetPointer();
		} else {
			return 0;
		}
	}

	bool SelectDoor(int loop, int vert);
	bool SelectHitDoor(void);
	void DeselectAll(void);
	
	bool AddDoor(bool loading=false); 
	bool BuildDoor(bool loading=false);
	void BuildAllDoors(bool loading=false);
	bool FlipDoor(void);
	bool OrientDoor(void);
	bool AdvanceStartVert(void);

	bool DeleteDoor();
	void DeleteAllDoors(void);

	Door* GetDoor(const int dnum);

	int	GetDoorVerts(const int dnum,std::list<int>& vlist);

	//-----------------------
	// Internal Doors

	IDoor* GetSelectedIDoor(void) {
		if (d_selectedIDoor != d_iDoorList.end()) {
			return (*d_selectedIDoor).GetPointer();
		} else {
			return 0;
		}
	}

	bool SelectHitIDoor(int dnum);
	void DeselectAllIDoors(void);
	
	bool AddIDoor(bool loading=false); 
	bool BuildIDoor(bool loading=false);
	void BuildAllIDoors(bool loading=false);
	bool FlipIDoor(void);
	bool OrientIDoor(void);

	bool DeleteIDoor();
	void DeleteAllIDoors(void);

	IDoor* GetIDoor(const int dnum);

	int NumIDoors(void) { return d_iDoorList.size(); }

//	int NumSelectedEdges(void) { return d_selectedIDoorEdgeArray.size(); }

	void ClearIDoorSelection(void) {
		d_selectedIDoor = d_iDoorList.end();
		d_selectedIDoorVertList.clear();
	}

//	int	GetIDoorVerts(const int dnum,std::list<int>& vlist);
		
	//-----------------------
	// Floors

	void AddFloor(void);
	void DeleteFloor(void);
	void ResetFloor(void);

	void SetFloorSelectFromOriginal(void);

	int GetFloorFaceCount(void) {return d_floorFaceCount;}
	int GetSelectedFloorFaceCount(void) {return d_selectedFloorFaceCount;}

	//-----------------------
	// Water

	void AddWater(void);
	void DeleteWater(void);
	void ResetWater(void);

	void SetWaterSelectFromOriginal(void);

	int GetWaterFaceCount(void) {return d_waterFaceCount;}
	int GetSelectedWaterFaceCount(void) {return d_selectedWaterFaceCount;}

	//-----------------------
	// Ignored Faces

	void AddIgnored(void);
	void DeleteIgnored(void);
	void ResetIgnored(void);

	void SetIgnoredSelectFromOriginal(void);

	int GetIgnoredCount(void) {return d_ignoredCount;}
	int GetSelectedIgnoredCount(void) {return d_selectedIgnoredCount;}

	//-----------------------
	// Locked Normals

	void LockNorms(void);
	void UnlockNorms(void);
	void ResetNorms(void);

	int GetLockedNormCount(void) {return d_lockedNormCount;}
	int GetSelectedNormCount(void) {return d_selectedNormCount;}

	//-----------------------
	// Spoofed Normals

	void SpoofNorms(void);
	void UnspoofNorms(void);
	void ResetSpoofed(void);

	int GetSpoofedNormCount(void) {return d_spoofedNormCount;}
	int GetSelectedSpoofedNormCount(void) {return d_selectedSpoofedNormCount;}

	//-----------------------

	// Save & Load
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload,IndexList& floor_faces,IndexList& water_faces,IndexList& ignored_faces,IndexList& locked_norms,IndexList& spoofed_norms);
	void PostLoadFixup(INode *src_node, const IndexList& floor_faces, const IndexList& water_faces, const IndexList& ignored_faces,const IndexList& locked_norms,const IndexList& spoofed_norms);



};


#define BOUNDARY_H
#endif
