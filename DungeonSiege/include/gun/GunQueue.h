/*-------------------------------------------------------------------------
  GunQueue.hpp
  
  Generic queue for Gun events used by arbitrary subsystems
  
  Owner: curtc
  
  Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/
#pragma once

namespace Gun2 // namespace for all gun reusable library components
{


typedef DWORD GUNMSGID;

interface IGUNAllocator;

//flags for queue
#define QMF_PRIORITY_ONE  0x01  // Message goes to front of queue
#define QMF_IUNKNOWN      0x02  // Buffer paramter is actually IUnknown
#define QMF_POINTER       0x04  // Just hang the pointer, don't copy.

//Default memory management can be that next read on same thread will result in freeing of buffer from last read

struct GUNQueueMessage
{
    DWORD     flags;
    GUNMSGID  gmid;
    DWORD     param;
    DWORD     cb;
    PVOID     pvBuffer;
};

struct GUNQueueInfo
{
    DWORD   cItems;
    DWORD   cSize;
};

interface IGUNQueueRead //no : public IUnknown because it will never be referenced
{
    STDMETHOD(Read)(GUNQueueMessage *pgqm)=0;
};

interface 
__declspec(uuid("{A12E0C77-D9BB-46c1-9A94-6D78811632FF}"))
IGUNQueue : public IUnknown
{
    STDMETHOD(Write)(GUNQueueMessage * pgqm)=0;
    STDMETHOD(Read)(IGUNQueueRead *pRead)=0;
    STDMETHOD(UseEvent)(bool bUseEvent)=0;
    STDMETHOD_(HANDLE,GetWaitHandle)(void)=0;
    STDMETHOD(GetInfo)(GUNQueueInfo *pInfo)=0;
};


STDMETHODIMP GUNCreateQueue(IGUNQueue **ppQ, IGUNAllocator * pAllocator);

//
// Reserved GUN Queue Message Ranges
//
#define GUNMSGID_GunNet_First   50000
#define GUNMSGID_GunSql_First   60000


} // namespace Gun2
