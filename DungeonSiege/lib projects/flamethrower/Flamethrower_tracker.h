#pragma once
#ifndef _FLAMETHROWER_TRACKER_H_
#define _FLAMETHROWER_TRACKER_H_


#include <vector>

#include "vector_3.h"
#include "matrix_3x3.h"
#include "godefs.h"
#include "siege_pos.h"
#include "siege_database_guid.h"
#include "BucketVector.h"


class Flamethrower_target
{
public:

	enum eTargetType
	{
		SET_BEGIN_ENUM( FXTT_, 0 ), 

		DEFAULT,
		STATIC,
		EFFECT,
		EMITTER,
		INVALID,

		SET_END_ENUM( FXTT_ ), 
	};

	enum eOrientMode
	{
		ORIENT_GO		=	0,
		ORIENT_BONE		=	1,
		ORIENT_WORLD	=	2
	};


	Flamethrower_target				( eTargetType type ) : m_Type( type ), m_BoundingRadius(1.0f), m_ID(0) {}
	virtual	~Flamethrower_target	( void ) {}

	static Flamethrower_target*
							CreateByType			( eTargetType type, const SiegePos &position = SiegePos::INVALID, UINT32 id = 0 );

	bool					Xfer					( FuBi::PersistContext &persist );
	virtual bool			ShouldPersist			( bool& hasGoid ) const						{  hasGoid = false;  return ( false );  }

	bool					Duplicate				( Flamethrower_target **pTarget );
	virtual bool			OnDuplicate				( Flamethrower_target **pTarget ) = 0;


	eTargetType				GetType					( void ) const								{ return m_Type; }

	UINT32					GetID					( void ) const								{ return m_ID; }
	void					SetID					( UINT32 id )								{ m_ID = id; }

	virtual bool			GetPlacement			( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode = ORIENT_GO, const gpstring &sBoneName = gpstring::EMPTY );

	virtual bool			GetDirection			( vector_3 &direction );
	virtual void			SetDirection			( const vector_3 &direction )				{ m_Direction = direction; }

	virtual bool			GetBoundingRadius		( float &radius );
	virtual void			SetBoundingRadius		( const float &radius )						{ m_BoundingRadius = radius; }

	virtual void			SetPositionOffsetName	( const gpstring &/*sName*/ )				{}
	virtual bool			HasPositionOffsetName	( const gpstring &sName );

	virtual bool			GetPositionOffset		( vector_3 &offset );
	virtual void			SetPositionOffset		( const vector_3 &offset )					{ m_Offset = offset; }

	virtual bool			IsValid					( bool* /*shouldDelete*/ = NULL )			{ return true; }

	virtual bool			GetInterestOnly			( void )									{ return false; }

	virtual bool			GetIsInsideInventory	( void ) = 0;


protected:
	Flamethrower_target								( void ) : m_Type(INVALID), m_ID(0), m_BoundingRadius(1.0f) {}

	virtual	bool			OnXfer					( FuBi::PersistContext &persist );

	eTargetType				m_Type;
	UINT32					m_ID;
	SiegePos				m_Position;
	matrix_3x3				m_Orientation;
	vector_3				m_Direction;
	float					m_BoundingRadius;
	vector_3				m_Offset;
};




/*=======================================================================================

  TattooTracker

  purpose:	Storage container for the various target types used by Flamethrower

  author:	Rick Saenz

  (C)opyright Gas Powered Games 1999

---------------------------------------------------------------------------------------*/
class TattooTracker
{
public:

	typedef UnsafeBucketVector <my Flamethrower_target *> TargetColl;

	enum TARGET_INDEX
	{
		INVALID = 0,
		TARGET,
		SOURCE,
		REMAINING
	};

	// Existence
	TattooTracker( void );
	~TattooTracker( void );

	TattooTracker
				*Duplicate				( void );


	// Convert to TattooTracker to string for network transport
#	if !GP_RETAIL
	gpstring	ToString				( void ) const;
#	endif // !GP_RETAIL

	// Target addition helpers
	void		AddGoTarget				( Goid target_goguid, const char* boneName = NULL );
	void		AddGoTarget				( Goid target_goguid, int boneIndex );
	void		AddGoEmitter			( Goid target_goguid );
	void		AddStaticTarget			( const SiegePos &position, const vector_3 &direction, float radius );

	bool		HasInterestOnlyTarget	( void );


	// Persistence
	void		PrepareForSave			( void );

	bool		Xfer					( FuBi::PersistContext& persist );
	bool		Xfer					( FuBi::BitPacker& packer );
	bool		ShouldPersist			( void ) const;

	// Implementation
	//
	bool		AtLeastOneTarget		( void );
	UINT32		GetTargetCount			( void )			{	return ( m_Targets.size() ); }

	// Put all the positions in a container
	stdx::fast_vector< SiegePos >
				GetAllPositions			( void );

	// Get the placement of target n
	bool		GetPlacement			( SiegePos *pPos, matrix_3x3 *pOrient, UINT32 orient_mode = Flamethrower_target::ORIENT_GO, TARGET_INDEX n = TARGET );
	
	// Get the orientation of target n
	bool		GetDirection			( vector_3 &dir, TARGET_INDEX n = TARGET );
	// Get the bouding radius of target n
	bool		GetBoundingRadius		( float &r, TARGET_INDEX n = TARGET );

	// Get the id of a Target
	Goid		GetID					( TARGET_INDEX n = TARGET ) const;
	// Get the target type
	Flamethrower_target::eTargetType
				GetType					( TARGET_INDEX n = TARGET ) const;

	// Remove all targets
	void		RemoveAllTargets		( void );

	// Add a default target
	void		AddTarget				( Flamethrower_target *pTarget );
	// Set a target - Calls AddTarget if n > number of targets
	void		SetTarget				( Flamethrower_target *pTarget, TARGET_INDEX n );
	// Get a target
	bool		GetTarget				( TARGET_INDEX n, Flamethrower_target **pTarget );
	TargetColl::iterator
				GetTargetsBegin			( void )			{  return ( m_Targets.begin() );  }
	TargetColl::iterator
				GetTargetsEnd			( void )			{  return ( m_Targets.end() );  }

	// Set the name of piece offset for positional offset reference later ( bone )
	bool		SetPositionOffsetName	( const gpstring &sPieceName, TARGET_INDEX n );
	// Set an offset to adjust position by using target orientation
	bool		SetPositionOffset		( const vector_3 &offset, TARGET_INDEX n );
	// Returns true if all targets are valid
	bool		IsValid					( bool* shouldDelete = NULL );
	// Cleans out any targets that aren't valid
	void		ClearInvalidTargets		( void );

	// Checks to see if the target actually has the specified bone name
	bool		HasPositionOffsetName	( const gpstring &sName );

private:

	TargetColl m_Targets;

	SET_NO_COPYING( TattooTracker );
};



#endif //_FLAMETHROWER_TRACKER_H_
