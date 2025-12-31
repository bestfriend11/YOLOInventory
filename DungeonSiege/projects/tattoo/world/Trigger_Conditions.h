#pragma once
#ifndef _TRIGGER_CONDITIONS_H
#define _TRIGGER_CONDITIONS_H

#include "trigger_sys.h"
#include "world.h"
#include <vector>
#include <map>

////////////////////////////////////////////////////////////////////////////////////
// External debug function
#if !GP_RETAIL
void DebugDrawBoxStack(const SiegePos& p, matrix_3x3 m, const float s, DWORD c, float d);
#endif
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
//
// Conditions
//
////////////////////////////////////////////////////////////////////////////////////

namespace trigger
{
enum eBoundaryTest 
{
	SET_BEGIN_ENUM( BTEST_, 0 ),
	BTEST_WHILE_INSIDE,
	BTEST_WAIT_FOR_FIRST_MESSAGE,
	BTEST_WAIT_FOR_EVERY_MESSAGE,
	BTEST_ON_EVERY_ENTER,
	BTEST_ON_FIRST_ENTER,
	BTEST_ON_EVERY_FIRST_ENTER,
	BTEST_ON_UNIQUE_ENTER,
	BTEST_ON_EVERY_LEAVE,
	BTEST_ON_FIRST_LEAVE,
	BTEST_ON_LAST_LEAVE,
	BTEST_ON_EVERY_LAST_LEAVE,
	BTEST_ON_FIRST_MESSAGE,
	BTEST_ON_EVERY_MESSAGE,
	SET_END_ENUM( BTEST_ )
};

inline eBoundaryTest ConvertToOneShotBoundaryTest(eBoundaryTest bt)
{
	switch (bt)
	{
	case BTEST_WHILE_INSIDE					: { bt = BTEST_ON_FIRST_ENTER;			break; }		
	case BTEST_ON_EVERY_ENTER				: { bt = BTEST_ON_FIRST_ENTER;			break; }		
	case BTEST_ON_EVERY_FIRST_ENTER			: { bt = BTEST_ON_FIRST_ENTER;			break; }		
	case BTEST_ON_EVERY_LEAVE				: { bt = BTEST_ON_FIRST_LEAVE;			break; }		
	case BTEST_ON_EVERY_LAST_LEAVE			: { bt = BTEST_ON_LAST_LEAVE;			break; }		
	case BTEST_ON_EVERY_MESSAGE				: { bt = BTEST_ON_FIRST_MESSAGE;		break; }		
	case BTEST_WAIT_FOR_EVERY_MESSAGE		: { bt = BTEST_WAIT_FOR_FIRST_MESSAGE;	break; }		
	}
	return bt;
}

inline bool CheckForOneShotBoundaryTest(eBoundaryTest bt)
{
	switch (bt)
	{
	case BTEST_WAIT_FOR_FIRST_MESSAGE:
	case BTEST_ON_FIRST_ENTER:
	case BTEST_ON_UNIQUE_ENTER:
	case BTEST_ON_FIRST_LEAVE:
	case BTEST_ON_LAST_LEAVE:
	case BTEST_ON_FIRST_MESSAGE:
		{
			return true;
		}
	}
	return false;
}

inline eBoundaryTest ConvertToWaitForMessageBoundaryTest(eBoundaryTest bt)
{
	switch (bt)
	{
	case BTEST_WHILE_INSIDE		: { bt = BTEST_WAIT_FOR_EVERY_MESSAGE;	break; }		
	case BTEST_ON_EVERY_ENTER	: { bt = BTEST_WAIT_FOR_EVERY_MESSAGE;	break; }		
	case BTEST_ON_EVERY_LEAVE	: { bt = BTEST_WAIT_FOR_EVERY_MESSAGE;	break; }		
	}
	return bt;
}

inline bool CheckForWaitForMessageBoundaryTest(eBoundaryTest bt)
{
	return 
		(bt == BTEST_WAIT_FOR_FIRST_MESSAGE) ||
		(bt == BTEST_WAIT_FOR_EVERY_MESSAGE) ||
		(bt == BTEST_ON_FIRST_MESSAGE) ||
		(bt == BTEST_ON_EVERY_MESSAGE);
}

inline eBoundaryTest ConvertToWhileInsideBoundaryTest(eBoundaryTest bt)
{
	return (bt == BTEST_ON_EVERY_ENTER) ? BTEST_WHILE_INSIDE : bt;
}

inline bool CheckForWhileInsideBoundaryTest(eBoundaryTest bt)
{
	return (bt == BTEST_WHILE_INSIDE);
}

typedef std::multimap<double,Goid> IllegalCrossingDb;

MAKE_ENUM_MATH_OPERATORS( eBoundaryTest );
// Base Class
class Condition
{
	friend class TriggerSys;

protected:

