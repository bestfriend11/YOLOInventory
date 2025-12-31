# Author: Dave Tomandl
# This script takes input (from the input file defined below) in the following form:
#[region]
#{
#	[container_name]
#	{
#		In here is text that shall be treated as a block.
#		If it's between those curly braces, it will be grabbed.
#		It can contain curly braces, as long as they match. (ie, no '}' without a preceeding '{')
#		When the closing curly brace is seen below, then that's the end of this block of text.	
#	}
#	[another_container_name]
#	{
#		another block of text
#	}
#}
#[another_region]
#...etc.

# Then it outputs to container.gas inside (output_map_path)\regions\(region_name)\objects\container.gas

# Each instance of the container listed in the input file will get the block of text copied into it.



#############################################################
#  CHANGE THESE TWO VARIABLES TO BE CORRECT FOR YOUR MACHINE

# This is the file which has the master list of pcontent per container type per region
$input_file = 'c:\dave\test\containercontent.txt';

# This points to the map where you wish to modify the container information
$output_map_path = 'C:\vssdir\TATTOO_ASSETS\CDimage\world\maps\davet_test';

#
#############################################################


###########  INPUT  ###################
open (INPUT, "<" . $input_file) or die "Could not open $input_file!\n";

#print "input file opened\n";
$bracecount = 0;
while ($line = <INPUT>) {
	$linenumber++;
#	print $line;	
	$ignore = 0;
	if ($line =~ /\{/) {
		$bracecount++;
		if ($bracecount == 2) {
#			$ignore = 1;
			if (not $container) {
				die "Need container name at line $linenumber!\n";
			}
		}
		if ($bracecount == 3) {
			$ignore = 1;
		}
	}
	if ($line =~ /\}/) {
		$bracecount--;
		if ($bracecount == 0) {
			$regionlist[$regionlistcount++] = $region;
			undef $region;
			undef $region_container_list_count;
		}	
		if ($bracecount == 1) {
			$$region[$region_container_list_count++] = $container;
			$$region{$container} = $block_text;
			undef $container;
			undef $block_text;
		}
	}
	if ($bracecount < 0) {die "Too many right curly braces! Error on line $linenumber.\n";}

	if (not $ignore) {
		if ($bracecount == 0) {
			if ($line =~ /\[(.*)\]/) {
				$region = $1;
			}
		}
		if ($bracecount == 1) {
			if ($line =~ /\[(.*)\]/) {
				$container = $1;
			}
		}
		if ($bracecount == 2) {
			if ($line =~ /\[(.*)\]/) {
				if ($1 ne "pcontent") {die "A non-pcontent block was spotted at line $linenumber!\n";}
			}
		}
		if ($bracecount > 2) {
			$block_text .= $line;
		}
	}
}

close INPUT or die "Could not close $input_file!\n";
#print "input file closed\n";

###########  END INPUT  ###################

### Debug stuff
#print "This is the region list:\n";
#foreach $region (@regionlist) {
#	print $region . "\n";
#	foreach $container (sort keys %$region) {
#		print "\t$container\n";
#		print "$$region{$container}\n";
#	}
#}
#die "have a nice day\n";
###########  OUTPUT  ###################


# I need to look in each container.gas file, and find each instance of the containers that need modifying.
# Once I find an instance, I need to see if it already has an inventory block, and if so, if it already has a pcontent block.
# Existing pcontent blocks will be discarded, and the new pcontent blocks will go in their place.

