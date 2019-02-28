package ARC0mod;

######################################################################
# DISCLAIMER
######################################################################
# This module is obsolete and deprecated starting from ARC 6.0 
# and all the modules depending on it are candidates for deprecation.
# Please DO NOT build new LRMS modules based on this one but follow
# the indications in
#                       LRMSInfo.pm
# instead.
######################################################################

#
# Loads ARC0.6 LRMS modules for use with ARC1
#

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# !!!! WARNING: DO *NOT* DEVELOP NEW MODULES BASED ON WHAT FOLLOWS !!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# THIS MODULE IS MAINTANED TO RUN LEGACY CODE.
# THE INFORMATION BELOW IS KEPT FOR MAINTENANCE REFERENCE. 
# THIS MODULE WILL BE DEPRECATED.
# ALL THE MODULES BASED ON THIS WILL BE REMOVED IN THE FUTURE.
# IF YOU PLAN TO DEVELOP A NEW PERL LRMS MODULE, 
# READ THE DISCLAIMER ABOVE.
#
# !!! LEAGACY DO NOT USE  FOR FUTURE DEVELOPMENTS - SEE DISCLAIMER !!!!
# To include a new (ARC 0.6) LRMS plugin:
#
# 1. Each LRMS specific module needs to provide subroutines
#    cluster_info, queue_info, jobs_info, and users_info.
#    
# 2. References to subroutines defined in new LRMS modules are added
#    to the select_lrms subroutine in this module, and the module reference
#    itself, naturally.
#
# NB: ARC0 modules use minutes for time units. ARC1 modules use seconds.
#
# !!! LEAGACY DO NOT USE  FOR FUTURE DEVELOPMENTS - SEE DISCLAIMER !!!!


require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(get_lrms_info get_lrms_options_schema);

use LogUtils;
use strict;

our $log = LogUtils->getLogger(__PACKAGE__);

our %modnames = ( PBS    => "PBS",
                  SGE    => "SGE",
                  LL     => "LL",
                  LSF    => "LSF",
                  CONDOR => "Condor",
                  SLURM  => "SLURM",
                  BOINC  => "Boinc",
                  FORK   => "Fork"
                );

# Whether the module implements support for listing nodes.
our $has_nodes = 1;

# input: lrmsname, loglevel
sub load_lrms($$) {
    my $lrms_name = uc(shift);
    my $loglevel = uc(shift);
    my $module = $modnames{$lrms_name};
    $log->error("No ARC0 module for $lrms_name") unless $module;
    eval { require "$module.pm" };
    $log->error("Failed to load LRMS module $module: $@") if $@;
    import $module qw(cluster_info queue_info jobs_info users_info);
    eval { import $module qw(nodes_info) };
    if ($@) {
        $log->debug("LRMS module $module does not export 'nodes_info'");
        $has_nodes=0;
    }
    $LogUtils::default_logger = LogUtils->getLogger($module);
    $LogUtils::loglevel = $loglevel;
}

# Just generic options, cannot assume anything LRMS specific here

sub get_lrms_options_schema {
    return {
        'lrms' => '',              # name of the LRMS module
        'queues' => {              # queue names are keys in this hash
            '*' => {
                'users' => [ '' ]  # list of user IDs to query in the LRMS
            }
        },
        'jobs' => [ '' ],           # list of jobs IDs to query in the LRMS
        'controldir' => '*',    # path to controldir, taken from main config
        'loglevel' => ''        # infoproviders loglevel
    }
}


sub get_lrms_info($) {
    my $options = shift;

    my %cluster_config = %$options;
    delete $cluster_config{queues};
    delete $cluster_config{jobs};

    my $lrms_info = {cluster => {}, queues => {}, jobs => {}};

    my $cluster_info = { cluster_info(\%cluster_config) };
    delete $cluster_info->{queue};
    $lrms_info->{cluster} = delete_empty($cluster_info);

    $lrms_info->{nodes} = { nodes_info(\%cluster_config) } if $has_nodes;

    for my $qname ( keys %{$options->{queues}} ) {

        my %queue_config = (%cluster_config, %{$options->{queues}{$qname}});
        delete $queue_config{users};

        my $jids = $options->{jobs};

        # TODO: interface change: jobs under each queue
        my $jobs_info = { jobs_info(\%queue_config, $qname, $jids) };
        for my $job ( values %$jobs_info ) {
            $job->{status} ||= 'EXECUTED';
            delete_empty($job);
        }
        $lrms_info->{jobs} = { %{$lrms_info->{jobs}}, %$jobs_info };

        my $queue_info = { queue_info(\%queue_config, $qname) };
        $lrms_info->{queues}{$qname} = delete_empty($queue_info);

        my $users = $options->{queues}{$qname}{users};

        $queue_info->{users} = { users_info(\%queue_config, $qname, $users) };
        for my $user ( values %{$queue_info->{users}} ) {
            my $freecpus = $user->{freecpus};
            $user->{freecpus} = split_freecpus($freecpus) if defined $freecpus;
            delete_empty($user);
        }
        $queue_info->{acl_users} = $queue_config{acl_users}
            if defined $queue_config{acl_users};

    }

    # ARC0 LRMS plugins use minutes. Convert to seconds here.

    for my $queue (values %{$lrms_info->{queues}}) {
        $queue->{minwalltime} = int 60*$queue->{minwalltime} if $queue->{minwalltime};
        $queue->{mincputime}  = int 60*$queue->{mincputime}  if $queue->{mincputime};
        $queue->{maxwalltime} = int 60*$queue->{maxwalltime} if $queue->{maxwalltime};
        $queue->{maxcputime}  = int 60*$queue->{maxcputime}  if $queue->{maxcputime};
        $queue->{defaultwallt} = int 60*$queue->{defaultwallt} if $queue->{defaultwallt};
        $queue->{defaultcput}  = int 60*$queue->{defaultcput}  if $queue->{defaultcput};
    }

    for my $job (values %{$lrms_info->{jobs}}) {
        $job->{reqcputime}  = int 60*$job->{reqcputime}  if $job->{reqcputime};
        $job->{reqwalltime} = int 60*$job->{reqwalltime} if $job->{reqwalltime};
        $job->{cputime}     = int 60*$job->{cputime}     if $job->{cputime};
        $job->{walltime}    = int 60*$job->{walltime}    if $job->{walltime};
        delete $job->{nodes} unless @{$job->{nodes}};
        delete $job->{comment} unless @{$job->{comment}};
    }

    return $lrms_info;
}


sub delete_empty($) {
    my $hashref = shift;
    foreach my $k ( keys %$hashref) {
        delete $hashref->{$k}
            if ! defined $hashref->{$k} || $hashref->{$k} eq '';
    }
    return $hashref;
}

# Convert frecpus string into a hash.
# Example: "6 11:2880 23:1440" --> { 6 => 0, 11 => 2880, 23 => 1440 }

# OBS: Assuming the function cpu vs. time is monotone, this transformation is safe.

sub split_freecpus($) {
    my $freecpus_string = shift;
    my $freecpus_hash = {};
    for my $countsecs (split ' ', $freecpus_string) {
        if ($countsecs =~ /^(\d+)(?::(\d+))?$/) {
            $freecpus_hash->{$1} = $2 || 0; # 0 means unlimited
        } else {
            $log->warning("Bad freecpus string: $freecpus_string");
            return {};
        }
    }
    return $freecpus_hash;
}

1;
