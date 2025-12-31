# From bertb,

# Todo: make the remote connection stuff work.  It should:
# 	connect to Buildbox2 and do something like this:
#	system ('rexec buildbox2 -l "burns" P:\vss_db_tattoo\GPG\Batch\make_game_buildbox2.pl');
# 	connect to Tankbox and do something like this:
#	system ('rexec tankbox -l "burns" P:\vss_db_tattoo\GPG\Batch\make_tanks.btm');

# 	using sleep(30), then check against a file which can be copied to a commonly accessible location
# After that runs, the code below should be run.

# So, when Tank and Builds complete:

$vss = 'C:\Progra~1\Micros~3\VSS\win32\ss.exe'; #vss executable for future reference
$vss_code = 'C:\Progra~1\Micros~3\VSS2\win32\ss.exe'; #vss executable for future reference
$gpgout = 'P:\vss_db_tattoo\GPG\Out';
$assets_root = 'P:\vss_db_tattoo\TATTOO_ASSETS';
$toolspath = $assets_root . '\tools';
$cdimagepath = $assets_root . '\cdimage';

#print "When Tanks and builds are complete, press <enter>.\n";
#$input = <STDIN>;

print 'This should copy all .exe and .pdb files as well as the \\loc directory from GPG\Out on buildbox2 to p:\vss_db_tattoo\TATTOO_ASSETS\cdimage' . "\n";#
#Press <enter>\n";
#$input = <STDIN>;

system ('attrib -r P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\* /s /d > NUL');
system ('mkdir P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\\loc');
system ('copy \\\\buildbox2\p$\vss_db_tattoo\GPG\Out\*.exe P:\vss_db_tattoo\TATTOO_ASSETS\cdimage');
system ('copy \\\\buildbox2\p$\vss_db_tattoo\GPG\Out\*.pdb P:\vss_db_tattoo\TATTOO_ASSETS\cdimage');
system ('xcopy \\\\buildbox2\p$\vss_db_tattoo\GPG\Out\\loc P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\\loc /e /q /h /k /y');

print 'This should copy all .exe and .pdb files as well as the \\loc directory from GPG\Out on buildbox2 to p:\vss_db_tattoo\GPG\Out' . "\n";
#Press <enter>\n";
#$input = <STDIN>;

system ('attrib -r P:\vss_db_tattoo\GPG\Out\* /s /d > NUL');
system ('mkdir P:\\vss_db_tattoo\\GPG\\Out\\loc');
system ('copy \\\\buildbox2\\p$\\vss_db_tattoo\GPG\Out\*.exe P:\vss_db_tattoo\GPG\Out');
system ('copy \\\\buildbox2\\p$\\vss_db_tattoo\GPG\Out\*.pdb P:\vss_db_tattoo\GPG\Out');
system ('xcopy \\\\buildbox2\\p$\\vss_db_tattoo\GPG\Out\\loc P:\vss_db_tattoo\GPG\Out\\loc /e /q /h /k /y');

print 'This should move all .exe and .pdb files as well as the \\loc directory from GPG\Out on buildbox2 to \\\\Bertb\TANKBOX_test' . "\n";
#print "Press <enter>\n";
#$input = <STDIN>;

system ('attrib -r \\\\bertb\tankbox_test\* /s /d > NUL');
#system ('mkdir \\\\Bertb\TANKBOX_test\\loc');
system ('move \\\\buildbox2\p$\vss_db_tattoo\GPG\Out\*.exe \\\\Bertb\TANKBOX_test');
system ('move \\\\buildbox2\p$\vss_db_tattoo\GPG\Out\*.pdb \\\\Bertb\TANKBOX_test');
system ('move \\\\buildbox2\\p$\\vss_db_tattoo\\GPG\\Out\\loc \\\\Bertb\\TANKBOX_test\\loc');# /e /q /h /k /y');

#Do a VSS get of latest stuff from '$\TATTOO_ASSETS\Installed' folder to
#
#P:\vss_db_tattoo\TATTOO_ASSETS\cdimage
#
#and
#
#P:\vss_db_tattoo\GPG\Out
#
#and
#
#\\Bertb\TANKBOX_test

print 'This will do a VSS get of latest stuff from \'$\TATTOO_ASSETS\Installed\' folder to p:\vss_temp.  From there is will be copied to' . "\n" . 'P:\vss_db_tattoo\TATTOO_ASSETS\cdimage and' . "\n" .
		'P:\vss_db_tattoo\GPG\Out' . " and\n" .
		'\\\\Bertb\TANKBOX_test' . "\n";

#print "After each step (for now), you will be prompted to press <enter>.\n";
#print "Press <enter> to begin the process.\n";
#$input = <STDIN>;

system ('rd /s /q p:\vss_temp > NUL');#dangerous command--but nobody should be using a directory called p:\vss_temp
system ('mkdir p:\vss_temp');
chdir ('p:\vss_temp') or die "I couldn't chdir to p:\\vss_temp! Quitting...\n";
system ("$vss Get " . '$\TATTOO_ASSETS\Installed -R -I-');
print "Get into " . 'P:\vss_temp' . " finished.\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;

system ('attrib -r P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\* /s /d > NUL');
system ('xcopy P:\vss_temp\* P:\vss_db_tattoo\TATTOO_ASSETS\cdimage /e /q /h /k /y');
print "Copy into " . 'P:\vss_db_tattoo\TATTOO_ASSETS\cdimage' . " finished.\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;

