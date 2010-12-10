package HostInfo;

use POSIX;
use Sys::Hostname;
use Time::Local;

use strict;

BEGIN {
    eval {require Time::HiRes; import Time::HiRes "time"};
}

use Sysinfo;
use LogUtils;
use InfoChecker;

our $host_options_schema = {
        x509_user_cert => '*',
        x509_cert_dir  => '*',
        sessiondir     => [ '' ],
        cachedir       => [ '*' ],
        processes      => [ '' ],
        localusers     => [ '' ],
        gmconfig       => '*',
};

our $host_info_schema = {
        hostname      => '',
        osname        => '*', # see OSName_t, GFD.147
        osversion     => '*', # see OSName_t, GFD.147
        sysname       => '',  # uname -s
        release       => '',  # uname -r
        machine       => '',  # uname -m (what would it be on a linux machine)
        cpuvendor     => '*',
        cpumodel      => '*',
        cpufreq       => '*', # unit: MHz
        cpustepping   => '*',
        pmem          => '*', # unit: MB
        vmem          => '*', # unit: MB
        cputhreadcount=> '*',
        cpucorecount  => '*',
        cpusocketcount=> '*',
        issuerca      => '',
        issuerca_hash => '',
        issuerca_enddate => '*',
        issuerca_expired => '*',
        hostcert_enddate => '*',
        hostcert_expired => '*',
        trustedcas    => [ '' ],
        session_free  => '', # unit: MB
        session_total => '', # unit: MB
        cache_free    => '', # unit: MB
        cache_total   => '', # unit: MB
        globusversion => '*',
        processes  => { '*' => '' },
        localusers => {
            '*' => {
                gridarea => '',
                diskfree => '' # unit: MB
            }
        }
};

our $log = LogUtils->getLogger(__PACKAGE__);

{   my ($t0, $descr);
    sub timer_start($) { $descr = shift; $t0 = time(); }
    sub timer_stop() {
        my $dt = sprintf("%.3f", time() - $t0);
        $log->debug("Time spent $descr: ${dt}s");
}   }


sub collect($) {
    my ($options) = @_;
    my ($checker, @messages);

    $checker = InfoChecker->new($host_options_schema);
    @messages = $checker->verify($options);
    $log->warning("config key options->$_") foreach @messages;
    $log->fatal("Some required options are missing") if @messages;

    my $result = get_host_info($options);

    $checker = InfoChecker->new($host_info_schema);
    @messages = $checker->verify($result);
    $log->debug("SelfCheck: result key hostinfo->$_") foreach @messages;

    return $result;
}


# private subroutines

sub grid_diskspace ($$) {
    my ($sessiondir, $localids) = @_;
    my $commonspace = Sysinfo::diskinfo($sessiondir);
    $log->warning("Failed checking disk space in sessiondir $sessiondir")
         unless $commonspace;

    my $users = {};
    foreach my $u (@$localids) {
        $users->{$u} = {};

        my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$home,$shell,$expire) = getpwnam($u);
        $log->warning("User $u is not listed in the passwd file") unless $name;

        if ($sessiondir =~ /^\s*\*\s*$/) {
            $users->{$u}{gridarea} = $home."/.jobs";
            my $gridspace = Sysinfo::diskinfo($users->{$u}{gridarea});
            $log->warning("Failed checking disk space in personal gridarea $users->{$u}{gridarea}")
                unless $gridspace;
            $users->{$u}{diskfree} = $gridspace->{megsfree} || 0;
        } else {
            $users->{$u}{gridarea} = $sessiondir;
            $users->{$u}{diskfree} = $commonspace->{megsfree} || 0;
        }
    }
    return $users;
}

# Obtain the end date of a certificate (in seconds since the epoch)
sub enddate {
    my ($openssl, $certfile) = @_;

    # assuming here that the file exists and is a well-formed certificate.
    chomp (my $stdout=`$openssl x509 -noout -enddate -in '$certfile'`);
    return undef if $?;

    my %mon = (Jan=>0,Feb=>1,Mar=>2,Apr=>3,May=>4,Jun=>5,Jul=>6,Aug=>7,Sep=>8,Oct=>9,Nov=>10,Dec=>11);
    if ($stdout =~ m/notAfter=(\w{3})  ?(\d\d?) (\d\d):(\d\d):(\d\d) (\d{4}) GMT/ and exists $mon{$1}) {
        return timegm($5,$4,$3,$2,$mon{$1},$6);
    } else {
        $log->warning("Unexpected -enddate from openssl for $certfile");
        return undef;
    }
}


