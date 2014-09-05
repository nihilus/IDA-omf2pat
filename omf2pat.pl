#!/usr/bin/perl
# -*- perl -*-

	use integer;
	use strict;
	use Getopt::Long; # command line parsing
	use dir;
	use Win32;

	my ($verbose, $recursive, $outpath, $backup);
	Getopt::Long::Configure qw(prefix_pattern=(--|-|\+|\/) no_ignore_case);
	GetOptions('verbose|v' => \$verbose, 'recursive|r|s' => \$recursive,
		'outputdir|o' => \$outpath, 'backup|b!' => \$backup);
	$outpath = $outpath ? (Win32::GetFullPathName($outpath))[0] : '.\\';
	push @ARGV, ('*.obj') unless (@ARGV);
	print STDERR "omf2pat.pl v1.0 by servil\n\n";
	my $bcname = qr/[_a-zA-Z]\w*/;
	my $patoffset = qr/(\:\-?[a-zA-Z\d]{4}\@?\s+)/;
	my $bcpproc = qr/$patoffset((?:\@\w+)*\@{1,2}$bcname(?:\@\$[a-zA-Z]+)?\$[a-zA-Z][\w\@\$\%\&\?]+)(?!\S)/;
	my $bcppclassordata = qr/$patoffset((?:\@\w+)*\@$bcname\@?)(?!\S)/;
	my $bcpptype = qr/$patoffset\@\$x[pt]\$(\d*)([\w@\$\%\&\?]+)(?!\S)/;
	for (@ARGV) {
		for (globfiles($_, $recursive)) {
			print STDERR "Processing file: $_\n";
			my $basename = (Win32::GetFullPathName($_))[1];
			my ($objfile, $patfile);
			if (m/\.pat/i) {
				$patfile = $_;
				$objfile = derive_filename($_, 'obj');
			} else {
				$patfile = $outpath.derive_filename($basename, 'pat');
				$objfile = $_;
				if (system("plb \"$objfile\" \"$patfile\"") != 0) {
					warn "warning: plb \"$objfile\" \"$patfile\" failed";
					next;
				}
			}
			my @tdump = `tdump \"$objfile\"`;
			my ($unitname, $impl);
			for (@tdump) {
				if ($impl == 2 && m/^\s*Name\s*\:\s*(.*?)\s*/i) {
					$unitname = ucfirst(lc($1));
					last if ($unitname);
				} elsif ($impl == 1 && m/^\s*LIBMOD\s*$/i) {
					$impl = 2;
				} else {
					$impl = m/\bClass\:\s+163\s+\(\s*0A3h\s*\)/i;
				}
			}
			unless ($unitname) {
				undef $impl;
				for (@tdump) {
					if ($impl && m/^\s*Implements\:\s+(.*)\.obj\b/i) {
						$unitname = ucfirst(lc($1));
						last if ($unitname);
					} else {
						$impl = m/\bClass\:\s+251\s+\(\s*0FBh\s*\)\,\s+SubClass\:\s+10\s+\(\s*0Ah\s*\)/i
					}
				}
			}
			unless ($unitname) {
				for (@tdump) {
					if (m/^\s*[a-f\d]+\s+THEADR\s+(?:.*\\)*([^\.]+)/i) {
						$unitname = ucfirst(lc($1));
						last if ($unitname);
					}
				}
			}
			unless ($unitname) {
				$basename =~ m/^[^\.]+/;
				$unitname = ucfirst(lc($&));
				print STDERR "$basename: warning: no header retrieved, defaults to $unitname\n";
			}
			open IFH, '<', $patfile or (warn "$!\n", next);
			my @patfile = <IFH>;
			close IFH or warn "$!\n";
			my $changed;
			for my $line (@patfile) {
				my ($prematch, $postmatch);
				if ($line =~ m/$bcpproc/ || $line =~ m/$bcppclassordata/) {
					$prematch = $`.$1;
					$postmatch = $2.$';
					unless ($2 =~ m/^\@\Q$unitname@\E/i) {
						$line = $prematch.'@'.$unitname.$postmatch;
						$changed++;
					}
				} elsif ($line =~ m/$bcpptype/) {
					$prematch = $`.$1.substr($&, 0, 5);
					$postmatch = $';
					my $var = $3;
					if ($2 && $3 !~ m/^\Q$unitname@\E/i) {
						$line = $prematch.sprintf('%u%s@%s', length($unitname) +
							1 + length($var), $unitname, $var).$postmatch;
						$changed++;
					}
				}
			}
			if ($changed) {
				rename $patfile, $patfile.'.bak' if ($backup);
				open OFH, '>', $patfile or (warn "$!\n", next);
				print OFH @patfile;
				close OFH or warn "$!\n";
				print STDERR "Pattern file updated successfully: unitname $unitname, changedtotal $changed\n";
			}
		}
	}
