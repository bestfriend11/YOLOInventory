//////////////////////////////////////////////////////////////////////////
//
// File     :  BuildCopy_Debug.js
// Author(s):  Scott Bilas, Marsh Macy
//
//
// PURPOSE:	JScript that copies debug executable and pdb to server
//			into directory named with build time
//
// Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
//------------------------------------------------------------------------
//  $Revision:: $              $Date:$
//------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////


// constants
	var BUILDS_DIR	= "K:\\";
	var USERNAME	= "burns";
	var EXE			= "P:\\vss_db_tattoo\\GPG\\Out\\DungeonSiegeD.exe";
	var PDB			= "P:\\vss_db_tattoo\\GPG\\Out\\DungeonSiegeD.pdb";

// global
	var fso;
	var dirDebug;

fso = WScript.CreateObject( "Scripting.FileSystemObject" );

// create dest dir on server
dirDebug = BUILDS_DIR + VerTime() + "_" + USERNAME + "_debug";
fso.CreateFolder ( dirDebug );

// copy executable and pdb into the above folder
fso.CopyFile ( EXE, dirDebug + "\\" );
fso.CopyFile ( PDB, dirDebug + "\\" );

///// utility //////////////////////////////////

	//pieces together the version inf0z
	function VerTime()
	{
		var curDate = new Date();
		var mon 	= curDate.getMonth() + 1;
		var day 	= curDate.getDate();
		var year 	= curDate.getFullYear();
		var hour	= curDate.getHours();
		var minute = curDate.getMinutes();

		return NumToStr( mon )
			+ "-" + NumToStr( day )
			+ "-" + NumToStr( year % 100 )
			+ "_" + NumToStr( hour ) + NumToStr( minute );
	}

	//precede number with 0 if less than 10 [stolen from UpdateVersion.js]
	function NumToStr( num )
	{
		var str = "";
		if ( num < 10 )
		{
			str = "0";
		}
		return ( str + num.toString() );
	}
