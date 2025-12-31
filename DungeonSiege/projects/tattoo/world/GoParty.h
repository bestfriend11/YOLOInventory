//////////////////////////////////////////////////////////////////////////////
//
// File     :  GoParty.h
// Author(s):  Bartosz Kijanka, Scott Bilas
//
// Summary  :  Contains the GoParty go component.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __GOPARTY_H
#define __GOPARTY_H

//////////////////////////////////////////////////////////////////////////////

#include "Go.h"
#include "quat.h"

//////////////////////////////////////////////////////////////////////////////

class Formation;
class FormSpot;
enum eQPlace;

//////////////////////////////////////////////////////////////////////////////
// class GoParty declaration

class GoParty : public GoComponent
{

public:

	SET_INHERITED( GoParty, GoComponent );

// Setup.

	// ctor/dtor
	GoParty( Go* go );
	GoParty( const GoParty& source, Go* newGo );
	virtual ~GoParty( void );

	void TakeFrustum( FrustumId frustumId );

// Interface.

	void AddMemberDeferred( Scid scid );
FEX	Formation * GetFormation( void )			{ return m_pFormation; }
	void GetMembersAccordingToRank( GopColl & out );

FEX void RSAddMemberNow   ( Go* member );
FEX void RCAddMemberNow   ( Go* member );
FEX void AddMemberNow     ( Go* member );
FEX void RSRemoveMemberNow( Go* member );
FEX void RCRemoveMemberNow( Go* member );
FEX void RemoveMemberNow  ( Go* member );

FEX int GetWaitingToJoinCount() { return (int)m_WaitingToJoin.size(); }

// Overrides.

	////////////////////////////////////////
	// required overrides


	virtual int          GetCacheIndex( void );
	virtual GoComponent* Clone        ( Go* newGo );
	virtual bool         Xfer         ( FuBi::PersistContext& persist );

	////////////////////////////////////////
	// event handlers

	virtual void HandleMessage  ( const WorldMessage& msg );
	virtual void HandleCcMessage( const WorldMessage& msg );
	virtual void Update         ( float timeDelta )		{  PrivateUpdate( timeDelta, true );  }
	virtual void UpdateSpecial  ( float deltaTime );
	virtual void LinkChild      ( Go* child );
	virtual void UnlinkChild    ( Go* child );
	virtual void DetectWantsBits( void )				{ SetWantsUpdates();  SetWantsDraws(); }

	////////////////////////////////////////
	//	for the editor

#	if !GP_RETAIL
	virtual void DrawDebugHud       ( void );
	virtual bool ShouldSaveInternals( void );
	virtual void SaveInternals      ( FuelHandle fuel );
#	endif // !GP_RETAIL

	////////////////////////////////////////
	//	misc queries

	float GetMaxEquppedRangedWeaponRange();
	DWORD GetNumRangedFighters()	{ return( m_NumRangedFighters ); }

	////////////////////////////////////////
	//	temp variables - for skrit users
	//
	//	IMPORTANT: temp variables are not preserved across Update() calls
	//

FEX GopColl & GetTempGopCollA()		{ return( m_TempGopCollA ); }
FEX GopColl & GetTempGopCollB()		{ return( m_TempGopCollB ); }
FEX GopColl & GetTempGopCollC()		{ return( m_TempGopCollC );	}

FEX QtColl & GetTempQtColl1()		{ return( m_TempQtColl1  ); }

	////////////////////////////////////////
	//	debug

	GPDEBUG_ONLY( virtual bool AssertValid( void ) const; )


private:

	void PrivateCheckInWorld( void );
	void PrivateUpdate      ( float timeDelta, bool updateLight );

	void CheckMemberToAdd( Go* potentialMember );

	static DWORD    GoPartyToNet( GoParty* x );
	static GoParty* NetToGoParty( DWORD d, FuBiCookie* cookie );

	typedef std::vector< Scid > ScidColl;

	////////////////////////////////////////////////////////////////////////////////
	//	data

	////////////////////////////////////////
	//	light

	float m_HotRadius;				// radius around focus character to pull into hot group
	float m_InnerLightRadius;
	float m_OuterLightRadius;
	float m_LightVerticalOffset;
	DWORD m_LightColor;
	siege::database_guid m_lightId;

	////////////////////////////////////////
	//	party

	float m_MaxEquppedRangedWeaponRange;
	DWORD m_NumRangedFighters;
	double m_NextUpdateTime;
	my Formation * m_pFormation;
	ScidColl m_WaitingToJoin;
	Goid m_CurrentFocus;
	bool m_bUpdateShadows;

