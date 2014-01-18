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

	my $prev_cnt = 0;
	while ($line =~ s/^([\da-f]{2})//) {
		my $cnt = hex($1);	
		my $tm = int((($cnt-$prev_cnt) / 16) + 0.49);
		$prev_cnt = $cnt;

		for (my $i=0; $i < $tm; $i++) {
			$bits .= $value; # push @bits, $v; #print $v;
		}
		$value = $value == 0 ? 1 : 0; # invert
	}


	print $bits."\n\n";
}

