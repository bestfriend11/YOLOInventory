# //////////////////////////////////////////////////////////////////////////
# //
# // File     :  make_game.pl
# // Author(s):  Scott Bilas, David Tomandl
# //
# // Copyright © 2000 Gas Powered Games, Inc.  All rights reserved.
# //------------------------------------------------------------------------
# //  $Revision# $              $Date:$
# //------------------------------------------------------------------------
# //////////////////////////////////////////////////////////////////////////

sub ask($);
sub Extract();

if ($ARGV[0]) {
	$config_file = $ARGV[0];
	open ( CONFIG, "<" . $config_file ) or die "Couldn't open config file $config_file!  Exiting...\n";
	while ($line = <CONFIG>) {
		if ($line =~ /^(.*) = (.*)$/) {
			$var = $1;
			$answer = $2;
			$$var{'answer'} = $answer;
		}
	}
}

$buildlog = 'P:\vss_db_tattoo\GPG\buildlog.txt';
$libpath = 'P:\vss_db_tattoo\GPG\Lib Projects';
$gpgpath = 'P:\vss_db_tattoo\GPG\Projects';
$gpgout = 'P:\vss_db_tattoo\GPG\Out';
$gpgroot = 'P:\vss_db_tattoo\GPG';
$gpgtools = 'p:\vss_db_tattoo\TATTOO_ASSETS\Tools';
$gpgtemp = 'p:\temp';

system("del /q /f $buildlog");
system("md $gpgout > NUL");
system("md $gpgout\\loc > NUL");

# //////////////////////////////////////////////////////////////////////////

system ('net stop "Network Associates Alert Manager"');
system ('net stop "Network Associates Task Manager"');
system ('net stop "Network Associates McShield"');

# //////////////////////////////////////////////////////////////////////////

print "======================================================================\n\n";
$dellocal{'question'} =		'Delete local source tree?         (y/n): ';
$deltemp{'question'} =		'Delete temp directory?            (y/n): ';
$fetchvss{'question'} =		'Fetch VSS into local source tree? (y/n): ';
$buildgame{'question'} =	'Build retail game?                (y/n): ';
$buildgamer{'question'} =	'Build release game?               (y/n): ';
$buildgamed{'question'} =	'Build debug game?                 (y/n): ';
$buildeditor{'question'} =	'Build editor?                     (y/n): ';
$createlang{'question'} =	'Create LANGUAGE.DLL?              (y/n): ';

if (not $dellocal{'answer'}) { ask( 'dellocal' ) };
if (not $deltemp{'answer'}) { ask( 'deltemp' ) };
if (not $fetchvss{'answer'}) { ask( 'fetchvss' ) };
if (not $buildgame{'answer'}) { ask( 'buildgame' ) };
if (not $buildgamer{'answer'}) { ask( 'buildgamer' ) };
if (not $buildgamed{'answer'}) { ask( 'buildgamed' ) };
if (not $buildeditor{'answer'}) { ask( 'buildeditor' ) };
if (not $createlang{'answer'}) { ask( 'createlang' ) };

if ($this_script_has_asked_the_user_a_question) {
	print "If you would like to save this configuration to a file, type the path and filename in now. (Leave blank and hit <enter> to skip.)\n: ";
	$new_config_file = <STDIN>;
	chomp $new_config_file;
	if ($new_config_file) {
		if (open ( NEWCONFIG, ">" . $new_config_file ) ) {
			print NEWCONFIG "dellocal = $dellocal{'answer'}\n";
			print NEWCONFIG "deltemp = $deltemp{'answer'}\n";
			print NEWCONFIG "fetchvss = $fetchvss{'answer'}\n";
			print NEWCONFIG "buildgame = $buildgame{'answer'}\n";
			print NEWCONFIG "buildgamer = $buildgamer{'answer'}\n";
			print NEWCONFIG "buildgamed = $buildgamed{'answer'}\n";
			print NEWCONFIG "buildeditor = $buildeditor{'answer'}\n";
			print NEWCONFIG "createlang = $createlang{'answer'}\n";
			close NEWCONFIG or print "Tell Dave--I couldn't close $new_config_file! I'll continue running anyway...\n";
		}
		else {print "Couldn't create $new_config_file!\nIf you were trying to save a config file, make sure that it is a valid path and filename.\n";}
	}
}

print "\n======================================================================\n";

# //////////////////////////////////////////////////////////////////////////

