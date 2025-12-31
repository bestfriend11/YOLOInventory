#include "Precomp_game.h"
#include "zping.h"

const DWORD ZONE_PING_SENDS   = 1;
const DWORD ZONE_PING_TIMEOUT = 2000;
const short ZONE_PING_PORT = 28800;
// mdm - min/max number of ping requests that will be sent out for one ZONE_PING_TIMEOUT interval
const DWORD ZONE_PING_MIN_SENDS_PER_INTERVAL = 5;
const DWORD ZONE_PING_MAX_SENDS_PER_INTERVAL = 30; // assuming max udp packet size of 50, results in 50 * 3 * 30 bytes per 2 seconds = 3500 bytes per 2 seconds, or 1750 bytes per second (roughly half a 33.6 modem bandwidth)
// mdm - timeout in milliseconds for when we wrap around the buckets.
const DWORD ZONE_PING_WRAP_TIMEOUT = ZONE_PING_TIMEOUT * 3;

#define ZONE_PING_SIG    2
#define ZONE_PING_VER    1

#define ZONE_PING_TYPE_PING                  0
#define ZONE_PING_TYPE_PING_RESPONSE         1
#define ZONE_PING_TYPE_RESPONSE_RESPONSE     2
#define ZONE_PING_TYPE_PING_NO_RESPONSE      3


// Ping info is stuffed into 1 DWORD as follow
//              3                   2                   1
//            1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// Pinger-Pingee tick                               |-----------------------|
// Pingee-Pinger tick     |-----------------------|
// type               |-|
// ver            |-|
// sig(10)    |-|

typedef DWORD ZonePingPacket;

const DWORD PrePingSigVerPacket = (DWORD)((ZONE_PING_SIG << 30) | (ZONE_PING_VER << 28 ));
const DWORD PrePingPacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_PING << 26 );
const DWORD PrePingResponsePacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_PING_RESPONSE << 26 );
const DWORD PrePingNoResponsePacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_PING_NO_RESPONSE << 26 );
const DWORD PreResponseResponsePacket = PrePingSigVerPacket | ( ZONE_PING_TYPE_RESPONSE_RESPONSE << 26 );

#define MAKE_PING_PACKET( StartTick ) \
    ( (ZonePingPacket) ( PrePingPacket | ( StartTick & 0x1FFF ) ) )

#define MAKE_PING_RESPONSE_PACKET( packet, StartTick ) \
    ( (ZonePingPacket) ( PrePingResponsePacket | ( ( StartTick & 0x1FFF ) << 13 ) | (packet & 0x000001FFF ) ) )

#define MAKE_PING_NO_RESPONSE_PACKET( packet, StartTick ) \
    ( (ZonePingPacket) ( PrePingNoResponsePacket | ( ( StartTick & 0x1FFF ) << 13 ) | (packet & 0x000001FFF ) ) )

#define MAKE_RESPONSE_RESPONSE_PACKET( packet, measuredTick, adjustedTick ) \
    ( (ZonePingPacket) ( PreResponseResponsePacket | ( (adjustedTick & 0x1FFF) << 13 ) | (measuredTick & 0x000001FFF ) ) )

#define PING_PACKET_SIG_VER_OK( packet ) \
    ( ( packet & 0xF0000000 ) == PrePingSigVerPacket )

#define PING_PACKET_PINGER_PINGEE_TICK( packet ) \
    ( packet & 0x1FFF )

#define PING_PACKET_PINGEE_PINGER_TICK( packet ) \
    ( ( packet >> 13 ) & 0x1FFF )

#define PING_PACKET_TYPE( packet ) \
    ( ( packet >> 26 ) & 0x3 )




CZonePing::ZonePing::ZonePing(DWORD inet /*= 0*/) :
    m_inet(inet),
	m_latency(INFINITE),
    m_samples(0),
	m_tick(0),
	m_state(UNKNOWN),
    m_pNext(NULL),
    m_pCallback(NULL),
    m_context(NULL)
{
}



CZonePing::CZonePing() :
	m_PingArray(NULL), m_PingEntries(0),
    m_PingIntervalSec(0), m_CurInterval(0),
    m_pCurrentItem(0),
    m_Socket(INVALID_SOCKET), m_bWellKnownPort(TRUE), m_hStopEvent(NULL),
    m_hPingerThread(NULL), m_hPingeeThread(NULL),
    m_hStartupMutex(NULL),
    m_refCountStartup(0),
    m_inetArray(NULL), m_inetAlloc( 0 )
{
}