#$looping_through = 0;
#print $#regionlist . "\n";
foreach $region (@regionlist) {

#	print "Looping through $looping_through times...\n";
#	$looping_through++; if ($looping_through == 10) {exit(0);}
#	print $region . "\n";
	open (READ_CONTAINER, "<" . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas') or die "Couldn't open " . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas' . " for reading!\n";
	open (TEMPFILE, ">" . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas.new') or die "Couldn't open " . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas.new' . " for writing!\n";

#	print TEMPFILE "File opening...\n";
	
	$bracecount = 0;
	$linenumber = 0;
	$inventory_level = 0;
#	$print_block_this_time = 0;
	while ($line = <READ_CONTAINER>) {
		$linenumber++;
#		print "line $linenumber\n";
#	print $line;
#		$ignore = 0;
		$print_block_this_time = 0;
		$do_not_print_this_line = 0;
		if ($line =~ /inventory/) {$inside_inventory = 1;}
#		if ($line =~ /pcontent/) {$inside_pcontent = 1;}
		if ($line =~ /\{/) {
			$bracecount++;
#			print TEMPFILE "bracecount increased\n";
#			if ($inside_pcontent and not $pcontent_level) {$pcontent_level = $bracecount;}
			if (($inside_inventory) and (not $inventory_level)) {$inventory_level = $bracecount;} #print "inv_lvl = $bracecount\n";}
			if (not $template) {die "Need template name at line $linenumber!\nline:\n$line\n";}
		}
		if ($line =~ /\}/) {
			$bracecount--;
#			print TEMPFILE "bracecount decreased\n";
#			if ($pcontent_flag_set and ($bracecount < $pcontent_level)) {undef $pcontent_level; $pcontent_flag_set = 0; $inside_pcontent = 0;}
#			if ($bracecount < $pcontent_level) {undef $pcontent_level; $inside_pcontent = 0;}
			if ($bracecount < $inventory_level) {undef $inventory_level; $inside_inventory = 0; $do_not_print_this_line = 1;}

			if (($bracecount == 0) and (not $this_one_is_done) and ($$region{$template})) {
				$this_one_is_done = 1;
				$print_block_this_time = 1;
			}
			elsif ($bracecount == 0) {
				undef $template;
				undef $this_one_is_done;
			}
		}
		if ($bracecount < 0) {die "Too many right curly braces! Error in " . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas' . "on line $linenumber.\n";}

		if ($bracecount == 0) {
			if ($line =~ /\[t\:(.*),n\:[0123456789abcdefx]*\]/) {
					$template = $1;
			}
		}

#		if (($bracecount == 1) and (not $this_one_is_done) and ($$region{$template})) {
#			$this_one_is_done = 1;
#			$print_block_this_time = 1;
#		}
				
		if (($inside_inventory) and (not $this_one_is_done) and ($$region{$template})) {
			$print_block_this_time = 1;
			$this_one_is_done = 1;
		}
		
		if ($print_block_this_time) {
			if (not $inside_inventory) {$bracecount++;}
			for ($i = 0; $i < $bracecount; $i++) {
				print TEMPFILE "\t";
			}
			print TEMPFILE "[inventory]\n";
			for ($i = 0; $i < $bracecount; $i++) {
				print TEMPFILE "\t";
			}
			print TEMPFILE "\{\n";
			for ($i = 0; $i < ($bracecount + 1); $i++) {
				print TEMPFILE "\t";
			}
			print TEMPFILE "[pcontent]\n";
			for ($i = 0; $i < ($bracecount + 1); $i++) {
				print TEMPFILE "\t";
			}
			print TEMPFILE "\{\n";
			print TEMPFILE $$region{$template};
			for ($i = 0; $i < ($bracecount + 1); $i++) {
				print TEMPFILE "\t";
			}
			print TEMPFILE "\}\n";
			for ($i = 0; $i < $bracecount; $i++) {
				print TEMPFILE "\t";
			}
			print TEMPFILE "\}\n";
			if (not $inside_inventory) {$bracecount--;}
		}
		print TEMPFILE $line if ((not $inside_inventory) and (not $do_not_print_this_line));#(not $inside_pcontent) or (not $pcontent_flag_set));

		if (($bracecount == 0) and ($print_block_this_time)) {
			undef $template;
			undef $this_one_is_done;
		}

	}
#	print TEMPFILE "Closing file...\n";
	close READ_CONTAINER or die "Couldn't close container file!\n"; #for more detail, copy the death stuff from above
	close TEMPFILE or die "Couldn't close tempfile!\n"; #for more detail, copy the death stuff from above
}

#	foreach $container (sort keys %$region) {
#		print "\t$container\n";
#		print "$$region{$container}\n";
#	}
#}



foreach $region (@regionlist) {

	$do_not_print = 0;

	open (TEMPFILE, "<" . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas.new') or die "Couldn't open " . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas.new' . " for reading!\n";
	open (WRITE_CONTAINER, ">" . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas') or (print "Couldn't open " . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas' . " for writing!\n") and ($do_not_print = 1);

	if (not $do_not_print) {
		while ($line = <TEMPFILE>) {print WRITE_CONTAINER $line;}
	}
	
	
	close TEMPFILE or die "Couldn't close tempfile!\n";

	if (not $do_not_print) {
		close WRITE_CONTAINER or die "Couldn't close container.gas in $region after writing to it!\n";
	}

	system("del " . $output_map_path . "\\regions\\"  . $region . '\objects\container.gas.new');
	wait;

}

###########  END OUTPUT  ###################
