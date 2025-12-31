/*---------------------------------------------------------------------*

Project:  portable function
File:     lzo_free.h

Copyright 2001 Nintendo.  All rights reserved.

These coded instructions, statements and computer programs contain
proprietary information of Nintendo of America Inc. and/or Nintendo
Company Ltd., and are protected by Federal copyright law.  They may
not be disclosed to third parties or copied or duplicated in any form,
in whole or in part, without the prior written consent of Nintendo.

Author:  Steve Rabin
Created: March 14th, 2001

-----------------------------------------------------------------------*/

#ifndef __LZO_FREE_H
#define __LZO_FREE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


int FreeDecompress_LZO1X_999( const unsigned char * in, const unsigned int in_length, unsigned char* out, unsigned int* out_length );


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __LZO_FREE_H