CZonePing::~CZonePing()
{
    if ( m_hStartupMutex )
    {
        CloseHandle( m_hStartupMutex );
		m_hStartupMutex=NULL;
    }
}


BOOL CZonePing::CreateSocket()
{
    SOCKADDR_IN sin;
    BOOL bRet = FALSE;

    m_Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( m_Socket != INVALID_SOCKET )
    {

        // if we can not bind to the port, the user probably has an other
        // process instance open using the port, so remote clients are
        // able to successfully ping us.  So we can get our latency measurement
        // to them by using another port - of course if the first instance
        // goes away, the remote users are hosed...
            m_bWellKnownPort = TRUE;
            sin.sin_port = htons(ZONE_PING_PORT);
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = INADDR_ANY;
        retry:
            if ( SOCKET_ERROR == bind( m_Socket,
                                       (const struct sockaddr FAR *) &sin,	
                                       sizeof(sin) ) )
            {
                if ( m_bWellKnownPort )
                {
                    m_bWellKnownPort = FALSE;
                    sin.sin_port = htons(0);
                    goto retry;
                }
            }
            else
            {
                bRet = TRUE;
            }
    }
    return bRet;
}


int __stdcall CZonePing::StartupServer( )
{

    HANDLE hStartupMutex = CreateMutex( NULL, FALSE, "ZonePingStartupMutex" );
    if ( !hStartupMutex )
        return E_FAIL;

    WaitForSingleObject( hStartupMutex, INFINITE );
    if ( !m_hStartupMutex )
        m_hStartupMutex = hStartupMutex;
    else
        CloseHandle( hStartupMutex ); // use m_hStartupMutex from now on

    if ( m_refCountStartup == 0 )
    {
		m_refCountStartup = 1;

        InitializeCriticalSection( m_pCS );

        WSADATA wsa;
        if ( WSAStartup(MAKEWORD(1,1), &wsa ) != 0 )
            goto shutdown;

        // mdm - removed m_inetLocal enumeration because gethostbyname call was
        // leading to crashes on WinME for some systems.  It is acceptable that we
        // send UDP ping packets to our localhost, though it might be wastefull.


        m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if ( !m_hStopEvent )
            goto shutdown;

        if ( !CreateSocket() )
            goto shutdown;

        m_hWellKnownPortEvent = CreateEvent(NULL, TRUE, FALSE, "ZonePingWellKnownPortEvent" );
        if ( !m_hWellKnownPortEvent )
            goto shutdown;

        DWORD tid;
        m_hPingeeThread = CreateThread(NULL, 4096, PingeeThreadProc, this, 0, &tid);
        if ( !m_hPingeeThread )
            goto shutdown;
    }

    ReleaseMutex(m_hStartupMutex);
	return S_OK;

  shutdown:
	
	Shutdown();
    ReleaseMutex(m_hStartupMutex);
	
	return E_FAIL;
}


HRESULT __stdcall CZonePing::StartupClient( DWORD ping_interval_sec )
{
    if ( !m_hStartupMutex )
        return E_UNEXPECTED;

    // mdm - we can't spin up the thread if there is no server thread
    // running.  This is determined from the value of the refCoutnStartup
    // member variable.
    if ( m_refCountStartup == 0 )
    {
        return E_UNEXPECTED;
    }

    WaitForSingleObject( m_hStartupMutex, INFINITE );

	if (!m_hPingerThread)
	{
		if ( ( ping_interval_sec ) && ( m_PingIntervalSec == 0 ) )
		{
			m_PingIntervalSec = ping_interval_sec;
			if ( m_PingIntervalSec / (ZONE_PING_TIMEOUT/1000) == 0 )
			{
				m_PingIntervalSec = ZONE_PING_TIMEOUT/1000;
			}

			m_CurInterval = m_PingIntervalSec;
			m_PingArray = new ZonePing[m_PingIntervalSec];
			if ( !m_PingArray )
				goto shutdown;

			DWORD tid;
			m_hPingerThread = CreateThread(NULL, 4096, PingerThreadProc, this, 0, &tid);
			if ( !m_hPingerThread )
				goto shutdown;
		}
	}

    ReleaseMutex(m_hStartupMutex);
    return S_OK;

  shutdown:
    Shutdown();
	ReleaseMutex(m_hStartupMutex);
    
    return E_FAIL;
} 


