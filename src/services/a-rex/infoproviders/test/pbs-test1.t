#!/usr/bin/perl

use Test::More tests => 25;

use strict;
use LRMSInfo;
use setupPBS;

my $setup = setupPBS->new('pbs-test1');
my $cfg = $setup->defaultcfg();
$cfg->{queues}{queue1} = {users => ['user1']};

my $lrms_info = LRMSInfo::collect($cfg);

#use Data::Dumper "Dumper";
#print(Dumper $lrms_info);

is(ref $lrms_info, 'HASH', 'result type');
is(ref $lrms_info->{cluster}, 'HASH', 'has cluster');
is($lrms_info->{cluster}{lrms_type}, 'torque', 'lrms_type');
is($lrms_info->{cluster}{lrms_version}, '2.3.7', 'lrms_version');
is($lrms_info->{cluster}{runningjobs}, 0, 'runningjobs');
is($lrms_info->{cluster}{totalcpus}, 2, 'totalcpus');
is($lrms_info->{cluster}{queuedcpus}, 0, 'queuedcpus');
is($lrms_info->{cluster}{queuedjobs}, 0, 'queuedjobs');
is($lrms_info->{cluster}{usedcpus}, 0, 'usedcpus');
# NB: unnecessary space in value
is($lrms_info->{cluster}{cpudistribution}, ' 2cpu:1', 'cpudistribution');

is(ref $lrms_info->{queues}, 'HASH', 'has queues');
is(ref $lrms_info->{queues}{queue1}, 'HASH', 'has queue1');
ok($lrms_info->{queues}{queue1}{status} >= 0, 'queue1->status');
is($lrms_info->{queues}{queue1}{totalcpus}, 2, 'queue1->totalcpus');
is($lrms_info->{queues}{queue1}{queued}, 0, 'queue1->queued');
is($lrms_info->{queues}{queue1}{running}, 0, 'queue1->running');
is($lrms_info->{queues}{queue1}{maxrunning}, 10, 'queue1->maxrunning');

is(ref $lrms_info->{queues}{queue1}{users}, 'HASH', 'has users');
is(ref $lrms_info->{queues}{queue1}{users}{user1}, 'HASH', 'has user1');
is($lrms_info->{queues}{queue1}{users}{user1}{queuelength}, 0, 'user1->queuelenght');
is_deeply($lrms_info->{queues}{queue1}{users}{user1}{freecpus}, { '2' => 0 }, 'user1->freecpus');

is(ref $lrms_info->{nodes}, 'HASH', 'has nodes');
is(ref $lrms_info->{nodes}{node1}, 'HASH', 'has node1');
is($lrms_info->{nodes}{node1}{isavailable}, 1, 'node1->isavailable');
is($lrms_info->{nodes}{node1}{slots}, 2, 'node1->slots');

