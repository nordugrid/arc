package InfosysHelper;

# Helper functions to be used for communication between the A-REX infoprovider and ldap-infosys
#
#  * for A-REX infoprovider:
#      - createLdifScript: creates a script that prints the ldif from the infoprovider when executed,
#      - notifyInfosys: notifies ldap-infosys through a fifo file created by ldap-infosys.
#  * for ldap-infosys:
#      - waitForProvider: waits for A-REX infoprovider to give a life sign on the fifo it created
#      - ldifIsReady: calls waitForProvider and checks that ldif is fresh enough

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
# stat the file, get the uid, pid
#
sub uidGidFromFile {
    my ($file) = @_;
    my @stat = stat $file;
    return () unless @stat;
    return (@stat[4,5]);
}

#
# switch effective user if possible. This is reversible.
#
sub switchEffectiveUser {
    my ($uid) = @_;
    if ($UID == 0 && $uid != 0) {
        my ($name, $pass, $uid, $gid) = getpwuid($uid);
        return unless defined $gid;
        eval { $EGID = $gid;
               $EUID = $uid;
        };
    }
}

#
# Waits for a sign from the infoprovider. Implemented using a fifo file.
#  * creates a fifo (unlinks it first if it already exists)
#  * opens the fifo -- this blocks until the other end opens the fifo for writing
#  * returns false in case of error
#
sub waitForProvider {
    my ($runtime_dir) = @_;
 
    my $fifopath = "$runtime_dir/ldif-provider.fifo";

    if (! -d $runtime_dir) {
        $log->warning("No such directory: $runtime_dir");
        return undef;
    }

    if (-e $fifopath) {
        $log->info("Unlinking stale fifo file: $fifopath");
        unlink $fifopath;
    }

    unless (POSIX::mkfifo $fifopath, 0600) {
        $log->warning("Mkfifo failed: $fifopath: $!");
        return undef;
    }
    $log->verbose("New fifo created: $fifopath");
 
    my $handle;

    # This might be a long wait. In case somebody kills us, be nice and clean up.
    $log->info("Start waiting for notification from A-REX's infoprovider");
    my $ret;
    eval {
        local $SIG{TERM} = sub { die "terminated\n" };
        unless ($ret = sysopen($handle, $fifopath, O_RDONLY)) {
            $log->warning("Failed to open: $fifopath: $!");
            unlink $fifopath;
        } else {
            while(<$handle>){}; # not interested in contents
        }
    };
    close $handle;
    unlink $fifopath;
    if ($@) {
        $log->error("Unexpected: $@") unless $@ eq "terminated\n";

        $log->warning("SIGTERM caught while waiting for notification from A-REX's infoprovider");
        return undef;
    }
    return undef unless $ret;

    $log->info("Notification received from A-REX's infoprovider");
    return 1;
}

{   my $cache = undef;

    #
    # Finds infosys' runtime directory and the infosys user's uid, gid
    #
    sub findInfosys {
        return @$cache if defined $cache;

        my ($bdii4_var_dir) = @_;
    
        $bdii4_var_dir ||= "/var/run/bdii4";
    
        my ($infosys_uid, $infosys_gid);
        my $infosys_runtime_dir;
    
        my $bdii_pidfile = "/var/run/arc/bdii-update.pid";
        my $bdii4_pidfile = "$bdii4_var_dir/bdii-update.pid";
        for my $pidfile ( $bdii_pidfile, $bdii4_pidfile ) {
            unless ( ($infosys_uid, $infosys_gid) = uidGidFromFile($pidfile) ) {
                $log->verbose("Infosys pidfile not found at: $pidfile");
                next;
            }
            $log->verbose("Infosys pidfile found at: $pidfile");
            next unless (my $user = getpwuid($infosys_uid));
            $infosys_runtime_dir = "/var/run/arc/infosys";
            $log->verbose("Infosys pidfile owned by: $user ($infosys_uid)");
            last;
        }
        unless ($infosys_runtime_dir) {
            $log->warning("Infosys pid file not found. Check that ldap-infosys is running");
            return @$cache = ();
        }
        $log->verbose("Infosys runtime directory: $infosys_runtime_dir");
        unless (-d $infosys_runtime_dir) {
            $log->warning("Infosys runtime directory does not exist. Check that ldap-infosys is running");
            return @$cache = ();
        }
        return @$cache = ($infosys_runtime_dir, $infosys_uid, $infosys_gid);
    }
}

#
# 
# Notify Infosys that there is a new fresh ldif. Implemented using a fifo file.
#  * finds out whether there is a reader on the other end if the fifo
#  * opens the file and then closes it (thus waking up the listener on other end)
#  * returns false on error
#
sub notifyInfosys {
    my ($config) = @_;

    my ($infosys_runtime_dir) = findInfosys($config->{bdii_var_dir});
    return undef unless $infosys_runtime_dir;

    my $fifopath = "$infosys_runtime_dir/ldif-provider.fifo";

    unless (-e $fifopath) {
        $log->info("Ldap-infosys has not yet created fifo file $fifopath");
        return undef;
    }

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
        $log->error("Unexpected: $@") unless $@ eq "alarm\n";
 
        # timed out -- no reader
        $log->warning("Fifo file exists but ldap-infosys is not listening");
        return undef;
    }
    return undef unless $ret;
    close $handle;

    $log->info("Ldap-infosys notified on fifo: $fifopath");
    return $handle;
}

