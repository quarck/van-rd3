#!/usr/bin/perl


use strict;
use warnings;

sub handle_824
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} > 4) {
		my $rpm = ($bytes->[1] + 256 * $bytes->[0]) * 8000 / 65536;
		my $speed = ($bytes->[3] + 256 * $bytes->[2]) / 100;
		printf "RPM: ".int($rpm).", speed: ".int($speed)." km/h\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_e24
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 17) {
		my $vin = join '', map {chr($_)} @$bytes;
		print "Car VIN: $vin\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}


sub handle_4d4
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} > 10) {
		my $src = $bytes->[4] == 0x51 ? 'Radio' : $bytes->[4] == 0x52 ? 'CD' : 'UNKNOWN';
		my $vol = $bytes->[5];
		my $bass = $bytes->[6];
		my $treb = $bytes->[7];
		my $fader = $bytes->[8];
		my $balance = $bytes->[9];

		printf "Audio: playing $src, vol: $vol\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_524
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 14) {
		my $b4 = $bytes->[4];
		my $hbrk = $bytes->[5] ? 'On' : 'Off';
		my $b6 = $bytes->[6];
		my $b8 = $bytes->[8];
		my $b9 = $bytes->[9];

		my $key_in = $b4 & 0x20 ? 1 : 0;
		my $left_stick_btn = $b6 & 0x10 ? 1 : 0;
		my $seatbelt_warn = $b6 & 0x2 ? 1 : 0;
		my $car_locked = $b8 & 1 ? 1 : 0;
		
		printf "Handbrake: $hbrk, key_in: $key_in, left stckbtn: $left_stick_btn, sealbelt warn: $seatbelt_warn, car locked: $car_locked";
		if ($b9 == 1) {
			printf ", doors are open!\n";
		} else {
			printf "\n";
		}
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_8a4
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 7) {
		my $b0 = $bytes->[0];
		my $b1 = $bytes->[1];
		my $water_temp = $bytes->[2] * 170 / 256;
		my $oil_level = $bytes->[3];
		my $fuel_level = $bytes->[4];
		my $oil_temp = $bytes->[5] / 2;
		my $outside_temp = $bytes->[6] / 2;
		my $eng_running = $b1 & 0x4 ? 1 : 0;
		my $ign_on = $b1 & 0x2 ? 1 : 0;
		
		printf "Water temp: %0.0f, oil lvl:  %0.0f, fuel lvl: %0.0f, oil temp: %0.0f, outside temp: %0.0f, engine rn: $eng_running, ign_on: $ign_on\n", 
		       $water_temp, $oil_level, $fuel_level, $oil_temp, $outside_temp;
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_8c4
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 3) {
		my $b = $bytes->[2];
		my $bb = $b & 0x1f;
		if ($bb == 1) { print "CONTROL: Radio Btn 1" }
		elsif ($bb == 2) { print "CONTROL: Radio Btn 2" }
		elsif ($bb == 3) { print "CONTROL: Radio Btn 3" }
		elsif ($bb == 4) { print "CONTROL: Radio Btn 4" }
		elsif ($bb == 5) { print "CONTROL: Radio Btn 5" }
		elsif ($bb == 6) { print "CONTROL: Radio Btn 6" }
		elsif ($bb == 0x10) { print "CONTROL: Audio Up"   }
		elsif ($bb == 0x11) { print "CONTROL: Audio Down" }
		elsif ($bb == 0x16) { print "CONTROL: Sel audio"  }
		elsif ($bb == 0x1b) { print "CONTROL: Sel radio"  }
		elsif ($bb == 0x1d) { print "CONTROL: Sel cd"     }
		elsif ($bb == 0x1e) { print "CONTROL: Sel cdc"    }
		else { printf "CONTROL: unknown key code %02x", $bb }

		if ($bb & 0x40) {
			print " released\n";
		} else {
			print " pressed\n";
		}

	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}


