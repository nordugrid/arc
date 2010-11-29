#!/usr/bin/perl

use Test::More tests => 21;

use strict;
use LRMSInfo;
use setupSGE;

my $setup = setupSGE->new('sge-test1');
my $cfg = $setup->defaultcfg();
$cfg->{queues}{'all.q'} = {users => ['user1']};

my $lrms_info = LRMSInfo::collect($cfg);

#use Data::Dumper "Dumper";
#print STDERR (Dumper $lrms_info);

is(ref $lrms_info, 'HASH', 'result type');
is(ref $lrms_info->{cluster}, 'HASH', 'has cluster');
is($lrms_info->{cluster}{lrms_type}, 'SGE', 'lrms_type');
is($lrms_info->{cluster}{lrms_version}, '6.2', 'lrms_version');
is($lrms_info->{cluster}{runningjobs}, 0, 'runningjobs');
is($lrms_info->{cluster}{totalcpus}, 1, 'totalcpus');
is($lrms_info->{cluster}{queuedcpus}, 0, 'queuedcpus');
is($lrms_info->{cluster}{queuedjobs}, 0, 'queuedjobs');
is($lrms_info->{cluster}{usedcpus}, 0, 'usedcpus');
is($lrms_info->{cluster}{cpudistribution}, '1cpu:1', 'cpudistribution');

is(ref $lrms_info->{queues}, 'HASH', 'has queues');
is(ref $lrms_info->{queues}{'all.q'}, 'HASH', 'has queue all.q');
ok($lrms_info->{queues}{'all.q'}{status} >= 0, 'all.q->status');
is($lrms_info->{queues}{'all.q'}{totalcpus}, 1, 'all.q->totalcpus');
is($lrms_info->{queues}{'all.q'}{queued}, 0, 'all.q->queued');
is($lrms_info->{queues}{'all.q'}{running}, 0, 'all.q->running');
is($lrms_info->{queues}{'all.q'}{maxrunning}, 1, 'all.q->maxrunning');

is(ref $lrms_info->{queues}{'all.q'}{users}, 'HASH', 'has users');
is(ref $lrms_info->{queues}{'all.q'}{users}{user1}, 'HASH', 'has user1');
is($lrms_info->{queues}{'all.q'}{users}{user1}{queuelength}, 0, 'user1->queuelenght');
is_deeply($lrms_info->{queues}{'all.q'}{users}{user1}{freecpus}, { '1' => 0 }, 'user1->freecpus');

#is(ref $lrms_info->{nodes}, 'HASH', 'has nodes');
#is(ref $lrms_info->{nodes}{node1}, 'HASH', 'has node1');
#is($lrms_info->{nodes}{node1}{isavailable}, 1, 'node1->isavailable');
#is($lrms_info->{nodes}{node1}{slots}, 2, 'node1->slots');

