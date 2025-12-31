/*************************************************************************
 * $Archive: /Eng/SafeCast2/API/MSVC/Include/CdaDecl.h $
 * $Revision: 11 $
 *
 * Description:
 *		Standard type declarations: MSVC 32-bit
 *		
 *
 ***************************************************************************************************
 * NOTIFICATION OF COPYRIGHT AND OWNERSHIP OF SOFTWARE:
 *
 * Copyright (c) 1993-2000, C-Dilla Ltd.  All Rights Reserved.
 *
 * This computer program is the property of C-Dilla Ltd. of Reading, Berkshire, England
 * and Macrovision Corp. of Sunnyvale, California, U.S.A.  
 *
 * It may be used and copied only by licensed customers of Macrovision for the purpose
 * of developing API client programs.
 *
 ***************************************************************************************************
 *
 * 
 * $Nokeywords: $
 *************************************************************************
 */

#ifndef	CDADECL_H
#define	CDADECL_H

#define CdaDeclWin32
#define CdaDeclMSVC

// function scope

#define PUBLIC

// macro for defining pointer types
#define	TYPEDEF_POINTER(T)	typedef	T * P##T;
#define	TYPEDEF_HUGE_POINTER(T)	typedef	T * HP##T;

// NONE OF THE INTEGER TYPES MUST HAVE THEIR SIZES CHANGED
// basic integer types
// @type CdaUINT32 | An unsigned 32-bit integer
typedef	unsigned long int	CdaUINT32;
// @type CdaSINT32 | A signed 32-bit integer
typedef	  signed long int	CdaSINT32;
// @type CdaUINT16 | An unsigned 16-bit integer
typedef	unsigned short int	CdaUINT16;
// @type CdaSINT16 | A signed 16-bit integer
typedef	  signed short int	CdaSINT16;
// @type CdaUINT8 | An unsigned 8-bit integer
typedef	unsigned char		CdaUINT8;
// @type CdaSINT8 | A signed 8-bit integer
typedef	  signed char		CdaSINT8;

// @type CdaCHAR | A character
typedef	char				CdaCHAR;

// @type CdaMCHAR | A byte of a multi-byte character
typedef	char				CdaMCHAR;

// @type CdaWCHAR | A wide character
typedef	unsigned short		CdaWCHAR;

// @topic Maximum and minimum values for the basic integer types |

#define CdaUINT32_MIN (0)			// @flag CdaUINT32_MIN | Minimum 32-bit unsigned value
#define CdaUINT32_MAX (0xFFFFFFFF)	// @flag CdaUINT32_MAX | Maximum 32-bit unsigned value

#define CdaSINT32_MIN (0x80000000)	// @flag CdaSINT32_MIN | Minimum 32-bit signed value
#define CdaSINT32_MAX (0x7FFFFFFF)	// @flag CdaSINT32_MAX | Maximum 32-bit signed value

#define CdaUINT16_MIN (0)			// @flag CdaUINT16_MIN | Minimum 16-bit unsigned value
#define CdaUINT16_MAX (0xFFFF)		// @flag CdaUINT16_MAX | Maximum 16-bit unsigned value

#define CdaSINT16_MIN (0x8000)		// @flag CdaSINT16_MIN | Minimum 16-bit signed value
#define CdaSINT16_MAX (0x7FFF)		// @flag CdaSINT16_MAX | Maximum 16-bit signed value

#define CdaUINT8_MIN  (0)			// @flag CdaUINT8_MIN  | Minimum 8-bit unsigned value
#define CdaUINT8_MAX  (0xFF)		// @flag CdaUINT8_MAX  | Maximum 8-bit unsigned value

#define CdaSINT8_MIN  (0x80)		// @flag CdaUINT8_MIN  | Minimum 8-bit signed value
#define CdaSINT8_MAX  (0x7F)		// @flag CdaUINT8_MAX  | Maximum 8-bit signed value

