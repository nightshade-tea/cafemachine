#!/usr/bin/perl

# script options
my $client      = '../client/client';
my $dev_ip      = 'localhost';
my $basename    = 'truth';
my $dump_flags  = '--strict --tolerance 1';
my $ps_ip       = 'localhost';
my $ps_port     = 7735;
my $volatility  = 'python3 ~/src/volatility/volatility3/vol.py';


use strict;
use warnings;
use IO::Socket::INET;
use JSON::MaybeXS qw(decode_json);

my $ps_sock; # ps-server connection
my %ps_pids; # ps-server obtained pids

my $truth_json; # volatility log in json
my @truth_procs; # volatility obtained processes

my @diff_procs; # processes on @truth_procs that are not in %ps_pids

my $dump_cmd = "$client dump --basename $basename $dump_flags $dev_ip";
my $get_cmd = "$client get --basename $basename $dev_ip";

my $vol_plugin = "linux.pidhashtable.PIDHashTable";
my $vol_cmd = "$volatility -r json -f $basename.dat $vol_plugin";

# get pids from ps-server

$ps_sock = IO::Socket::INET->new(
  PeerAddr => $ps_ip,
  PeerPort => $ps_port,
  Proto    => 'tcp'
) or die;

$ps_pids{$1} = 1 while <$ps_sock> =~ /(\d+)/;


# get pids from memory dump

system($dump_cmd) and die;
system($get_cmd) and die;

$truth_json = decode_json(`$vol_cmd`);
push @truth_procs, $_ foreach @$truth_json;


# compare results

map { push @diff_procs, $_ unless exists $ps_pids{$_->{'PID'}} } @truth_procs;

print "\nhidden pids:\n\n";

print "PID=$_->{'PID'} COMM=$_->{'COMM'}\n" foreach @diff_procs;
