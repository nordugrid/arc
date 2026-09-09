package SGEmod;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(get_lrms_info get_lrms_options_schema);

use POSIX qw(floor ceil);
use LogUtils;
use Text::ParseWords qw(shellwords);
use XML::Simple qw(:strict);

use strict;

our $path;
our $options;
our $lrms_info = {};

# status of nodes and queues
our %node_stats = ();
# all running jobs, indexed by job-ID and task-ID
our %running_jobs = ();
# all waiting jobs, indexed by job-ID
our %waiting_jobs = ();

# Switch to choose between codepaths for SGE 6.x (the default) and SGE 5.x
our $compat_mode;

our $sge_type;
our $sge_version;
our $cpudistribution;
our @queue_names;

our $queuedjobs = 0;
our $queuedcpus = 0;

our $max_jobs;
our $max_user_running;
our %user_total_jobs;
our %user_waiting_jobs;
our %queue_free_slots;
our %queue_waiting_jobs;
our %queue_user_waiting_jobs;

our $log = LogUtils->getLogger(__PACKAGE__);

##########################################
# Public interface
##########################################

sub get_lrms_options_schema {
    return {
            'sge_root'         => '*',
            'sge_bin_path'     => '*',
            'sge_cell'         => '*',
            'sge_qmaster_port' => '*',
            'sge_execd_port'   => '*',
            'sge_pe'           => '*',
            'sge_exclusive_resource' => '*',
            'sge_memory_resource' => '*',
            'sge_wakeupperiod' => '*',
            'sge_query_retries' => '*',
            'sge_accounting_retries' => '*',
            'queues' => {
                '*' => {
                    'users'       => [ '' ],
                    'sge_queues' => '*',
                    'sge_jobopts' => '*',
                    'sge_pe' => '*',
                    'sge_exclusive_resource' => '*',
                    'sge_memory_resource' => '*'
                }
            },
            'jobs' => [ '' ]
        };
}

sub get_lrms_info($) {

    # Client configuration belongs to this collection, including on exceptions.
    # Do not leak a cell or port override into the next collection's defaults.
    local %ENV = %ENV;
    $options = shift;

    reset_state();

    lrms_init();
    type_and_version();
    run_qconf();
    run_qstat();

    cluster_info();

    my %qconf = %{$options->{queues}};
    for my $qname ( keys %qconf ) {
        queue_info($qname);
    }

    my $jids = $options->{jobs};
    jobs_info($jids);

    for my $qname ( keys %qconf ) {
        my $users = $qconf{$qname}{users};
        users_info($qname,$users);
    }

    nodes_info();

    return $lrms_info
}

##########################################
# Private subs
##########################################

sub reset_state {
    $lrms_info = {};

    %node_stats = ();
    %running_jobs = ();
    %waiting_jobs = ();
    %user_total_jobs = ();
    %user_waiting_jobs = ();
    %queue_free_slots = ();
    %queue_waiting_jobs = ();
    %queue_user_waiting_jobs = ();
    @queue_names = ();

    $compat_mode = 0;
    $sge_type = undef;
    $sge_version = undef;
    $cpudistribution = '';
    $queuedjobs = 0;
    $queuedcpus = 0;
    $max_jobs = undef;
    $max_user_running = undef;
}

#
# Generic function to process the output of an external program. The callback
# function will be invoked with a file descriptor receiving the standard output
# of the external program as its first argument.
#   

sub run_callback {
    my ($command, $callback, @extraargs) = @_;
    # Keep command labels, numbers and dates stable; see bug #3314. Localize
    # these settings here, not at module load or in the caller's environment.
    local $ENV{LC_ALL} = 'C';
    local $ENV{LANG} = 'C';
    my @argv = ref($command) eq 'ARRAY' ? @$command : shellwords($command);
    my $label = join ' ', map { shell_quote($_) } @argv;
    unless (@argv && -x $argv[0]) {
        $log->warning("Not an executable: $label");
        return 0;
    }
    $log->debug("SGE query_start command=$label");
    my $started = time;
    my $pid = open(my $pipe, '-|');
    unless (defined $pid) {
        $log->warning("Failed creating pipe from: $label: $!");
        return 0;
    }
    unless ($pid) {
        # Explicit exec PROGRAM LIST also avoids the single-argument pipe
        # open's shell interpretation when an executable has spaces in its name.
        exec { $argv[0] } @argv or do {
            $log->warning("Failed executing command: $label: $!");
            POSIX::_exit(127);
        };
    }
    my $output = do { local $/; <$pipe> };
    my $closed = close($pipe);
    my $status = $?;
    $log->debug("SGE query_result command=$label exit_status=" . ($status >> 8)
                . " signal=" . ($status & 127) . " bytes=" . length(defined($output) ? $output : '')
                . " elapsed_seconds=" . (time - $started));
    unless ($closed) {
        $log->warning("Failed running command: $label (exit_status=" . ($status >> 8)
                      . " signal=" . ($status & 127) . "); discarding output");
        return 0;
    }
    # A failed command may still emit plausible, but incomplete, records.
    # Invoke parsers only after successful exit, including the line callbacks.
    $output = '' unless defined $output;
    open(my $fh, '<', \$output) or die "Cannot read output of $label: $!\n";
    eval { &$callback($fh, @extraargs); 1 }
        or die "Invalid output from $label: $@";
    return 1;
}

#
# Generic function to process the output of an external program. The callback
# function will be invoked for each line of output from the external program.
#

sub loop_callback {
    my ($command, $callback) = @_;
    return run_callback($command, sub {
            my $fh = shift;
            my $line;
            while (defined ($line = <$fh>)) {
                chomp $line;
                &$callback($line);
            }
    });
    
}

sub command_output {
    my $command = shift;
    my $output = '';
    my $label = ref($command) eq 'ARRAY' ? join(' ', @$command) : $command;
    die "Failed running command: $label\n" unless run_callback($command, sub {
        my $fh = shift;
        local $/;
        $output = <$fh>;
    });
    return defined $output ? $output : '';
}

sub shell_quote {
    my $value = shift;
    $value =~ s/'/'"'"'/g;
    return "'$value'";
}

sub validate_qstat_defaults {
    my @files = @_;
    unless (@files) {
        push @files, "$ENV{SGE_ROOT}/$ENV{SGE_CELL}/common/sge_qstat";
        my @passwd = getpwuid($<);
        push @files, "$passwd[7]/.sge_qstat"
            if @passwd and defined $passwd[7];
    }

    for my $file (@files) {
        next unless defined $file and -r $file;
        open(my $fh, '<', $file)
            or die "Cannot read qstat defaults $file: $!\n";
        my $number = 0;
        while (my $line = <$fh>) {
            ++$number;
            $line =~ s/#.*//;
            next if $line =~ /^\s*$/;
            my @words = eval { shellwords($line) };
            die "Malformed qstat defaults in $file line $number\n"
                if $@ or !@words;
            tr/'"//d for @words;
            while (@words) {
                my $option = shift @words;
                die "Unsafe qstat default '$option' in $file line $number\n"
                    unless $option eq '-u' or $option eq '-s' or $option eq '-q';
                die "Missing value for qstat default '$option' in $file line $number\n"
                    unless @words;
                my $value = shift @words;
                die "Invalid value for qstat default '$option' in $file line $number\n"
                    if !length($value) or $value =~ /^-/;
            }
        }
        close($fh) or die "Cannot close qstat defaults $file: $!\n";
    }
}

#
# Determine SGE variant and version.
# Set compatibility mode if necessary.
#

