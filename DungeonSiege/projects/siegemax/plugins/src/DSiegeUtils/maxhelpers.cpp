#include "maxhelpers.h"
#include <stringtool.h>
#include "phyexp.h"
#include "bipexp.h"

// --- From Phyexp.h ------------------------------------------------------
// example code to find if a given node contains a Physique Modifier
// IDerivedObject requires you include "modstack.h" from the MAX SDK

//-----------------------------------------------------------------
Modifier* FetchPhysiqueModifier(INode* nodePtr)
{
	// Get object from node. Abort if no object.
	
	Object* pObj = nodePtr->GetObjectRef();

	if (!pObj) return NULL;

	// Is derived object ?
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* pDerObj = static_cast<IDerivedObject*>(pObj);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < pDerObj->NumModifiers())
		{
			// Get current modifier.
			Modifier* mod = pDerObj->GetModifier(ModStackIndex);

			// Is this a Physique mod?
			if ( mod->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B) ) 
			{
				// Yes -> Exit.
				return mod;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		pObj = pDerObj->GetObjRef();
	}

	// Not found.
	return NULL;

	// Not found.
	return NULL;
}


//-----------------------------------------------------------------
ISkin* FetchSkinInterface(INode* node)
{
	// Get object from node. Abort if no object.
	Object* pObj = node->GetObjectRef();

	if (!pObj) return NULL;

	// Is derived object ?
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* pDerObj = static_cast<IDerivedObject*>(pObj);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < pDerObj->NumModifiers())
		{
			// Get current modifier.
			Modifier* mod = pDerObj->GetModifier(ModStackIndex);

			// Is this Skin ?
			if (mod->ClassID() == SKIN_CLASSID )
			{
				// Yes -> Exit.
				return (ISkin*)mod->GetInterface(I_SKIN);
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		pObj = pDerObj->GetObjRef();
	}

	// Not found.
	return NULL;
}

//-----------------------------------------------------------------
MSPlugin* FetchSiegeMaxInterface(INode* node, const gpstring& ifacename )
{

	// Get object from node. Abort if no object.
	Object* pObj = node ? node->GetObjectRef() : NULL;

	if (!pObj) return NULL;

	// Is derived object ?
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* pDerObj = static_cast<IDerivedObject*>(pObj);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < pDerObj->NumModifiers())
		{
			// Get current modifier.
			Modifier* mod = pDerObj->GetModifier(ModStackIndex);
			
			MSPlugin* msplug = (MSPlugin*)mod->GetInterface(I_MAXSCRIPTPLUGIN);

			if ( msplug && msplug->pc)
			{
				TSTR internalname = msplug->pc->InternalName();

				if (ifacename.same_no_case(internalname))
				{
					if ( msplug->get_delegate() && 
					   ( msplug->get_delegate()->ClassID() == Class_ID(MESHSELECT_CLASS_ID,0)) )
					{
						return msplug;
					}
				}
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		pObj = pDerObj->GetObjRef();
	}

	// Not found.
	return NULL;
}


//-----------------------------------------------------------------
INode* FetchValidParent(INode* kid)
{
	
	INode* vparent = kid->GetParentNode();
	
	while ( vparent ) 
	{
		if ( vparent->IsRootNode())
		{
			// We are at the root of the scene
			return kid;
		}

		gpstring vpname(vparent->GetName());
		vpname.to_upper();
		stringtool::RemoveBorderingWhiteSpace(vpname);

		if ( vpname.same_with_case("ROOT") || vpname.same_with_case("TEMPROOT") ) 
		{
			// The we are at the ROOT of the tree
			return kid;
		}
			
	    if (   (vpname.find("DUMMY") != vpname.npos)
			|| FetchSkinInterface(vparent)
			|| FetchPhysiqueModifier(vparent) ) 
		{
			// We need to skip this parent
			vparent = vparent->GetParentNode();
		}
		else
		{
			return vparent;
		}
	}
	
	// The kid itself had no valid parent, so it me be the parent
	return kid;

}

