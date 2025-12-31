/* ========================================================================
   Source File: boundary.cpp
   Defines: 
   ======================================================================== */

#include "precompiled_siegenodeobject.h"


// Existence

BoundaryMesh::BoundaryMesh() 
	: d_borderLoopCount(0)
	, d_doorLoopCount(0)
	, d_loopSource()
	, d_hitLoop(-1)
	, d_hitVert(-1)
	, d_hitEdge(-1)
	, d_flippedDoor(false)
	, d_selectedDoor(d_doorList.end())
	, d_selectedIDoor(d_iDoorList.end())
	, d_floorFaceCount(0)
	, d_selectedFloorFaceCount(0)
	, d_waterFaceCount(0)
	, d_selectedWaterFaceCount(0)
	, d_ignoredCount(0)
	, d_selectedIgnoredCount(0)
	, d_lockedNormCount(0)
	, d_selectedNormCount(0)
	, d_spoofedNormCount(0)
	, d_selectedSpoofedNormCount(0)
		
{
	d_borderLoops.Clear();
}

BoundaryMesh::~BoundaryMesh(void) 
{
	d_doorList.clear();
	d_iDoorList.clear();
	d_borderLoops.Clear();
	d_selectedDoor = d_doorList.end();
}

void BoundaryMesh::ClearHit(void){
	d_hitLoop = -1;
	d_hitVert = -1;
	d_hitEdge = -1;
}

int BoundaryMesh::FindLoops(INode *source, bool loading)
{

	if (!source) return 0;

	d_loopSource = source;

	Class_ID cl = d_loopSource->ClassID();

	Object* boundobj= d_loopSource->EvalWorldState(0).obj;

	if (boundobj->CanConvertToType(triObjectClassID)) {

		TriObject *tempmesh;
		
		tempmesh = (TriObject *)boundobj->ConvertToType(0,triObjectClassID);

		// Build a new mnmesh
		Clear();
		SetFromTri(tempmesh->mesh);

		d_borderLoops.Clear();
		GetBorder(d_borderLoops);

		if (!loading && ((numv	!= tempmesh->mesh.numVerts) || (numf	!= tempmesh->mesh.numFaces))) {

			MessageBox(NULL,"The source mesh is irregular and the Siege Node can't be validated\nAll the nasty verts have been selected\nCheck for verts shared by two loops\n\nYou have been warned!","Hey, Sailor!",MB_OK| MB_ICONSTOP );

			tempmesh->mesh.vertSel.ClearAll();

			for (int badvert = tempmesh->mesh.numVerts; badvert <= numv; badvert++) {
				for (int goodvert = 0; goodvert <= tempmesh->mesh.numVerts ; goodvert++) {
					if (tempmesh->mesh.verts[goodvert] == P(badvert)) {
						tempmesh->mesh.vertSel.Set(goodvert);
					}
				}
			}
		}

		d_borderLoopCount = d_borderLoops.Num();
	
		if (boundobj != tempmesh){
			tempmesh->DeleteMe();
		}
		
	} else {
		
		Clear();
		d_borderLoops.Clear();
		d_borderLoopCount = 0;
		
	}

	ClearHit();

	return d_borderLoopCount;
}

int BoundaryMesh::GetBorderLoopCount(int loop) {

	if (loop >= 0 && loop < d_borderLoopCount) {
		return d_borderLoops.Loop(loop)->Count();
	}

	return 0;
}
	

void BoundaryMesh::Render(ViewExp *vpt) {

	DWORD savedLimits = vpt->getGW()->getRndLimits();
	
	if (savedLimits & GW_WIREFRAME) {
		vpt->getGW()->setRndLimits(((savedLimits & !(GW_WIREFRAME|GW_Z_BUFFER))|GW_TWO_SIDED|GW_POLY_EDGES));
	} else {
		vpt->getGW()->setRndLimits((savedLimits|GW_TWO_SIDED|GW_POLY_EDGES|GW_Z_BUFFER));
	}

	DrawFloor(vpt);
	DrawWater(vpt);
	DrawIgnored(vpt);
	DrawLoops(vpt);
	DrawIDoors(vpt);
	DrawNorms(vpt);
	DrawSpoofed(vpt);

	vpt->getGW()->setRndLimits(savedLimits);

}

bool BoundaryMesh::UpdateBoundaryMesh(INode *source)
{

	if (!source) return false;

	if (source != d_loopSource) return false;

	Object* boundobj= source->EvalWorldState(0).obj;

	if (boundobj->CanConvertToType(triObjectClassID)) {

		TriObject *temptriobj;
		
		temptriobj = (TriObject *)boundobj->ConvertToType(0,triObjectClassID);

		Mesh& tempmesh = temptriobj->GetMesh();

		if (   (numv	!= tempmesh.numVerts)
			|| (numf	!= tempmesh.numFaces)
			) {
			return false;
		}

		IndexList floor_faces,ignored_faces,locked_verts,spoofed_verts,water_faces;

		{for (int i=0; i < FNum(); ++i) {

			MNFace* f = F(i);
			
			if (f->GetFlag(FLOOR_FACE_FLAG)) {
				floor_faces.push_back(i);
			}
			if (f->GetFlag(IGNORED_FLAG)) {
				ignored_faces.push_back(i);
			}
			if (f->GetFlag(WATER_FACE_FLAG)) {
				water_faces.push_back(i);
			}
		}}

		{for (int i=0; i < VNum(); ++i) {

			MNVert* v = V(i);
						
			if (v->GetFlag(LOCKED_NORM_FLAG)) {
				locked_verts.push_back(i);
			}
			if (v->GetFlag(SPOOFED_FLAG)) {
				spoofed_verts.push_back(i);
			}

		}}


		Clear();
		SetFromTri(tempmesh);

		if (boundobj != temptriobj){
			temptriobj->DeleteMe();
		}

		d_borderLoops.Clear();
		GetBorder(d_borderLoops);

		if (d_borderLoopCount != d_borderLoops.Num()) {
			return false;
		}

		BuildAllDoors(false);
		BuildAllIDoors(false);

		{for (int i=0; i < FNum(); ++i) {

			MNFace* f = F(i);
			
			f->ClearFlag(SELECTED_FLAG);
			f->ClearFlag(FLOOR_FACE_FLAG);
			f->ClearFlag(WATER_FACE_FLAG);
			f->ClearFlag(IGNORED_SEL_FLAG);
			f->ClearFlag(IGNORED_FLAG);

		}}

		{for (int i=0; i < VNum(); ++i) {

			MNVert* v = V(i);

			v->ClearFlag(NORM_SEL_FLAG);
			v->ClearFlag(LOCKED_NORM_FLAG);
			v->ClearFlag(SPOOFED_FLAG);
			v->ClearFlag(SPOOFED_SEL_FLAG);
		}}
		
		{
			IndexListConstIter i;

			{for (i = floor_faces.begin(); i != floor_faces.end(); ++i) {
				MNFace* f = F(*i);
				f->SetFlag(FLOOR_FACE_FLAG);
			}}

			{for (i = water_faces.begin(); i != water_faces.end(); ++i) {
				MNFace* f = F(*i);
				f->SetFlag(WATER_FACE_FLAG);
			}}

			{for (i = ignored_faces.begin(); i != ignored_faces.end(); ++i) {
				MNFace* f = F(*i);
				f->SetFlag(IGNORED_FLAG);
			}}

			{for (i = locked_verts.begin(); i != locked_verts.end(); ++i) {
				MNVert* v = V(*i);
				v->SetFlag(LOCKED_NORM_FLAG);
			}}

			{for (i = spoofed_verts.begin(); i != spoofed_verts.end(); ++i) {
				MNVert* v = V(*i);
				v->SetFlag(SPOOFED_FLAG);
			}}

		}

	}


	return true;

}

//******************************************
//
// Doors
//
//******************************************

void BoundaryMesh::DrawLoops(ViewExp *vpt)
{

	if (INode* boundNode = d_loopSource) {
	
		GraphicsWindow *gw = vpt->getGW();
		
		Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
//		gw->setTransform(ntm);
		gw->setTransform(Matrix3(1));

		Point3 pts[3];
		
		// Draw all the border edges in appropriate colour
		
		{for (int i = 0; i < d_borderLoopCount ; i++) {
	
			const IntTab* lp = d_borderLoops.Loop(i);
				
			gw->setColor(LINE_COLOR,GetUIColor(COLOR_GIZMOS));

			// Draw the plain edges
			{for (int j = 0; j<lp->Count(); j++) {

				MNEdge* edge = E((*lp)[j]);
				
				pts[0] = ntm*V(edge->v2)->p ;
				pts[1] = ntm*V(edge->v1)->p ;				
						
				if (!edge->GetFlag(DOOR_EDGE_FLAG)) {

					gw->setColor(LINE_COLOR,GetUIColor(COLOR_GIZMOS));
					gw->polyline(2,pts,0,0,0,0);
					gw->marker(&pts[0],PLUS_SIGN_MRKR);
					gw->marker(&pts[1],PLUS_SIGN_MRKR);
					
				}
			}}
		

			// Now draw the door edges
			{for (int j = 0; j<lp->Count(); j++) {

				MNEdge* edge = E((*lp)[j]);
				
				pts[0] = ntm*V(edge->v2)->p ;
				pts[1] = ntm*V(edge->v1)->p ;						
			
				if (edge->GetFlag(DOOR_EDGE_FLAG)) {

					gw->setColor(LINE_COLOR,Point3(1,0,0));
					gw->polyline(2,pts,0,0,0,0);
					gw->marker(&pts[0],PLUS_SIGN_MRKR);
					gw->marker(&pts[1],PLUS_SIGN_MRKR);
					
				}
			}}

			// Now draw the misc. door info
		
			{for (SmartDoorListIter d = d_doorList.begin();
				  d != d_doorList.end();
				  ++d) {
	
				const IntTab* lp = d_borderLoops.Loop((*d)->GetLoopUsed());

				MNEdge* edge = E((*lp)[(*d)->GetFirstEdge()]);
				
				pts[0] = ntm*V(edge->v2)->p ;

				edge = E((*lp)[(*d)->GetLastEdge()]);

				pts[1] = ntm*V(edge->v1)->p ;						

				// Start point
				gw->setColor(LINE_COLOR,Point3(0,1,0));
				gw->marker(&pts[0],TRIANGLE_MRKR);
				
				// End point
				gw->setColor(LINE_COLOR,Point3(1,0,0));
				gw->marker(&pts[1],HOLLOW_BOX_MRKR);

				// Cap line
				if (pts[0] != pts[1]) {
					gw->setColor(LINE_COLOR,Point3(0,0,1));
					gw->polyline(2,pts,0,0,0,0);
				}

				// Center point
				pts[0] = ntm*(*d)->GetCenterPoint();

				gw->setColor(LINE_COLOR,Point3(1,1,0));
				gw->marker(&pts[0],DIAMOND_MRKR);	

				Matrix3 tm = ntm*(*d)->GetOrientation();
				tm.SetTrans(ntm*(*d)->GetCenterPoint());
			
				DrawDoorArrow(vpt,tm);
				
			}}
			
			// Draw the selected verts on top of everything else
			
			gw->setColor(LINE_COLOR,Point3(1,1,1));
			
			{for (int j = 0; j<lp->Count(); j++) {

				MNEdge* edge = E((*lp)[j]);
				
				pts[0] = ntm*V(edge->v1)->p ;
				pts[1] = ntm*V(edge->v2)->p ;
				
				if (edge->GetFlag(SELECTED_FLAG)) {					
					gw->marker(&pts[0],SM_CIRCLE_MRKR);
					gw->marker(&pts[1],SM_CIRCLE_MRKR);
				}

			}}

		}}
	}
}
		
		


