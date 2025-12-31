#include "precomp_world.h"

#include "Trigger_Sys.h"
#include "Trigger_Groups.h"
#include "Trigger_Conditions.h"


using namespace trigger;

///////////////////////////////////////////////////////////
bool GroupLink::AddWatcher(const Cuid& c)
{
	bool ret = false;

	gpassert(c != Cuid());
	WatcherIter it = m_Watchers.find(c);
	gpassert(it == m_Watchers.end());
	if (it == m_Watchers.end())
	{
		Condition* watchcond;
		if (gTriggerSys.FetchConditionByCuid(c,watchcond))
		{
			// Add the new watcher to all the member triggers
			MemberIter mit = m_Members.begin();
			for (;mit != m_Members.end(); ++mit)
			{
				Trigger* memtrig;
				if (gTriggerSys.FetchTriggerByTruid(*mit,memtrig))
				{
					if (memtrig->AddWatchedByCondition(watchcond))
					{
						ret = ret && true;
					}
					else
					{
						Trigger* watchtrig = watchcond->GetTrigger();
						
						const char *wn = (watchtrig && watchtrig->GetOwner()) ? watchtrig->GetOwner()->GetTemplateName() : "UNKNOWN_WATCHER";
						int ws = (watchtrig && watchtrig->GetOwner()) ? MakeInt(watchtrig->GetOwner()->GetScid()) : -1;
						int wt = (watchtrig && watchtrig->GetOwner()) ? watchtrig->GetTruid().GetTrigNum() : -1;
						int wc = watchcond ? watchcond->GetID() : -1;
						
						const char* wgn = (watchcond) ? watchcond->GetGroupName() : "UNKNOWN_GROUP";

						const char *mn = (memtrig && memtrig->GetOwner()) ? memtrig->GetOwner()->GetTemplateName() : "UNKNOWN_MEMBER";
						int ms = (memtrig && memtrig->GetOwner()) ? MakeInt(memtrig->GetOwner()->GetScid()) : -1;
						int mt = (memtrig && memtrig->GetOwner()) ? memtrig->GetTruid().GetTrigNum() : -1;

						const char* mgn = (memtrig) ? memtrig->GetGroupName() : "UNKNOWN_GROUP";

						gperrorf(("Trigger Groups - AddWatcher::Attempted to add duplicate reference to watched_by_condition defined"
							" in [%s:0x%08x:%04x:%04x] watching group [%s] "
							"to member trigger [%s:0x%08x:%04x] of group [%s]"
								  	,wn
									,ws
									,wt
									,wc
									,wgn
								  	,mn
									,ms
									,mt
									,mgn
									));
						ret = false;
					}
				}
				else
				{
					Trigger* watchtrig = watchcond->GetTrigger();
					
					const char *wn = (watchtrig && watchtrig->GetOwner()) ? watchtrig->GetOwner()->GetTemplateName() : "UNKNOWN_WATCHER";
					int ws = (watchtrig && watchtrig->GetOwner()) ? MakeInt(watchtrig->GetOwner()->GetScid()) : -1;
					int wt = (watchtrig && watchtrig->GetOwner()) ? watchtrig->GetTruid().GetTrigNum() : -1;
					int wc = watchcond ? watchcond->GetID() : -1;

					const char* wgn = watchcond ? watchcond->GetGroupName() : "UNKNOWN_GROUP";

					gperrorf(("Trigger Groups - AddWatcher::Failed to locate member trigger for watcher "
						    "[%s:0x%08x:%04x:%04x]"
							" of group [%s]"
								,wn
								,ws
								,wt
								,wc
								,wgn
								));

					ret = false;
				}
			}		
			watchcond->SetGroupSize(m_Members.size());
		}
		m_Watchers.insert(c);
	}
	else
	{
		gperrorf(("Trigger Groups - Attempted to add to duplicate watcher to group "));
		ret = false;
	}
	return ret;
}

///////////////////////////////////////////////////////////
bool GroupLink::RemoveWatcher(const Cuid& c)
{
	bool ret = true;

	gpassert(c != Cuid());
	WatcherIter it = m_Watchers.find(c);
	gpassert(it != m_Watchers.end());
	if (it != m_Watchers.end())
	{
		// Remove the old watcher from all the member triggers
		Condition* watchcond;
		if (gTriggerSys.FetchConditionByCuid(c,watchcond))
		{
			// Add the new watcher to all the member triggers
			MemberIter mit = m_Members.begin();
			for (;mit != m_Members.end(); ++mit)
			{
				Trigger* memtrig;
				if (gTriggerSys.FetchTriggerByTruid(*mit,memtrig))
				{
					ret &= memtrig->RemoveWatchedByCondition(watchcond);
				}
			}		
			watchcond->SetGroupSize(0);
		}
		m_Watchers.erase(it);
	}
	else
	{
		gperrorf(("Trigger Groups - RemoveWatcher::Attempted to remove invalid watcher from group "));
		ret = false;
	}
	return ret;
}