HRESULT __stdcall CZonePing::Shutdown()
{
    if ( m_hStartupMutex )
    {
        WaitForSingleObject( m_hStartupMutex, INFINITE );
        
		//make sure we haven't already called shutdown
		if (m_refCountStartup)
		{
			m_refCountStartup = 0;
			
			EnterCriticalSection(m_pCS);

			if ( m_hStopEvent )
			{
				SetEvent( m_hStopEvent );

				closesocket( m_Socket );
				m_Socket = INVALID_SOCKET;

				if ( m_hPingerThread )
				{
					WaitForSingleObject( m_hPingerThread, 5000 );
					CloseHandle( m_hPingerThread );
					m_hPingerThread = NULL;

					if ( m_inetArray )
					{
						free(m_inetArray);
						m_inetArray = NULL;
					}
					m_inetAlloc = 0;
				}

				if ( m_hPingeeThread )
				{
					WaitForSingleObject( m_hPingeeThread, 5000 );
					CloseHandle( m_hPingeeThread );
					m_hPingeeThread = NULL;
				}

				CloseHandle( m_hStopEvent );
				m_hStopEvent = NULL;

			}

			if ( m_hWellKnownPortEvent )
			{
				if ( m_bWellKnownPort )
				{
					//printf( "Setting wellknown event\n" );
					SetEvent( m_hWellKnownPortEvent );
				}
				CloseHandle( m_hWellKnownPortEvent );
				m_hWellKnownPortEvent = NULL;
			}

			if ( m_PingArray )
			{
				for ( DWORD ndx = 0; ndx < m_PingIntervalSec; ndx++ )
				{
					ZonePing* pPing = m_PingArray[ndx].m_pNext;
					while ( pPing )
					{
						ZonePing* pThis = pPing;
						pPing = pPing->m_pNext;
						delete pThis;
					}
				}
				delete [] m_PingArray;
				m_PingArray = NULL;
			}
			m_PingEntries = 0;
			m_PingIntervalSec = 0;
			m_pCurrentItem = 0;

			LeaveCriticalSection(m_pCS);
			DeleteCriticalSection( m_pCS );

			WSACleanup();
		}
		

        ReleaseMutex(m_hStartupMutex);

    }
    return S_OK;
}


HRESULT __stdcall CZonePing::Add( DWORD inet )
{
    DWORD ndx = GetListIndex( inet );

    // we'll create one assuming that we're going to us it.
    // Do it hear so we don't do it in the critical section
    ZonePing* pPingNew = new ZonePing( inet );
    if ( pPingNew  )
    {
        EnterCriticalSection(m_pCS);

        if ( m_PingArray )
        {
            // first check to see if we already exist
            ZonePing* pPing = m_PingArray[ndx].m_pNext;
            while ( pPing )
            {
                if ( pPing->m_inet == inet )
                    break;
                pPing = pPing->m_pNext;
            }
            if ( !pPing )
            {
				// add new entry
                pPingNew->m_pNext = m_PingArray[ndx].m_pNext;
                m_PingArray[ndx].m_pNext = pPingNew;
                m_PingEntries++;

                if ( m_pCurrentItem == NULL )
                {
                    // this is our new place in the list
                    m_pCurrentItem = pPingNew;
                }

                pPingNew = NULL;


            }
            else
            {
                // mdm - shouldn't happen but just in case.
                if ( m_pCurrentItem == NULL )
                {
                    m_pCurrentItem = pPing;
                }
            }

            LeaveCriticalSection(m_pCS);

            if ( pPingNew )
                delete pPingNew;

            return S_OK;
        }
        else
        {
            LeaveCriticalSection(m_pCS);
        }
    }
    return E_FAIL;
}


HRESULT __stdcall CZonePing::Ping( DWORD inet )
{
    SOCKADDR_IN sin;
    sin.sin_port = htons(ZONE_PING_PORT);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(inet);

    for ( DWORD ndx = 0; ndx < ZONE_PING_SENDS; ndx++ )
    {
        if ( ndx != 0 )
            Sleep(20); // provide a slight delay between iterations

        ZonePingPacket packet = MAKE_PING_PACKET( GetTickCount() );
        int len = sendto (
					m_Socket,
					(const char FAR *) &packet,
					sizeof(packet),
					0,
					(const struct sockaddr FAR *) &sin,
					sizeof(sin) );

        if ( len == SOCKET_ERROR )
            return E_FAIL;

    }

    return S_OK;
}


