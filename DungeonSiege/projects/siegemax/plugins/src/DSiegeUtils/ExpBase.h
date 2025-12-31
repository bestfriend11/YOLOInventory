#pragma once
/**********************************************************************
 *<
	FILE: PRSExp.cpp

	DESCRIPTION: Animation Exporter

	CREATED BY: biddle

	-- PRS REVISIONS:
	--   Version 4.0 : Ported to gmax/3dsmax 4.2
	--   Version 3.1 : Removed root keys
	--   Version 3.0 : Separated rotation and position keys
	--   Version 2.3 : Added Weapon Tracer position & rotation
	--   Version 2.2 : Note Tracks written before any keys
	--   Version 2.1 : Added ROOT keys
	--   Version 2.0 : converted to Y is UP, Z is FACING FORWARD
	--   Version 1.1 : added support for Rotation Deltas
	--   Version 1.0 : added support for NoteTracks

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#ifndef _EXPBASE_H
#define _EXPBASE_H

#include <gpcore.h>
#include <list>
#include <vector>
#include "DSiegeUtils.h"
#include "maxhelpers.h"

//========================================================================
class ExpBase 
{
	
public:

	int m_ExportedCount;

	ExpBase(const gpstring& maxfname, Interface* iface);
	~ExpBase();

	virtual bool FetchTextureFiles(std::vector<gpstring>& filenames) = 0;

	virtual DSiegeUtils::eExpErrType ExportINode(INode *node, const gpstring& expfname, FileSys::File& outfile) = 0;

protected:

	struct ExpBone
	{
		INode	*m_INode;
		int		 m_ParentIndex;

		ExpBone(INode* n,int p ) : m_INode(n), m_ParentIndex(p)	{};

		struct SortByName
		{
			bool operator() (const ExpBone& lhs, const ExpBone& rhs) const
			{
				if (lhs.m_INode && rhs.m_INode)
				{
					return _stricmp(lhs.m_INode->GetName(),rhs.m_INode->GetName()) < 0;
				}
				else
				{	
					return false;
				}
			}
		};
	};

	typedef std::vector<ExpBone>	ExpBoneArray;
	typedef ExpBoneArray::iterator	ExpBoneIter;
	
	struct RotKey
	{
		float time;
		Quat rotation;
		
		RotKey(float t, const Quat& r)
			: time(t), rotation(r) {};
	};
	struct PosKey
	{
		float time;
		Point3 position;

		PosKey(float t, const Point3& p)
			: time(t), position(p) {};
	};

	typedef std::list<RotKey> RotKeyList;
	typedef std::list<PosKey> PosKeyList;

	const gpstring& m_MaxFName;
	Interface* m_Interface;

	INode* m_ExportNode;
	INode* m_RootPosNode;

	bool m_IsDeformable;

	ExpBoneArray	m_Bones;
	int				m_RigidBoneID;
	INode*			m_RootBone;					// The bone at the top of the hierarchy

	std::vector<gpstring>	m_StringTable;
	std::vector<int>		m_StringTablePos;
	int						m_StringTableSize;
	
	Matrix3 m_CurrRootTransform;

	Quat	m_CurrRootRotation;
	Matrix3	m_CurrRootRotMatrix;
	Matrix3 m_CurrRootRotMatrixInverse;

	Point3	m_CurrRootPosition;
	
	virtual bool VerifyRequiredGlobals() = 0;

	void DetermineRootAdjustment(TimeValue t);

	bool BuildBoneList(void);
	void BuildBoneListRecursive(ExpBone& parent);
	bool FindRootBone(void);

	void InitializeStringTable(void);
	void AddStringToStringTable(const gpstring& newbie);

	void BuildBoneStringTable(void);

	void WriteStringTable(FileSys::File& outfile);
	void WriteInfoChunk(FileSys::File& outfile);

	virtual void Dump(void) = 0;
	
};

#endif
	
