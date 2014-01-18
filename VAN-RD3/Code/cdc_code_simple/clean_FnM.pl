#!/usr/bin/perl


foreach my $file (@ARGV) {
	if (open F, '<', $file) {
		my @lines = <F>; close F;
		@lines = map {s/\r//g; $_} @lines;
		if (open O, '>', $file) {
			print O join '', @lines;
			close O;
		} else {
			printf STDERR "Could not open $file for writing\n";
			next;
		}
	} else {
		printf STDERR "Could not open $file for reading\n";
		next;
	}
}
