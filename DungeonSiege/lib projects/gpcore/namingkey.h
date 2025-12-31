#pragma once
#ifndef NAMINGKEY_H
#define NAMINGKEY_H
/*==================================================================================================
 
 NamingKey.h
 
 purpose: Provide support for an art content naming key
 
 author:  biddle
 
 (c)opyright Gas Powered Games, 1999
 
---------------------------------------------------------------------------------------------------*/

#include "gpcore.h"

#include <list>
#include <vector>

class NameNode {

public:

	// Now supports 'parentname' as well as 'parentnode'

	typedef std::list<NameNode*> NameNodeList;
	typedef NameNodeList::iterator NameNodeIter;

	NameNode(gpstring a, gpstring n, gpstring d, NameNode* p) 
		: m_abbr(a)
		, m_name(n)
		, m_desc(d)
		, m_parent(p)
	{
		if (p) {
			p->m_children.push_back(this);
		}
	};

	~NameNode() {
		for (NameNodeIter i = m_children.begin(); i != m_children.end(); ++i) {
			delete (*i);
		}
	};

	NameNode* NameNode::InsertChild(gpstring childname);

	gpstring m_abbr;	
	gpstring m_name;
	gpstring m_desc;	

	NameNode* m_parent;
	NameNodeList m_children;
	
};


class NamingKey : public Singleton<NamingKey> {

public :

	NamingKey();
	
	NamingKey(const char* dirname);

	~NamingKey(void); 

	DWORD GetNumFileNames() { return m_FileNames.size(); }

	const gpstring& GetFileName(DWORD i) { return ((i < m_FileNames.size()) ? m_FileNames[i] : gpstring::EMPTY);}

	bool LoadNamingKey(const char* dirname);

	bool BuildContentPath(const char *candidate,gpstring &dest) const;
	bool BuildContentLocation(const char *candidate,gpstring &dest) const;

	bool BuildASPLocation  (const char *candidate,gpstring &dest) const;
	bool BuildPRSLocation  (const char *candidate,gpstring &dest) const;
	bool BuildIMGLocation  (const char *candidate,gpstring &dest) const;
	bool BuildSNOLocation  (const char *candidate,gpstring &dest) const;
	bool BuildWAVLocation  (const char *candidate,gpstring &dest) const;
	bool BuildMP3Location  (const char *candidate,gpstring &dest) const;
	bool BuildSkritLocation(const char *candidate,gpstring &dest) const;

	bool BuildContentDescription(const char *candidate,std::list<gpstring> &desc) const;

	void DumpTrees(void) const;

private:

	enum ParseError
	{
		NNK_SUCCEEDED = 0,
		NNK_ERROR_NO_MATCH,
		NNK_ERROR_ILLEGAL,
		NNK_ERROR_DUPLICATE,
		NNK_ERROR_COULD_NOT_CREATE,
		NNK_ERROR_UNKNOWN_TREE
	};

	std::vector<gpstring>		m_FileNames;

	NameNode*		m_BitMapTree;
	NameNode*		m_MeshTree;
	NameNode*		m_AnimTree;
	NameNode*		m_TerrainTree;
	NameNode*		m_SoundTree;
	NameNode*		m_SkritTree;

	bool ParseNamingKey(const char *buf,int bufLen, int& numparsed);

	NameNode* BitMapRoot()		{ return m_BitMapTree;	}
	NameNode* MeshRoot()		{ return m_MeshTree;	}
	NameNode* AnimRoot()		{ return m_AnimTree;	}
	NameNode* TerrainRoot()		{ return m_TerrainTree;	}
	NameNode* SoundRoot()		{ return m_SoundTree;	}
	NameNode* SkritRoot()		{ return m_SkritTree;	}

	NameNode* FindNode(NameNode* tree,const char* searchabbr) const;

	bool BuildTreePath(NameNode* tree, const char *abbr, gpstring &path) const;

	bool BuildTreeDesc(NameNode* tree,const char *searchabbr, std::list<gpstring> &desc) const;

	ParseError InsertNode(NameNode* &tree,const char* abbr, const gpstring& name, const gpstring& desc);

	bool DumpTree(NameNode* tree, int spacing) const;

	const char* LookupErrorCode(ParseError err);

};

#define gNamingKey NamingKey::GetSingleton()

#endif