sub type_and_version {
    my @output;
    run_callback(["$path/qstat", '-help'], sub {
        my $fh = shift;
        @output = <$fh>;
    });

    for my $line (@output) {
        if ($line =~ /^\s*((?:A|U|S)?GE)\s+(?:version\s+)?((?:pre)?[0-9][A-Za-z0-9_.+-]*)/i) {
            ($sge_type, $sge_version) = (uc($1), $2);
            last;
        }
        if ($line =~ /^\s*(Altair|Univa|Sun|Son of Sun|Son of)\s+Grid\s+Engine\D+([0-9][A-Za-z0-9_.+-]*)(?:\s+\(([0-9][A-Za-z0-9_.+-]*)\))?/i) {
            my %types = (
                altair       => 'AGE',
                univa        => 'UGE',
                sun          => 'SGE',
                'son of sun' => 'SGE',
                'son of'     => 'SGE',
            );
            ($sge_type, $sge_version) = ($types{lc($1)}, $3 || $2);
            last;
        }
    }

    if (not defined $sge_type or not defined $sge_version) {
        my $banner = @output ? $output[0] : '';
        chomp $banner;
        $log->warning("Cannot identify SGE version from output of '$path/qstat -help': $banner");
        ($sge_type, $sge_version) = ('SGE', 'unknown');
    } elsif ($sge_version =~ /^(?:5(?:\.|$)|pre6\.0)/) {
        $compat_mode = 1;
        $log->info("Using SGE 5.x compatibility mode");
    }
}

#
# Processes an array task definition (i.e.: '3,4,6-8,10-20:2')
# and returns the number of individual tasks
#

sub count_array_spec($) {
    my $count = 0;
    my $value = shift;
    return 0 unless defined $value and !ref($value) and length $value;
    for my $spec (split /,/, $value, -1) {
        # handles expressions like '6-10:2' and '6-10' and '6'
        return 0 unless $spec =~ '^(\d+)(?:-(\d+)(?::(\d+))?)?$';
        my ($lower,$upper,$step) = ($1,$2,$3);
        $upper = $lower unless defined $upper;
        $step = 1 unless defined $step;
        return 0 if $lower < 1 or $upper < $lower or $step < 1;
        $count += 1 + floor(($upper-$lower)/$step);
    }
    return $count;
}

sub as_array {
    my $value = shift;
    return [] unless ref $value;
    return $value if ref $value eq 'ARRAY';
    return [ $value ];
}

sub canonical_node_name {
    my $name = shift;
    return $name if exists $node_stats{$name};

    my ($shortname) = split /\./, $name;
    my @matches = grep {
        my ($short) = split /\./;
        lc($_) eq lc($name) ||
            (lc($short) eq lc($shortname) && ($name !~ /\./ || $_ !~ /\./))
    } keys %node_stats;
    return $matches[0] if @matches == 1;
    return $name;
}

sub is_retained_finished {
    my $state = shift;
    return defined($state) && !ref($state) && $state =~ /^\s*f\s*$/;
}

# Validate the complete snapshot before updating any counters. Well-formed XML
# alone is insufficient: an error document must not mean an empty cluster.
sub xml_document {
    my ($output, $root, @arrays) = @_;
    my $xml = eval {
        XMLin($output, KeyAttr => [], KeepRoot => 1,
              ForceArray => \@arrays, NoAttr => ($root eq 'job_info' ? 1 : 0));
    };
    die "Failed parsing $root XML: $@\n" unless ref($xml) eq 'HASH';
    die "Invalid $root XML root\n" unless ref($xml->{$root}) eq 'HASH';
    return $xml->{$root};
}

sub require_scalar {
    my ($record, $field, $context, $pattern) = @_;
    my $value = $record->{$field};
    die "Invalid $field in $context\n"
        unless defined $value && !ref($value) && $value =~ $pattern;
    return $value;
}

sub validate_job {
    my $job = shift;
    die "Invalid job_list in qstat XML\n" unless ref($job) eq 'HASH';
    my $id = require_scalar($job, 'JB_job_number', 'qstat XML', qr/^[1-9][0-9]*$/);
    require_scalar($job, 'state', "qstat job $id", qr/^[A-Za-z]+$/);
    return if is_retained_finished($job->{state});
    require_scalar($job, 'JB_owner', "qstat job $id", qr/^\S+$/);
    require_scalar($job, 'slots', "qstat job $id", qr/^[1-9][0-9]*$/);
    die "Invalid tasks in qstat job $id\n"
        if exists $job->{tasks} && !count_array_spec($job->{tasks});
}

sub qstat_document {
    my $output = shift;
    my $xml = xml_document($output, 'job_info', 'Queue-List', 'job_list');
    die "Unexpected section in qstat XML\n" if grep { $_ ne 'queue_info' && $_ ne 'job_info' } keys %$xml;
    for my $section ('queue_info', 'job_info') {
        die "Missing or invalid $section section in qstat XML\n"
            unless ref($xml->{$section}) eq 'HASH';
        die "Unexpected entry in qstat $section section\n"
            if grep { $_ ne 'job_list' && !($section eq 'queue_info' && $_ eq 'Queue-List') }
                keys %{$xml->{$section}};
    }
    my %instances;
    for my $queue (@{as_array($xml->{queue_info}{'Queue-List'})}) {
        die "Invalid Queue-List in qstat XML\n" unless ref($queue) eq 'HASH';
        my $name = require_scalar($queue, 'name', 'qstat queue', qr/^[^\s\@]+\@[^\s\@]+$/);
        die "Duplicate qstat queue $name\n" if $instances{$name}++;
        require_scalar($queue, $_, "qstat queue $name", qr/^[0-9]+$/)
            for ('slots_used', 'slots_total');
        require_scalar($queue, 'slots_resv', "qstat queue $name", qr/^[0-9]+$/)
            if exists $queue->{slots_resv};
        # XML::Simple represents <state/> as an empty hash.
        die "Invalid state in qstat queue $name\n" if exists $queue->{state}
            && !(ref($queue->{state}) eq 'HASH' && !keys %{$queue->{state}})
            && (ref($queue->{state}) || $queue->{state} !~ /^[A-Za-z]*$/);
        validate_job($_) for @{as_array($queue->{job_list})};
    }
    for my $job (@{as_array($xml->{queue_info}{job_list})}) {
        validate_job($job);
        require_scalar($job, 'queue_name', 'qstat job', qr/^[^\s\@]+\@[^\s\@]+$/);
    }
    validate_job($_) for @{as_array($xml->{job_info}{job_list})};
    return $xml;
}

sub record_running_job {
    my ($job, $queue, $node) = @_;
    return if is_retained_finished($job->{state});
    my $jobid = $job->{JB_job_number};
    return unless defined $jobid;

    my $taskid = defined $job->{tasks} ? $job->{tasks} : 0;
    my $ntasks = $taskid ? count_array_spec($taskid) : 1;
    $ntasks = 1 unless $ntasks;
    my $is_new = not exists $running_jobs{$jobid}{$taskid};
    my $task = $running_jobs{$jobid}{$taskid} ||= {};

    my $user = $job->{JB_owner};
    $user_total_jobs{$user} += $ntasks if $is_new and defined $user;

    $task->{user} = $user;
    $task->{state} = $job->{state} || '';
    $task->{date} = $job->{JAT_start_time};
    $task->{queue} ||= $queue;
    $task->{queues}{$queue} = 1;
    $task->{tasks} = $ntasks;

    my $slots = $job->{slots} || 1;
    $task->{nodes}{$node} += $slots;
    $task->{slots} += $slots;
    if ($task->{state} =~ /[sST]/) {
        $node_stats{$node}{queues}{$queue}{suspslots} += $slots;
    } else {
        $node_stats{$node}{runningslots} += $slots;
    }
}

