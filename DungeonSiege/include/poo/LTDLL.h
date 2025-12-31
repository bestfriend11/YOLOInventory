/***************************************************************************************************
 * $Archive: /Eng/CopyLok/ltdll/ltdll.h $
 *
 * $Revision: 26 $
 *
 * Description:
 *		This file contains the exported function interface for the SafeDisc LT DLL. This DLL
 *		allows the installer to authenticate a CD, DVD or network before installing a product.
 *		It also provides decryption facilities. 
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

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif	


#ifndef LTDLL_H
#define LTDLL_H


//**************************************************************************************************
// Global definitions.
//**************************************************************************************************

// Define the DLL's version number.
#define LTDLL_VERSIONNUMBER					((CdaUINT32)1)


//**************************************************************************************************
// SUCCESS Reason Codes.
//**************************************************************************************************

// From the authentication function, the following successful codes may be returned. 
// These codes specify which of the 3 authentication methods succeeded.
#define LTDLL_NETWORK_AUTHENTICATED			((CdaUINT32)100)
#define LTDLL_DVD_AUTHENTICATED				((CdaUINT32)101)
#define LTDLL_CD_AUTHENTICATED				((CdaUINT32)102)

// CD Authentication on sector read - this should be accepted as valid CD authentication.
#define LTDLL_FAIL_SAFE_CD_AUTHENTICATED	((CdaUINT32)103)


// This reason code is returned from a successful call to the unwrap function.
#define LTDLL_SUCCESS						((CdaUINT32)104)


//**************************************************************************************************
// FAILURE Reason Codes.
//**************************************************************************************************

// The specified source directory was NULL.
#define LTDLL_BAD_SOURCE_DIR				((CdaUINT32)150)

// The specified source directory does not exist.
#define LTDLL_SOURCE_DOESNT_EXIST			((CdaUINT32)152)

// The specified file does not exist.
#define LTDLL_FILE_DOESNT_EXIST				((CdaUINT32)154)

// Failed CD authentication because the source path was a local drive but no a CDROM drive.
#define LTDLL_NOT_A_CDROM_DRIVE				((CdaUINT32)155)

// Failed to find support files that were expected to be in the source directory. These include
// 00000001.lt1, clcd16.dll and clcd32.dll.
#define LTDLL_FAILED_TO_FIND_FILE			((CdaUINT32)157)

// Authentication failed because an old version of a SafeDisc disc was in the drive.
#define LTDLL_OLD_SAFEDISC_VERSION			((CdaUINT32)158)

// Authentication failed because the source dir contains a mismatched product serial number.
#define LTDLL_WRONG_PRODUCT					((CdaUINT32)159)

// Authentication failed because the 00000001.lt1 file contains invalid data.
#define LTDLL_INVALID_FILE_DATA				((CdaUINT32)160)

// CD Authentication failed because the CD is a gold copy.
#define LTDLL_INVALID_CD					((CdaUINT32)162)

// Failed to load the support files clcd32.dll and clcd16.dll. These are required for CD access
// under 95/98 if ASPI is not functional. These files must be present in the same directory as
// ltdll.dll is loaded from.
#define LTDLL_FAILED_TO_LOAD_CLCD32			((CdaUINT32)167)

// Failed to dynamically allocate memory for authentication process. System low on resources.
// Very little memory is allocated this way (of the order of 1K) so this should not occur in practice.
#define LTDLL_OUT_OF_MEMORY					((CdaUINT32)161)

// Failures when unwrapping the protected file.
#define LTDLL_FAILED_TO_SET_FILE_POS		((CdaUINT32)171)
#define LTDLL_FAILED_TO_WRITE_TO_FILE		((CdaUINT32)172)
#define LTDLL_FAILED_TO_OPEN_FILE			((CdaUINT32)173)

// Src directory cannot be translated from UNC to drive:\... format.
#define LTDLL_CANNOT_TRANSLATE_SRC			((CdaUINT32)177)

// The Unwrap function was passed a NULL pointer.
#define LTDLL_NULL_FILE_TO_UNWRAP			((CdaUINT32)179)

// The initialisation function has not been called.
#define LTDLL_UNINITIALISED					((CdaUINT32)181)


//**************************************************************************************************
// Global type definitions.
//**************************************************************************************************

// Define the type definitions for the interface function described below.
typedef CdaBOOL (*PFN_LTDLL_INITIALISE)(CdaUINT32  VersionNumber);
typedef CdaBOOL (*PFN_LTDLL_AUTHENTICATE)(CdaTCHAR  *pSourceDirectory, CdaUINT32 *pReasonCode);
typedef CdaBOOL (*PFN_LTDLL_UNWRAP)(CdaTCHAR  *pFileToUnwrap, CdaUINT32  *pReasonCode);


//**************************************************************************************************
// Exported Function Prototypes.
//**************************************************************************************************


/***************************************************************************************************

 NAME: 		LTDLL_Initialise

 DESCRIPTION:
	This function is used to check that correct version of the DLL is being used with callers 
	program.
	This function checks the supplied version number against the version number of the
	DLL and if they match, CdaTRUE is returned. Otherwise, CdaFALSE is returned.  
	The caller is expected to call this function with LTDLL_VERSIONNUMBER as the parameter.
	
 PARAMETERS: 
	VersionNumber	- The expected version number of the DLL
 	
 RETURN VALUES:
	CdaTRUE		- If the supplied version number matched the DLL's version number.
	CdaFALSE	- If the supplied version number did not match the DLLs version number.

***************************************************************************************************/ 

