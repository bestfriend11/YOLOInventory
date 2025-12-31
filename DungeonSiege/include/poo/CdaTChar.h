/***************************************************************************************************
 * Name:	  CdaTChar.h
 *
 * $Revision: 8 $
 *
 * Description:
 *
 *		Character string abstraction.
 *
 *		This version uses the tchar options 
 *
 *			_UNICODE defined	Uicode strings
 *			_MBCS defined		Multibyte character strings
 *			neither defined		Single byte character strings
 *
 * Dependent on headers:
 *
 *		tchar.h
 *
 ***************************************************************************************************
 * NOTIFICATION OF COPYRIGHT AND OWNERSHIP OF SOFTWARE:
 *
 * Copyright (c) 1993-2000, C-Dilla Ltd.  All Rights Reserved.
 *
 * This computer program is the property of C-Dilla Ltd. of Reading, Berkshire, England
 * and Macrovision Corp. of Sunnyvale, California, U.S.A.  It may be used and copied only as 
 * specifically permitted under written agreement signed by C-Dilla Ltd. or Macrovision Corp.
 * 
 * Whether this program is copied in whole or in part and whether this program is copied in original
 * or in modified form, ALL COPIES OF THIS PROGRAM MUST DISPLAY THIS NOTICE OF COPYRIGHT AND 
 * OWNERSHIP IN FULL.
 ***************************************************************************************************
 *
 * $Nokeywords: $
 *
 **************************************************************************************************/
 
#ifndef CDATCHAR_H
#define CDATCHAR_H

// include the system tchar declarations

#include "tchar.h"

#ifdef _UNICODE

#define CdaTIsUnicode CdaTRUE

//******************************************************************************
// UNICODE
//******************************************************************************

#ifndef CdaTCHAR
	#define CdaTCHAR	CdaWCHAR
	#define CdaCTCHAR	CdaCWCHAR
	#define PCdaTCHAR	PCdaWCHAR
	#define PCdaCTCHAR	PCdaCWCHAR
#endif	// ndef CdaTCHAR

// Byte length of a string including terminator

#define CdaTMemSize(x)	((wcslen(x) + 1) * sizeof(CdaWCHAR))

// Number of CdaTCHARs in string (index of terminator at end of string)

#define CdaTEndIndex(x)	(wcslen(x))

// C-Dilla internal use only

#define CdaTIsValidPointer(x)	PfMemIsValidWideStringPointer(x)

#else	// ndef _UNICODE 

#define CdaTIsUnicode CdaFALSE

#ifdef _MBCS
//******************************************************************************
// MBCS
//******************************************************************************

#ifndef CdaTCHAR
	#define CdaTCHAR	CdaMCHAR
	#define CdaCTCHAR	CdaCMCHAR
	#define PCdaTCHAR	PCdaMCHAR
	#define PCdaCTCHAR	PCdaCMCHAR
#endif	// ndef CdaTCHAR

// Byte length of a string including terminator

#define CdaTMemSize(x)	(strlen(x) + 1)

// Number of CdaTCHARs in string (index of terminator at end of string)

#define CdaTEndIndex(x)	(strlen(x))

// C-Dilla internal use only

#define CdaTIsValidPointer(x)	PfMemIsValidASCIIZPointer((PCdaCCHAR)x)

#else	// ndef _MBCS 
//******************************************************************************
// SBCS
//******************************************************************************

#ifndef CdaTCHAR
	#define CdaTCHAR	CdaCHAR
	#define CdaCTCHAR	CdaCCHAR
	#define PCdaTCHAR	PCdaCHAR
	#define PCdaCTCHAR	PCdaCCHAR
#endif	// ndef CdaTCHAR

// Byte length of a string including terminator

#define CdaTMemSize(x)	(strlen(x) + 1)

// Number of CdaTCHARs in string (index of terminator at end of string)

#define CdaTEndIndex(x)	(strlen(x))

// C-Dilla internal use only

#define CdaTIsValidPointer	PfMemIsValidASCIIZPointer

#endif	// def _MBCS
#endif	// def _UNICODE

//******************************************************************************
// All: text macro (as CdaT("literal") )
//******************************************************************************

#define CdaT(x)		_T(x)

//******************************************************************************
// All: mappings to "tcs" functions
//******************************************************************************

#define CdaTLen		_tcslen
#define CdaTChr		_tcschr 
#define CdaTCpy		_tcscpy
#define CdaTNCpy	_tcsncpy
#define CdaTCat		_tcscat	
#define CdaTNCat	_tcsncat
#define CdaTPBrk	_tcspbrk
#define CdaTRChr	_tcsrchr
#define CdaTSpn		_tcsspn 
#define CdaTStr		_tcsstr 
#define CdaTTok		_tcstok 

#define CdaTCmp		_tcscmp 
#define CdaTColl	_tcscoll
#define CdaTXfrm	_tcsxfrm

// ctype-functions 

#define CdaTIsAlnum	_istalnum  
#define CdaTIsAlpha	_istalpha  
#define CdaTIsAscii	_istascii
#define CdaTIsCntrl	_istcntrl
#define CdaTIsDigit	_istdigit  
#define CdaTIsGraph	_istgraph  
#define CdaTIsLower	_istlower  
#define CdaTIsPrint	_istprint  
#define CdaTIsPunct	_istpunct  
#define CdaTIsSpace	_istspace  
#define CdaTIsUpper	_istupper  

#define CdaTToUpper	_totupper  
#define CdaTToLower	_totlower  

//******************************************************************************
// All: mappings to "tchar" functions (non-ANSI)
//******************************************************************************

#define CdaTDec		_tcsdec    
#define CdaTInc		_tcsinc    
#define CdaTNBCnt	_tcsnbcnt  
#define CdaTNCCnt	_tcsnccnt  
#define CdaTNextC	_tcsnextc  
#define CdaTNInc	_tcsninc   
#define CdaTSPnp	_tcsspnp   

#define CdaTLwr		_tcslwr    
#define CdaTUpr		_tcsupr    

#define CdaTIsLegal _istlegal
#define CdaTIsLead	_istlead
#define CdaTIsLeadByte	_istleadbyte

#endif
