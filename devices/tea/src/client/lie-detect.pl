#!/usr/bin/perl
use strict;
use warnings;
use IO::Socket::INET;
use JSON::MaybeXS qw(decode_json);

system("make");
if ($? != 0) {
  warn "Failed to build client\n";
}

system("./client dump --strict --basename truth --tolerance 1 localhost");
if ($? != 0) {
  warn "Failed to dump client\n";
}

system("./client get --basename truth localhost");
if ($? != 0) {
  warn "Failed to get dump\n";
}

my $socket = IO::Socket::INET->new(
  PeerAddr => '127.0.0.1',
  PeerPort => 7735,
  Proto    => 'tcp'
) or die "Connection failed: $!";

my %pids;

while (<$socket>) {
  chomp;
  if (my ($pid) = $_ =~ /(\d+)/) {
    $pids{$pid} = 1;
  }
}

my $truth_json = `python3 ~/volatility3/vol.py -r json -f truth.dat linux.pidhashtable.PIDHashTable`;
my $truth = decode_json($truth_json); 

my @truth_pids;
push @truth_pids, $_->{PID} foreach @$truth;


foreach (@truth_pids) {
  print "$_ " unless (exists $pids{"$_"});
}
print "\n";



