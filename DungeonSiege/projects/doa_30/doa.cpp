#include "MAXScrpt.h"

#include "Numbers.h"
#include "Arrays.h"
#include "Name.h"
#include "3DMath.h"
#include "MNMath.h"

#include "MAXObj.h"

#include "bipexp.h"
#include "PhyExp.H"

#include "stdio.h"		// including this file binds in libs and makes for a big dll
						// but I'm lazy and want to use sscanf & sprintf
						// for string conversions

#include <vector>

// selection sets

Modifier* FindPhysiqueModifier (INode* nodePtr)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = nodePtr->GetObjectRef();
	

	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
			{
				// Yes -> Exit.
				return ModifierPtr;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
	}

	// Not found.
	return NULL;
}


#include "definsfn.h"

//*********************************************************

void DOA_init(void) {

	mputs("DOA - experimental MAXScript support for CStudio loaded\n"); 

}
 
//*********************************************************
//*********************************************************
//*********************************************************

def_visible_primitive( doaHello,					"doaHello");

def_visible_primitive( doaValidMesh,				"doaValidMesh");
def_visible_primitive( doaNumVerts,					"doaNumVerts");
def_visible_primitive( doaNumBonesAttachedToVert,	"doaNumBonesAttachedToVert");
def_visible_primitive( doaNumVertsAttachedToBone,	"doaNumVertsAttachedToBone");

def_visible_primitive( doaGetWeightByBoneNode,		"doaGetWeightByBoneNode");
def_visible_primitive( doaGetWeightByBoneIndex,		"doaGetWeightByBoneIndex");
def_visible_primitive( doaGetOffsetByBoneNode,		"doaGetOffsetByBoneNode");
def_visible_primitive( doaGetOffsetByBoneIndex,		"doaGetOffsetByBoneIndex");
def_visible_primitive( doaGetTotalWeight,			"doaGetTotalWeight");
def_visible_primitive( doaGetBoneByVertexIndex,		"doaGetBoneByVertexIndex");

def_visible_primitive( doaValidBone,				"doaValidBone");
def_visible_primitive( doaValidFootstep,			"doaValidFootstep");
def_visible_primitive( doaValidBody,				"doaValidBody");

def_visible_primitive( doaValidRigidVertex,			"doaValidRigidVertex");

def_visible_primitive( doaNumTMKeys,				"doaNumTMKeys");
def_visible_primitive( doaNumHorizontalKeys,		"doaNumHorizontalKeys");
def_visible_primitive( doaNumVerticalKeys,			"doaNumVerticalKeys");
def_visible_primitive( doaNumRotationKeys,			"doaNumRotationKeys");

def_visible_primitive( doaGetTMKeyTimes,			"doaGetTMKeyTimes");
def_visible_primitive( doaGetHorizontalKeyTimes,	"doaGetHorizontalKeyTimes");
def_visible_primitive( doaGetVerticalKeyTimes,		"doaGetVerticalKeyTimes");
def_visible_primitive( doaGetRotationKeyTimes,		"doaGetRotationKeyTimes");

def_visible_primitive( doaGetNextKeyTime,			"doaGetNextKeyTime");

def_visible_primitive( doaSetKeyTMValue,			"doaSetKeyTMValue");

def_visible_primitive( doaUniformMatrix,			"doaUniformMatrix");
def_visible_primitive( doaUniformInverseMatrix,		"doaUniformInverseMatrix");
def_visible_primitive( doaRelativeMatrix,			"doaRelativeMatrix");
def_visible_primitive( doaAbsoluteMatrix,			"doaAbsoluteMatrix");

def_visible_primitive( doaNumAnimatedParents,		"doaNumAnimatedParents");
def_visible_primitive( doaGetParentAtTime,			"doaGetParentAtTime");

def_visible_primitive( doaStringToInt,				"doaStringToInt");
def_visible_primitive( doaIntToName,				"doaIntToName");
def_visible_primitive( doaFourCCToInt,				"doaFourCCToInt");

#if (MAX_RELEASE == 3000)
def_visible_primitive( doaBeginFigureMode,			"doaBeginFigureMode");
def_visible_primitive( doaEndFigureMode,			"doaEndFigureMode");
#endif

def_visible_primitive( doaGetUserName,				"doaGetUserName");
def_visible_primitive( doaGetHostName,				"doaGetHostName");
def_visible_primitive( doaGetEnvironmentVar,		"doaGetEnvironmentVar");

def_visible_primitive( doaGetTransformLock,			"doaGetTransformLock");
def_visible_primitive( doaSetTransformLock,			"doaSetTransformLock");

def_visible_primitive( doaAddPhysiqueRoot,			"doaAddPhysiqueRoot");
def_visible_primitive( doaInitializePhysique,		"doaInitializePhysique");
def_visible_primitive( doaCopyPhysiqueWeights,		"doaCopyPhysiqueWeights");

def_visible_primitive( doaCheckForBadMNMesh,		"doaCheckForBadMNMesh");

//def_visible_primitive( doaSplineFit,				"doaSplineFit");

//*********************************************************

Value* doaHello_cf(Value** arg_list, int count) {

	mputs("Hello, Kirkland!\n"); 
	mputs("doaHello\n");

	mputs("doaValidMesh\n");
	mputs("doaNumVerts\n");
	mputs("doaNumBonesAttachedToVert\n");
	mputs("doaNumVertsAttachedToBone\n");

	mputs("doaGetWeightByBoneNode\n");
	mputs("doaGetWeightByBoneIndex\n");
	mputs("doaGetOffsetByBoneNode\n");
	mputs("doaGetOffsetByBoneIndex\n");
	mputs("doaGetTotalWeight\n");
	mputs("doaGetBoneByVertexIndex\n");

	mputs("doaValidBone\n");
	mputs("doaValidFootstep\n");
	mputs("doaValidBody\n");

	mputs("doaValidRigidVertex\n");

	mputs("doaNumTMKeys\n");
	mputs("doaNumHorizontalKeys\n");
	mputs("doaNumVerticalKeys\n");
	mputs("doaNumRotationKeys\n");

	mputs("doaGetTMKeyTimes\n");
	mputs("doaGetHorizontalKeyTimes\n");
	mputs("doaGetVerticalKeyTimes\n");
	mputs("doaGetRotationKeyTimes\n");
	
	mputs("doaGetNextKeyTime\n");

	mputs("doaSetKeyTMValue\n");

	mputs("doaUniformMatrix\n");
	mputs("doaUniformInverseMatrix\n");
	mputs("doaRelativeMatrix\n");
	mputs("doaAbsoluteMatrix\n");

	mputs("doaNumAnimatedParents\n");
	mputs("doaGetParentAtTime\n");

	mputs("doaStringToInt\n");
	mputs("doaFourCCToInt\n");
	mputs("doaIntToName\n");

#if (MAX_RELEASE == 3000)
	mputs("doaBeginFigureMode\n");
	mputs("doaEndFigureMode\n");
#endif

	mputs("doaGetUserName\n");
	mputs("doaGetHostName\n");
	mputs("doaGetEnvironmentVar\n");

	mputs("doaGetTransformLock\n");
	mputs("doaSetTransformLock\n");

	mputs("doaAddPhysiqueRoot\n");
	mputs("doaInitializePhysique\n");
	mputs("doaCopyPhysiqueWeights\n");

	mputs("doaCheckForBadMNMesh\n");

	mputs("doaSplineFit\n");

	return &true_value;

}

