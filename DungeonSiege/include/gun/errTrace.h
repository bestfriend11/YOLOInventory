#pragma once

namespace Gun2
{

#ifdef _DEBUG
#define DBG 1
#endif

#if DBG != 1

  #define DECLARE_DEBUG(comp)
  #define DECLARE_INFOLEVEL(comp)

#else

//+----------------------------------------------------------------------
// (taken from NT debnot.hxx)
//
// DECLARE_DEBUG(comp)
// DECLARE_INFOLEVEL(comp)
//
// This macro defines xxDebugOut where xx is the component prefix
// to be defined. This declares a static variable 'xxInfoLevel', which
// can be used to control the type of xxDebugOut messages printed to
// the terminal. For example, xxInfoLevel may be set at the debug terminal.
// This will enable the user to turn debugging messages on or off, based
// on the type desired. The predefined types are defined below. Component
// specific values should use the upper 24 bits
//
// To Use:
//
// 1)   In your components main include file, include the line
//              DECLARE_DEBUG(comp)
//      where comp is your component prefix
//
// 2)   In one of your components source files, include the line
//              DECLARE_INFOLEVEL(comp)
//      where comp is your component prefix. This will define the
//      global variable that will control output.
//
// It is suggested that any component define bits be combined with
// existing bits. For example, if you had a specific error path that you
// wanted, you might define DEB_<comp>_ERRORxxx as being
//
// (0x100 | DEB_ERROR)
//
// This way, we can turn on DEB_ERROR and get the error, or just 0x100
// and get only your error.
//
// Define values that are specific to your xxInfoLevel variable in your
// own file, like ciquery.hxx.
//
//-----------------------------------------------------------------------

  #define DEB_ERROR      0x00000001      // exported error paths
  #define DEB_WARN       0x00000002      // exported warnings
  #define DEB_TRACE      0x00000004      // exported trace messages

  #define DEB_IERROR     0x00000010      // internal error paths
  #define DEB_IWARN      0x00000020      // internal warnings
  #define DEB_ITRACE     0x00000040      // internal trace messages
    
  #define DEB_USER1      0x00000100      // User Defined
  #define DEB_USER2      0x00000200      // User Defined
  #define DEB_USER3      0x00000400      // User Defined
  #define DEB_USER4      0x00000800      // User Defined
  #define DEB_USER5      0x00001000      // User Defined
  #define DEB_USER6      0x00002000      // User Defined
  #define DEB_USER7      0x00004000      // User Defined
  #define DEB_USER8      0x00008000      // User Defined
  #define DEB_USER9      0x00010000      // User Defined
  #define DEB_USER10     0x00020000      // User Defined
  #define DEB_USER11     0x00040000      // User Defined
  #define DEB_USER12     0x00080000      // User Defined

  #define DEB_ID_FRONT   0x40000000      // Put Pid:Tid:MOD at start of line.
  #define DEB_CONSOLE    0x80000000      // Write output to console (also)

  #ifndef DEF_INFOLEVEL
    #define DEF_INFOLEVEL (DEB_ERROR | DEB_WARN)
  #endif

#define GetInfoLevelInitA     Gun2::ZTechInfoLevelInit

    void vdprintf( unsigned long infoLevel, unsigned long ulCompMask,
                    char const *pszComp,
                    char const *ppszfmt,
                    va_list  ArgList );


    #define ZTechAssert(x) (void)((x) || (Gun2::ZTechAssertImp(#x, __FILE__, __LINE__),0))


// ZTechAssertImp will call the current Assert Handler.
//
    void ZTechAssertImp( char const *pszExp,
                         char const *pszFile,
                         int iLine );

    typedef void (*ZTechAssertHandler)(LPCSTR pszExp, LPCSTR pszFile, int line );

//  These are routines to set and get the assert handler.
//
//extern BOOL DefaultAssertHandler( char* pAssertString );
    void ZTechAssertSetHandler( ZTechAssertHandler pHandler );
    ZTechAssertHandler ZTechAssertGetHandler();

