//-----------------------------------------------------------------------------
//    ZoneAsync.h
//
//    Copyright (c) Microsoft Corp. 1999-2000. All rights reserved.
//
//    Content: Interface for Managing Asynchronous objects.
//	 
//-----------------------------------------------------------------------------

#pragma once

#include "ZoneTechErr.h"

interface __declspec(
        uuid( "9DAF85BB-851D-42df-A236-B5EE965582DD" ) ) IZoneAsync;

#define IID_IZoneAsync  __uuidof( IZoneAsync )


interface IZoneAsync: public IUnknown
{
    enum CAPS {
            CAPS_SINGLE_USE     =0x1,
            CAPS_SUPPORTS_PAUSE =0x2,
            CAPS_CONTINUOUS     =0x4
    };
        
    enum STATUS {
        // These status are the "not running" states.
        //
        STATUS_CREATED=0x100,   // Object is blank.
        STATUS_INITIALIZED,     // Object is ready to Start()
        STATUS_FAILED,
        STATUS_COMPLETED,
        STATUS_CANCELLED,
    
        // Transition States.
        STATUS_STARTING=0x200,
        STATUS_STOPPING,
    
        // Operating States.
        STATUS_RUNNING=0x400,
        STATUS_PAUSED,
    };
    
    
    virtual HRESULT STDMETHODCALLTYPE Start( IN HANDLE hEvent ) = 0;

    virtual HRESULT STDMETHODCALLTYPE Stop () = 0;

    virtual HRESULT STDMETHODCALLTYPE Pause () = 0;

    virtual HRESULT STDMETHODCALLTYPE Continue () = 0;

    virtual HRESULT STDMETHODCALLTYPE GetCaps ( OUT DWORD *pdwCaps ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetStatus ( OUT STATUS* pStatus ) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetLastError () = 0;
};

#define IS_NOT_RUNNING(state) (state&0x100)
#define IS_TRANSITION(state)  (state&0x200)
#define IS_OPERATING(state)	  (state&0x400)