//*********************************************************

Value* doaValidMesh_cf(Value** arg_list, int count) {

	check_arg_count(doaValidMesh, 1, count);

	INode* nodeptr = arg_list[0]->to_node();

	if (FindPhysiqueModifier(nodeptr)) {
		return &true_value;
	}
	
	return &false_value;
}

//*********************************************************

Value* doaNumVerts_cf(Value** arg_list, int count) {

	check_arg_count(doaNumVerts, 1, count);

	INode* nodeptr = arg_list[0]->to_node();

	Modifier* phy;

	if (! (phy = FindPhysiqueModifier(nodeptr)) ) {
		// Could raise an exception?
		return Integer::intern(0); 
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(nodeptr);

	int numverts = phyContextExp->GetNumberVertices();

	phyExp->ReleaseContextInterface(phyContextExp);

	return Integer::intern(numverts); 


}

//*********************************************************

Value* doaNumBonesAttachedToVert_cf(Value** arg_list, int count) {

	check_arg_count(doaNumBonesAttachedToVert, 2, count);

	INode* mesh = arg_list[0]->to_node();
	int vert = arg_list[1]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!

	Modifier* phy;

	int numbones = 0;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				numbones =  phyBlendVert->GetNumberNodes();

			} else {

				numbones =  1;

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}


	return Integer::intern(numbones); 


}
//*********************************************************

Value* doaNumVertsAttachedToBone_cf(Value** arg_list, int count) {

	check_arg_count(doaNumVertsAttachedToBone, 2, count);

	INode* mesh = arg_list[0]->to_node();
	INode* bone = arg_list[1]->to_node();

	int numverts = 0 ;

	Modifier* phy;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int vmax = phyContextExp->GetNumberVertices();

		for (int vert = 0; vert < vmax ; ++vert)
		{
			if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

				int vtype = phyVertExp->GetVertexType();

				if (vtype & BLENDED_TYPE) {

					IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

					for (int b = 0; b < phyBlendVert->GetNumberNodes(); b++) {
						if (phyBlendVert->GetNode(b) == bone) {
							++numverts;
						}
					}

				} else {

					IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;
					if (phyRigidVert->GetNode() == bone) {
						++numverts;
					}

				}

				phyContextExp->ReleaseVertexInterface(phyVertExp);

			}
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}

	return Integer::intern(numverts); 
}

//*********************************************************

Value* doaGetWeightByBoneNode_cf(Value** arg_list, int count) {

	check_arg_count(doaGetWeightByBoneNode, 3, count);

	INode* mesh = arg_list[0]->to_node();
	INode* bone = arg_list[1]->to_node();
	int vert = arg_list[2]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!

	Modifier* phy;
	float weight = 0.0;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				for (int b = 0; b < phyBlendVert->GetNumberNodes(); b++) {


					if (phyBlendVert->GetNode(b) == bone) {
						weight += phyBlendVert->GetWeight(b);
						// break;  The bone may appear more than once!!!!
					}
				}

			} else {

				IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;
				if (phyRigidVert->GetNode() == bone) {
					weight = 1.0;
				}

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}


	return Float::intern(weight); 


}

//*********************************************************
Value* doaGetWeightByBoneIndex_cf(Value** arg_list, int count) {

	check_arg_count(doaGetWeightByBoneIndex, 3, count);

	INode* mesh = arg_list[0]->to_node();
	int bonenum = arg_list[1]->to_int();
	int vert = arg_list[2]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!
	bonenum--;

	Modifier* phy;
	float weight = 0.0;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				if ((bonenum >= 0) && (bonenum < phyBlendVert->GetNumberNodes())) {
					weight = phyBlendVert->GetWeight(bonenum);
				}

			} else {

				IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;
				if (bonenum == 0) {
					weight = 1.0;
				}

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}


	return Float::intern(weight); 


}

//*********************************************************

Value* doaGetOffsetByBoneNode_cf(Value** arg_list, int count) {

	check_arg_count(doaGetWeightByBoneNode, 3, count);

	INode* mesh = arg_list[0]->to_node();
	INode* bone = arg_list[1]->to_node();
	int vert = arg_list[2]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!

	Modifier* phy;
	Point3 offset(0,0,0);

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				for (int b = 0; b < phyBlendVert->GetNumberNodes(); b++) {

					if (phyBlendVert->GetNode(b) == bone) {
						offset = phyBlendVert->GetOffsetVector(b);
						break;
					}
				}

			} else {

				IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;
				if (phyRigidVert->GetNode() == bone) {
					offset = phyRigidVert->GetOffsetVector();
				}

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}

	one_typed_value_local(Point3Value* result);
	vl.result = new Point3Value(offset);
	return vl.result;

}

