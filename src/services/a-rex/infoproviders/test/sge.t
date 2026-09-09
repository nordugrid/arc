#!/usr/bin/perl

use strict;
no warnings 'once';
use InfoproviderTestSuite;

my $suite = new InfoproviderTestSuite('sge');

$suite->test('altair-2024', sub {
  my @progs = qw(qsub qstat qconf qhost qdel);
  my $simulator_output = <<'ENDSIMULATOROUTPUT';
# Altair Grid Engine 2024.1.0 (AGE 8.9.0).
# The XML layout follows the qstat and qhost schemas shipped with Grid Engine.

args="qstat -help"
output=<<<ENDOUTPUT
AGE 8.9.0
usage: qstat [options]
ENDOUTPUT

args="qhost -F -xml"
output=<<<ENDOUTPUT
<?xml version='1.0'?>
<qhost xmlns:xsd="https://github.com/gridengine/gridengine/raw/master/source/dist/util/resources/schemas/qhost/qhost.xsd">
  <host name="global">
    <hostvalue name="arch_string">-</hostvalue>
    <hostvalue name="num_proc">-</hostvalue>
  </host>
  <host name="node1.site.org">
    <hostvalue name="mem_total">32.0G</hostvalue>
    <hostvalue name="arch_string">lx-amd64</hostvalue>
    <hostvalue name="swap_total">4.0G</hostvalue>
    <hostvalue name="num_proc">8</hostvalue>
    <resourcevalue name="m_socket" dominance="hl">2</resourcevalue>
  </host>
  <host name="node2.site.org">
    <hostvalue name="num_proc">4</hostvalue>
    <hostvalue name="swap_total">0.0G</hostvalue>
    <hostvalue name="arch_string">lx-amd64</hostvalue>
    <hostvalue name="mem_total">8192.0M</hostvalue>
    <resourcevalue name="m_socket" dominance="hl">1</resourcevalue>
  </host>
</qhost>
ENDOUTPUT

args="qconf -sconf global"
output=<<<ENDOUTPUT
#global:
max_u_jobs                   7
max_jobs                     50
ENDOUTPUT

args="qconf -ssconf"
output=<<<ENDOUTPUT
algorithm                    default
maxujobs                     3
ENDOUTPUT

args="qconf -sql"
output=<<<ENDOUTPUT
all.q
batch.q
gpu.q
ENDOUTPUT

args="qconf -sq all.q,batch.q"
output=<<<ENDOUTPUT
qname                 all.q
slots                 8,[node2.site.org=4]
s_rt                  02:00:00
h_rt                  01:30:00,[node2.site.org=01:00:00],\
                      [@slowhosts=00:45:00]
s_cpu                 INFINITY
h_cpu                 00:45:00,[node2.site.org=00:30:00]
qname                 batch.q
s_rt                  INFINITY
h_rt                  02:00:00
s_cpu                 INFINITY
h_cpu                 INFINITY
ENDOUTPUT

args="qconf -sq gpu.q"
output=<<<ENDOUTPUT
qname                 gpu.q
slots                 1
s_rt                  INFINITY
h_rt                  01:00:00
s_cpu                 INFINITY
h_cpu                 00:30:00
ENDOUTPUT

args="qstat -xml -u * -s a -q * -f"
output=<<<ENDOUTPUT
<?xml version='1.0'?>
<job_info xmlns:xsd="https://github.com/gridengine/gridengine/raw/master/source/dist/util/resources/schemas/qstat/qstat.xsd">
  <queue_info>
    <Queue-List>
      <name>all.q@node1.site.org</name>
      <qtype>BIP</qtype>
      <slots_used>2</slots_used>
      <slots_resv>0</slots_resv>
      <slots_total>8</slots_total>
      <load_avg>0.25</load_avg>
      <arch>lx-amd64</arch>
      <state></state>
      <job_list state="running">
        <JB_job_number>100</JB_job_number>
        <JAT_prio>0.55500</JAT_prio>
        <JB_name>a_parallel_job_name_longer_than_ten_characters</JB_name>
        <JB_owner>alice</JB_owner>
        <state>r</state>
        <JAT_start_time>2024-08-20T10:50:23</JAT_start_time>
        <slots>2</slots>
      </job_list>
    </Queue-List>
    <Queue-List>
      <name>all.q@node2.site.org</name>
      <qtype>BIP</qtype>
      <slots_used>3</slots_used>
      <slots_resv>0</slots_resv>
      <slots_total>4</slots_total>
      <load_avg>1.00</load_avg>
      <arch>lx-amd64</arch>
      <state>d</state>
      <job_list state="running">
        <JB_job_number>100</JB_job_number>
        <JAT_prio>0.55500</JAT_prio>
        <JB_name>a_parallel_job_name_longer_than_ten_characters</JB_name>
        <JB_owner>alice</JB_owner>
        <state>r</state>
        <JAT_start_time>2024-08-20T10:50:23</JAT_start_time>
        <slots>2</slots>
      </job_list>
      <job_list state="running">
        <JB_job_number>101</JB_job_number>
        <JAT_prio>0.50000</JAT_prio>
        <JB_name>suspended_job</JB_name>
        <JB_owner>bob</JB_owner>
        <state>s</state>
        <JAT_start_time>2024-08-20T10:55:23</JAT_start_time>
        <slots>1</slots>
      </job_list>
    </Queue-List>
  </queue_info>
  <job_info>
    <job_list state="pending">
      <JB_job_number>200</JB_job_number>
      <JAT_prio>0.10000</JAT_prio>
      <JB_name>queued_job</JB_name>
      <JB_owner>alice</JB_owner>
      <state>qw</state>
      <JB_submission_time>2024-08-20T11:00:00</JB_submission_time>
      <slots>2</slots>
    </job_list>
    <job_list state="pending">
      <JB_job_number>201</JB_job_number>
      <JAT_prio>0.09000</JAT_prio>
      <JB_name>held_job</JB_name>
      <JB_owner>bob</JB_owner>
      <state>hqw</state>
      <JB_submission_time>2024-08-20T11:01:00</JB_submission_time>
      <slots>1</slots>
    </job_list>
    <job_list state="pending">
      <JB_job_number>202</JB_job_number>
      <JAT_prio>0.08000</JAT_prio>
      <JB_name>error_job</JB_name>
      <JB_owner>alice</JB_owner>
      <state>Eqw</state>
      <JB_submission_time>2024-08-20T11:02:00</JB_submission_time>
      <slots>1</slots>
    </job_list>
    <job_list state="pending">
      <JB_job_number>300</JB_job_number>
      <JAT_prio>0.07000</JAT_prio>
      <JB_name>array_job</JB_name>
      <JB_owner>alice</JB_owner>
      <state>qw</state>
      <JB_submission_time>2024-08-20T11:03:00</JB_submission_time>
      <slots>2</slots>
      <tasks>1-3:1</tasks>
    </job_list>
    <job_list state="finished">
      <JB_job_number>301</JB_job_number>
      <JB_name>retained_finished_job</JB_name>
      <JB_owner>alice</JB_owner>
      <state>f</state>
      <slots>1</slots>
    </job_list>
  </job_info>
</job_info>
ENDOUTPUT

args="qstat -u * -s a -q * -j 100,101"
output=<<<ENDOUTPUT
==============================================================
job_number:                 100
usage    1:                 cpu=00:01:30, mem=1.0 GBs, io=0.0, vmem=2.5G, maxvmem=2.5G
hard resource_list:         h_cpu=00:10:00,h_rt=01:00:00
==============================================================
job_number:                 101
usage    1:                 cpu=00:00:05, mem=1.0 MBs, io=0.0, vmem=512K, maxvmem=512K
hard resource_list:         h_cpu=00:20:00,h_rt=00:30:00
ENDOUTPUT

args="qstat -u * -s a -q * -j 200,201,202"
output=<<<ENDOUTPUT
==============================================================
job_number:                 200
hard resource_list:         h_cpu=00:30:00,h_rt=02:00:00
cannot run because no queue offers all requested resources
==============================================================
job_number:                 201
hard resource_list:         h_cpu=900,h_rt=01:00:00
==============================================================
job_number:                 202
hard resource_list:         h_cpu=00:05:00,h_rt=00:10:00
error reason    1:          can't chdir to /missing: No such file or directory
ENDOUTPUT

args="qstat -xml -u * -s a -f -q all.q,batch.q"
output=<<<ENDOUTPUT
<?xml version='1.0'?>
<job_info>
  <queue_info/>
  <job_info>
    <job_list><JB_job_number>200</JB_job_number><JB_owner>alice</JB_owner><state>qw</state><slots>2</slots></job_list>
    <job_list><JB_job_number>201</JB_job_number><JB_owner>bob</JB_owner><state>hqw</state><slots>1</slots></job_list>
    <job_list><JB_job_number>202</JB_job_number><JB_owner>alice</JB_owner><state>Eqw</state><slots>1</slots></job_list>
    <job_list><JB_job_number>300</JB_job_number><JB_owner>alice</JB_owner><state>qw</state><slots>2</slots><tasks>1-3:1</tasks></job_list>
    <job_list><JB_job_number>301</JB_job_number><JB_owner>alice</JB_owner><state>f</state><slots>1</slots></job_list>
  </job_info>
</job_info>
ENDOUTPUT

args="qstat -xml -u * -s a -f -q gpu.q"
output=<<<ENDOUTPUT
<?xml version='1.0'?>
<job_info><queue_info/><job_info/></job_info>
ENDOUTPUT
ENDSIMULATOROUTPUT

  my $config = sub {
    return {
      sge_bin_path => '<TESTDIR>/bin',
      sge_root => '<TESTDIR>/bin',
      sge_pe => 'mpi',
      sge_exclusive_resource => 'exclusive',
      sge_memory_resource => 'h_vmem',
      sge_wakeupperiod => 30,
      sge_query_retries => 3,
      sge_accounting_retries => 5,
      queues => {
        production => {
          users => [qw(alice bob)],
          sge_queues => 'all.q batch.q',
          sge_pe => 'smp',
          sge_exclusive_resource => 'exclusive_queue',
          sge_memory_resource => 'mem_free',
        },
        gpu => {
          users => [qw(alice bob)],
          sge_queues => 'gpu.q',
        },
      },
      jobs => [qw(100 101 200 201 202 301 999)],
      loglevel => '5',
    };
  };

  my $lrms_info = $suite->collect(\@progs, $simulator_output, $config->());

  is(ref $lrms_info, 'HASH', 'result type');
  is($lrms_info->{cluster}{lrms_type}, 'SGE', 'canonical SGE type');
  is($lrms_info->{cluster}{lrms_version}, '8.9.0', 'Altair version');
  is($lrms_info->{cluster}{totalcpus}, 12, 'cluster totalcpus');
  is($lrms_info->{cluster}{usedcpus}, 4, 'cluster usedcpus excludes suspended slots');
  is($lrms_info->{cluster}{runningjobs}, 2, 'cluster running jobs');
  is($lrms_info->{cluster}{queuedjobs}, 6, 'cluster queued jobs includes array tasks');
  is($lrms_info->{cluster}{queuedcpus}, 10, 'cluster queued CPUs');
  is($lrms_info->{cluster}{cpudistribution}, '4cpu:1 8cpu:1', 'CPU distribution');

  my $queue = $lrms_info->{queues}{production};
  is($queue->{status}, 1, 'queue has an available instance');
  is($queue->{totalcpus}, 12, 'queue totalcpus');
  is($queue->{running}, 4, 'queue running slots excludes suspended slots');
  is($queue->{queued}, 6, 'queue queued jobs');
  is($lrms_info->{queues}{gpu}{queued}, 0, 'disjoint share does not inherit global pending jobs');
  is($queue->{maxrunning}, 12, 'queue maxrunning');
  is($queue->{maxqueuable}, 50, 'queue maxqueuable');
  is($queue->{maxuserrun}, 3, 'scheduler running-jobs-per-user limit');
  is($queue->{maxcputime}, 1800, 'lowest queue-instance CPU limit');
  is($queue->{maxwalltime}, 2700, 'lowest continued queue-instance wall limit');
  is($queue->{mincputime}, 0, 'queue minimum CPU time');
  is($queue->{minwalltime}, 0, 'queue minimum wall time');
  is_deeply($queue->{nodes}, [qw(node1.site.org node2.site.org)], 'queue nodes');
  is($queue->{users}{alice}{queuelength}, 5, 'alice queued tasks');
  is_deeply($queue->{users}{alice}{freecpus}, { 6 => 45 }, 'alice free CPUs use minutes, not queue-limit seconds');
  is($queue->{users}{bob}{queuelength}, 1, 'bob queued jobs');
  is_deeply($queue->{users}{bob}{freecpus}, { 6 => 45 }, 'bob free CPUs');
  is($lrms_info->{queues}{gpu}{users}{alice}{queuelength}, 0,
     'per-user pending count is filtered by share');

  is($lrms_info->{jobs}{100}{status}, 'R', 'parallel job status');
  is($lrms_info->{jobs}{100}{cpus}, 4, 'parallel job slots');
  is_deeply($lrms_info->{jobs}{100}{nodes}, [qw(node1.site.org node2.site.org)], 'parallel job nodes');
  is($lrms_info->{jobs}{100}{mem}, 2621440, 'parallel job max memory in kB');
  is($lrms_info->{jobs}{100}{cputime}, 90, 'parallel job CPU time');
  is($lrms_info->{jobs}{100}{reqcputime}, 600, 'parallel job requested CPU time');
  is($lrms_info->{jobs}{100}{reqwalltime}, 3600, 'parallel job requested wall time');

  is($lrms_info->{jobs}{101}{status}, 'S', 'suspended job status');
  is_deeply($lrms_info->{jobs}{101}{nodes}, ['node2.site.org'], 'suspended job node');
  is($lrms_info->{jobs}{101}{mem}, 512, 'K memory suffix is already kB');
  is($lrms_info->{jobs}{200}{status}, 'Q', 'queued job status');
  is($lrms_info->{jobs}{200}{rank}, 1, 'queued job rank');
  is($lrms_info->{jobs}{200}{cpus}, 2, 'queued job slots');
  like($lrms_info->{jobs}{200}{comment}[0], qr/no queue offers/, 'queued reason');
  is($lrms_info->{jobs}{201}{status}, 'H', 'held job status');
  is($lrms_info->{jobs}{202}{status}, 'O', 'error job status');
  like($lrms_info->{jobs}{202}{comment}[0], qr/can't chdir/, 'error reason is reported without deleting job');
  is($lrms_info->{jobs}{301}{status}, 'EXECUTED', 'retained finished job is not reported as live');
  is($lrms_info->{jobs}{999}{status}, 'EXECUTED', 'missing job is executed');

  my $node1 = $lrms_info->{nodes}{'node1.site.org'};
  is($node1->{isavailable}, 1, 'node1 available');
  is($node1->{isfree}, 1, 'node1 free');
  is($node1->{lcpus}, 8, 'node1 logical CPUs');
  is($node1->{slots}, 8, 'node1 slots');
  is($node1->{pcpus}, 2, 'node1 sockets');
  is($node1->{pmem}, 33554432, 'node1 physical memory in kB');
  is($node1->{vmem}, 37748736, 'node1 virtual memory in kB');
  is($node1->{sysname}, 'Linux', 'node1 operating system');
  is($node1->{machine}, 'x86_64', 'node1 architecture');

  my $node2 = $lrms_info->{nodes}{'node2.site.org'};
  is($node2->{isavailable}, 0, 'disabled node unavailable');
  is($node2->{isfree}, 0, 'disabled node not free');
  is($node2->{lcpus}, 4, 'disabled node capacity retained');
  is($node2->{pmem}, 8388608, 'node2 physical memory in kB');

  # A second collection in the same process must not retain counters or nodes.
  my $long_banner = $simulator_output;
  $long_banner =~ s/\nAGE 8\.9\.0\n/\nAltair Grid Engine 2024.1.0 (8.9.0)\n/;
  my $again = $suite->collect(\@progs, $long_banner, $config->());
  is($again->{cluster}{lrms_type}, 'SGE', 'long Altair banner keeps canonical type');
  is($again->{cluster}{lrms_version}, '8.9.0', 'long Altair banner technical version');
  is($again->{cluster}{queuedjobs}, 6, 'queued counter reset between collections');
  is_deeply($again->{queues}{production}{users}{alice}{freecpus}, { 6 => 45 }, 'user counters reset between collections');

  # A failed query may still have emitted well-formed but incomplete XML.  It
  # must not replace the last complete snapshot with that data.
  my $failed_query = <<'ENDFAILEDQUERY';
args="qstat -xml -u * -s a -q * -f"
rc=1
output=<<<ENDOUTPUT
<?xml version='1.0'?>
<job_info>
  <queue_info>
    <Queue-List>
      <name>all.q@partial.site.org</name>
      <slots_used>0</slots_used>
      <slots_total>1</slots_total>
    </Queue-List>
  </queue_info>
</job_info>
ENDOUTPUT
ENDFAILEDQUERY
  $suite->setup([ 'qstat' ], $failed_query);
  {
    local $SGEmod::path = "$suite->{_current_testdir}/bin";
    local $SGEmod::compat_mode = 0;
    local %SGEmod::node_stats = (sentinel => { totalcpus => 1 });
    my $queried = eval { SGEmod::run_qstat(); 1 };
    ok(!$queried, 'failed qstat query is fatal');
    like($@, qr/Failed running command/, 'failed qstat query is identified');
    is_deeply([ sort keys %SGEmod::node_stats ], ['sentinel'],
              'failed qstat query does not publish partial XML');

    my $parsed = eval {
      SGEmod::qstat_xml_parser_callback('<job_info><queue_info>');
      1;
    };
    ok(!$parsed, 'malformed qstat XML is fatal');
    is_deeply([ sort keys %SGEmod::node_stats ], ['sentinel'],
              'malformed qstat XML does not mutate provider state');
  }

  my $schema = SGEmod::get_lrms_options_schema();
  is($schema->{sge_root}, '*', 'SGE_ROOT remains an optional environment fallback');
  is($schema->{sge_bin_path}, '*', 'SGE binary path remains an optional PATH fallback');

  my $defaults_file = "$suite->{_current_testdir}/sge_qstat";
  open(my $defaults, '>', $defaults_file) or die $!;
  print {$defaults} '-s rs -u $user -q all.q', "\n";
  close($defaults) or die $!;
  my $defaults_ok = eval { SGEmod::validate_qstat_defaults($defaults_file); 1 };
  ok($defaults_ok, 'overridden qstat selector defaults are accepted');
  open($defaults, '>', $defaults_file) or die $!;
  print {$defaults} q{-u '*' # all users}, "\n";
  close($defaults) or die $!;
  $defaults_ok = eval { SGEmod::validate_qstat_defaults($defaults_file); 1 };
  ok($defaults_ok, 'inline qstat comments follow Grid Engine parsing');
  open($defaults, '>', $defaults_file) or die $!;
  print {$defaults} "-l arch=lx-amd64\n";
  close($defaults) or die $!;
  $defaults_ok = eval { SGEmod::validate_qstat_defaults($defaults_file); 1 };
  ok(!$defaults_ok, 'unneutralizable qstat defaults fail the provider closed');
  like($@, qr/Unsafe qstat default '-l'/, 'unsafe qstat option is identified');
  open($defaults, '>', $defaults_file) or die $!;
  print {$defaults} qq{-q "unterminated\n};
  close($defaults) or die $!;
  $defaults_ok = eval { SGEmod::validate_qstat_defaults($defaults_file); 1 };
  ok(!$defaults_ok, 'malformed qstat defaults fail the provider closed');
  like($@, qr/Malformed qstat defaults/, 'malformed qstat defaults are identified');
  unlink $defaults_file;

  # A failed share-filtered query must not publish a partial pending count.
  my $failed_filtered_query = <<'ENDFAILEDFILTER';
args="qstat -xml -u * -s a -f -q all.q"
rc=1
output=<<<ENDOUTPUT
<?xml version='1.0'?>
<job_info><queue_info/><job_info><job_list><JB_job_number>900</JB_job_number><JB_owner>alice</JB_owner><tasks>1-2:1</tasks></job_list></job_info></job_info>
ENDOUTPUT
ENDFAILEDFILTER
  $suite->setup([ 'qstat' ], $failed_filtered_query);
  {
    local $SGEmod::path = "$suite->{_current_testdir}/bin";
    my $queried = eval { SGEmod::queue_waiting_counts('all.q'); 1 };
    ok(!$queried, 'failed queue-filtered qstat query is fatal');
    like($@, qr/Failed running command/, 'failed filtered query is identified');
  }
});

$suite->testing_done();
