
// A library to support Oriented Bounding Boxes
#include "..\math\gpcore.h"

#include <fstream>

#include "OBBTree.h"
#include "..\math\space_3.h"
#include "..\math\oriented_bounding_box_3.h"

using namespace std;

bool drawboxes = false;

// Existence **********************************************

OBBNode::
OBBNode(OBBNode *leftKid,OBBNode *rightKid)
{

	if (leftKid && rightKid) {

		d_obb = MergeBoundingBoxes(leftKid->mutableOBB(),rightKid->mutableOBB());

	} else if (leftKid) {

		d_obb = leftKid->d_obb;

	} else if (rightKid) {

		d_obb = rightKid->d_obb;

	} else {

		// The OBB is uninitialized

	}

	d_leftKid = leftKid;
	d_rightKid = rightKid;
}

OBBNode::
OBBNode(const oriented_bounding_box_3& obb)
	: d_obb(obb)
	, d_leftKid(0)
	, d_rightKid(0)
{
}

OBBNode::
~OBBNode(void)
{
	delete d_leftKid;
	delete d_rightKid;
}

unsigned long counter = 0;

// Input/Output *******************************************
	
bool
OBBNode::
Write(ofstream& ofs,char flags) const {

	unsigned long marker = 0xefbeadde;
	ofs.write((char*)&marker,4);
	ofs.write((char*)&counter,4);
	counter++;
	
	vector_3	center = d_obb.GetCenter();
	vector_3	diag = d_obb.GetHalfDiagonal();
	matrix_3x3	orient = d_obb.GetOrientation();

	if (!(flags & 0x04)) {
		orient =  XRotationColumns(-RealPi/2)*orient;
		center = XRotationColumns(-RealPi/2)*center;
	}

	diag = diag * 0.001f;
	center = center * 0.001f;

	if (d_leftKid)  { flags |= 0x01; };
	if (d_rightKid) { flags |= 0x02; };

	ofs << flags;

	ofs.write((char*)center.GetElementsPointer(),3*sizeof(float));
	ofs.write((char*)diag.GetElementsPointer(),3*sizeof(float));
	ofs.write((char*)orient.GetElementsPointer(),9*sizeof(float));

	bool NoError = ofs.fail() == 0;

	if (d_leftKid && NoError) {
		NoError = d_leftKid->Write(ofs,0x14);
	} 

	if (d_rightKid && NoError) {
		NoError = d_rightKid->Write(ofs,0x24);
	}

	return (NoError);
}




bool
OBBNode::
Read(ifstream& ifs,char parentflags) {

// Returns TRUE if an error occurs while reading the node 

	unsigned long  marker;
	ifs.read((char*)&marker,4);
	//gpassert (marker == 0xefbeadde);
	ifs.read((char*)&counter,4);

	char flags;

//	ifs >> flags;
	ifs.read(&flags,1);

	bool Error;

	if ((0!=ifs.fail())) {
		return true;
	}

	// Check to ensure that this is the correct child of the parent
	if ((parentflags & flags) != parentflags) {
		return true;
	}

	vector_3	center;
	vector_3	diag;
	matrix_3x3	orient;

	ifs.read((char*)center.GetElementsPointer(),3*sizeof(float));
	if (!(Error = (0!=ifs.fail()))) {
		ifs.read((char*)diag.GetElementsPointer(),3*sizeof(float));
		if (!(Error = (0!=ifs.fail()))) {
			ifs.read((char*)orient.GetElementsPointer(),9*sizeof(float));
		}	
	}

	d_obb.SetCenter(center);
	d_obb.SetHalfDiagonal(diag);
	d_obb.SetOrientation(orient);
	
	if ( (flags & 0x01) && !Error ) {
		d_leftKid = new OBBNode;
		Error = d_leftKid->Read(ifs,0x10);
	} 

	if ( (flags & 0x02) && !Error ) {
		d_rightKid = new OBBNode;
		Error = d_rightKid->Read(ifs,0x20);
	}

	return (Error);

}