if ($dellocal{'answer'} =~ /y/i) {
	print "\nDeleting local source tree...\n";
	system ("del /q /s /f \"$libpath\"");
	system ("md \"$libpath\" > NUL");
	system ("del /q /s /f \"$gpgpath\"");
	system ("md $gpgpath > NUL");
	print "\n";
#	print "dellocal block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($deltemp{'answer'} =~ /y/i) {
	print "\nDeleting temp directory...\n";
	system ("del /q /s /f $gpgtemp");
	print "\n";
#	print "deltemp block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($fetchvss{'answer'} =~ /y/i) {
	print "\nFetching VSS into local source tree...\n\n";
	chdir ($gpgroot);
	system ("$vss get \"\$/gpg/\*\" -R -I-");
	print "\n";
	chdir ($gpgtools);
	system ("$vss_data get \"\$/TATTOO_ASSETS/Tools/\*\" -R -I-");
	print "\n";
	chdir ($gpgroot); #just for fun--whee!
#	print "fetchvss block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($buildgame{'answer'} =~ /y/i) {
	print "\nBuilding Dungeon Siege [Retail]...\n\n";
	system ("del /q /f $gpgout\\DungeonSiege.exe;DungeonSiege.pdb");
	print "All normal output is being piped to $buildlog, look at that file for progress info.\n";
	system ("msdev $gpgpath\\tattoo\\exe\\exe.dsw /Y3 /MAKE \"_Exe - Win32 Retail\" > $buildlog");
	print "\n";
	chdir "$gpgpath\\tattoo\\exe\\retail";
	$exename = "DungeonSiege";
	Extract();
#	print "buildgame block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($buildgamer{'answer'} =~ /y/i) {
	print "\nBuilding Dungeon Siege [Release]...\n\n";
	system ("del /q /f $gpgout\\DungeonSiegeR.exe;DungeonSiegeR.pdb");
	print "All normal output is being piped to $buildlog, look at that file for progress info.\n";
	system ("msdev $gpgpath\\tattoo\\exe\\exe.dsw /Y3 /MAKE \"_Exe - Win32 Release\" >> $buildlog");
	print "\n";
	chdir ("$gpgpath\\tattoo\\exe\\release");
	$exename = "DungeonSiegeR";
	Extract();
#	print "buildgamer block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($buildgamed{'answer'} =~ /y/i) {
	print "\nBuilding Dungeon Siege [Debug]...\n";
	system ("del /q /f $gpgout\\DungeonSiegeD.exe;DungeonSiegeD.pdb");
	print "All normal output is being piped to $buildlog, look at that file for progress info.\n";
	system ("msdev $gpgpath\\tattoo\\exe\\exe.dsw /Y3 /MAKE \"_Exe - Win32 Debug\" >> $buildlog");
	print "\n";
	chdir "$gpgpath\\tattoo\\exe\\debug";
	$exename = "DungeonSiegeD";
	Extract();
#	print "buildgamed block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($buildeditor{'answer'} =~ /y/i) {
	print "\nBuilding Siege Editor...\n";
	system ("del /q /f $gpgout\\SiegeEditor.exe;SiegeEditor.pdb");
	system ("del /q /f $gpgout\\SiegeEditorD.exe;SiegeEditorD.pdb");
	print "All normal output is being piped to $buildlog, look at that file for progress info.\n";
	system ("msdev $gpgpath\\siegeeditor\\siegeeditor.dsw /MAKE \"SiegeEditor - Win32 Release\" > $buildlog");
	system ("msdev $gpgpath\\siegeeditor\\siegeeditor.dsw /MAKE \"SiegeEditor - Win32 Debug\" > $buildlog");
	print "\n";
	chdir "$gpgpath\\siegeeditor\\debug\\";
	$exename = "SiegeEditorD";
	Extract();
	chdir "$gpgpath\\siegeeditor\\release\\";
	$exename = "SiegeEditor";
	Extract();
#	net send bertb \"Siege Editor built!\"
#	print "buildeditor block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if ($createlang{'answer'} =~ /y/i) {
	print "\nCreating LANGUAGE.DLL...\n";
	system ("del /q /f $gpgout\\loc\\language.dll");
	system ("$gpgtools\\codeloc $gpgroot $gpgout\\loc\\language.dll");
#	print "createlang block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

if (($buildgame{'answer'} =~ /y/i) or ($buildgamer{'answer'} =~ /y/i) or ($buildgamed{'answer'} =~ /y/i) or ($buildeditor{'answer'} =~ /y/i)) {
	print "\n======================================================================\n\n";
	print "--== Summary ==--\n\n";
	system ("type $buildlog | find /i \"warning(s)\"");
#	print "print summary block done\nPress <enter> to continue...\n"; $pause = <STDIN>; #debug output
}

# //////////////////////////////////////////////////////////////////////////

print "\nDone!\n";
print "Totally finished!\nPress <enter> to continue...\n"; $pause = <STDIN>; #pause at the end, then done

# //////////////////////////////////////////////////////////////////////////
# Subroutines

sub ask($) {
	$this_script_has_asked_the_user_a_question = 1;
	$var = shift @_;
	print $$var{'question'};
	$answer = <STDIN>;
	if ($answer !~ /^(y|n)$/i) {
		print "I'm looking for just a \"y\" or \"n\" character.  Please try again.\n";
		ask ($var);
	}
	else {
		$$var{'answer'} = $answer;
	}
}

sub Extract () {
	print "Extracting debug info...\n";
	system ("copy /y $exename.exe $exename.exe.bak");
	print "\n";
	system ("rebase -q -b 0x00400000 -x . $exename.exe");
	print "\n";
	system ("move /y $exename.pdb $gpgout\\$exename.pdb");
	print "\n";
	system ("move /y $exename.exe $gpgout\\$exename.exe");
	print "\n";
	system ("rename $exename.exe.bak $exename.exe");
	print "\n";
	system ("$gpgtools\\fubipacker -file $gpgout\\$exename.exe");
	print "\n";
}
