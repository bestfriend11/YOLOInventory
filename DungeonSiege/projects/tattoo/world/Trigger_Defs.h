#pragma once
#ifndef _TRIGGER_DEFS_H
#define _TRIGGER_DEFS_H

namespace trigger {  // begin namespace trigger


class Condition;
class Action;
class TriggerSys;
class Trigger;
class GroupLink;

struct	Params;
class	Parameter;

typedef std::vector< gpstring >		gpstring_coll;

/////////////////////////////////////////////////////////////////////////////
// Boundary Crossing types

enum eBoundaryCrossType
{
	SET_BEGIN_ENUM( BCROSS_, 0 ),
	BCROSS_ENTER_AT_TIME,
	BCROSS_ENTER_ON_NODE_CHANGE,
	BCROSS_ENTER_FORCED,
	BCROSS_LEAVE_AT_TIME,
	BCROSS_LEAVE_ON_NODE_CHANGE,
	BCROSS_LEAVE_FORCED,
	SET_END_ENUM( BCROSS_ )
};

inline bool IsEnteringAtCrossing(eBoundaryCrossType bct ) 
{
	return	(bct == BCROSS_ENTER_AT_TIME)		 ||
			(bct == BCROSS_ENTER_ON_NODE_CHANGE) ||
			(bct == BCROSS_ENTER_FORCED);
}

inline bool IsForcedCrossing(eBoundaryCrossType bct ) 
{
	return	(bct == BCROSS_LEAVE_FORCED)		 ||
			(bct == BCROSS_ENTER_FORCED);
}

inline bool IsDelayedCrossing(eBoundaryCrossType bct ) 
{
	return	(bct == BCROSS_LEAVE_ON_NODE_CHANGE) ||
			(bct == BCROSS_ENTER_ON_NODE_CHANGE);
}

/////////////////////////////////////////////////////////////////////////////
//	Trigger Unique identifier that uses Guids to track the owner of the trigger
class Truid
{
public:

	Truid()				    : m_OwnGoid(GOID_INVALID), m_TrigNum(0) {};
	Truid(Goid og, WORD tn) : m_OwnGoid(og), m_TrigNum(tn) {};
	Truid(const Truid& s)   : m_OwnGoid(s.m_OwnGoid), m_TrigNum(s.m_TrigNum) {};

	bool Xfer( FuBi::PersistContext& persist );

	Goid  GetOwnGoid() const	{ return m_OwnGoid; };
	WORD GetTrigNum() const		{ return m_TrigNum; };

	bool  IsValid() const		{ return (m_OwnGoid != GOID_INVALID) && (m_TrigNum != 0); };

	friend bool operator< (const Truid& l, const Truid& r)
	{
		return (l.m_OwnGoid < r.m_OwnGoid) || ((l.m_OwnGoid == r.m_OwnGoid) && (l.m_TrigNum < r.m_TrigNum));
	};

	Truid& operator= (const Truid& r)
	{
		m_OwnGoid = r.m_OwnGoid;
		m_TrigNum = r.m_TrigNum;
		return *this;	
	};

	bool operator== (const Truid& r) const
	{
		return (m_OwnGoid == r.m_OwnGoid) && (m_TrigNum == r.m_TrigNum);
	};

	bool operator!= (const Truid& r) const
	{
		return (m_OwnGoid != r.m_OwnGoid) || (m_TrigNum != r.m_TrigNum);
	};

private:
	Goid	m_OwnGoid;
	WORD	m_TrigNum;

};

/////////////////////////////////////////////////////////////////////////////
//	Condition Unique identifier
class Cuid
{
public:

	Cuid()					: m_OwnTrig(Truid()), m_CondNum(0) {};
	Cuid(Truid tg, WORD cn)	: m_OwnTrig(tg), m_CondNum(cn) {};
	Cuid(const Cuid& s)		: m_OwnTrig(s.m_OwnTrig), m_CondNum(s.m_CondNum) {};

	bool Xfer( FuBi::PersistContext& persist );

	Truid  GetOwnTrig() const	{ return m_OwnTrig; };
	WORD   GetCondNum() const	{ return m_CondNum; };

	bool  IsValid() const		{ return m_OwnTrig.IsValid() && (m_CondNum != 0); };

	friend bool operator< (const Cuid& l, const Cuid& r)
	{
		return  (l.m_OwnTrig < r.m_OwnTrig) ||
			   ((l.m_OwnTrig == r.m_OwnTrig) && (l.m_CondNum < r.m_CondNum));
	};

	Cuid& operator= (const Cuid& r)
	{
		m_OwnTrig = r.m_OwnTrig;
		m_CondNum = r.m_CondNum;
		return *this;	
	};

	bool operator== (const Cuid& r) const
	{
		return (m_OwnTrig == r.m_OwnTrig) && (m_CondNum == r.m_CondNum);
	};

	bool operator!= (const Cuid& r) const
	{
		return (m_OwnTrig != r.m_OwnTrig) || (m_CondNum != r.m_CondNum);
	};

private:
	Truid	m_OwnTrig;
	WORD	m_CondNum;

};

/////////////////////////////////////////////////////////////////////////////
//	Trigger Unique identifier that uses Scids to track the owner of the trigger
class Scuid
{
public:

	Scuid()				    : m_OwnScid(SCID_INVALID), m_TrigNum(0) {};
	Scuid(Scid og, WORD tn) : m_OwnScid(og), m_TrigNum(tn) {};
	Scuid(const Scuid& s)   : m_OwnScid(s.m_OwnScid), m_TrigNum(s.m_TrigNum) {};

	bool Xfer( FuBi::PersistContext& persist );

	Scid  GetOwnScid() const	{ return m_OwnScid; };
	WORD GetTrigNum() const		{ return m_TrigNum; };

	bool  IsValid() const		{ return (m_OwnScid != SCID_INVALID) && (m_TrigNum != 0); };

	friend bool operator< (const Scuid& l, const Scuid& r)
	{
		return (l.m_OwnScid < r.m_OwnScid) || ((l.m_OwnScid == r.m_OwnScid) && (l.m_TrigNum < r.m_TrigNum));
	};

	Scuid& operator= (const Scuid& r)
	{
		m_OwnScid = r.m_OwnScid;
		m_TrigNum = r.m_TrigNum;
		return *this;	
	};

	bool operator== (const Scuid& r) const
	{
		return (m_OwnScid == r.m_OwnScid) && (m_TrigNum == r.m_TrigNum);
	};

	bool operator!= (const Scuid& r) const
	{
		return (m_OwnScid != r.m_OwnScid) || (m_TrigNum != r.m_TrigNum);
	};

private:
	Scid	m_OwnScid;
	WORD	m_TrigNum;

};


bool LessThanCondition( const Condition* x, const Condition* y);

} // trigger namespace


FUBI_DECLARE_ENUM_TRAITS( trigger::eBoundaryCrossType, trigger::BCROSS_ENTER_AT_TIME );
const char* ToString( trigger::eBoundaryCrossType e );
bool FromString( const char* str, trigger::eBoundaryCrossType& e );


#endif // _TRIGGER_DEFS_H