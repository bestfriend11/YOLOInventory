//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorGizmos.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __EDITOR_GIZMOS_H
#define __EDITOR_GIZMOS_H

#include "siege_pos.h"

class FuelHandle;

struct StartingPos
{
	SiegePos	spos;
	int			id;
	int			gizmoId;
	SiegePos	camPos;
	float		azimuth;
	float		distance;
	float		orbit;
};

typedef std::vector< StartingPos > StartingPositions;

struct WorldLevel
{
	gpstring sWorld;
	int		 level;
};

typedef std::vector< WorldLevel > WorldLevels;

struct StartingGroup
{
	gpstring sGroup;
	gpstring sDescription;
	gpstring sScreenName;
	StartingPositions positions;
	bool bDefault;
	bool bDevOnly;
	int id;
	WorldLevels levels;
};

typedef std::vector< StartingGroup > StartingGroups;


struct Gizmo;

class EditorGizmos : public Singleton <EditorGizmos>
{
public:

	EditorGizmos();
	~EditorGizmos();

	DWORD AddDecal( gpstring sTexture, SiegePos spos );
	void DeleteDecal( int index );
	void CreateDecalFromGizmo( Gizmo & gizmo );

	void CutDecal();
	void CopyDecal();
	void PasteDecal();
	void Draw();

	void GrabDecalGuid();

	int AddStartingPosition( SiegePos spos );
	void DeleteStartingPosition( int id );

	void SaveStartingPositions( FuelHandle & hMap );
	void LoadStartingPositions( FuelHandle & hMap );

	int GetStartingPositionFromGizmo( int gizmoId );

	void		SetStartingPosition( int id, SiegePos spos );
	SiegePos	GetStartingPosition( int id );

	gpstring GetDefaultStartingGroup();

	StringVec GetStartingGroups();

	gpstring GetStartingGroup( int id );
	
	void AddStartingGroup( gpstring sGroup, gpstring sDescription );
	void EditStartingGroup( gpstring sGroup, gpstring sDescription, bool bDefault, bool bDevOnly, gpstring sScreenName );
	void DeleteStartingGroup( gpstring sGroup );
	void GetStartingGroupProperties( gpstring sGroup, gpstring & sDescription, bool & bDefault, bool & bDevOnly, gpstring & sScreenName );
	void AddStartingGroupWorldLevel( gpstring sGroup, gpstring sWorld, int level );
	void RemoveStartingGroupWorldLevel( gpstring sGroup, gpstring sWorld );
	void RemoveAllStartingGroupWorldLevels( gpstring sGroup );
	void GetStartingGroupWorldLevels( gpstring sGroup, WorldLevels & worldLevels );

	void SetPositionStartGroup( int id, gpstring sGroup );
	void GetPositionStartGroup( int id, gpstring & sGroup );

	void SetSelectedStartGroup( gpstring sGroup ) { m_selectedStartGroup = sGroup; }
	gpstring GetSelectedStartGroup() { return m_selectedStartGroup; }
	
	void SetStartingPosCamera( int id, SiegePos spos, float azimuth, float orbit, float distance );

	void SetSelectedCameraPos();
	void SetCameraToSelectedPos();

	void	SetDecalHorizontal( float horiz )	{ m_decalHoriz = horiz; }
	float	GetDecalHorizontal()				{ return m_decalHoriz; }

	void	SetDecalVertical( float vert )		{ m_decalVert = vert; }
	float	GetDecalVertical()					{ return m_decalVert; }

	void		SetDecalTexture( gpstring sName )	{ m_decalTexture = sName; }
	gpstring	GetDecalTexture()					{ return m_decalTexture; }

	void	LoadDecalsFromGas();
	void	SaveDecalsToGas();

	void	RecalcDecals();

private:

	int GetNextId();

	StartingGroups		m_startGroups;
	float				m_decalHoriz;
	float				m_decalVert;
	gpstring			m_decalTexture;
	bool				m_bCutting;
	std::vector< DWORD > m_clipboard;
	gpstring			m_selectedStartGroup;

};


#define gEditorGizmos EditorGizmos::GetSingleton()


#endif