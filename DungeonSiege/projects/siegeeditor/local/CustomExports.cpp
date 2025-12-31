//////////////////////////////////////////////////////////////////////////////
//
// File     :  CustomExports.inc
// Author(s):  Scott Bilas
//
// Summary  :  Contains custom local FuBi exports functions. Set this file to
//             read/write and modify it locally to add functions. This file
//             is ignored in regular builds and production modes.
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//----------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////////

#include "PrecompEditor.h"
#include "FuBiDefs.h"

//////////////////////////////////////////////////////////////////////////////
// class Custom declaration and implementation

class Custom : public AutoSingleton <Custom>
{
public:
	SET_NO_INHERITED( Custom );

	Custom( void )  {  }
   ~Custom( void )  {  }

// Insert functions here and they will appear in Skrit namespace "custom".

	// simple test function
	FUBI_EXPORT															// must always use FUBI_EXPORT tag
	float																// supports standard types for returns
	MyTest1(															// name of function as it appears in FuBi/Skrit (case-insensitive though)
	int var1, float var2, const char* var3 );							// supports standard types for parameters

	// another test function
	FUBI_EXPORT void MyTest2( int var );

	FUBI_SINGLETON_CLASS( Custom, "My custom local exports." );
	SET_NO_COPYING( Custom );
};

#define gCustom Custom::GetSingleton()

float Custom::MyTest1( int var1, float var2, const char* var3 )
{
	if ( var2 < 0 )
	{
		gperrorf(( "Custom::MyTest1: I want var2 to be >= 0. Right now it\'s %d.\n", var2 ));
	}
	else
	{
		gpgenericf(( "Custom::MyTest1: var1 is %d, var2 is %f, var3 is %s\n", var1, var2, var3 ));
	}

	// return a value
	return ( 12345.0f );
}

void Custom::MyTest2( int var )
{
	gpgenericf(( "Custom::MyTest2: var is %d.\n", var ));
}

//  Skrit to test out the above functions:
//
//  func$
//  {
//  	float f$ = Custom.MyTest1( 1, 2.598234, "bla" );
//  	debug.printf( "MyTest1 returned %f\n", f$ );
//  	Custom.MyTest2( 5 );
//  }

//////////////////////////////////////////////////////////////////////////////