	WORD		m_wId;
	int			m_iSubGroup;
	Trigger*	m_pTrigger;
	bool		m_bIsReset;		// Flag that indicates when is is safe to change m_CurrentSatisfying set

	gpstring	m_sDoc;

	eBoundaryTest	m_BoundaryTest;
	GoOccupantMap	m_CurrentInsideBoundary;
	GoOccupantMap	m_CurrentSatisfyingOccupants;
	MessageList		m_CurrentSatisfyingMessages;

	SiegePos	m_Center;
	
	Condition() 
		: m_wId(0)
		, m_iSubGroup(0)
		, m_pTrigger(0)
		, m_bIsReset(true)
	{}

	bool EvaluateSatisfaction();

public:

	virtual ~Condition()					{};

	WORD GetID() const						{ return m_wId;			}
	void SetID( WORD id )					{ m_wId = IsMessageHandler() ? id : ((WORD)(id|0x4000)); }

	int  GetSubGroup() const				{ return m_iSubGroup;	}
	void SetSubGroup( int sg )				{ m_iSubGroup = sg;		}

	bool IsSatisfied() const				{ return IsMessageHandler() ? !m_CurrentSatisfyingMessages.empty() : !m_CurrentSatisfyingOccupants.empty(); }

	const gpstring& GetDocs(void) const             { return m_sDoc; }
	void            SetDocs(const gpstring& s)      { m_sDoc.assign(s); }
	void            AppendDocs(const gpstring& s)   { m_sDoc.append(s); }

	bool HasBoundaryOccupants()	const		{ return !m_CurrentInsideBoundary.empty(); }
	bool IsBoundaryOccupant(Goid g) const	{ return m_CurrentInsideBoundary.find(g) != m_CurrentInsideBoundary.end(); }
	int GetBoundaryOccupantCount(Goid g) const
											{
												GoOccupantMapConstIter i = m_CurrentInsideBoundary.find(g);
												return ((i != m_CurrentInsideBoundary.end()) ? (*i).second : 0);
											}
	GoOccupantMap* GetBoundaryOccupants()	{ return &m_CurrentInsideBoundary; }

	bool HasSatisfyingOccupants()	const	{ return !m_CurrentSatisfyingOccupants.empty(); }
	bool IsSatisfyingOccupant(Goid g) const	{ return m_CurrentSatisfyingOccupants.find(g) != m_CurrentSatisfyingOccupants.end(); }
	GoOccupantMap* GetSatisfyingOccupants()	{ return &m_CurrentSatisfyingOccupants; }
	void ClearSatisfyingOccupants()			{
												if ((m_BoundaryTest != BTEST_ON_FIRST_ENTER) &&
													(m_BoundaryTest != BTEST_ON_LAST_LEAVE) &&
													(m_BoundaryTest != BTEST_WHILE_INSIDE) &&
													(m_BoundaryTest != BTEST_WAIT_FOR_FIRST_MESSAGE) &&
												    (m_BoundaryTest != BTEST_WAIT_FOR_EVERY_MESSAGE))
												{
													m_CurrentSatisfyingOccupants.clear();
												}
											}

	bool RemoveOccupant( Goid g )			{ 
												bool wasempty = m_CurrentSatisfyingOccupants.empty();
												m_CurrentInsideBoundary.erase(g);
												m_CurrentSatisfyingOccupants.erase(g);
												m_bIsReset = m_CurrentSatisfyingOccupants.empty();
												return wasempty != m_bIsReset;
											}
		
	bool HasSatisfyingMessages()	const	{ return !m_CurrentSatisfyingMessages.empty();	}
	MessageList* GetSatisfyingMessages()	{ return &m_CurrentSatisfyingMessages;			}
	void ClearSatisfyingMessages()			{ m_CurrentSatisfyingMessages.clear();			}

	virtual const char* GetName() const = 0;
	virtual const char* GetGroupName() const				{ return "!!NOT_A_GROUP_WATCHER!!"; }
	virtual int			GetGroupSize() const				{ return 0; }
	virtual void		SetGroupSize(int)					{ }
	virtual void		SetTrigger(Trigger* t) = 0;
	virtual	bool		IsSpatial() const = 0;
	virtual bool		IsFrustumSpecific() const = 0;
	virtual	bool		IsMessageHandler() const = 0;
	virtual bool        IsGroupWatcher() const				{ return false;	}
	virtual bool        IsPartySnooper() const				{ return false;	}
	virtual bool        IsComplicated() const = 0;