// constant versions of basic integer types
// @type CdaCUINT32 | A constant unsigned 32-bit integer
typedef	const unsigned long int		CdaCUINT32;
// @type CdaCSINT32 | A constant signed 32-bit integer
typedef	const   signed long int		CdaCSINT32;
// @type CdaCUINT16 | A constant unsigned 16-bit integer
typedef	const unsigned short int	CdaCUINT16;
// @type CdaCSINT16 | A constant signed 16-bit integer
typedef	const   signed short int	CdaCSINT16;
// @type CdaCUINT8 | A constant unsigned 8-bit integer
typedef	const unsigned char			CdaCUINT8;
// @type CdaCSINT8 | A constant signed 8-bit integer
typedef	const   signed char			CdaCSINT8;

// @type CdaCCHAR | A constant character
typedef	const char					CdaCCHAR;

// @type CdaMCHAR | A constant byte of a multi-byte character
typedef	const char					CdaCMCHAR;

// @type CdaWCHAR | A constant wide character
typedef	const unsigned short		CdaCWCHAR;

// pointer to the integer types
// @type PCdaUINT32 | A pointer to an unsigned 32-bit integer
TYPEDEF_POINTER(CdaUINT32)
// @type PCdaSINT32 | A pointer to a signed 32-bit integer
TYPEDEF_POINTER(CdaSINT32)
// @type PCdaUINT16 | A pointer to an unsigned 16-bit integer
TYPEDEF_POINTER(CdaUINT16)
// @type PCdaSINT16 | A pointer to a signed 16-bit integer
TYPEDEF_POINTER(CdaSINT16)
// @type PCdaUINT8 | A pointer to an unsigned 8-bit integer
TYPEDEF_POINTER(CdaUINT8)
// @type PCdaSINT8 | A pointer to a signed 8-bit integer
TYPEDEF_POINTER(CdaSINT8)
// @type PCdaCHAR | A pointer to an unsigned character
TYPEDEF_POINTER(CdaCHAR)
// @type PCdaMCHAR | A pointer to a multi-byte character
TYPEDEF_POINTER(CdaMCHAR)
// @type PCdaMCHAR | A pointer to a pointer to a multi-byte character
TYPEDEF_POINTER(PCdaMCHAR)
// @type PCdaWCHAR | A pointer to wide character
TYPEDEF_POINTER(CdaWCHAR)
// @type PCdaWCHAR | A pointer to a pointer to a wide character
TYPEDEF_POINTER(PCdaWCHAR)

// @type HPCdaCHAR | A huge pointer to an unsigned character
TYPEDEF_HUGE_POINTER(CdaCHAR)
// @type HPCdaUINT8 | A huge pointer to an unsigned 8-bit integer
TYPEDEF_HUGE_POINTER(CdaUINT8)
// @type HPCdaUINT32 | A huge pointer to an unsigned 32-bit integer
TYPEDEF_HUGE_POINTER(CdaUINT32)

// @type PCdaCUINT32 | A pointer to a constant unsigned 32-bit integer
TYPEDEF_POINTER(CdaCUINT32)
// @type PCdaCSINT32 | A pointer to a constant signed 32-bit integer
TYPEDEF_POINTER(CdaCSINT32)
// @type PCdaCUINT16 | A pointer to a constant unsigned 16-bit integer
TYPEDEF_POINTER(CdaCUINT16)
// @type PCdaCSINT16 | A pointer to a constant signed 16-bit integer
TYPEDEF_POINTER(CdaCSINT16)
// @type PCdaCUINT8 | A pointer to a constant unsigned 8-bit integer
TYPEDEF_POINTER(CdaCUINT8)
// @type PCdaCSINT8 | A pointer to a constant signed 8-bit integer
TYPEDEF_POINTER(CdaCSINT8)
// @type PCdaCCHAR | A pointer to a constant unsigned character
TYPEDEF_POINTER(CdaCCHAR)
// @type PCdaCMCHAR | A pointer to a constant multi-byte character
TYPEDEF_POINTER(CdaCMCHAR)
// @type PCdaWCHAR | A pointer to a constant wide character
TYPEDEF_POINTER(CdaCWCHAR)

