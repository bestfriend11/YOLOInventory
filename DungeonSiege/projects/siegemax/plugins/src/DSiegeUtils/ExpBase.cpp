/**********************************************************************
 *<
	FILE: ExpBase.cpp

	DESCRIPTION: Dungeon Siege Exporter base class(es)

	CREATED BY: biddle

	HISTORY: 

 *>	Copyright (c) 2002, All Rights Reserved.
 **********************************************************************/

#include <gpcore.h>
#include <gpmath.h>
#include <list>
#include <vector>
#include "DSiegeUtils.h"
#include "MAXhelpers.h"

#include "ExpBase.h"

ExpBase::ExpBase(const gpstring& maxfname, Interface* iface)
	: m_MaxFName(maxfname)
	, m_Interface(iface)
	, m_ExportNode(NULL)
	, m_RootPosNode(NULL)
	, m_IsDeformable(false)
	, m_RigidBoneID(-2)
	, m_RootBone(NULL)
	, m_StringTableSize(0)
{
}

ExpBase::~ExpBase()
{
}

//-----------------------------------------------------------------
void ExpBase::DetermineRootAdjustment(TimeValue t) 
{
	m_RootPosNode = NULL;

	gpstring dummrootnodename;
	gpstring expname = m_ExportNode->GetName();
	stringtool::RemoveBorderingWhiteSpace(expname);

	dummrootnodename.assignf("dummyroot_%s",expname.c_str());

	// Look for a a root position node in the parent of the export node
	INode* searchnode = m_ExportNode->GetParentNode();
	while (searchnode && !searchnode->IsRootNode())
	{
		gpstring searchname = searchnode->GetName();
		stringtool::RemoveBorderingWhiteSpace(searchname);

		if (   ::same_no_case(searchname,dummrootnodename)
			|| ::same_no_case(searchname.c_str(),"dummyroot")
			|| ::same_no_case(searchname.c_str(),"root"))
		{
			m_RootPosNode = searchnode;
			break;
		}
		searchnode =  searchnode->GetParentNode();
	}

	// Look for roots that are NOT in the parent hierarchy, but are correctly named
	if (!m_RootPosNode)
	{
		m_RootPosNode = m_Interface->GetINodeByName(dummrootnodename.c_str());
	}

	if (!m_RootPosNode)
	{
		m_RootPosNode = m_Interface->GetINodeByName("dummyroot");
	}

	if (!m_RootPosNode)
	{
		m_RootPosNode = m_Interface->GetINodeByName("root");
	}


	if (!m_RootPosNode)
	{
		// There is no root pos node, the exported node will act as it own root pos.
		m_RootPosNode = m_ExportNode;
		m_CurrRootTransform = m_RootPosNode->GetNodeTM(t);
		m_CurrRootTransform.PreRotateX(HALFPI);
	}
	else
	{
		m_CurrRootTransform = m_RootPosNode->GetNodeTM(t);
	}

	m_CurrRootRotation = Inverse(Quat(m_CurrRootTransform));
	
	m_CurrRootRotation.MakeMatrix(m_CurrRootRotMatrix);
	m_CurrRootRotation.Inverse().MakeMatrix(m_CurrRootRotMatrixInverse);
	m_CurrRootPosition = m_CurrRootTransform.GetTrans();
}

//-----------------------------------------------------------------
bool ExpBase::FindRootBone(void)
{
	m_RootBone = FetchTopmostBone(m_ExportNode);
	return IsValidBone(m_RootBone);
}