sub record_waiting_job {
    my ($job, $rank_ref) = @_;
    return if is_retained_finished($job->{state});
    my $jobid = $job->{JB_job_number};
    return unless defined $jobid;

    my $taskdef = $job->{tasks};
    my $ntasks = $taskdef ? count_array_spec($taskdef) : 1;
    if (not $ntasks) {
        $log->error("Failed parsing task definition: $taskdef");
        $ntasks = 1;
    }

    my $user = $job->{JB_owner};
    my $waiting = $waiting_jobs{$jobid} ||= {};
    $waiting->{user} = $user;
    $waiting->{state} = $job->{state} || '';
    $waiting->{date} = $job->{JB_submission_time};
    $waiting->{slots} = $job->{slots} || 1;
    $waiting->{tasks} += $ntasks;
    $waiting->{rank} = $$rank_ref unless defined $waiting->{rank};
    $user_total_jobs{$user} += $ntasks if defined $user;
    $user_waiting_jobs{$user} += $ntasks if defined $user;
    $$rank_ref += $ntasks;
}

sub qstat_xml_parser_callback {
    my $output = shift;
    my $xml = qstat_document($output);

    for my $queue_info (@{as_array($xml->{queue_info})}) {
        for my $queue (@{as_array($queue_info->{'Queue-List'})}) {
            my ($qname, $nodename) = split /\@/, ($queue->{name} || ''), 2;
            unless ($qname and $nodename) {
                $log->error("Queue name of the form 'queue\@host' expected. Got: "
                            .($queue->{name} || ''));
                next;
            }
            $nodename = canonical_node_name($nodename);

            my $used = $queue->{slots_used} || 0;
            my $total = $queue->{slots_total} || 0;
            my $flags = ref($queue->{state}) ? '' : ($queue->{state} || '');
            $node_stats{$nodename}{load} = $queue->{load_avg}
                if defined $queue->{load_avg};
            $node_stats{$nodename}{arch} ||= $queue->{arch}
                if defined $queue->{arch};
            $node_stats{$nodename}{runningslots} ||= 0;
            $node_stats{$nodename}{queues}{$qname} = {
                usedslots => $used,
                reservedslots => $queue->{slots_resv} || 0,
                totalslots => $total,
                suspslots => 0,
                flags => $flags,
            };

            record_running_job($_, $qname, $nodename)
                for @{as_array($queue->{job_list})};
        }

        # Some Grid Engine derivatives put assigned jobs directly below
        # queue_info and provide queue_name in each job record.
        for my $job (@{as_array($queue_info->{job_list})}) {
            my ($qname, $nodename) = split /\@/, ($job->{queue_name} || ''), 2;
            next unless $qname and $nodename;
            $nodename = canonical_node_name($nodename);
            record_running_job($job, $qname, $nodename);
        }
    }

    my $rank = 1;
    for my $job_info (@{as_array($xml->{job_info})}) {
        record_waiting_job($_, \$rank) for @{as_array($job_info->{job_list})};
    }
}

sub queue_waiting_counts {
    my @qnames = @_;
    die "Invalid empty SGE queue mapping\n" unless @qnames;
    die "Invalid SGE queue mapping\n"
        if grep { not defined $_ or $_ !~ /^[A-Za-z0-9_.-]+$/ } @qnames;

    my $selection = join(',', @qnames);
    my $command = ["$path/qstat", '-xml', '-u', '*', '-s', 'a', '-f', '-q', $selection];
    my $output = command_output($command);
    my $xml = qstat_document($output);

    my $queued = 0;
    my %users;
    for my $job_info (@{as_array($xml->{job_info})}) {
        for my $job (@{as_array($job_info->{job_list})}) {
            next if is_retained_finished($job->{state});
            my $jobid = $job->{JB_job_number};
            die "Invalid job ID in queue-filtered qstat XML\n"
                unless defined $jobid and not ref $jobid and $jobid =~ /^\d+$/;
            my $taskdef = $job->{tasks};
            die "Invalid task list in queue-filtered qstat XML\n" if ref $taskdef;
            my $ntasks = $taskdef ? count_array_spec($taskdef) : 1;
            die "Invalid task list in queue-filtered qstat XML\n" unless $ntasks;
            $queued += $ntasks;
            my $user = $job->{JB_owner};
            $users{$user} += $ntasks if defined $user and not ref $user;
        }
    }
    return ($queued, \%users);
}

#
# this block contains the functions used to parse the output of qstat
#

