//
//   Copyright (c) 2000 Microsoft Corporation
//

#pragma once
#include <GunQueue.h>

namespace Gun2 // namespace for all gun reuseable components.
{

//ids for server host events
#define SHM_QUIT			1	//no message contents at this time
#define SHM_HEARTBEAT		2	//sent by server host to let server know server host is still running

#define SHM_RESERVED	1000	//all application message ids must be greater than this value

interface _declspec(uuid("{B8F1EF9A-913C-4665-A0A3-DD634E2B0331}"))
IServerHost : public IUnknown
{
	STDMETHOD(SetQueue)(IGUNQueue* pQueue)=0;

	//can be on another interface but not necessary right now
	STDMETHOD(SetUserCount)(DWORD usercount)=0;
};

interface _declspec(uuid("{8A4F87E9-9153-454d-A822-6C200881E875}"))
IServer : public IUnknown
{
	STDMETHOD(Run)(IServerHost *pHost, bool bIsService, const char * szName, 
                   const char * szFile)=0;
};



interface _declspec(uuid("{DA7D08A8-7D5C-42a7-9A6F-21CC7B8AAC69}"))
IServerHostEvents
{
	STDMETHOD(Exit)()=0;
	STDMETHOD(Heartbeat)()=0;
};


DWORD DispatchServerHostEvent(GUNQueueMessage *pMsg, IServerHostEvents * pEvents);

} // namespace Gun2
