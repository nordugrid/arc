#!/usr/bin/perl
use strict;
use warnings;
no warnings 'once';
use Test::More;
use File::Temp qw(tempdir);
use SGEmod;

# Each corruption is one named case. No scheduler, credentials, or root needed.
# The process-runner cases execute real child processes; parser cases substitute
# only command_output so failures can be reproduced without a live qmaster.
my $root = tempdir(CLEANUP => 1);
symlink($^X, "$root/client with spaces") or die $!;
my @lines;
ok(SGEmod::loop_callback(["$root/client with spaces", '-e', 'print "first\n\nlast"'],
                        sub { push @lines, shift }), 'executable path with spaces works');
is_deeply(\@lines, ['first', '', 'last'], 'blank and unterminated final lines are delivered');
@lines = ();
ok(!SGEmod::loop_callback([$^X, '-e', 'print "plausible\n"; exit 7'],
                         sub { push @lines, shift }), 'nonzero child status is failure');
is_deeply(\@lines, [], 'failed command cannot mutate callback state');
ok(!SGEmod::run_callback([$^X, '-e', 'kill 15, $$'], sub { fail('signal invoked callback') }),
   'signal death is failure, not exit status zero');
ok(!SGEmod::run_callback(["$root/missing"], sub { fail('missing executable invoked callback') }),
   'missing executable is failure');
my $literal = 'a b;$(echo bad)*';
is(SGEmod::command_output([$^X, '-e', 'print $ARGV[0]', $literal]), $literal,
   'arguments are passed literally, without a shell');
is(SGEmod::command_output([$^X, '-e', 'exit 0']), '', 'successful empty output is distinct from failure');
ok(!eval { SGEmod::run_callback([$^X, '-e', 'print "bad"'], sub { die "bad record\n" }); 1 },
   'parser exceptions propagate');
like($@, qr/Invalid output from .*bad record/s, 'parser error identifies the command');

for my $bad ('', '0', '1-3:0', '3-1:1', '1,', ',1', '1-2:', 'junk') {
    is(SGEmod::count_array_spec($bad), 0, "reject array specification [$bad]");
}
is(SGEmod::count_array_spec('1,3-7:2'), 4, 'valid sparse array count');

my $job = '<job_list><JB_job_number>12</JB_job_number><JB_owner>alice</JB_owner>'
        . '<state>qw</state><slots>2</slots></job_list>';
my $queue = '<Queue-List><name>all.q@node1</name><slots_used>0</slots_used>'
          . '<slots_resv>2</slots_resv><slots_total>2</slots_total><state/></Queue-List>';
my $snapshot = "<job_info><queue_info>$queue</queue_info><job_info>$job</job_info></job_info>";

sub changed {
    my ($from, $to) = @_;
    my $xml = $snapshot;
    $xml =~ s/\Q$from\E/$to/ or die "Bad test replacement: $from";
    return $xml;
}
my @bad_snapshots = (
    ['wrong root', '<error>qmaster unavailable</error>'],
    ['empty root', '<job_info/>'],
    ['error in pending section', '<job_info><queue_info/><job_info><error>denied</error></job_info></job_info>'],
    ['missing pending section', "<job_info><queue_info>$queue</queue_info></job_info>"],
    ['missing queue section', "<job_info><job_info>$job</job_info></job_info>"],
    ['text job list', changed($job, '<job_list>bad</job_list>')],
    ['missing job ID', changed('<JB_job_number>12</JB_job_number>', '')],
    ['duplicate job ID field', changed('<JB_job_number>12</JB_job_number>', '<JB_job_number>12</JB_job_number><JB_job_number>13</JB_job_number>')],
    ['missing owner', changed('<JB_owner>alice</JB_owner>', '')],
    ['missing job state', changed('<state>qw</state>', '')],
    ['nested job state', changed('<state>qw</state>', '<state><value>qw</value></state>')],
    ['missing slots', changed('<slots>2</slots>', '')],
    ['nonnumeric slots', changed('<slots>2</slots>', '<slots>NaN</slots>')],
    ['negative queue slots', changed('<slots_total>2</slots_total>', '<slots_total>-2</slots_total>')],
    ['invalid reserved slots', changed('<slots_resv>2</slots_resv>', '<slots_resv>oops</slots_resv>')],
    ['bad queue name', changed('all.q@node1', 'all.q')],
    ['duplicate queue', changed($queue, $queue . $queue)],
    ['zero array step', changed('</job_list>', '<tasks>1-3:0</tasks></job_list>')],
    ['empty task list', changed('</job_list>', '<tasks/></job_list>')],
);
for my $case (@bad_snapshots) {
    SGEmod::reset_state();
    $SGEmod::node_stats{sentinel} = { totalcpus => 1 };
    ok(!eval { SGEmod::qstat_xml_parser_callback($case->[1]); 1 }, "$case->[0]: rejected");
    is_deeply([sort keys %SGEmod::node_stats], ['sentinel'], "$case->[0]: no partial node mutation");
    is_deeply(\%SGEmod::waiting_jobs, {}, "$case->[0]: no partial job mutation");
    {
        no warnings 'redefine';
        local *SGEmod::command_output = sub { $case->[1] };
        ok(!eval { SGEmod::queue_waiting_counts('all.q'); 1 }, "$case->[0]: filtered query also rejects");
    }
}
SGEmod::reset_state();
ok(eval { SGEmod::qstat_xml_parser_callback('<job_info><queue_info/><job_info/></job_info>'); 1 },
   'genuine empty snapshot succeeds');
SGEmod::cluster_info();
is($SGEmod::lrms_info->{cluster}{totalcpus}, 0, 'empty cluster has defined zero CPUs');
ok(eval { SGEmod::qstat_xml_parser_callback($snapshot); 1 }, 'valid collection recovers after failures');
is($SGEmod::waiting_jobs{12}{slots}, 2, 'recovered job slots');