int BoundaryMesh::DoorHitTest(ViewExp *vpt, ModContext *mc, int flags) {

	if (INode* boundNode = d_loopSource) {
		
		int res = RET_HIT_NOTHING;
	
		GraphicsWindow *gw = vpt->getGW();

		if (flags) {
			bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
			bool selOnly    = flags&SUBHIT_SELONLY?true:false;
			bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
		}

		Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
		gw->setTransform(ntm);

		DWORD closest = (DWORD)-1;
		
		int closest_loop = -1;
		int closest_vert = -1;
		
		{for (int i=0; i<d_borderLoopCount; ++i){
		
			const IntTab* lp = d_borderLoops.Loop(i);
			
			{for (int j=0; j<lp->Count(); ++j) {

				int vert_index = E((*lp)[j])->v2;
				MNVert* vert = V(vert_index);

				{
					
					gw->marker(&vert->p ,POINT_MRKR);		
				
					if (gw->checkHitCode()) {
						
						DWORD dist = gw->getHitDistance();
						if (dist <= closest) {
							closest = dist;
							closest_loop = i;
							closest_vert = vert_index;
						}
						
						vert->SetFlag(HIT_FLAG);
						
						if (vert->GetFlag(DOOR_VERT_FLAG)) {
							res |= RET_HIT_DOOR;
						} else {
							res |= RET_HIT_NON_DOOR;
						}

					} else {
						vert->ClearFlag(HIT_FLAG);	
					}
					
					gw->clearHitCode();
				
				}
					

			}}


		}}

		if (res != RET_HIT_NOTHING) {

 			int packed_info = 0;

			// This packed_info stuff is a littly loony, but I didn't want to
			// have to put together a full hitInfo structure. Maybe later...
			
			if (res == RET_HIT_DOOR) {
				packed_info = (1 & PACKED_FLAG_MASK)<<PACKED_FLAG_SHIFT;
			} 
			
			packed_info |= ((closest_loop & PACKED_LOOP_MASK) << PACKED_LOOP_SHIFT);
			packed_info	|= ((closest_vert & PACKED_VERT_MASK) << PACKED_VERT_SHIFT);
						
			vpt->LogHit(boundNode, mc, 0, packed_info);

			return TRUE;

		}
		
	}
	
	return FALSE;
	
}

void BoundaryMesh::DoorSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

	const IntTab* phitloop = d_borderLoops.Loop(d_hitLoop);

	int	edgecount = phitloop->Count();
	int next;

	MNEdge* edge;
//	MNVert* vert;

 	bool flip = 0 != (GetKeyState(VK_SHIFT) & (1<<15));
 	bool append = 0 != (GetKeyState(VK_CONTROL) & (1<<15));
 	bool remove = 0 != (GetKeyState(VK_MENU) & (1<<15));
		
	int num_selected = 0;

	{for (int j=0; j<edgecount; ++j) {

		edge = E((*phitloop)[j]);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			edge->ClearFlag(SELECTED_FLAG);
			d_firstEdge = d_lastEdge = d_hitEdge;
		}
		
		// Set the flag appropriately
		if (V(edge->v1)->GetFlag(HIT_FLAG) && V(edge->v2)->GetFlag(HIT_FLAG)) {
			
			if (flip) {
				edge->SetFlag(SELECTED_FLAG,!edge->GetFlag(SELECTED_FLAG));
			} else if (remove) {
				edge->ClearFlag(SELECTED_FLAG);			
			} else {
				edge->SetFlag(SELECTED_FLAG);
			}
		}
		
		// Tally
		if (edge->GetFlag(SELECTED_FLAG)) {
			num_selected++;
		}
	}}

	if (!num_selected) {
		d_firstEdge = -1;
		d_lastEdge = -1;
		d_hitLoop = -1;
		return;
	}

	if (num_selected == edgecount) {

		int e = 0;
		

		do {
			// Only need to check one vert for all but the last edge
			int searchVert = E((*phitloop)[e])->v1;
			if (searchVert == d_hitVert) {
				d_lastEdge = e;
				d_firstEdge = (d_lastEdge+1) % edgecount;
				break;
			}
			e = (e+1) % edgecount;

		} while (e);
		

		return;
	}
	
	if (d_firstEdge == -1) {
		d_firstEdge = 0;
		d_lastEdge = 0;
	}

	edge = E((*phitloop)[d_firstEdge]);
	
	if (!edge->GetFlag(SELECTED_FLAG)) {
			
		// Move forward to the start of the first run
		do {
			d_firstEdge = (d_firstEdge+1) % edgecount;
			edge =  E((*phitloop)[d_firstEdge]);
		} while(!edge->GetFlag(SELECTED_FLAG));
				
		next = d_firstEdge;
			
	} else {
			
		// Need to move backward to the start of the first run

		d_lastEdge = d_firstEdge;
			
		next = d_firstEdge;
		do {
			d_firstEdge = next;
			next = (next+edgecount-1) % edgecount;
			edge =  E((*phitloop)[next]);		
		} while(edge->GetFlag(SELECTED_FLAG));
			
		next = d_lastEdge;
			
	}

	// Move forward to the last selected in this run
	do {
		d_lastEdge = next;
		next = (next+1) % edgecount;
		edge =  E((*phitloop)[next]);		
	} while(edge->GetFlag(SELECTED_FLAG));
			
	
	// Delete all the widowed selections
	while (next != d_firstEdge) {
		edge->ClearFlag(SELECTED_FLAG);
		next = (next+1) % edgecount;
		edge = E((*phitloop)[next]);
	}

}

bool BoundaryMesh::SelectHitDoor(void) {

	if (d_hitLoop >= 0 || d_hitVert >= 0) {
		return SelectDoor(d_hitLoop,d_hitVert);
	}

	return false;
}

bool BoundaryMesh::SelectDoor(int loop, int vert) {

	SmartDoorListIter searchDoor = d_doorList.end();
	
	// Find the door the lies along 'loop' and contains 'vert'
	{for (SmartDoorListIter d = d_doorList.begin();
		  d != d_doorList.end();
		  ++d) {
	
		if (loop == (*d)->GetLoopUsed()) {
			
			const IntTab* lp = d_borderLoops.Loop(d_hitLoop);
	
			int i = (*d)->GetFirstEdge();
			
			while (i != (*d)->GetLastEdge()) {

				// Only need to check one vert for all but the last edge
				int searchVert = E((*lp)[i])->v2;
				if (searchVert == vert) {
					searchDoor = d;
					break;
				}
 				i = (*d)->NextEdge(i);
			}
			
			if (searchDoor != d_doorList.end()) {
				break;
			}
				
			if ((E((*lp)[i])->v1 == vert) || (E((*lp)[i])->v2 == vert)) {
				searchDoor = d;
				break;
			}
		}

	}}

	if (searchDoor != d_doorList.end()) {

		d_selectedDoor = searchDoor;

		// Deselect everything and then re-select the door

		DeselectAll();
		
		IntTab* lp = d_borderLoops.Loop((*d_selectedDoor)->GetLoopUsed());
	
		int i = (*d_selectedDoor)->GetFirstEdge();
		MNEdge* edge =  E((*lp)[i]);
		
		while (i != (*d_selectedDoor)->GetLastEdge()) {
			
			edge->SetFlag(SELECTED_FLAG);			
			i = (*d_selectedDoor)->NextEdge(i);
			edge = E((*lp)[i]);
		}

		edge->SetFlag(SELECTED_FLAG);

		return true;
	} 
		
			
	return false;
	
}

void BoundaryMesh::DeselectAll(void) {
	
	{for (int i = 0; i < d_borderLoopCount ; i++) {
	
		const IntTab* lp = d_borderLoops.Loop(i);
				
		{for (int j = 0; j<lp->Count(); j++) {

			MNEdge* edge = E((*lp)[j]);
			
			edge->ClearFlag(SELECTED_FLAG);
			V(edge->v2)->ClearFlag(SELECTED_FLAG);
				
		}}
	}}
}
		

