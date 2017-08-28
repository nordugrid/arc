package InfoproviderTestSuite;

use Cwd;
use File::Path;
use File::Temp;
use LRMSInfo;
use Test::More;

BEGIN {
  require Exporter;
  require DynaLoader;
  @ISA = qw(Exporter DynaLoader);
  @EXPORT = qw(ok is isnt like unlike cmp_ok can_ok new_ok is_deeply pass fail);
}

sub new {
  my $class = shift;
  my $self = {
    _lrms => shift,
    _ntests => shift,
    _current_test => "",
    _current_name => "",
    _current_testdir => "",
  };

  bless $self, $class;
  return $self;
}

sub test {
  my ( $self, $name, $test ) = @_;
  $self->{_current_name} = $name;
  $self->{_current_test} = $test;

  &$test;
}

sub setup {
  my ( $self, $progs, $simulator_output ) = @_;

  # Create directory for executing test.
  $self->{_current_testdir} = File::Temp::tempdir(getcwd()."/$self->{_lrms}_test_$self->{_current_name}_XXXXXX", CLEANUP => 1);
  mkdir "$self->{_current_testdir}/bin";

  # Create simulator outcome file
  open(my $fh, '>', "$self->{_current_testdir}/simulator-outcome.dat");
  print $fh "$simulator_output";
  close $fh;

  $ENV{SIMULATOR_OUTCOME_FILE} = "$self->{_current_testdir}/simulator-outcome.dat";
  $ENV{SIMULATOR_ERRORS_FILE}  = "$self->{_current_testdir}/simulator-errors.dat";

  # Create soft links
  for my $prog ( @{$progs} ) {
     my $newname = "$self->{_current_testdir}/bin/$prog";
     symlink(getcwd()."/command-simulator.sh", $newname)
         or die("$0: Failed creating symlink: $newname");
  }
}

sub collect {
  my ( $self, $progs, $simulator_output, $cfg ) = @_;
  $self->setup(\@{$progs}, $simulator_output);

  foreach $key (keys %{$cfg}) {
    if (ref($$cfg{$key}) eq "") {
      $$cfg{$key} =~ s/<TESTDIR>/$self->{_current_testdir}/g;
    }
  }
  $$cfg{lrms} = $self->{_lrms};
  
  my $lrms_info = LRMSInfo::collect($cfg);

  if (-e "$self->{_current_testdir}/simulator-errors.dat") {
    open FILE, "$self->{_current_testdir}/simulator-errors.dat";
    $simulator_errors = join("", <FILE>);
    close FILE;
    diag($simulator_errors);
    fail("command simulatation");
  }

  return $lrms_info;
}

sub done_testing {
  Test::More::done_testing();
}

sub testing_done {
  Test::More::done_testing();
}

1;