//*********************************************************
Value* doaGetOffsetByBoneIndex_cf(Value** arg_list, int count) {

	check_arg_count(doaGetWeightByBoneIndex, 3, count);

	INode* mesh = arg_list[0]->to_node();
	int bonenum = arg_list[1]->to_int();
	int vert = arg_list[2]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!
	bonenum--;

	Modifier* phy;
	Point3 offset(0,0,0);

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				if ((bonenum >= 0) && (bonenum < phyBlendVert->GetNumberNodes())) {
					offset = phyBlendVert->GetOffsetVector(bonenum);
				}

			} else {

				IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;
				if (bonenum == 0) {
					offset = phyRigidVert->GetOffsetVector();
				}

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}


	one_typed_value_local(Point3Value* result);
	vl.result = new Point3Value(offset);
	return_value(vl.result);


}

//*********************************************************

Value* doaGetBoneByVertexIndex_cf(Value** arg_list, int count) {

	check_arg_count(doaGetBoneByVertexIndex, 3, count);

	INode* mesh = arg_list[0]->to_node();
	int vert = arg_list[1]->to_int();
	int bonenum = arg_list[2]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!
	bonenum--;	

	Modifier* phy;

	INode* bone = NULL;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				bone = phyBlendVert->GetNode(bonenum);

			} else if (bonenum == 0 ){

				IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;

				bone = phyRigidVert->GetNode();

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}

	if (bone) {

		one_typed_value_local(MAXNode* result);
		vl.result = new MAXNode(bone);

		return_value(vl.result);

	}

	return &false_value; 


}

//*********************************************************

Value* doaGetTotalWeight_cf(Value** arg_list, int count) {

	check_arg_count(doaGetTotalWeight, 2, count);

	INode* mesh = arg_list[0]->to_node();
	int vert = arg_list[1]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!

	Modifier* phy;
	float totalweight = 0.0;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(true);
		phyContextExp->AllowBlending(true);

		int numverts = phyContextExp->GetNumberVertices();

		if (vert < 0 || numverts <= vert) {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				for (int b = 0; b < phyBlendVert->GetNumberNodes(); b++) {
					totalweight += phyBlendVert->GetWeight(b);
				}

			} else {

				totalweight = -1.0;

			}

			phyContextExp->ReleaseVertexInterface(phyVertExp);

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyContextExp);
			return &false_value;
		}


		phyExp->ReleaseContextInterface(phyContextExp);

	}


	return Float::intern(totalweight); 


}

//*********************************************************

Value* doaValidFootstep_cf(Value** arg_list, int count) {

	check_arg_count(doaValidFootstep, 1, count);

	INode* bone = arg_list[0]->to_node();

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == FOOTPRINT_CLASS_ID) ) {

			return &true_value;

		}
	}

	return &false_value;

}

//*********************************************************

Value* doaValidBody_cf(Value** arg_list, int count) {

	check_arg_count(doaValidBody, 1, count);

	INode* bone = arg_list[0]->to_node();

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			return &true_value;

		}
	}

	return &false_value;

}
//*********************************************************

Value* doaValidBone_cf(Value** arg_list, int count) {

	check_arg_count(doaValidBone, 1, count);

	INode* bone = arg_list[0]->to_node();

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID)) {

			return &true_value;

		}
	}

	return &false_value;

}

//*********************************************************

Value* doaValidRigidVertex_cf(Value** arg_list, int count) {

	check_arg_count(doaValidRigidVertex, 2, count);

	INode* mesh = arg_list[0]->to_node();
	int vert = arg_list[1]->to_int();

	vert--;		// don't get all off-by-one coming out of MAXSCRIPT!

	Modifier* phy;
	bool IsRigid = false;

	if (! (phy = FindPhysiqueModifier(mesh)) ) {
		// Could raise an exception?
		return &false_value;
	}

	IPhysiqueExport *phyExp = (IPhysiqueExport *)phy->GetInterface(I_PHYINTERFACE);

	if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(mesh) ) {

		phyContextExp->ConvertToRigid(false);

		int numverts = phyContextExp->GetNumberVertices();
		if (vert >= 0 && numverts > vert) {

			IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(vert);

			if (phyVertExp) {

				int vtype = phyVertExp->GetVertexType();

				if (!(vtype & DEFORMABLE_TYPE)) {
					IsRigid = true;
				}

				phyContextExp->ReleaseVertexInterface(phyVertExp);
			}
		}

		phyExp->ReleaseContextInterface(phyContextExp);

	}


	if (IsRigid) {
		return &true_value;
	}

	return &false_value;
}

//*********************************************************

Value* doaNumTMKeys_cf(Value** arg_list, int count) {
	check_arg_count(doaNumKeys, 1, count);

	INode* bone = arg_list[0]->to_node();

	int val = 0;

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID)) {

			val = c->NumKeys();

		}

	}

	return Integer::intern(val);
}

//*********************************************************

Value* doaNumVerticalKeys_cf(Value** arg_list, int count) {
	check_arg_count(doaNumVerticalKeys, 1, count);

	INode* bone = arg_list[0]->to_node();

	int val = 0;

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			val = c->SubAnim(0)->NumKeys();

		}

	}

	return Integer::intern(val);
}
//*********************************************************

Value* doaNumHorizontalKeys_cf(Value** arg_list, int count) {
	check_arg_count(doaNumHorizontalKeys, 1, count);

	INode* bone = arg_list[0]->to_node();

	int val = 0;

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			val = c->SubAnim(1)->NumKeys();

		}

	}

	return Integer::intern(val);
}
//*********************************************************

Value* doaNumRotationKeys_cf(Value** arg_list, int count) {
	check_arg_count(doaNumRotationKeys, 1, count);

	INode* bone = arg_list[0]->to_node();

	int val = 0;

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			val = c->SubAnim(2)->NumKeys();

		}

	}

	return Integer::intern(val);
}

//*********************************************************
/*
Value* doaKillHorizontal_cf(Value** arg_list, int count) {

	check_arg_count(doaKillHorizontal, 1, count);

	INode* bone = arg_list[0]->to_node();

	int val = 0;

	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			val = c->SubAnim(0)->NumKeys();

				ITCBPoint3Key	tcbKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &tcbKey);
					if (tcbKey.time > prevtime && tcbKey.time < next_time) {
						next_time = tcbKey.time;
						break;
					}
				}

			for (k = 0; k < val; val++) {


			}

		}

	}

	return Integer::intern(val);
}
*/
//*********************************************************

