#!/usr/bin/perl

use strict;
use warnings;
use IO::Socket::INET;
use JSON::MaybeXS qw(decode_json);

# script options ---------------------------------------------------------------

my $client      = '../client/client';
my $dev_ip      = 'localhost';
my $basename    = 'truth';
my $dump_flags  = '--strict --tolerance 1';
my $ps_ip       = 'localhost';
my $ps_port     = 7735;
my $volatility_path = $ARGV[0];
my $volatility  = "python3 $volatility_path";
my $vol_plugin  = "linux.pidhashtable.PIDHashTable";


# get pids from ps-server ------------------------------------------------------

print "1. GET PIDS FROM PS-SERVER\n\n";

print "CONNECTING TO PS-SERVER AT $ps_ip:$ps_port ";

my $ps_sock = IO::Socket::INET->new(
  PeerAddr => $ps_ip,
  PeerPort => $ps_port,
  Proto    => 'tcp'
) or die;

print "OK\n";

print "RECEIVING PIDS ";

my %ps_pids;

$ps_pids{$1} = 1 while <$ps_sock> =~ /(\d+)/;

print "OK\n\n";


# get pids from memory dump ----------------------------------------------------

print "2. GET PIDS FROM MEMORY DUMP\n\n";

print "TRIGGER DUMP\n\n";

system("$client dump --basename $basename $dump_flags $dev_ip") and die;

print "\nTRANSFER DATA\n\n";

system("$client get --basename $basename $dev_ip") and die;

print "\nANALYZE WITH VOLATILITY\n\n";

my $truth_json = `$volatility -r json -f $basename.dat $vol_plugin`;

die if $?;

print "\n\nPARSE VOLATILITY RESULTS ";

my $truth_procs = decode_json($truth_json);

print "OK\n\n";


# compare results --------------------------------------------------------------

print "3. COMPARE RESULTS\n\n";

print "FILTER PROCESSES DETECTED BY VOLATILITY AND NOT DETECTED BY PS-SERVER ";

my @diff_procs = grep { !exists $ps_pids{$_->{'PID'}} } @$truth_procs;

print "OK\n";

if (!@diff_procs) {
  print "\nNO HIDDEN PROCESSES FOUND.\n\n";
  exit 0;
}

print "\nHIDDEN PROCESSES:\n\n";
print "PID=$_->{'PID'} COMM=$_->{'COMM'}\n" foreach @diff_procs;
print "\n";