			Trigger*	GetTrigger(void) { return m_pTrigger; }

			bool		IsDormant() 	{ return (!m_bIsReset) && 
											((m_BoundaryTest == BTEST_ON_FIRST_ENTER) ||
											 (m_BoundaryTest == BTEST_ON_FIRST_LEAVE) ||
											 (m_BoundaryTest == BTEST_ON_FIRST_MESSAGE)) &&
											 IsZero(m_pTrigger->GetResetDuration());
										}


	virtual void FetchParameterList(Parameter &paramformat) = 0;
	virtual bool StoreParameterValues(const Params &params) = 0;
	virtual bool FetchParameterValues(Params &params) = 0;

	virtual bool HandleMessage( Trigger& , const WorldMessage& )	{ return true; }
	virtual bool Evaluate()											{ return EvaluateSatisfaction(); }

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos ) = 0;
	virtual bool CollectInsideBoundary( GopColl& in_Occupants ) = 0;
	virtual bool CheckForBoundaryCollision(	const Go*		in_Mover, 
											const SiegePos&	in_PosA, 
											const SiegePos&	in_PosB, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards
											) = 0;


	virtual eBoundaryCrossType GetEnterCrossType() const	{ return BCROSS_ENTER_AT_TIME; }
	virtual eBoundaryCrossType GetLeaveCrossType() const	{ return BCROSS_LEAVE_AT_TIME; }

	bool Reset();
	
	const SiegePos& GetCenter()					{ return m_Center; }

	void OnEnterBoundary(const Goid g);
	void OnLeaveBoundary(const Goid g);

	virtual bool AddOccupantIntoBoundary(const Goid g);
	virtual bool RemoveOccupantFromBoundary(const Goid g, bool isDeactivating, GoOccupantMapIter& outIter);
	virtual void AddIllegalOccupantCrossing(const double&, const Goid)	{}
	virtual bool IsIllegalOccupantCrossing(const double&, const Goid)	{return false;}

	virtual void ConvertToOneShotTest()			{ m_BoundaryTest = ConvertToOneShotBoundaryTest(m_BoundaryTest); }
	virtual bool HasOneShotTest()				{ return CheckForOneShotBoundaryTest(m_BoundaryTest); }

	virtual void ConvertToWhileInsideTest()		{ m_BoundaryTest = ConvertToWhileInsideBoundaryTest(m_BoundaryTest); }
	virtual bool HasWhileInsideTest()			{ return CheckForWhileInsideBoundaryTest(m_BoundaryTest); }

	virtual void ConvertToWaitForMessageTest()	{ m_BoundaryTest = ConvertToWaitForMessageBoundaryTest(m_BoundaryTest); }
	virtual bool HasWaitForMessageTest()		{ return CheckForWaitForMessageBoundaryTest(m_BoundaryTest); }

	virtual bool HasUniqueEnterTest()			{ return m_BoundaryTest == BTEST_ON_UNIQUE_ENTER; }

	virtual bool Xfer( FuBi::PersistContext& persist );

#	if !GP_RETAIL
	virtual bool Validate( const Trigger & trigger, gpstring& out_errmsg ) const;
	virtual SiegePos GetLabelPos() { return m_Center; }
	virtual void Draw(const ConditionDb& /*cond*/ ) {}
#	endif

	SET_NO_COPYING( Condition );

private:
//	bool		m_IsSatisfied;
	
};

// Sorting function required by Condition Set predicate
inline bool LessThanCondition( const Condition* x, const Condition* y)
{
	return x->GetID() < y->GetID();
}

//
// receive_world_message( "event type" );
//
class receive_world_message : public Condition
{
	
protected:
	
	eWorldEvent	m_WorldEventExpected;
	bool		m_WorldEventIsQualified;
	int			m_WorldEventQualifier;

public:

	receive_world_message();

