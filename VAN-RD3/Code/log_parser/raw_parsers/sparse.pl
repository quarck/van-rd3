#!/usr/bin/perl

use strict;
use warnings;

sub e_man_decode($)
{
	my $out;
	my $arg = $_[0];
	while ($arg =~ s/^([01])([01])([01])([01])([01])//) {
		my $bt = int($1) * 8 + int($2) * 4 + int($3) * 2;
		if ($4 eq '1' and $5 eq '0') {
			$bt += 1; 
		} elsif ($4 eq '0' and $5 eq '1') {
			$bt += 0; # symbolic... 
		} else {
			printf STDERR "**** E-Manchester error on ".$_[0]."\n";
		}

		$out = $out.(sprintf "%x", $bt);
	}

	return $out;
}

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
	$value = hex($value) != 0 ? 0 : 1;
	my $prev_cnt = 0;

	while ($line =~ s/^([\da-f]{2})//) {
		my $cnt = hex($1);
		my $diff = ($cnt- $prev_cnt + 256) % 256;
		$prev_cnt = $cnt;
		my $tm = int($diff / 16 + 0.49);
		$tm = 1 if $tm == 0;
		$value = $value == 0 ? 1 : 0; # invert

		print "$diff ";
#		for (my $i=0; $i< hex($diff); $i++) {
#			print ''.($value ? 'H' : '_');
#		}

	
		for (my $i=0; $i < $tm; $i++) {
			$bits .= $value; # push @bits, $v; #print $v;
		}
	}

print "\n\n\n\n";
next;
	
# pream - 10ts
# iden - 12 ts
# com - 5 ts
# data - 0-280 ts
# ctrl - 18 ts
# eod - 2 ts (all zero on place of e-man)
# ack - 2 ts (1/0 0/1)
# EOF - 5 ts

	while (defined $bits and $bits =~ m/0000111101(.{15})(.{5})(.+)/) {
		my $id = e_man_decode($1);
		my $com = e_man_decode($2);
		my $bd = $3;
		my $data_with_ctrl;
		my $data; my $ctrl;
		while ($bd =~ s/^([01]{3})([01])([01])//) {
			if (($2 ne '0') or ($3 ne '0')) {
				$data_with_ctrl = $data_with_ctrl.&e_man_decode($1.$2.$3);
			} else {
				$data_with_ctrl = $data_with_ctrl.&e_man_decode($1.'01');
				last;
			}
		}

		my $ack;
		($ack, $bits) = $bd =~ m/^(.{2})(.+)$/;

		$ack = "undef" unless defined $ack;
		if (defined $data_with_ctrl) {
			($data, $ctrl) = $data_with_ctrl =~ m/^(.*)(.{4})/g;
			$data = "ERROR: $data_with_ctrl" unless defined $data;
			if (defined $ctrl) {
				$ctrl = sprintf "%04x", hex($ctrl) / 2; 
			} else {
				$ctrl = "ERROR";
			}
		} else {
			$data = "undef";
			$ctrl = "undef";
		}
		print "$id $com $data $ctrl $ack\n";
	}
}

