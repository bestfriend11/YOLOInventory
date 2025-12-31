# This is a build script meant to be run by Bert, after all of the files are
# generated and sitting on buildbox2 and tankbox.  This is meant to be run from tankbox.
# 7/19/02

sub two_digit_format($); # prototype

print "\[M\]ain build, or \[U\]pdate1 build?  Press the letter and <enter>\n";
$build_type = <STDIN>;
die "Neither an 'M' or 'U' detected, quitting.\n" if ($build_type !~ /[mMuU]/);
if ($build_type =~ /[mM]/) { $build_dir = "main"; }
if ($build_type =~ /[uU]/) { $build_dir = "update1"; }

print "Copying from \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out to P:\\work\\ds1\\$build_dir\\out\n\n";

system ("md P:\\work\\ds1\\$build_dir\\out 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\loc 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\logs 2>NUL");

print "Deleting all *.lqd* files under P:\\work\\ds1\\$build_dir\\cdimage . . .\n";
system ("del /f /s /q P:\\work\\ds1\\$build_dir\\cdimage\\*.lqd* 1>NUL");
print "Making all files under P:\\work\\ds1\\$build_dir\\cdimage writeable . . .\n";
system ("attrib -r -h P:\\work\\ds1\\$build_dir\\cdimage\\* /s");
print "Deleting all *.lqd* files under P:\\work\\ds1\\$build_dir\\out . . .\n";
system ("del /f /s /q P:\\work\\ds1\\$build_dir\\out\\*.lqd* 1>NUL");
print "Making all files under P:\\work\\ds1\\$build_dir\\out writeable . . .\n";
system ("attrib -r -h P:\\work\\ds1\\$build_dir\\out\\* /s");

system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\loc\\* P:\\work\\ds1\\$build_dir\\out\\loc");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\logs\\* P:\\work\\ds1\\$build_dir\\out\\logs");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsiege*.* P:\\work\\ds1\\$build_dir\\out");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;

print "Copying from \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out to P:\\work\\ds1\\$build_dir\\cdimage\n\n";

system ("md P:\\work\\ds1\\$build_dir\\cdimage 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\cdimage\\loc 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\cdimage\\logs 2>NUL");

system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\loc\\* P:\\work\\ds1\\$build_dir\\cdimage\\loc");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\logs\\* P:\\work\\ds1\\$build_dir\\cdimage\\logs");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsiege*.* P:\\work\\ds1\\$build_dir\\cdimage");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;

print "Copying from P:\\work\\ds1\\$build_dir\\out\\logs to P:\\work\\ds1\\$build_dir\\cdimage\n\n";

system ("md P:\\work\\ds1\\$build_dir\\cdimage\\logs 2>NUL");
system ("copy /y P:\\work\\ds1\\$build_dir\\out\\logs P:\\work\\ds1\\$build_dir\\cdimage\\logs");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;

print "Copying from P:\\work\\ds1\\$build_dir\\cdimage to P:\\work\\ds1\\$build_dir\\out\n\n";

system ("md \"P:\\work\\ds1\\$build_dir\\out\\save games\" 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\system 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1028 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1031 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1033 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1036 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1040 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1041 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\1042 2>NUL");
system ("md P:\\work\\ds1\\$build_dir\\out\\3082 2>NUL");

system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\*.* P:\\work\\ds1\\$build_dir\\out");
system ("copy /y \"P:\\work\\ds1\\$build_dir\\cdimage\\save games\\*\" \"P:\\work\\ds1\\$build_dir\\out\\save games\"");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\system\\* P:\\work\\ds1\\$build_dir\\out\\system");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1028\\* P:\\work\\ds1\\$build_dir\\out\\1028");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1031\\* P:\\work\\ds1\\$build_dir\\out\\1031");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1033\\* P:\\work\\ds1\\$build_dir\\out\\1033");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1036\\* P:\\work\\ds1\\$build_dir\\out\\1036");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1040\\* P:\\work\\ds1\\$build_dir\\out\\1040");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1041\\* P:\\work\\ds1\\$build_dir\\out\\1041");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\1042\\* P:\\work\\ds1\\$build_dir\\out\\1042");
system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\3082\\* P:\\work\\ds1\\$build_dir\\out\\3082");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;

