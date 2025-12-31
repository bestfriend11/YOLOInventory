//////////////////////////////////////////////////////////////////////////////
//
// File     :  nema_persist.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2001 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_NeMa.h"
#include "nema_persist.h"

#include "FuBiPersist.h"

//////////////////////////////////////////////////////////////////////////////
// Aspect* traits implementation

bool FuBi::Traits <nema::Aspect*> :: XferDef( PersistContext& persist, eXfer xfer, const char* key, Type& obj )
{
	nema::HAspect hasp;
	if ( persist.IsSaving() && (obj != NULL) )
	{
		hasp = obj->GetHandle();
	}

	bool rc = persist.Xfer( xfer, key, hasp );

	if ( persist.IsRestoring() )
	{
		obj = hasp.Get();
	}

	return ( rc );
}

//////////////////////////////////////////////////////////////////////////////
