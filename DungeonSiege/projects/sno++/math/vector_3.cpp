//////////////////////////////////////////////////////////////////////////////
//
// File     :  vector_3.cpp
// Author(s):  Scott Bilas
//
// Copyright © 1999 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "Precomp_GPCore.h"
#include "vector_3.h"

//////////////////////////////////////////////////////////////////////////////
// struct vector_3 implementation

const vector_3 vector_3::ZERO   ( 0.0f, 0.0f, 0.0f );
const vector_3 vector_3::ONE    ( 1.0f, 1.0f, 1.0f );
const vector_3 vector_3::INVALID( FLOAT_INFINITE, FLOAT_INFINITE, FLOAT_INFINITE );

//////////////////////////////////////////////////////////////////////////////