//-----------------------------------------------------------------
void ExpBase::BuildBoneListRecursive(ExpBone& parentBone)
{
//	if ( IsValidBone(parentBone) )
	{	
		m_Bones.push_back(parentBone);

		INode* parentNode = parentBone.m_INode;
		
		gpstring parentName(parentNode->GetName());
		stringtool::RemoveBorderingWhiteSpace(parentName);

		if (!parentName.same_no_case("weapon_grip") && !parentName.same_no_case("shield_grip"))
		{
			ExpBoneArray sortedkids;

			// Does the node have ForceChildren?
			TSTR propstring;
			if (parentNode->GetUserPropString("ActualChildren",propstring))
			{
				// We have encountered a node with a list of Actual Children created by
				// the Character Studio to Skin conversion process.

				// Extract the names of the "actual kids", rather than using the 'real' kids
				gpstring kidstring(propstring);

				DWORD numstrings = stringtool::GetNumDelimitedStrings(kidstring, ',');
				for2 (int i = 0; i < numstrings; ++i)
				{
					gpstring value;
					if ( stringtool::GetDelimitedValue( kidstring, ',', i, value ) )
					{
						// 'Get' the value to convert it to a string
						gpstring kidname;
						if ( stringtool::Get( value, kidname ) )
						{
							INode* kidNode = GetCOREInterface()->GetINodeByName(kidname);
							
							if ( kidNode && IsValidBone(kidNode) )
							{
								sortedkids.push_back(ExpBone(kidNode,m_Bones.size()-1));
							}
						}
					}
				}
			}
			else
			{
				for2 (int k = 0; k < parentNode->NumberOfChildren(); ++k)
				{
					INode* kidNode = parentNode->GetChildNode(k);
					if ( IsValidBone(kidNode) )
					{
						sortedkids.push_back(ExpBone(kidNode,m_Bones.size()-1));
					}
				}
			}

			std::sort(sortedkids.begin(),sortedkids.end(),ExpBone::SortByName());

			for2 (ExpBoneArray::iterator it = sortedkids.begin(); it != sortedkids.end(); ++it )
			{
				BuildBoneListRecursive(*it);
			}
			
		}

	}
}

//-----------------------------------------------------------------
bool ExpBase::BuildBoneList(void)
{
	m_Bones.clear();
	m_RigidBoneID = 0;

	FindRootBone();

	if (!m_RootBone)
	{
		return false;
	}

	BuildBoneListRecursive(ExpBone(m_RootBone,-1));

	if (!m_IsDeformable)
	{
		for2 (int i = 0; i != m_Bones.size(); ++i)
		{
			if (m_Bones[i].m_INode == m_ExportNode)
			{
				m_RigidBoneID = i;
				break;
			}
		}
	}

	return m_Bones.size()>0;

}

//-----------------------------------------------------------------
void ExpBase::InitializeStringTable(void)
{
	m_StringTable.clear();
	m_StringTablePos.clear();
	m_StringTableSize = 0;
}

//-----------------------------------------------------------------
void ExpBase::AddStringToStringTable(const gpstring& newbie)
{
	m_StringTable.push_back(newbie);
	m_StringTablePos.push_back(m_StringTableSize); 
	m_StringTableSize += (int)(ceilf((newbie.size() + 1)/4.0f) * 4);
}

//-----------------------------------------------------------------
void ExpBase::BuildBoneStringTable(void)
{

	gpstring gripname;

	gpstring expname = m_ExportNode->GetName();
	stringtool::RemoveBorderingWhiteSpace(expname);

	gripname.assignf("grip_%s",expname.c_str());

	for (int b = 0; b < m_Bones.size(); ++b)
	{
		gpstring bname = m_Bones[b].m_INode->GetName();
		stringtool::RemoveBorderingWhiteSpace(bname);

		if (bname.same_no_case(gripname))
		{
			bname = "GRIP";
		}
		else
		{
			stringtool::ReplaceAllChars(bname,' ','_');
		}
		
		AddStringToStringTable(bname);
	}
}

//-----------------------------------------------------------------
void ExpBase::WriteStringTable(FileSys::File& outfile)
{
	for (int i=0; i<m_StringTable.size(); ++i )
	{
		int slen = m_StringTable[i].size()+1;
		int padsize = (int)((ceilf((slen)/4.0f) * 4) - slen);

		outfile.WriteText(m_StringTable[i]);
		DWORD zero = 0;
		outfile.Write(&zero,padsize+1);
	}
}

//-----------------------------------------------------------------
void ExpBase::WriteInfoChunk(FileSys::File& outfile)
{
	DWORD fourcc = REVERSE_FOURCC('INFO');
	outfile.Write(&fourcc,sizeof(fourcc));

	DWORD numstrings = 1;
	outfile.Write(&numstrings,sizeof(numstrings));
	outfile.WriteF("SiegeMax%c",0);

};