    DWORD ZTechInfoLevelInit( LPCSTR pszSys, LPCSTR pszSubSys, DWORD dwDefault );



//
//  Initialize the info level from a setting in the [GameX] section
//  of the registry.
//
  #if defined(GetInfoLevelInitA)

    #define DECLARE_INFOLEVEL(comp) \
    namespace Gun2 { \
        extern unsigned long comp##InfoLevel = GetInfoLevelInitA (     \
                                                        "Gun",         \
                                                        #comp,           \
                                                        DEF_INFOLEVEL ); \
        extern char *comp##InfoLevelString = #comp; \
    }
  #else

// Simple initialisation
    #define DECLARE_INFOLEVEL(comp) \
  namespace Gun2 { \
        extern unsigned long comp##InfoLevel = DEF_INFOLEVEL;        \
        extern char *comp##InfoLevelString = #comp; \
  }

  #endif

    #define DECLARE_DEBUG(comp) \
  namespace Gun2 { \
    extern unsigned long comp##InfoLevel; \
    extern char *comp##InfoLevelString; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...) \
    { \
        if (comp##InfoLevel & fDebugMask) \
        { \
            va_list va; \
            va_start (va, pszfmt); \
            vdprintf(comp##InfoLevel, fDebugMask, comp##InfoLevelString, pszfmt, va);\
            va_end(va); \
        } \
    }     \
    \
    class comp##CDbgTrace\
    {\
    private:\
        unsigned long _ulFlags;\
        char const * _pszName;\
    public:\
        comp##CDbgTrace(unsigned long ulFlags, char const * const pszName);\
        ~comp##CDbgTrace();\
    };\
    \
    inline comp##CDbgTrace::comp##CDbgTrace(\
            unsigned long ulFlags,\
            char const * pszName)\
    : _ulFlags(ulFlags), _pszName(pszName)\
    {\
        comp##InlineDebugOut(_ulFlags, "Entering %s\n", _pszName);\
    }\
    \
    inline comp##CDbgTrace::~comp##CDbgTrace()\
    {\
        comp##InlineDebugOut(_ulFlags, "Exiting %s\n", _pszName);\
    }\
  }

#endif // DBG


/////////////////////////////////////////////////
//
// Module Non-Specific ztASSERT.
//
/////////////////////////////////////////////////

#if DBG == 1                                
    
    #define ztASSERT(x)      ZTechAssert(x)

	// Usage:  ASSERTIMP(a+c==b+c, IMPLIES a==b);
	#define ASSERTIMP(a, b) ztASSERT(!(a) || (b));
	#define IMPLIES

	// Usage:  ASSERTIFF(a==b, b==a);
	#define ASSERTIFF(a, b) ztASSERT(((a)==0) == ((b)==0));

#else

    #define ztASSERT(x)      

	// Usage:  ASSERTIMP(a+c==b+c, IMPLIES a==b);
	#define ASSERTIMP(a, b)
	#define IMPLIES

	// Usage:  ASSERTIFF(a==b, b==a);
	#define ASSERTIFF(a, b)

#endif  // DBG


/////////////////////////////////////////////////
//
// Error Jump Macros.
//
/////////////////////////////////////////////////

#if DBG == 1

    #define ErrJmp(comp, label, errval, var) \
    {\
        var = errval;\
        comp##Debug(( DEB_IERROR, "%s(%d): Error %lx",     \
                __FILE__, __LINE__, (unsigned long)var )); \
        goto label;\
    }
    
#else
    #define ErrJmp(comp, label, errval, var) \
    {\
        var = errval;\
        goto label;\
    }

#endif

inline HRESULT GetLastHRESULT()
{
    DWORD err = GetLastError();

    if( FAILED(err) )
        return err;
    else
        return HRESULT_FROM_WIN32(err);
}

} // namespace