{
    # shared variables

    my $line; # used to keep the most recently line read from qstat

    my $currentjobid = undef;
    my $currentqueue = undef;
    my $currentnode  = undef;

    #### Regular expression matching a queue line, like:
    # libero@compute-3-7.local       BPC   0/8       4.03     lx24-amd64    S
    # all.q@hyper.uio.no             BIP   0/0/1     0.00     lx24-x86
    # all.q@compute-14-19.local      BIPC  0/8       -NA-     lx24-amd64    Adu
    # corvus.q             BICP  0/16      99.99    solaris64 aAdu
    my $queue_regex = '^\s*(\S+)\s+\w+\s+(?:(\d+)/)?(\d+)/(\d+)\s+(\S+)\s+\S+(?:\s+(\w+))?\s*$';

    #### Regular expression matching complex lines from qstat -F
    #         hl:num_proc=1
    #         hl:mem_total=1009.523M
    #         qf:qname=shar
    #         qf:hostname=squark.uio.no
    my $complex_regex = '\s+(\w\w:\w+)=(.*)\s*';

    #### Regular expressions matching jobs (for SGE version 6.x), like:
    #  4518 2.71756 brkhrt_5ch whe042   r   08/20/2008 10:50:23  4
    #  1602 2.59942 runmain_4_ user1    r   08/13/2008 22:42:17  1 21
    #  1872 2.59343 test_GG1   otherusr Eqw 08/05/2008 17:36:45  1 4,6-8:1
    #  7539 7.86785 methane_i  user11   qw  06/26/2008 11:16:52  4
    #### Assume job name column is exactly 10 characters wide.
    my $jobid_prio_name_regex6 = '(\d+)\s+[.\d]+\s+\S.{9}';
    my $user_state_date_regex6 = '(\S+)\s+(\w+)\s+(\d\d/\d\d/\d{4} \d\d:\d\d:\d\d)';
    my $slots_tid_regex6 = '(\d+)(?:\s+(\d+))?'; # for running jobs
    my $slots_taskdef_regex6   = '(\d+)(?:\s+([:\-\d,]+))?';    # for queued jobs
    my $base_regex6 = '^\s*'.$jobid_prio_name_regex6.' '.$user_state_date_regex6;
    my $running_regex6 = $base_regex6.'\s+'.$slots_tid_regex6.'\s*$';
    my $waiting_regex6 = $base_regex6.'\s+'.$slots_taskdef_regex6.'\s*$';
    
    #### Regular expressions matching jobs (for SGE version 5.x), like:
    # 217  0 submit.tem lemlib r   07/21/2008 09:55:32 MASTER
    #      0 submit.tem lemlib r   07/21/2008 09:55:32 SLAVE
    #  27  0 exam.sh    c01p01 r   02/03/2006 16:40:49 MASTER 2
    #      0 exam.sh    c01p01 r   02/03/2006 16:40:49 SLAVE 2
    # 254  0 CPMD       baki   qw  08/14/2008 10:12:29
    # 207  0 STDIN      adi    qw  08/15/2008 17:23:37         2-10:2
    #### Assume job name column is exactly 10 characters wide.
    my $jobid_prio_name_regex5 = '(?:(\d+)\s+)?[.\d]+\s+\S.{9}';
    my $user_state_date_regex5 = '(\S+)\s+(\w+)\s+(\d\d/\d\d/\d{4} \d\d:\d\d:\d\d)';
    my $master_tid_regex5 = '(MASTER|SLAVE)(?:\s+(\d+))?'; # for running jobs
    my $taskdef_regex5   = '(?:\s+([:\-\d,]+))?';    # for queued jobs
    my $base_regex5 = '^\s*'.$jobid_prio_name_regex5.' '.$user_state_date_regex5;
    my $running_regex5 = $base_regex5.'\s+'.$master_tid_regex5.'\s*$';
    my $waiting_regex5 = $base_regex5.$taskdef_regex5.'\s*$';


    sub run_qstat {
        unless ($compat_mode) {
            my $command = ["$path/qstat", '-xml', '-u', '*', '-s', 'a', '-q', '*', '-f'];
            # Do not mutate provider state until qstat has exited successfully.
            qstat_xml_parser_callback(command_output($command));
            return;
        }

        my $command = ["$path/qstat", '-u', '*', '-s', 'a', '-q', '*', '-F'];
        die unless run_callback($command, \&qstat_parser_callback);
    }


    sub qstat_parser_callback {
        my $fh = shift;

        # validate header line
        $line = <$fh>;
        return unless defined $line; # if there was no output
        my @hdr = split ' ',$line;
        $log->error("qstat header line not recognized")
            unless ($hdr[0] eq 'queuename');
        
        $line = <$fh>;
        while (defined $line and $line =~ /^--------------/) {
            handle_queue($fh);
            handle_running_jobs($fh);
        }
        return unless defined $line; # if there are no waiting jobs

        $line = <$fh>; $log->error("Unexpected line from qstat") unless $line =~ /############/;
        $line = <$fh>; $log->error("Unexpected line from qstat") unless $line =~ /PENDING JOBS/;
        $line = <$fh>; $log->error("Unexpected line from qstat") unless $line =~ /############/;
        $line = <$fh>;
        handle_waiting_jobs($fh);
    
        # there should be no lines left
        $log->error("Cannot parse qstat output line: $line")
            if defined $line;
    }


    sub handle_queue {
        my $fh = shift;
        $line = <$fh>;

        if (defined $line and $line =~ /$queue_regex/) {

            my ($qname,$used,$total,$load,$flags) = ($1,$3,$4,$5,$6||'');
            $line = <$fh>;

            if (not $compat_mode) {
                ($currentqueue, $currentnode) = split '@',$qname,2;
                unless ($currentnode) {
                    $log->error("Queue name of the form 'queue\@host' expected. Got: $qname");
                }
            } else {
                $currentqueue = $qname;
                # parse complexes to extract hostname
                while (defined $line and $line =~ /$complex_regex/) {
                    $currentnode = $2 if $1 eq 'qf:hostname';
                    $line = <$fh>;
                }
                $log->warning("Could not extract hostname for queue $qname") unless $currentnode;
            }
            if ($currentnode) {
                # Was this node not listed with qhost -xml ?
                if (not exists $node_stats{$currentnode} or
                    not exists $node_stats{$currentnode}{totalcpus}) {
                    # Node name may have been truncated by qstat -f
                    if (length $qname >= 30) {
                        # Try to match it with a node already listed by qhost -xml
                        my @fullnames = grep { length($_) >= length($currentnode)
                                                   and $_ =~ m/^\Q$currentnode\E/
                                        } grep { exists $node_stats{$_}{totalcpus}
                                        } keys %node_stats;
                        $currentnode = $fullnames[0] if @fullnames == 1;
                    }
                    # Node name may have been truncated by qhost -xml
                    for (my $name = $currentnode; length $name >= 24; chop $name) {
                        $currentnode = $name if exists $node_stats{$name}
                                            and exists $node_stats{$name}{totalcpus}
                    }
                }
                if (not exists $node_stats{$currentnode} or
                    not exists $node_stats{$currentnode}{totalcpus}) {
                    $log->warning("Queue $currentqueue\@$currentnode cannot be matched"
                                  ." with a hostname listed by qhost -xml");
                }
                $node_stats{$currentnode}{load} = $load unless $load eq '-NA-';
                $node_stats{$currentnode}{runningslots} ||= 0; # will be counted later
                $node_stats{$currentnode}{queues}{$currentqueue}
                         = {usedslots=>$used, totalslots=>$total, suspslots=>'0', flags=>$flags};
            }
        }
    }

    # Running jobs in a queue instance

    sub handle_running_jobs {
        my $fh = shift;

        my $regex = $compat_mode ? $running_regex5 : $running_regex6;

        while (defined $line and $line =~ /$regex/) {

            if (not $compat_mode) { ### SGE v 6.x ###

                my ($jobid,$user,$slots,$taskid) = ($1,$2,$5,$6);
                $taskid = 0 unless $taskid; # 0 is an invalid task id
                # Index running jobs by job-ID and task-ID
                my $task = $running_jobs{$jobid}{$taskid} || {};
                $running_jobs{$jobid}{$taskid} = $task;
                $user_total_jobs{$user}++;
                $task->{user} = $user;
                $task->{state} = $3;
                $task->{date} = $4;
                $task->{queue} = $currentqueue;
                $task->{nodes}{$currentnode} = $slots;
                $task->{slots} += $slots;
                if ($task->{state} =~ /[sST]/) {
                    $node_stats{$currentnode}{queues}{$currentqueue}{suspslots} += $slots;
                } else {
                    $node_stats{$currentnode}{runningslots} += $slots;
                }

            } else { ### SGE 5.x, pre 6.0 ###

                my ($jobid,$user,$role,$taskid) = ($1,$2,$5,$6);
                $taskid = 0 unless $taskid; # 0 is an invalid task id
                if ($role eq 'MASTER' and not defined $jobid) {
                    $log->error("Cannot parse qstat output line: $line");
                } elsif (not defined $jobid) {
                    $jobid = $currentjobid;
                } else {
                    $currentjobid = $jobid;
                }
                # Index running jobs by job-ID and task-ID
                my $task = $running_jobs{$jobid}{$taskid} || {};
                $running_jobs{$jobid}{$taskid} = $task;
                if ($role eq 'MASTER') {        # each job has one MASTER
                    $user_total_jobs{$user}++;
                    $task->{user} = $user;
                    $task->{state} = $3;
                    $task->{date} = $4;
                    $task->{queue} = $currentqueue;
                    $task->{slots}++;
                    $task->{nodes}{$currentnode}++;
                    $task->{is_parallel} = 0;
                    if ($task->{state} =~ /[sST]/) {
                        $node_stats{$currentnode}{queues}{$currentqueue}{suspslots}++;
                    } else {
                        $node_stats{$currentnode}{runningslots}++;
                    }
                } elsif (not $task->{is_parallel}) {  # Fist SLAVE following the MASTER
                    $task->{is_parallel} = 1;         # Don't count this SLAVE
                } else {                              # Other SLAVEs, resume counting
                    $task->{slots}++;
                    $task->{nodes}{$currentnode}++;
                    if ($task->{state} =~ /[sST]/) {
                        $node_stats{$currentnode}{queues}{$currentqueue}{suspslots}++;
                    } else {
                        $node_stats{$currentnode}{runningslots}++;
                    }
                }
            }
            last unless defined ($line = <$fh>);
        }
    }


    sub handle_waiting_jobs {
        my $fh = shift;

        my $rank = 1;
        my $regex = $compat_mode ? $waiting_regex5 : $waiting_regex6;

        while (defined $line and $line =~ /$regex/) {

            if (not $compat_mode) { ### SGE v 6.x ###

                my ($jobid,$user,$taskdef) = ($1,$2,$6);
                my $ntasks = $taskdef ? count_array_spec($taskdef) : 1;
                unless ($ntasks) {
                    $log->error("Failed parsing task definition: $taskdef");
                }
                $waiting_jobs{$jobid}{user} = $user;
                $waiting_jobs{$jobid}{state} = $3;
                $waiting_jobs{$jobid}{date} = $4;
                $waiting_jobs{$jobid}{slots} = $5;
                $waiting_jobs{$jobid}{tasks} += $ntasks;
                $waiting_jobs{$jobid}{rank} = $rank;
                $user_total_jobs{$user} += $ntasks;
                $user_waiting_jobs{$user} += $ntasks;
                $rank += $ntasks;

            } else { ### SGE 5.x, pre 6.0 ###

                my ($jobid,$user,$taskdef) = ($1,$2,$5);
                my $ntasks = $taskdef ? count_array_spec($taskdef) : 1;
                unless ($ntasks) {
                    $log->error("Failed parsing task definition: $taskdef");
                }
                # The number of slots is not available from qstat output.
                $waiting_jobs{$jobid}{user} = $user;
                $waiting_jobs{$jobid}{state} = $3;
                $waiting_jobs{$jobid}{date} = $4;
                $waiting_jobs{$jobid}{tasks} += $ntasks;
                $waiting_jobs{$jobid}{rank} = $rank;
                # SGE 5.x does not list number of slots. Assuming 1 slot per job!
                $waiting_jobs{$jobid}{slots} = 1;
                $user_total_jobs{$user} += $ntasks;
                $user_waiting_jobs{$user} += $ntasks;
                $rank += $ntasks;
            }
            last unless defined ($line = <$fh>);
        }
    }

} # end of qstat block