// @type PPCdaUINT32 | A pointer to a pointer to an unsigned 32-bit integer
TYPEDEF_POINTER(PCdaUINT32)
// @type PPCdaSINT32 | A pointer to a pointer to a signed 32-bit integer
TYPEDEF_POINTER(PCdaSINT32)
// @type PPCdaUINT16 | A pointer to a pointer to an unsigned 16-bit integer
TYPEDEF_POINTER(PCdaUINT16)
// @type PPCdaSINT16 | A pointer to a pointer to a signed 16-bit integer
TYPEDEF_POINTER(PCdaSINT16)
// @type PPCdaUINT8 | A pointer to a pointer to an unsigned 8-bit integer
TYPEDEF_POINTER(PCdaUINT8)
// @type PPCdaSINT8 | A pointer to a pointer to a signed 8-bit integer
TYPEDEF_POINTER(PCdaSINT8)
// @type PPCdaCHAR | A pointer to a pointer to an unsigned character
TYPEDEF_POINTER(PCdaCHAR)

// @type PPCdaCUINT32 | A pointer to a pointer to a constant unsigned 32-bit integer
TYPEDEF_POINTER(PCdaCUINT32)
// @type PPCdaCSINT32 | A pointer to a pointer to a constant signed 32-bit integer
TYPEDEF_POINTER(PCdaCSINT32)
// @type PPCdaCUINT16 | A pointer to a pointer to a constant unsigned 16-bit integer
TYPEDEF_POINTER(PCdaCUINT16)
// @type PPCdaCSINT16 | A pointer to a pointer to a constant signed 16-bit integer
TYPEDEF_POINTER(PCdaCSINT16)
// @type PPCdaCUINT8 | A pointer to a pointer to a constant unsigned 8-bit integer
TYPEDEF_POINTER(PCdaCUINT8)
// @type PPCdaCSINT8 | A pointer to a pointer to a constant signed 8-bit integer
TYPEDEF_POINTER(PCdaCSINT8)
// @type PPCdaCCHAR | A pointer to a pointer to a constant unsigned character
TYPEDEF_POINTER(PCdaCCHAR)


// 64-bit types (and pointers to them)
// implemented as structures to allow for the 16-bit compilers
typedef struct tagCdaBITS64 CdaBITS64;
TYPEDEF_POINTER(CdaBITS64)

struct tagCdaBITS64
{
	CdaUINT8	UINT8s[8];
};

typedef struct tagCdaUINT64 CdaUINT64;
TYPEDEF_POINTER(CdaUINT64)

typedef const struct tagCdaUINT64 CdaCUINT64;
TYPEDEF_POINTER(CdaCUINT64)

struct tagCdaUINT64
{
	union
	{
		CdaUINT32	u32[2];	// access only via macros below
	} u64;
};

#define HighOfCdaUINT64(x) ((x).u64.u32[1])
#define LowOfCdaUINT64(x)  ((x).u64.u32[0])

#define HighOfPCdaUINT64(px) ((px)->u64.u32[1])
#define LowOfPCdaUINT64(px)   ((px)->u64.u32[0])

// boolean types (and pointers to them)
// @type CdaBOOL | A boolean value which can be CdaTRUE or CdaFALSE
typedef	CdaUINT16	CdaBOOL;
// @type CdaCBOOL | A constant boolean value which can be CdaTRUE or CdaFALSE
typedef	CdaCUINT16	CdaCBOOL;

// @type PCdaBOOL | A pointer to a boolean value which can be CdaTRUE or CdaFALSE
TYPEDEF_POINTER(CdaBOOL)
// @type PCdaCBOOL | A pointer to a constant boolean value which can be CdaTRUE or CdaFALSE
TYPEDEF_POINTER(CdaCBOOL)

