//////////////////////////////////////////////////////////////////////////////
//
// File     :  gpmem_new_off.h
// Author(s):  Scott Bilas
//
// Summary  :  #include this to turn off #defines for mem alloc wrappers
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#if GPMEM_DBG_NEW || (GP_ENABLE_STATS && !GP_DEBUG)

#ifdef new
#undef new
#endif

#ifdef malloc
#undef malloc
#endif
#ifdef realloc
#undef realloc
#endif
#ifdef free
#undef free
#endif

#elif GP_DEBUG

#ifdef realloc
#undef realloc
#endif

#endif

//////////////////////////////////////////////////////////////////////////////