bool BoundaryMesh::BuildDoor(bool loading) {

	// Builds the selected door

	if (d_selectedDoor == d_doorList.end()) {
		return false;	
	}
	
	Door* newdoor = (*d_selectedDoor).GetPointer();

	if (!newdoor) {
		return false;
	}
		
	IntTab* lp = d_borderLoops.Loop(newdoor->GetLoopUsed());

	newdoor->SetMaxEdge(lp->Count());
	
	int i = newdoor->GetFirstEdge();
		
	MNEdge* edge =  E((*lp)[i]);

	Point3 avg(0,0,0);

	int tally = 0;

	while (i != newdoor->GetLastEdge()) {
			
		edge->SetFlag(DOOR_EDGE_FLAG);			
		V(edge->v1)->SetFlag(DOOR_VERT_FLAG);
		V(edge->v2)->SetFlag(DOOR_VERT_FLAG);

		edge->SetFlag(SELECTED_FLAG,!loading);

		avg += ((V(edge->v1)->p + V(edge->v2)->p))/2;
		tally++;
			
		i = newdoor->NextEdge(i);
		edge =  E((*lp)[i]);
	}
		
	edge->SetFlag(DOOR_EDGE_FLAG);			
	V(edge->v1)->SetFlag(DOOR_VERT_FLAG);
	V(edge->v2)->SetFlag(DOOR_VERT_FLAG);

	edge->SetFlag(SELECTED_FLAG,!loading);

	avg += ((V(edge->v1)->p + V(edge->v2)->p))/2;
	tally++;

	if ( newdoor->GetLastEdge() == newdoor->GetFirstEdge()) {

		newdoor->SetCenterPoint(avg/tally);

	} else {

		avg = V(E((*lp)[newdoor->GetFirstEdge()])->v2)->p 
			+ V(E((*lp)[newdoor->GetLastEdge()])->v1)->p;

		newdoor->SetCenterPoint(avg/2);

	}


	Matrix3 m(1);
			
	Point3 x_axis,y_axis,z_axis;

	Point3 pointA = V(E((*lp)[newdoor->GetLastEdge()])->v1)->p;
	Point3 pointB = V(E((*lp)[newdoor->GetFirstEdge()])->v2)->p;

	if (pointA == pointB) {
		pointB = V(E((*lp)[newdoor->GetLastEdge()])->v2)->p;
	}

	if (newdoor->GetVerticalAlignment()) {
		y_axis = Point3(0,1,0);
	} else {
		y_axis = Point3(0,0,1);
	}
	
	if (newdoor->GetFlipped()) {
		x_axis = pointA - pointB;
	} else {
		x_axis = pointB - pointA;
	}

	z_axis = CrossProd(x_axis,y_axis);

	if (Length(z_axis) < 0.0001) {
		double t = x_axis[0];
		if (newdoor->GetVerticalAlignment()) {
		x_axis[0] = x_axis[2];
		x_axis[2] = -t;    	
		} else {
		x_axis[0] = x_axis[1];
		x_axis[1] = -t;    	
		}
		z_axis = CrossProd(x_axis,y_axis);
	}
	
	x_axis = CrossProd(y_axis,z_axis);
	
	m.SetRow(0,Normalize(x_axis));
	m.SetRow(1,Normalize(y_axis));
	m.SetRow(2,Normalize(z_axis));
	newdoor->SetOrientation(m);	 

	return true;
}

void BoundaryMesh::BuildAllDoors(bool loading) {
	

	{for (SmartDoorListIter d = d_doorList.begin();
		  d != d_doorList.end();
		  ++d) {
	
		d_selectedDoor = d;
		BuildDoor(loading);
		
	}}
	
}

bool BoundaryMesh::AddDoor(bool loading) {

	int loop = d_hitLoop;
	int first = d_firstEdge;
	int last = d_lastEdge;
	bool flipper = false;
	bool aligner = false;

	int id;

	SmartDoorListIter next_door;
	
	if (loading) {

		flipper = 0 != d_flippedDoor;
		aligner = 0 != d_verticalDoor;
		id = d_hitID;

		next_door = d_doorList.end();	// Insert this door at the end of the list

	} else {
		if (loop < 0 || loop >= d_borderLoopCount) 	return false;	
		int max_edge = d_borderLoops.Loop(loop)->Count();
		if (first < 0 || first >= max_edge) 	return false;	
		if (last < 0 || last >= max_edge) return false;
	
		// Find the next available door id
		id = 1;
		for (next_door = d_doorList.begin() ; next_door != d_doorList.end(); ++next_door) {
			if ((*next_door)->GetID() != id) {
				break;
			}
			id++;
		}

		d_hitID = id;
	
	}
	
	SmartDoor newdoor = SmartDoor(new Door(loop,first,last,flipper,aligner,id));

	if (newdoor) {

		d_selectedDoor = d_doorList.insert(next_door,newdoor);
		d_doorLoopCount++;
	
		if (!loading) {
			BuildDoor();
		}
		
		return true;
	}

	return false;
}

bool BoundaryMesh::FlipDoor() {

	if (d_selectedDoor != d_doorList.end()) {
		
		bool flip_state = (*d_selectedDoor)->GetFlipped();
		flip_state = !flip_state;
		(*d_selectedDoor)->SetFlipped(flip_state);

		// Rebuild with reversed orientation
		BuildDoor(false);

		return true;
	}
	
	return false;

}


bool BoundaryMesh::OrientDoor() {

	if (d_selectedDoor != d_doorList.end()) {
		
		bool state = (*d_selectedDoor)->GetVerticalAlignment();
		state = !state;
		(*d_selectedDoor)->SetVerticalAlignment(state);

		// Rebuild with new orientation
		BuildDoor(false);

		return true;
	}
	
	return false;

}


bool BoundaryMesh::DeleteDoor() {

	// clear all the USED_IN_DOOR flags and
	// delete the selected door from the list

	if (d_selectedDoor != d_doorList.end()) {

		int i = (*d_selectedDoor)->GetFirstEdge();

		// TODO:biddle put this in a GetBorderLoop(lindex)
		IntTab* lp = d_borderLoops.Loop((*d_selectedDoor)->GetLoopUsed());
		
		MNEdge* edge =  E((*lp)[i]);
		
		while (i != (*d_selectedDoor)->GetLastEdge()) {
			
			edge->ClearFlag(DOOR_EDGE_FLAG);			
			V(edge->v1)->ClearFlag(DOOR_VERT_FLAG);
			V(edge->v2)->ClearFlag(DOOR_VERT_FLAG);
			edge->ClearFlag(SELECTED_FLAG);			

			i = (*d_selectedDoor)->NextEdge(i);
			edge = E((*lp)[i]);
		}

		edge->ClearFlag(DOOR_EDGE_FLAG);			
		V(edge->v1)->ClearFlag(DOOR_VERT_FLAG);
		V(edge->v2)->ClearFlag(DOOR_VERT_FLAG);
		edge->ClearFlag(SELECTED_FLAG);			
		
		d_doorList.erase(d_selectedDoor);
		d_selectedDoor = d_doorList.end();

		d_doorLoopCount--;

		return true;
	}
		
	return false;
}

void BoundaryMesh::DeleteAllDoors(void) {
	
	// Find the door the lies along 'loop' and contains 'vert'

	d_selectedDoor = d_doorList.begin();

	while (d_selectedDoor != d_doorList.end()) {
	
		DeleteDoor();

		d_selectedDoor = d_doorList.begin();
		
	}
	
}

Door* BoundaryMesh::GetDoor(const int dnum) {

	// search through the list of doors for the requested one

	if (dnum >= (int)d_doorList.size()) {
		return 0;
	}

	SmartDoorListIter d = d_doorList.begin();


	int count = dnum;

	while (count) {
		d++;
		count--;


	}


	return (*d).GetPointer();
}

int	BoundaryMesh::GetDoorVerts(const int dnum, std::list<int>& vlist) {

	vlist.clear();

	Door* vdoor = GetDoor(dnum);

	if (!vdoor) return 0;

	int i = vdoor->GetFirstEdge();

	// TODO:biddle put this in a GetBorderLoop(lindex)
	IntTab* lp = d_borderLoops.Loop(vdoor->GetLoopUsed());
	
	MNEdge* edge =  E((*lp)[i]);
	vlist.push_back(edge->v2);
	
	while (i != vdoor->GetLastEdge()) {
		
		vlist.push_back(edge->v1);

		i = vdoor->NextEdge(i);
		edge = E((*lp)[i]);
	}

//	vlist.push_back(edge->v2);
	vlist.push_back(edge->v1);

	return vlist.size();

}

bool BoundaryMesh::AdvanceStartVert() {

	if (d_selectedDoor != d_doorList.end()) {
		
		(*d_selectedDoor)->AdvanceTheEdges();

		// Rebuild with new first vert (edge)
		BuildDoor(false);

		return true;
	}
	
	return false;

}

//******************************************
//
// Internal Doors
//
//******************************************

void BoundaryMesh::DrawIDoors(ViewExp *vpt)
{


	if (INode* boundNode = d_loopSource) {
	
		GraphicsWindow *gw = vpt->getGW();
		
		Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0

		Point3 tpts[3];

		{for (SmartIDoorListIter d = d_iDoorList.begin();
			  d != d_iDoorList.end();
			  ++d) {

			gw->setTransform(ntm);

			int numv = (*d)->NumVerts();
			Point3* tmps = new Point3[numv+1];

			for (int i = 0; i < numv ; ++i ) {
				tmps[i] = P((*d)->d_vertIndexArray[i]);
			}

			gw->setColor(LINE_COLOR,Point3(1,0,0));

			gw->polyline(numv,tmps,NULL,NULL,false,NULL);

			gw->setColor(LINE_COLOR,Point3(0,1,0));

			// Start point
			gw->marker(&tmps[0],TRIANGLE_MRKR);

			// End point
			gw->marker(&tmps[numv-1],HOLLOW_BOX_MRKR);

			// Cap line
			tpts[0] = tmps[0];
			tpts[1] = tmps[numv-1];

			gw->setColor(LINE_COLOR,Point3(0,0,1));
			gw->polyline(2,tpts,0,0,0,0);

			// Center point
			tpts[0] = (*d)->GetCenterPoint();

			gw->setColor(LINE_COLOR,Point3(1,1,0));
			gw->marker(&tpts[0],DIAMOND_MRKR);	

			Matrix3 tm = ntm*(*d)->GetOrientation();
			tm.SetTrans(ntm*(*d)->GetCenterPoint());
		
			DrawDoorArrow(vpt,tm);

			delete [] tmps;


		}}
			
		if (d_selectedIDoorVertList.size() > 0) {

			// Draw the selected edges on top of everything else
			gw->setTransform(ntm);
			
			Point3* tmps = new Point3[d_selectedIDoorVertList.size()];

			int j = 0;

			gw->setColor(LINE_COLOR,Point3(1,1,1));

			{for (std::list<int>::iterator i = d_selectedIDoorVertList.begin();
					i != d_selectedIDoorVertList.end(); ++i ) {

				tmps[j] = P((*i));
				gw->marker(&tmps[j],SM_CIRCLE_MRKR);
				
				j++;
			}}

			gw->setColor(LINE_COLOR,Point3(0,1,1));
			gw->polyline(d_selectedIDoorVertList.size(),tmps,0,0,0,0);

			delete [] tmps;

		}

	}
		
}
		


