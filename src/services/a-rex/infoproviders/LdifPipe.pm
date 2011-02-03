package LdifPipe;

# Communication between the a-rex infoprovider and ldap-infosys
#  * data is passed through a fifo
#  * ldap-infosys waits until data has arrived, thus synchronizig it's cycle with a-rex

use POSIX;
use Fcntl;
use English;
use File::Basename;
use File::Temp qw(tempfile tempdir);
use File::Path qw(mkpath);

use LogUtils;

our $log = LogUtils->getLogger(__PACKAGE__);

LogUtils::level("VERBOSE");

#
# Given a pid file, returns the user id of the running process
#
sub uidFromPidfile {
    my ($pidfile) = @_;
    open(my $fh, "<", "$pidfile") or return undef;
    my @stat = stat $pidfile;
    my $pid = <$fh>;
    close $fh;
    $pid =~ m/^\s*(\d+)\s*$/ or return undef;
    my $uid = `ps -ouid= $pid`;
    close $fh;
    $uid =~ m/^\s*(\d+)\s*$/ or return undef;
    return $1;
}

#
# stat the file, get the uid
#
sub uidFromFile {
    my ($file) = @_;
    my @stat = stat $file;
    return undef unless @stat;
    return $stat[4];
}

#
# switch effective user if possible. This is reversible.
#
sub switchEffectiveUser {
    my ($uid) = @_;
    if ($UID == 0 && $uid != 0) {
        my ($name, $pass, $uid, $gid) = getpwuid($uid);
        return unless  $gid;
        eval { $EGID = $gid;
               $EUID = $uid
        };
    }
}

#
# Fancy way to create a fifo and open it for reading
#  * if the file already exists, unlinks it
#  * blocks until the other end opens the fifo for writing
#  * returns a file handle for the opened fifo or undef on error
#
sub openForReading {
    my ($fifopath) = @_;
 
    my $basename = basename($fifopath);
    my $dirname = dirname($fifopath);

    eval { mkpath($dirname); };
    if ($@) {
        $log->warning("Failed creating parent directory $dirname: $@");
        return undef;
    }

    if (-e $fifopath) {
        $log->warning("Fifo file has not been removed yet by a-rex. Check that a-rex is running");
        $log->info("Unlinking $fifopath");
        unlink $fifopath;
    }
    
    unless (POSIX::mkfifo $fifopath, 0600) {
        $log->warning("Mkfifo failed: $fifopath: $!");
        return undef;
    }
 
    my $handle;

    # This might be a long wait. In case somebody kills us, be nice and clean up.
    $log->verbose("Opening fifo for reading: $fifopath");
    my $ret;
    eval {
        local $SIG{TERM} = sub { die "terminated\n" };
        unless ($ret = sysopen($handle, $fifopath, O_RDONLY)) {
            $log->warning("Failed to open: $fifopath: $!");
            unlink $fifopath;
        }
    };
    if ($@) {
        unlink $fifopath;
        $log->error("Unexpected: $@") unless $@ eq "terminated\n";

        $log->warning("SIGTERM caught while waiting for ldif data");
        return undef;
    }
    return undef unless $ret;

    $log->verbose("Reading ldif data...");

    return $handle;
}

#
# Opens an existing file (assumed to be a fifo) for reading
#  * finds out whether there is a reader on the other end
#  * opens the file and then unlinks it
#  * returns a file handle for the opened fifo or undef on error
#
sub openForWriting {
    my ($fifopath) = @_;
 
    my $handle;
 
    # Open the fifo -- Normally it should't block since the other end is
    # supposed to be listening. If it blocks nevertheless, something must have
    # happened to the reader and it's not worth waiting here. Set an alarm and
    # get out.
    my $ret;
    eval {
        local $SIG{ALRM} = sub { die "alarm\n" };
        alarm 5;
        unless ($ret = sysopen($handle, $fifopath, O_WRONLY)) {
            $log->warning("Failed to open fifo (as user id $EUID): $fifopath: $!");
        }
        alarm 0;
    };
    if ($@) {
        unlink $fifopath;
        $log->error("Unexpected: $@") unless $@ eq "alarm\n";
 
        # timed out -- no reader
        $log->warning("Ldap-infosys is not listening on fifo: $fifopath");
        return undef;
    }
    return undef unless $ret;
 
    $log->verbose("Opened for writing: $fifopath");
 
    unless (unlink $fifopath) {
        $log->warning("Failed to unlink: $fifopath: $!");
        close $handle if $handle;
        return undef;
    }
    $log->verbose("Unlinked: $fifopath");
 
    return $handle;
}

