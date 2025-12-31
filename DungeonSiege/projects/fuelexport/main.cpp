// Project  :  FuelExport
// Author(s):  David Tomandl
//
// Summary  :  Along with parse_jake_spreadsheet.pl, converts Jake's uber-spreadsheet
//				of weapons and armor into .gas files.
//				Specifically, this reads in a backtick delimited text file, then looks
//				for specific templates, changing their properties.

#include <windows.h>

#include "gpcore.h"
#include "fuel.h"
#include "fueldb.h"
#include "gpmem.h"
#include "filesys.h"
#include "pathfilemgr.h"
#include "masterfilemgr.h"
#include "filesysutils.h"
#include "stringtool.h"
#include "config.h"

#include <wtypes.h>
#include <stdio.h>
#include <iostream>

using namespace std;
using namespace FileSys;


typedef std::map< gpstring, FuelHandle, istring_less > TemplateMap; // templateName, fuel handle
TemplateMap Templates;

int main( int argc, char * argv[] )
{
	if( (argc != 3) /* && (argc != 4) */  )
	{
		std::cout << "FuelExport <Import File> <Fuel Path>" << endl << endl;
		return( 0 );
	}

	gpstring fileName( argv[1] );
	gpstring fuelPath = argv[2];

	/* Just a little experiment that...failed.
	gpstring debugFileName = argv[3];
	FileHandle debugFileHandle( debugFileName, USAGE_READWRITE );
	debugFileHandle.WriteText( "This is a test.\n" );
	debugFileHandle.Close();
	*/

	// Initialize file manager for fuel
	FileSys::MasterFileMgr * pMasterFileMgr	= new MasterFileMgr();
	FileSys::PathFileMgr * pPathFileMgr		= new PathFileMgr( fuelPath );
	pMasterFileMgr->AddFileMgr( pPathFileMgr, true );

	// Create our fuel database
	FuelSys * pFuelSys = new FuelSys;
	TextFuelDB * pTextFuelDB = pFuelSys->AddTextDb( "default" );
	pTextFuelDB->Init( TextFuelDB::OPTION_JIT_READ | TextFuelDB::OPTION_WRITE /* | TextFuelDB::OPTION_NO_READ_DEV_BLOCKS */, fuelPath, fuelPath );

	// Create a map of the templates
	FuelHandle hTemplates( "world:contentdb:templates" );
	if ( hTemplates )
	{
		FuelHandleList hlTemplates = hTemplates->ListChildBlocks( -1 );
		for ( FuelHandleList::iterator i = hlTemplates.begin(); i != hlTemplates.end(); ++i )
		{
			if ( gpstring( "template" ).same_no_case( (*i)->GetType() ) )
			{
				Templates.insert( TemplateMap::value_type( gpstring( (*i)->GetName() ), (*i) ) );
			}
		}
	}

	// Read in file
	std::cout << "Reading import file..." << endl;

	FileHandle readFile;
	if ( readFile.Open( fileName ) )
	{
		gpstring readLine;
		while ( readFile.ReadLine( readLine ) )
		{

			// Quick summary:
			// 7 fields: weapon with pcontent
			// 6 fields: armor with pcontent
			// 5 fields: armor without pcontent

			//gpstring templateName;

			//If there are 7 fields, it's a weapon or bow (base name, modifier, min/max damage, min/max modifier, requirements)
			if ( stringtool::GetNumDelimitedStrings( readLine, '`' ) == 7 )
			{
				gpstring templateName, weaponType, minRequirements;
				float minDamage, maxDamage, minModifier, maxModifier;

				stringtool::GetDelimitedValue( readLine, '`', 0, templateName );
				stringtool::GetDelimitedValue( readLine, '`', 1, weaponType );
				stringtool::GetDelimitedValue( readLine, '`', 2, minDamage );
				stringtool::GetDelimitedValue( readLine, '`', 3, maxDamage );
				stringtool::GetDelimitedValue( readLine, '`', 4, minModifier );
				stringtool::GetDelimitedValue( readLine, '`', 5, maxModifier );
				stringtool::GetDelimitedValue( readLine, '`', 6, minRequirements );

				//debug only
				//cout << "These six fields were found: " << templateName << "," << weaponType << "," << minDamage << "," << maxDamage << "," << minModifier << "," << maxModifier << endl;
				//end debug output
				// find the template in the template map
				TemplateMap::iterator findTemplate = Templates.find( templateName );
				if ( findTemplate != Templates.end() )
				{
					// modify the template's properties:

					//debug output
					//cout << templateName << "," << weaponType << "," << minDamage << "," << maxDamage << "," << minModifier << "," << maxModifier << endl;
					//end debug output

					// Right off the top--a new (10/31/01) rule--if the min and max modifier are 0, set
					// allow_modifiers = false in [common]
					if ( ( minModifier == 0 ) && ( maxModifier == 0 ) )
					{
						//cout << templateName << ": min and max are both zero\n"; // debug output
						FuelHandle hCommon = findTemplate->second->GetChildBlock( "common" );
						if ( hCommon ) { hCommon->Set( "allow_modifiers", false ); }
					}
					
					//Rules: if the weaponType is "low" or "fun", just adjust the attack values
					if ( 
						( gpstring( weaponType ).same_no_case( "c_fun" ) ) || ( gpstring( weaponType ).same_no_case( "c_low" ) ) ||
						( gpstring( weaponType ).same_no_case( "o_fun" ) ) || ( gpstring( weaponType ).same_no_case( "o_low" ) ) 
						)
					{
						FuelHandle hAttack = findTemplate->second->GetChildBlock( "attack" );
						if ( hAttack )
						{
							hAttack->Set( "damage_max", maxDamage );
							hAttack->Set( "damage_min", minDamage );
						}
						FuelHandle hGui = findTemplate->second->GetChildBlock( "gui" );
						if (( hGui ) && !gpstring( minRequirements ).same_no_case( "none" ) )
						{
							hGui->Set( "equip_requirements", minRequirements, FVP_STRING ); 
						}
						else if (( hGui ) && gpstring( minRequirements ).same_no_case( "none" ) )
						{
							hGui->Remove( "equip_requirements" ); 
						}
					}
					else 
					{
						//if it's not "low" or "fun", adjust the attack values and modifier values
						//and to complicate things, they're usually in pcontent under their own section

						// special case: weaponType is c_avg
						// in this case: attack is messed with in the same manner as low/fun weapons
						// and "base" under pcontent gets the modifier values messed with
						// additional special case: weaponType is "unique", signifying a UNIQUE WEAPON
						if ( (gpstring( weaponType ).same_no_case( "c_avg" ) ) || (gpstring( weaponType ).same_no_case( "unique" ) ) )
						{
							FuelHandle hAttack = findTemplate->second->GetChildBlock( "attack" );
							if ( hAttack )
							{
								hAttack->Set( "damage_max", maxDamage );
								hAttack->Set( "damage_min", minDamage );
							}
							if (!gpstring( weaponType ).same_no_case( "unique" ) ) {
								weaponType = "base"; //this will force the next block to set the modifier values for
							}					 //C_avg weapons correctly
							FuelHandle hGui = findTemplate->second->GetChildBlock( "gui" );
							if (( hGui ) && !gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
							{
								hGui->Set( "equip_requirements", minRequirements, FVP_STRING ); 
							}
							else if (( hGui ) && gpstring( minRequirements ).same_no_case( "none" ) )
							{
								hGui->Remove( "equip_requirements" ); 
							}
						}

						FuelHandle hPcontent = findTemplate->second->GetChildBlock( "pcontent" );

						// For non-unique weapons
						if ( ( !gpstring( weaponType ).same_no_case( "unique" ) ) && ( hPcontent ) )
						{
							FuelHandle hWeaponType = hPcontent->GetChildBlock( weaponType );
							if ( hWeaponType )
							{
								if ( !gpstring( minRequirements ).same_no_case( "none" ) && !gpstring( weaponType ).same_no_case( "base" ) )
								{
									hWeaponType->Set( "equip_requirements", minRequirements, FVP_STRING ); 
								}
								else if ( gpstring( minRequirements ).same_no_case( "none" ) && !gpstring( weaponType ).same_no_case( "base" ) )
								{
									hWeaponType->Remove( "equip_requirements" ); 
								}
								hWeaponType->Set( "modifier_max", maxModifier );
								hWeaponType->Set( "modifier_min", minModifier );
/*
This got changed--now damage_max and _min get set without being in an attack block (if it's in pcontent)
								FuelHandle hAttack = hWeaponType->GetChildBlock( "attack" );
								if ( hAttack )
								{
									hAttack->Set( "damage_max", maxDamage );
									hAttack->Set( "damage_min", minDamage );
								}
*/
								FuelHandle hAttack = hWeaponType->GetChildBlock( "attack" );
								if ( hAttack )
								{
									hWeaponType->DestroyChildBlock( hAttack );
								}

								if ( gpstring( weaponType ).same_no_case( "base" ) )
								{
									hWeaponType->Remove( "damage_max" );
									hWeaponType->Remove( "damage_min" );
								}
								else
								{
									hWeaponType->Set( "damage_max", maxDamage );
									hWeaponType->Set( "damage_min", minDamage );
								}
							}
						}

						// If it's a unique weapon with pcontent, just set modifier min/max
						if ( ( gpstring( weaponType ).same_no_case( "unique" ) ) && ( hPcontent ) )
						{
							FuelHandle hWeaponType = hPcontent->GetChildBlock( "base" );
							if ( !hWeaponType )
							{
								hWeaponType = hPcontent->CreateChildBlock( "base" );
							}
							if ( hWeaponType )
							{
								hWeaponType->Set( "modifier_max", maxModifier );
								hWeaponType->Set( "modifier_min", minModifier );
							}
						}

						// If it's a unique weapon without pcontent, add a pcontent block, and set modifier min/max
						if ( ( gpstring( weaponType ).same_no_case( "unique" ) ) && !hPcontent)
						{
							FuelHandle hPcontent = findTemplate->second->CreateChildBlock( "pcontent" );
							FuelHandle hWeaponType = hPcontent->CreateChildBlock( "base" );
							hWeaponType->Set( "modifier_max", maxModifier );
							hWeaponType->Set( "modifier_min", minModifier );
						}
					}
				}//if ( findTemplate != Templates.end() )
			}//if ( stringtool::GetNumDelimitedStrings( readLine, '`' ) == 7 )


			//If there are 6 fields, it's gloves (or similar), (base name, modifier, armor value, min/max modifier, requirements)
			if ( stringtool::GetNumDelimitedStrings( readLine, '`' ) == 6 )
			{
				gpstring templateName, armorType, minRequirements;
				float defenseValue, minModifier, maxModifier;

				stringtool::GetDelimitedValue( readLine, '`', 0, templateName );
				stringtool::GetDelimitedValue( readLine, '`', 1, armorType );
				stringtool::GetDelimitedValue( readLine, '`', 2, defenseValue );
				stringtool::GetDelimitedValue( readLine, '`', 3, minModifier );
				stringtool::GetDelimitedValue( readLine, '`', 4, maxModifier );
				stringtool::GetDelimitedValue( readLine, '`', 5, minRequirements );

				//debug only
				//cout << "These five fields were found: " << templateName << "," << armorType << "," << defenseValue << "," << minModifier << "," << maxModifier << endl;
				//end debug output

				// find the template in the template map
				TemplateMap::iterator findTemplate = Templates.find( templateName );
				if ( findTemplate != Templates.end() )
				{
					// modify the template's properties:

					//debug output
					//cout << templateName << "," << weaponType << "," << minDamage << "," << defenseValue << "," << minModifier << "," << maxModifier << endl;
					//end debug output

					// If the min and max modifier are 0, set
					// allow_modifiers = false in [common]
					if ( ( minModifier == 0 ) && ( maxModifier == 0 ) )
					{
						//cout << templateName << ": min and max are both zero\n"; // debug output
						FuelHandle hCommon = findTemplate->second->GetChildBlock( "common" );
						if ( hCommon ) { hCommon->Set( "allow_modifiers", false ); }
					}

					//Rules: if the armorType is "low" or "fun", just adjust the defense values
					if ( 
						( gpstring( armorType ).same_no_case( "c_fun" ) ) || ( gpstring( armorType ).same_no_case( "c_low" ) ) ||
						( gpstring( armorType ).same_no_case( "o_fun" ) ) || ( gpstring( armorType ).same_no_case( "o_low" ) ) 
					   )
					{
						FuelHandle hDefend = findTemplate->second->GetChildBlock( "defend" );
						if ( hDefend )
						{
							hDefend->Set( "defense", defenseValue );
						}
						FuelHandle hGui = findTemplate->second->GetChildBlock( "gui" );
						if (( hGui ) && !gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
						{
							hGui->Set( "equip_requirements", minRequirements, FVP_STRING ); 
						}
						else if (( hGui ) && gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
						{
							hGui->Remove( "equip_requirements" ); 
						}
					}
					
					else 
					{
						//if it's not "low" or "fun", adjust the attack values and modifier values
						//and to complicate things, they're usually in pcontent under their own section
						FuelHandle hPcontent = findTemplate->second->GetChildBlock( "pcontent" );

						// special case: armorType is c_avg
						// in this case: attack is messed with in the same manner as low/fun weapons
						// and "base" under pcontent gets the modifier values messed with
						if ( gpstring( armorType ).same_no_case( "c_avg" ) )
						{
							FuelHandle hDefend = findTemplate->second->GetChildBlock( "defend" );
							if ( hDefend )
							{
								hDefend->Set( "defense", defenseValue );
							}
							armorType = "base"; //this will force the next block to set the modifier values for
												 //C_avg weapons correctly
							FuelHandle hGui = findTemplate->second->GetChildBlock( "gui" );
							if (( hGui ) && !gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
							{
								hGui->Set( "equip_requirements", minRequirements, FVP_STRING ); 
							}
							else if (( hGui ) && gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
							{
								hGui->Remove( "equip_requirements" ); 
							}
						}
						if ( hPcontent )
						{
							FuelHandle hArmorType = hPcontent->GetChildBlock( armorType );
							if ( hArmorType )
							{
								if ( !gpstring( minRequirements ).same_no_case( "none" ) && !gpstring( armorType ).same_no_case( "base" ) )
								{
									hArmorType->Set( "equip_requirements", minRequirements, FVP_STRING ); 
								}
								else if ( gpstring( minRequirements ).same_no_case( "none" ) && !gpstring( armorType ).same_no_case( "base" ) )
								{
									hArmorType->Remove( "equip_requirements" ); 
								}
								hArmorType->Set( "modifier_max", maxModifier );
								hArmorType->Set( "modifier_min", minModifier );

								if ( !gpstring( armorType ).same_no_case( "base" ) )
								{
									hArmorType->Set( "defense", defenseValue );
								}
								
								//actually, things are done differently now
								/*FuelHandle hDefend = hArmorType->GetChildBlock( "defend" );
								if ( hDefend )
								{
									hDefend->Set( "defense", defenseValue );
								}*/
							}
						}
					}
				}//if ( findTemplate != Templates.end() )
			}//if ( stringtool::GetNumDelimitedStrings( readLine, '`' ) == 6 )

			
			//If there are 5 fields, it's body armor (or similar), (base name, armor, min/max modifier, requirements)
			else if ( stringtool::GetNumDelimitedStrings( readLine, '`' ) == 5 )
			{
				gpstring templateName, minRequirements;
				float defenseValue, minModifier, maxModifier;

				stringtool::GetDelimitedValue( readLine, '`', 0, templateName );
				stringtool::GetDelimitedValue( readLine, '`', 1, defenseValue );
				stringtool::GetDelimitedValue( readLine, '`', 2, minModifier );
				stringtool::GetDelimitedValue( readLine, '`', 3, maxModifier );
				stringtool::GetDelimitedValue( readLine, '`', 4, minRequirements );

				//debug only
				//cout << "These four fields were found: " << templateName << "," << defenseValue << "," << minModifier << "," << maxModifier << endl;
				//end debug output

				// find the template in the template map
				TemplateMap::iterator findTemplate = Templates.find( templateName );
				if ( findTemplate != Templates.end() )
				{
					// modify the template's properties:

					// If the min and max modifier are 0, set
					// allow_modifiers = false in [common]
					if ( ( minModifier == 0 ) && ( maxModifier == 0 ) )
					{
						//cout << templateName << ": min and max are both zero\n"; // debug output
						FuelHandle hCommon = findTemplate->second->GetChildBlock( "common" );
						if ( hCommon ) { hCommon->Set( "allow_modifiers", false ); }
					}

					// All body armor that doesn't have c_avg is handled the same:
					// lookup the template, adjust the "defense" in the [defend] block
					// adjust the modifier_min and modifier_max in the [base] block inside the [pcontent] block

					FuelHandle hDefend = findTemplate->second->GetChildBlock( "defend" );
					if ( !hDefend )
					{
						hDefend = findTemplate->second->CreateChildBlock( "defend" );
					}
					if ( hDefend )
					{
						hDefend->Set( "defense", defenseValue );
					}

					FuelHandle hGui = findTemplate->second->GetChildBlock( "gui" );
					if (( hGui ) && !gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
					{
						hGui->Set( "equip_requirements", minRequirements, FVP_STRING ); 
					}
					else if (( hGui ) && gpstring( minRequirements ).same_no_case( "none" ) ) // if it's not none, put requirements in
					{
						hGui->Remove( "equip_requirements" ); 
					}

					FuelHandle hPcontent = findTemplate->second->GetChildBlock( "pcontent" );
					if ( !hPcontent )
					{
						hPcontent = findTemplate->second->CreateChildBlock( "pcontent" );
						hPcontent->CreateChildBlock( "base" );
					}
					if ( hPcontent )
					{
						FuelHandle hBase = hPcontent->GetChildBlock( "base" );
						if ( hBase )
						{
							hBase->Set( "modifier_min", minModifier );
							hBase->Set( "modifier_max", maxModifier );
						}
					}
				}//if ( findTemplate != Templates.end() )
			}//else if ( stringtool::GetNumDelimitedStrings( readLine, '`' ) == 5 )
			//debug output
			/*else {
				cout << stringtool::GetNumDelimitedStrings( readLine, '`' ) << " was the number of params.\n";
			}*/
			//end debug output
			readLine.clear();
		}
		readFile.Close();
	}
	else
	{
		std::cout << "The input file you specified could not be opened." << endl << "Remember, first param is full path and filename for the input file," <<
			endl << "The second param is the path to your CD image directory." << endl;
	}

	// Write fuel
	std::cout << "Writing Fuel..." << endl;
	pTextFuelDB->SaveChanges();

	delete pFuelSys;
	delete pMasterFileMgr;

	int throwaway;
	cout << "Done!\nClose this window to quit.\n";
	cin >> throwaway;

	return( 0 );
}