Value* doaGetTMKeyTimes_cf(Value** arg_list, int count) {
	check_arg_count(doaGetKeyTimes, 1, count);

	INode* bone = arg_list[0]->to_node();

	int offset = 0;

	one_typed_value_local(Array* result);

	vl.result = new Array(100); 
	
	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c && (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID)) {

			Tab<TimeValue> timetable;

			offset = c->GetKeyTimes(timetable,Interval(GetAnimStart(),GetAnimEnd()),0);

			{for (int k=0; k < timetable.Count(); k++ ){
				vl.result->append(MSTime::intern(timetable[k]));
			}}

		}

	}

	return_value(vl.result);
}

//*********************************************************

Value* doaGetHorizontalKeyTimes_cf(Value** arg_list, int count) {
	check_arg_count(doaGetHorizontalKeyTimes, 1, count);

	INode* bone = arg_list[0]->to_node();

	int offset = 0;

	one_typed_value_local(Array* result);

	vl.result = new Array(100); 
	
	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c &&  (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			Tab<TimeValue> timetable;

			offset = c->SubAnim(0)->GetKeyTimes(timetable,Interval(GetAnimStart(),GetAnimEnd()),0);

			{for (int k=0; k < timetable.Count(); k++ ){
				vl.result->append(MSTime::intern(timetable[k]));
			}}

		}

	}

	return_value(vl.result);
}


//*********************************************************
Value* doaGetVerticalKeyTimes_cf(Value** arg_list, int count) {
	check_arg_count(doaGetVerticalKeyTimes, 1, count);

	INode* bone = arg_list[0]->to_node();

	int offset = 0;

	one_typed_value_local(Array* result);

	vl.result = new Array(100); 
	
	if (bone) { 

		// Get the node's transform control
		Control *c = bone->GetTMController();

		if (c &&  (c->ClassID() == BIPBODY_CONTROL_CLASS_ID)) {

			Tab<TimeValue> timetable;

			offset = c->SubAnim(1)->GetKeyTimes(timetable,Interval(GetAnimStart(),GetAnimEnd()),0);

			{for (int k=0; k < timetable.Count(); k++ ){
				vl.result->append(MSTime::intern(timetable[k]));
			}}

		}

	}

	return_value(vl.result);
}


//*********************************************************
Value* doaGetRotationKeyTimes_cf(Value** arg_list, int count) {
	check_arg_count(doaGetRotationKeyTimes, 1, count);

	INode* bone = arg_list[0]->to_node();

	int offset = 0;

	one_typed_value_local(Array* result);

	vl.result = new Array(100); 
	
	if (bone) { 

		// Get the node's transform control

		Control *c = bone->GetTMController();

		if (c &&  c->ClassID() == BIPBODY_CONTROL_CLASS_ID) {

			Tab<TimeValue> timetable;

			offset = c->SubAnim(2)->GetKeyTimes(timetable,Interval(GetAnimStart(),GetAnimEnd()),0);

			{for (int k=0; k < timetable.Count(); k++ ){
				vl.result->append(MSTime::intern(timetable[k]));
			}}

		}

	}

	return_value(vl.result);
}

//*********************************************************
Value* doaGetNextKeyTime_cf(Value** arg_list, int count) {

	check_arg_count(doaGetNextKeyTime, 2, count);

	INode* bone = arg_list[0]->to_node();
	TimeValue prevtime = arg_list[1]->to_timevalue();


	TimeValue t;

	Control *c = bone->GetTMController();

	DWORD flags = NEXTKEY_RIGHT;//|NEXTKEY_POS|NEXTKEY_ROT|NEXTKEY_SCALE;

	TimeValue next_time = TIME_PosInfinity;
	
	if (c) {

		if (c->ClassID() == BIPBODY_CONTROL_CLASS_ID) {

			{for (int i = 0; i < 3; i ++) {
				while (c->SubAnim(i)->GetNextKeyTime(prevtime,flags,t)) {
					if (t == prevtime) {
						prevtime++;			// Screen out multiple keys
					} else {
						if (next_time > t && t > prevtime) {
							next_time = t;
						}
						break;
					}
				}
			}}

		} else if (c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) {

			while (c->GetNextKeyTime(prevtime,flags,t)) {
				if (t == prevtime) {
					prevtime++;			// Screen out multiple keys
				} else {
					next_time = t;
					break;
				}
			}


		} else if (c->ClassID() == Class_ID(PRS_CONTROL_CLASS_ID,0)){

			IKeyControl *ikeys;
			Control* sub;

			int kn,k;

			sub = c->GetPositionController();

			if (sub->ClassID() == Class_ID(TCBINTERP_POSITION_CLASS_ID, 0)) {
				ITCBPoint3Key	tcbKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &tcbKey);
					if (tcbKey.time > prevtime && tcbKey.time < next_time) {
						next_time = tcbKey.time;
						break;
					}
				}
			}
			else if (sub->ClassID() == Class_ID(LININTERP_POSITION_CLASS_ID, 0)) {
				ILinPoint3Key	linKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &linKey);
					if (linKey.time > prevtime && linKey.time < next_time) {
						next_time = linKey.time;
						break;
					}
				}
			}
			else if (sub->ClassID() == Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0)) {
				IBezPoint3Key	bezKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &bezKey);
					if (bezKey.time > prevtime && bezKey.time < next_time) {
						next_time = bezKey.time;
						break;
					}
				}
			}

			sub = c->GetRotationController();

			if (sub->ClassID() == Class_ID(TCBINTERP_ROTATION_CLASS_ID, 0)) {
				ITCBRotKey	tcbKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &tcbKey);
					if (tcbKey.time > prevtime && tcbKey.time < next_time) {
						next_time = tcbKey.time;
						break;
					}
				}
			}
			else if (sub->ClassID() == Class_ID(LININTERP_ROTATION_CLASS_ID, 0)) {
				ILinRotKey	linKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &linKey);
					if (linKey.time > prevtime && linKey.time < next_time) {
						next_time = linKey.time;
						break;
					}
				}
			}
			else if (sub->ClassID() == Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID, 0)) {
				IBezQuatKey	bezKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &bezKey);
					if (bezKey.time > prevtime && bezKey.time < next_time) {
						next_time = bezKey.time;
						break;
					}
				}
			}

			sub = c->GetScaleController();

			if (sub->ClassID() == Class_ID(TCBINTERP_SCALE_CLASS_ID, 0)) {
				ITCBScaleKey	tcbKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &tcbKey);
					if (tcbKey.time > prevtime && tcbKey.time < next_time) {
						next_time = tcbKey.time;
						break;
					}
				}
			}
			else if (sub->ClassID() == Class_ID(LININTERP_SCALE_CLASS_ID, 0)) {
				ILinScaleKey	linKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &linKey);
					if (linKey.time > prevtime && linKey.time < next_time) {
						next_time = linKey.time;
						break;
					}
				}
			}
			else if (sub->ClassID() == Class_ID(HYBRIDINTERP_SCALE_CLASS_ID, 0)) {
				IBezScaleKey	bezKey;
				ikeys = GetKeyControlInterface(sub);
				kn = ikeys->GetNumKeys();
				for (k = 0; k < kn; k++) {
					ikeys->GetKey(k, &bezKey);
					if (bezKey.time > prevtime && bezKey.time < next_time) {
						next_time = bezKey.time;
						break;
					}
				}
			}


		}
	}

	if (next_time != TIME_PosInfinity) {
		return MSTime::intern(next_time);
	}

	return &false_value;

}