int BoundaryMesh::IDoorHitTest(ViewExp *vpt, ModContext *mc, int flags) {

	INode* boundNode = d_loopSource;
	
	if (!boundNode) {
		return FALSE;
	}
		
	int res = RET_HIT_NOTHING;
	
	GraphicsWindow *gw = vpt->getGW();

	if (flags) {
		bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
		bool selOnly    = flags&SUBHIT_SELONLY?true:false;
		bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
	}

	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
	gw->setTransform(ntm);

	Point3 xyz[3];

	DWORD closest = -1;
	int closest_edge = -1;
	int closest_idoor = -1;

	int hv,tv;

	int i=0;
	
	{for (SmartIDoorListIter d = d_iDoorList.begin(); d != d_iDoorList.end(); ++d) {

		int numv = (*d)->NumVerts();

		for (int j = 0; j < numv ; ++j ) {

			xyz[0] = P((*d)->d_vertIndexArray[j]);

			gw->marker(xyz,SM_CIRCLE_MRKR);

			if (gw->checkHitCode()) {

				DWORD dist = gw->getHitDistance();

				if (dist <= closest) {
					closest = dist;
					closest_idoor = i;
				}

				res |= RET_HIT_IDOOR;
			}

			gw->clearHitCode();

		}

		i++;

	}}

	if (d_selectedIDoorVertList.size() > 0) { 
		hv = *d_selectedIDoorVertList.begin();
		tv = *d_selectedIDoorVertList.rbegin();
	} else {
		hv = tv = -1;
	}

	closest = -1;

	{for (i=0; i < ENum(); ++i){

		MNEdge* edge = E(i);

		edge->ClearFlag(IDOOR_HIT_FLAG);	

		if (hv == -1 || edge->v1 == hv || edge->v2 == hv ||	edge->v1 == tv || edge->v2 == tv) {

			xyz[0] = P(edge->v1);
			xyz[1] = P(edge->v2);

			gw->polyline(2,xyz,NULL,NULL,false,NULL);

			if (gw->checkHitCode()) {
							
				DWORD dist = gw->getHitDistance();
				if (dist <= closest) {
					closest = dist;
					closest_edge = i;

				}
							
				res |= RET_HIT_IDOOR_ENDPOINT;
		
				edge->SetFlag(IDOOR_HIT_FLAG);					
			}
						
			gw->clearHitCode();
		}
				
	}}

	if (res != RET_HIT_NOTHING) {
		vpt->LogHit(boundNode, mc, 0, closest_idoor);
	}

	return (res);
}

void BoundaryMesh::IDoorSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

 	bool flip = (GetKeyState(VK_SHIFT) & (1<<15)) != 0;
 	bool append = (GetKeyState(VK_CONTROL) & (1<<15)) != 0;
 	bool remove = (GetKeyState(VK_MENU) & (1<<15)) != 0;
		
	{for (int j=0; j<ENum(); ++j) {

		MNEdge* edge = E(j);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			edge->ClearFlag(IDOOR_SEL_FLAG);
		}
		
		// Set the flag appropriately
		if (edge->GetFlag(IDOOR_HIT_FLAG)) {
			
			if (flip) {
				edge->SetFlag(IDOOR_SEL_FLAG,!edge->GetFlag(IDOOR_SEL_FLAG));
			} else if (remove) {
				edge->ClearFlag(IDOOR_SEL_FLAG);			
			} else {
				edge->SetFlag(IDOOR_SEL_FLAG);
			}
		}
		
		// Tally
		if (edge->GetFlag(IDOOR_SEL_FLAG)) {
			if (d_selectedIDoorVertList.size() == 0) {

//				d_selectedIDoorEdgeArray.push_back(j);
				d_selectedIDoorVertList.push_back(edge->v1);
				d_selectedIDoorVertList.push_back(edge->v2);

			} else {
				// Make sure that this edge can attach to the front or back of the chain

				int hv = *d_selectedIDoorVertList.begin();
				int tv = *d_selectedIDoorVertList.rbegin();

				int newbie;
				bool addfront;

				if (edge->v1 == hv) {
					addfront = true;
					newbie = edge->v2;
				} else if (edge->v2 == hv) {
					addfront = true;
					newbie = edge->v1;
				} else if (edge->v1 == tv) {
					addfront = false;
					newbie = edge->v2;
				} else if (edge->v2 == tv) {
					addfront = false;
					newbie = edge->v1;
				} else {
					edge->ClearFlag(IDOOR_SEL_FLAG);
					continue;
				}
				
				bool found = false;
				for (std::list<int>::iterator i = d_selectedIDoorVertList.begin(); i != d_selectedIDoorVertList.end(); ++i ) {
					if ((*i) == newbie) {
						found = true;
						break;
					}
				}

				if (!found) {
					if (addfront) {
						d_selectedIDoorVertList.push_front(newbie);
					} else {
						d_selectedIDoorVertList.push_back(newbie);
					}
				} else {
					edge->ClearFlag(IDOOR_SEL_FLAG);
				}

			}
		}
	}}


}

bool BoundaryMesh::SelectHitIDoor(int idoor) {

	if (idoor >= (int)d_iDoorList.size()) {
		return false;
	}

	ClearIDoorSelection();

	SmartIDoorListIter d = d_iDoorList.begin();
	while (idoor) {
		d++;
		idoor--;
	}
	d_selectedIDoor = d;

	IDoor* newseldoor = (*d).GetPointer();

	d_hitID			= newseldoor->GetID();
	d_flippedDoor	= newseldoor->GetFlipped();
	d_verticalDoor	= newseldoor->GetVerticalAlignment();

	{for (int v = 0; v < newseldoor->NumVerts(); v++) {
		d_selectedIDoorVertList.push_back(newseldoor->d_vertIndexArray[v]);
	}}

	return true;
	
}

void BoundaryMesh::DeselectAllIDoors(void) {

	ClearIDoorSelection();
	
}
		

bool BoundaryMesh::BuildIDoor(bool keep_vert_list) {

	// Builds the selected i-door

	if (d_selectedIDoor == d_iDoorList.end()) {
		return false;	
	}
	
	IDoor* newdoor = (*d_selectedIDoor).GetPointer();

	if (!newdoor) {
		return false;
	}

	if (!keep_vert_list) {

		// Copy all the selected verts to the door
		for (std::list<int>::iterator i = d_selectedIDoorVertList.begin(); i != d_selectedIDoorVertList.end(); ++i ) {
//			newdoor->d_vertList.push_back(P(*i));
			newdoor->d_vertIndexArray.push_back(*i);
		}

	} else {

		// Select all the verts from the existing door...
		d_selectedIDoorVertList.clear();

		{for (int v = 0; v < newdoor->NumVerts(); v++) {
			d_selectedIDoorVertList.push_back(newdoor->d_vertIndexArray[v]);
		}}

	}

	Point3 pointA = P(*(newdoor->d_vertIndexArray.begin()));
	Point3 pointB = P(*(newdoor->d_vertIndexArray.rbegin()));

	newdoor->SetCenterPoint((pointA + pointB)/2);

	Point3 x_axis,y_axis,z_axis;

	if (newdoor->GetVerticalAlignment()) {
		y_axis = Point3(0,1,0);
	} else {
		y_axis = Point3(0,0,1);
	}
	
	if (newdoor->GetFlipped()) {
		x_axis = pointA - pointB;
	} else {
		x_axis = pointB - pointA;
	}

	z_axis = CrossProd(x_axis,y_axis);

	if (Length(z_axis) < 0.0001) {
		double t = x_axis[0];
		if (newdoor->GetVerticalAlignment()) {
		x_axis[0] = x_axis[2];
		x_axis[2] = -t;    	
		} else {
		x_axis[0] = x_axis[1];
		x_axis[1] = -t;    	
		}
		z_axis = CrossProd(x_axis,y_axis);
	}
	
	x_axis = CrossProd(y_axis,z_axis);
	
	Matrix3 m(1);
			
	m.SetRow(0,Normalize(x_axis));
	m.SetRow(1,Normalize(y_axis));
	m.SetRow(2,Normalize(z_axis));
	newdoor->SetOrientation(m);	 

	return true;
}


void BoundaryMesh::BuildAllIDoors(bool loading) {
	
	{for (SmartIDoorListIter d = d_iDoorList.begin();
		  d != d_iDoorList.end();
		  ++d) {
	
		d_selectedIDoor = d;

		BuildIDoor(true);

		int v = (*d)->NumVerts();
		
	}}
	
}

bool BoundaryMesh::AddIDoor(bool loading) {


	if (d_selectedIDoorVertList.size() == 0) return false;

	bool flipper = false;
	bool aligner = false;

	int id;

	SmartIDoorListIter next_door;
	
	if (loading) {

		flipper = 0 != d_flippedDoor;
		aligner = 0 != d_verticalDoor;
		id = d_hitID;

		next_door = d_iDoorList.end();	// Insert this door at the end of the list

	} else {

		// Find the next available door id
		id = 1;
		for (next_door = d_iDoorList.begin() ; next_door != d_iDoorList.end(); ++next_door) {
			if ((*next_door)->GetID() != id) {
				break;
			}
			id++;
		}

		d_hitID = id;
	
	}
	
	SmartIDoor newidoor = SmartIDoor(new IDoor(flipper,aligner,id));

	if (newidoor) {

		d_selectedIDoor = d_iDoorList.insert(next_door,newidoor);
	
		if (!loading) {
			BuildIDoor(false);
		} else {
			// Copy all the selected verts to the door
			// wait for the post load callback before we derive the guts of the door
			for (std::list<int>::iterator i = d_selectedIDoorVertList.begin(); i != d_selectedIDoorVertList.end(); ++i ) {
				newidoor->d_vertIndexArray.push_back(*i);
			}
		}

		ClearIDoorSelection();
		
		return true;
	}

	return false;
}

bool BoundaryMesh::FlipIDoor() {

	if (d_selectedIDoor != d_iDoorList.end()) {
		
		bool flip_state = (*d_selectedIDoor)->GetFlipped();
		flip_state = !flip_state;
		(*d_selectedIDoor)->SetFlipped(flip_state);

		// Rebuild with reversed orientation, keeping verts intact
		BuildIDoor(true);

		return true;
	}
	
	return false;

}


bool BoundaryMesh::OrientIDoor() {

	if (d_selectedIDoor != d_iDoorList.end()) {
		
		bool state = (*d_selectedIDoor)->GetVerticalAlignment();
		state = !state;
		(*d_selectedIDoor)->SetVerticalAlignment(state);

		// Rebuild with new orientation, keeping verts intact
		BuildIDoor(true);

		return true;
	}
	
	return false;

}