	virtual const char* GetName() const	{ return "receive_world_message"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return false; }
	virtual bool IsFrustumSpecific() const	{ return false; }
	virtual	bool IsMessageHandler()	const	{ return true;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual	bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool HandleMessage( Trigger &trigger, const WorldMessage &message );
	virtual bool Evaluate();

	virtual bool CheckForInsideBoundary( const Go* , const SiegePos& )	{ return false; }
	virtual bool CollectInsideBoundary( GopColl& )						{ return false; }
	virtual bool CheckForBoundaryCollision( const Go*, 
											const SiegePos&, 
											const SiegePos&, 
											float&, 
											float&,
											bool&,
											bool&)						{ return false;	}

	virtual bool AddOccupantIntoBoundary(const Goid /*g*/)		{return true;}
	virtual bool RemoveOccupantFromBoundary(const Goid /*g*/, bool /*isDeactivating*/, GoOccupantMapIter& /*outIter*/) {return true;}

	virtual bool Xfer( FuBi::PersistContext& persist );

#	if !GP_RETAIL
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// party_member_within_node( region, section, level, object );
//
class party_member_within_node : public Condition
{
protected:
	
	int			m_Region;
	int			m_Section;
	int			m_Level;
	int			m_Object;

public:

	party_member_within_node();

	virtual const char* GetName() const	{ return "party_member_within_node"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true; }
	virtual bool IsFrustumSpecific() const	{ return false; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsPartySnooper() const		{ return true;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards);

	virtual eBoundaryCrossType GetEnterCrossType() const	{ return BCROSS_ENTER_ON_NODE_CHANGE; }
	virtual eBoundaryCrossType GetLeaveCrossType() const	{ return BCROSS_LEAVE_ON_NODE_CHANGE; }
	
	virtual bool Xfer( FuBi::PersistContext& persist );
	
#	if !GP_RETAIL
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// party_member_within_sphere( radius );
//
class party_member_within_sphere : public Condition
{
protected:
	
	float		m_Radius;

public:

	party_member_within_sphere();

	virtual const char* GetName() const	{ return "party_member_within_sphere"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true;  }
	virtual bool IsFrustumSpecific() const	{ return true; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsPartySnooper() const		{ return true;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool Xfer( FuBi::PersistContext& persist );

#	if !GP_RETAIL
	virtual SiegePos GetLabelPos();
	virtual void Draw(const ConditionDb& cond);
#	endif
	
};

//
// party_member_within_bounding_box
//
class party_member_within_bounding_box : public Condition
{
protected:

	vector_3	m_HalfDiag;
	matrix_3x3	m_InvOrient;

public:

	party_member_within_bounding_box(); 

	virtual const char* GetName() const	{ return "party_member_within_bounding_box"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true;  }
	virtual bool IsFrustumSpecific() const	{ return true; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsPartySnooper() const		{ return true;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool Xfer( FuBi::PersistContext& persist );

#	if !GP_RETAIL
	virtual SiegePos GetLabelPos();
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// actor_within_sphere
//
class actor_within_sphere : public Condition
{
protected:
	float m_Radius;

public:

	actor_within_sphere();

	virtual const char* GetName() const	{ return "actor_within_sphere"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true; }
	virtual bool IsFrustumSpecific() const	{ return true; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool Xfer( FuBi::PersistContext& persist );

#	if !GP_RETAIL
	virtual SiegePos GetLabelPos();
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// actor_within_bounding_box
//
class actor_within_bounding_box : public Condition
{
protected:

	vector_3	m_HalfDiag;
	matrix_3x3	m_InvOrient;

public:

	actor_within_bounding_box(); 

	virtual const char* GetName() const	{ return "actor_within_bounding_box"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true; }
	virtual bool IsFrustumSpecific() const	{ return true; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool Xfer( FuBi::PersistContext& persist );

#	if !GP_RETAIL
	virtual SiegePos GetLabelPos();
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// go_within_sphere
//
class go_within_sphere : public Condition
{
protected:
	Scid		m_Scid;
	gpstring	m_TemplateNameFilter;
	float		m_Radius;
	double		m_UpdateTimeout;

public:

	go_within_sphere();

	virtual const char* GetName() const	{ return "go_within_sphere"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true; }
	virtual bool IsFrustumSpecific() const	{ return true; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool Evaluate();

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool Xfer( FuBi::PersistContext& persist );
	
#	if !GP_RETAIL
	virtual SiegePos GetLabelPos();
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// go_within_bounding_box
//
class go_within_bounding_box : public Condition
{
protected:
	Scid		m_Scid;
	gpstring	m_TemplateNameFilter;
	vector_3	m_HalfDiag;
	matrix_3x3	m_InvOrient;
	double		m_UpdateTimeout;
	
public:

	go_within_bounding_box();

	virtual const char* GetName() const	{ return "go_within_bounding_box"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return true; }
	virtual bool IsFrustumSpecific() const	{ return true; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool Evaluate();

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool Xfer( FuBi::PersistContext& persist );
	
#	if !GP_RETAIL
	virtual SiegePos GetLabelPos();
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// has_go_in_inventory
//
class has_go_in_inventory : public Condition
{
protected:
	bool		m_LocalInventory;
	Scid		m_Scid;
	gpstring	m_TemplateNameFilter;

public:

	has_go_in_inventory(); 

	virtual const char* GetName() const	{ return "has_go_in_inventory"; }
	virtual void SetTrigger(Trigger* t);
	virtual bool IsSpatial() const			{ return false; }
	virtual bool IsFrustumSpecific() const	{ return false; }
	virtual	bool IsMessageHandler()	const	{ return false;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

			void SearchInventoryHelper( const GopColl& partyColl, const gpstring& filter, const Scid& scid );
	virtual bool Evaluate();
	
	virtual bool CheckForInsideBoundary( const Go* , const SiegePos& )	{ return false; }
	virtual bool CollectInsideBoundary( GopColl& )						{ return false; }
	virtual bool CheckForBoundaryCollision( const Go*, 
											const SiegePos&, 
											const SiegePos&, 
											float&, 
											float&,
											bool&,
											bool&)						{ return false;	}

	virtual bool AddOccupantIntoBoundary(const Goid /*g*/)		{return true;}
	virtual bool RemoveOccupantFromBoundary(const Goid /*g*/, bool /*isDeactivating*/, GoOccupantMapIter& /*outIter*/) {return true;}

	virtual bool Xfer( FuBi::PersistContext& persist );
	
#	if !GP_RETAIL
	virtual void Draw(const ConditionDb& cond);
#	endif

};

//
// party_member_within_trigger_group
//
class party_member_within_trigger_group : public Condition
{
protected:
	gpstring m_GroupName;
	int		 m_GroupSize;
	IllegalCrossingDb m_Illegals;

public:

	party_member_within_trigger_group();
	virtual ~party_member_within_trigger_group();
	
	virtual const char* GetName() const			{ return "party_member_within_trigger_group"; }
	virtual const char* GetGroupName() const	{ return m_GroupName.c_str(); }
	virtual int GetGroupSize() const			{ return m_GroupSize; }
	virtual void SetGroupSize(int sz)			{ m_GroupSize = sz; }
	virtual void SetTrigger(Trigger*);
	virtual bool IsSpatial() const			{ return false; }
	virtual bool IsFrustumSpecific() const	{ return false; }
	virtual	bool IsMessageHandler() const	{ return false;	}
	virtual bool IsGroupWatcher() const		{ return true;	}
	virtual bool IsPartySnooper() const		{ return true;	}
	virtual bool IsComplicated() const;

	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
	virtual bool FetchParameterValues( Params &params );

	virtual bool CheckForInsideBoundary( const Go* in_Mover, const SiegePos& in_Pos );
	virtual bool CollectInsideBoundary( GopColl& in_Occupants );
	virtual bool CheckForBoundaryCollision( const Go*		in_Mover, 
											const SiegePos&	in_Pos, 
											const SiegePos&	in_Dir, 
											float&			out_tEnter, 
											float&			out_tLeave,
											bool&			out_AlreadyInside,
											bool&			out_OutsideAfterwards );

	virtual bool AddOccupantIntoBoundary(const Goid g);
	virtual bool RemoveOccupantFromBoundary(const Goid g, bool isDeactivating, GoOccupantMapIter& outIter);
	
	virtual void AddIllegalOccupantCrossing(const double& t, const Goid g);
	virtual bool IsIllegalOccupantCrossing(const double& t, const Goid g);

	// These are a special class of conditions, don't monkey with the boundary tests!
	virtual void ConvertToOneShotTest()			{}
	virtual bool HasOneShotTest()				{ return true; }

	virtual void ConvertToWaitForMessageTest()	{}
	virtual bool HasWaitForMessageTest()		{ return true; }

	virtual bool Xfer( FuBi::PersistContext& persist );
	
#	if !GP_RETAIL
	virtual void Draw(const ConditionDb& cond);
#	endif

private:
	
};

class party_member_entered_trigger_group : public party_member_within_trigger_group
{
public:
	party_member_entered_trigger_group();
	virtual const char* GetName() const	{ return "party_member_entered_trigger_group"; }
	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
};

class party_member_left_trigger_group : public party_member_within_trigger_group
{
public:
	party_member_left_trigger_group();
	virtual const char* GetName() const	{ return "party_member_left_trigger_group"; }
	virtual void FetchParameterList(Parameter &parameterlist);
	virtual bool StoreParameterValues(const Params &params);
};
	
}	// namespace trigger

#endif //_TRIGGER_CONDITIONS_H