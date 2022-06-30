#!/usr/bin/perl -w

use File::Basename;
use Getopt::Long;
use Sys::Hostname;
use Data::Dumper;
use Cwd;

use strict;

BEGIN {
    my $pkgdatadir = getcwd;
    unshift @INC, $pkgdatadir;
}

use GMJobsInfo;
