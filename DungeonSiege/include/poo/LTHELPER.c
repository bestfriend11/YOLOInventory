/***************************************************************************************************
 * $Archive: /Eng/CopyLok/lthelper/lthelper.c $ 
 *
 * $Revision: 3 $
 *
 * Description:
 *		This file holds helper functions for the SafeDisc LT API.  It is expected to be used in
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

#include <windows.h>

#include "cdadecl.h"
#include "cdatchar.h"
#include "lthelper.h"


//*****************************************************************************
// Local definitions.
//*****************************************************************************

// Define the name of the ltdll.
#define LTDLL_NAME				("ltdll.dll")



//*****************************************************************************
// Local function prototypes.
//*****************************************************************************

PRIVATE CdaVOID CopyPath(CdaTCHAR *pDestination, CdaTCHAR *pSource);
PRIVATE CdaVOID CopyPathRootOnly(CdaTCHAR *pDestination, CdaTCHAR *pSource);
PRIVATE CdaVOID RemoveLowestDirectory(CdaTCHAR *pPath);
PRIVATE CdaBOOL IsDLLInDirectory(CdaTCHAR *pDirectory);



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

CdaBOOL LTHELPER_FindLTDLL(CdaTCHAR *pCurrentDirectory)
{
	PCdaTCHAR pLocalPath;
	CdaUINT32 LocalPathSize;
	CdaBOOL   RetVal = CdaFALSE;

	// 
	// Calculate the size and allocate some dynamic memory as workspace.
	LocalPathSize = CdaTMemSize(pCurrentDirectory) + CdaTMemSize(LTDLL_NAME) + 4;
	pLocalPath = (PCdaTCHAR)malloc(LocalPathSize);

	// Was the allocation successfull?
	if (pLocalPath)
	{
		// Initialise the dynamically allocated memory.
		memset(pLocalPath, 0, CdaTMemSize(pCurrentDirectory));

		// Copy the supplied current directory into workspace.
		CopyPath(pLocalPath, pCurrentDirectory);

		while (strlen(pLocalPath))
		{
			// Can we find the DLL in the current directory.
			if (IsDLLInDirectory(pLocalPath) == CdaFALSE)
			{
				// Remove the lowest directory.
				RemoveLowestDirectory(pLocalPath);
			}
			else
			{
				// Found the file.
				RetVal = CdaTRUE;
				break;
			}
		}
	}

	// Return the path if the DLL was found.
	if (RetVal == CdaTRUE)
	{
		_tcscpy(pCurrentDirectory, pLocalPath);
	}

	// Free the allocated resources.
	free(pLocalPath);

	return RetVal;
}


/******************************************************************************
    
 NAME:   		CopyPath
   
 DESCRIPTION:
 	This function copies the source path into the destination path, removing
	the last character if it is a backslash.
   
 PARAMETERS:
	pDestination	- Pointer to the destination buffer.
	pSource			- Pointer to the source buffer.
	   
 RETURN VALUES:
	None.

*******************************************************************************/

PRIVATE CdaVOID CopyPath(CdaTCHAR *pDestination, CdaTCHAR *pSource)
{
	CdaTCHAR *pTmp;

	// Copy the source into the destination.
	_tcscpy(pDestination, pSource);

	// Is there a last backslash.
	pTmp = _tcsrchr(pDestination, '\\');

	if (pTmp == &pDestination[strlen(pDestination) - 1])
	{
		// Found backslash as last character.  Clear it.
		*pTmp = 0;
	}

	return;
}


/******************************************************************************
    
 NAME:   		CopyPathRootOnly
   
 DESCRIPTION:
 	This function copies the root of the source path into the destination path,
	removing the last character if it is a backslash.
	This function does not handle UNC paths of the form \\?\c:\fred or 
	\\?\UNC\fred
   
 PARAMETERS:
	pDestination	- Pointer to the destination buffer.
	pSource			- Pointer to the source buffer.
	   
 RETURN VALUES:
	None.

*******************************************************************************/

PRIVATE CdaVOID CopyPathRootOnly(CdaTCHAR *pDestination, CdaTCHAR *pSource)
{
	CdaTCHAR *pDst = pDestination;
	CdaTCHAR *pTmp;

	// Copy the whole path to the destination.
	CopyPath(pDestination, pSource);

	// do we have a UNC?
	if ((*pDestination == '\\') && (*(pDestination + 1) == '\\'))
	{
		// Skip these two characters
		pDst += 2;
	}

	// Find the next backslash.
	pTmp = _tcschr(pDst, '\\');
	if (pTmp)
	{
		// Terminate the string here.
		*pTmp = 0;
	}

	return;
}


/******************************************************************************
    
 NAME:   		RemoveLowestDirectory
   
 DESCRIPTION:
 	This function removes the lowest directory from the supplied path.
	If the resulting path is not a valid directory, then supplied path is set
	to a length of 0.
	This function does not handle UNC paths of the form \\?\c:\fred or 
	\\?\UNC\fred
   
 PARAMETERS:
	pPath	- Pointer to the path.
	   
 RETURN VALUES:
	None.

*******************************************************************************/

PRIVATE CdaVOID RemoveLowestDirectory(CdaTCHAR *pPath)
{
	CdaTCHAR *pTmp;

	// Locate the last backslash.
	pTmp = _tcsrchr(pPath, '\\');

	// Did we find one.
	if (pTmp)
	{
		// Have we come up against a UNC
		if ((*pPath == '\\') && ((pPath + 1) == pTmp))
		{
			// We have found the second backslash in the UNC.  Set the path to empty.
			*pPath = 0;
		}
		else
		{
			// Terminate the path at this point.
			*pTmp = 0;
		}
	}
	else
	{
		// No more found, set the path to empty.
		*pPath = 0;
	}

	return;
}


/******************************************************************************
    
 NAME:   		IsDLLInDirectory
   
 DESCRIPTION:
 	This function looks for the LTDLL in the supplied directory and returns
	CdaTRUE if found or CdaFALSE if not found.
   
 PARAMETERS:
	pDirectory	- Pointer to the directory.
	   
 RETURN VALUES:
	CdaTRUE		- DLL was found.
	CdaFALSE	- DLL was not found.

*******************************************************************************/

PRIVATE CdaBOOL IsDLLInDirectory(CdaTCHAR *pDirectory)
{
	CdaUINT32 PathSize;
	CdaTCHAR  *pDLLPath;
	CdaBOOL   RetVal;

	// Allocate some dynamic memory
	PathSize = CdaTMemSize(pDirectory) + CdaTMemSize(LTDLL_NAME) + 4;
	pDLLPath = (CdaTCHAR *)malloc(PathSize);

	if (pDLLPath)
	{
		// Copy the supplied path
		CopyPath(pDLLPath, pDirectory);

		// Can we find the DLL in the indicated directory.
		strcat(pDLLPath, "\\");
		strcat(pDLLPath, LTDLL_NAME);
		if (GetFileAttributes(pDLLPath) == 0xFFFFFFFF)
		{
			RetVal = CdaFALSE;
		}
		else
		{
			RetVal = CdaTRUE;
		}

		// Free the allocated resources
		free(pDLLPath);
	}
	else
	{
		RetVal = CdaFALSE;
	}

	return RetVal;
}
