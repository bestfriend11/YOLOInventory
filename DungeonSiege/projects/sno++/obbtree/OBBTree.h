#pragma once
#ifndef OBBTREE_H
#define OBBTREE_H


// A library to support Oriented Bounding Boxes

class OBBNode;
class CollisionPair;




#include <fstream>
#include <list>
#include "..\math\oriented_bounding_box_3.h"
#include "..\smart_pointer.h"

namespace siege {
	class read_stream;
}

class OBBNode {

public:

	//OBBNode(void);
	explicit OBBNode(OBBNode *leftKid=0,OBBNode *rightKid=0);
	OBBNode(const oriented_bounding_box_3& obb);
	~OBBNode(void);

	bool Write( std::ofstream& ofs,char flags) const;
	bool Read( std::ifstream& ifs,char flags);
//	bool Read( siege::read_stream& ifs,char flags);

//	void Render(unsigned int) const;

	oriented_bounding_box_3 MergeBoundingBoxes(
		oriented_bounding_box_3& leftOBB,
		oriented_bounding_box_3& rightOBB);
	
	int CollideBoundingBox(const matrix_3x3& Reference_Rotation,
		const vector_3& Reference_Translation,
		OBBNode* TargetOBB) const;

	const oriented_bounding_box_3& OBB(void) const {return d_obb;}

	// TODO: What is the 'correct' name for a non-const
	oriented_bounding_box_3& mutableOBB(void) {return d_obb;}

	bool ValidLeftKid(void) const {return d_leftKid != NULL;}
	bool ValidRightKid(void) const {return d_rightKid != NULL;}

	const OBBNode& LeftKid(void) const {return *d_leftKid;}
	const OBBNode& RightKid(void) const {return *d_rightKid;}

	void SetLeftKid(OBBNode* leftKid) {d_leftKid = leftKid;}
	void SetRightKid(OBBNode* rightKid) {d_rightKid = rightKid;}
	void SetOBB(const oriented_bounding_box_3& obb) {d_obb = obb;}

	// Use for debugging
	mutable char DrawFlags;

private:

	oriented_bounding_box_3 d_obb;

	OBBNode* d_leftKid;
	OBBNode* d_rightKid;

};


class CollisionPair {

public:
	CollisionPair( OBBNode * paramA, OBBNode* paramB) { Hitter = paramA; Hittee = paramB; }

private:
	OBBNode* Hitter;
	OBBNode* Hittee;

};

typedef std::list<smart_pointer<CollisionPair> > CollisionList;
typedef CollisionList CollisionList_iter;

void SetOBBDrawBoxes( bool flag );
bool GetOBBDrawBoxes( void );

bool CheckForContact(const OBBNode& A_Node,const matrix_3x3& A_Orient, const vector_3& A_Trans,
					const OBBNode& B_Node,const matrix_3x3& B_Orient, const vector_3& B_Trans);

int obb_disjoint( const matrix_3x3 & paramB, const vector_3& paramT, const vector_3& parama, const vector_3& paramb);

#endif	// OBBTREE_H