#pragma once
/**********************************************************************
 *<
	FILE: MAXhelpers.h

	DESCRIPTION: Helpers for maxscript

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef _MAXHELPERS_H
#define _MAXHELPERS_H

#include <MAXScrpt.h>
#include <IParamb2.h>
#include <ModStack.h>
#include <ISkin.h>
#include "3DMath.h"
#include <decomp.h>
#include <MAXObj.h>
#include <msplugin.h>

#include <gpmath.h>
#include <gpstring.h>


Modifier* FetchPhysiqueModifier(INode* nodePtr);

ISkin* FetchSkinInterface(INode* nodePtr);

MSPlugin* FetchSiegeMaxInterface(INode* nodePtr, const gpstring& ifacenname);

bool IsDummyObject(INode* nodePtr);
bool IsValidBone(INode *nodePtr);
INode* FetchValidParent(INode* nodePtr);
INode* FetchTopmostBone(INode* nodePtr);

//-----------------------------------------------------------------
inline int SortINodesByName(INode* lhs, INode* rhs)
{
	if (lhs && rhs)
	{
		return _stricmp(lhs->GetName(),rhs->GetName());
	}
	else
	{
		return 0;
	}
};

//-----------------------------------------------------------------
inline bool IsEqual(const Point2& p0, const Point2& p1)
{
	return IsEqual(p0.x,p1.x) && IsEqual(p0.y,p1.y);
}

//-----------------------------------------------------------------
inline bool IsEqual(const Point2& p0, const Point2& p1, float tol)
{
	return IsEqual(p0.x,p1.x,tol) && IsEqual(p0.y,p1.y,tol);
}

//-----------------------------------------------------------------
inline bool IsEqual(const Point3& p0, const Point3& p1)
{
	return IsEqual(p0.x,p1.x) && IsEqual(p0.y,p1.y) && IsEqual(p0.z,p1.z);
}

//-----------------------------------------------------------------
inline bool IsEqual(const Point3& p0, const Point3& p1, float tol)
{
	return IsEqual(p0.x,p1.x,tol) && IsEqual(p0.y,p1.y,tol) && IsEqual(p0.z,p1.z,tol);
}

//-----------------------------------------------------------------
inline float RoundOff(float v, float eps)
{
	if (eps == 0.0f)
	{
		return v;
	}
	
	if (v < 0)
	{
		return ceil((v/eps) - 0.5f)*eps;
	}
	else
	{
		return floor((v/eps) + 0.5f)*eps;
	}
}

//-----------------------------------------------------------------
inline DWORD FourCCToDWORD(const TCHAR* fourcc,bool reversed=false)
{
	int len = strlen(fourcc);
	int val = 0;
	if (reversed)
	{
		if (len>3) val  = fourcc[3]      ;
		if (len>2) val += fourcc[2] <<  8;
		if (len>1) val += fourcc[1] << 16;
		if (len>0) val += fourcc[0] << 24;
	}
	else
	{
		if (len>0) val  = fourcc[0]      ;
		if (len>1) val += fourcc[1] <<  8;
		if (len>2) val += fourcc[2] << 16;
		if (len>3) val += fourcc[3] << 24;
	}
	return val;
}


//-----------------------------------------------------------------
inline gpstring DWORDToFourCC(DWORD val,bool reversed=false)
{

	TCHAR fourcc[5];
	if (reversed)
	{
		fourcc[3] = (val      ) & 0xff;
		fourcc[2] = (val >>  8) & 0xff;
		fourcc[1] = (val >> 16) & 0xff;
		fourcc[0] = (val >> 24) & 0xff;
	}
	else
	{
		fourcc[0] = (val      ) & 0xff;
		fourcc[1] = (val >>  8) & 0xff;
		fourcc[2] = (val >> 16) & 0xff;
		fourcc[3] = (val >> 24) & 0xff;
	}

	for (int i = 0; i!=4; i++)
	{
		if (fourcc[i] == 0)
		{
			fourcc[i] = ' ';
		}
	}
	
	fourcc[4] = 0;

	return gpstring(fourcc);
}

//-----------------------------------------------------------------
inline bool HasNegativeParity(Matrix3 mat)
{
	if (mat.IsIdentity())
	{
		return false;
	}

	AffineParts ap;
	decomp_affine(mat, &ap);

	return  (ap.f < 0.0f);
}


#endif