//*********************************************************
Value* doaSetKeyTMValue_cf(Value** arg_list, int count) {

	check_arg_count(doaSetKeyTMValue, 3, count);

	INode* bone = arg_list[0]->to_node();
	TimeValue t = arg_list[1]->to_timevalue();
	Matrix3 relativetm = arg_list[2]->to_matrix3();

	DWORD flags = NEXTKEY_RIGHT; //|NEXTKEY_POS|NEXTKEY_ROT|NEXTKEY_SCALE;

	Control *c = bone->GetTMController();

	if (c) {
		c->SetValue(t, &SetXFormPacket(relativetm), 1,CTRL_RELATIVE);
		return &true_value;
	}

	return &false_value;

}

#include "decomp.h"

//*********************************************************
Matrix3 MakeUniform(const Matrix3& non_uniform) {

	AffineParts parts;
	Matrix3 uniform;

	decomp_affine(non_uniform,&parts);

	parts.q.MakeMatrix(uniform);

	uniform.SetRow(3,parts.t);

	return uniform;

}

//*********************************************************

Value* doaUniformMatrix_cf(Value** arg_list, int count) {

	check_arg_count(doaUniformMatrix,1, count);

	Matrix3& orig = arg_list[0]->to_matrix3();

	Matrix3 uniform = MakeUniform(orig);

	one_typed_value_local(Matrix3Value* result);

	vl.result = new Matrix3Value(uniform);

	return_value(vl.result);
      
}
//+++++++++++++++
//+++++++++++++++
void my_invert_affine(AffineParts *parts, AffineParts *inverse) {
    Quat t, p;
    inverse->f = parts->f;
    inverse->q = Conjugate(parts->q);
    inverse->u = parts->q * parts->u;
    inverse->k.x = (parts->k.x==0.0) ? 0.0f : 1.0f/parts->k.x;
    inverse->k.y = (parts->k.y==0.0) ? 0.0f : 1.0f/parts->k.y;
    inverse->k.z = (parts->k.z==0.0) ? 0.0f : 1.0f/parts->k.z;
    t = Quat(-parts->t.x, -parts->t.y, -parts->t.z, 0);
    t = Conjugate(inverse->u) * (t * inverse->u);
    t = Quat(inverse->k.x*t.x, inverse->k.y*t.y, inverse->k.z*t.z, 0);
    p = inverse->q * inverse->u;
    t = p *(t * Conjugate(p));
    inverse->t = (inverse->f>0.0) ? Point3(t.x, t.y, t.z) : Point3(-t.x, -t.y, -t.z);

}


//*********************************************************
Matrix3 MakeUniformInverse(const Matrix3& non_uniform) {

	AffineParts parts;
	AffineParts inverseparts;
	Matrix3 uniform;

	decomp_affine(non_uniform,&parts);
	my_invert_affine(&parts,&inverseparts);

	inverseparts.q.MakeMatrix(uniform);

	uniform.SetRow(3,inverseparts.t);

	return uniform;

}

//*********************************************************

Value* doaUniformInverseMatrix_cf(Value** arg_list, int count) {

	check_arg_count(doaUniformInverseMatrix,1, count);

	Matrix3& orig = arg_list[0]->to_matrix3();

	Matrix3 uniforminverse = MakeUniformInverse(orig);

	one_typed_value_local(Matrix3Value* result);

	vl.result = new Matrix3Value(uniforminverse);

	return_value(vl.result);
      
}

//*********************************************************

Value* doaRelativeMatrix_cf(Value** arg_list, int count) {

	check_arg_count(doaRelativeMatrix,2, count);

	INode* bone = arg_list[0]->to_node();

	TimeValue t = arg_list[1]->to_timevalue();

    
	/* Note: This function removes the non-uniform scaling 
	from MAX node transformations. before multiplying the 
	current node by  the inverse of its parent. The 
	removal  must be done on both nodes before the 
	multiplication and Inverse are applied. This is especially 
	useful for Biped export (which uses non-uniform scaling on 
	its body parts.) */

	Matrix3  cur_mat;       // for current and parent 
	Matrix3  par_mat;       // decomposed matrices 
                                                                 
	//Get transformation matrices

	cur_mat = MakeUniform(bone->GetNodeTM(t));

	INode *p_node = bone->GetParentNode();

	if (p_node) {
		par_mat = MakeUniform(p_node->GetNodeTM(t));
	} else {
		par_mat = Matrix3(1); 
	}

	one_typed_value_local(Matrix3Value* result);
	vl.result = new Matrix3Value(cur_mat * Inverse( par_mat));

	return_value(vl.result);
      
}

//*********************************************************

Value* doaAbsoluteMatrix_cf(Value** arg_list, int count) {

	check_arg_count(doaAbsoluteMatrix,2, count);

	INode* bone = arg_list[0]->to_node();

	TimeValue t = arg_list[1]->to_timevalue();

	Matrix3  cur_mat = MakeUniform(bone->GetNodeTM(t));

	one_typed_value_local(Matrix3Value* result);
	vl.result = new Matrix3Value(cur_mat);

	return_value(vl.result);
      
}


//*********************************************************