/*
bool
OBBNode::
Read(siege::read_stream& ifs,char parentflags) {

// Returns TRUE if an error occurs while reading the node 

	try {

		bool Error(false);

		unsigned long  marker;
		ifs.ReadUInt8s((unsigned char*)&marker,4);
		gpassert (marker == 0xefbeadde);
		ifs.ReadUInt8s((unsigned char*)&counter,4);

		char flags;

		ifs.ReadUInt8s((unsigned char*)&flags,1);


		// Check to ensure that this is the correct child of the parent
		if ((parentflags & flags) != parentflags) {
			return true;
		}

		vector_3	center;
		vector_3	diag;
		matrix_3x3	orient;

		ifs.ReadUInt8s((unsigned char*)center.GetElementsPointer(),3*sizeof(float));
		ifs.ReadUInt8s((unsigned char*)diag.GetElementsPointer(),3*sizeof(float));
		ifs.ReadUInt8s((unsigned char*)orient.GetElementsPointer(),9*sizeof(float));

		d_obb.SetCenter(center);
		d_obb.SetHalfDiagonal(diag);
		d_obb.SetOrientation(orient);
		
		if ( (flags & 0x01) && ! Error) {
			d_leftKid = new OBBNode;
			Error = d_leftKid->Read(ifs,0x10);
		} 

		if ( (flags & 0x02) && ! Error) {
			d_rightKid = new OBBNode;
			Error = d_rightKid->Read(ifs,0x20);
		}

		return Error;
	}

	catch (...) {
		return true;
	}

}
*/


// ************************************************************************
inline void ProjectVector(const vector_3& source_vec,
					 const vector_3& onto_vec,
					 float &max_seen) {

	float test_val =  fabs(InnerProduct(source_vec,onto_vec));		
	if (test_val > max_seen) {										
		max_seen = test_val;								
	}
}


// ************************************************************************
// Function: MergeOrientedBoundingBoxes
//
// Description: Returns a new oriented bounding box that encloses the two
//				OBBs passed in.
//
//              Modifies the argument OBBs so that they are oriented 
//				along the line passing through their centers
// ************************************************************************