bool BoundaryMesh::DeleteIDoor() {

	// clear all the USED_IN_DOOR flags and
	// delete the selected door from the list

	if (d_selectedIDoor != d_iDoorList.end()) {

		d_iDoorList.erase(d_selectedIDoor);
		d_selectedIDoor = d_iDoorList.end();

		ClearIDoorSelection();

		return true;
	}
		
	return false;
}

void BoundaryMesh::DeleteAllIDoors(void) {
	
	// Find the door the lies along 'loop' and contains 'vert'

	d_selectedIDoor = d_iDoorList.begin();

	while (d_selectedIDoor != d_iDoorList.end()) {
	
		DeleteIDoor();

		d_selectedIDoor = d_iDoorList.begin();
		
	}
	
}

IDoor* BoundaryMesh::GetIDoor(const int dnum) {

	// search through the list of doors for the requested one

	if (dnum >= (int)d_iDoorList.size()) {
		return 0;
	}

	SmartIDoorListIter d = d_iDoorList.begin();


	int count = dnum;

	while (count) {
		d++;
		count--;


	}

	return (*d).GetPointer();
}

/*
int	BoundaryMesh::GetIDoorVerts(const int dnum, std::list<int>& vlist) {

	vlist.clear();

	IDoor* vdoor = GetIDoor(dnum);

	if (!vdoor) return 0;
	int i = vdoor->GetFirstEdge();

	// TODO:biddle put this in a GetBorderLoop(lindex)
	IntTab* lp = d_borderLoops.Loop(vdoor->GetLoopUsed());
	
	MNEdge* edge =  E((*lp)[i]);
	vlist.push_back(edge->v2);
	
	while (i != vdoor->GetLastEdge()) {
		
		vlist.push_back(edge->v1);

		i = vdoor->NextEdge(i);
		edge = E((*lp)[i]);
	}

//	vlist.push_back(edge->v2);
	vlist.push_back(edge->v1);

	return vlist.size();
return 0;

}
*/

//******************************************
//
// Floors
//
//******************************************

int BoundaryMesh::FloorHitTest(ViewExp *vpt, ModContext* mc, int flags) {

	INode* boundNode = d_loopSource;
	
	if (!boundNode) {
		return FALSE;
	}
		
	int res = RET_HIT_NOTHING;
	
	GraphicsWindow *gw = vpt->getGW();

	if (flags) {
		bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
		bool selOnly    = flags&SUBHIT_SELONLY?true:false;
		bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
	}

	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
	gw->setTransform(ntm);

	DWORD closest = (DWORD)-1;
	int closest_face = -1;
		
	Point3 xyz[4];
	Point3 rgb[4];
	Point3 uvw[4];

	rgb[0]=	rgb[1]=	rgb[2]=	rgb[3]= Point3(1,1,1);
	uvw[0]=	uvw[1]=	uvw[2]=	uvw[3]= Point3(0,0,0);
		
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);
			
		xyz[0] = V(f->vtx[0])->p;
		xyz[1] = V(f->vtx[1])->p;
		xyz[2] = V(f->vtx[2])->p;
			
		gw->polygon(3,xyz,rgb,uvw);
			
		if (gw->checkHitCode()) {
						
			DWORD dist = gw->getHitDistance();
			if (dist <= closest) {
				closest = dist;
				closest_face = i;

			}
						
			res = RET_HIT_FLOOR;
	
			f->SetFlag(HIT_FLAG);					

		} else {
			
			f->ClearFlag(HIT_FLAG);	
		}
					
		gw->clearHitCode();
				
	}}

	if (res == RET_HIT_FLOOR) {
		vpt->LogHit(boundNode, mc, 0, closest_face);
	}

	return (res);
		
	
}

void BoundaryMesh::FloorSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

 	bool flip = (GetKeyState(VK_SHIFT) & (1<<15)) != 0;
 	bool append = (GetKeyState(VK_CONTROL) & (1<<15)) != 0;
 	bool remove = (GetKeyState(VK_MENU) & (1<<15)) != 0;
		
	d_selectedFloorFaceCount = 0;
	
	{for (int j=0; j<FNum(); ++j) {

		MNFace* face = F(j);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			face->ClearFlag(SELECTED_FLAG);
		}
		
		// Set the flag appropriately
		if (face->GetFlag(HIT_FLAG)) {
			
			if (flip) {
				face->SetFlag(SELECTED_FLAG,!face->GetFlag(SELECTED_FLAG));
			} else if (remove) {
				face->ClearFlag(SELECTED_FLAG);			
			} else {
				face->SetFlag(SELECTED_FLAG);
			}
		}
		
		// Tally
		if (face->GetFlag(SELECTED_FLAG)) {
			d_selectedFloorFaceCount++;
		}
	}}

}

void BoundaryMesh::SetFloorSelectFromOriginal(void) {

	// Find out which faces are selected in the original and update our selection with them

	Object* boundobj= d_loopSource->EvalWorldState(0).obj;

	if (boundobj->CanConvertToType(triObjectClassID)) {

		TriObject *temptriobj;
		
		temptriobj = (TriObject *)boundobj->ConvertToType(0,triObjectClassID);

		Mesh& tempmesh = temptriobj->GetMesh();

		BitArray& selfaces = tempmesh.FaceSel();

		{for (int j=0; j<FNum(); ++j) {
			MNFace* f = F(j);
			if (selfaces[j]) {
				f->SetFlag(SELECTED_FLAG);
			} else {
				f->ClearFlag(SELECTED_FLAG);
			}
		}}

		d_selectedFloorFaceCount = selfaces.NumberSet();

	}

}

void BoundaryMesh::DrawFloor(ViewExp *vpt)
{


	GraphicsWindow *gw = vpt->getGW();

	INode* boundNode = d_loopSource;
	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0

//	gw->setTransform(ntm);
	gw->setTransform(Matrix3(1));
	
	Point3 xyz[4];
	
	Color wirecolour= boundNode->GetWireColor();
	gw->setColor( FILL_COLOR, wirecolour*0.75f );
	
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->deg == 3) {

			if (f->GetFlag(SELECTED_FLAG|FLOOR_FACE_FLAG)) {		

				xyz[0] = ntm*V(f->vtx[0])->p;
				xyz[1] = ntm*V(f->vtx[1])->p;
				xyz[2] = ntm*V(f->vtx[2])->p;
				
				if (f->GetFlag(FLOOR_FACE_FLAG)) {		
					gw->polygon(3,xyz,0,0);
				}
				if (f->GetFlag(SELECTED_FLAG)) {		
					gw->setColor( LINE_COLOR, Point3(0.8,0.8,0.8) );
				} else {
					gw->setColor( LINE_COLOR, wirecolour);
				}
				gw->polyline(3,xyz,0,0,TRUE,0);

			}
		}
			
	}}
		
}

void BoundaryMesh::AddFloor(void)
{
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->GetFlag(SELECTED_FLAG)) {
			if (!f->GetFlag(FLOOR_FACE_FLAG)) {
				f->SetFlag(FLOOR_FACE_FLAG);
				f->ClearFlag(IGNORED_FLAG);
				d_floorFaceCount++;
			}
		}
	}}
}

void BoundaryMesh::DeleteFloor(void)
{
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->GetFlag(SELECTED_FLAG)) {
			if (f->GetFlag(FLOOR_FACE_FLAG)) {
				f->ClearFlag(FLOOR_FACE_FLAG);
				d_floorFaceCount--;
			}
		}
	}}
	
}
void BoundaryMesh::ResetFloor(void)
{
	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		f->ClearFlag(FLOOR_FACE_FLAG);

	}}

	d_floorFaceCount = 0;
	d_selectedFloorFaceCount = 0;
	
}

//******************************************
//
// Water
//
//******************************************

int BoundaryMesh::WaterHitTest(ViewExp *vpt, ModContext* mc, int flags) {

	INode* boundNode = d_loopSource;
	
	if (!boundNode) {
		return FALSE;
	}
		
	int res = RET_HIT_NOTHING;
	
	GraphicsWindow *gw = vpt->getGW();

	if (flags) {
		bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
		bool selOnly    = flags&SUBHIT_SELONLY?true:false;
		bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
	}

	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
	gw->setTransform(ntm);

	DWORD closest = (DWORD)-1;
	int closest_face = -1;
		
	Point3 xyz[4];
	Point3 rgb[4];
	Point3 uvw[4];

	rgb[0]=	rgb[1]=	rgb[2]=	rgb[3]= Point3(1,1,1);
	uvw[0]=	uvw[1]=	uvw[2]=	uvw[3]= Point3(0,0,0);
		
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);
			
		xyz[0] = V(f->vtx[0])->p;
		xyz[1] = V(f->vtx[1])->p;
		xyz[2] = V(f->vtx[2])->p;
			
		gw->polygon(3,xyz,rgb,uvw);
			
		if (gw->checkHitCode()) {
						
			DWORD dist = gw->getHitDistance();
			if (dist <= closest) {
				closest = dist;
				closest_face = i;

			}
						
			res = RET_HIT_WATER;
	
			f->SetFlag(HIT_FLAG);					

		} else {
			
			f->ClearFlag(HIT_FLAG);	
		}
					
		gw->clearHitCode();
				
	}}

	if (res == RET_HIT_WATER) {
		vpt->LogHit(boundNode, mc, 0, closest_face);
	}

	return (res);
		
	
}

void BoundaryMesh::WaterSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

 	bool flip = (GetKeyState(VK_SHIFT) & (1<<15)) != 0;
 	bool append = (GetKeyState(VK_CONTROL) & (1<<15)) != 0;
 	bool remove = (GetKeyState(VK_MENU) & (1<<15)) != 0;
		
	d_selectedWaterFaceCount = 0;
	
	{for (int j=0; j<FNum(); ++j) {

		MNFace* face = F(j);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			face->ClearFlag(SELECTED_FLAG);
		}
		
		// Set the flag appropriately
		if (face->GetFlag(HIT_FLAG)) {
			
			if (flip) {
				face->SetFlag(SELECTED_FLAG,!face->GetFlag(SELECTED_FLAG));
			} else if (remove) {
				face->ClearFlag(SELECTED_FLAG);			
			} else {
				face->SetFlag(SELECTED_FLAG);
			}
		}
		
		// Tally
		if (face->GetFlag(SELECTED_FLAG)) {
			d_selectedWaterFaceCount++;
		}
	}}

}