//-----------------------------------------------------------------
bool IsValidBone(INode* bone)
{
	if (!bone) return false;

	// $$ 3dsmax: Need to look for Physique stuff
	//
	// if (doaValidFootstep b) then  return false
	// if b.modifiers["physique"] != undefined then return false

	if (FetchSkinInterface(bone)) return false;

	if (FetchPhysiqueModifier(bone)) return false;

	if (IsDummyObject(bone)) return false;

	gpstring bname(bone->GetName());
	bname.to_upper();
	stringtool::RemoveBorderingWhiteSpace(bname);

	if ( bname.find("SKINMESH")	!= bname.npos ) return false;

	return true;
}

//-----------------------------------------------------------------
bool IsBipedBone(INode* bone)
{
	if (!bone) return false;

	// $$ 3dsmax: Need to look for Physique stuff
	//
	// if (doaValidFootstep b) then  return false
	// if b.modifiers["physique"] != undefined then return false

	if (FetchSkinInterface(bone)) return false;

	if (FetchPhysiqueModifier(bone)) return false;

	if (IsDummyObject(bone)) return false;

	gpstring bname(bone->GetName());
	bname.to_upper();
	stringtool::RemoveBorderingWhiteSpace(bname);

	if ( bname.find("SKINMESH")	!= bname.npos ) return false;

	return true;
}

//-----------------------------------------------------------------
bool IsDummyObject(INode* bone)
{
	if (!bone) return false;

	gpstring bname(bone->GetName());
	bname.to_upper();
	stringtool::RemoveBorderingWhiteSpace(bname);

	if ( bname.same_with_case("ROOT") ) return true;

	if ( bname.find("DUMMY")		!= bname.npos ) return true;
	if ( bname.find("BBOX_")		== 0 ) return true;
	if ( bname.find("__")			== 0 ) return true;

	// $$ 3dsmax: Need to look for Physique stuff
	//
	// if (doaValidFootstep b) then  return false
	// if b.modifiers["physique"] != undefined then return false

	return false;
}

//-----------------------------------------------------------------
INode* FetchTopmostBone(INode* kid)
{
	INode* tmb = NULL;

	ISkin* skin = FetchSkinInterface(kid);
	if (skin)
	{
		int nb = skin->GetNumBones();
		if (nb)
		{
			tmb = skin->GetBone(0);
		}
	}
	else
	{
		Modifier* physmod = FetchPhysiqueModifier(kid);
		if ( physmod )
		{
			IPhysiqueExport *phyExp = (IPhysiqueExport *)physmod->GetInterface(I_PHYINTERFACE);
			if ( IPhyContextExport *phyContextExp = (IPhyContextExport *)phyExp->GetContextInterface(kid) ) 
			{
				phyContextExp->ConvertToRigid(true);
				phyContextExp->AllowBlending(true);

				int numverts = phyContextExp->GetNumberVertices();
				if (numverts > 0)
				{
					if (IPhyVertexExport *phyVertExp = (IPhyVertexExport *)phyContextExp->GetVertexInterface(0) )
					{
						int vtype = phyVertExp->GetVertexType();
						if (vtype & BLENDED_TYPE)
						{
							IPhyBlendedRigidVertex* phyBlendVert = (IPhyBlendedRigidVertex *)phyVertExp;
							if ( phyBlendVert->GetNumberNodes() > 0 )
							{
								tmb = phyBlendVert->GetNode(0);
							}
						}
						else
						{
							IPhyRigidVertex* phyRigidVert = (IPhyRigidVertex *)phyVertExp;
							tmb = phyRigidVert->GetNode();
						}
					}
				}
			}
		}
		else
		{
			if (IsValidBone(kid))
			{
				tmb = kid;
			}
			else
			{
				tmb = FetchValidParent(kid);
			}
		}
	}

	if ( IsValidBone(tmb) )
	{
		INode* kid = tmb;
		tmb = FetchValidParent(tmb);
		while (tmb != kid) 
		{
			kid = tmb; 
			tmb = FetchValidParent(tmb);
		}
	}

	return tmb;
}
