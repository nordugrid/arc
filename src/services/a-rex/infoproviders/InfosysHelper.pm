package InfosysHelper;

# Helper functions to be used for communication between the A-REX infoprovider and ldap-infosys
#
#  * for A-REX infoprovider:
#      - createLdifScript: creates a script that prints the ldif from the infoprovider when executed,
#      - notifyInfosys: notifies ldap-infosys through a fifo file created by ldap-infosys.
#  * for ldap-infosys:
#      - waitForProvider: waits for A-REX infoprovider to give a life sign on the fifo it created
#      - ldifIsReady: calls waitForProvider and checks that ldif is fresh enough

## TODO: do NOT hardcode defaults here anymore. Take them from configuration.

use POSIX;
use Fcntl;
use English;
use File::Basename;
use File::Temp qw(tempfile tempdir);
use File::Path qw(mkpath);
#use Data::Dumper::Concise;
## usage:
# print Dumper($datastructure); 

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
# It switches back to root if the passed parameter
# is 0.
#
sub switchEffectiveUser {
    my ($uid) = @_;
    if ($UID == 0 && $uid != 0) {
        my ($name, $pass, $uid, $gid) = getpwuid($uid);
        return unless defined $gid;
        eval { $EGID = $gid;
               $EUID = $uid;
        };
    # Switch back to original UID/GID
    } else {
        eval { $EGID = $GID;
               $EUID = $UID;
        };
    };
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
    # TODO: this is a bit complicated and due to BDII4/BDII5.
    # Maybe it needs simplification, but requires understanding
    # of what happens in BDII5 since they changed directory paths.
    #
    sub findInfosys {
        return @$cache if defined $cache;
        my ($config) = @_;
        my ($bdii_run_dir) = $config->{bdii_run_dir};
        # remove trailing slashes
        $bdii_run_dir =~ s|/\z||;
        $log->debug("BDII run dir set to: $bdii_run_dir");
        
        # TODO: remove this legacy BDII4 location from here and from grid-infosys
        my ($bdii_var_dir) = $config->{bdii_var_dir};
        # remove trailing slashes
        $bdii_var_dir =~ s|/\z||;
        $log->debug("BDII var dir set to: $bdii_var_dir");
        
        my ($bdii_update_pid_file) = $config->{bdii_update_pid_file};
        $log->debug("BDII pid guessed location: $bdii_update_pid_file. Will search for it later");
        
        my ($infosys_uid, $infosys_gid);
        
        my $infosys_ldap_run_dir = $config->{infosys_ldap_run_dir};
        # remove trailing slashes
        $infosys_ldap_run_dir =~ s|/\z||;
        $log->debug("LDAP subsystem run dir set to $infosys_ldap_run_dir");
        
        # search for bdii pid file: legacy bdii4 locations still here
        # TODO: remove bdii_var_dir from everywhere (also from grid-infosys)
        # if not specified with bdii_update_pid_file, it's likely here
        my $existsPidFile = 0;
        my $bdii5_pidfile = "$bdii_run_dir/bdii-update.pid";
        my $bdii4_pidfile = "$bdii_var_dir/bdii-update.pid";
        for my $pidfile ( $bdii_update_pid_file, $bdii5_pidfile, $bdii4_pidfile) {
            unless ( ($infosys_uid, $infosys_gid) = uidGidFromFile($pidfile) ) {
                $log->verbose("BDII pidfile not found at: $pidfile");
                next;
            }
            $existsPidFile = 1;
            $log->verbose("BDII pidfile found at: $pidfile");
            next unless (my $user = getpwuid($infosys_uid));
            $log->verbose("BDII pidfile owned by: $user ($infosys_uid)");
            last;
        }
        unless ($existsPidFile) {
            $log->warning("BDII pid file not found. Check that nordugrid-arc-bdii is running, or that bdii_run_dir is set");
            return @$cache = ();
        }
        unless (-d $infosys_ldap_run_dir) {
            $log->warning("LDAP information system runtime directory does not exist. Check that:");
            $log->warning("1) The arc.conf parameter infosys_ldap_run_dir is correctly set if manually added.");
            $log->warning("2) nordugrid-arc-bdii is running");
            return @$cache = ();
        }
        return @$cache = ($infosys_ldap_run_dir, $infosys_uid, $infosys_gid);
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

    # my ($infosys_ldap_run_dir) = findInfosys($config);
    my $infosys_ldap_run_dir = $config->{infosys_ldap_run_dir};
    return undef unless $infosys_ldap_run_dir;

    my $fifopath = "$infosys_ldap_run_dir/ldif-provider.fifo";

    unless (-e $fifopath) {
        $log->info("LDAP subsystem has not yet created fifo file $fifopath");
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
        $log->warning("Fifo file exists but LDAP information system is not listening");
        return undef;
    }
    return undef unless $ret;
    close $handle;

    $log->info("LDAP information system notified on fifo: $fifopath");
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

    my ($infosys_ldap_run_dir, $infosys_uid, $infosys_gid) = findInfosys($config);
    return undef unless $infosys_ldap_run_dir;

    eval { mkpath($infosys_ldap_run_dir); };
    if ($@) {
        $log->warning("Failed creating parent directory $infosys_ldap_run_dir: $@");
        return undef;
    }
    unless (chown $infosys_uid, $infosys_gid, $infosys_ldap_run_dir) {
        $log->warning("Chown to uid($infosys_uid) gid($infosys_gid) failed on: $infosys_ldap_run_dir: $!");
        return undef;
    }

    switchEffectiveUser($infosys_uid);

    my ($h, $tmpscript);
    eval {
        my $template = "ldif-provider.sh.XXXXXXX";
        ($h, $tmpscript) = tempfile($template, DIR => $infosys_ldap_run_dir);
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

    my $finalscript = "$infosys_ldap_run_dir/ldif-provider.sh";

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
#  max_cycle is a number calculated as such by the init scripts: 
#     max_cycle=$(( $bdii_provider_timeout + $infoproviders_timelimit + $wakeupperiod ))
#     where:
#     - $bdii_provider_timeout:    max time for BDII infoproviders to run
#     - $infoproviders_timelimit:  max time for PERL infoproviders to run
#     - $wakeupperiod:             interval used by A-REX to restart CEinfo.pl
#  max_cycle is the time bdii-update will trust the content of any provider to be fresh enough
sub ldifIsReady {
    my ($infosys_ldap_run_dir, $max_cycle) = @_;

    LogUtils::timestamps(1);

    # Check if ldif generator script exists and is fresh enough
    my $scriptpath = "$infosys_ldap_run_dir/ldif-provider.sh";
    unless (-e $scriptpath) {
        $log->info("The ldif generator script was not found ($scriptpath)");
        $log->info("This file should have been created by A-REX's infoprovider. Check that A-REX is running.");
        return undef;
    }
    my @stat = stat $scriptpath;
    $log->error("Cant't stat $scriptpath: $!") unless @stat;
    if (time() - $stat[9] > $max_cycle) {
        $log->info("The ldif generator script is too old ($scriptpath)");
        $log->info("This file should have been refreshed by A-REX's infoprovider. Check that A-REX is running.");
        return undef;
    }

    # A-REX has started up... Wait for the next infoprovider cycle
    waitForProvider($infosys_ldap_run_dir)
        or $log->warning("Failed to receive notification from A-REX's infoprovider");

    $log->verbose("Using ldif generator script: $scriptpath");
    return 1;
}
	
1;
