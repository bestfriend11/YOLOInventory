//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorTerrain.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once 
#ifndef __EDITORTERRAIN_H
#define __EDITORTERRAIN_H


// Include Files
#include "SiegeEditorTypes.h"
#include "siege_database_guid.h"


// Forward Declarations
class Rapi;
class RapiFont;
struct vector_3;

typedef std::vector< siege::database_guid >							GuidVec;
typedef std::set< unsigned char >									LogicalIdColl;
typedef std::map< siege::database_guid, LogicalIdColl >				LogicalNodeColl;
typedef std::pair< siege::database_guid, LogicalIdColl >			LogicalNodePair;
typedef std::vector< int >											StitchedVector;


struct NodeMatchData
{
	gpstring sSno;
	siege::database_guid meshGuid;
	std::vector< int > doorMatches;
};
typedef std::vector< NodeMatchData > NodeMatchDataColl;


// This class deals with anything having to do with terrain within the editor
class EditorTerrain : public Singleton <EditorTerrain>
{
public:

	EditorTerrain();
	~EditorTerrain();

	void ReinitColors();
	RapiFont * GetFont() { return m_pFont; }

	//*********************** Node Information ************************************	
	
	siege::database_guid	GetSelectedNode()		{ return m_selected_node;	} 
	siege::database_guid	GetSecondaryNode()		{ return m_secondary_node;	}
	siege::database_guid	GetLastAddedNode()		{ return m_last_added;		}

	void					LoadSelectedNode();
	void					DrawSelectedNode();
	void					CalculateNodePlacement();
	void					RotateSelectedNode();
	void					PlaceSelectedNode();
	void					UnloadSelectedNode();
	bool					IsNodeSnapped()			{ return (m_SelectedAlpha == 200); }
	void					ResetNodeCalcTimer();
	void					UpdateNodeCalcTimer();
	void					GetSelectedPosPoint( int & x, int & y );
	void					RotateSelectedNodeMesh( int x, int y );
	void					SetSelectedSnapped( bool bSet ) { m_bSelectedIsSnapped = bSet; }
	bool					GetSelectedSnapped()			{ return m_bSelectedIsSnapped; }

	void					SetSelectedNode( siege::database_guid SelectedNode );
	void					SetSecondaryNode( siege::database_guid SecondaryNode );	
	void					ClearAllSelectedNodes();
	void					SelectAllNodes();
	void					SelectAllNodesOfMesh( siege::database_guid meshGUID, bool bClear = true );
	void					AddNodeAsSelected( siege::database_guid nodeGUID );
	void					PaintAddNodeAsSelected( siege::database_guid nodeGUID );
	void					RemoveNodeAsSelected( siege::database_guid nodeGUID );
	GuidVec &				GetSelectedNodes() { return m_selected_nodes; }
	bool					IsNodeSelected( siege::database_guid nodeGuid, bool bUnselect = false );

	void					GotoNode( gpstring szGUID );
	
	void					SetTargetNode( siege::database_guid targetNode );
	siege::database_guid	GetTargetNode() { return m_target_node; };

	void					DrawDoorNumbers( siege::database_guid nodeGUID );	
	void					DrawDoor( siege::database_guid nodeGuid, int id, DWORD dwColor = 0x00000000 );
	void					DrawDoor( siege::database_guid nodeGuid, int id, vector_3 nodeOrigin, matrix_3x3 nodeOrient, DWORD dwColor = 0x00000000 );
	void					DrawStitchDoors();
	void					DrawNodeBox( vector_3 c, vector_3 d, vector_3 colorv, Rapi& renderer );
	void					DrawNodeBox( vector_3 c, vector_3 d, DWORD dwColor, Rapi& renderer );
	void					DrawSelectedNodeBoxes();
	void					DrawLogicalFlags();

	void					GrabSelectedGUID();

	void					CompileGuidSet();
	siege::database_guid	GetValidNodeGuid();

	void					AddSelectedLogicalId( siege::database_guid nodeGuid, unsigned char id );
	void					DeselectLogicalId( siege::database_guid nodeGuid, unsigned char id );
	void					DeselectAllLogicalIds();
	void					DeselectLogicalIdsForNode( siege::database_guid nodeGuid );
	void					SelectAllLogicalIds();
	void					DrawSelectedLogicalNodeBoxes();
	void					SetSelectedLogicalFlags( siege::eLogicalNodeFlags flags );
	DWORD					GetSelectedLogicalFlags();

	void					SetTempHitLogicalNode( unsigned char id )	{ m_tempHitLogicalId = id;		}
	unsigned char			GetTempHitLogicalNode()						{ return m_tempHitLogicalId;	}

	void					InitNodeCallback();

	void					SaveLogicalFlags();
	void					SetLogicalFlagInBlock(	siege::eLogicalNodeFlags currentFlags, 
													siege::eLogicalNodeFlags testFlag,
													FuelHandle & hSave );
	void					LoadLogicalFlags();

	void					SaveLightLocks();
	void					LoadLightLocks();

