# This is a script to help out the art team.
# It will read in a bunch of template names from one spreadsheet
# and match it against another spreadsheet to get the proper gui size for each item
# --DT

##########################################################
# CHANGE THESE PATH/FILENAMES TO WHAT YOU'RE GOING TO USE

#$template_spreadsheet = 'c:\dave\weapons_master.txt';

$template_spreadsheet = 'c:\dave\w.txt';

$size_spreadsheet = 'c:\dave\art_weapons_sizes_11-08-00.txt';

#
##########################################################


open (DATAFILE, "<" . $template_spreadsheet) or die "couldn't open $template_spreadsheet!\n";

$total_template_names = 0;
$total_lines = 0;
$i = 0;
while ($line = <DATAFILE>) {
	chomp $line;
	@current_line = split /\t/, $line;
	$names[$i++] = $current_line[2];
	$total_template_names++ if ($current_line[2]);
#	push @names, [@current_line];
	$total_lines++;
}
close (DATAFILE);

print "Lines in template spreadsheet:  $total_lines.\n";
#print "Names are:\n";
#foreach $name (@names) {
#	print "$name\n";
#}
print "Number of template names: $total_template_names\n";

open (DATAFILE, "<" . $size_spreadsheet) or die "couldn't open $size_spreadsheet!\n";

$size_lines = 0;
while ($line = <DATAFILE>) {
	chomp $line;
	@current_line = split /\t/, $line;
	push @size, [@current_line];
	$size_lines++;
}
close (DATAFILE);

print "Lines in size spreadsheet:  $size_lines.\n";
#print "line follows:\n$size[1][5]\n";


$i = 0;

for ($line = 0; $line < $size_lines; $line++) {

	if ((not $size[$line][0]) or
		($size[$line][0] =~ /ignore/i) or
		($size[$line][1] =~ /ignore/i))	{
	}
	else {
#		print "line is $line, $size[$line][0] $size[$line][1]\n";
		@matches = ();
		$entry = 0;
		for ($column = 2; $column < 26; $column++){
			if ($size[$line][$column]) {
#				print "inside if ($size[$line][$column]) loop,$size[0][$column],$size[1][$column]\n";
				if ($size[$line][$column] =~ /all/i) {$size[$line][$column] = "low/fun/avg/fin/mag/str";}
				while ($size[$line][$column] =~ /(.*?)\/(.*)/) {
#if (($line == 1) and ($column == 5)) {print "line follows:\n$size[1][5]\n";print "1,2 are:$1,$2\n";print "size:$size[0][$column]\nentry:$entry\n";}
#if (($line == 1) and ($column == 5)) {print "\!\!@matches\n";}

					$matches[$entry++] = $1 . ":" . $size[0][$column];
					$size[$line][$column] = $2;
				}
				$matches[$entry++] = $size[$line][$column] . ":" . $size[0][$column];
#if (($line == 1) and ($column == 5)) {print "$size[1][5]\n";}
#if (($line == 1) and ($column == 5)) {print "\!\!@matches\n";}
#				print "matches is now: @matches\n";

#				if (not $size[$line][$column] =~ /(.*?)\/(.*)/) {
#					$match[$entry++] = $size[$line][$column];
#				}
#				else {
#					$match[$entry++] = $size[$line][$column];
#				}
			
			}
		}
		foreach $match (@matches) {
#			print "$match\n" if ($line == 1);
#		for ($index = 0; $index <= $#matches; $index++) {
#			$match = $matches[$index];
			$match =~ /(.*):(.*)/;
			$field1 = $1;
			$size_field = $2;
			$field2 = $size[$line][0];
			if ($size[$line][1] =~ /(.*) (.*)/) {
				$field3 = $1;
				$field4 = $2;
			}
			else {
				$field3 = $size[$line][1];
				$field4 = "";
			}

#			if #(($size_field =~ /large.*2/i) and 
#			($field1 =~ /low/i)
			#)
#			{
#			 	print "size,fields:$size_field,$field1,$field2,$field3,$field4\n";
#			}
			

#			$template_matches = 0;
			foreach $template (@names) {
				if (
					($template =~ /$field1/i) and
					($template =~ /$field2/i) and
					($template =~ /$field3/i) and
					($template =~ /$field4/i)
					) {
#					$template_matches++;
#					if ($template_matches > 1) {print "More than one template matched!  $field1 $field2 $field3 $field4 $template\n"; exit(1);}
					$name_lookup{$template} = $size_field;
#					print "template,size_field is $template,$size_field\n";
				}
			}
		}
	}
}

#print "$#names entries in \$names.\n";	


$outputfile = 'c:\dave\a.txt';

open (OUTPUT, ">" . $outputfile) or die "couldn't open $outputfile for writing!\n";
foreach $template (@names) {
	print OUTPUT "$template\t$name_lookup{$template}\n";
}
close (OUTPUT);

#print "line follows:\n$size[1][5]\n";
