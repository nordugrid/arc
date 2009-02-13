package LRMSInfo;

use base InfoCollector;
use InfoChecker;

use Storable;

use strict;

##############################################################################
# To include a new LRMS:
##############################################################################
#
# Each LRMS specific module needs to:
#
# 1. Provide subroutine get_lrms_info. The interfaces are documented in this
#    file. All variables required in lrms_info_schema should be defined in
#    LRMS modules.  Returning empty variable "" is perfectly ok if variable
#    does not apply to a LRMS.
#    
# 2. Provide subroutine get_lrms_options_schema. The return value must be a
#    schema describing the options that are recognized by the plugin.

##############################################################################
# Public interface to LRMS modules
##############################################################################
#
# use LRMSInfo;
#
# my $collector = LRMSInfo->new();
# my $lrms_info = $collector->get_info($options);
#
# Arguments:
#    $options - a hash reference containing options. This module will check it
#               against $lrms_options_schema and the LRMS plugin's own schema
#               and then pass it on to the LRMS plugin.
#
# Returns:
#    $lrms_info - a hash reference containing all information collected from
#                 the LRMS. This module will check it against
#                 $lrms_info_schema (see below)


##############################################################################
# Schemas
##############################################################################
#
# The usage of these schemas is described in InfoChecker.pm
#
#    $lrms_options_schema - for checking $options hash. This is just a minimal
#                           schema, LRMS plugins may use an extend version
#    $lrms_info_schema - for checking data returned by LRMS modules

my $lrms_options_schema = {
    'lrms' => '',              # name of the LRMS module
    'queues' => {              # queue names are keys in this hash
        '*' => {
	    'users' => [ '' ]  # list of user IDs to query in the LRMS
        }
    },
    'jobs' => [ '' ]           # list of jobs IDs to query in the LRMS
};

my $lrms_info_schema = {
    'cluster' => {
        'lrms_type'       => '',
        'lrms_glue_type'  => '',  # one of: bqs condor fork loadleveler lsf openpbs sungridengine torque torquemaui ...
        'lrms_version'    => '',
        'scheduling_policy' => '*',
        'totalcpus'       => '',
        'queuedcpus'      => '',
        'usedcpus'        => '',
        'queuedjobs'      => '',
        'runningjobs'     => '',
        'cpudistribution' => ''
    },
    'queues' => {
        '*' => {
            'status'       => '',
            'maxrunning'   => '',  # the max number of jobs allowed to run in this queue
            'maxqueuable'  => '*', # the max number of jobs allowed to be queued
            'maxuserrun'   => '*', # the max number of jobs that a singele user can run
            'maxcputime'   => '*', # units: seconds
            'mincputime'   => '*', # units: seconds
            'defaultcput'  => '*', # units: seconds
            'maxwalltime'  => '*', # units: seconds
            'minwalltime'  => '*', # units: seconds
            'defaultwallt' => '*', # units: seconds
            'running'      => '',  # the number of cpus being occupied by running jobs
            'queued'       => '',  # the number of queued jobs
            'total'        => '*', # the total number of jobs in this queue
            'totalcpus'    => '',  # the number of cpus dedicated to this queue
            'preemption' => '*',
            'acl_users'  => [ '*' ],
            'users' => {
                '*' => {
                    'freecpus'    => {
                        '*' => ''  # key: # of cpus, value: time limit in minutes (0 for unlimited)
                     },
                    'queuelength' => ''
                }
            }
        }
    },
    'jobs' => {
        '*' => {
            'status'      => '',
            'cpus'        => '*',
            'rank'        => '*',
            'mem'         => '*', # units: kB
            'walltime'    => '*', # units: seconds
            'cputime'     => '*', # units: seconds
            'reqwalltime' => '*', # units: seconds
            'reqcputime'  => '*', # units: seconds
            'nodes'     => [ '*' ], # names of nodes where the job runs
            'comment'   => [ '*' ]
        }
    }
};