	void					ReportNodeDoorMatch();
	bool					DoesNodeMatchSelected( siege::database_guid destGuid, NodeMatchData & nodeMatch );
	bool					DoesNodeMatchSourceDoor( siege::database_guid sourceGuid, int sourceDoor,
													 siege::database_guid destGuid, NodeMatchData & nodeMatch );
	bool					DoDoorsMatch( siege::database_guid sourceGuid, int sourceDoor,
										  siege::database_guid destGuid, int destDoor, bool bIgnoreY = false );
	bool					FindNodeMatches( int searchLevel, NodeMatchDataColl & nodeMatchData );
	void					RecursiveFindNodeMatches( HTREEITEM hItem, NodeMatchDataColl & nodeMatchData );

	//*********************** Node Information ************************************



	//*********************** Node Manipulation ***********************************
	
	// Attach Selected and Destination Nodes together by creating a new siege node
	void						AttachSiegeNodes( int iSource, int iDest );
	void						AttachSiegeNodes(	siege::database_guid nodeGUID,
													siege::database_guid meshGUID,
													siege::database_guid destGUID,
													int iSource, int iDest );
		
	// Delete a node from the world
	void						DeleteSiegeNode( siege::database_guid nodeGUID );
	void						DeleteSelectedNodes();

	void						SetSelectedNodeFadeSettings( int nodesection, int nodelevel, int nodeobject );

	void						ShowAllNodes( bool bShow = true );	
	void						HideSelectedNodes();

	void						SetAmbientColor( DWORD dwAmbient, DWORD dwObjectAmbient, DWORD dwActorAmbient );
	void						LoadRegionNodes();

	void						SetEnvironmentMap( gpstring sTexture );

	DWORD						AddLnoPath( siege::database_guid nodeGuid );

	void						NodeChangeWorld( const siege::SiegeNode& node, unsigned int oldMembership, bool nodeWasDeleted );

	void						SaveNodes();

	void						ReloadBoundingValues();

	//*********************** Node Manipulation ***********************************
	

	
	//*********************** Node Connection *************************************

	// Connect two existing adjacent siege nodes together by adding the doors
	void						ConnectSiegeNodes(	siege::database_guid sourceGUID, int iSource, 
													siege::database_guid destGUID, int iDest );

	// Automatically connect all adjacent doors
	void						AutoNodeConnection();	
	void						AutoNodeConnectSelected();
	void						AutoNodeConnect( siege::database_guid nodeGUID );
	void						AutoStitch( int startingId, const gpstring & sRegion, StitchedVector & stitches );

	// Door Connection Data
	void						SetSourceDoors( siege::database_guid SourceConnection );
	StringVec					GetSourceDoors()		{ return m_source_doors; };
	void						SetDestDoors( siege::database_guid DestConnection );
	StringVec					GetDestDoors()			{ return m_dest_doors; };
	void						SetSelectedSourceDoorID( int id );
	int							GetSelectedSourceDoorID()			{ return m_source_door_id; }
	void						SetDestConnectionID( int id )		{ m_dest_door_id = id; }
	int							GetDestConnectionID()				{ return m_dest_door_id; }

	//*********************** Node Connection *************************************



	//*********************** Node Properties *************************************

	void			SetNodeSetCharacters( gpstring sNodeSet )	{ m_sNodeSet = sNodeSet;	}
	gpstring &		GetNodeSetCharacters()							{ return m_sNodeSet;		}
	void			SetTerrainNodeSet( gpstring sSet, gpstring sVersion );
	
	void UpdateSelectedNodeLighting();
	void UpdateNodeLighting( siege::database_guid nodeGUID );
	void UpdateAllNodeLighting();

	void NodeCaps( bool bVisible );

	//*********************** Node Properties *************************************

	gpstring GenerateRandomGUID();

	// Grid drawing
	void DrawGrid();

	// Frustum drawing
	void DrawFrustum();

private:

	siege::database_guid			m_selected_node;			// Currently Selected Node
	siege::database_guid			m_secondary_node;			// Node the user may try to attach
	siege::database_guid			m_target_node;				// Target node
	siege::database_guid			m_last_added;				// The last added node
	siege::database_guid			m_DrawSelected;
	int								m_source_door_id;			// Selected Source Door id
	StringVec						m_source_doors;				// Text list of all the source doors
	StringVec						m_dest_doors;				// Text list of all the dest doors
	int								m_selected_door_id;			// Selected door ID
	int								m_dest_door_id;				// Destination door id
	unsigned char					m_tempHitLogicalId;			// Selected Logical Node id

	// Visual node connection data
	vector_3						m_SelectedPos;
	matrix_3x3						m_SelectedOrient;
	DWORD							m_SelectedAlpha;
	siege::database_guid			m_SelectedGuid;
	bool							m_bSelectedRotate;
	float							m_SelectedCalcTime;	
	bool							m_bSelectedIsSnapped;
	
	GuidVec							m_selected_nodes;			// This holds a list of multiple selected nodes
																// allowing for the change of properties
	
	LogicalNodeColl					m_selectedLogicalIds;			

	gpstring						m_sNodeSet;
	RapiFont *						m_pFont;

	std::set< int >					m_usedGuids;

	typedef std::map< DWORD, DWORD > ValueToColorMap;
	typedef std::map< DWORD, bool >	 ColorUsageMap;

	ColorUsageMap					m_ColorUsageMap;
	ValueToColorMap					m_ValueToColorMap;	

};


#define gEditorTerrain EditorTerrain::GetSingleton()


#endif