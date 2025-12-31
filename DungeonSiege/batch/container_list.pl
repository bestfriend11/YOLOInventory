# This will list all of the template names in container.gas in each region under map_world.
# The output report is meant to be imported into excel.  --DT

##################################################################
#  CHANGE THESE NEXT TWO TO WHATEVER PATH YOU WANT

# This path needs to point to the map that you want to probe
$path = 'C:\vssdir\TATTOO_ASSETS\CDimage\world\maps\map_world';

# This is where your report will go.
$outputfile = 'c:\dave\test\output.txt';

#
##################################################################


$path .= '\regions';
$suffix = '\objects\container.gas';

opendir MYDIR, $path or die "Could not open $dir!\n";

@allregions = grep !/^\.\.?$/, readdir MYDIR;

closedir MYDIR or die "Could not close $dir!\n";

open (OUTPUT, ">" . $outputfile) or die "Couldn't open $outputfile for writing!\n";

print OUTPUT "Region\tContainer\tCount\n";

#print "All files follow:\n";

foreach $region (@allregions) {

	undef %alltemplates;
	$filename = $path . "\\" . $region . $suffix;
#	print "$filename\n";
	if (open (CONTAINER, "<" . $filename)) {

		while ($line = <CONTAINER>) {
			
#I'm trying to match something like this, for example
#[t:barrel_glb_powder_keg,n:0x001000dd]
			if ($line =~ /\[t\:(.*?),n\:/) {
			
				$template = $1;
				$alltemplates{$template}++;
								
			}
			
		}
		
#		print "Region: $region\n";
		foreach $key (sort keys %alltemplates) {
			print OUTPUT "$region\t$key\t$alltemplates{$key}\n";
		}
		
#	print "$filename\n";
	
	close CONTAINER or die "Couldn't close $filename!\n";
		
	}

}

close OUTPUT or die "Could not close $outputfile!\n";
#print "All files done.\n";