sub parse_memory_kb {
    my $value = shift;
    return undef unless defined $value;
    return undef unless $value =~ /^\s*(\d+(?:\.\d+)?)\s*([kKmMgGtT]?)\s*$/;

    my ($number, $unit) = ($1, $2);
    my %multiplier = (
        '' => 1 / 1024,
        k => 1000 / 1024,
        K => 1,
        m => 1000 * 1000 / 1024,
        M => 1024,
        g => 1000 * 1000 * 1000 / 1024,
        G => 1024 * 1024,
        t => 1000 * 1000 * 1000 * 1000 / 1024,
        T => 1024 * 1024 * 1024,
    );
    return int($number * $multiplier{$unit});
}

sub run_qhost {
    # -F includes host complexes in resourcevalue elements.  Some Grid Engine
    # variants do not expose topology fields in the fixed hostvalue set.
    my $output = command_output(["$path/qhost", '-F', '-xml']);
    my $xml = xml_document($output, 'qhost', 'host', 'hostvalue', 'resourcevalue');
    die "Missing host list in qhost XML\n" unless exists $xml->{host};
    my %hosts;
    for my $host (@{as_array($xml->{host})}) {
        die "Invalid host in qhost XML\n" unless ref($host) eq 'HASH';
        my $hostname = require_scalar($host, 'name', 'qhost XML', qr/^\S+$/);
        die "Duplicate qhost host $hostname\n" if $hosts{$hostname}++;
        for my $entry (@{as_array($host->{hostvalue})}, @{as_array($host->{resourcevalue})}) {
            die "Invalid host value in qhost host $hostname\n" unless ref($entry) eq 'HASH';
            my $name = require_scalar($entry, 'name', "qhost host $hostname", qr/^\S+$/);
            next unless $name =~ /^(num_proc|m_socket|mem_total|swap_total|virtual_total|arch_string|arch)$/;
            my $value = require_scalar($entry, 'content', "qhost $hostname/$name", qr/^[\s\S]*$/);
            # A complex may also occur as a hostvalue. Check each occurrence;
            # '-' is the documented unavailable value, not zero capacity.
            next if $value eq '-';
            die "Invalid $name in qhost host $hostname\n"
                if ($name eq 'num_proc' || $name eq 'm_socket') && $value !~ /^[0-9]+$/;
            die "Invalid $name in qhost host $hostname\n"
                if $name =~ /^(mem_total|swap_total|virtual_total)$/ && !defined parse_memory_kb($value);
        }
    }

    for my $host (@{as_array($xml->{host})}) {
        my $hostname = $host->{name};
        next unless defined $hostname and $hostname ne 'global';

        my %values;
        for my $entry (@{as_array($host->{hostvalue})},
                       @{as_array($host->{resourcevalue})}) {
            next unless defined $entry->{name};
            $values{$entry->{name}} = $entry->{content};
        }

        my $node = $node_stats{$hostname} ||= {};
        $node->{arch} = $values{arch_string} || $values{arch}
            if defined($values{arch_string}) or defined($values{arch});
        $node->{totalcpus} = int($values{num_proc})
            if defined $values{num_proc} and $values{num_proc} =~ /^\d+$/;
        $node->{pcpus} = int($values{m_socket})
            if defined $values{m_socket} and $values{m_socket} =~ /^\d+$/;

        $node->{pmem} = parse_memory_kb($values{mem_total});
        my $virtual = parse_memory_kb($values{virtual_total});
        if (not defined $virtual) {
            my $swap = parse_memory_kb($values{swap_total});
            $virtual = $node->{pmem} + $swap
                if defined $node->{pmem} and defined $swap;
        }
        $node->{vmem} = $virtual if defined $virtual;
    }
}

sub run_qconf {

    # qconf -sep is deprecated; qhost XML supplies all hosts in one query.
    run_qhost();

    my %cpuhash;
    $cpuhash{$_->{totalcpus}}++
        for grep { defined $_->{totalcpus} } values %node_stats;
    $cpudistribution = join ' ', map {
        "${_}cpu:$cpuhash{$_}"
    } sort { $a <=> $b } grep { $_ > 0 } keys %cpuhash;

    # global limits
    my $global = command_output(["$path/qconf", '-sconf', 'global']);
    $max_jobs = qconf_integer($global, 'max_jobs', '-sconf global');

    # maxujobs is a scheduler limit on running jobs per user.  The similarly
    # named global max_u_jobs limits all active jobs and is not maxuserrun.
    my $scheduler = command_output(["$path/qconf", '-ssconf']);
    $max_user_running = qconf_integer($scheduler, 'maxujobs', '-ssconf') || undef;

    # list all queues
    my $queues = command_output(["$path/qconf", '-sql']);
    @queue_names = grep { length } split /\n/, $queues;
    die "Invalid queue name in qconf -sql\n"
        if grep { !/^[A-Za-z0-9_.-]+$/ } @queue_names;
    my %seen;
    die "Duplicate queue name in qconf -sql\n" if grep { $seen{$_}++ } @queue_names;
}

sub qconf_integer {
    my ($output, $field, $command) = @_;
    my @lines = grep { /^\Q$field\E\b/ } split /\n/, $output;
    die "Invalid, missing or duplicate $field in qconf $command\n"
        unless @lines == 1 && $lines[0] =~ /^\Q$field\E[ \t]+([0-9]+)[ \t]*$/;
    return $1;
}

sub parse_duration {
    my $value = shift;
    return undef unless defined $value and $value ne 'INFINITY';
    return int($value) if $value =~ /^\d+$/;

    my @parts = split /:/, $value, -1;
    return undef unless (@parts == 3 or @parts == 4)
                        and not grep { $_ !~ /^\d+$/ } @parts;
    my $seconds = pop @parts;
    my $minutes = pop @parts;
    my $hours = pop @parts;
    my $days = @parts ? pop @parts : 0;
    return $seconds + 60 * ($minutes + 60 * ($hours + 24 * $days));
}

