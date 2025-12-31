/**********************************************************************
 *<
	FILE: SiegeNodeObject.h

	DESCRIPTION:	Template Utility

	CREATED BY:

	HISTORY:


 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "IgnoredWarnings.h"

#ifndef __SIEGENODEOBJECT__H
#define __SIEGENODEOBJECT__H

#include "resource.h"
#include "Max.h"
#include "istdplug.h"

#include "indexlists.h"

class OBB;		// from obb.h
class BoundaryMesh; // From boundary.h

extern TCHAR *GetString(int id);

#define CLASS_NAME_STRING  (_T("SiegeNode"))
	
extern HINSTANCE hInstance;

// The unique ClassID
#define SIEGENODEOBJECT_CLASS_ID	Class_ID(0xd7ef9f9b, 0x80208718)

class SiegeNodeObject : public Object {

   private:

	friend class SiegeNodeObjectCreateCallback;

	friend BOOL CALLBACK SiegeNodeObjectDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK DoorSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK FloorSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK WaterSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK OBBSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK InternalDoorSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK IgnoredSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK NormsSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );
	friend BOOL CALLBACK SpoofedSubPanelDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam );

	friend void resetSiegeNodeObjectParams(BOOL fileReset);

	// Class vars
	static HWND hPanelDlg;
	static IObjParam *iObjParams;
	// Support for sub object selection
	static SelectModBoxCMode *selectMode;

	// Object vars
	bool d_created; // The user has created this SNO
	bool d_valid; // The SNO is good to go, all loops are accounted for

	//References to other objects
	
	// The d_nodeMesh is the ONLY ReferenceMaker in the class
	INode* d_nodeMesh;


	// This WAS a ReferenceMaker, but it got far too hairy
	BoundaryMesh *d_boundaryMesh;

	// Keep a pointer to the triangulation
	TriObject* d_triMesh;

	void UpdateUI(TimeValue t);

	RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget,
							   PartID& partID, RefMessage message);

	int d_subselState;

	
	// Keep indices of the start/finish of the loop section we use as a portal
	// Need this to support 'open to the sky' nodes
	// NOTE: for a closed loop, these must be equal
	int d_firstBoundaryVert;
	int d_lastBoundaryVert;

	OBB *d_boundaryOBB;

	bool d_UserWasWarned;
	
public:

	// Existence
	SiegeNodeObject();
	~SiegeNodeObject();

	// From BaseObject

	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt);
	int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext *mc);
	
	int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags);

	CreateMouseCallBack* GetCreateMouseCallBack();
	void BeginEditParams( IObjParam *ip, ULONG flags,Animatable *prev);
	void EndEditParams( IObjParam *ip, ULONG flags,Animatable *next);
	TCHAR *GetObjectName() { return CLASS_NAME_STRING; }
	
	// From Object
	int IsRenderable() {return 0;}

	int CanConvertToType(Class_ID obtype);
	Object* ConvertToType(TimeValue t, Class_ID obtype);
	
	Object *MakeShallowCopy(ChannelMask channels);	
	void ShallowCopy(Object* fromOb, ChannelMask channels);
	
	ObjectState Eval(TimeValue time);
	void InitNodeName(TSTR& s) { s = CLASS_NAME_STRING; }
	int DoOwnSelectHilite() {
		return 1;
	}
	
	// Animatable methods
	void DeleteThis() { delete this; }
	
	Class_ID ClassID() {
		return SIEGENODEOBJECT_CLASS_ID;
	}
	
	void GetClassName(TSTR& s) { s = CLASS_NAME_STRING; }
	int IsKeyable(){ return 0;}
	SClass_ID SuperClassID() {return SYSTEM_CLASS_ID;}


	// From ReferenceMaker

	enum {REF_MESH = 0,
		  REF_SIZE
	};
	
	RefTargetHandle Clone(RemapDir& remap = NoRemap());
	int NumRefs() {return (REF_SIZE);}
	RefTargetHandle GetReference(int i);
	void SetReference(int i, RefTargetHandle rtarg);

	// IO
	IOResult Save(ISave *isave);
	IOResult Load(ILoad *iload);

	// Start of (somewhat) original content....

	// Define a fixup callback for the loader
	class LoadFixup : public PostLoadCallback {
  
	public:
		
		IndexList d_floorFaceList;
		IndexList d_waterFaceList;
		IndexList d_ignoredFaceList;
		IndexList d_lockedNormList;
		IndexList d_spoofedNormList;

		SiegeNodeObject* d_sno;
	
		void proc(ILoad *iload);
	};

	// Accessors to private data
	void SetCreated(bool f) {d_created = f;}
	bool GetCreated(void) {return d_created;}
	
	void SetValid(bool f) {d_valid = f;}
	bool GetValid(void) {return d_valid;}

	// Bounding Mesh manipulators

	void ActivateSubobjSel(int level, XFormModes& modes );
	
	void SetBMesh(BoundaryMesh* bm) {d_boundaryMesh = bm;}
	BoundaryMesh* GetBMesh(void) {return d_boundaryMesh;}

	DWORD GetSubselState(void) {return d_subselState;}
	void SelectSubComponent(HitRecord *hitRec, BOOL selected, BOOL all, BOOL invert=FALSE);

	void SetOBB(OBB *o) {d_boundaryOBB = o;}
	OBB* GetOBB(void) {return d_boundaryOBB;}

	void SetUserWasWarned(bool newval) {d_UserWasWarned = newval;}
	bool GetUserWasWarned(void) {return d_UserWasWarned;}

};


// This is the Class Descriptor for the SiegeNodeObject plug-in
class SiegeNodeObjectClassDesc:public ClassDesc {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading = FALSE) {return new SiegeNodeObject();}
	const TCHAR *	ClassName() {return CLASS_NAME_STRING;}
	SClass_ID		SuperClassID() {return SYSTEM_CLASS_ID;}
	Class_ID		ClassID() {return SIEGENODEOBJECT_CLASS_ID;}
	const TCHAR* 	Category() {return GetString(IDS_CATEGORY);}
	void			ResetClassParams (BOOL fileReset);
};

// This is the creator callback for the SiegeNodeObject

class SiegeNodeObjectCreateCallback: public CreateMouseCallBack {
	SiegeNodeObject *node;
public:
	int proc( ViewExp *vpt,int msg, int point, int flags, IPoint2 m, Matrix3& mat );
	void SetObj(SiegeNodeObject *newnode);
};


// Handles the work of picking the bounding mesh.

class PickSiegeNodeObjectBoundingMeshCallback  : public PickModeCallback, PickNodeCallback {
public:
	SiegeNodeObject *d_siegeNodeObj;
	int doingPick;
	CommandMode *cm;
	HWND hDlg;

	IObjParam* d_ip;

	PickSiegeNodeObjectBoundingMeshCallback() {}

	BOOL HitTest (IObjParam *ip, HWND hWnd, ViewExp *vpt, IPoint2 m, int flags);
	BOOL Pick (IObjParam *ip, ViewExp *vpt);
	void EnterMode(IObjParam *ip);
	void ExitMode(IObjParam *ip);

	// Allow right-clicking out of mode.
	BOOL RightClick (IObjParam *ip,ViewExp *vpt) {
		return TRUE;
	}
	
	// Is supposed to return a PickNodeCallback-derived class: we qualify!
	PickNodeCallback *GetFilter() {return this;}
	
	// PickNodeCallback methods:
	BOOL Filter(INode *node);
};

#endif // __SIEGENODEOBJECT__H
