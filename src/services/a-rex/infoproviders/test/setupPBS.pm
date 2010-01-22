package setupPBS;

use Cwd;
use File::Temp;

our @progs = qw(qmgr qstat pbsnodes showq showbf);

sub new {
    my ($this, $testname) = @_;

    # find the path to the fake binary
    my $srcdir = $ENV{srcdir};
    die("$0: environment variable 'srcdir' not defined") unless defined $srcdir;
    $srcdir = getcwd()."/$srcdir" unless $srcdir =~ m{^/};
    my $mockprog = "$srcdir/$testname-uimock.sh";
    die("$0: File not found: $mockprog") unless -f $mockprog;

    # directory where symlinks to the fake binary will be created
    my $tmpdir = File::Temp::tempdir('./binXXXXXX', CLEANUP => 1);

    # create soft links
    for my $prog (@progs) {
       my $newname = "$tmpdir/$prog";
       symlink($mockprog, $newname)
           or die("$0: Failed creating symlink: $newname");
    }

    my $class = ref($this) || $this;
    my $self = { testname => $testname, tmpdir => $tmpdir };
    return bless $self, $class;
}

sub defaultcfg {
    my ($self) = @_;
    return { lrms => 'pbs',
             maui_bin_path => $self->{tmpdir},
             pbs_bin_path => $self->{tmpdir},
             pbs_log_path => $self->{tmpdir},
             dedicated_node_string => '', # NB: it's set in order to avoid perl warnings 
             queues => {},
             jobs => []
    };
}

1;
