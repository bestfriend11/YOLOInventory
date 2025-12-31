//////////////////////////////////////////////////////////////////////////////
//
// File     :  nema_persist.h
// Author(s):  Scott Bilas
//
// Summary  :  Contains persistence support functions for Nema.
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef __NEMA_PERSIST_H
#define __NEMA_PERSIST_H

//////////////////////////////////////////////////////////////////////////////

#include "FuBiTraits.h"
#include "nema_types.h"

//////////////////////////////////////////////////////////////////////////////

FUBI_DECLARE_POINTERCLASS_TRAITS( nema::HAspect, nema::HAspect::Null );
FUBI_DECLARE_POINTERCLASS_TRAITS( nema::HPRSKeys, nema::HPRSKeys::Null );

FUBI_DECLARE_XFER_TRAITS( nema::Aspect* )
{
	static bool XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj );
};

template <typename COLL, typename T>
bool XferCollPtr( FuBi::PersistContext& persist, const char* keyName, COLL& coll, T*& ptr )
{
	size_t distance = 0;
	if ( persist.IsSaving() )
	{
		COLL::const_iterator i, ibegin = coll.begin(), iend = coll.end();
		for ( i = ibegin ; i != iend ; ++i, ++distance )
		{
			if ( ptr == *i )
			{
				break;
			}
		}
		gpassert( (ptr == NULL) || (i != iend) );
	}

	persist.Xfer( keyName, distance );

	if ( persist.IsRestoring() )
	{
		COLL::const_iterator i = coll.begin();
		std::advance( i, distance );
		ptr = (i == coll.end()) ? NULL : *i;
	}

	return ( true );
}

//////////////////////////////////////////////////////////////////////////////

#endif  // __NEMA_PERSIST_H

//////////////////////////////////////////////////////////////////////////////
