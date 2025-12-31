/***************************************************************************************************
 * $Archive: /Eng/CopyLok/lthelper/lthelper.h $ 
 *
 * $Revision: 4 $
 *
 * Description:
 *		This is the header file for the SafeDisc LT 'C' file.  This file holds the interface 
 *		to the helper functions for the SafeDisc LT API.  It is expected to be used in
 *		association with installation DLLs.
 *		
 *
 ***************************************************************************************************
 * NOTIFICATION OF COPYRIGHT AND OWNERSHIP OF SOFTWARE:
 *
 * Copyright (c) 2000, C-Dilla Ltd.  All Rights Reserved.
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


#ifndef _LTHELPER_H_INCLUDED_
#define _LTHELPER_H_INCLUDED_


#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif	

//*****************************************************************************
// Interface function prototypes.
//*****************************************************************************


/******************************************************************************
    
 NAME:   		LTHELPER_FindLTDLL
   
 DESCRIPTION:
 	This function searches for an ltdll.dll in the supplied path.  The rules
	used to locate the DLL are:
	1)  Look in the current directory.
	2)  Look in the root directory.
	3)  Progressively look in each parent directory until located.
   
 PARAMETERS:
	pCurrentDirectory	- The full path - UNC format or drive letter format -
						  of the directory to start looking from.  The supplied
						  buffer is used to return the path to the ltdll.dll
						  file if found.  Otherwise, the returned contents 
						  are invalid.
	   
 RETURN VALUES:
 	CdaTRUE		- If the DLL was located.  The buffer pointer to by 
				  pCurrentDirectory contains the path to the ltdll.dll file.
	CdaFALSE	- If the DLL could not be located.  The contents of the buffer
				  pointed to by pCurrentDirectory are not valid.

*******************************************************************************/

CdaBOOL LTHELPER_FindLTDLL(CdaTCHAR *pCurrentDirectory);


#ifdef __cplusplus
}
#endif	

#endif // _LTHELPER_H_INCLUDED_