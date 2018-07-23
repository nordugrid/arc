package LRMSInfo;

use Storable;

use strict;

use LogUtils;
use InfoChecker;

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
#                           schema, LRMS plugins may use an extended version
#    $lrms_info_schema - for checking data returned by LRMS modules

my $lrms_options_schema = {
	# C 10 new lrms block, more options are passed
    'lrms' => '',              # old name of the LRMS module 
    'defaultqueue' => '*',      # default queue, optional
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
        'lrms_glue_type'  => '*',  # one of: bqs condor fork loadleveler lsf openpbs sungridengine torque torquemaui ...
        'lrms_version'    => '',
        'schedpolicy'     => '*',
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
            'maxuserrun'   => '*', # the max number of jobs that a single user can run
            'maxcputime'   => '*', # units: seconds (per-slot)
            'maxtotalcputime' => '*', # units: seconds
            'mincputime'   => '*', # units: seconds
            'defaultcput'  => '*', # units: seconds
            'maxwalltime'  => '*', # units: seconds
            'minwalltime'  => '*', # units: seconds
            'defaultwallt' => '*', # units: seconds
            'running'      => '',  # the number of cpus being occupied by running jobs
            'queued'       => '',  # the number of queued jobs
            'suspended'    => '*', # the number of suspended jobs
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
    },
    'nodes' => {
        '*' => {                 # key: hostname of the node (as known to the LRMS)
            'isavailable' => '',      # is available for running jobs
            'isfree'      => '',      # is available and not yet fully used, can accept more jobs
            'tags'        => [ '*' ], # tags associated to nodes, i.e. node properties in PBS
            'vmem'        =>   '*',   # virtual memory, units: kb
            'pmem'        =>   '*',   # physical memory, units: kb
            'slots'       =>   '*',   # job slots or virtual processors
            'lcpus'       =>   '*',   # cpus visible to the os
            'pcpus'       =>   '*',   # number of sockets
            'sysname'     =>   '*',   # what would uname -s print on the node
            'release'     =>   '*',   # what would uname -r print on the node
            'machine'     =>   '*',   # what would uname -m print (if the node would run linux)
        }
    }
};

our $log = LogUtils->getLogger("LRMSInfo");


sub collect($) {
    my ($options) = @_;
    my ($checker, @messages);

    
    #print Dumper($options);
    # these lines are meant to take the 
    # default queue. Maybe should be done differently having the default
    # explicitly stored in the config.
    #my $lrmsstring = ($options->{lrms});
    #my ($lrms_name, $share) = split / /, $lrmsstring; # C 10 new lrms block
    my $lrms_name = $options->{lrms};
    my $share = $options->{defaultqueue};
    $log->error('lrms option is missing') unless $lrms_name;
    load_lrms($lrms_name);

    # merge schema exported by the LRMS plugin
    my $schema = { %$lrms_options_schema, %{get_lrms_options_schema()} };

    $checker = InfoChecker->new($schema);
    @messages = $checker->verify($options);
    $log->warning("config key options->$_") foreach @messages;
    $log->fatal("Some required options are missing") if @messages;

    my $result = get_lrms_info($options);

    use Data::Dumper('Dumper');
    my $custom_lrms_schema = customize_info_schema($lrms_info_schema, $options);
    $checker = InfoChecker->new($custom_lrms_schema);
    @messages = $checker->verify($result);
    $log->warning("return value lrmsinfo->$_") foreach @messages;

    # some backends leave extra spaces -- trim them
    $result->{cluster}{cpudistribution} =~ s/^\s+//;
    $result->{cluster}{cpudistribution} =~ s/\s+$//;

    # make sure nodes are unique
    for my $job (values %{$result->{jobs}}) {
        next unless $job->{nodes};
        my %nodes;
        $nodes{$_} = 1 for @{$job->{nodes}};
        $job->{nodes} = [ sort keys %nodes ];
    }

    return $result;
}

# Loads the needed LRMS plugin at runtime
# First try to load XYZmod.pm (implementing the native ARC1 interface)
# otherwise try to load XYZ.pm (ARC0.6 plugin)

sub load_lrms($) {
    my $lrms_name = uc(shift);

    my $module = $lrms_name."mod";
    eval { require "$module.pm" };
    
    if ($@) {
        $log->debug("require for $module returned: $@");
        $log->debug("Using ARC0.6 compatible $lrms_name module");

        require ARC0mod;
        ARC0mod::load_lrms($lrms_name);
        $module = "ARC0mod";
    }
    import $module qw(get_lrms_info get_lrms_options_schema);
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
            queues => {'shar' => {users => []}, 'loca' => {users => ['joe','pete'], maxjobs => '4 2'}},
            jobs => [qw(7 101 5865)]
           };

my $opt2 = {lrms => 'sge',
            sge_root => '/opt/n1ge6',
            sge_cell => 'cello',
            sge_bin_path => '/opt/n1ge6/bin/lx24-amd64',
            queues => {'shar' => {users => []}, 'all.q' => {users => ['joe','pete']}},
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
    LogUtils::level('VERBOSE');
    require Data::Dumper; import Data::Dumper qw(Dumper);
    $log->debug("Options: " . Dumper($options));
    my $results = LRMSInfo::collect($options->{lrms});
    $log->debug("Results: " . Dumper($results));
}

#test($opt3);

1;