system ('attrib -r P:\vss_db_tattoo\GPG\Out\* /s /d > NUL');
system ('xcopy P:\vss_temp\* P:\vss_db_tattoo\GPG\Out /e /q /h /k /y');
print "Copy into " . 'P:\vss_db_tattoo\GPG\Out' . " finished.\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;

system ('attrib -r \\\\bertb\tankbox_test\* /s /d > NUL');
system ('xcopy P:\vss_temp\* \\\\bertb\tankbox_test /e /q /h /k /y');
print "Copy into " . '\\\\bertb\tankbox_test' . " finished.\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;


#________________________________________________
#
#activate make_loc batch file 
#or rather, do the stuff that's in the make_loc batch file
print "This will do the things that the \"Make_loc.btm\" batch file did.\n";
#print "For now, you will need to run the \"Make_loc.btm\" batch file yourself.  When you have done this,\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;

# This is the make_loc stuff

system ("md $gpgout > NUL");
system ("md $gpgout\\loc > NUL");

# //////////////////////////////////////////////////////////////////////////

print "\nFetching latest tools...\n";
chdir ($toolspath);
system ($vss . ' get "$/tattoo_assets/tools/codeloc.exe" -I-');
system ($vss . ' get "$/tattoo_assets/tools/textloc.exe" -I-');
system ($vss . ' get "$/tattoo_assets/tools/uiloc.exe" -I-');

# //////////////////////////////////////////////////////////////////////////

chdir ($gpgout . "\\loc");

print "\nCreating TEXT.DLL (this takes a while)...\n\n";
system ('del /q /f text.dll > NUL');
system ("$toolspath\\textloc $cdimagepath");

print "\nCreating UI.DLL and LOCALIZATION_UI_MAP.GAS...\n\n";
system ('del /q /f ui.dll > NUL');
system ("$toolspath\\uiloc $cdimagepath");

system ('del /q /f temp.rc > NUL');
system ('del /q /f temp.res > NUL');

# //////////////////////////////////////////////////////////////////////////

print "\nDone with make_loc stuff.\n";

# end make_loc stuff

#print "Press <enter> to continue.\n";
#print "Check to make sure that text.dll, ui.dll, and localization_ui_map.gas got generated.  Press <enter> if it's ok.\nFix it now if it's not!\n";
#$input = <STDIN>;

#________________________________________________
#
#activate get_loc_stuff batch file
#or rather, do the stuff that it used to do
print "This will now get the loc stuff.\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;

system ("md $gpgout > NUL");
system ("md $gpgout\\loc > NUL");

print "\nFetching Files...\n\n";
chdir ("$gpgout\\loc");
system ($vss . ' get "$/tattoo_assets/tools/textloc.exe" -I-');
system ($vss . ' get "$/tattoo_assets/tools/uiloc.exe" -I-');
system ($vss . ' get "$/docs/technical/international support.doc" -I-');
system ($vss . ' get "$/docs/technical/loc_tools_readme.htm" -I-');
system ($vss . ' get "$/tattoo_assets/tools/tankbuilder.exe" -I-');
system ($vss . ' get "$/tattoo_assets/tools/gplzo.dll" -I-');
system ($vss_code . ' get "$/gpg/batch/make_tank.gas" -I-');
system ($vss_code . ' get "$/gpg/batch/make_language_tank.bat" -I-');
system ('copy p:\\loc_tank_readme\\*.htm .');
system ('del vssver.scc > NUL');

print "\nDone Fetching Files!\n";

#print "Press <enter> to continue.\n";
#$input = <STDIN>;

#________________________________________________
# 
print 'This will copy the loc directory from GPG\Out to TATTOO_ASSETS\cdimage and \\Bertb\TANKBOX_test' . "\n";
#print "Press <enter> to continue.\n";
#$input = <STDIN>;
system ('attrib -r P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\* /s /d > NUL');
system ('xcopy P:\vss_db_tattoo\GPG\Out\\loc P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\\loc /e /q /h /k /y');
system ('attrib -r \\Bertb\TANKBOX_test\* /s /d > NUL');
system ('xcopy P:\vss_db_tattoo\GPG\Out\\loc \\\\Bertb\TANKBOX_test\\loc /e /q /h /k /y');
#print "Press <enter> to continue.\n";
#$input = <STDIN>;
##________________________________________________
##
# Delete all *.scc files in the P:\vss_db_tattoo\TATTOO_ASSETS\cdimage folder
system ('del P:\vss_db_tattoo\TATTOO_ASSETS\cdimage\*.scc /s /q /f > NUL');
# Place all *.txt and *.log files into a new folder in P:\vss_db_tattoo\GPG\Out called 'tank.txt'
system ('mkdir P:\vss_db_tattoo\GPG\Out\tank.txt > NUL');
system ('move p:\vss_db_tattoo\GPG\Out\*.txt P:\vss_db_tattoo\GPG\Out\tank.txt');
system ('move p:\vss_db_tattoo\GPG\Out\*.log P:\vss_db_tattoo\GPG\Out\tank.txt');
## Let Bert know that everything is ready for the smoketest
system ('net send bertb "Ready for smoketest!"');


print "Pre-smoketest has finished!  When you are\ndone reviewing the above material,\npress <enter> to continue.\n";
$input = <STDIN>;
