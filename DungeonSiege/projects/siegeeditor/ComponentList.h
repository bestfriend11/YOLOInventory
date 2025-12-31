//////////////////////////////////////////////////////////////////////////////
//
// File     :  ComponentList.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////



#pragma once
#ifndef __COMPONENTLIST_H
#define __COMPONENTLIST_H


// Include Files
#include "SiegeEditorShell.h"
#include "GoDefs.h"


class GoDataTemplate;


// Enumerated Data Types
enum eDisplayType
{
	BLOCK_NAME,
	SCREEN_NAME,
	DEV_NAME,
};


// Structures
struct TATTOO_NODE_COMPONENT 
{
	gpstring		GUIDstring;
	gpstring		FileName;
	gpstring		Name;	
};


struct ObjectComponent
{	
	gpstring sName;
	gpstring sScreenName;
	gpstring sFuelName;
	gpstring sDevName;
	int		 dwScid;
	Goid object;
};


struct DuplicateNode
{
	gpstring sName1;
	gpstring sName2;
	int		 nodeID;	
};


//////////////////////////////////
// View Db Searching

enum eSearchType
{
	ST_SCREEN_NAME,
	ST_TEMPLATE,
	ST_DOCUMENTATION,
	ST_CATEGORY,
};

struct ComponentSearchData
{
	gpstring	sName;
	bool		bRequired;
	eSearchType type;
	
};

typedef std::vector< ComponentSearchData > ViewSearchVec;
typedef std::vector< int > ViewSearchIdMatches;

struct SearchFolder
{
	gpstring sName;
	gpstring sParent;

	ViewSearchVec searchRequests;
	ViewSearchIdMatches matches;
};

typedef std::vector< SearchFolder > ViewSearchFolderVec;



///////////////////////////////////


typedef std::vector< ObjectComponent > ComponentVec;

typedef std::map< int, ObjectComponent > ComponentIdMap;
typedef std::pair< int, ObjectComponent > ComponentIdPair;

typedef std::map< gpstring, int, istring_less > ComponentTemplateMap;
typedef std::pair< gpstring, int > ComponentTemplatePair;

typedef std::map< gpstring, TATTOO_NODE_COMPONENT, istring_less > NodeComponentMap;
typedef std::pair< gpstring, TATTOO_NODE_COMPONENT > NodeComponentPair;

typedef std::vector< DuplicateNode > DuplicateNodeColl;

typedef std::set< gpstring, istring_less > StringSet;

class ComponentList : public Singleton <ComponentList>
{
public:
	// Public Methods

	// Constructor
	ComponentList() { m_currId = 1; };
	~ComponentList() {};

	// Retrieve a list of available nodes and store them in our data structure
	void RetrieveNodeCollection( StringVec & node_list );
	void RetrieveExistingNodeCollection( StringVec & nodeColl );
	void RetrieveNodeMeshCollection( StringVec & node_list );

	// Retrieve a list of available objects and store them in our data structure			
	void RetrieveObjectList			( ComponentVec & object_list, eDisplayType type = BLOCK_NAME );
	void RecursiveRetrieveObjectList( GoDataTemplate * pTemplate, ComponentVec & object_list, eDisplayType disp_type );

	// Retrieve a list of available equipment depending on the filter type
	void RetrieveEquipmentList		( ComponentVec & item_vec, eEquipSlot filter = ES_NONE );
	void RecursiveRetrieveEquipmentList( GoDataTemplate * pTemplate, ComponentVec & item_vec, eEquipSlot filter = ES_NONE );
	
	void SearchAndRetrieveContentNames(	ComponentVec & content, gpstring block_name, 
										gpstring screen_name,
										gpstring description,
										gpstring type );

	void RecursiveSearchAndRetrieveContentNames(	GoDataTemplate * pTemplate,
													ComponentVec & content, gpstring block_name, 
													gpstring screen_name,
													gpstring description,
													gpstring type );

	bool DoesSpecialize( GoDataTemplate * pTemplate, gpstring sBase );	

	void LoadViewSearchDb( ViewSearchFolderVec & searchFolders );
	void ViewSearch( ViewSearchFolderVec & searchFolders );
	void RecursiveViewSearch( GoDataTemplate * pTemplate, ViewSearchFolderVec & searchFolders );

	gpstring GetTemplateField( gpstring sFieldName );
	ObjectComponent GetObjectComponent( gpstring sTemplate );
	
	// Retrieve a list of the actual used siegenodes
	void RetrieveUsedNodeList( StringVec & used_nodes );	
	
	void GetMeshToSnoMap( std::map< gpstring, gpstring, istring_less > & mesh_list );

	gpstring GetMeshSnoFile( gpstring sMesh );	

	// Get a GUID to a node mesh
	gpstring GetNodeGUID( char *szNode );	

	void RetrieveObjectInventoryVec( Goid object, ComponentVec & inventory );
	
	void GetTemplateTypes( StringVec & types );
	void RecursiveGetTemplateTypes( const GoDataTemplate * pTemplate, StringVec & types );

	void RetrieveLocalActorList( ComponentVec & actor_list );

	gpstring GetFuelName( Scid scid );

	bool	GetNameFromMapId( int id, gpstring & sName );
	bool	GetTemplateNameFromMapId( int id, gpstring & sTemplate );
	bool	GetMapIdFromTemplateName( gpstring sTemplate, int & id );
	void	InsertTemplateId( gpstring sTemplate );
	int		GetCurrentTemplateId() { return m_currId; }

	const ComponentTemplateMap&	GetTemplateMap() { return m_ComponentTemplates; }

	void BuildCategoryNames();
	void GetCategoryNames( StringSet & categoryNames );

private:

	ComponentIdMap			m_ComponentIds;
	ComponentTemplateMap	m_ComponentTemplates;
	ComponentVec			m_objectColl;
	StringSet				m_categoryNames;

	int m_currId;

	NodeComponentMap m_node_component_list;
};

#define gComponentList ComponentList::GetSingleton()


// Remove directory listings from a name
char * RemoveDirectories( char *name );
gpstring RemoveDirectories( const gpstring & sName );
gpstring GetFileFromFullPath( const gpstring & sFullPath );

#endif