// @type PPCdaBOOL | A pointer to a pointer to a boolean value which can be CdaTRUE or CdaFALSE
TYPEDEF_POINTER(PCdaBOOL)
// @type PPCdaCBOOL | A pointer to a pointer to a constant boolean value which can be CdaTRUE or CdaFALSE
TYPEDEF_POINTER(PCdaCBOOL)

#define	CdaTRUE		((CdaBOOL)1)
#define	CdaFALSE	((CdaBOOL)0)


// void and pointer to void types
// @type CdaVOID | The void type
typedef	void	CdaVOID;
// @type CdaCVOID | The constant void type
typedef	const void	CdaCVOID;

// @type PCdaVOID | A pointer to the vod type
TYPEDEF_POINTER(CdaVOID)
// @type PCdaVOID | A pointer to a constant void type
TYPEDEF_POINTER(CdaCVOID)

// @type PPCdaVOID | A pointer to a pointer to the void type
TYPEDEF_POINTER(PCdaVOID)
// @type PPCdaCVOID | A pointer to a pointer to the constant void type
TYPEDEF_POINTER(PCdaCVOID)

// @type HPCdaVOID | A huge pointer to the void type
TYPEDEF_HUGE_POINTER(CdaVOID)

// @type CdaLENGTH | Integer type used to indicate the length of a piece of memory
// or a string
// note that CdaLENGTH may have different bit length per platform so must not be
// used in persistent objects

typedef	CdaUINT32	CdaLENGTH;
// @type CdaCLENGTH | A constant <t CdaLENGTH>
typedef	CdaCUINT32	CdaCLENGTH;

#define CdaLENGTH_MAX CdaUINT32_MAX

// @type PCdaLENGTH | A pointer to a <t CdaLENGTH>
TYPEDEF_POINTER(CdaLENGTH)
// @type PCdaLENGTH | A pointer to a constant <t CdaLENGTH>
TYPEDEF_POINTER(CdaCLENGTH)

// @const MIN_CdaLENGTH | The minimum value that can be stored in a CdaLENGTH
#define	MIN_CdaLENGTH	(0x00000000L)
// @const MAX_CdaLENGTH | The maximum value that can be stored in a CdaLENGTH
#define	MAX_CdaLENGTH	(0xffffffffL)

// @type CdaOS_ERROR | Operating system error type
typedef	CdaUINT32	CdaOS_ERROR;
// @type PCdaOS_ERROR | A pointer to a <t CdaOS_ERROR>
TYPEDEF_POINTER(CdaOS_ERROR)

// @const CdaOS_ERROR_NONE | The error value which represents 'no error'
#define	CdaOS_ERROR_NONE		(0L)

// a file offset 
typedef	CdaUINT32	CdaFILEOFFSET;
TYPEDEF_POINTER(CdaFILEOFFSET)
#define CdaFILEOFFSET_MAX CdaUINT32_MAX

// type of other return values 
typedef	CdaUINT32	CdaRESULT;
TYPEDEF_POINTER(CdaRESULT)

//******************************************************************************
// CdaDAY - number of days since 31-Dec-1992 (Day 1 is 1-Jan-1993)
//
// MUST REMAIN TWO BYTES LONG (IT APPEARS IN PERSISTENT STRUCTURES)
//******************************************************************************

typedef	CdaUINT16	CdaDAY;
typedef	CdaCUINT16	CdaCDAY;

TYPEDEF_POINTER(CdaDAY)
TYPEDEF_POINTER(CdaCDAY)

TYPEDEF_POINTER(PCdaDAY)
TYPEDEF_POINTER(PCdaCDAY)

#define CdaDAY_MAX	CdaUINT16_MAX

//******************************************************************************
// CdaTIME - number of seconds since midnight (00:00:00) 1-Jan-1970 UCT
//
// MUST REMAIN FOUR BYTES LONG (IT APPEARS IN PERSISTENT STRUCTURES)
//******************************************************************************

typedef	CdaUINT32	CdaTIME;
typedef	CdaCUINT32	CdaCTIME;