HRESULT __stdcall CZonePing::Remove( DWORD inet )
{
    HRESULT  hrRet = E_FAIL;
    DWORD ndx  = GetListIndex( inet );

    EnterCriticalSection(m_pCS);

    if ( m_PingArray )
    {
        ZonePing* pPrev = &(m_PingArray[ndx]);
        ZonePing* pPing = m_PingArray[ndx].m_pNext;
        while ( pPing )
        {
            if ( pPing->m_inet == inet )
            {
                if ( m_pCurrentItem == pPing )
                {
                    bool bWrapped = false;

                    m_pCurrentItem = FindNextItem( m_pCurrentItem, &bWrapped );
                 
                    if ( m_pCurrentItem == pPing ) // this means pping was last item in list
                    {
                        m_pCurrentItem = 0;
                    }
                }

                pPrev->m_pNext = pPing->m_pNext;
                delete pPing;
                m_PingEntries--;
                hrRet = S_OK;

                break;
            }
            pPrev = pPing;
            pPing = pPing->m_pNext;
        }
    }

    LeaveCriticalSection(m_pCS);

    return hrRet;

}


HRESULT __stdcall CZonePing::RemoveAll()
{
    EnterCriticalSection(m_pCS);

    if ( m_PingArray )
    {
        for ( DWORD ndx = 0; ndx < m_PingIntervalSec; ndx++ )
        {
            ZonePing* pPing = m_PingArray[ndx].m_pNext;
            while ( pPing )
            {
                ZonePing* pThis = pPing;
                pPing = pPing->m_pNext;
                delete pThis;
            }
			m_PingArray[ndx].m_pNext = NULL;
        }
    }

    m_PingEntries = 0;
    m_pCurrentItem = NULL;
    LeaveCriticalSection(m_pCS);

    return S_OK;
}


HRESULT __stdcall CZonePing::RegisterCallback( DWORD inet, PingCallbackFunc * pFunc, void * context )
{
    HRESULT hrRet = E_FAIL;
    DWORD ndx  = GetListIndex( inet );

    EnterCriticalSection(m_pCS);

    if ( m_PingArray )
    {
        ZonePing* pPing = m_PingArray[ndx].m_pNext;
        while ( pPing )
        {
            if ( pPing->m_inet == inet )
            {
                pPing->m_pCallback = pFunc;
                pPing->m_context = context;

                hrRet = S_OK;
                break;
            }
            pPing = pPing->m_pNext;
        }
    }

    LeaveCriticalSection(m_pCS);

    return hrRet;
}

HRESULT __stdcall CZonePing::Lookup( DWORD inet, DWORD* pLatency )
{
    HRESULT hrRet = E_FAIL;
    DWORD ndx  = GetListIndex( inet );

	if ( !pLatency )
		return E_FAIL;

    EnterCriticalSection(m_pCS);

    if ( m_PingArray )
    {
        ZonePing* pPing = m_PingArray[ndx].m_pNext;
        while ( pPing )
        {
            if ( pPing->m_inet == inet )
            {
                // give a grace period of 4*m_PingIntervalSec secs
                DWORD now = GetTickCount();
                if (	(pPing->m_samples != 0)
					&&	(GetTickDelta( now, pPing->m_tick ) > ((1000*m_PingIntervalSec)<<2)) )
                {
                    pPing->m_samples = 0;
                    pPing->m_latency = INFINITE;
                }
                *pLatency = pPing->m_latency;

                //printf( "Lookup - %x  now:%x  then:%x  delta:%d latency:%d\n", inet, now, pPing->m_tick, GetTickDelta( now, pPing->m_tick ), *pLatency );
                hrRet = S_OK;
                break;
            }
            pPing = pPing->m_pNext;
        }
    }

    LeaveCriticalSection(m_pCS);

    return hrRet;
}
                

DWORD WINAPI CZonePing::PingerThreadProc( LPVOID p )
{
    ((CZonePing*)p)->PingerThread();
    ExitThread(0);
}