sub req_limits ($) {
    my $line = shift;
    my ($reqcputime, $reqwalltime);
    my %seen;
    $line =~ s/^hard resource_list:\s*//;
    for my $entry (split /,/, $line) {
        next unless $entry =~ /^\s*([sh]_(?:cpu|rt))\b(.*)$/;
        my ($resource, $raw) = ($1, $2);
        die "Invalid $resource in qstat -j resource list: $entry\n"
            unless $raw =~ s/^=//;
        $raw =~ s/^\s+|\s+$//g;
        die "Duplicate $resource in qstat -j resource list\n" if $seen{$resource}++;
        my $limit = parse_duration($raw);
        die "Invalid $resource in qstat -j resource list: $raw\n"
            unless defined $limit || $raw eq 'INFINITY';
        next unless defined $limit;
        if ($resource =~ /_cpu$/) {
            $reqcputime = $limit if !defined $reqcputime || $reqcputime > $limit;
        } else {
            $reqwalltime = $limit if !defined $reqwalltime || $reqwalltime > $limit;
        }
    }
    return ($reqcputime, $reqwalltime);
}

sub queue_is_available {
    my $flags = shift || '';
    # Lower-case 'a' is a load alarm: the host is alive, although it is not
    # currently accepting work. All other queue state flags are unavailable.
    return $flags !~ /[cdosuACDEPS]/;
}

sub lrms_init() {
    $ENV{SGE_ROOT} = $options->{sge_root} || $ENV{SGE_ROOT};
    die "could not determine SGE_ROOT\n" unless $ENV{SGE_ROOT};

    $ENV{SGE_CELL} = $options->{sge_cell} || $ENV{SGE_CELL} || 'default';
    $ENV{SGE_QMASTER_PORT} = $options->{sge_qmaster_port} if $options->{sge_qmaster_port};
    $ENV{SGE_EXECD_PORT} = $options->{sge_execd_port} if $options->{sge_execd_port};

    for (split ':', $ENV{PATH}) {
        $ENV{SGE_BIN_PATH} = $_ and last if -x "$_/qsub";
    }
    $ENV{SGE_BIN_PATH} = $options->{sge_bin_path} || $ENV{SGE_BIN_PATH};

    validate_qstat_defaults();

    die "SGE executables not found\n"
        unless $ENV{SGE_BIN_PATH} and -x "$ENV{SGE_BIN_PATH}/qsub";

    $path = $ENV{SGE_BIN_PATH};
}


sub cluster_info () {

    my $lrms_cluster = {};

    # add this cluster to the info tree
    $lrms_info->{cluster} = $lrms_cluster;

    # Figure out SGE type and version

    $lrms_cluster->{lrms_glue_type} = "sungridengine";
    # ARC's public LRMS identifier is vendor-neutral.  $sge_type is retained
    # internally so vendor-specific banners can still be recognized.
    $lrms_cluster->{lrms_type} = "SGE";
    $lrms_cluster->{lrms_version} = $sge_version;

    $lrms_cluster->{cpudistribution} = $cpudistribution;
    $lrms_cluster->{totalcpus} = 0;
    $lrms_cluster->{totalcpus} += $_->{totalcpus} || 0 for values %node_stats;

    # Count used/free CPUs and queued jobs in the cluster
    
    # Note: SGE has the concept of "slots", which roughly corresponds to
    # concept of "cpus" in ARC (PBS) LRMS interface.

    my $usedcpus = 0;
    my $runningjobs = 0;
    for my $tasks (values %running_jobs) {
        for my $task (values %$tasks) {
            $runningjobs += $task->{tasks} || 1;
            # Skip suspended jobs
            $usedcpus += $task->{slots}
                unless $task->{state} =~ /[sST]/;
         }
    }

    $queuedjobs = 0;
    $queuedcpus = 0;
    for my $job (values %waiting_jobs) {
        $queuedjobs += $job->{tasks};
        $queuedcpus += $job->{tasks} * $job->{slots};
    }

    $lrms_cluster->{usedcpus} = $usedcpus;
    $lrms_cluster->{queuedcpus} = $queuedcpus;
    $lrms_cluster->{queuedjobs} = $queuedjobs;
    $lrms_cluster->{runningjobs} = $runningjobs;

    # List LRMS queues
    #$lrms_cluster->{queue} = [ @queue_names ];
}