oriented_bounding_box_3
OBBNode::
MergeBoundingBoxes(oriented_bounding_box_3& leftOBB,
				   oriented_bounding_box_3& rightOBB) {


	vector_3 parent_diff = rightOBB.GetCenter() - leftOBB.GetCenter();

	vector_3 x_basis;
	
	float max_len;

	x_basis = parent_diff;

	/* TODO:biddle Improvements can be made on the choice of an X-axis

	/// could try to re-use an existing axis....

	max_len = Length(x_basis);

	{for (int i = 0; i< 3; i++) {

		if (max_len < leftOBB.GetHalfDiagonal()(i)) {
			max_len = leftOBB.GetHalfDiagonal()(i);
			x_basis = leftOBB.GetOrientation().GetColumn(i);
		}
		if (max_len < rightOBB.GetHalfDiagonal()(i)) {
			max_len = rightOBB.GetHalfDiagonal()(i);
			x_basis = rightOBB.GetOrientation().GetColumn(i);
		}

	}}

	*/

	x_basis = Normalize(x_basis);

	// Set parent Y axis parallel to longest component axis
	vector_3 y_basis;

	max_len = FLOAT_MIN;

	{
		float test_len;

		test_len = Length(CrossProduct(leftOBB.GetOrientation().GetColumn_0() * (leftOBB.GetHalfDiagonal().GetX()),x_basis));
		if (max_len < test_len) {
			y_basis = leftOBB.GetOrientation().GetColumn_0();
			max_len = test_len;
		}

		test_len = Length(CrossProduct(rightOBB.GetOrientation().GetColumn_0() * (rightOBB.GetHalfDiagonal().GetX()),x_basis));
		if (max_len < test_len) {
			y_basis = rightOBB.GetOrientation().GetColumn_0();
			max_len = test_len;
		}
	}
	{
		float test_len;

		test_len = Length(CrossProduct(leftOBB.GetOrientation().GetColumn_1() * (leftOBB.GetHalfDiagonal().GetY()),x_basis));
		if (max_len < test_len) {
			y_basis = leftOBB.GetOrientation().GetColumn_1();
			max_len = test_len;
		}

		test_len = Length(CrossProduct(rightOBB.GetOrientation().GetColumn_1() * (rightOBB.GetHalfDiagonal().GetY()),x_basis));
		if (max_len < test_len) {
			y_basis = rightOBB.GetOrientation().GetColumn_1();
			max_len = test_len;
		}
	}
	{
		float test_len;

		test_len = Length(CrossProduct(leftOBB.GetOrientation().GetColumn_2() * (leftOBB.GetHalfDiagonal().GetZ()),x_basis));
		if (max_len < test_len) {
			y_basis = leftOBB.GetOrientation().GetColumn_2();
			max_len = test_len;
		}

		test_len = Length(CrossProduct(rightOBB.GetOrientation().GetColumn_2() * (rightOBB.GetHalfDiagonal().GetZ()),x_basis));
		if (max_len < test_len) {
			y_basis = rightOBB.GetOrientation().GetColumn_2();
			max_len = test_len;
		}
	}

	vector_3 z_basis;

	z_basis = Normalize(CrossProduct(x_basis,y_basis));
	y_basis = Normalize(CrossProduct(z_basis,x_basis));

	matrix_3x3 parent_orientation = MatrixColumns(x_basis,y_basis,z_basis);

//	gpassert(IsOrthonormal(parent_orientation));

	// Locate the new extrema

	float max_lx = 0;
	float max_rx = 0;
	float max_y  = 0;
	float max_z  = 0;

	vector_3 v0 = leftOBB.GetOrientation().GetColumn_0() * leftOBB.GetHalfDiagonal().GetX();
	vector_3 v1 = leftOBB.GetOrientation().GetColumn_1() * leftOBB.GetHalfDiagonal().GetY();
	vector_3 v2 = leftOBB.GetOrientation().GetColumn_2() * leftOBB.GetHalfDiagonal().GetZ();

	ProjectVector((v0+v1+v2), x_basis, max_lx);
	ProjectVector((v0+v1-v2), x_basis, max_lx);
	ProjectVector((v0-v1+v2), x_basis, max_lx);
	ProjectVector((v0-v1-v2), x_basis, max_lx);

	ProjectVector((v0+v1+v2), y_basis, max_y);
	ProjectVector((v0+v1-v2), y_basis, max_y);
	ProjectVector((v0-v1+v2), y_basis, max_y);
	ProjectVector((v0-v1-v2), y_basis, max_y);

	ProjectVector((v0+v1+v2), z_basis, max_z);
	ProjectVector((v0+v1-v2), z_basis, max_z);
	ProjectVector((v0-v1+v2), z_basis, max_z);
	ProjectVector((v0-v1-v2), z_basis, max_z); 

	v0 = rightOBB.GetOrientation().GetColumn_0() * rightOBB.GetHalfDiagonal().GetX();
	v1 = rightOBB.GetOrientation().GetColumn_1() * rightOBB.GetHalfDiagonal().GetY();
	v2 = rightOBB.GetOrientation().GetColumn_2() * rightOBB.GetHalfDiagonal().GetZ();

	ProjectVector((v0+v1+v2), x_basis, max_rx);
	ProjectVector((v0+v1-v2), x_basis, max_rx);
	ProjectVector((v0-v1+v2), x_basis, max_rx);
	ProjectVector((v0-v1-v2), x_basis, max_rx);

	ProjectVector((v0+v1+v2), y_basis, max_y);
	ProjectVector((v0+v1-v2), y_basis, max_y);
	ProjectVector((v0-v1+v2), y_basis, max_y);
	ProjectVector((v0-v1-v2), y_basis, max_y);

	ProjectVector((v0+v1+v2), z_basis, max_z);
	ProjectVector((v0+v1-v2), z_basis, max_z);
	ProjectVector((v0-v1+v2), z_basis, max_z);
	ProjectVector((v0-v1-v2), z_basis, max_z); 


	float parent_diff_length = Length(parent_diff);

	if (max_lx > parent_diff_length + max_rx) {
		max_rx = max_lx - parent_diff_length;
	}

	if (max_rx > parent_diff_length + max_lx) {
		max_lx = max_rx - parent_diff_length;
	}

	vector_3 parent_half_diag((parent_diff_length + max_lx + max_rx)/2,max_y,max_z);

	vector_3 parent_center = (leftOBB.GetCenter() + rightOBB.GetCenter()) * 0.5;
	parent_center += Normalize(parent_diff)*((max_rx-max_lx)/2);

	// Transform the children back into the parent's space..
	leftOBB.SetOrientation(Transpose(parent_orientation)*leftOBB.GetOrientation());
	leftOBB.SetCenter(vector_3( -parent_half_diag.GetX()+max_lx ,0,0));

	rightOBB.SetOrientation(Transpose(parent_orientation)*rightOBB.GetOrientation());
	rightOBB.SetCenter(vector_3( parent_half_diag.GetX()-max_rx,0,0));


	return (oriented_bounding_box_3(parent_center,parent_half_diag,parent_orientation));
}
/*

// Test to see if an OBB collides with a target OBB
int
OBBNode::
CollideBoundingBox(const matrix_3x3& Reference_Rotation,
				   const vector_3& Reference_Translation,
				   OBBNode* TargetOBB) const {

	matrix_3x3 Relative_Rotation =
			Transpose(OBB.GetOrientation())
			* Reference_Rotation
			* TargetOBB->OBB.GetOrientation();

	vector_3 Relative_Translation =
			Transpose(OBB.GetOrientation()) *
			(- OBB.GetCenter()
			 + Reference_Translation
			 + (TargetOBB->OBB.GetOrientation()*TargetOBB->OBB.GetCenter()));

	int ret = obb_disjoint(
			Relative_Rotation,
			Relative_Translation,
			OBB.GetHalfDiagonal(),
			TargetOBB->OBB.GetHalfDiagonal()
		);

	if ((ret != 0) || ( // LeftKid == NULL && RightKid == NULL &&
		  TargetOBB->LeftKid == NULL && TargetOBB->RightKid == NULL)) {

		if (ret == 0) {		
			DrawFlags |= 0x01;
			TargetOBB->DrawFlags |= 0x01;
		}

		return ret;

	}

	ret = 0;

//	if (LeftKid == NULL && RightKid == NULL) {

		if (TargetOBB->LeftKid != NULL) {

			ret |= CollideBoundingBox(
				Reference_Rotation*TargetOBB->OBB.GetOrientation(), 
				(TargetOBB->OBB.GetOrientation()*TargetOBB->OBB.GetCenter()) + Reference_Translation,
				TargetOBB->LeftKid);

		} 

		if (TargetOBB->RightKid != NULL) {

			ret |= CollideBoundingBox(
				Reference_Rotation*TargetOBB->OBB.GetOrientation(), 
				(TargetOBB->OBB.GetOrientation()*TargetOBB->OBB.GetCenter()) + Reference_Translation,
				TargetOBB->RightKid);

		}

#if 0
	}  else {

		if (LeftKid != NULL) {

			ret |= LeftKid->CollideBoundingBox(
				Transpose(OBB.GetOrientation())*Reference_Rotation, 
				-(OBB.GetOrientation()*OBB.GetCenter()) + Reference_Translation,
				TargetOBB);

		}

		if (RightKid != NULL) {

			ret |= RightKid->CollideBoundingBox(
				Transpose(OBB.GetOrientation())*Reference_Rotation, 
				-(OBB.GetOrientation()*OBB.GetCenter()) + Reference_Translation,
				TargetOBB);

		}
	}
#endif

	return ret;
}

*/