////////////////////////////////////////////////////////////////////////////////////////
//  CZonePing::FindNextItem()
//  Purpose:
//       Given a zoneping object, find the next zoneping object to process, going 
//       through the buckets sequentially by index.  Returns NULL if there is no next 
//       pointer
//  
//  Parameters:
//       [pPing] :
//  
//  Returns:  ZonePing *
//  
//  Change Log
//       Author                 MMDDYY  Description
//       --------------------   ------  ------------------------------------------------
//       Michael Moore          101800  Initial Version
//  
//       
////////////////////////////////////////////////////////////////////////////////////////
CZonePing::ZonePing * CZonePing::FindNextItem( ZonePing * pPing, bool * bWrapped )
{
    if ( pPing == NULL )
    {
        return NULL;
    }

    // mdm - this generally covers finding the next item, unless we are at the end of
    // the current bucket.
    ZonePing * pRet = pPing->m_pNext;

    *bWrapped = false;
    DWORD loopCount = 0;

    // mdm - this while loop only iterates if you have a sparse distribution in
    // m_PingArray.  m_CurInterval is the index of the current bucket of m_PingArray.
    // Typically, you will enter here once, find the next bucket by incrementing 
    // m_CurInterval and set the return value to the first item in that bucket.
    // However, degenerate cases exist for sparse buckets.  For example, 
    // if you only have two items and 20 buckets.  In these type of degenerate 
    // cases where the array is sparsely populated, you will loop here as you go
    // through buckets that have nothing in them.  Eventually you will find a bucket
    // with something in it or loopcount will exceed the number of buckets.  In
    // the first case, you've found your return value, and in the second case your
    // return value is NULL because the entire m_PingArray is empty.
    while ( loopCount < m_PingIntervalSec && pRet == NULL)
    {
        m_CurInterval++;
        if ( m_CurInterval >= m_PingIntervalSec )
        {
            m_CurInterval = 0;
            *bWrapped = true;
        }
        
        // mdm - note that head of each bucket is dummy entry
        pRet = m_PingArray[m_CurInterval].m_pNext;
        loopCount++;
    }
    return pRet;
}

void CZonePing::PingerThread()
{
    HANDLE hEvents[] = { m_hStopEvent, m_hWellKnownPortEvent };

    SOCKADDR_IN sin;
    sin.sin_port = htons(ZONE_PING_PORT);
    sin.sin_family = AF_INET;

    bool bWrapped = false;
    DWORD pingSendsPerInterval = 0;
    DWORD dwWait = WAIT_OBJECT_0; // default to the stop event
  loop:
    do
    {
        EnterCriticalSection(m_pCS);

        DWORD nInet = 0;
        ZonePing * pSentinal = m_pCurrentItem;
        if ( !m_pCurrentItem )
        {
            LeaveCriticalSection(m_pCS);
            goto SLEEP;
        }

        if ( m_PingIntervalSec )
        {
            pingSendsPerInterval = m_PingEntries / (m_PingIntervalSec/(ZONE_PING_TIMEOUT/1000));
        }
        else
        {
            // mdm - only happens if m_PingIntervalSec is 0, which should never be used.
            pingSendsPerInterval = ZONE_PING_MIN_SENDS_PER_INTERVAL;
        }

        if ( pingSendsPerInterval > ZONE_PING_MAX_SENDS_PER_INTERVAL )
        {
            pingSendsPerInterval = ZONE_PING_MAX_SENDS_PER_INTERVAL;
        }
        else if ( pingSendsPerInterval < ZONE_PING_MIN_SENDS_PER_INTERVAL )
        {
            pingSendsPerInterval = ZONE_PING_MIN_SENDS_PER_INTERVAL;
        }


        // build an array of inet addresses to send ping requests 
        // each iteration of this do-while results in adding at most one new
        // entry to the m_inetArray, which is later used to batch the send
        // of ping requests.
        do
        {
            if ( m_pCurrentItem->m_state != ZonePing::PINGEE )
            {
                if ( nInet >= m_inetAlloc )
                {
                    DWORD * array = (DWORD *) malloc( sizeof(DWORD) * (m_inetAlloc+25) );
                    if ( m_inetArray )
                    {
                        CopyMemory( array, m_inetArray, m_inetAlloc * sizeof(DWORD) );
                        free( m_inetArray );
                    }
                    m_inetArray = array;
                    m_inetAlloc++;
                }
                m_inetArray[nInet++] = m_pCurrentItem->m_inet;
            }
            
            m_pCurrentItem = FindNextItem( m_pCurrentItem, &bWrapped );

            if ( bWrapped )
            {
                break;
            }

            if ( m_pCurrentItem == NULL )
            {
                // mdm - this should NEVER happen but I don't have an assert macro
                break;
            }

        } while ( pSentinal != m_pCurrentItem && nInet < pingSendsPerInterval);

        LeaveCriticalSection(m_pCS);

        while( nInet )
        {
            nInet--;

            sin.sin_addr.s_addr = htonl(m_inetArray[nInet]);
            ZonePingPacket packet = MAKE_PING_PACKET( GetTickCount() );
            int len = sendto (
						m_Socket,
						(const char FAR *) &packet,
						sizeof(packet),
						0,
						(const struct sockaddr FAR *) &sin,
						sizeof(sin) );

            if ( len == SOCKET_ERROR ) // stop event will be set, so let terminate end there
                break;

        }

SLEEP:
        // wrapping signals a pause
        if ( bWrapped )
        {
            // enforce a longer pause between full walks of all zoneping objects
            dwWait = WaitForMultipleObjects( sizeof(hEvents)/sizeof(*hEvents), hEvents, FALSE, ZONE_PING_WRAP_TIMEOUT);
            bWrapped = false;
        }
        else
        {
            dwWait = WaitForMultipleObjects( sizeof(hEvents)/sizeof(*hEvents), hEvents, FALSE, ZONE_PING_TIMEOUT );
        }
    } while( dwWait == WAIT_TIMEOUT );


    // see if we are being signaled to take of the well known port
    if ( !m_bWellKnownPort && (dwWait == WAIT_OBJECT_0+1) )
    {
        //printf( "attempting to take over wellknown port\n" );
           
        DWORD tid;     
        ResetEvent( m_hWellKnownPortEvent );

        closesocket( m_Socket );
        m_Socket = INVALID_SOCKET;

        if ( m_hPingeeThread )
        {
            WaitForSingleObject( m_hPingeeThread, 5000 );
            CloseHandle( m_hPingeeThread );
            m_hPingeeThread = NULL;
        }

        if ( !CreateSocket() )
            goto shutdown;

        m_hPingeeThread = CreateThread(NULL, 4096, PingeeThreadProc, this, 0, &tid);
        if ( !m_hPingeeThread )
            goto shutdown;

        goto loop;

    }

  shutdown:
    ;
}