#
# To be called by the A-REX infoprovider
#  * Takes the ldif generated by calling &$print_ldif and creates an executable
#    script that when executed, outputs that ldif.
#  * If applicable: switches user to that running infosys and then switches back to root
#  * Returns false on error
#
sub createLdifScript {
    my ($config, $print_ldif) = @_;

    my ($infosys_runtime_dir, $infosys_uid, $infosys_gid) = findInfosys($config->{bdii_var_dir});
    return undef unless $infosys_runtime_dir;

    eval { mkpath($infosys_runtime_dir); };
    if ($@) {
        $log->warning("Failed creating parent directory $infosys_runtime_dir: $@");
        return undef;
    }
    unless (chown $infosys_uid, $infosys_gid, $infosys_runtime_dir) {
        $log->warning("Chown to uid($infosys_uid) gid($infosys_gid) failed on: $infosys_runtime_dir: $!");
        return undef;
    }

    switchEffectiveUser($infosys_uid);

    my $tmpscript;
    eval {
        my $template = "ldif-provider.sh.XXXXXXX";
        ($h, $tmpscript) = tempfile($template, DIR => $infosys_runtime_dir);
    };
    if ($@) {
        $log->warning("Failed to create temporary file: $@");
        switchEffectiveUser($UID);
        return undef;
    }

    # Hopefully this string is not in the ldif
    my $mark=substr rand(), 2;

    eval {
        local $SIG{TERM} = sub { die "terminated\n" };
        die "file\n" unless print $h "#!/bin/sh\n\n";
        die "file\n" unless print $h "# Autogenerated by A-REX's infoprovider\n\n";
        die "file\n" unless print $h "cat <<'EOF_$mark'\n";
        &$print_ldif($h);
        die "file\n" unless print $h "\nEOF_$mark\n";
        die "file\n" unless close $h;
    };
    if ($@) {
        my $msg = "An error occured while creating ldif generator script: $@";
        $msg = "An error occured while writing to: $tmpscript: $!" if $@ eq "file\n";
        $msg = "SIGTERM caught while creating ldif generator script" if $@ eq "terminated\n";
        close $h;
        unlink $tmpscript;
        $log->warning($msg);
        $log->verbose("Removing temporary ldif generator script");
        switchEffectiveUser($UID);
        return undef;
    }

    unless (chmod 0700, $tmpscript) {
        $log->warning("Chmod failed: $tmpscript: $!");
        unlink $tmpscript;
        switchEffectiveUser($UID);
        return undef;
    }

    my $finalscript = "$infosys_runtime_dir/ldif-provider.sh";

    unless (rename $tmpscript, $finalscript) {
        $log->warning("Failed renaming temporary script to $finalscript: $!");
        unlink $tmpscript;
        switchEffectiveUser($UID);
        return undef;
    }
    $log->verbose("Ldif generator script created: $finalscript");

    switchEffectiveUser($UID);
    return 1;
}

#
# To be called by ldap-infosys
#  * returns true if/when there is a fresh ldif
#
sub ldifIsReady {
    my ($infosys_runtime_dir, $max_age, $is_bdii4) = @_;

    if (not $is_bdii4) {
        LogUtils::timestamps(1);
    } else {
        # We can't just print anything to stderr because BDII4 redirects stderr
        # to stdout. Disguise log records as comments!
        $log = LogUtils->getLogger("# ".__PACKAGE__);
    }

    # Check if ldif generator script exists and is fresh enough
    my $scriptpath = "$infosys_runtime_dir/ldif-provider.sh";
    unless (-e $scriptpath) {
        $log->info("The ldif generator script was not found ($scriptpath)");
        $log->info("This file should have been created by A-REX's infoprovider. Check that A-REX is running.");
        $log->info("For operation without A-REX, set infosys_compat=enable under the [infosys] section.");
        return undef;
    }
    my @stat = stat $scriptpath;
    $log->error("Cant't stat $scriptpath: $!") unless @stat;
    if (time() - $stat[9] > $max_age) {
        $log->info("The ldif generator script is too old ($scriptpath)");
        $log->info("This file should have been refreshed by A-REX's infoprovider. Check that A-REX is running.");
        $log->info("For operation without A-REX, set infosys_compat=enable under the [infosys] section.");
        return undef;
    }

    # A-REX has started up... Wait for the next infoprovider cycle
    waitForProvider($infosys_runtime_dir)
        or $log->warning("Failed to receive notification from A-REX's infoprovider");

    $log->verbose("Using ldif generator script: $scriptpath");
    return 1;
}

1;