TYPEDEF_POINTER(CdaTIME)
TYPEDEF_POINTER(CdaCTIME)

#define CdaTIME_MAX	CdaUINT32_MAX

//******************************************************************************
// CdaLOCALTIME - CdaTIME adjusted to local time zone
//******************************************************************************

typedef	CdaUINT32	CdaLOCALTIME;
typedef	CdaCUINT32	CdaCLOCALTIME;

TYPEDEF_POINTER(CdaLOCALTIME)
TYPEDEF_POINTER(CdaCLOCALTIME)

#define CdaLOCALTIME_MAX	CdaUINT32_MAX

//******************************************************************************
// File paths
//******************************************************************************

typedef	CdaCHAR	 CdaPATH;
typedef	CdaCCHAR CdaCPATH;

TYPEDEF_POINTER(CdaPATH)
TYPEDEF_POINTER(CdaCPATH)

//******************************************************************************
// Integer types stored in LBO format (least significant byte at low memory 
// address); used when data may be accessed from different environments
//******************************************************************************

typedef	struct	tagLboUINT32	CdaLboUINT32;
struct	tagLboUINT32
{
	CdaUINT32	Value;
};
typedef	struct	tagLboSINT32	CdaLboSINT32;
struct	tagLboSINT32
{
	CdaSINT32	Value;
};

typedef	struct	tagLboUINT16	CdaLboUINT16;
struct	tagLboUINT16
{
	CdaUINT16	Value;
};
typedef	struct	tagLboSINT16	CdaLboSINT16;
struct	tagLboSINT16
{
	CdaSINT16	Value;
};

typedef	struct	tagLboDAY		CdaLboDAY;
struct	tagLboDAY
{
	CdaDAY		Value;
};

TYPEDEF_POINTER(CdaLboUINT32)
TYPEDEF_POINTER(CdaLboSINT32)
TYPEDEF_POINTER(CdaLboUINT16)
TYPEDEF_POINTER(CdaLboSINT16)
TYPEDEF_POINTER(CdaLboDAY)

//******************************************************************************
// Handle
//******************************************************************************

typedef CdaUINT32	CdaHANDLE;
TYPEDEF_POINTER(CdaHANDLE)

//******************************************************************************
// Helper macros
//******************************************************************************

#define CdaMINIMUM_OF(x,y) ( ((x) < (y))? (x) : (y) )
#define CdaMAXIMUM_OF(x,y) ( ((x) > (y))? (x) : (y) )
										
//******************************************************************************
// Function calling
//******************************************************************************

#define PUBLIC
#define PRIVATE		static

#define CDAAPI		
#define CDASTDAPI	__stdcall

//******************************************************************************
// Environment - integer byte ordering
//******************************************************************************

// Return the byte which will be stored at the low memory address

#define CdaIboLoMemByte32(x32) ( (CdaUINT8)(x32 & 0xFF) )
#define CdaIboLoMemByte16(x16) ( (CdaUINT8)(x16 & 0xFF) )

// Return the byte which will be stored at the high memory address

#define CdaIboHiMemByte32(x32) ( (CdaUINT8)(x32 >> 24) )
#define CdaIboHiMemByte16(x16) ( (CdaUINT8)(x16 >> 8) )

//******************************************************************************
// Miscellaneous Data Types
//******************************************************************************

// @type CdaSCOPT | Subscription Change Options.
//	These options define what information is to be included in the 
//	Request Code or codes for a Change Subscription helpdesk transaction.  
//	This, in turn, determines the number and type of codes required.
typedef	CdaUINT16	CdaSCOPT;
// @type CdaCSCOPT | A constant <t CdaSCOPT>
typedef	CdaCUINT16	CdaCSCOPT;

// @type PCdaSCOPT | A pointer to a <t CdaSCOPT>
TYPEDEF_POINTER(CdaSCOPT)
// @type PCdaCSCOPT | A pointer to a constant <t CdaSCOPT>
TYPEDEF_POINTER(CdaCSCOPT)