	////////////////////////////////////////
	//	temp

	GopColl		m_TempGopCollA;
	GopColl		m_TempGopCollB;
	GopColl		m_TempGopCollC;

	QtColl		m_TempQtColl1;

	FUBI_RPC_CLASS( GoParty, GoPartyToNet, NetToGoParty, "" );
	SET_NO_COPYING( GoParty );
};




/*===========================================================================

	Class		: FormSpot

	Author(s)	: Bartosz Kijanka

	Purpose		: This class represents the idea of a "spot" in a formation.
				  A "spot" has an occupant, and has a relative position
				  within the formation.  The position is relative to the
				  origin of the formation, and is addressed by discrete
				  coortinates ( W, L )  Width, Length.  The coordinate system
				  parallels standard cartesian coordinates.

---------------------------------------------------------------------------*/
class FormSpot
{

public:

	FormSpot( Formation * formation )
		: m_W( 0 )
		, m_L( 0 )
		, m_DecalId( 0 )
		, m_Occupant( GOID_INVALID )
		, m_Dirty( true )
		, m_IsReserved( false )
		, m_IsFree( true )
		, m_IsLeader( false )
		, m_IsTerrainBlocked( false )
		, m_IsAlternate( false )
		, m_Position( SiegePos::INVALID )
		, m_Orientation( matrix_3x3::ZERO_INIT )
		, m_Formation( formation )
	{
	}

	~FormSpot()
	{
		RemoveDecal();
	}

	////////////////////////////////////////
	//	logical formation spot location

	int GetW()													{ return m_W; }
	void SetW( int w )											{ m_W = w; }

	int GetL()													{ return m_L; }
	void SetL( int l )											{ m_L = l; }

	Goid GetOccupant()											{ return m_Occupant; }
	void SetOccupant( Goid occ )								{ SetDirty(); m_Occupant = occ; }

	bool IsLeader()												{ return m_IsLeader; }
	bool IsTerrainBlocked()										{ return m_IsTerrainBlocked; }
	bool IsAlternate()											{ return m_IsAlternate; }

	void SetIsReserved( bool flag )								{ SetDirty(); m_IsReserved = flag; }
	void SetIsFree( bool flag )									{ SetDirty(); m_IsFree = flag; }
	void SetIsLeader( bool flag )								{ SetDirty(); m_IsLeader = flag; }
	void SetIsTerrainBlocked( bool flag )						{ SetDirty(); m_IsTerrainBlocked = flag; }
	void SetIsAlternate( bool flag )							{ SetDirty(); m_IsAlternate = flag; }

	bool IsOccupantInSpot();

	////////////////////////////////////////
	//	actual location of spot in world

	SiegePos const & GetPosition()								{ return m_Position; }
	void SetPos( SiegePos const & pos )							{ SetDirty(); m_Position = pos; }

	matrix_3x3 const & GetOrientation()							{ return m_Orientation; }
	void SetOrientation( matrix_3x3 const & orient )			{ SetDirty(); m_Orientation = orient; }

	void Draw();

	void AddDecal();											// palce decal in world
	void RemoveDecal();											// remove decal if one was added


private:

	void SetDirty() { m_Dirty = true; }		// mark as needing to replace decal
	
	////////////////////////////////////////
	//	data

	Formation * m_Formation;

	int			m_W;
	int			m_L;
	Goid		m_Occupant;

	bool		m_Dirty					: 1;

	bool		m_IsReserved			: 1;
	bool		m_IsFree				: 1;
	bool		m_IsLeader				: 1;
	bool		m_IsTerrainBlocked		: 1;
	bool		m_IsAlternate			: 1;

	SiegePos	m_Position;
	matrix_3x3	m_Orientation;

	DWORD		m_DecalId;
};


typedef std::vector< FormSpot * > FormSpotColl;


/*===========================================================================

	Class		: Formation

	Author(s)	: Bartosz Kijanka

	Purpose		: This class represents the primitive concept of a formation.
								
---------------------------------------------------------------------------*/
class Formation
{

public:

	Formation( GoParty * party );
	~Formation();

	bool Xfer( FuBi::PersistContext& persist );

	void Update();

	bool Validate();

	GoParty * GetParty()											{ return m_pParty; }

	////////////////////////////////////////
	//	formation geometry

