#include "gpcore.h"

#include "fuel.h"
#include "fueldb.h"
#include "gpmem.h"
#include "filesys.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "filesysutils.h"
#include "stringtool.h"

#include <iostream>
#include <wtypes.h>

using namespace std;
using namespace BinFuel;




void TestTextFuel( int argc, char * argv[] )
{
	if( argc < 3 )
	{
		gperror( "Invalid arguments passed in." );
		return;
	}

	// Initialize FileSys.

	gpstring sInDirA( argv[1] );
	gpstring sOutDirA( argv[2] );

	// initialize old Tank stuff
	stringtool::AppendTrailingBackslash( sInDirA );
	stringtool::AppendTrailingBackslash( sOutDirA );

	gpstring sInDirB("");
	gpstring sOutDirB("");

	if( argc == 5 )
	{
		sInDirB		= argv[3];
		sOutDirB	= argv[4];
		stringtool::AppendTrailingBackslash( sInDirB );
		stringtool::AppendTrailingBackslash( sOutDirB );
	}

	//----- begin test -----------------------------------------------

	gpgeneric( "Fuel test started.  Using .gas as fuel\n\n" );

	//----- read everything from root dir

	DWORD TickCountBeforeRead = GetTickCount();


	//////////////////////////////////////////////////
	//	read Db A

	gpgeneric( "reading into Db A\n\n" );

	FuelSys * mFuelSys  = new FuelSys;

	TextFuelDB * dbA = gFuelSys.AddTextDb( "default" );
	dbA->Init( FuelDB::OPTION_READ|FuelDB::OPTION_WRITE, "", sOutDirA );
	FuelHandle rootA( "root" );


	//////////////////////////////////////////////////
	//	read Db B

	TextFuelDB * dbB = NULL;
	if( !sInDirB.empty() )
	{
		gpgeneric( "reading into Db B\n\n" );

		dbB = gFuelSys.AddTextDb( "load" );
		dbB->Init( FuelDB::OPTION_READ|FuelDB::OPTION_WRITE, sInDirB, sOutDirB );
		FuelHandle rootB( "::load:root" );
	}

	
	//////////////////////////////////////////////////
	//	display read stats

	std::cout << "Reading from Fuel root =" << argv[1] << endl;
	std::cout << "Parsed " << dbA->GetNumFilesLoaded() << " files." << endl;
	std::cout << "Parsed " << dbA->GetNumBlocksLoaded() << " blocks." << endl;
	std::cout << "Parsed " << dbA->GetLogicalLinesLoaded() << " logical lines." << endl;
	std::cout << ends;

	DWORD TickCountAfterRead	= GetTickCount();
	DWORD TickCountBeforeWrite	= GetTickCount();

	gpstring basepath( argv[2] );

	if( !basepath.empty() )
	{
		//////////////////////////////////////////////////
		//	write Db A

		gpgeneric( "writing Db A\n\n" );

		std::cout << "Writing to Fuel root = " << sOutDirA << endl;
		dbA->SaveAll();

		//////////////////////////////////////////////////
		//	write Db B
		
		if( dbB )
		{
			gpgeneric( "writing Db B\n\n" );

			std::cout << "Writing to Fuel root = " << sOutDirB << endl;
//			dbB->SaveAll();
		}
	}
	
	//FuelHandle test( "P:\\TATTOO_ASSETS\\CDimage\\ui\\Interfaces\\Frontend\\credits_menu\\" );

	DWORD TickCountAfterWrite = GetTickCount();

	
	gpgeneric( "\n========================\n" );

	float ReadTime, WriteTime;

	ReadTime	= ( float( TickCountAfterRead ) - float( TickCountBeforeRead ) ) / 1000.0;
	WriteTime	= ( float( TickCountAfterWrite ) - float( TickCountBeforeWrite ) ) / 1000.0;

	gpgenericf(( "Time to read = %f sec, Time to write = %f sec\n", ReadTime, WriteTime ));

	gpgenericf(( "Read %1.0f logical lines per sec.\n", float( dbA->GetLogicalLinesLoaded() ) / ReadTime ));
	gpgenericf(( "Write %1.0f logical lines per sec.\n", float( dbA->GetLogicalLinesLoaded() ) / WriteTime ));

	gpgeneric( "========================\n" );
	delete mFuelSys;
}




