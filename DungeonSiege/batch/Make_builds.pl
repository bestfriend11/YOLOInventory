# Make_builds.pl
#
# meant to be used on Bertb by someone with the burns account password
# This script will begin the build process on buildbox2
#
# Parameters passed in will be sent to the script on the other machine.  Useful for
# executing make_game_buildbox2.pl without it prompting for input.
#
$command_string = 'rexec buildbox2 -l "burns" P:\vss_db_tattoo\GPG\Batch\make_game_buildbox2.pl';
if ($ARGV[0]) {$command_string .= " $ARGV[0]";}
system ($command_string);