/******************
/*

int
obb_disjoint(double B[3][3], double T[3], double a[3], double b[3]);

This is a test between two boxes, box A and box B.  It is assumed that
the coordinate system is aligned and centered on box A.  The 3x3
matrix B specifies box B's orientation with respect to box A.
Specifically, the columns of B are the basis vectors (axis vectors) of
box B.  The center of box B is located at the vector T.  The
dimensions of box B are given in the array b.  The orientation and
placement of box A, in this coordinate system, are the identity matrix
and zero vector, respectively, so they need not be specified.  The
dimensions of box A are given in array a.

This test operates in two modes, depending on how the library is
compiled.  It indicates whether the two boxes are overlapping, by
returning a boolean.  

The second version of the routine will return a conservative bounds on
the distance between the polygon sets which the boxes enclose.  It is
used when RAPID is being used to estimate the distance between two
models.

*/


int obb_disjoint(const matrix_3x3& B, const vector_3& T, const vector_3& a, const vector_3& b) {
  register float t, s;
  register int r;
  matrix_3x3 Bf;
  const float reps = 1e-6f;
  
  // Bf = fabs(B)
  Bf.SetElement_00( fabs(B.GetElement_00()) + reps );
  Bf.SetElement_01( fabs(B.GetElement_01()) + reps );
  Bf.SetElement_02( fabs(B.GetElement_02()) + reps );
  Bf.SetElement_10( fabs(B.GetElement_10()) + reps );
  Bf.SetElement_11( fabs(B.GetElement_11()) + reps );
  Bf.SetElement_12( fabs(B.GetElement_12()) + reps );
  Bf.SetElement_20( fabs(B.GetElement_20()) + reps );
  Bf.SetElement_21( fabs(B.GetElement_21()) + reps );
  Bf.SetElement_22( fabs(B.GetElement_22()) + reps );

  // if any of these tests are one-sided, then the polyhedra are disjoint
  r = 1;

  // A1 x A2 = A0
  t = fabs(T.GetX());
  
  r &= (t <= 
	  (a.GetX() + b.GetX() * Bf.GetElement_00() + b.GetY() * Bf.GetElement_01() + b.GetZ() * Bf.GetElement_02()));
  if (!r) return 1;
  
  // B1 x B2 = B0
  s = T.GetX()*B.GetElement_00() + T.GetY()*B.GetElement_10() + T.GetZ()*B.GetElement_20();
  t = fabs(s);

  r &= ( t <=
	  (b.GetX() + a.GetX() * Bf.GetElement_00() + a.GetY() * Bf.GetElement_10() + a.GetZ() * Bf.GetElement_20()));
  if (!r) return 2;
    
  // A2 x A0 = A1
  t = fabs(T.GetY());
  
  r &= ( t <= 
	  (a.GetY() + b.GetX() * Bf.GetElement_10() + b.GetY() * Bf.GetElement_11() + b.GetZ() * Bf.GetElement_12()));
  if (!r) return 3;

  // A0 x A1 = A2
  t = fabs(T.GetZ());

  r &= ( t <= 
	  (a.GetZ() + b.GetX() * Bf.GetElement_20() + b.GetY() * Bf.GetElement_21() + b.GetZ() * Bf.GetElement_22()));
  if (!r) return 4;

  // B2 x B0 = B1
  s = T.GetX()*B.GetElement_01() + T.GetY()*B.GetElement_11() + T.GetZ()*B.GetElement_21();
  t = fabs(s);

  r &= ( t <=
	  (b.GetY() + a.GetX() * Bf.GetElement_01() + a.GetY() * Bf.GetElement_11() + a.GetZ() * Bf.GetElement_21()));
  if (!r) return 5;

  // B0 x B1 = B2
  s = T.GetX()*B.GetElement_02() + T.GetY()*B.GetElement_12() + T.GetZ()*B.GetElement_22();
  t = fabs(s);

  r &= ( t <=
	  (b.GetZ() + a.GetX() * Bf.GetElement_02() + a.GetY() * Bf.GetElement_12() + a.GetZ() * Bf.GetElement_22()));
  if (!r) return 6;

  // A0 x B0
  s = T.GetZ() * B.GetElement_10() - T.GetY() * B.GetElement_20();
  t = fabs(s);
  
  r &= ( t <= 
	(a.GetY() * Bf.GetElement_20() + a.GetZ() * Bf.GetElement_10() +
	 b.GetY() * Bf.GetElement_02() + b.GetZ() * Bf.GetElement_01()));
  if (!r) return 7;
  
  // A0 x B1
  s = T.GetZ() * B.GetElement_11() - T.GetY() * B.GetElement_21();
  t = fabs(s);

  r &= ( t <=
	(a.GetY() * Bf.GetElement_21() + a.GetZ() * Bf.GetElement_11() +
	 b.GetX() * Bf.GetElement_02() + b.GetZ() * Bf.GetElement_00()));
  if (!r) return 8;

  // A0 x B2
  s = T.GetZ() * B.GetElement_12() - T.GetY() * B.GetElement_22();
  t = fabs(s);

  r &= ( t <=
	  (a.GetY() * Bf.GetElement_22() + a.GetZ() * Bf.GetElement_12() +
	   b.GetX() * Bf.GetElement_01() + b.GetY() * Bf.GetElement_00()));
  if (!r) return 9;

  // A1 x B0
  s = T.GetX() * B.GetElement_20() - T.GetZ() * B.GetElement_00();
  t = fabs(s);

  r &= ( t <=
	  (a.GetX() * Bf.GetElement_20() + a.GetZ() * Bf.GetElement_00() +
	   b.GetY() * Bf.GetElement_12() + b.GetZ() * Bf.GetElement_11()));
  if (!r) return 10;

  // A1 x B1
  s = T.GetX() * B.GetElement_21() - T.GetZ() * B.GetElement_01();
  t = fabs(s);

  r &= ( t <=
	  (a.GetX() * Bf.GetElement_21() + a.GetZ() * Bf.GetElement_01() +
	   b.GetX() * Bf.GetElement_12() + b.GetZ() * Bf.GetElement_10()));
  if (!r) return 11;

  // A1 x B2
  s = T.GetX() * B.GetElement_22() - T.GetZ() * B.GetElement_02();
  t = fabs(s);

  r &= (t <=
	  (a.GetX() * Bf.GetElement_22() + a.GetZ() * Bf.GetElement_02() +
	   b.GetX() * Bf.GetElement_11() + b.GetY() * Bf.GetElement_10()));
  if (!r) return 12;

  // A2 x B0
  s = T.GetY() * B.GetElement_00() - T.GetX() * B.GetElement_10();
  t = fabs(s);

  r &= (t <=
	  (a.GetX() * Bf.GetElement_10() + a.GetY() * Bf.GetElement_00() +
	   b.GetY() * Bf.GetElement_22() + b.GetZ() * Bf.GetElement_21()));
  if (!r) return 13;

  // A2 x B1
  s = T.GetY() * B.GetElement_01() - T.GetX() * B.GetElement_11();
  t = fabs(s);

  r &= ( t <=
	  (a.GetX() * Bf.GetElement_11() + a.GetY() * Bf.GetElement_01() +
	   b.GetX() * Bf.GetElement_22() + b.GetZ() * Bf.GetElement_20()));
  if (!r) return 14;

  // A2 x B2
  s = T.GetY() * B.GetElement_02() - T.GetX() * B.GetElement_12();
  t = fabs(s);

  r &= ( t <=
	  (a.GetX() * Bf.GetElement_12() + a.GetY() * Bf.GetElement_02() +
	   b.GetX() * Bf.GetElement_21() + b.GetY() * Bf.GetElement_20()));
  if (!r) return 15;

  return 0;  // should equal 0
}