our $log = LogUtils->getLogger("LRMSInfo");


# Loads the needed LRMS plugin at runtime
# First try to load XYZmod.pm (implementing the native ARC1 interface)
# otherwise try to load XYZ.pm (ARC0.6 plugin)

sub load_lrms($) {
    my $lrms_name = uc(shift);

    my $module = $lrms_name."mod";
    eval { require "$module.pm" };

    if ($@) {
        $log->debug("Native ARC1 LRMS module $module not found");
        $log->debug("Falling back to ARC0.6 compatible module $lrms_name");

        require ARC0mod;
        ARC0mod::load_lrms($lrms_name);
        $module = "ARC0mod";
    }
    import $module qw(get_lrms_info get_lrms_options_schema);
}


# override InfoCollector base class methods 

sub _check_options($$) {
    my ($self,$options) = @_;

    # store this for later use
    $self->{options} = $options;

    my $lrms_name = $options->{lrms};
    $log->error('lrms option is missing') unless $lrms_name;

    load_lrms($options->{lrms});

    # merge schema exported by the LRMS plugin
    my %schema = (%$lrms_options_schema, %{get_lrms_options_schema()});
    $self->{options_schema} = \%schema;

    return $self->SUPER::_check_options($options);
}

# will be used by InfoCollector::_check_options

sub _get_options_schema($) {
    my ($self) = @_;
    return $self->{options_schema} || $lrms_options_schema;
}

# will be used by InfoCollector::_check_results

sub _get_results_schema($) {
    my ($self,$results) = @_;
    my $options = $self->{options};
    return customize_info_schema($lrms_info_schema,$options);
}

sub _collect($$) {
    my ($self,$options) = @_;

    # exported by the LRMS plugin
    return get_lrms_info($options);
}


# prepares a custom schema that has individual keys for each queue and each job
# which is named in $options

sub customize_info_schema($$) {
    my ($info_schema,$options) = @_;

    my $new_schema;

    # make a deep copy
    $new_schema = Storable::dclone($info_schema);

    # adjust schema for each job: Replace "*" with actual job id's
    for my $job (@{$options->{jobs}}) {
        $new_schema->{jobs}{$job} = $new_schema->{jobs}{"*"};
    }
    delete $new_schema->{jobs}{"*"};

    # adjust schema for each queue: Replace "*" with actual queue names
    for my $queue (keys %{$options->{queues}}) {
        $new_schema->{queues}{$queue} = $new_schema->{queues}{"*"};
    }
    delete $new_schema->{queues}{"*"};

    return $new_schema;
}

#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

my $opt1 = {lrms => 'fork',
            sge_root => '/opt/n1ge6',
            sge_cell => 'cello',
            sge_bin_path => '/opt/n1ge6/bin/lx24-x86',
            queues => {'shar' => {users => []}, 'loca' => {users => ['joe','pete'], fork_job_limit => 5}},
            jobs => [qw(7 101 5865)]
           };

my $opt2 = {lrms => 'sge',
            sge_root => '/opt/n1ge6',
            sge_cell => 'cello',
            sge_bin_path => '/opt/n1ge6/bin/lx24-x86',
            queues => {'shar' => {users => []}, 'loca' => {users => ['joe','pete']}},
            jobs => [63, 36006]
           };

my $opt3 = {lrms => 'pbs',
            pbs_bin_path => '/opt/torque/bin',
            pbs_log_path => '/var/spool/torque/server_logs',
            queues => {'batch' => {users => ['joe','pete']}},
            jobs => [63, 453]
           };

sub test {
    my $options = shift;
    LogUtils->getLogger()->level($LogUtils::DEBUG);
    require Data::Dumper; import Data::Dumper qw(Dumper);
    $log->debug("Options: " . Dumper($options));
    my $results = LRMSInfo->new()->get_info($options);
    $log->debug("Results: " . Dumper($results));
}

#test($opt3);

1;
