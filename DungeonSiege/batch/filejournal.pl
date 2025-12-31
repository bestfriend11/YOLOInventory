# by: Scott Bilas
#
# this script reads in a DS file journal, removes duplicates, and cleans it up
# for input into the tank builder.

%files = ();

while ( <> )
{
	if ( !/:\/\// && /Open: / )
	{
		$_ = "/" . lc $';
		s/\//\\/g;

		push( @uniq, $_ ) unless $files{ $_ }++;
	}
}

print @uniq;
