//////////////////////////////////////////////////////////////////////////////
//
// File     :  EditorLights.h
// Author(s):  Chad Queen
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////


#pragma once
#ifndef __EDITORLIGHTS_H
#define __EDITORLIGHTS_H


// Include Files
#include "gpcore.h"
#include <list>
#include <vector>
#include "siege_pos.h"
#include "siege_database_guid.h"
#include "GizmoManager.h"
#include "siege_light_structs.h"



struct LightClipboardObject
{
	siege::LightDescriptor	descriptor;
	SiegePos				spos;
	SiegePos				sdirection;
	siege::database_guid	source;
};


// Type Definitions
typedef std::list< siege::LightDescriptor* > LightList;
typedef std::vector< LightClipboardObject > LightClipboard;


class EditorLights : public Singleton <EditorLights>
{
public:

	EditorLights();	
	~EditorLights() {};
	
	siege::database_guid AddDefaultPointLight( SiegePos spos );
	siege::database_guid AddDefaultSpotlight( SiegePos spos );
	siege::database_guid AddDefaultDirectionalLight( SiegePos spos );
	siege::database_guid AddLight( siege::database_guid lightGUID, SiegePos spos, SiegePos sdirection, siege::LightDescriptor & ld );

	void DeleteLight( siege::database_guid lightGUID );

	void SetLightPosition		( siege::database_guid lightGUID, SiegePos spos );
	void SetLightColor			( siege::database_guid lightGUID, DWORD color );
	void SetLightIRadius		( siege::database_guid lightGUID, float iradius );
	void SetLightRadius			( siege::database_guid lightGUID, float radius );
	void SetLightIntensity		( siege::database_guid lightGUID, float intensity );
	void SetLightOcclusion		( siege::database_guid lightGUID, bool bOcclude );
	void SetLightShadow			( siege::database_guid lightGUID, bool bDrawShadow );
	void SetLightActive			( siege::database_guid lightGUID, bool bActive );	
	void SetLightAffectActors	( siege::database_guid lightGUID, bool bAffect );	
	void SetLightAffectItems	( siege::database_guid lightGUID, bool bAffect );	
	void SetLightAffectTerrain	( siege::database_guid lightGUID, bool bAffect );
	void SetLightOnTimer		( siege::database_guid lightGUID, bool bOnTimer );	

	void Cut();	
	void Copy();
	void Paste();
	void ClearClipboard();

	void ComputeStaticLighting( bool bOcclude = true );

	void DrawDirectionalLights();
	void DrawLightHuds();

	void ReinsertLightSource( siege::database_guid lightGUID );

	
	bool GetColor( DWORD & dwColor );
	void SetColor( DWORD & dwColor );	
	void SetLightProperties( vector_3 & vDir, float & iradius, float & oradius, float & intensity,
							 bool & bOcclude, bool & bDrawShadows, bool & bActive, bool & bAffectActors,
							 bool & bAffectItems, bool & bAffectTerrain, bool & bOnTimer );
	void GetLightProperties( vector_3 & vDir, float & iradius, float & oradius, float & intensity,
							 bool & bOcclude, bool & bDrawShadows, bool & bActive, bool & bAffectActors,
							 bool & bAffectItems, bool & bAffectTerrain, bool & bOnTimer );

	void SaveLightToGas();
	void LoadLightFromGas();

	void SetLightDirection( siege::database_guid lightGUID, SiegePos sdir );

	void GrabSelectedLightGuid();

private:

	LightClipboard m_clipboard;
	bool		   m_bCutting;

};


#define gEditorLights EditorLights::GetSingleton()


#endif