Value* doaNumAnimatedParents_cf(Value** arg_list, int count) {

	check_arg_count(doaNumAnimatedParents, 1, count);

	INode* n = arg_list[0]->to_node();

	if (n) { 

		// Get the node's transform control
		Control *c = n->GetTMController();

		// Is it animated with a link controller?
		if (c && (c->ClassID() == LINKCTRL_CLASSID)) {

			ILinkCtrl* lc = (ILinkCtrl*)c;

			int cnt = lc->GetParentCount();

			return Integer::intern(cnt);

		}
	}

	return Integer::intern(0);

}
//*********************************************************

Value* doaGetParentAtTime_cf(Value** arg_list, int count) {

	check_arg_count(doaGetParentAtTime, 2, count);

	INode* n = arg_list[0]->to_node();
	TimeValue t = arg_list[1]->to_timevalue();

	if (n) { 

		// Get the node's transform control
		Control *c = n->GetTMController();

		// Is it animated with a link controller?
		if (c && (c->ClassID() == LINKCTRL_CLASSID)) {

			ILinkCtrl* lc = (ILinkCtrl*)c;

			int cnt = lc->GetParentCount();

			int active_parent = 0;

			for (int p = 0; p < cnt; p++) {

				TimeValue currtime = lc->GetLinkTime(p);

				if (currtime <= t) {
					active_parent = p+1;
				} else {
					break;
				}
			}

			int r = lc->NumRefs();

			if ((active_parent > 0) && (active_parent < lc->NumRefs())) {

				INode* pnode = (INode*)lc->GetReference(active_parent);
		
				one_typed_value_local(MAXNode* result);

				vl.result = new MAXNode(pnode);
				return_value(vl.result);

			}

//			return Integer::intern(r);

		}
	}

	return &false_value;

}

//*********************************************************

Value* doaStringToInt_cf(Value** arg_list, int count) {

	check_arg_count(doaStringToInt,2, count);

	char* numstring = arg_list[0]->to_string();
	char* fmt = arg_list[1]->to_string();

	int val;
	sscanf(numstring,fmt, &val);

	return Integer::intern(val);
  
    
}

//*********************************************************

Value* doaFourCCToInt_cf(Value** arg_list, int count) {

	check_arg_count(doaFourCCToInt,1, count);

	char* fourcc = arg_list[0]->to_string();

	int len = strlen(fourcc);

	int val = 0;

	if (len>0) val  = fourcc[0]      ;
	if (len>1) val += fourcc[1] <<  8;
	if (len>2) val += fourcc[2] << 16;
	if (len>3) val += fourcc[3] << 24;

	return Integer::intern(val);
  
}

//*********************************************************

Value* doaIntToName_cf(Value** arg_list, int count) {

	check_arg_count(doaIntToName,2, count);

	int val = arg_list[0]->to_int();
	char* fmt = arg_list[1]->to_string();

	char buffer[16];
	sprintf(buffer, fmt, val);


	return Name::intern(buffer);
    
}

#if (MAX_RELEASE == 3000)
//*********************************************************

Value* doaBeginFigureMode_cf(Value** arg_list, int count) {

	check_arg_count(doaNumVerticalKeys, 1, count);

	INode* bone = arg_list[0]->to_node();

	int val = 0;

	if (bone) { 

         Control *c = bone->GetTMController();

         if ((c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
             (c->ClassID() == BIPBODY_CONTROL_CLASS_ID) ||
             (c->ClassID() == FOOTPRINT_CLASS_ID))	{
       
			// Get the Biped Export Interface from the controller 
			IBipedExport *BipIface = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);
     
			BipIface->BeginFigureMode(1);
           
			// Release the interface when you are done with it
			c->ReleaseInterface(I_BIPINTERFACE, BipIface);

			return &true_value;
		}

	}

	return &false_value;

}

//*********************************************************

Value* doaEndFigureMode_cf(Value** arg_list, int count) {

	check_arg_count(doaNumVerticalKeys, 1, count);

	INode* bone = arg_list[0]->to_node();

	if (bone) { 

         Control *c = bone->GetTMController();
         
		 if ((c->ClassID() == BIPSLAVE_CONTROL_CLASS_ID) ||
             (c->ClassID() == BIPBODY_CONTROL_CLASS_ID) ||
             (c->ClassID() == FOOTPRINT_CLASS_ID))	{
       
			// Get the Biped Export Interface from the controller 
			IBipedExport *BipIface = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);


			BipIface->EndFigureMode(1);
           
			// Release the interface when you are done with it
			c->ReleaseInterface(I_BIPINTERFACE, BipIface);

			return &true_value;
		}
	}

	return &false_value;

}


#endif



//*********************************************************

Value* doaGetUserName_cf(Value** arg_list, int count) {

	check_arg_count(doaGetUserName,0, count);

	unsigned long nbufsize = 0;
	char *nbuf=0;

	WNetGetUser(NULL,nbuf,&nbufsize);	// Calling with a ZERO length will give us 
										// the size we need
	nbuf = new char[nbufsize];

	WNetGetUser(NULL,nbuf,&nbufsize);

	Value *v = Name::intern(nbuf);

	delete [] nbuf;

	return v;

}

//*********************************************************
#include "winsock.h"

Value* doaGetHostName_cf(Value** arg_list, int count) {

	check_arg_count(doaGetHostName,0, count);

	int nbufsize = 256;
	char nbuf[256];

	gethostname(nbuf,nbufsize);

	nbufsize = strlen(nbuf);

	char* tbuf = new char[nbufsize];

	strncpy(tbuf,nbuf,nbufsize);

	Value *v = Name::intern(nbuf);

	delete [] tbuf;

	return v;

}


//*********************************************************
Value* doaGetEnvironmentVar_cf(Value** arg_list, int count) {

	check_arg_count(doaGetEnviromentVariable,1, count);

	char* varname = arg_list[0]->to_string();

	char vBuff[255];
	DWORD vBuffLen = 255;

	int len = GetEnvironmentVariable(varname,vBuff,vBuffLen);

	Value *v;

	if (len) {
		v = Name::intern(vBuff);
	} else {
		v = Name::intern("");
	}

	return v;

}