sub handle_4fc
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 11) {
		my $b = $bytes->[5];

		printf "Lights: Dipped beam: %s, High beam: %s, Front fog: %s, Rear fog: %s, Rigth ind: %s, Left ind: %s\n",
		       ($b & 0x80) != 0 ? 'On': 'Off', 
		       ($b & 0x40) != 0 ? 'On': 'Off', 
		       ($b & 0x20) != 0 ? 'On': 'Off', 
		       ($b & 0x10) != 0 ? 'On': 'Off', 
		       ($b & 0x08) != 0 ? 'On': 'Off', 
		       ($b & 0x04) != 0 ? 'On': 'Off', 
		       ($b & 0x02) != 0 ? 'On': 'Off'; 
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_664
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 11) {
		printf "Car status not decoded: ".(join ' ', map {sprintf "%02x", $_} @$bytes)."\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}


sub handle_8d4
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 1) {
		my $b = $bytes->[0];
		my $b2 = scalar @{$bytes} > 1 ? $bytes->[1] : undef;
		print "Radio control: ";
		if ($b == 0x11) {
			if (defined $b2) {
				if ($b2 == 0x00) {
					print "Switch off radio";
				} elsif ($b2 == 0xc0) {
					printf "Update settings for loudness on";
				} elsif ($b2 == 0xc2) {
					printf "Update settings for loudness off";
				} elsif ($b2 == 0xe0) {
					printf "Update settings for speed volume on";
				} elsif ($b2 == 0xe1) {
					printf "Update settings for speed volume off";
				} else {
					print "unknown command: ";
					printf "%02x ", $_ foreach (@$bytes);
				}
			} else {
				printf "Unknown cmd 11";
			}
		} elsif ($b == 0x12) {
			if (defined $b2) {
				if ($b2 == 0x01) {
					print "Switch to radio";
				} elsif ($b2 == 0x02) {
					print "Switch to internal CD";
				} elsif ($b2 == 0x03) {
					print "Switch to CDC";
				} else {
					print "unknown command: ";
					printf "%02x ", $_ foreach (@$bytes);
				}

			} else {
				printf "Unknown cmd 12";
			}

		} elsif ($b == 0x14) {
			print "update fader, balance, bass, treble: ";
			printf "%02x ", $_ foreach (@$bytes);
		} elsif ($b == 0x27) {
			if (defined $b2) {
				print "Read present mem ".($b2-0x20);
			} else {
				print "unknown command: ";
				printf "%02x ", $_ foreach (@$bytes);
			}
		} elsif ($b == 0xd1) {
			printf "Tell radio to return radio info when reading from port 0x554";
		} elsif ($b == 0xd3) {
			printf "Tell radio to return preset stations when reading from port 0x554";
		} elsif ($b == 0xd6) {
			printf "Tell radio to return CD/track info when reading from port 0x554";
		} else {
			print "unknown command: ";
			printf "%02x ", $_ foreach (@$bytes);
		}
		printf "\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub bcd
{
	my ($i) = @_;
	$i = ($i >>4)*10 + ($i & 0xf);
	return $i;
}
sub handle_554
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 3) {
		shift @$bytes; 
		my $b1 = shift @$bytes;
		pop @$bytes;
		printf "Radio Info: ";
		if ($b1 == 0xd1) {
			print "Freq Info: ";
			my $f1 = bcd(shift @$bytes);
			my $f2 = bcd(shift @$bytes);
			my $freq = ($f1 * 100 + $f2) *0.05 + 50.0;
			for (1..7) {shift @$bytes} pop @$bytes;
			my $name  = join '', map {$_ != 0 ? chr($_) : ''} @$bytes;
			printf "F: %0.2f, name: $name", $freq;
		} elsif ($b1 == 0xd3) {
			print "Radio Preset Data: ";
			shift @$bytes;
			my $name  = join '', map {$_ != 0 ? chr($_) : ''} @$bytes;
			printf "name: $name";
		
		} elsif ($b1 == 0xd6) {
			print "CD Track Info:";
			shift @$bytes;
			shift @$bytes;
			shift @$bytes;
			my $mm = bcd(shift @$bytes);
			my $ss = bcd(shift @$bytes);
			my $tn = bcd(shift @$bytes);
			my $tt = bcd(shift @$bytes);
			
			print "Track $tn of $tt, cur: $mm:$ss\n";
		}

		printf "\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_564
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} == 27) {
		my $header = $bytes->[0];
		my $footer = $bytes->[26];
		my $doors = $bytes->[7];
		my $byte10 = $bytes->[10];
		my $trip1_mean_speed = $bytes->[11];
		my $trip2_mean_speed = $bytes->[12];
		my $trip1_dist = $bytes->[14] * 256 + $bytes->[15];
		my $trip1_fuel_conumpt = $bytes->[16] * 256 + $bytes->[17];
		my $trip2_dist = $bytes->[18] * 256 + $bytes->[19];
		my $trip2_fuel_conumpt = $bytes->[20] * 256 + $bytes->[21];
		my $instantaneous_fuel_consumpt = $bytes->[22] * 256 + $bytes->[23];
		my $kms_fuel_left  = ($bytes->[24] * 256 + $bytes->[25]) * 1.60934;

		printf "Car Status: ";
		print "FR Door open; " if ($doors & 0x80);
		print "FL Door open; " if ($doors & 0x40);
		print "RR Door open; " if ($doors & 0x20);
		print "RL Door open; " if ($doors & 0x10);

		print "T1: ms: $trip1_mean_speed, dist: $trip1_dist, fc: $trip1_fuel_conumpt, ".
			"T2: ms: $trip2_mean_speed, dist: $trip2_dist, fc: $trip2_fuel_conumpt, ".
			"Inst FC: $instantaneous_fuel_consumpt, kms left: $kms_fuel_left\n";

	} else {
		printf "Car Status: MSG_SHORT: $id $com @$bytes ".scalar @$bytes."\n";
	}
}

