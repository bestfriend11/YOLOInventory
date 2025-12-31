//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpmem_new_on.h
// Author(s):  Scott Bilas
//
// Summary  :  #include this to turn on the #defines for mem alloc wrappers
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if GPMEM_DBG_NEW

#ifndef new
#define new new ( __FILE__, __LINE__ )
#endif

#define malloc( SIZE )       gpmalloc( SIZE, __FILE__, __LINE__ )
#define realloc( MEM, SIZE ) gprealloc( MEM, SIZE, __FILE__, __LINE__ )
#define free( MEM )          gpfree( MEM )

#elif GP_DEBUG

#define realloc( MEM, SIZE ) gprealloc( MEM, SIZE )

#elif GP_ENABLE_STATS

#define malloc( SIZE )       gpmalloc( SIZE )
#define realloc( MEM, SIZE ) gprealloc( MEM, SIZE )
#define free( MEM )          gpfree( MEM )

#endif

//////////////////////////////////////////////////////////////////////////////