#
# Finds infosys' runtime directory and the infosys user's uid
#
sub findInfosys {
    my $bdii_var_dir = "/var/run/bdii4";

    my $infosys_uid;
    my $infosys_runtime_dir;

    my $bdii_pidfile = "/var/run/bdii-update.pid";
    my $bdii4_pidfile = "$bdii_var_dir/bdii-update.pid";
    for my $pidfile ( $bdii_pidfile, $bdii4_pidfile ) {
        unless ( defined ($infosys_uid = uidFromFile($pidfile)) ) {
            $log->verbose("Infosys pidfile not found at: $pidfile");
            next;
        }
        $log->verbose("Infosys pidfile found at: $pidfile");
        next unless (my $user = getpwuid($infosys_uid));
        if ($user eq "root") {
            $infosys_runtime_dir = "/var/run/nordugrid";
        } else {
            $infosys_runtime_dir = "/var/run/${user}-nordugrid";
        }
        $log->verbose("Infosys pidfile owned by: $user ($infosys_uid)");
        last;
    }
    unless ($infosys_runtime_dir) {
        $log->warning("Infosys pid file not found. Check that ldap-infosys is running");
        return ();
    }
    $log->verbose("Infosys runtime directory: $infosys_runtime_dir");
    unless (-d $infosys_runtime_dir) {
        $log->warning("Infosys runtime directory does not exist. Check that ldap-infosys is running");
        return ();
    }
    return ($infosys_runtime_dir, $infosys_uid);
}

#
# To be called by the a-rex infoprovider
#  * switches to the infosys user and opens fifo for reading
#  * calls $print_ldif - a func ref that does the actual ldif printing
#  * switches back to root
#  * return value should be ignored
#
sub pushLdif {
    my ($config, $print_ldif) = @_;

    if (my ($infosys_runtime_dir, $infosys_uid) = findInfosys($config->{bdii_var_dir})) {
        my $fifopath = "$infosys_runtime_dir/arc-ldif.fifo";
        switchEffectiveUser($infosys_uid);
        unless (-e $fifopath) {
            $log->info("Infosys has not yet re-created fifo file $fifopath");
            $log->info("Ldif data not sent this time. Will try again next cycle.");
            switchEffectiveUser($UID);
            return;
        }
        if (my $handle = openForWriting($fifopath)) {
            &$print_ldif($handle);
            close($handle);
            $log->info("Ldif data was sent to infosys");
        } else {
            $log->warning("An error occured while opening fifo for writing");
        }
    }
    switchEffectiveUser($UID);
}

#
# To be called by ldap-infosys
#  * creates fifo then reads data from it (ldif from a-rex infoprovider)
#  * saves the data into an executable script which, when
#    executed, outputs that same data (the ldif) to STDOUT.
#  * returns the path of the script
#  * exits the program in case of error (!)
#
sub pullLdif {
    my ($infosys_runtime_dir, $bdii4) = @_;

    if (not $bdii4) {
        LogUtils::timestamps(1);
    } else {
        # We can't just print anything to stderr because BDII4 redirects stderr
        # to stdout. Disguise log records as comments!
        $log = LogUtils->getLogger("# ".__PACKAGE__);
    }

    my ($fh, $scriptpath);

    if (not $bdii4) {
        my $template = "arc-ldif.sh.XXXXXXX";
        eval {
            ($h, $scriptpath) = tempfile($template, DIR => $infosys_runtime_dir);
        };
        $log->error("Failed to create temporary file: $@") if $@;

    } else {
        # BDII4 has the bad habit of killing with SIGKILL on timeout
        # We don't want to leave random files behind if that happens
        $scriptpath = "$infosys_runtime_dir/arc-ldif.sh";
        open($h, ">", $scriptpath);
        $log->error("Failed to open for writing: $scriptpath: $!") unless $h;
    }

    my $fifopath = "$infosys_runtime_dir/arc-ldif.fifo";
    my $fifo = openForReading($fifopath);

    unless ($fifo) {
        close $h;
        unlink $fifopath;
        unlink $scriptpath;
        $log->error("An error occured while opening fifo for reading");
    }

    # Hopefully this string is not in the ldif
    my $mark=substr rand(), 2;

    eval {
        local $SIG{TERM} = sub { die "terminated\n" };
        die unless print $h "#!/bin/sh\n\n";
        die unless print $h "# Autogenerated by ldap-infosys. Do not modify\n\n";
        die unless print $h "cat <<'EOF_$mark'\n";
        while (defined (my $line = <$fifo>)) {
            die unless print $h $line;
        }
        die unless print $h "\nEOF_$mark\n";
        die unless close $h;
    };
    if ($@) {
        my $msg = "Failed writing to: $scriptpath: $!";
        $msg = "SIGTERM caught while reading for ldif data" if $@ eq "terminated\n";
        close $h;
        close $fifo;
        unlink $fifopath;
        unlink $scriptpath;
        $log->error($msg);
    }
    close $fifo;
    unlink $fifopath;

    unless (chmod 0700, $scriptpath) {
        my $msg = "Chmod failed: $scriptpath: $!";
        unlink $scriptpath;
        $log->error($msg);
    }

    $log->verbose("Ldif received and generator script saved to $scriptpath");
    return $scriptpath;
}

1;