sub queue_info ($) {
    my $qname = shift;

    my $lrms_queue = {};

    # add this queue to the info tree
    $lrms_info->{queues}{$qname} = $lrms_queue;

    # multiple (even overlapping) queues are supported.

    my @qnames = ($qname);

    # This code prepares for a scenario where grid jobs in a ComputingShare are
    # submitted by a-rex to thout requesting specific queue. Jobs in can end up
    # in several possible queues. This function should then try to agregate
    # values over all the queues in a list.
    # OBS: more work is needed to make this work
    if ($options->{queues}{$qname}{sge_queues}) {
        @qnames = split ' ', $options->{queues}{$qname}{sge_queues};
    }
    die "Invalid SGE queue mapping for $qname\n"
        if grep { not defined $_ or $_ !~ /^[A-Za-z0-9_.-]+$/ } @qnames;

    # NOTE:
    # In SGE the relation between CPUs and slots is quite elastic. Slots is
    # just one of the complexes that the SGE scheduler takes into account. It
    # is quite possible (depending on configuration permits) to have more slots
    # used by jobs than total CPUs on a node.  On the other side, even if there
    # are unused slots in a queue, jobs might be prevented to start because of
    # some other constraints.

    # queuestatus - will be negative only if all queue instances have a status
    #               flag set other than 'a'.
    # queueused   - sum of slots used by jobs, not including suspended jobs.
    # queuetotal  - sum of slots limited by the number of cpus on each node.
    # queuefree   - attempt to calculate free slots.

    my $queuestatus = -1;
    my $queuetotal = 0;
    my $queuefree = 0;
    my $queueused = 0;
    my %queue_nodes;
    for my $nodename (keys %node_stats) {
        my $node = $node_stats{$nodename};
        my $queues = $node->{queues};
        next unless defined $queues;
        my $nodetotal = 0; # number of slots on this node in the selected queues
        my $nodefree = 0;
        my $nodeused = 0;
        for my $name (keys %$queues) {
            next unless grep {$name eq $_} @qnames;
            my $q = $queues->{$name};
            $queue_nodes{$nodename} = 1;
            $nodetotal += $q->{totalslots};
            $nodeused += $q->{usedslots} - $q->{suspslots};
            # Any flag on the queue implies that the queue is not taking more jobs.
            my $free = $q->{totalslots} - $q->{usedslots} - ($q->{reservedslots} || 0);
            $nodefree += $free if !$q->{flags} && $free > 0;
            # The queue is healty if there is an instance in any other states
            # than normal or (a)larm. See man qstat for the meaning of the flags.
            $queuestatus = 1 if queue_is_available($q->{flags});
        }
        # Cheating a bit here. SGE's scheduler would consider load averages
        # among other things to decide if there are free slots.
        if (defined $node->{totalcpus}) {
            my $maxslots = $node->{totalcpus};
            if ($nodetotal > $maxslots) {
                $log->debug("Capping nodetotal ($nodename): $nodetotal > ".$maxslots);
                $nodetotal = $maxslots;
            }
            if ($nodefree > $maxslots - $node->{runningslots}) {
                $log->debug("Capping nodefree ($nodename): $nodefree > ".$maxslots." - ".$node->{runningslots});
                $nodefree = $maxslots - $node->{runningslots};
                $nodefree = 0 if $nodefree < 0;
            }
        } else {
            $log->info("Node not listed by qhost -xml: $nodename");
        }
        $queuetotal += $nodetotal;
        $queuefree += $nodefree;
        $queueused += $nodeused;
    }

    $lrms_queue->{totalcpus} = $queuetotal;
    $lrms_queue->{running} = $queueused;
    $lrms_queue->{status} = $queuestatus;
    $lrms_queue->{nodes} = [ sort keys %queue_nodes ];
    $lrms_queue->{minwalltime} = 0;
    $lrms_queue->{mincputime} = 0;

    # settings in the config file override
    my $qopts = $options->{queues}{$qname};
    $lrms_queue->{totalcpus} = $qopts->{totalcpus} if $qopts->{totalcpus};
    $queuefree = $lrms_queue->{totalcpus}
        if $queuefree > $lrms_queue->{totalcpus};
    $queuefree = 0 if $queuefree < 0 or $queuestatus < 0;
    $queue_free_slots{$qname} = $queuefree;

    # reserve negative numbers for error states
    $log->warning("Negative status for queue $qname: $lrms_queue->{status}")
        if $lrms_queue->{status} < 0;

    # Grid Engine can override each limit per host or host group.  Advertise
    # the lowest value so jobs accepted through this share fit every queue
    # instance represented by it.

    my $command = ["$path/qconf", '-sq', join(',', @qnames)];
    my $queue_configuration = command_output($command);
    $queue_configuration =~ s/\\[ \t]*\r?\n[ \t]*/ /g;
    my (%limits, %seen);
    my $current;
    for my $l (split /\n/, $queue_configuration) {
        if ($l =~ /^qname\b/) {
            die "Invalid queue name in qconf -sq: $l\n"
                unless $l =~ /^qname[ \t]+([A-Za-z0-9_.-]+)[ \t]*$/;
            $current = $1;
            die "Unexpected or duplicate queue in qconf -sq: $current\n"
                if !grep({ $_ eq $current } @qnames) || $seen{$current}++;
            next;
        }
        next unless $l =~ /^([sh]_(?:rt|cpu))\s+(.+)/;
        my ($resource, $values) = ($1, $2);
        die "Missing qname before time limit in qconf -sq\n" unless defined $current;
        die "Duplicate $resource for $current in qconf -sq\n" if $limits{$current}{$resource}++;
        my $field = $resource =~ /_rt$/ ? 'maxwalltime' : 'maxcputime';
        my @values = split /,/, $values, -1;
        for my $index (0 .. $#values) {
            my $raw = $values[$index];
            $raw =~ s/^\s+|\s+$//g;
            if ($index) {
                die "Invalid host override in qconf -sq: $l\n"
                    unless $raw =~ /^\[[^\s=\[\]]+=([^\[\]]+)\]$/;
                $raw = $1;
            }
            my $timelimit = parse_duration($raw);
            if (not defined $timelimit) {
                die "Invalid time limit in qconf -sq: $l\n" unless $raw eq 'INFINITY';
                next;
            }
            if (not defined $lrms_queue->{$field}
                    or $lrms_queue->{$field} > $timelimit) {
                $lrms_queue->{$field} = $timelimit;
            }
        }
    }
    for my $name (@qnames) {
        die "Missing time limits for $name in qconf -sq\n"
            if grep { !$limits{$name}{$_} } qw(s_rt h_rt s_cpu h_cpu);
    }

    # Pending jobs live in a cluster-wide pool.  qstat's queue filter selects
    # jobs which can run in this share's native queues, preventing the global
    # pending count from being copied into every advertised share.
    if ($compat_mode) {
        $queue_waiting_jobs{$qname} = $queuedjobs;
        $queue_user_waiting_jobs{$qname} = { %user_waiting_jobs };
    } else {
        my ($count, $users) = queue_waiting_counts(@qnames);
        $queue_waiting_jobs{$qname} = $count;
        $queue_user_waiting_jobs{$qname} = $users;
    }
    $lrms_queue->{queued} = $queue_waiting_jobs{$qname};

    # nordugrid-queue-maxrunning
    # nordugrid-queue-maxqueuable
    # nordugrid-queue-maxuserrun

    # The total max running jobs is the number of slots for this queue
    $lrms_queue->{maxrunning} = $lrms_queue->{totalcpus};

    # SGE has a global limit on total number of jobs, but not per-queue limit.
    # This global limit gives an upper bound for maxqueuable and maxrunning
    if ($max_jobs) {
        $lrms_queue->{maxqueuable} = $max_jobs;
        $lrms_queue->{maxrunning} = $max_jobs if $lrms_queue->{maxrunning} > $max_jobs;
    }

    if ($max_user_running) {
        $lrms_queue->{maxuserrun} = $max_user_running;
    }
}


# Parse optional details into a separate result. Never attach a partly parsed
# response to the published jobs tree. Command failures may be races with job
# completion; malformed successful output is an error, not missing usage.
sub job_details {
    my ($jids, $waiting) = @_;
    my %requested = map { $_ => 1 } @$jids;
    my (%details, %seen);
    my $jid;
    my $ok = loop_callback(["$path/qstat", '-u', '*', '-s', 'a', '-q', '*', '-j', join(',', @$jids)], sub {
        my $l = shift;
        if ($l =~ /^job_number:/) {
            die "Invalid or unexpected job_number in qstat -j\n"
                unless $l =~ /^job_number:[ \t]+([1-9][0-9]*)[ \t]*$/ && $requested{$1};
            $jid = $1;
            die "Duplicate job_number $jid in qstat -j\n" if $seen{$jid}++;
        }
        elsif ($l =~ /^usage\b/ && !$waiting) {
            die "Missing job_number before usage in qstat -j\n" unless defined $jid;
            die "Invalid usage line for job $jid in qstat -j\n"
                unless $l =~ s/^usage\s*(?:[0-9]+\s*)?:\s*//;
            my %fields;
            # Match complete comma-separated values, not a numeric prefix of
            # corrupt data or a suffix of a different resource's name.
            for my $entry (split /,/, $l) {
                next unless $entry =~ /^\s*(cpu|maxvmem)\b(.*)$/;
                my ($field, $raw) = ($1, $2);
                die "Invalid $field for job $jid in qstat -j usage: $entry\n"
                    unless $raw =~ s/^=//;
                $raw =~ s/^\s+|\s+$//g;
                die "Duplicate $field for job $jid in qstat -j usage\n" if $fields{$field}++;
                my $value = $field eq 'cpu' ? parse_duration($raw) : parse_memory_kb($raw);
                die "Invalid $field for job $jid in qstat -j usage: $raw\n" unless defined $value;
                $details{$jid}{$field eq 'cpu' ? 'cputime' : 'mem'} = $value;
            }
        }
        elsif ($l =~ /^hard resource_list\b/) {
            die "Missing job_number before resource list in qstat -j\n" unless defined $jid;
            my ($cpu, $wall) = req_limits($l);
            $details{$jid}{reqcputime} = $cpu if defined $cpu;
            $details{$jid}{reqwalltime} = $wall if defined $wall;
        }
        elsif ($waiting && $l =~ /^\s*(cannot run because.*)/) {
            die "Missing job_number before reason in qstat -j\n" unless defined $jid;
            push @{$details{$jid}{comment}}, "LRMS: $1";
        }
        elsif ($waiting && ($l =~ /^\s*error reason\s*\d*:\s*(.*)/ || $l =~ /(job is in error state)/)) {
            die "Missing job_number before error reason in qstat -j\n" unless defined $jid;
            push @{$details{$jid}{comment}}, "SGE job state was Eqw. LRMS error message was: $1";
        }
    });
    unless ($ok) {
        $log->warning('Failed listing named jobs: ' . join(',', @$jids));
        return {};
    }
    die "Missing job_number in successful qstat -j response: " . join(',', grep { !$seen{$_} } @$jids) . "\n"
        if grep { !$seen{$_} } @$jids;
    return \%details;
}