///////////////////////////////////////////////////////////
bool GroupLink::AddMember(const Truid& t)
{
	bool ret = false;

	gpassert(t != Truid());
	MemberIter mit = m_Members.find(t);
	gpassert(mit == m_Members.end());
	if (mit == m_Members.end())
	{
		// Add all the watchers to the new member trigger
		Trigger* memtrig;	
		if (gTriggerSys.FetchTriggerByTruid(t,memtrig))
		{
			WatcherIter wit = m_Watchers.begin();
			for (;wit != m_Watchers.end(); ++wit)
			{
				Condition* watchcond;
				if (gTriggerSys.FetchConditionByCuid((*wit),watchcond))
				{
					if (memtrig->AddWatchedByCondition(watchcond))
					{
						watchcond->SetGroupSize(m_Members.size()+1);
						ret = ret && true;
					}
					else
					{

						Trigger* watchtrig = watchcond->GetTrigger();
						
						const char *wn = (watchtrig && watchtrig->GetOwner()) ? watchtrig->GetOwner()->GetTemplateName() : "UNKNOWN_WATCHER";
						int ws = (watchtrig && watchtrig->GetOwner()) ? MakeInt(watchtrig->GetOwner()->GetScid()) : -1;
						int wt = (watchtrig && watchtrig->GetOwner()) ? watchtrig->GetTruid().GetTrigNum() : -1;
						int wc = watchcond ? watchcond->GetID() : -1;
						
						const char* wgn = (watchcond) ? watchcond->GetGroupName() : "UNKNOWN_GROUP";

						const char *mn = (memtrig && memtrig->GetOwner()) ? memtrig->GetOwner()->GetTemplateName() : "UNKNOWN_MEMBER";
						int ms = (memtrig && memtrig->GetOwner()) ? MakeInt(memtrig->GetOwner()->GetScid()) : -1;
						int mt = (memtrig && memtrig->GetOwner()) ? memtrig->GetTruid().GetTrigNum() : -1;

						const char* mgn = (memtrig) ? memtrig->GetGroupName() : "UNKNOWN_GROUP";

						gperrorf(("Trigger Groups - AddMember::Attempted to add duplicate reference to watched_by_condition "
							"[%s:0x%08x:%04x:%04x] (watching group [%s]) "
							"to member trigger [%s:0x%08x:%04x] (of group [%s]) "
								  	,wn
									,ws
									,wt
									,wc
									,wgn
								  	,mn
									,ms
									,mt
									,mgn
									));
						ret = false;
					}
				}
				else
				{
					const char *mn = (memtrig && memtrig->GetOwner()) ? memtrig->GetOwner()->GetTemplateName() : "UNKNOWN_MEMBER";
					int ms = (memtrig && memtrig->GetOwner()) ? MakeInt(memtrig->GetOwner()->GetScid()) : -1;
					int mt = (memtrig && memtrig->GetOwner()) ? memtrig->GetTruid().GetTrigNum() : -1;

					const char* mgn = (memtrig) ? memtrig->GetGroupName() : "UNKNOWN_GROUP";

					gperrorf(("Trigger Groups - AddWatcher::Failed to locate watched_by_condition of "
							"group [%s] for new member "
						    "[%s:0x%08x:%04x:%04x]"
								,mgn
								,mn
								,ms
								,mt
								));

					ret = false;
				}

			}
		
		}

		m_Members.insert(t);

	}
	else
	{
		gperrorf(("Trigger Groups - AddMember::Attempted to add invalid member to group "));
		ret = false;
	}
	return ret;
}

///////////////////////////////////////////////////////////
bool GroupLink::RemoveMember(const Truid& t)
{
	bool ret = true;

	gpassert(t != Truid());
	MemberIter mit = m_Members.find(t);
	gpassert(mit != m_Members.end());
	if (mit != m_Members.end())
	{
		// Remove all the watchers from the old member trigger
		Trigger* memtrig;	
		if (gTriggerSys.FetchTriggerByTruid(*mit,memtrig))
		{
			WatcherIter wit = m_Watchers.begin();
			for (;wit != m_Watchers.end(); ++wit)
			{
				Condition* watchcond;
				if (gTriggerSys.FetchConditionByCuid((*wit),watchcond))
				{
					watchcond->SetGroupSize(m_Members.size()-1);
					ret &= memtrig->RemoveWatchedByCondition(watchcond);
				}
			}
		}
		m_Members.erase(mit);
	}
	else
	{
		gperrorf(("Trigger Groups - RemoveMember::Attempted to remove invalid member from group "));
		ret = false;
	}
	return ret;
}