	gpstring const & GetShape()										{ return m_Name; }
FEX	void SetShape( gpstring const & name );
	void SetNextShape();

FEX SiegePos const & GetPosition()									{ return m_Position; }
FEX	void SetPosition( SiegePos const & pos );

FEX bool Move( eQPlace place, eActionOrigin origin, bool followMode = false, bool bPreserveGuarding = false );

FEX vector_3 const & GetDirection()									{ return m_Direction; }
FEX	void SetDirection( vector_3 const & dir );

FEX vector_3 const & GetMemberDirection()							{ return m_MemberDirection; }
FEX void SetMemberDirection( vector_3 const & dir );

	float GetSpotSpacing()											{ return m_SpotSpacing; }
	void SetSpotSpacing( float spacing );

	float GetMinSpotSpacing()										{ return m_MinSpotSpacing; }
	float GetMaxSpotSpacing()										{ return m_MaxSpotSpacing; }

FEX bool Rotate( float rad );
FEX bool RotateMembers( float rad );
FEX bool Scale( float scale );	

	////////////////////////////////////////
	//	draw options

	void HideSpots();

	bool GetDrawAssignedSpots()										{ return m_DrawAssignedSpots; }
	void SetDrawAssignedSpots( bool flag )							{ m_DrawAssignedSpots = flag; }

	bool GetDrawFreeSpots()											{ return m_DrawFreeSpots; }
	void SetDrawFreeSpots( bool flag )								{ m_DrawFreeSpots = flag; }	

	bool CanOccupySpot( int w, int l );
	FormSpot * SetSpotOccupant( Goid occupant, int w, int l );
	bool CalcSpotPos( const float w, const float l, SiegePos & pos );
	bool CalcSpotPoints( const float w, const float l, SiegePos & center, SiegePos & tl, SiegePos & tr, SiegePos & br, SiegePos & bl );

	bool FindSpot( int w, int l, FormSpot * & spot );
	bool FindSpot( Goid occupant, FormSpot * & spot, bool findAlternate = false );

	////////////////////////////////////////////
	//	decals
		
	gpstring const & GetNormalSpotDecalFilename()					{ return( m_ReservedSpotDecalFilename ); }
	gpstring const & GetFreeSpotDecalFilename()						{ return( m_FreeSpotDecalFilename ); }
	gpstring const & GetLeaderSpotDecalFilename()					{ return( m_LeaderSpotDecalFilename ); }
	gpstring const & GetTerrainBlockedSpotDecalFilename()			{ return( m_TerrainBlockedSpotDecalFilename ); }

	////////////////////////////////////////////
	//	util

	bool HasOccupant( FormSpotColl const & spots, Goid occ );

	static DWORD MAX_FORMATION_MEMBERS;

	void SetDirty( bool bDirty = true ) { m_Dirty = bDirty; }


private:
	typedef std::pair< int, int > Coordinate;
	typedef std::map< int, Coordinate > PriorityToCoordinate;

	void UpdateSpots();
	void UpdateSpotsUtil();
	bool AllOccupiedSpotsBlocked();
	void ValidateAdjustFormation();

	void SetShapeSingleSpot( Go * member );	

	void Clear();

	bool FindNextFreeSpot( int & w, int & l );
	//void DeleteUnoccupiedSpots();

	////////////////////////////////////////////
	// data

	gpstring m_Name;

	gpstring m_ReservedSpotDecalFilename;
	gpstring m_FreeSpotDecalFilename;
	gpstring m_LeaderSpotDecalFilename;
	gpstring m_TerrainBlockedSpotDecalFilename;

	SiegePos m_Position;
	vector_3 m_Direction;
	vector_3 m_MemberDirection;

	float m_SpotSpacing;
	float m_MinSpotSpacing;
	float m_MaxSpotSpacing;

	float m_SpotWidth;
	float m_SpotLength;

	float m_CenterW;
	float m_CenterL;

	FormSpotColl m_Spots;

	int m_LastIntersectsW;
	int m_LastIntersectsL;

	int m_Width;
	int m_Length;

	int m_MaxWidth;
	int m_MaxLength;

	int m_LeaderW;
	int m_LeaderL;

	GoParty * m_pParty;

	bool m_DrawAssignedSpots		: 1;
	bool m_DrawFreeSpots			: 1;
	bool m_Dirty					: 1;



	SET_NO_COPYING( Formation );
};


//////////////////////////////////////////////////////////////////////////////

#endif  // __GOPARTY_H

//////////////////////////////////////////////////////////////////////////////
