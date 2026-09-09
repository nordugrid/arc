#!/usr/bin/perl
use strict;
use warnings;
no warnings 'once';
use Test::More;
use File::Temp qw(tempdir);
use SGEmod;

{
    my $dir = tempdir(CLEANUP => 1);
    symlink('/bin/true', "$dir/client without arguments") or die $!;
    ok(SGEmod::run_callback(["$dir/client without arguments"], sub {}),
       'single-element argv executes a path containing spaces without a shell');
    local $ENV{LC_ALL} = 'POSIX';
    local $ENV{LANG} = 'POSIX';
    is(SGEmod::command_output([$^X, '-e', 'print "$ENV{LC_ALL}:$ENV{LANG}"']), 'C:C',
       'each child gets the stable C locale');
    is($ENV{LC_ALL}, 'POSIX', 'runner preserves caller locale');
}

for my $failed (0, 1) {
    no warnings 'redefine';
    local $ENV{SGE_CELL} = 'caller-cell';
    local $ENV{SGE_QMASTER_PORT} = '1234';
    local *SGEmod::lrms_init = sub {
        $ENV{SGE_CELL} = 'collection-cell';
        $ENV{SGE_QMASTER_PORT} = '5678';
        die "test collection failure\n" if $failed;
    };
    local *SGEmod::type_and_version = sub {};
    local *SGEmod::run_qconf = sub {};
    local *SGEmod::run_qstat = sub {};
    my $ok = eval { SGEmod::get_lrms_info({ queues => {}, jobs => [] }); 1 };
    is(!!$ok, !$failed, "collection success/failure $failed is preserved");
    is($ENV{SGE_CELL}, 'caller-cell', "collection $failed restores caller cell");
    is($ENV{SGE_QMASTER_PORT}, '1234', "collection $failed restores caller port");
}

# Small, named reproductions of return-value bugs, independent of SGE binaries.
{
    local %SGEmod::node_stats = ('node1.site-a' => {});
    is(SGEmod::canonical_node_name('node1'), 'node1.site-a', 'unique short hostname resolves');
    is(SGEmod::canonical_node_name('NODE1.SITE-A'), 'node1.site-a', 'DNS case is immaterial');
    is(SGEmod::canonical_node_name('node1.site-b'), 'node1.site-b', 'different DNS domains are not merged');
    $SGEmod::node_stats{'node1.site-b'} = {};
    is(SGEmod::canonical_node_name('node1'), 'node1', 'ambiguous short hostname is not guessed');
}
SGEmod::reset_state();
$SGEmod::waiting_jobs{12} = { tasks => 3, slots => 2 };
SGEmod::cluster_info();
SGEmod::cluster_info();
is($SGEmod::lrms_info->{cluster}{queuedjobs}, 3, 'repeated summaries do not accumulate jobs');
is($SGEmod::lrms_info->{cluster}{queuedcpus}, 6, 'repeated summaries do not accumulate CPUs');

sub details {
    my ($output, $queued, $failed) = @_;
    SGEmod::reset_state();
    if ($queued) {
        $SGEmod::waiting_jobs{12} = { state => 'qw', slots => 2, rank => 1 };
    } else {
        $SGEmod::running_jobs{12}{0} = { state => 'r', slots => 2, nodes => { node1 => 2 } };
    }
    no warnings 'redefine';
    local *SGEmod::loop_callback = sub {
        my ($command, $callback) = @_;
        return 0 if $failed;
        $callback->($_) for split /\n/, $output;
        return 1;
    };
    $SGEmod::lrms_info->{jobs} = { sentinel => {} };
    return eval { SGEmod::jobs_info([12]); 1 };
}

for my $case (
    ['memory suffix garbage', 'usage 1: cpu=00:01:00, maxvmem=2Garbage'],
    ['CPU suffix garbage', 'usage 1: cpu=00:01:00oops, maxvmem=2G'],
    ['missing CPU equals', 'usage 1: cpu 00:01:00'],
    ['missing usage separator', 'usage 1 cpu=00:01:00'],
    ['duplicate memory', 'usage 1: maxvmem=1G, maxvmem=2G'],
    ['invalid requested limit', 'hard resource_list: h_rt=oops'],
    ['trailing requested limit garbage', 'hard resource_list: h_rt=60 oops'],
    ['missing requested limit value', 'hard resource_list: h_rt='],
    ['missing requested limit equals', 'hard resource_list: h_rt 60'],
    ['duplicate requested limit', 'hard resource_list: h_rt=60,h_rt=120'],
    ['duplicate job header', "job_number: 12\nusage 1: maxvmem=2G"],
    ['empty successful details', undef],
) {
    my $output = defined $case->[1] ? "job_number: 12\nusage 1: maxvmem=1G\n$case->[1]" : '';
    ok(!details($output), "$case->[0] rejected");
    like($@, qr/qstat -j/, "$case->[0] identifies the query");
    is_deeply($SGEmod::lrms_info->{jobs}, { sentinel => {} }, "$case->[0] cannot publish partial details");
}
ok(details("job_number: 12\nusage 1: cpu=1:02:03:04, maxvmem=1.5G\nhard resource_list: h_cpu=120,s_cpu=60,h_rt=INFINITY"),
   'valid details parse');
is($SGEmod::lrms_info->{jobs}{12}{mem}, 1572864, 'memory converted to kB');
is($SGEmod::lrms_info->{jobs}{12}{cputime}, 93784, 'CPU duration converted to seconds');
is($SGEmod::lrms_info->{jobs}{12}{reqcputime}, 60, 'lowest finite CPU limit selected');
ok(!exists $SGEmod::lrms_info->{jobs}{12}{reqwalltime}, 'INFINITY is not zero');
ok(details("job_number: 12\nusage 1: othercpu=00:01:00, othermaxvmem=2G"), 'unknown usage fields tolerated');
ok(!exists $SGEmod::lrms_info->{jobs}{12}{mem} && !exists $SGEmod::lrms_info->{jobs}{12}{cputime},
   'similarly named fields do not become usage');
ok(details("job_number: 12\nhard resource_list: h_rt=60\nerror reason 1: cannot start", 1), 'queued details parse');
is($SGEmod::lrms_info->{jobs}{12}{reqwalltime}, 60, 'queued limit in seconds');
like($SGEmod::lrms_info->{jobs}{12}{comment}[0], qr/cannot start/, 'queued error preserved');
ok(details('', 0, 1), 'optional command failure retains live status');
is($SGEmod::lrms_info->{jobs}{12}{status}, 'R', 'failed details do not mean executed');
ok(!exists $SGEmod::lrms_info->{jobs}{12}{mem}, 'failed details leave memory unknown');

{
    no warnings 'redefine';
    SGEmod::reset_state();
    $SGEmod::waiting_jobs{12} = { state => 'qw', slots => 1, rank => 1 };
    my @queries;
    local *SGEmod::loop_callback = sub {
        my ($command, $callback) = @_;
        push @queries, $command->[-1];
        $callback->('job_number: 12');
        return 1;
    };
    SGEmod::jobs_info([12, 12, 99]);
    is_deeply(\@queries, ['12'], 'duplicate requested IDs are queried once');
    is_deeply(\%SGEmod::running_jobs, {}, 'pending and absent lookups do not create running jobs');
    is($SGEmod::lrms_info->{jobs}{99}{status}, 'EXECUTED', 'absent job still uses validated snapshot');
}
done_testing();