void BoundaryMesh::SetWaterSelectFromOriginal(void) {

	// Find out which faces are selected in the original and update our selection with them

	Object* boundobj= d_loopSource->EvalWorldState(0).obj;

	if (boundobj->CanConvertToType(triObjectClassID)) {

		TriObject *temptriobj;
		
		temptriobj = (TriObject *)boundobj->ConvertToType(0,triObjectClassID);

		Mesh& tempmesh = temptriobj->GetMesh();

		BitArray& selfaces = tempmesh.FaceSel();

		{for (int j=0; j<FNum(); ++j) {
			MNFace* f = F(j);
			if (selfaces[j]) {
				f->SetFlag(SELECTED_FLAG);
			} else {
				f->ClearFlag(SELECTED_FLAG);
			}
		}}

		d_selectedWaterFaceCount = selfaces.NumberSet();

	}

}

void BoundaryMesh::DrawWater(ViewExp *vpt)
{


	GraphicsWindow *gw = vpt->getGW();

	INode* boundNode = d_loopSource;
	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0

//	gw->setTransform(ntm);
	gw->setTransform(Matrix3(1));
	
	Point3 xyz[4];
	
	Color wirecolour= boundNode->GetWireColor();
	gw->setColor( FILL_COLOR, wirecolour*0.25f );
	
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->deg == 3) {

			if (f->GetFlag(SELECTED_FLAG|WATER_FACE_FLAG)) {		

				xyz[0] = ntm*V(f->vtx[0])->p;
				xyz[1] = ntm*V(f->vtx[1])->p;
				xyz[2] = ntm*V(f->vtx[2])->p;
				
				if (f->GetFlag(WATER_FACE_FLAG)) {		
					gw->polygon(3,xyz,0,0);
				}
				if (f->GetFlag(SELECTED_FLAG)) {		
					gw->setColor( LINE_COLOR, Point3(0.8,0.8,0.8) );
				} else {
					gw->setColor( LINE_COLOR, wirecolour);
				}
				gw->polyline(3,xyz,0,0,TRUE,0);

			}
		}
			
	}}
		
}

void BoundaryMesh::AddWater(void)
{
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->GetFlag(SELECTED_FLAG)) {
			if (!f->GetFlag(WATER_FACE_FLAG)) {
				f->SetFlag(WATER_FACE_FLAG);
				f->ClearFlag(IGNORED_FLAG);
				d_waterFaceCount++;
			}
		}
	}}
}

void BoundaryMesh::DeleteWater(void)
{
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->GetFlag(SELECTED_FLAG)) {
			if (f->GetFlag(WATER_FACE_FLAG)) {
				f->ClearFlag(WATER_FACE_FLAG);
				d_waterFaceCount--;
			}
		}
	}}
	
}
void BoundaryMesh::ResetWater(void)
{
	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		f->ClearFlag(WATER_FACE_FLAG);

	}}

	d_waterFaceCount = 0;
	d_selectedWaterFaceCount = 0;
	
}

//******************************************
//
// Ignored (faces)
//
//******************************************

int BoundaryMesh::IgnoredHitTest(ViewExp *vpt, ModContext* mc, int flags) {

	INode* boundNode = d_loopSource;
	
	if (!boundNode) {
		return FALSE;
	}
		
	int res = RET_HIT_NOTHING;
	
	GraphicsWindow *gw = vpt->getGW();

	if (flags) {
		bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
		bool selOnly    = flags&SUBHIT_SELONLY?true:false;
		bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
	}

	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
	gw->setTransform(ntm);

	DWORD closest = (DWORD)-1;
	int closest_face = -1;
		
	Point3 xyz[4];
	Point3 rgb[4];
	Point3 uvw[4];

	rgb[0]=	rgb[1]=	rgb[2]=	rgb[3]= Point3(1,1,1);
	uvw[0]=	uvw[1]=	uvw[2]=	uvw[3]= Point3(0,0,0);
		
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);
			
		xyz[0] = V(f->vtx[0])->p;
		xyz[1] = V(f->vtx[1])->p;
		xyz[2] = V(f->vtx[2])->p;
			
		gw->polygon(3,xyz,rgb,uvw);
			
		if (gw->checkHitCode()) {
						
			DWORD dist = gw->getHitDistance();
			if (dist <= closest) {
				closest = dist;
				closest_face = i;

			}
						
			res = RET_HIT_IGNORED;
	
			f->SetFlag(IGNORED_HIT_FLAG);					

		} else {
			
			f->ClearFlag(IGNORED_HIT_FLAG);	
		}
					
		gw->clearHitCode();
				
	}}

	if (res == RET_HIT_IGNORED) {
		vpt->LogHit(boundNode, mc, 0, closest_face);
	}

	return (res);
		
	
}

void BoundaryMesh::IgnoredSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

 	bool flip = (GetKeyState(VK_SHIFT) & (1<<15)) != 0;
 	bool append = (GetKeyState(VK_CONTROL) & (1<<15)) != 0;
 	bool remove = (GetKeyState(VK_MENU) & (1<<15)) != 0;
		
	d_selectedIgnoredCount = 0;
	
	{for (int j=0; j<FNum(); ++j) {

		MNFace* face = F(j);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			face->ClearFlag(IGNORED_SEL_FLAG);
		}
		
		// Set the flag appropriately
		if (face->GetFlag(IGNORED_HIT_FLAG) &&
			!face->GetFlag(FLOOR_FACE_FLAG) &&
			!face->GetFlag(WATER_FACE_FLAG) )
		{
			if (flip) {
				face->SetFlag(IGNORED_SEL_FLAG,!face->GetFlag(IGNORED_SEL_FLAG));
			} else if (remove) {
				face->ClearFlag(IGNORED_SEL_FLAG);			
			} else {
				face->SetFlag(IGNORED_SEL_FLAG);
			}
		}
		
		// Tally
		if (face->GetFlag(IGNORED_SEL_FLAG)) {
			d_selectedIgnoredCount++;
		}
	}}

}

void BoundaryMesh::DrawIgnored(ViewExp *vpt)
{


	GraphicsWindow *gw = vpt->getGW();

	INode* boundNode = d_loopSource;
	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0

//	gw->setTransform(ntm);
	gw->setTransform(Matrix3(1));
	
	Point3 xyz[4];
	
//	Color wirecolour= boundNode->GetWireColor();
	gw->setColor( FILL_COLOR, Point3(0.8,0.2,0.8)  );
	
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->deg == 3) {

			if (f->GetFlag(IGNORED_SEL_FLAG|IGNORED_FLAG)) {		

				xyz[0] = ntm*V(f->vtx[0])->p;
				xyz[1] = ntm*V(f->vtx[1])->p;
				xyz[2] = ntm*V(f->vtx[2])->p;
				
				if (f->GetFlag(IGNORED_FLAG)) {		
					gw->polygon(3,xyz,0,0);
				}
				if (f->GetFlag(IGNORED_SEL_FLAG)) {		
					gw->setColor( LINE_COLOR, Point3(0.8,0.8,0.8) );
				} else {
					gw->setColor( LINE_COLOR, Point3(0.9,0.4,0.9));
				}
				gw->polyline(3,xyz,0,0,TRUE,0);

			}
		}
			
	}}
	
}

void BoundaryMesh::AddIgnored(void)
{
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->GetFlag(IGNORED_SEL_FLAG)) {
			if (!f->GetFlag(IGNORED_FLAG)) {
				f->SetFlag(IGNORED_FLAG);
				d_ignoredCount++;
			}
		}
	}}
}

void BoundaryMesh::DeleteIgnored(void)
{
	{for (int i=0; i < FNum(); ++i){

		MNFace* f = F(i);

		if (f->GetFlag(IGNORED_SEL_FLAG)) {
			if (f->GetFlag(IGNORED_FLAG)) {
				f->ClearFlag(IGNORED_FLAG);
				d_ignoredCount--;
			}
		}
	}}
	
}
void BoundaryMesh::ResetIgnored(void)
{
	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		f->ClearFlag(IGNORED_FLAG);
		f->ClearFlag(IGNORED_SEL_FLAG);

	}}

	d_ignoredCount = 0;
	d_selectedIgnoredCount = 0;
	
}

void BoundaryMesh::SetIgnoredSelectFromOriginal(void) {

	// Find out which faces are selected in the original and update our selection with them

	Object* boundobj= d_loopSource->EvalWorldState(0).obj;

	if (boundobj->CanConvertToType(triObjectClassID)) {

		TriObject *temptriobj;
		
		temptriobj = (TriObject *)boundobj->ConvertToType(0,triObjectClassID);

		Mesh& tempmesh = temptriobj->GetMesh();

		BitArray& selfaces = tempmesh.FaceSel();

		{for (int j=0; j<FNum(); ++j) {
			MNFace* f = F(j);
			if (selfaces[j]) {
				f->SetFlag(IGNORED_SEL_FLAG);
			} else {
				f->ClearFlag(IGNORED_SEL_FLAG);
			}
		}}

		d_selectedIgnoredCount = selfaces.NumberSet();

	}

}

//******************************************
//
// Locked Normals
//
//******************************************

int BoundaryMesh::NormsHitTest(ViewExp *vpt, ModContext* mc, int flags) {

	INode* boundNode = d_loopSource;
	
	if (!boundNode) {
		return FALSE;
	}
		
	int res = RET_HIT_NOTHING;
	
	GraphicsWindow *gw = vpt->getGW();

	if (flags) {
		bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
		bool selOnly    = flags&SUBHIT_SELONLY?true:false;
		bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
	}

	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
	gw->setTransform(ntm);

	DWORD closest = (DWORD)-1;
	int closest_vert = -1;
		
	Point3 xyz;
	
	{for (int i=0; i < VNum(); ++i){
	
		MNVert* v = V(i);

		gw->marker(&v->p,TRIANGLE_MRKR);
		
		if (gw->checkHitCode()) {
						
			DWORD dist = gw->getHitDistance();
			if (dist <= closest) {
				closest = dist;
				closest_vert = i;

			}
						
			res = RET_HIT_NORM;
	
			v->SetFlag(NORM_HIT_FLAG);					

		} else {
			
			v->ClearFlag(NORM_HIT_FLAG);	
		}
					
		gw->clearHitCode();
				
	}}

	if (res != RET_HIT_NOTHING) {
		vpt->LogHit(boundNode, mc, 0, closest_vert);
	}

	return (res);
		
	
}