DWORD WINAPI CZonePing::PingeeThreadProc( LPVOID p )
{
    ((CZonePing*)p)->PingeeThread();
    ExitThread(0);
}

void CZonePing::PingeeThread()
{
    char buf[100];
    ZonePingPacket* packetIn = (ZonePingPacket*)buf;
    ZonePingPacket  packetOut;
    SOCKADDR_IN sin;
    int sin_len = sizeof(sin);

    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
    
    for ( ; ; )
    {
        BOOL bSendResponse = TRUE;
        int len = recvfrom ( 
                    m_Socket,	
                    buf,
                    sizeof(buf),
                    0,
                    (struct sockaddr FAR *) &sin,	
                    &sin_len ); 	

        if (	(len == sizeof(*packetIn))
			&&	PING_PACKET_SIG_VER_OK( (*packetIn) ) )
        {
            switch( PING_PACKET_TYPE( (*packetIn) ) )
            {
                case ZONE_PING_TYPE_PING:
                    if ( m_PingEntries )
                    {
                        packetOut = MAKE_PING_RESPONSE_PACKET( (*packetIn), GetTickCount() );
                    }
                    else
                    {
                        packetOut = MAKE_PING_NO_RESPONSE_PACKET( (*packetIn), GetTickCount() );
                    }

                     // don't worry about error condition
                    // recv will catch the socket close
                    sendto (
                        m_Socket,
                        (const char FAR *) &packetOut,
                        sizeof(packetOut),
                        0,	
                        (const struct sockaddr FAR *) &sin,	
                        sin_len );	

                    break;

                case ZONE_PING_TYPE_PING_NO_RESPONSE:
                    bSendResponse = FALSE;
                    // fall thru
                case ZONE_PING_TYPE_PING_RESPONSE:
                {
                    DWORD inet = ntohl(sin.sin_addr.s_addr);
                    DWORD now  = GetTickCount();
                    DWORD latencyOld, latencyNew;
                    EnterCriticalSection(m_pCS);
                    if ( m_PingArray )
                    {
                        DWORD ndx  = GetListIndex( inet );

                        ZonePing* pPing = m_PingArray[ndx].m_pNext;
                        while ( pPing )
                        {
                            if ( pPing->m_inet == inet )
                            {
                                latencyOld = pPing->m_latency;
                                if ( GetTickDelta(now, pPing->m_tick) > ZONE_PING_TIMEOUT )
                                {
                                    pPing->m_latency = 0;
                                    pPing->m_samples = 0;
                                }

                                latencyNew = ((pPing->m_latency*pPing->m_samples)+Get13BitTickDelta( (now & 0x1FFF), PING_PACKET_PINGER_PINGEE_TICK( (*packetIn) ) ))/(++pPing->m_samples);
                                if ( (pPing->m_samples == 1) && ( latencyNew > latencyOld ) ) // we've gotten worse
                                {
                                    // so bias for graceful degradation
                                    pPing->m_latency = ((latencyNew*pPing->m_samples)+latencyOld ) / (pPing->m_samples+1);
                                }
                                else
                                {
                                    pPing->m_latency = latencyNew;
                                }
                                pPing->m_tick = now;
                                
                                pPing->m_state = ZonePing::PINGER;

                                if ( bSendResponse )
                                {
                                    packetOut = MAKE_RESPONSE_RESPONSE_PACKET(
													(*packetIn),
													(pPing->m_latency),
													( PING_PACKET_PINGEE_PINGER_TICK( (*packetIn) ) + GetTickDelta( GetTickCount(), now) ) );
                                }

                                // invoke callback if there is one registered
                                if ( pPing->m_pCallback )
                                {
                                    (*pPing->m_pCallback)( inet, pPing->m_latency, pPing->m_context);
                                }


                                LeaveCriticalSection(m_pCS);

                                if ( bSendResponse )
                                {
                                    // don't worry about error condition
                                    // recv will catch the socket close
                                    sendto (
                                        m_Socket,
                                        (const char FAR *) &packetOut,
                                        sizeof(packetOut),
                                        0,
                                        (const struct sockaddr FAR *) &sin,
                                        sin_len );
                                }

                                break;
                            }

                            pPing = pPing->m_pNext;

                        }
                        if ( pPing == NULL )
                        {
                            LeaveCriticalSection(m_pCS);
                        }
                    }
                    else
                    {
                        LeaveCriticalSection(m_pCS);
                    }
                    break;
                }

                case ZONE_PING_TYPE_RESPONSE_RESPONSE:
                {    
                    DWORD inet = ntohl(sin.sin_addr.s_addr);
                    DWORD now  = GetTickCount();

                    EnterCriticalSection(m_pCS);
                    if ( m_PingArray )
                    {
                        DWORD ndx  = GetListIndex( inet );

                        ZonePing* pPing = m_PingArray[ndx].m_pNext;
                        while ( pPing )
                        {
                            if ( pPing->m_inet == inet )
                            {
                                if ( GetTickDelta(now, pPing->m_tick) > ZONE_PING_TIMEOUT )
                                {
                                    pPing->m_latency = 0;
                                    pPing->m_samples = 0;
                                }

                                pPing->m_samples += 2;
                                pPing->m_latency = ((pPing->m_latency*pPing->m_samples) +
                                              Get13BitTickDelta( (now & 0x1FFF), PING_PACKET_PINGEE_PINGER_TICK( (*packetIn) ) ) +
                                              PING_PACKET_PINGER_PINGEE_TICK( (*packetIn) )  )/(pPing->m_samples);
                                pPing->m_tick = now;

                                if ( pPing->m_state == ZonePing::UNKNOWN )
                                    pPing->m_state = ZonePing::PINGEE;
                                if ( pPing->m_pCallback )
                                {
                                    (*pPing->m_pCallback)( inet, pPing->m_latency, pPing->m_context);
                                }
                                break;
                            }

                            pPing = pPing->m_pNext;
                        }
                    }
                    LeaveCriticalSection(m_pCS);

                    break;
                }

            }
        }
        else if ( len == SOCKET_ERROR )
        {
            int wsaError = WSAGetLastError();
            if ( ( wsaError == WSAECONNRESET ) ||
                 ( wsaError == WSAEMSGSIZE ) )
            {
                ;// do nothing
            }
            else
            {
                break;
            }

        }

    }

}

