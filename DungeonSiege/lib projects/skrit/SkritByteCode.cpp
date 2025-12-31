//////////////////////////////////////////////////////////////////////////////
//
// File     :  SkritByteCode.cpp
// Author(s):  Scott Bilas
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_Skrit.h"
#include "SkritByteCode.h"

namespace Skrit  {  // begin of namespace Skrit
namespace Op     {  // begin of namespace Op

//////////////////////////////////////////////////////////////////////////////
// Bytecode enum

CodeData sCodeData[] =
{
#	define   SKRITBYTECODE_IS_CPP 1
#	include "SkritBytecode.inc"
};

const CodeData& GetData( Code code )
{
	gpassert( (code >= 0) && (code < ELEMENT_COUNT( sCodeData )) );
	return ( sCodeData[ code ] );
}

static bool sCalcedTraits     = false;
static int  sMaxNameLen       = -1;
static int  sMaxDataSizeBytes = -1;

void UpdateTraits( void )
{
	sCalcedTraits = true;
	const CodeData* i, * begin = sCodeData, * end = begin + ELEMENT_COUNT( sCodeData);
	for ( i = begin ; i != end ; ++i )
	{
		maximize( sMaxNameLen      , i->m_NameLen       );
		maximize( sMaxDataSizeBytes, i->m_DataSizeBytes );
	}
}

int GetMaxNameLen( void )
{
	if ( !sCalcedTraits )  UpdateTraits();
	return ( sMaxNameLen );
}

int GetMaxDataSizeBytes( void )
{
	if ( !sCalcedTraits )  UpdateTraits();
	return ( sMaxDataSizeBytes );
}

//////////////////////////////////////////////////////////////////////////////

}  // end of namespace Op
}  // end of namespace Skrit

//////////////////////////////////////////////////////////////////////////////