# Hostcert, issuer CA, trustedca, issuercahash, enddate ...
sub get_cert_info {
    my ($options, $globusloc) = @_;

    my $host_info = {};

    if (not $options->{x509_user_cert}) {
        $log->info("x509_user_cert not configured");
        return $host_info;
    }

    # find an openssl
    my $openssl = '';
    for my $path (split ':', "$ENV{PATH}:$globusloc/bin") {
        $openssl = "$path/openssl" and last if -x "$path/openssl";
    }
    $log->error("Could not find openssl command") unless $openssl;

    # Inspect host certificate
    my $hostcert = $options->{x509_user_cert};
    chomp (my $issuerca = `$openssl x509 -noout -issuer -in '$hostcert'`);
    if ($?) {
        $log->warning("Failed processing host certificate file: $hostcert") if $?;
    } else {
        $issuerca =~ s/issuer= //;
        $host_info->{issuerca} = $issuerca;
        $host_info->{hostcert_enddate} = enddate($openssl, $hostcert);
        system("$openssl x509 -noout -checkend 3600 -in '$hostcert'");
        $host_info->{hostcert_expired} = $? ? 1 : 0;
        $log->warning("Host certificate is expired in file: $hostcert") if $?;
    }

    if (not $options->{x509_cert_dir}) {
        $log->info("x509_cert_dir not configured");
        return $host_info;
    }

    # List certs and elliminate duplication in case 2 soft links point to the same file.
    my %certfiles;
    my $certdir = $options->{x509_cert_dir};
    opendir(CERTDIR, $certdir) or $log->error("Failed listing certificates directory $certdir: $!");
    for (readdir CERTDIR) {
        next unless m/\.\d$/;
        my $file = $certdir."/".$_;
        my $link = -l $file ? readlink $file : $_;
        $certfiles{$link} = $file;
    }
    closedir CERTDIR;

    my %trustedca;
    foreach my $cert ( sort values %certfiles ) {
       chomp (my $ca_sn = `$openssl x509 -checkend 3600 -noout -subject -in '$cert'`);
       my $is_expired = $?;
       $ca_sn =~ s/subject= //;
       if ($ca_sn eq $issuerca) {
           chomp (my $issuerca_hash = `$openssl x509 -noout -hash -in '$cert'`);
           if ($?) {
               $log->warning("Failed processing issuer CA certificate file: $cert");
           } else {
               $host_info->{issuerca_hash} = $issuerca_hash || undef;
               $host_info->{issuerca_enddate} = enddate($openssl, $cert);
               $host_info->{issuerca_expired} = $is_expired ? 1 : 0;
               $log->warning("Issuer CA certificate is expired in file: $cert") if $is_expired;
           }
       }
       $log->warning("Certificate is expired for CA: $ca_sn") if $is_expired;
       $trustedca{$ca_sn} = 1 unless $is_expired;
    }
    $host_info->{trustedcas} = [ sort keys %trustedca ];
    $log->warning("Issuer CA certificate file not found") unless exists $host_info->{issuerca_hash};

    return $host_info;
}


sub get_host_info {
    my $options = shift;

    my $host_info = {};

    $host_info->{hostname} = hostname();

    my $osinfo = Sysinfo::osinfo() || {};
    my $cpuinfo = Sysinfo::cpuinfo() || {};
    my $meminfo = Sysinfo::meminfo() || {};
    $log->error("Failed querying CPU info") unless %$cpuinfo;
    $log->error("Failed querying OS info") unless %$osinfo;

    # Globus location
    my $globusloc = $ENV{GLOBUS_LOCATION} || "/usr";
    if ($ENV{GLOBUS_LOCATION}) {
        if ($ENV{LD_LIBRARY_PATH}) {
            $ENV{LD_LIBRARY_PATH} .= ":$ENV{GLOBUS_LOCATION}/lib";
        } else {
            $ENV{LD_LIBRARY_PATH} = "$ENV{GLOBUS_LOCATION}/lib";
        }
    }

    timer_start("collecting certificates info");
    my $certinfo = get_cert_info($options, $globusloc);
    timer_stop();

    $host_info = {%$host_info, %$osinfo, %$cpuinfo, %$meminfo, %$certinfo};

    #TODO: multiple session dirs
    my @sessiondirs = @{$options->{sessiondir}};
    if (@sessiondirs) {
        my $sessionspace = Sysinfo::diskinfo($sessiondirs[0]);
        $log->warning("Failed checking disk space in sessiondir ".$sessiondirs[0])
            unless $sessionspace;
        $host_info->{session_free} = $sessionspace->{megsfree} || 0;
        $host_info->{session_total} = $sessionspace->{megstotal} || 0;

        # users and diskspace
        $host_info->{localusers} = grid_diskspace($sessiondirs[0], $options->{localusers});
    }

    #OBS: only accurate if cache is on a filesystem of it's own
    my @paths = ();
    @paths = @{$options->{cachedir}} if $options->{cachedir};
    @paths = map { my @pair = split " ", $_; $pair[0] } @paths;
    my %cacheinfo = Sysinfo::diskspaces(@paths);
    $log->warning("Failed checking disk space in cache directories: @paths")
        unless %cacheinfo;
    $host_info->{cache_total} = $cacheinfo{totalsum} || 0;
    $host_info->{cache_free} = ($cacheinfo{freemin} || 0) * $cacheinfo{ndisks};

    #Globus Toolkit version
    #globuslocation/share/doc/VERSION
    my $globusversion;
    if (-r "$globusloc/share/doc/VERSION" ) {
       chomp ( $globusversion =  `cat $globusloc/share/doc/VERSION 2>/dev/null`);
       if ($?) { $log->warning("Failed reading the globus version file")}
    }
    #globuslocation/bin/globus-version
    elsif (-x "$globusloc/bin/globus-version" ) {
       chomp ( $globusversion =  `$globusloc/bin/globus-version 2>/dev/null`);
       if ($?) { $log->warning("Failed running $globusloc/bin/globus-version command")}
    }
    $host_info->{globusversion} = $globusversion if $globusversion;

    $host_info->{processes} = Sysinfo::processid(@{$options->{processes}});

    return $host_info;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $options = { x509_user_cert => '/etc/grid-security/hostcert.pem',
                    x509_cert_dir => '/etc/grid-security/certificates',
                    sessiondir => [ '/home/grid/session/' ],
                    cachedir => [ '/home/grid/cache' ],
                    libexecdir => '/opt/nordugrid/libexec/arc',
                    runtimedir => '/home/grid/runtime',
                    processes => [ qw(bash ps init grid-manager bogous) ],
                    localusers => [ qw(root adrianta) ] };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils::level('DEBUG');
    $log->debug("Options:\n" . Dumper($options));
    my $results = HostInfo::collect($options);
    $log->debug("Results:\n" . Dumper($results));
}

#test;


1;
