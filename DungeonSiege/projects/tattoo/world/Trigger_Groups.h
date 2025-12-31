#pragma once
#ifndef _TRIGGER_GROUPS_H_
#define _TRIGGER_GROUPS_H_

#include "trigger_sys.h"

namespace trigger
{


class GroupLink
{
	// The grouplink structure associates specific conditions of one set of triggers 
	// (the watchers) to another set of triggers (the members)

public:

	typedef stdx::linear_set<Cuid>	WatcherSet;
	typedef WatcherSet::iterator	WatcherIter;

	typedef stdx::linear_set<Truid>	MemberSet;
	typedef MemberSet::iterator		MemberIter;

	GroupLink() : m_ValidGroup(true)	{};

	bool AddWatcher(const Cuid& c);
	bool RemoveWatcher(const Cuid& c);
	WatcherSet* GetWatchers()				{ return &m_Watchers; }
	bool IsWatcher(const Cuid& c) const		{ return m_Watchers.find(c) != m_Watchers.end(); }

	bool AddMember(const Truid& t);
	bool RemoveMember(const Truid& t);
	MemberSet* GetMembers()					{ return &m_Members; }
	bool IsMember(const Truid& t) const		{ return m_Members.find(t) != m_Members.end(); }

	bool IsValidGroup()	const				{ return m_ValidGroup; }
	void SetValidGroup(bool f)				{ m_ValidGroup = f; }

private:

	WatcherSet	m_Watchers;
	MemberSet	m_Members;
	bool		m_ValidGroup;
};

}	// trigger namespace

#endif // _TRIGGER_GROUPS_H_
