# This utility was created by David Tomandl.  Direct all questions to him.
# Know this: the input file will be loaded completely into RAM.  Try not to use this parser
#   on insanely huge files--your OS will hate you.

# The purpose of this tool is to convert gas to .xml

# $i is the bracecount variable

if ($#ARGV != 1) {die "Usage:  gas2xml.pl <input file> <output file>\n";}

$input_file = $ARGV[0];
$output_file = $ARGV[1];

#$input_file = 'c:\dave\perl\world.gas';
#$output_file = 'c:\dave\test\world.xml';

open (INPUT, "<" . $input_file) or die "\nCouldn't open input file!\n";
open (OUTPUT, ">" . $output_file) or die "\nCouldn't open output file!\n";

# Apparently, it's possible to have Ctrl-z codes in the files now.  This should take care of it
print "Converting input file into temp file that doesn't have ctrl-z codes...\r";
binmode INPUT;
$temp_file = $input_file . ".tmp";
open (TEMPFILE, ">$temp_file") or die "\nCouldn't open temp file $temp_file: $!\n";
binmode TEMPFILE;
my $buffer;
while ( read( INPUT, $buffer, 16384 ) ) {
	$buffer =~ s/\x1A/0x1A/g; # Scott asked for ctrl-z (hex 1A) to become the string '0x1A'
	print TEMPFILE $buffer;
}
close TEMPFILE or die "\nCouldn't close temp file $temp_file: $!\n";
close INPUT or die "\nCouldn't close temp file $temp_file: $!\n";
open (INPUT, "<$temp_file") or die "\nCouldn't open temp file $temp_file: $!\n";

print "Converting input file into temp file that doesn't have ctrl-z codes...Done.\n";

$line_number = 0;
$i = 0;
#$closed = 1;
$lookahead_mode = 0;
$total_lines = 0;

# These two are for fun...and sometimes it takes a while to crank through large gas files.
$percent_done = 0;
$previous_percent_done = 0;

# OK, this just feels *wrong*...but it doesn't look like I have much of a choice.
# I can either put the whole input file in RAM, or close and re-open the file
# every time I'm done looking ahead.  Blech.
# So I'm putting the whole input file in RAM.

print "Loading input file...\r";

# The bad (memory HUNGRY way) to read in a big file:
#@the_whole_file = <INPUT>;

# The good (still memory hungry, but not so bad) way to read in a big file:
undef $/; # get whole file in one variable
$the_whole_file = <INPUT>;
@the_whole_file = split /\n/, $the_whole_file;
$/ = "\n"; #cleanup
undef $the_whole_file; #cleanup

close INPUT or die "\nCouldn't close input file!\n";

print OUTPUT "<gas>\n";

print "Input file loaded into memory.\n";
print "Parsing... \r";
while ($line = $the_whole_file[$line_number++]) {

	$total_lines++;

	$line =~ s/\"//g;
	$line =~ s/</.lt./g;
	$line =~ s/>/.gt./g;
	$line =~ s/&/+/g;

	if ($lookahead_mode) {

		if ($line =~ /\[(.*)\]/) { }
		elsif ($line =~ /\{/) { $lookahead_brace_count++; }
		elsif ($line =~ /\}/) {
			if ($lookahead_brace_count > 0) { $lookahead_brace_count--; }
			else {
				$lookahead_mode = 0;

				if ($skipping_lines_because_no_semicolon_yet) {
					die "I'm skipping lines because I haven't found a semicolon yet...but the block just ended.\nLine $line_number\nLine:\n$line\nIf there aren't problems with the .gas file, get Dave T. to fix this.\n";
				}
				
				#print "Setting line counter from $. to $saved_line_number.\n";

				$block[$i]{'total_lines'} = $line_number - $saved_line_number + 1; # the +1 accounts for the actual [whatever] line above the '{' line

				#$global_position = (int (1000 * ($line_number / $#the_whole_file))) / 1000;
				#if ($i > 1) { $local_position = (int (1000 * (($line_number - $block[$i - 1]{'start_line'}) / $block[$i - 1]{'total_lines'}))) / 1000;
				#}
				#elsif ($i == 1) { $local_position = "1.000";
				#}
				#print OUTPUT " _gpos=\"$global_position\" _lpos=\"$local_position\">\n";

				$global_percentage = (int (1000 * ($block[$i]{'total_lines'} / $#the_whole_file))) / 1000;
				if ($i > 1) {
#					print "i is $i, block i total lines is $block[$i]{'total_lines'} block i-1 total lines is $block[$i - 1]{'total_lines'}\n";
					$local_percentage = (int (1000 * ($block[$i]{'total_lines'} / $block[$i - 1]{'total_lines'}))) / 1000;
				}
				elsif ($i == 1) { $local_percentage = $global_percentage; }
				print OUTPUT " _gpercent=\"$global_percentage\" _lpercent=\"$local_percentage\">\n";
				
				$line_number = $saved_line_number;
			}
		
		}
		elsif ($lookahead_brace_count != 0) { } # if $lookahead_brace_count isn't zero, don't print		
    	elsif (($skipping_lines_because_no_semicolon_yet) and ($line !~ /;/)) {
    		print OUTPUT "(skipped line $line_number)";
    	}
    	
    	elsif ($skipping_lines_because_no_semicolon_yet) {
    		print OUTPUT "(skipped line $line_number, found semicolon)\"";
    		$skipping_lines_because_no_semicolon_yet = 0;
    	}
		elsif ($line =~ /(.*) (.*) = (.*);/) {
			$two = $2;
			$three = $3;
			$two =~ s/\*/_any$anycount/g;
			$anycount++;
			if ($two =~ /(.*)\$$/) {$two = $1;}
			if ($two =~ /^\d/) {
				print OUTPUT " \_$two=\"$three\"";
			}
			else {	
				print OUTPUT " $two=\"$three\"";
			}
		}
		
		elsif ($line =~ /[\t ]*(.*) = (.*);/) {
			$one = $1;
			$two = $2;
			$one =~ s/\*/_any$anycount/g;
			$anycount++;
			if ($one =~ /(.*)\$$/) {$one = $1;}
			if ($one =~ /^\d/) {
				print OUTPUT " \_$one=\"$two\"";
			}
			else {	
				print OUTPUT " $one=\"$two\"";
			}
		}

		elsif ($line =~ /(.*) (.*) = (.*)/) {
			$two = $2;
			$three = $3;
			$two =~ s/\*/_any$anycount/g;
			$anycount++;
			if ($two =~ /(.*)\$$/) {$two = $1;}
			if ($two =~ /^\d/) {
				print OUTPUT " \_$two=\"$three";
			}
			else {	
				print OUTPUT " $two=\"$three";
			}
    		$skipping_lines_because_no_semicolon_yet = 1;
		}
		
		elsif ($line =~ /[\t ]*(.*) = (.*)/) {
			$one = $1;
			$two = $2;
			$one =~ s/\*/_any$anycount/g;
			$anycount++;
			if ($one =~ /(.*)\$$/) {$one = $1;}
			if ($one =~ /^\d/) {
				print OUTPUT " \_$one=\"$two";
			}
			else {	
				print OUTPUT " $one=\"$two";
			}
    		$skipping_lines_because_no_semicolon_yet = 1;
		}

		else {
			die "***Get Dave T. to fix this***\nThis wasn't what I expected:\n$line\nOn line $line_number\n";
		}

	}

	else { # non-lookahead parsing
	
		$percent_done = int (($line_number / $#the_whole_file) * 100);
		if ($percent_done > $previous_percent_done) {print "Parsing... " . $percent_done . "%\r";}
		$previous_percent_done = $percent_done;
	
		if ($line =~ /\[(.*)\]/) {
			$text = $1;
			if ($text =~ /^\d/) {
				$text = "_$text";
			}
			$label[$i] = $text;
		}
		
		elsif ($line =~ /\{/) {
				$anycount = 0;
				#print OUTPUT ">\n" if (not $closed);
				#$closed = 0;
				for ($j = 0; $j < $i; $j++) {
					print OUTPUT "\t";
				}
				print OUTPUT '<' . $label[$i];
				$i++;
				$saved_line_number = $line_number;
				$block[$i]{'start_line'} = $line_number;
				$lookahead_brace_count = 0;
				$lookahead_mode = 1; #look ahead and print all values at this bracelevel
				#print "Entering lookahead mode on line $.\n";
		}
    	
		elsif ($line =~ /\}/) {
			$anycount = 0;
			$i--;
			#print OUTPUT '>' if (not $closed);
#			if ($closed) {
				for ($j = 0; $j < $i; $j++) {
					print OUTPUT "\t";
				}
#			}
			print OUTPUT '</' . $label[$i] . '>' . "\n";
			#$closed = 1;
		}
    	
#		elsif ($line =~ /\}/) {
#			$i--;
#			for ($j = 0; $j < $i; $j++) {
#				print OUTPUT "\t";
#			}
#			print OUTPUT '</' . $label[$i] . '>' . "\n";
#		}
#		
#		elsif ($line =~ /(.*) (.* = .*)/) {
#			for ($j = 0; $j < $i; $j++) {
#				print OUTPUT "\t";
#			}
#			print OUTPUT $2 . "\n";
#		}
    	
		# These two output blocks are commented out now that I have put a lookahead section above

# In fact, I'm going to comment out this whole last section.  The lookahead block should take care of it.
# the legacy "haven't found semi, skipping lines" stuff was messing stuff up" (10/26/01 -- DT)

#		elsif ($line =~ /(.*) (.*) = (.*);/) {
#			#$two = $2;
#			#$three = $3;
#			#$two =~ s/\*/_any$anycount/g;
#			#$anycount++;
#			#if ($two =~ /(.*)\$$/) {$two = $1;}
#			#if ($two =~ /^\d/) {
#			#	print OUTPUT " \_$two=\"$three\"";
#			#}
#			#else {	
#			#	print OUTPUT " $two=\"$three\"";
#			#}
#		}
#			
#		elsif ($line =~ /[\t ]*(.*) = (.*);/) {
#			#$one = $1;
#			#$two = $2;
#			#$one =~ s/\*/_any$anycount/g;
#			#$anycount++;
#			#if ($one =~ /(.*)\$$/) {$one = $1;}
#			#if ($one =~ /^\d/) {
#			#	print OUTPUT " \_$one=\"$two\"";
#			#}
#			#else {	
#			#	print OUTPUT " $one=\"$two\"";
#			#}
#		}
#
#    	elsif ($line !~ /;/) {
#    		#print OUTPUT "(skipped line)";
#    		$skipping_lines_because_no_semicolon_yet = 1;
#    	}
#    	
#    	elsif ($skipping_lines_because_no_semicolon_yet) {
#    		#print OUTPUT "(skipped line)";
#    		$skipping_lines_because_no_semicolon_yet = 0;
#    	}
#
#		else {
#			die "(Get Dave T. to fix this)\nThis wasn't what I expected:\n$line\nOn line $line_number\n";#print OUTPUT $line;
#		}
	} # end non-lookahead parsing
}

print OUTPUT "</gas>\n";

close OUTPUT or die "\nCouldn't close output file!\n";

#get rid of temp file
system ("\@del $temp_file > NUL");

print "Done parsing and writing file.\n";
print "Total number of lines parsed: $total_lines.\n";