void TestBinaryFuel( int argc, char * argv[] )
{
	if( argc < 3 )
	{
		gperror( "Invalid arguments passed in." );
		return;
	}

	gpstring sInDirA( argv[1] );
	gpstring sOutDirA( argv[2] );

	// initialize old Tank stuff
	stringtool::AppendTrailingBackslash( sInDirA );
	stringtool::AppendTrailingBackslash( sOutDirA );

	
	// make bin db
	FuelSys * mFuelSys  = new FuelSys;
	BinFuel::SetDictionaryPath( "" );
	BinaryFuelDB * dbA = gFuelSys.AddBinaryDb( "default_binary" );
	dbA->Init( FuelDB::OPTION_READ|FuelDB::OPTION_WRITE, "", sOutDirA );

	double timeBefore = GetGlobalSeconds();

//	dbA->BuildDictionary( 0xffff );
//	dbA->RecompileAll();

	FastFuelHandle root( "root" );

	root.GetRecursiveStaticChecksum();
//	root.Dump();

//	typedef std::set< FastFuelHandle > FuelSet;
//	FuelSet rootSet;
//	FastFuelHandleColl rootChildren;
//	root.ListChildren( rootChildren );
//	for( FastFuelHandleColl::iterator i = rootChildren.begin(); i != rootChildren.end(); ++i )
//	{
//		rootSet.insert( *i );
//	}

	FastFuelHandle t1a( "b" );
	FastFuelHandle t1b( "b" );

	FastFuelHandle t1( "b:bb1" );
	root.Dump();

	FastFuelHandle t2( "b:bb1:bbb1" );
	root.Dump();

	t1.Unload();
	root.Dump();

	FastFuelHandle t3( "b:bb1" );
	t3.Dump();
	t3.GetHeader();
	t3.Dump();

	FastFuelHandle badChild = t2.GetChildNamed( "invalid" );
	root.Dump();

	
	dbA->Dump();

	//FastFuelHandle test( "test" );
	//test.Dump();

#if 0
	/*
	Header * child = dbA->GetBlock( "dir_a:child_dir_a:cdaa:cdaa_child1" );
	gpassert( stricmp( GetName( child ), "cdaa_child1" ) == 0 );
	dbA->Dump( dbA->GetRootBlock() );
	*/

	FastFuelHandle expireTypes( "world:contentdb:components:common:expiration_class:choose" );

	FastFuelHandle th( "root" );
	gpassert( th.IsValid() );
	gpstring address = th.GetAddress();
	FastFuelHandleColl kids;
	th.ListChildren( kids );
	gpassert( kids.size() == 4 );

	FastFuelHandle dirccc = th.GetChildNamed( "dir_ccc" );
	gpassert( dirccc );

	FastFuelHandle dira = th.GetChildNamed( "dir_a" );
	gpassert( dira );

	FastFuelHandle cda3 = dira.GetChildNamed( "cda3" );
	gpassert( cda3 );

	FastFuelHandle typeBlock = dira.GetChildTyped( "aLieN" );
	gpassert( typeBlock );

	FastFuelHandle badBlock = dira.GetChildNamed( "zurbzurb321423" );
	gpassert( !badBlock );

	gpstring gggvalue;
	typeBlock.Get( "ggg", gggvalue );
	gpassert( gggvalue.same_no_case( "value_ggg" ) );

	gpstring zzzvalue;
	typeBlock.Get( "zzz", zzzvalue );
	gpassert( zzzvalue.same_no_case( "value_zzz" ) );

	gpstring cccvalue;
	typeBlock.Get( "ccc", cccvalue );
	gpassert( cccvalue.same_no_case( "value_ccc" ) );

	gpstring bbbvalue;
	typeBlock.Get( "bbb", bbbvalue );
	gpassert( bbbvalue.same_no_case( "value_bbb" ) );

	gpstring aaavalue;
	typeBlock.Get( "aaa", aaavalue );
	gpassert( aaavalue.same_no_case( "value_aaa" ) );

#endif


/*

// old test

#if 0

	FastFuelHandle th( "world:maps:map_world" );
	gpassert( th.IsValid() );
	FastFuelHandleColl kids;
	th.ListChildren( kids, -1 );
	gpgenericf(( "listed %d blocks\n", kids.size() ));

	th.Unload();
	kids.clear();
	th.ListChildren( kids, -1 );
	gpgenericf(( "listed %d blocks\n", kids.size() ));

	//th.Dump();
	//dbA->Dump();

#else

	TextFuelDB * dbB = gFuelSys.AddTextDb( "default" );
	dbB->Init( FuelDB::OPTION_READ | FuelDB::OPTION_JIT_READ, "" );
	FuelHandleList textList;
	FuelHandle textHandle( "art:bitmaps" );
	textHandle->ListChildBlocks( -1, 0, 0, textList );
	gpgenericf(( "listed %d blocks\n", textList.size() ));

#endif

*/

	double timeAfter = GetGlobalSeconds();
	gpgenericf(( "%f seconds elapsed\n", timeAfter - timeBefore ));

	//////////////////////////////////////////////////
	//	read Db B

	/*
	TextFuelDB * dbB = NULL;
	if( !sInDirB.empty() )
	{
		gpgeneric( "reading into Db B\n\n" );

		dbB = gFuelSys.AddTextDb( "load" );
		dbB->Init( FuelDB::OPTION_READ|FuelDB::OPTION_WRITE, sInDirB, sOutDirB );
		FuelHandle rootB( "::load:root" );
	}
	*/


	Delete( mFuelSys );
}




int main( int argc, char * argv[] )
{
	FileSys::IFileMgr * mFileMgr = NULL;

	// Initialize FileSys.

	gpstring sInDirA( argv[1] );
	stringtool::AppendTrailingBackslash( sInDirA );

	using FileSys::MasterPathFileMgrBuilder::PathColl;
	PathColl badPaths;

	FileSys::MasterPathFileMgrBuilder builder;
	if( !builder.SetHomePath( sInDirA ) )
	{
		badPaths.push_back( sInDirA );
	}

	builder.AddOverridePathSpec( "", &badPaths );

	// add it
	gpstring pathMsg;
	mFileMgr = builder.CreateMasterFileMgr( &pathMsg );

	//TestTextFuel( argc, argv );

	TestBinaryFuel( argc, argv );

	Delete( mFileMgr );

	return( 0 );
}