void BoundaryMesh::NormsSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

 	bool flip = (GetKeyState(VK_SHIFT) & (1<<15)) != 0;
 	bool append = (GetKeyState(VK_CONTROL) & (1<<15)) != 0;
 	bool remove = (GetKeyState(VK_MENU) & (1<<15)) != 0;
		
	d_selectedNormCount = 0;
	
	{for (int j=0; j<VNum(); ++j) {

		MNVert* vert = V(j);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			vert->ClearFlag(IGNORED_SEL_FLAG);
		}
		
		// Set the flag appropriately
		if (vert->GetFlag(NORM_HIT_FLAG) ) {
			
			if (flip) {
				vert->SetFlag(NORM_SEL_FLAG,!vert->GetFlag(NORM_SEL_FLAG));
			} else if (remove) {
				vert->ClearFlag(NORM_SEL_FLAG);			
			} else {
				vert->SetFlag(NORM_SEL_FLAG);
			}
		}
		
		// Tally
		if (vert->GetFlag(NORM_SEL_FLAG)) {
			d_selectedNormCount++;
		}
	}}

}

void BoundaryMesh::DrawNorms(ViewExp *vpt)
{


	GraphicsWindow *gw = vpt->getGW();

	INode* boundNode = d_loopSource;
	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0

	gw->setTransform(ntm);
	
	Point3 xyz[4];
		
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		if (v->GetFlag(NORM_SEL_FLAG|LOCKED_NORM_FLAG)) {		

			// Should draw a vertical arrow!
			if (v->GetFlag(NORM_SEL_FLAG)) {
				gw->setColor( LINE_COLOR, GetUIColor(COLOR_SELECTION) );
			} else {
				gw->setColor( LINE_COLOR, Point3(0.9f,0.9f,0.0f)  );
			}

			gw->marker(&v->p,TRIANGLE_MRKR);

		}
			
	}}
	
}

void BoundaryMesh::LockNorms(void)
{
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		if (v->GetFlag(NORM_SEL_FLAG)) {
			v->ClearFlag(NORM_SEL_FLAG);
			if (!v->GetFlag(LOCKED_NORM_FLAG)) {
				v->SetFlag(LOCKED_NORM_FLAG);
				if (v->GetFlag(SPOOFED_FLAG)) {
					v->ClearFlag(SPOOFED_FLAG);
					d_spoofedNormCount--;
				}
				d_lockedNormCount++;
			}
		}
	}}
}

void BoundaryMesh::UnlockNorms(void)
{
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		if (v->GetFlag(NORM_SEL_FLAG)) {
			v->ClearFlag(NORM_SEL_FLAG);
			if (v->GetFlag(LOCKED_NORM_FLAG)) {
				v->ClearFlag(LOCKED_NORM_FLAG);
				d_lockedNormCount--;
			}
		}
	}}
}

void BoundaryMesh::ResetNorms(void)
{
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		v->ClearFlag(NORM_SEL_FLAG);
		v->ClearFlag(LOCKED_NORM_FLAG);

	}}

	d_lockedNormCount = 0;
	d_selectedNormCount = 0;
	
}

//******************************************
//
// Spoofed Normals
//
//******************************************

int BoundaryMesh::SpoofedHitTest(ViewExp *vpt, ModContext* mc, int flags) {

	INode* boundNode = d_loopSource;
	
	if (!boundNode) {
		return FALSE;
	}
		
	int res = RET_HIT_NOTHING;
	
	GraphicsWindow *gw = vpt->getGW();

	if (flags) {
		bool abortOnHit = flags&SUBHIT_ABORTONHIT?true:false;
		bool selOnly    = flags&SUBHIT_SELONLY?true:false;
		bool unselOnly  = flags&SUBHIT_UNSELONLY?true:false;
	}

	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0
	gw->setTransform(ntm);

	DWORD closest = (DWORD)-1;
	int closest_vert = -1;
		
	Point3 xyz;
	
	{for (int i=0; i < VNum(); ++i){
	
		MNVert* v = V(i);

		gw->marker(&v->p,TRIANGLE_MRKR);
		
		if (gw->checkHitCode()) {
						
			DWORD dist = gw->getHitDistance();
			if (dist <= closest) {
				closest = dist;
				closest_vert = i;

			}
						
			res = RET_HIT_NORM;
	
			v->SetFlag(HIT_FLAG);					

		} else {
			
			v->ClearFlag(HIT_FLAG);	
		}
					
		gw->clearHitCode();
				
	}}

	if (res != RET_HIT_NOTHING) {
		vpt->LogHit(boundNode, mc, 0, closest_vert);
	}

	return (res);
		
	
}

void BoundaryMesh::SpoofedSelect(BOOL selected, BOOL all, BOOL invert, bool DoorsOnly) {

 	bool flip = (GetKeyState(VK_SHIFT) & (1<<15)) != 0;
 	bool append = (GetKeyState(VK_CONTROL) & (1<<15)) != 0;
 	bool remove = (GetKeyState(VK_MENU) & (1<<15)) != 0;
		
	d_selectedSpoofedNormCount = 0;
	
	{for (int j=0; j<VNum(); ++j) {

		MNVert* vert = V(j);

		// Clear the flag if there is no modifier
		if (!(flip|append|remove)) {
			vert->ClearFlag(SPOOFED_SEL_FLAG);
		}
		
		// Set the flag appropriately
		if (vert->GetFlag(HIT_FLAG) ) {
			
			if (flip) {
				vert->SetFlag(SPOOFED_SEL_FLAG,!vert->GetFlag(SPOOFED_SEL_FLAG));
			} else if (remove) {
				vert->ClearFlag(SPOOFED_SEL_FLAG);			
			} else {
				vert->SetFlag(SPOOFED_SEL_FLAG);
			}
		}
		
		// Tally
		if (vert->GetFlag(SPOOFED_SEL_FLAG)) {
			d_selectedSpoofedNormCount++;
		}
	}}

}

void BoundaryMesh::DrawSpoofed(ViewExp *vpt)
{


	GraphicsWindow *gw = vpt->getGW();

	INode* boundNode = d_loopSource;
	Matrix3 ntm = boundNode->GetObjectTM(0); // not animating ,so time==0

	gw->setTransform(ntm);
	
	Point3 xyz[4];
		
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		if (v->GetFlag(SPOOFED_SEL_FLAG|SPOOFED_FLAG)) {		

			// Should draw a vertical arrow!
			if (v->GetFlag(SPOOFED_SEL_FLAG)) {
				gw->setColor( LINE_COLOR, GetUIColor(COLOR_SELECTION) );
			} else {
				gw->setColor( LINE_COLOR, Point3(0.9f,0.9f,0.0f)  );
			}

			gw->marker(&v->p,DIAMOND_MRKR);

		}
			
	}}
	
}

void BoundaryMesh::SpoofNorms(void)
{
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		if (v->GetFlag(SPOOFED_SEL_FLAG)) {
			v->ClearFlag(SPOOFED_SEL_FLAG);
			if (!v->GetFlag(SPOOFED_FLAG)) {
				v->SetFlag(SPOOFED_FLAG);
				if (v->GetFlag(LOCKED_NORM_FLAG)) {
					v->ClearFlag(LOCKED_NORM_FLAG);
					d_lockedNormCount--;
				}
				d_spoofedNormCount++;
			}
		}
	}}
}

void BoundaryMesh::UnspoofNorms(void)
{
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		if (v->GetFlag(SPOOFED_SEL_FLAG)) {
			v->ClearFlag(SPOOFED_SEL_FLAG);
			if (v->GetFlag(SPOOFED_FLAG)) {
				v->ClearFlag(SPOOFED_FLAG);
				d_spoofedNormCount--;
			}
		}
	}}
}

void BoundaryMesh::ResetSpoofed(void)
{
	{for (int i=0; i < VNum(); ++i){

		MNVert* v = V(i);

		v->ClearFlag(SPOOFED_SEL_FLAG);
		v->ClearFlag(SPOOFED_FLAG);

	}}

	d_spoofedNormCount = 0;
	d_selectedSpoofedNormCount = 0;
	
}

#define SNO_BOUNDARY_CHUNK		0xF00B
#define SNO_DOOR_CHUNK		    0xF00D
#define SNO_DOOR2_CHUNK		    0xF02D
#define SNO_IDOOR_CHUNK		    0xF01D
#define SNO_FLOOR_CHUNK		    0xF00F
#define SNO_WATER_CHUNK			0xF00E
#define SNO_IGNORED_CHUNK		0xF001
#define SNO_LOCKED_NORM_CHUNK	0xF002
#define SNO_SPOOFED_NORM_CHUNK	0xF003