//*********************************************************
Value* doaSetTransformLock_cf(Value** arg_list, int count) {

	check_arg_count(doaSetTransformLock, 4, count);

	INode* node	= arg_list[0]->to_node();
	int	   type	= arg_list[1]->to_int();
	int	   axis	= arg_list[2]->to_int();
	BOOL   flag = arg_list[3]->to_bool();

	switch (type) {
		case 0 :	type = INODE_LOCKPOS; break;
		case 1 :	type = INODE_LOCKROT; break;
		case 2 :	type = INODE_LOCKSCL; break;
	}

	switch (axis) {
		case 0 : 	axis = INODE_LOCK_X; break;
		case 1 : 	axis = INODE_LOCK_Y; break;
		case 2 :	axis = INODE_LOCK_Z; break;
	}


	node->SetTransformLock(type,axis,flag);

	return &true_value;
}


//*********************************************************
Value* doaGetTransformLock_cf(Value** arg_list, int count) {

	check_arg_count(doaGetTransformLock, 3, count);

	INode* node	= arg_list[0]->to_node();
	int	   type	= arg_list[1]->to_int();
	int	   axis	= arg_list[2]->to_int();

	if (node->GetTransformLock(type,axis)) {
		return &true_value;
	} else {
		return &false_value;
	}


}

INode * GetImportBone(INode *ExportBone,TCHAR* oldprefix,TCHAR* newprefix) {

	TCHAR* expname = ExportBone->GetName();
	TCHAR impname[256];
	
	strcpy(impname,newprefix);

	if (!strnicmp(expname,oldprefix,strlen(oldprefix))) {
		strcat(impname,&expname[strlen(oldprefix)]);
	} else {
		strcat(impname,expname);
	}

	// Find the new bone in the scene

	return MAXScript_interface->GetINodeByName(impname);

}

class ImportBoneEnum : public ModContextEnumProc {
public:
	BOOL proc(ModContext *mc) {
		MessageBox(0,"Enumeratinga Mod","sad news",MB_OK);
		return true;
	}
};

ImportBoneEnum myModEnum;
 
//*********************************************************
Value* doaAddPhysiqueRoot_cf(Value** arg_list, int count) {

	check_arg_count(doaAddPhysiqueRoot, 2, count);

	INode* impmesh		= arg_list[0]->to_node();
	INode* root			= arg_list[1]->to_node();

	Modifier *modImp;

	if (! (modImp = FindPhysiqueModifier(impmesh)) ) {
		return &false_value;
	}

//	modImp->EnumModContexts(&myModEnum);

	IPhysiqueImport *phyImp = (IPhysiqueImport *)modImp->GetInterface(I_PHYIMPORT);
	if (!phyImp) {
		return &false_value;
	}

	phyImp->AttachRootNode(root, 0);

	return &true_value;

}

//*********************************************************
Value* doaInitializePhysique_cf(Value** arg_list, int count) {

	check_arg_count(doaInitializePhysique, 2, count);

	INode* impmesh		= arg_list[0]->to_node();
	INode* root			= arg_list[1]->to_node();

	Modifier *modImp;

	if (! (modImp = FindPhysiqueModifier(impmesh)) ) {
		return &false_value;
	}

//	modImp->EnumModContexts(&myModEnum);

	IPhysiqueImport *phyImp = (IPhysiqueImport *)modImp->GetInterface(I_PHYIMPORT);
	if (!phyImp) {
		return &false_value;
	}

	phyImp->InitializePhysique(root, 0);

	return &true_value;

}

