#!/usr/bin/perl -w

use File::Basename;
use Getopt::Long;
use Sys::Hostname;
use Data::Dumper;
use Cwd;

use strict;

BEGIN {
    my $pkgdatadir = "/home/florido/src/ARC/arc1/trunk/src/services/a-rex/infoproviders";
    unshift @INC, $pkgdatadir;
}

use GMJobsInfo;