sub handle_984
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 5) {
		my $h = bcd($bytes->[3]);
		my $m = bcd($bytes->[4]);
		printf "Time: $h:%02d\n", $m;
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_5e4
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 1) {
		my $b = $bytes->[0];
		print "Display settings ignored\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}

sub handle_9c4
{
	my ($id, $com, $bytes, $crc, $ack) = @_;
	if (scalar @{$bytes} >= 2) {
		my $b0 = $bytes->[0];
		my $b1 = $bytes->[1];
		print "Radio Remote Control: ";
		print "Seek up; " if $b0 & 0x80;
		print "Seek down; " if $b0 & 0x40;
		print "UNK1; " if $b0 & 0x20;
		print "UNK2; " if $b0 & 0x10;
		print "Vol up; " if $b0 & 0x8;
		print "Vol down; " if $b0 & 0x4;
		print "Source; " if $b0 & 0x2;
		print "UNK3; " if $b0 & 0x1;
		print "Wheel Pos: $b1\n";
	} else {
		printf "MSG_SHORT: $id $com @$bytes\n";
	}
}




my %handlers = 
(
 	'824' => \&handle_824, 
 	'4d4' => \&handle_4d4, 
 	'8a4' => \&handle_8a4,
 	'524' => \&handle_524,
	'8c4' => \&handle_8c4,
	'4fc' => \&handle_4fc,
	'664' => \&handle_664,
	'e24' => \&handle_e24,
	'8d4' => \&handle_8d4,
	'564' => \&handle_564,
	'554' => \&handle_554,
	'984' => \&handle_984,
	'5e4' => \&handle_5e4,
	'9c4' => \&handle_9c4
 );



open (F, '<', $ARGV[0] ) || die "Waga-waga";

my %unhandled;

while (my $msg = <F>) {
	chomp $msg;
	if ($msg =~ s/^(.{3})(.)(.+)(.{4})(.{2})/$3/) {
		my $id = lc($1);
		my $com = $2;
		my $crc = $4;
		my $ack = $5;
		my @bytes;
		my $hack = hex($ack);
		print "$id $com $msg $crc $ack"."[".
			(($hack & 0x80) == 0 ? 'A':'-').
			(($hack & 0x40) == 0 ? 'A':'-').
			(($hack & 0x20) == 0 ? 'A':'-').
			(($hack & 0x10) == 0 ? 'A':'-').
			(($hack & 0x08) == 0 ? 'A':'-').
			(($hack & 0x04) == 0 ? 'A':'-').
			(($hack & 0x03) == 0x02 ? '':' INVALID CRC').
			"]\n";
		while ($msg =~ s/^(..)//) {
			push @bytes, hex($1);
		}
		print "Decoded: ";
		if (exists $handlers{$id}) {
			&{$handlers{$id}}($id, $com, \@bytes, $crc, $ack);
		} else {
			$unhandled{$id} = undef;
		}
		print "\n";
	}
}

close F;

if (scalar keys %unhandled) {
	printf "\n\n\nUnhandled codes are:\n";
	print join '', map {$_."\n"} keys %unhandled;
}