// @type CdaHGROUP | Handle to a group, obtained by calling <f CdaGroupOpen>
// and used in all other <f CdaGroup*> functions
typedef PCdaVOID		CdaHGROUP;
// @type PCdaHGROUP | A pointer to a <t CdaHGROUP>
TYPEDEF_POINTER(CdaHGROUP)


// @type CdaLENGTH | Integer type used to indicate the length of a piece of memory
// or a string
typedef	CdaUINT32	CdaLENGTH;
// @type CdaCLENGTH | A constant <t CdaLENGTH>
typedef	CdaCUINT32	CdaCLENGTH;

#define CdaLENGTH_MAX CdaUINT32_MAX

// @type PCdaLENGTH | A pointer to a <t CdaLENGTH>
TYPEDEF_POINTER(CdaLENGTH)
// @type PCdaLENGTH | A pointer to a constant <t CdaLENGTH>
TYPEDEF_POINTER(CdaCLENGTH)

// @const MIN_CdaLENGTH | The minimum value that can be stored in a CdaLENGTH
#define	MIN_CdaLENGTH	(0x00000000L)
// @const MAX_CdaLENGTH | The maximum value that can be stored in a CdaLENGTH
#define	MAX_CdaLENGTH	(0xffffffffL)

// @type CdaTYPE | Integer type used to hold flags or enum values
typedef	CdaSINT16	CdaTYPE;
// @type CdaCTYPE | A constant <t CdaTYPE>
typedef	CdaCSINT16	CdaCTYPE;

// @type PCdaTYPE | A pointer to a <t CdaTYPE>
TYPEDEF_POINTER(CdaTYPE)
// @type PCdaTYPE | A pointer to a constant <t CdaTYPE>
TYPEDEF_POINTER(CdaCTYPE)

// @type CdaOS_ERROR | Operating system error type
typedef	CdaUINT32	CdaOS_ERROR;
// @type PCdaOS_ERROR | A pointer to a <t CdaOS_ERROR>
TYPEDEF_POINTER(CdaOS_ERROR)

// @const CdaOS_ERROR_NONE | The error value which represents 'no error'
#define	CdaOS_ERROR_NONE		(0L)

// type of return value from CD-Secure API functions 
// (always equates to the platorm's "int" type, holds values from cdaerr.h)
typedef	CdaSINT32	CdaRETVAL;
TYPEDEF_POINTER(CdaRETVAL)

// type of other return values 
typedef	CdaUINT32	CdaRESULT;
TYPEDEF_POINTER(CdaRESULT)

// a file offset 
typedef	CdaUINT32	CdaFILEOFFSET;
TYPEDEF_POINTER(CdaFILEOFFSET)
#define CdaFILEOFFSET_MAX CdaUINT32_MAX

// calling convention for API functions
#define CDAAPI
#define CDASTDAPI			__stdcall
#define CDAAPICALLBACK
#define CDASTDAPICALLBACK	__stdcall

//******************************************************************************
// UNICODE/MBCS/SBCS string abstraction through CdaTCHAR
//******************************************************************************

#ifdef _UNICODE

// UNICODE

#define CdaTCHAR	CdaWCHAR
#define CdaCTCHAR	CdaCWCHAR
#define PCdaTCHAR	PCdaWCHAR
#define PCdaCTCHAR	PCdaCWCHAR

#else	// ndef _UNICODE 

#ifdef _MBCS

// MBCS

#define CdaTCHAR	CdaMCHAR
#define CdaCTCHAR	CdaCMCHAR
#define PCdaTCHAR	PCdaMCHAR
#define PCdaCTCHAR	PCdaCMCHAR

#else	// ndef _MBCS 

// SBCS

#define CdaTCHAR	CdaCHAR
#define CdaCTCHAR	CdaCCHAR
#define PCdaTCHAR	PCdaCHAR
#define PCdaCTCHAR	PCdaCCHAR

#endif	// def _MBCS
#endif	// def _UNICODE

#endif	// CDADECL_H
