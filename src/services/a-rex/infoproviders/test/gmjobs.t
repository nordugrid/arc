#!/usr/bin/perl
use strict;
use warnings;
use Test::More;
use File::Temp qw(tempdir);
use File::Path qw(make_path);

our ($lookups, $owner);
BEGIN {
    *CORE::GLOBAL::getpwuid = sub { ++$lookups; return $owner; };
}
use GMJobsInfo;

my $root = tempdir(CLEANUP => 1);
make_path("$root/accepting", "$root/processing", "$root/finished");
for my $id ('123456789001', '123456789002', '123456789003') {
    my $path = GMJobsInfo::control_path($root, $id, '');
    make_path($path);
    open(my $local, '>', "${path}local") or die $!;
    print $local "queue=short\nlocalid=42\nsubject=test\ninterface=org.ogf.glue.emies.activitycreation\n";
    close($local) or die $!;
    open(my $status, '>', "$root/processing/$id.status") or die $!;
    print $status "INLRMS\n";
    close($status) or die $!;
}

$lookups = 0;
$owner = 'first-owner';
my $jobs = GMJobsInfo::get_gmjobs($root, 1);
is(scalar keys %$jobs, 3, 'all jobs are collected');
is($lookups, 1, 'one account lookup for jobs sharing a UID');
is($jobs->{'123456789003'}{localowner}, $owner, 'cached account name is published');
is($jobs->{'123456789001'}{share}, 'short', 'share mapping is unchanged');

$owner = 'renamed-owner';
$jobs = GMJobsInfo::get_gmjobs($root, 1);
is($lookups, 2, 'account cache is refreshed on the next collection');
is($jobs->{'123456789003'}{localowner}, $owner, 'account changes are observed');

$owner = undef;
$lookups = 0;
$jobs = GMJobsInfo::get_gmjobs($root, 1);
is($lookups, 1, 'failed account lookups are also cached for one collection');
ok(!exists $jobs->{'123456789001'}{localowner}, 'unknown accounts remain unknown');

# Failure details are needed for terminal classification even in summary mode,
# but are not published for active jobs when individual jobs are suppressed.
for my $case (
    ['123456789001', 'INLRMS', "temporary failure\n"],
    ['123456789002', 'FINISHED', "Job is canceled by external request\n"],
    ['123456789003', 'FINISHED', ''],
) {
    my ($id, $state, $failure) = @$case;
    open(my $status, '>', "$root/processing/$id.status") or die $!;
    print $status "$state\n";
    close($status) or die $!;
    my $path = GMJobsInfo::control_path($root, $id, 'failed');
    open(my $failed, '>', $path) or die $!;
    print $failed $failure;
    close($failed) or die $!;
}
$jobs = GMJobsInfo::get_gmjobs($root, 1);
is($jobs->{'123456789001'}{status}, 'INLRMS', 'active summary status is unchanged');
ok(!exists $jobs->{'123456789001'}{errors}, 'summary omits unused active-job failure details');
is($jobs->{'123456789002'}{status}, 'KILLED', 'summary still classifies cancelled jobs');
is($jobs->{'123456789003'}{status}, 'FAILED', 'empty failure markers still classify failed jobs');
$jobs = GMJobsInfo::get_gmjobs($root, 0);
is_deeply($jobs->{'123456789001'}{errors}, ['temporary failure'], 'full collection retains active-job errors');
is($jobs->{'123456789002'}{status}, 'KILLED', 'full collection still classifies cancelled jobs');
is($jobs->{'123456789003'}{status}, 'FAILED', 'full collection still classifies failed jobs');

my $failed_path = GMJobsInfo::control_path($root, '123456789003', 'failed');
unlink($failed_path) or die $!;
$jobs = GMJobsInfo::get_gmjobs($root, 1);
is($jobs->{'123456789003'}{status}, 'FINISHED', 'removed marker is observed on the next collection');
for my $summary (0, 1) {
    $jobs = GMJobsInfo::get_gmjobs($root, $summary);
    ok(!exists $jobs->{'123456789003'}{errors}, "missing marker has no errors, summary=$summary");
}
symlink('missing-failure-target', $failed_path) or die $!;
$jobs = GMJobsInfo::get_gmjobs($root, 0);
is($jobs->{'123456789003'}{status}, 'FINISHED', 'dangling marker symlink remains absent');
unlink($failed_path) or die $!;
symlink(GMJobsInfo::control_path($root, '123456789002', 'failed'), $failed_path) or die $!;
$jobs = GMJobsInfo::get_gmjobs($root, 1);
is($jobs->{'123456789003'}{status}, 'KILLED', 'readable marker symlink retains classification');
unlink($failed_path) or die $!;
{
    open(my $failed, '>', $failed_path) or die $!;
    print $failed 'x' x 2048;
    close($failed) or die $!;
}
$jobs = GMJobsInfo::get_gmjobs($root, 0);
is(length($jobs->{'123456789003'}{errors}[0]), 1024, 'failure reads remain bounded');
SKIP: {
    skip 'root can read mode-000 files', 2 if $< == 0;
    chmod(0000, $failed_path) or die $!;
    $jobs = GMJobsInfo::get_gmjobs($root, 1);
    is($jobs->{'123456789003'}{status}, 'FINISHED', 'unreadable marker preserves existing classification');
    ok(!exists $jobs->{'123456789003'}{errors}, 'unreadable marker does not invent errors');
    chmod(0600, $failed_path) or die $!;
}
done_testing();