#ifndef MAX_EXPORTER
// Need to get a hold of this Tatto project file in order to draw debug lines
#include "terrain.h"
#endif

bool CheckForContact(const OBBNode& A_Node,const matrix_3x3& A_Orient, const vector_3& A_Trans,
					const OBBNode& B_Node,const matrix_3x3& B_Orient, const vector_3& B_Trans) {


#ifndef MAX_EXPORTER

	if( drawboxes )
	{
		matrix_3x3 LocalOrientation =  A_Orient * A_Node.OBB().GetOrientation();
		vector_3 LocalOffset =  A_Trans+(A_Orient * A_Node.OBB().GetCenter());

		vector_3 diag = A_Node.OBB().GetHalfDiagonal();

		vector_3 x_basis = LocalOrientation.GetColumn_0() * diag.GetX();
		vector_3 y_basis = LocalOrientation.GetColumn_1() * diag.GetY();
		vector_3 z_basis = LocalOrientation.GetColumn_2() * diag.GetZ();

		vector_3 A = LocalOffset + x_basis + y_basis + z_basis;
		vector_3 B = LocalOffset + x_basis + y_basis - z_basis;
		vector_3 C = LocalOffset + x_basis - y_basis - z_basis;
		vector_3 D = LocalOffset + x_basis - y_basis + z_basis;

		vector_3 E = LocalOffset - x_basis + y_basis + z_basis;
		vector_3 F = LocalOffset - x_basis + y_basis - z_basis;
		vector_3 G = LocalOffset - x_basis - y_basis - z_basis;
		vector_3 H = LocalOffset - x_basis - y_basis + z_basis;


		DrawTheDamnLine(A,B,vector_3(1,0,0));
		DrawTheDamnLine(B,C,vector_3(1,0,0));
		DrawTheDamnLine(C,D,vector_3(1,0,0));
		DrawTheDamnLine(D,A,vector_3(1,0,0));

		DrawTheDamnLine(E,F,vector_3(1,1,1));
		DrawTheDamnLine(F,G,vector_3(1,1,1));
		DrawTheDamnLine(G,H,vector_3(1,1,1));
		DrawTheDamnLine(H,E,vector_3(1,1,1));

		DrawTheDamnLine(A,E,vector_3(1,1,1));
		DrawTheDamnLine(B,F,vector_3(1,1,1));
		DrawTheDamnLine(C,G,vector_3(1,1,1));
		DrawTheDamnLine(D,H,vector_3(1,1,1));

	}

	if( drawboxes )
	{
		matrix_3x3 LocalOrientation =  B_Orient * B_Node.OBB().GetOrientation();
		vector_3 LocalOffset =  B_Trans+(B_Orient * B_Node.OBB().GetCenter());

		vector_3 diag = B_Node.OBB().GetHalfDiagonal();

		vector_3 x_basis = LocalOrientation.GetColumn_0() * diag.GetX();
		vector_3 y_basis = LocalOrientation.GetColumn_1() * diag.GetY();
		vector_3 z_basis = LocalOrientation.GetColumn_2() * diag.GetZ();

		vector_3 A = LocalOffset + x_basis + y_basis + z_basis;
		vector_3 B = LocalOffset + x_basis + y_basis - z_basis;
		vector_3 C = LocalOffset + x_basis - y_basis - z_basis;
		vector_3 D = LocalOffset + x_basis - y_basis + z_basis;

		vector_3 E = LocalOffset - x_basis + y_basis + z_basis;
		vector_3 F = LocalOffset - x_basis + y_basis - z_basis;
		vector_3 G = LocalOffset - x_basis - y_basis - z_basis;
		vector_3 H = LocalOffset - x_basis - y_basis + z_basis;


		DrawTheDamnLine(A,B,vector_3(0,0,1));
		DrawTheDamnLine(B,C,vector_3(0,0,1));
		DrawTheDamnLine(C,D,vector_3(0,0,1));
		DrawTheDamnLine(D,A,vector_3(0,0,1));

		DrawTheDamnLine(E,F,vector_3(1,1,1));
		DrawTheDamnLine(F,G,vector_3(1,1,1));
		DrawTheDamnLine(G,H,vector_3(1,1,1));
		DrawTheDamnLine(H,E,vector_3(1,1,1));

		DrawTheDamnLine(A,E,vector_3(1,1,1));
		DrawTheDamnLine(B,F,vector_3(1,1,1));
		DrawTheDamnLine(C,G,vector_3(1,1,1));
		DrawTheDamnLine(D,H,vector_3(1,1,1));

	}

#endif

	matrix_3x3 Relative_Rotation =
		  Transpose(A_Node.OBB().GetOrientation()) 
		* Transpose(A_Orient) 
		* (B_Orient)
		* (B_Node.OBB().GetOrientation())
		;

	vector_3 Relative_Translation =
		 Transpose( A_Orient * A_Node.OBB().GetOrientation() )
		 * (- (A_Trans + (A_Orient * A_Node.OBB().GetCenter()))
		    + (B_Trans + (B_Orient * B_Node.OBB().GetCenter())))
		;

	bool IsHit = (0 == obb_disjoint(
			Relative_Rotation,
			Relative_Translation,
			A_Node.OBB().GetHalfDiagonal(),
			B_Node.OBB().GetHalfDiagonal()
		));

	if (!IsHit || ( !A_Node.ValidLeftKid() && !A_Node.ValidRightKid() &&
		 !B_Node.ValidLeftKid() && !B_Node.ValidRightKid())) {

		if (IsHit) {		
			A_Node.DrawFlags |= 0x01;
			B_Node.DrawFlags |= 0x01;
		} 

		return IsHit;
	}

	if (A_Node.ValidLeftKid()) {

			IsHit |= (0 == CheckForContact(
				A_Node.LeftKid(),
				(A_Orient)*(A_Node.OBB().GetOrientation()),
				A_Trans + (A_Orient * A_Node.OBB().GetCenter()),
				B_Node,
				B_Orient,
				B_Trans
				));
	}

	if (A_Node.ValidRightKid()) {

			IsHit |= (0 == CheckForContact(
				A_Node.RightKid(),
				(A_Orient)*(A_Node.OBB().GetOrientation()),
				A_Trans + (A_Orient * A_Node.OBB().GetCenter()),
				B_Node,
				B_Orient,
				B_Trans
				));
	}

	if (B_Node.ValidLeftKid()) {

			IsHit |= (0 == CheckForContact(
				A_Node,
				A_Orient,
				A_Trans,
				B_Node.LeftKid(),
				(B_Orient)*(B_Node.OBB().GetOrientation()),
				B_Trans + (B_Orient * B_Node.OBB().GetCenter())
				));
	}

	if (B_Node.ValidRightKid()) {

			IsHit |= (0 == CheckForContact(
				A_Node,
				A_Orient,
				A_Trans,
				B_Node.RightKid(),
				(B_Orient)*(B_Node.OBB().GetOrientation()),
				B_Trans + (B_Orient * B_Node.OBB().GetCenter())
				));
	}


	return (IsHit);
}

void SetOBBDrawBoxes( bool flag )
{
	drawboxes = flag;
}

bool GetOBBDrawBoxes( void )
{
	return( drawboxes );
}