_declspec(dllexport) CdaBOOL LTDLL_Initialise (CdaUINT32  VersionNumber);


/***************************************************************************************************

 NAME: 		LTDLL_Authenticate

 DESCRIPTION:
	This function authenticates a Network drive, DVD or CD depending on the Source path provided. 

 PARAMETERS: 
	pSourceDirectory	- Points to the source directory for the install process. The source 
						  directory must contain the file 00000001.lt1 produced as part of the 
						  SafeDisc dataprep process. 
	pReasonCode			- Pointer to memory to receive the reason code of the authentication.
						  A value is returned in all cases.
 	
 RETURN VALUES:
	CdaTRUE		- If the authentication was successfull.  The reason code will be set with one
				  of the following reasons:

		LTDLL_NETWORK_AUTHENTICATED			- Network authentication completed successfully.
		LTDLL_DVD_AUTHENTICATED				- DVD authentication completed successfully.
		LTDLL_CD_AUTHENTICATED				- CD authentication completed succesfully.
		LTDLL_FAIL_SAFE_CD_AUTHENTICATED	- Unable to determine if the CD has been copied.


	CdaFALSE	- If the authentication was not successfull.  The reason code will be set with
				  one of the following reasons for the failure:

		LTDLL_FAILED_TO_LOAD_CLCD32	- The CLCD32.DLL or CLCD16.DLL dll's could not be loaded.
		LTDLL_BAD_SOURCE_DIR		- The source directory is a NULL pointer.
		LTDLL_SOURCE_DOESNT_EXIST	- The source directory does not exist.
		LTDLL_OUT_OF_MEMORY			- Failed to allocate dynamic memory in function.
		LTDLL_CANNOT_TRANSLATE_SRC	- Supplied UNC Path could not be translated into drive 
									  letter form.
		LTDLL_NOT_A_CDROM_DRIVE		- The source directory does not point to the root of a CD.
		LTDLL_FAILED_TO_FIND_FILE	- Failed to find the 00000001.lt1 file in the source 
									  directory.
		LTDLL_OLD_SAFEDISC_VERSION	- Old version of the SafeDisc Disc was detected.
		LTDLL_WRONG_PRODUCT			- The SafeDisc Disc serial number does not match the DLL's
									  serial number.
		LTDLL_INVALID_FILE_DATA		- The 00000001.lt1 contains invalid data.
		LTDLL_INVALID_CD			- The source CD has been detected as a copied CD.
		LTDLL_UNINITIALISED			- The DLL has not been initialised.  LTDLL_Initialise() must 
									  be called successfully before this function.

***************************************************************************************************/ 

_declspec(dllexport) CdaBOOL LTDLL_Authenticate (CdaTCHAR  *pSourceDirectory, CdaUINT32 *pReasonCode);


/***************************************************************************************************

 NAME: 		LTDLL_Unwrap

 DESCRIPTION:
	This function decrypts the specified protected exe in situ.
	
 PARAMETERS: 
	pFileToUnwrap		- Points to the full path of the target file to be decrypted.
	pReasonCode			- Pointer to memory to receive the reason code for the unwrap failure.
 	
 RETURN VALUES:
	CdaTRUE		- If the unwrap was successfull.  The reason code will be set with one of the 
				  following reasons:

		LTDLL_SUCCESS					- The unwrap was a success.


	CdaFALSE	- If the unwrap was not successfull.  The reason code will be set with
				  one of the following reasons for the failure:

		LTDLL_NULL_FILE_TO_UNWRAP		- The supplied pointer to the path was NULL.
		LTDLL_FILE_DOESNT_EXIST			- The file doesn't exist.
		LTDLL_OUT_OF_MEMORY				- Failed to allocate dynamic memory.
		LTDLL_FAILED_TO_SET_FILE_POS	- Failed to set the file position.
		LTDLL_FAILED_TO_WRITE_TO_FILE	- Failed to write the decryped information to the file.
		LTDLL_FAILED_TO_OPEN_FILE		- Failed to open the file - Possibly read only.
		LTDLL_UNINITIALISED				- The DLL has not been initialised.  LTDLL_Initialise() must 
										  be called successfully before this function.

***************************************************************************************************/ 

_declspec(dllexport) CdaBOOL LTDLL_Unwrap (CdaTCHAR  *pFileToUnwrap, CdaUINT32  *pReasonCode);


#endif // LTDLL_H

#ifdef __cplusplus
}
#endif	
