#!/usr/bin/perl

use strict;
use warnings;


open (F, '<', $ARGV[0]) || die "Open";
my @lines = <F>; 
close F;
chomp @lines;

#
# Yes, line level should been inverted!
#


foreach my $line (@lines) {

	my $bits;

	my ($value) = $line =~ s/^([\da-f]{2})//;
	$value = hex($value) == 0 ? 0 : 1;

	my $prev_tm = 0;
	while ($line =~ s/^([\da-f]{2})//) {
		my $tm = hex($1);
		my $cnt = int( ($tm-$prev_tm) );
		$prev_tm = $tm;

		for (my $i=0; $i< $cnt; $i++) {
			print ''.($value ? 'H' : '_');
		}
		$value = $value == 0 ? 1 : 0; # invert
	}

	print "\n\n\n\n";
}