//******************************************
IOResult BoundaryMesh::Save(ISave *isave) {
	
	ULONG nb;
	
	isave->BeginChunk(SNO_BOUNDARY_CHUNK);
	isave->Write(&d_doorLoopCount, sizeof(int), &nb);
	isave->EndChunk();
	
	{for (SmartDoorListIter d = d_doorList.begin();
		  d != d_doorList.end();
		  ++d) {

		isave->BeginChunk(SNO_DOOR2_CHUNK);

		int loop  = (*d)->GetLoopUsed();
		int first = (*d)->GetFirstEdge();
		int last  = (*d)->GetLastEdge();
		int flip = (*d)->GetFlipped();
		int verta = (*d)->GetVerticalAlignment();
		UINT32 id = (*d)->GetID();
		
		isave->Write(&id ,sizeof(UINT32),&nb);
		isave->Write(&loop ,sizeof(int),&nb);
		isave->Write(&first,sizeof(int),&nb);
		isave->Write(&last ,sizeof(int),&nb);
		isave->Write(&flip ,sizeof(int),&nb);
		isave->Write(&verta ,sizeof(int),&nb);

		isave->EndChunk();
		
		

	}}

	{for (SmartIDoorListIter d = d_iDoorList.begin();
		  d != d_iDoorList.end();
		  ++d) {

		isave->BeginChunk(SNO_IDOOR_CHUNK);

		int flip = (*d)->GetFlipped();
		int verta = (*d)->GetVerticalAlignment();
		UINT32 id = (*d)->GetID();
		
		isave->Write(&id ,sizeof(UINT32),&nb);
		isave->Write(&flip ,sizeof(int),&nb);
		isave->Write(&verta ,sizeof(int),&nb);

		int numverts = (*d)->d_vertIndexArray.size();

		isave->Write(&numverts ,sizeof(int),&nb);

		for (int i = 0; i < numverts ; ++i ) {
			isave->Write(&((*d)->d_vertIndexArray[0]) ,numverts*sizeof(int),&nb);
		}

		isave->EndChunk();
		
		

	}}

	isave->BeginChunk(SNO_FLOOR_CHUNK);

	isave->Write(&d_floorFaceCount, sizeof(d_floorFaceCount), &nb);

	int written = 0;

	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		
		if (f->GetFlag(FLOOR_FACE_FLAG)) {
			unsigned short outval = i;
			isave->Write(&outval, sizeof(outval), &nb);
			written++;
		}

	}}

	assert(written == d_floorFaceCount);
		
	isave->EndChunk();

	isave->BeginChunk(SNO_WATER_CHUNK);

	isave->Write(&d_waterFaceCount, sizeof(d_waterFaceCount), &nb);

	written = 0;

	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		
		if (f->GetFlag(WATER_FACE_FLAG)) {
			unsigned short outval = i;
			isave->Write(&outval, sizeof(outval), &nb);
			written++;
		}

	}}

	assert(written == d_waterFaceCount);
		
	isave->EndChunk();

	isave->BeginChunk(SNO_IGNORED_CHUNK);

	isave->Write(&d_ignoredCount, sizeof(d_ignoredCount), &nb);

	written = 0;

	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		
		if (f->GetFlag(IGNORED_FLAG)) {
			unsigned short outval = i;
			isave->Write(&outval, sizeof(outval), &nb);
			written++;
		}

	}}

	assert(written == d_ignoredCount);
		
	isave->EndChunk();

	isave->BeginChunk(SNO_LOCKED_NORM_CHUNK);

	isave->Write(&d_lockedNormCount, sizeof(d_lockedNormCount), &nb);

	written = 0;

	{for (int i=0; i < VNum(); ++i) {

		MNVert* v = V(i);
		
		if (v->GetFlag(LOCKED_NORM_FLAG)) {
			unsigned short outval = i;
			isave->Write(&outval, sizeof(outval), &nb);
			written++;
		}

	}}

	assert(written == d_lockedNormCount);
		
	isave->EndChunk();

	isave->BeginChunk(SNO_SPOOFED_NORM_CHUNK);

	isave->Write(&d_spoofedNormCount, sizeof(d_spoofedNormCount), &nb);

	written = 0;

	{for (int i=0; i < VNum(); ++i) {

		MNVert* v = V(i);
		
		if (v->GetFlag(SPOOFED_FLAG)) {
			unsigned short outval = i;
			isave->Write(&outval, sizeof(outval), &nb);
			written++;
		}

	}}

	assert(written == d_spoofedNormCount);
		
	isave->EndChunk();

	return IO_OK;
}

//******************************************
IOResult BoundaryMesh::Load(ILoad *iload, IndexList& floor_faces, IndexList& water_faces, IndexList& ignored_faces, IndexList& locked_norms, IndexList& spoofed_norms) {
	
	ULONG nb;
	IOResult res;

	int supposed_door_count = 0;
	
	d_doorLoopCount = 0;
	
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {

			case SNO_BOUNDARY_CHUNK:
				res=iload->Read(&supposed_door_count,sizeof(int), &nb);
				break;

			case SNO_DOOR_CHUNK:		
				res=iload->Read(&d_hitLoop, sizeof(int), &nb);
				res=iload->Read(&d_firstEdge, sizeof(int), &nb);
				res=iload->Read(&d_lastEdge, sizeof(int), &nb);
				res=iload->Read(&d_flippedDoor, sizeof(bool), &nb);
				res=iload->Read(&d_hitID, sizeof(UINT32), &nb);
				d_verticalDoor = 0;
				AddDoor(true);
				break;

			// DOOR2's keep track of the Door's vertical alignment
			case SNO_DOOR2_CHUNK:		
				res=iload->Read(&d_hitID,        sizeof(UINT32), &nb);
				res=iload->Read(&d_hitLoop,      sizeof(int), &nb);
				res=iload->Read(&d_firstEdge,    sizeof(int), &nb);
				res=iload->Read(&d_lastEdge,     sizeof(int), &nb);
				res=iload->Read(&d_flippedDoor,  sizeof(int), &nb);
				res=iload->Read(&d_verticalDoor, sizeof(int), &nb);
				AddDoor(true);
				break;

			// DOOR2's keep track of the Door's vertical alignment
			case SNO_IDOOR_CHUNK:
				
				res=iload->Read(&d_hitID,		 sizeof(UINT32), &nb);
				res=iload->Read(&d_flippedDoor,  sizeof(int), &nb);
				res=iload->Read(&d_verticalDoor, sizeof(int), &nb);

				int numverts;
				res=iload->Read(&numverts,		 sizeof(int), &nb);

				{for (int v = 0; v < numverts; v++) {
					int vindex;
					res=iload->Read(&vindex,	 sizeof(int), &nb);
					d_selectedIDoorVertList.push_back(vindex);
				}}

				AddIDoor(true);

				ClearIDoorSelection();

				break;

			case SNO_FLOOR_CHUNK:
				res=iload->Read(&d_floorFaceCount, sizeof(int), &nb);
				floor_faces.clear();
				{for (int i = 0; i < d_floorFaceCount; i++) {
					unsigned short inval;
					res=iload->Read(&inval, sizeof(inval), &nb);
					floor_faces.push_back(inval);
				}}
				break;

			case SNO_WATER_CHUNK:
				res=iload->Read(&d_waterFaceCount, sizeof(int), &nb);
				water_faces.clear();
				{for (int i = 0; i < d_waterFaceCount; i++) {
					unsigned short inval;
					res=iload->Read(&inval, sizeof(inval), &nb);
					water_faces.push_back(inval);
				}}
				break;

			case SNO_IGNORED_CHUNK:
				res=iload->Read(&d_ignoredCount, sizeof(int), &nb);
				ignored_faces.clear();
				{for (int i = 0; i < d_ignoredCount; i++) {
					unsigned short inval;
					res=iload->Read(&inval, sizeof(inval), &nb);
					ignored_faces.push_back(inval);
				}}
				break;

			case SNO_LOCKED_NORM_CHUNK:
				res=iload->Read(&d_lockedNormCount, sizeof(int), &nb);
				locked_norms.clear();
				{for (int i = 0; i < d_lockedNormCount; i++) {
					unsigned short inval;
					res=iload->Read(&inval, sizeof(inval), &nb);
					locked_norms.push_back(inval);
				}}
				break;

			case SNO_SPOOFED_NORM_CHUNK:
				res=iload->Read(&d_spoofedNormCount, sizeof(int), &nb);
				spoofed_norms.clear();
				{for (int i = 0; i < d_spoofedNormCount; i++) {
					unsigned short inval;
					res=iload->Read(&inval, sizeof(inval), &nb);
					spoofed_norms.push_back(inval);
				}}
				break;
			}


		iload->CloseChunk();
		if (res!=IO_OK) {
			return res;
		}
	}

	assert(supposed_door_count == d_doorLoopCount);

	return IO_OK;
}

//*****************************************
void BoundaryMesh::PostLoadFixup(INode *src_node, const IndexList& floor_faces, const IndexList& water_faces, const IndexList& ignored_faces, const IndexList& locked_verts, const IndexList& spoofed_verts) {
		
	FindLoops(src_node,true);

	BuildAllDoors(true);
	BuildAllIDoors(true);

	{for (int i=0; i < FNum(); ++i) {

		MNFace* f = F(i);
		
		f->ClearFlag(SELECTED_FLAG);
		f->ClearFlag(FLOOR_FACE_FLAG);
		f->ClearFlag(WATER_FACE_FLAG);
		f->ClearFlag(IGNORED_SEL_FLAG);
		f->ClearFlag(IGNORED_FLAG);

	}}

	{for (int i=0; i < VNum(); ++i) {

		MNVert* v = V(i);

		v->ClearFlag(NORM_SEL_FLAG);
		v->ClearFlag(LOCKED_NORM_FLAG);
		v->ClearFlag(SPOOFED_FLAG);
		v->ClearFlag(SPOOFED_SEL_FLAG);
	}}
	
	IndexListConstIter i;

	{for (i = floor_faces.begin(); i != floor_faces.end(); ++i) {
		MNFace* f = F(*i);
		f->SetFlag(FLOOR_FACE_FLAG);
	}}

	{for (i = water_faces.begin(); i != water_faces.end(); ++i) {
		MNFace* f = F(*i);
		f->SetFlag(WATER_FACE_FLAG);
	}}

	{for (i = ignored_faces.begin(); i != ignored_faces.end(); ++i) {
		MNFace* f = F(*i);
		f->SetFlag(IGNORED_FLAG);
	}}

	{for (i = locked_verts.begin(); i != locked_verts.end(); ++i) {
		MNVert* v = V(*i);
		v->SetFlag(LOCKED_NORM_FLAG);
	}}

	{for (i = spoofed_verts.begin(); i != spoofed_verts.end(); ++i) {
		MNVert* v = V(*i);
		v->SetFlag(SPOOFED_FLAG);
	}}

	ClearIDoorSelection();

}


// Get width of viewport in world units:  --DS

#define ZFACT (float).005;

void BoundaryMesh::DrawDoorArrow(ViewExp * vpt,Matrix3& tmn) {

	float zoom = vpt->GetScreenScaleFactor(tmn.GetTrans())*ZFACT;

	float length=10.0;
	
	tmn.Scale( Point3(zoom,zoom,zoom) );

	vpt->getGW()->setTransform( tmn );	

	vpt->getGW()->setColor( TEXT_COLOR, Point3(0.0,0.8,1.0) );
	vpt->getGW()->setColor( LINE_COLOR, Point3(0.0,0.8,1.0) );
	
	Point3 axis(0.0f,0.0f,length);	

//	vpt->getGW()->text( &axis, _T("out" ));	

	Point3 v1, v2, v3, v[3];	
	v1 = axis * (float)0.7;
	v2 = Point3(  0.0f, axis.z, 0.0f ) * (float)0.175;
	v3 = Point3(  axis.z, 0.0f, 0.0f ) * (float)0.175;
	
	v[0] = Point3(0.0,0.0,0.0);
	v[1] = axis;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );
	
	v[0] = axis;
	v[1] = v1-v2;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );

	v[0] = axis;
	v[1] = v1-v3;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );

	v[0] = axis;
	v[1] = v1+v3;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );

	vpt->getGW()->setColor( LINE_COLOR, Point3(1.0,1.0,0.0) );
				
	v[0] = axis;
	v[1] = v1+v2;
	vpt->getGW()->polyline( 2, v, NULL, NULL, FALSE, NULL );

	vpt->getGW()->setTransform(Matrix3(1) );	

}
