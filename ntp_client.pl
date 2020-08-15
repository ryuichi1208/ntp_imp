#!perl
use strict;
use warnings;

use Net::NTP ();
use YAML;

my %response = Net::NTP::get_ntp_response('jp.pool.ntp.org');
die YAML::Dump(\%response);