my $host = '<qhost><host name="node1"><hostvalue name="num_proc">4</hostvalue>'
         . '<hostvalue name="mem_total">8G</hostvalue></host></qhost>';
for my $case (
    ['wrong root', '<job_info/>'],
    ['missing host list', '<qhost><error>denied</error></qhost>'],
    ['nonnumeric CPUs', $host =~ s/>4</>junk</r],
    ['bad memory units', $host =~ s/>8G</>8oops</r],
    ['nested memory value', $host =~ s/>8G</><value>8G<\/value></r],
) {
    no warnings 'redefine';
    local *SGEmod::command_output = sub { $case->[1] };
    local %SGEmod::node_stats = (sentinel => { totalcpus => 1 });
    ok(!eval { SGEmod::run_qhost(); 1 }, "qhost $case->[0]: rejected");
    is_deeply([sort keys %SGEmod::node_stats], ['sentinel'], "qhost $case->[0]: no partial mutation");
}
{
    no warnings 'redefine';
    local *SGEmod::command_output = sub { $host =~ s/>4</>-</r };
    local %SGEmod::node_stats;
    ok(eval { SGEmod::run_qhost(); 1 }, 'qhost unavailable value is accepted');
    ok(!defined $SGEmod::node_stats{node1}{totalcpus}, 'unavailable CPU count stays unknown');
}

my %configuration = ('-sconf' => "max_jobs 50\n", '-ssconf' => "maxujobs 3\n", '-sql' => "all.q\n");
for my $option (sort keys %configuration) {
    no warnings 'redefine';
    local *SGEmod::run_qhost = sub {};
    local *SGEmod::command_output = sub {
        my $args = shift;
        die "Failed running command: qconf $option\n" if $args->[1] eq $option;
        return $configuration{$args->[1]};
    };
    ok(!eval { SGEmod::run_qconf(); 1 }, "qconf $option failure aborts collection");
    like($@, qr/\Q$option\E/, 'failing command identified');
}
for my $case (['-sconf', 'max_jobs 50junk'], ['-ssconf', ''], ['-sql', 'error: denied'],
              ['-sconf', "max_jobs 50\nmax_jobs 0\n"],
              ['-ssconf', "maxujobs 3\nmaxujobs invalid\n"],
              ['-sql', "all.q\nall.q\n"]) {
    no warnings 'redefine';
    local *SGEmod::run_qhost = sub {};
    local *SGEmod::command_output = sub {
        my $args = shift;
        return $args->[1] eq $case->[0] ? $case->[1] : $configuration{$args->[1]};
    };
    ok(!eval { SGEmod::run_qconf(); 1 }, "malformed qconf $case->[0] is rejected");
}

my $limits = "qname all.q\ns_rt INFINITY\nh_rt 00:45:00\ns_cpu INFINITY\nh_cpu INFINITY\n";
{
    no warnings 'redefine';
    local $SGEmod::options = { queues => { 'all.q' => {} } };
    local *SGEmod::command_output = sub { $limits };
    local *SGEmod::queue_waiting_counts = sub { (0, {}) };
    SGEmod::reset_state();
    SGEmod::qstat_xml_parser_callback($snapshot);
    SGEmod::queue_info('all.q');
    SGEmod::users_info('all.q', ['alice']);
    SGEmod::nodes_info();
    is($SGEmod::queue_free_slots{'all.q'}, 0, 'reserved slots are not advertised as free');
    is($SGEmod::lrms_info->{nodes}{node1}{isfree}, 0, 'fully reserved node is not free');
    $SGEmod::queue_free_slots{'all.q'} = 2;
    SGEmod::users_info('all.q', ['alice']);
    is_deeply($SGEmod::lrms_info->{queues}{'all.q'}{users}{alice}{freecpus}, {2 => 45},
              '45-minute wall limit stays 45 minutes in the freecpus contract');
    $SGEmod::lrms_info->{queues}{'all.q'}{maxwalltime} = 30;
    SGEmod::users_info('all.q', ['alice']);
    is_deeply($SGEmod::lrms_info->{queues}{'all.q'}{users}{alice}{freecpus}, {0 => 0},
              'subminute finite limit does not become unlimited availability');
    for my $bad ('', $limits =~ s/h_rt 00:45:00/h_rt garbage/r,
                 $limits =~ s/h_rt 00:45:00/h_rt 00:45:00,[node1=01:00:00/r,
                 $limits =~ s/qname all.q/qname wrong.q/r,
                 $limits =~ s/h_cpu INFINITY\n//r) {
        local *SGEmod::command_output = sub { $bad };
        ok(!eval { SGEmod::queue_info('all.q'); 1 }, 'missing/malformed queue limits are not unlimited');
    }
}

for my $case (
    ['usage without identity', 'usage 1: cpu=00:01:00, maxvmem=1G'],
    ['unrequested identity', "job_number: 999\nusage 1: cpu=00:01:00"],
    ['malformed identity', "job_number: 12bad\nusage 1: cpu=00:01:00"],
) {
    no warnings 'redefine';
    SGEmod::reset_state();
    $SGEmod::running_jobs{12}{0} = { state => 'r', slots => 1, nodes => {node1 => 1} };
    local *SGEmod::loop_callback = sub { my ($cmd, $cb) = @_; $cb->($_) for split /\n/, $case->[1]; 1 };
    ok(!eval { SGEmod::jobs_info([12]); 1 }, "qstat -j $case->[0] rejected");
    ok(!exists $SGEmod::lrms_info->{jobs}{999} && !exists $SGEmod::lrms_info->{jobs}{''},
       'bad details do not create phantom jobs');
}
done_testing();