//*********************************************************
Value* doaCopyPhysiqueWeights_cf(Value** arg_list, int count) {

//	check_arg_count(doaCopyPhysiqueWeights, 3, count);

	INode* expmesh		= arg_list[0]->to_node();
	INode* impmesh		= arg_list[1]->to_node();
	INode* root			= arg_list[2]->to_node();

	TCHAR* oldprefix;
	if (count > 3) {
		oldprefix	= arg_list[3]->to_string();
	} else {
		oldprefix = "Bip";
	}

	TCHAR* newprefix;
	if (count > 4) {
		newprefix	= arg_list[4]->to_string();
	} else {
		newprefix = "MBone";
	}


	Modifier *modExp,*modImp;

	if (expmesh == impmesh ) {
		return &false_value;
	}

	if (! (modExp = FindPhysiqueModifier(expmesh)) ) {
		return &false_value;
	}

	if (! (modImp = FindPhysiqueModifier(impmesh)) ) {
		return &false_value;
	}

//	modImp->EnumModContexts(&myModEnum);

	IPhysiqueExport *phyExp = (IPhysiqueExport *)modExp->GetInterface(I_PHYEXPORT);
	if (!phyExp) {
		return &false_value;
	}

	IPhyContextExport *phyExpContext = phyExp->GetContextInterface(expmesh);
	if (!phyExpContext) {
		return &false_value;
	}

	phyExpContext->ConvertToRigid(true);
	phyExpContext->AllowBlending(true);

	IPhysiqueImport *phyImp = (IPhysiqueImport *)modImp->GetInterface(I_PHYIMPORT);
	if (!phyImp) {
		phyExp->ReleaseContextInterface(phyExpContext);
		return &false_value;
	}

	IPhyContextImport *phyImpContext = phyImp->GetContextInterface(impmesh);
	if (!phyImpContext) {
		phyExp->ReleaseContextInterface(phyExpContext);
		return &false_value;
	}

	int expvertcount = phyExpContext->GetNumberVertices();
	int impvertcount = phyImpContext->GetNumberVertices();

	if (expvertcount != impvertcount) {
		phyImp->ReleaseContextInterface(phyImpContext);
		phyExp->ReleaseContextInterface(phyExpContext);
		return &false_value;
	}

	for (int v = 0; v < phyExpContext->GetNumberVertices(); v++) {

		if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyExpContext->GetVertexInterface(v) ) {

			int vtype = phyVertExp->GetVertexType();

			if (vtype & BLENDED_TYPE) {

				IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;

				std::vector<INode*> oldbones;
				std::vector<float> oldwgts;

				{for (int b = 0; b < phyBlendVert->GetNumberNodes(); b++) {

					if (phyBlendVert->GetNode(b)) {

						int currb;
						for (currb = 0; currb < oldbones.size(); ++currb) {
							if (oldbones[currb] == phyBlendVert->GetNode(b)) break;
						}

						if (currb < oldbones.size()) {
							oldwgts[currb] += phyBlendVert->GetWeight(b);
						} else {
							oldbones.push_back(phyBlendVert->GetNode(b));
							oldwgts.push_back(phyBlendVert->GetWeight(b));
						}
					}

				}}


				if (oldbones.size() > 1) {
					
					IPhyBlendedRigidVertexImport *blendvert = (IPhyBlendedRigidVertexImport *)phyImpContext->SetVertexInterface(v, RIGID_BLENDED_TYPE);

					if (!blendvert) {
						phyExpContext->ReleaseVertexInterface(phyVertExp);
						phyExp->ReleaseContextInterface(phyExpContext);
						phyImp->ReleaseContextInterface(phyImpContext);
						MessageBox(0,"Failed set a blended-rigid import vertex","bad news",MB_OK);
						return &false_value;
					} 

					blendvert->LockVertex(false);

					for (int actual = 0; actual < oldbones.size(); ++actual) {

						INode* impbone = GetImportBone(oldbones[actual],oldprefix, newprefix);
						int count = blendvert->SetWeightedNode(impbone,oldwgts[actual],actual==0);
					}

					blendvert->LockVertex(true);
					phyImpContext->ReleaseVertexInterface(blendvert);
					phyExpContext->ReleaseVertexInterface(phyVertExp);

				} else if (oldbones.size() == 1) {

					IPhyRigidVertexImport *rigidvert = (IPhyRigidVertexImport *)phyImpContext->SetVertexInterface(v, RIGID_TYPE);

					if (!rigidvert) {
						phyExpContext->ReleaseVertexInterface(phyVertExp);
						phyExp->ReleaseContextInterface(phyExpContext);
						phyImp->ReleaseContextInterface(phyImpContext);
						MessageBox(0,"Failed set a rigid import vertex for single source blended vert","bad news",MB_OK);
						return &false_value;
					}
					
					rigidvert->LockVertex(false);

					INode* impbone = GetImportBone(oldbones[0],oldprefix, newprefix);
					if (impbone) {
						BOOL success = rigidvert->SetNode(impbone);
					}

					rigidvert->LockVertex(true);
					phyImpContext->ReleaseVertexInterface(rigidvert);
					phyExpContext->ReleaseVertexInterface(phyVertExp);

				} else {

					MessageBox(0,"Failed to find any bones on a blended vert","bad news",MB_OK);

				}

			} else {

				IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;

				IPhyRigidVertexImport *rigidvert = (IPhyRigidVertexImport *)phyImpContext->SetVertexInterface(v, RIGID_TYPE);
				if (!rigidvert) {
					phyExpContext->ReleaseVertexInterface(phyVertExp);
					phyExp->ReleaseContextInterface(phyExpContext);
					phyImp->ReleaseContextInterface(phyImpContext);
					MessageBox(0,"Failed set a simple rigid import vertex","bad news",MB_OK);
					return &false_value;
				} 

				INode* impbone = GetImportBone(phyRigidVert->GetNode(),oldprefix, newprefix);

				rigidvert->LockVertex(false);

				if (impbone) {
					BOOL success = rigidvert->SetNode(impbone);
				}

				rigidvert->LockVertex(true);
				phyImpContext->ReleaseVertexInterface(rigidvert);
				phyExpContext->ReleaseVertexInterface(phyVertExp);
			}

		} else {
			// This deserves an ERROR!
			phyExp->ReleaseContextInterface(phyExpContext);
			phyImp->ReleaseContextInterface(phyImpContext);
			return &false_value;
		}

	}

	phyExp->ReleaseContextInterface(phyExpContext);
	phyImp->ReleaseContextInterface(phyImpContext);


	return &true_value;
}


//*********************************************************
Value* doaCheckForBadMNMesh_cf(Value** arg_list, int count) {

	//	check_arg_count(doaCopyPhysiqueWeights, 1, count);

	INode* node		= arg_list[0]->to_node();

	if (!node) return &false_value;

	Class_ID cl = node->ClassID();

	Object* boundobj= node->EvalWorldState(0).obj;

	bool ret = true;

	if (boundobj->CanConvertToType(triObjectClassID)) {

		TriObject *temptriobj = (TriObject *)boundobj->ConvertToType(0,triObjectClassID);

		Mesh& tempmesh = temptriobj->GetMesh();

		MNMesh mnmesh(tempmesh);

		MNMeshBorder mnbord;

		mnmesh.GetBorder(mnbord);

		if (   (mnmesh.numv	!= tempmesh.numVerts)
			|| (mnmesh.numf	!= tempmesh.numFaces)
			) {
			ret = false;
		}

		if (boundobj != temptriobj){
			temptriobj->DeleteMe();
		}
	}

	if (ret) return &true_value;

	return &false_value;
		
}
/*
Value* doaNasty_cf(Value** arg_list, int count) {

	Interface *my_ip = GetCOREInterface(); 

	my_ip->SetCommandPanelTaskMode(TASK_MODE_MODIFY);
	my_ip->SetSubObjectLevel(0);
		// This method returns a rollup window interface to the command panel rollup.  
		// This interface provides methods for showing and hiding rollups, adding and removing rollup pages, etc. 
	IRollupWindow *rw = my_ip->GetCommandPanelRollup();

	HWND hphysique;
	hphysique = rw->GetPanelDlg(2);

	SendMessage(GetWindow(GetDlgItem(hphysique,1004), GW_CHILD),
				WM_LBUTTONDOWN,MK_LBUTTON ,MAKELPARAM(0,0));
	SendMessage(GetWindow(GetDlgItem(hphysique,1004), GW_CHILD),
				WM_LBUTTONUP,0,MAKELPARAM(0,0));



#include "bspline_fit.h"
Value* doaSplineFit_cf(Value** arg_list, int count) {

	one_typed_value_local(Array* result);

	vl.result = new Array(100); 

	cas::bspline_fit myfitter(15,2,3);

	for (int i = 0; i < 15; i++) {

		Point2* mypoint = (Point2*)myfitter.Sample(i);

		mypoint->x = float(i);
		mypoint->y = sin(i * 3.14f * .025f);

	}

	myfitter.Fit(0.0f);

	return_value(vl.result);

	return &false_value;
}
*/