print "Moving from P:\\work\\ds1\\$build_dir\\out\\Resources to P:\\work\\ds1\\$build_dir\\out\\mods\n\n";

system ("md P:\\work\\ds1\\$build_dir\\out\\mods 2>NUL");

system ("move P:\\work\\ds1\\$build_dir\\out\\Resources\\benchmark.dsres P:\\work\\ds1\\$build_dir\\out\\mods");
system ("move P:\\work\\ds1\\$build_dir\\out\\Resources\\gremal_hunter.dsres P:\\work\\ds1\\$build_dir\\out\\mods");
system ("move P:\\work\\ds1\\$build_dir\\out\\Resources\\looping_demo.dsres P:\\work\\ds1\\$build_dir\\out\\mods");
system ("move P:\\work\\ds1\\$build_dir\\out\\Resources\\siegeeditorextras.dsres P:\\work\\ds1\\$build_dir\\out\\mods");
system ("move P:\\work\\ds1\\$build_dir\\out\\Resources\\yesterhaven.dsres P:\\work\\ds1\\$build_dir\\out\\mods");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;


# Date/Time format directories look like this:
# 07-19-02_1000_retail
@timearray = localtime;

$month = two_digit_format($timearray[4] + 1);
$day = two_digit_format($timearray[3]);
$year = two_digit_format($timearray[5] - 100);
$hour = two_digit_format($timearray[2]);
$minute = two_digit_format($timearray[1]);

print "Creating directories in TheKeep\\Builds\n\n";

system ("md \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_retail 2>NUL");
system ("md \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_release 2>NUL");
system ("md \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_debug 2>NUL");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;

print "Copying executables (and .pdb files) to TheKeep\\Builds\n";

system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsiege.exe \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_retail");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsiege.pdb \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_retail");

system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsieger.exe \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_release");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsieger.pdb \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_release");

system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsieged.exe \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_debug");
system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\out\\dungeonsieged.pdb \\\\thekeep\\Builds\\${month}-${day}-${year}_${hour}${minute}_debug");

#print "Paused, press <enter> to continue.\n"; $throwaway = <STDIN>;

# From the MSQA Version number, the directory needs to look like this for a main build tank of 02.07.1901: 
# ds1_main_02.07.1901_tank

print "Right now, I don't have a way to get the MSQA number, so you will have to do these steps yourself:\n";
print "\n";
print "copy P:\\work\\ds1\\$build_dir\\out\\* P:\\testing_data\\current\\ds1_main_(msqa number)_tank\n";
print "\n";
print "copy P:\\work\\ds1\\$build_dir\\cdimage\\* p:\\testing_data\\current\\ds1_main_(msqa number)_bits\n";
print "\n";
print "copy \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\gpg p:\\testing_data\\current\\ds1_main_(msqa number)_code\n";
print "\nWhen you are done with this, press <enter>.\n(If you wish to close this script, press Ctrl-C or close this window)\n";
$throwaway = <STDIN>;

# system ("copy /y P:\\work\\ds1\\$build_dir\\out\\* P:\\testing_data\\current\\ds1_main_(msqa number)_tank");

# system ("copy /y P:\\work\\ds1\\$build_dir\\cdimage\\* p:\\testing_data\\current\\ds1_main_(msqa number)_bits");

# system ("copy /y \\\\buildbox2\\p\$\\work\\ds1\\$build_dir\\gpg p:\\testing_data\\current\\ds1_main_(msqa number)_code");

# MOVE old versions of debug, retail, release in:

print "Currently I don't have a way to figure out\nwhich directories in TheKeep\\Builds are 'old',\nso you will need to do this step yourself:\n";

print ("move \\\\thekeep\\Builds\\(old build directories in date/time_blah format) \\\\thekeep\\Builds\\previous_exes");

# system ("move \\\\thekeep\\Builds\\(old build directories in date/time_blah format) \\\\thekeep\\Builds\\previous_exes");

print "Done!\n";

print "Press <enter> to terminate this program.\n";
$throwaway = <STDIN>;

sub two_digit_format($) {
	my $number = shift @_;
	if ($number =~ /^\d{1}$/) { $number = "0" . $number; }
	return $number;
}