sub jobs_info ($) {

    # LRMS job IDs from Grid Manager
    my $jids = shift;

    my $lrms_jobs = {};

    my ($job, @running, @queueing);
    my %seen;

    # loop through all requested jobs
    for my $jid (@$jids) {
        die "Invalid requested SGE job ID\n" unless defined $jid && !ref($jid) && $jid =~ /^[1-9][0-9]*$/;
        next if $seen{$jid}++;

        if (defined $running_jobs{$jid} and not defined $running_jobs{$jid}{0}) {
            $log->warning("SGE job $jid is an array job. Unable to handle it");

        } elsif (exists $running_jobs{$jid} && defined ($job = $running_jobs{$jid}{0})) {
            push @running, $jid;

            # OBS: it's assumed that jobs in this loop are not part of array
            # jobs, which is true for grid jobs (non-array jobs have taskid 0)

            if ($job->{state} =~ /[rt]/) {
                # running or transfering
                $lrms_jobs->{$jid}{status} = 'R';
            } elsif ($job->{state} =~ /[sST]/) {
                # suspended
                $lrms_jobs->{$jid}{status} = 'S';
            } else {
                # Shouldn't happen
                $lrms_jobs->{$jid}{status} = 'O';
                push @{$lrms_jobs->{$jid}{comment}}, "Unexpected SGE state: $job->{state}";
                $log->warning("SGE job $jid is in an unexpected state: $job->{state}");
            }
            $lrms_jobs->{$jid}{nodes} = [ sort keys %{$job->{nodes} || {}} ];
            $lrms_jobs->{$jid}{cpus} = $job->{slots};

        } elsif (defined ($job = $waiting_jobs{$jid})) {
            push @queueing, $jid;

            $lrms_jobs->{$jid}{rank} = $job->{rank};

            # Old SGE versions do not list the number of slots for queing jobs
            $lrms_jobs->{$jid}{cpus} = $job->{slots} if not $compat_mode;
    
            if ($job->{state} =~ /E/) {
                # DRMAA: SYSTEM_ON_HOLD ?
                # TODO: query qacct for error msg
                $lrms_jobs->{$jid}{status} = 'O';
            } elsif ($job->{state} =~ /h/) {
                # Job is on hold
                $lrms_jobs->{$jid}{status} = 'H';
            } elsif ($job->{state} =~ /w/ and $job->{state} =~ /q/) {
                # Normally queued
                $lrms_jobs->{$jid}{status} = 'Q';
            } else {
                # Shouldn't happen
                $lrms_jobs->{$jid}{status} = 'O';
                push @{$lrms_jobs->{$jid}{comment}}, "Unexpected SGE state: $job->{state}";
                $log->warning("SGE job $jid is in an unexpected state: $job->{state}");
            }
        } else {

            # The job has finished.
            # Querying accounting system is slow, so we skip it for now.
            # That will be done by scan-sge-jobs.
        
            $log->debug("SGE job $jid has finished");
            $lrms_jobs->{$jid}{status} = 'EXECUTED';
            $lrms_jobs->{$jid}{comment} = [];
        }
    }

    for my $group ([\@running, 0], [\@queueing, 1]) {
        next unless @{$group->[0]};
        my $details = job_details(@$group);
        for my $id (keys %$details) {
            # Preserve status comments if optional details add their own.
            my $comments = delete $details->{$id}{comment};
            push @{$lrms_jobs->{$id}{comment}}, @$comments if $comments;
            @{$lrms_jobs->{$id}}{keys %{$details->{$id}}} = values %{$details->{$id}};
        }
    }
    $lrms_info->{jobs} = $lrms_jobs;
}


sub users_info($$) {
    my ($qname, $accts) = @_;

    my $lrms_users = {};

    # add users to the info tree
    my $lrms_queue = $lrms_info->{queues}{$qname};
    $lrms_queue->{users} = $lrms_users;

    # freecpus
    # queue length
    #
    # This is hard to implement correctly for a complex system such as SGE.
    # Using simple estimate.

    foreach my $u ( @{$accts} ) {
        my $freecpus = $queue_free_slots{$qname} || 0;

        $lrms_users->{$u}{queuelength} =
            $queue_user_waiting_jobs{$qname}{$u} || 0;
        $freecpus = 0 if $freecpus < 0;
        if (defined $lrms_queue->{maxwalltime}) {
            # Queue limits use seconds; the LRMSInfo freecpus contract uses
            # whole minutes (zero means unlimited), including for GLUE2.
            my $minutes = int($lrms_queue->{maxwalltime} / 60);
            $freecpus = 0 unless $minutes;
            $lrms_users->{$u}{freecpus} = { $freecpus => $minutes };
        } else {
            $lrms_users->{$u}{freecpus} = { $freecpus => 0 }; # unlimited
        }
    }
}

sub nodes_info {
    my $lrms_nodes = {};
    $lrms_info->{nodes} = $lrms_nodes;

    my %configured_queues;
    for my $arc_queue (keys %{$options->{queues}}) {
        my $names = $options->{queues}{$arc_queue}{sge_queues};
        $configured_queues{$_} = 1
            for ($names ? split(' ', $names) : ($arc_queue));
    }

    for my $host (keys %node_stats) {
        my $node = $node_stats{$host};
        my $queues = $node->{queues} || {};
        my @relevant = grep { $configured_queues{$_} } keys %$queues;
        next unless @relevant;

        my $isavailable = 0;
        my $isfree = 0;
        for my $qname (@relevant) {
            my $queue = $queues->{$qname};
            $isavailable = 1 if queue_is_available($queue->{flags});
            $isfree = 1 if not $queue->{flags}
                           and $queue->{usedslots} + ($queue->{reservedslots} || 0) < $queue->{totalslots};
        }

        my $lrms_node = $lrms_nodes->{$host} = {
            isavailable => $isavailable,
            isfree => $isfree,
        };
        $lrms_node->{lcpus} = $node->{totalcpus}
            if defined $node->{totalcpus};
        $lrms_node->{slots} = $node->{totalcpus}
            if defined $node->{totalcpus};
        $lrms_node->{pmem} = $node->{pmem} if defined $node->{pmem};
        $lrms_node->{vmem} = $node->{vmem} if defined $node->{vmem};
        $lrms_node->{pcpus} = $node->{pcpus} if defined $node->{pcpus};

        if (defined $node->{arch} and
            $node->{arch} =~ /^(lx\d*|linux|sol|darwin)-(.+)$/i) {
            my ($system, $machine) = (lc($1), lc($2));
            $lrms_node->{sysname} = $system =~ /^(?:lx|linux)/ ? 'Linux'
                                   : $system eq 'sol' ? 'SunOS'
                                   : 'Darwin';
            my %machines = (amd64 => 'x86_64', x86 => 'i686');
            $lrms_node->{machine} = $machines{$machine} || $machine;
        }
    }
}

sub test {
    LogUtils::level("VERBOSE");
    require Data::Dumper; import Data::Dumper qw(Dumper);

    $path = shift;
    (%running_jobs,%waiting_jobs) = ();
    get_lrms_info("");
    print Dumper(\%node_stats,\%running_jobs,\%waiting_jobs);
}


#test('./test/6.0');
#test('./test/5.3');
#test($ARGV[0]);

1;
