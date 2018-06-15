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
        x509_host_cert => '*',
        x509_cert_dir  => '*',
        wakeupperiod   => '*',
        processes      => [ '' ],
        ports => {
           '*' => [ '*' ] #process name, ports
        },
        localusers     => [ '' ],
        control => {
            '*' => {
                sessiondir => [ '' ],
                cachedir => [ '*' ],
                remotecachedir => [ '*' ],
                cachesize => '*'
            }
        },
        remotegmdirs   => [ '*' ]
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
        ports      => {
             '*' => {  # process name
                 '*' => ''    # port -> port status
             }
        },
        gm_alive      => '',
        localusers => {
            '*' => {
                gridareas => [ '' ],
                diskfree => '' # unit: MB
            }
        },
   EMIversion => [ '' ] # taken from /etc/emi-version if exists
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

# Obtain the end date of a certificate (in seconds since the epoch)
sub enddate {
    my ($openssl, $certfile) = @_;
    # assuming here that the file exists and is a well-formed certificate.
    my $stdout =`$openssl x509 -noout -enddate -in '$certfile' 2>&1`;
    if ($?) {
       $log->info("openssl error: $stdout");
       return undef;
    }
    chomp ($stdout);


    my %mon = (Jan=>0,Feb=>1,Mar=>2,Apr=>3,May=>4,Jun=>5,Jul=>6,Aug=>7,Sep=>8,Oct=>9,Nov=>10,Dec=>11);
    if ($stdout =~ m/notAfter=(\w{3})  ?(\d\d?) (\d\d):(\d\d):(\d\d) (\d{4}) GMT/ and exists $mon{$1}) {
        return timegm($5,$4,$3,$2,$mon{$1},$6);
    } else {
        $log->warning("Unexpected -enddate from openssl for $certfile");
        return undef;
    }
}

sub get_ports_info {
    my (@processes, %ports) = @_;
    
    # check if to use either netstat or ss
    my $netcommand = '';
    for my $path (split ':', "$ENV{PATH}") {
        $netcommand = "$path/netstat" and last if -x "$path/netstat";
        $netcommand = "$path/ss" and last if -x "$path/ss";
    }
    
    if ($netcommand eq '') {
       $log->verbose("Could not find neither netstat nor ss command, cannot probe open ports, assuming services are up");
       # TODO: set all port statuses to ok and return
    }
    
    # run net command
    my $stdout ||= `$netcommand -antup 2>&1`;
    if ($?) {
       $log->info("$netcommand error: $stdout");
       return undef;
    }
    chomp ($stdout);

    $log->debug("$stdout");
    
    foreach my $process (@processes) {
       # TODO: scan ports
       #foreach my $port
    }
    #if ($> == 0) {
        #my $netstat=`netstat -antup`;
        #if ( $? != 0 ) {
        ## push @{$healthissues{unknown}}, "Checking if ARC WS interface is running: error in executing netstat. Infosys will assume the service is in ok HealthState";
        #$log->verbose("Checking if ARC WS interface is running: error in executing netstat. Infosys will assume AREX WSRF/XBES running properly");
        #} else {
             ## searches if arched is listed in netstat output
             ## best way would be ask arched if its service is up...?
             #if( $netstat !~ m/arched/ ) {
                 #push @{$healthissues{critical}}, "arched A-REX endpoint not found with netstat" ;
              #}
        #}
    #} else {
        ## push @{$healthissues{unknown}}, "user ".getpwuid($>)." cannot run netstat -p. Infosys will assume the service is in ok HealthState";
        #$log->verbose("Checking if ARC WS interface is running: user ".getpwuid($>)." cannot run netstat -p. Infosys will assume AREX WSRF/XBES is running properly");
    #}

}

# Hostcert, issuer CA, trustedca, issuercahash, enddate ...
sub get_cert_info {
    my ($options, $globusloc) = @_;

    my $host_info = {};

    if (not $options->{x509_host_cert}) {
        $log->info("x509_host_cert not configured");
        return $host_info;
    }

    # find an openssl
    my $openssl = '';
    for my $path (split ':', "$ENV{PATH}:$globusloc/bin") {
        $openssl = "$path/openssl" and last if -x "$path/openssl";
    }
    $log->error("Could not find openssl command") unless $openssl;

    # Inspect host certificate
    my $hostcert = $options->{x509_host_cert};
    chomp (my $issuerca = `$openssl x509 -noout -issuer -nameopt oneline -in '$hostcert' 2>&1`);
    if ($?) {
        $log->warning("Failed processing host certificate file: $hostcert, openssl error: $issuerca") if $?;
    } else {
        $issuerca =~ s/, /\//g;
        $issuerca =~ s/ = /=/g;
        $issuerca =~ s/^[^=]*= */\//;
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
       chomp (my $ca_sn = `$openssl x509 -checkend 3600 -noout -subject -nameopt oneline -in '$cert'`);
       my $is_expired = $?;
       $ca_sn = (split(/\n/, $ca_sn))[0];
       $ca_sn =~ s/, /\//g;
       $ca_sn =~ s/ = /=/g;
       $ca_sn =~ s/^[^=]*= */\//;
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


# Returns 'all'  if all grid-managers are up
#         'some' if one or more grid-managers are down
#         'none' if all grid-managers are down
sub gm_alive {
    my ($timeout, @controldirs) = @_;

    my $up = 0;
    my $down = 0;
    for my $dir (@controldirs) {
        my @stat = stat("$dir/gm-heartbeat");
        if (@stat and time() - $stat[9] < $timeout) {
            $up++;
        } else {
            $down++;
        }
    }
    return 'none' if not $up;
    return $down ? 'some' : 'all';
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

    my @controldirs;
    my $control = $options->{control};
    push @controldirs, $_->{controldir} for values %$control;

    # Considering only common session disk space (not including per-user session directoires)
    my (%commongridareas, $commonfree);
    if ($control->{'.'}) {
        $commongridareas{$_} = 1 for map { my ($path, $drain) = split /\s+/, $_; $path; } @{$control->{'.'}{sessiondir}};
    }
    # Also include remote session directoires.
    if (my $remotes = $options->{remotegmdirs}) {
        for my $remote (@$remotes) {
            my ($ctrldir, @sessions) = split ' ', $remote;
            $commongridareas{$_} = 1 for grep { $_ ne 'drain' } @sessions;
            push @controldirs, $ctrldir;
        }
    }
    if (%commongridareas) {
        my %res = Sysinfo::diskspaces(keys %commongridareas);
        if ($res{errors}) {
            $log->warning("Failed checking disk space available in session directories");
        } else {
            $host_info->{session_free} = $commonfree = $res{freesum};
            $host_info->{session_total} = $res{totalsum};
        }
    }

    # calculate free space on the sessionsirs of each local user.
    my $user = $host_info->{localusers} = {};

    foreach my $u (@{$options->{localusers}}) {

        # Are there grid-manager settings applying for this local user?
        if ($control->{$u}) {
            my $sessiondirs = [ map { my ($path, $drain) = split /\s+/, $_; $path; } @{$control->{$u}{sessiondir}} ];
            my %res = Sysinfo::diskspaces(@$sessiondirs);
            if ($res{errors}) {
                $log->warning("Failed checking disk space available in session directories of user $u")
            } else {
                $user->{$u}{gridareas} = $sessiondirs;
                $user->{$u}{diskfree} = $res{freesum};
            }
        } elsif (defined $commonfree) {
            # default for other users
            $user->{$u}{gridareas} = [ keys %commongridareas ];
            $user->{$u}{diskfree} = $commonfree;
        }
    }

    # Considering only common cache disk space (not including per-user caches)
    if ($control->{'.'}) {
        my $cachedirs = $control->{'.'}{cachedir} || [];
        my $remotecachedirs = $control->{'.'}{remotecachedir} || [];
        my @paths = map { my @pair = split " ", $_; $pair[0] } @$cachedirs, @$remotecachedirs;
        if (@paths) {
            my %res = Sysinfo::diskspaces(@paths);
            if ($res{errors}) {
                $log->warning("Failed checking disk space available in common cache directories")
            } else {
                # What to publish as CacheFree if there are multiple cache disks?
                # Should be highWatermark factored in?
                # Opting to publish the least free space on any of the cache
                # disks -- at least this has a simple meaning and is useful to
                # diagnose if a disk gets full.
                $host_info->{cache_free} = $res{freemin};
                # Only accurate if caches are on filesystems of their own
                $host_info->{cache_total} = $res{totalsum};
            }
        }
    }

    my $gm_timeout = $options->{wakeupperiod}
                   ? $options->{wakeupperiod} * 10
                   : 1800;
    $host_info->{gm_alive} = gm_alive($gm_timeout, @controldirs);

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

    $host_info->{ports} = get_ports_info(@{$options->{processes}},%{$options->{ports}});

    # gets EMI version from /etc/emi-version if any.
    my $EMIversion;
     if  (-r "/etc/emi-version") {
       chomp ( $EMIversion = `cat /etc/emi-version 2>/dev/null`);
       if ($?) { $log->warning("Failed reading EMI version file. Assuming non-EMI deployment. Install emi-version package if you're running EMI version of ARC")}
     }
     $host_info->{EMIversion} = [ 'MiddlewareName=EMI' , "MiddlewareVersion=$EMIversion" ] if ($EMIversion);

    return $host_info;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $options = { x509_host_cert => '/etc/grid-security/hostcert.pem',
                    x509_cert_dir => '/etc/grid-security/certificates',
                    control => {
                        '.' => {
                            sessiondir => [ '/home', '/boot' ],
                            cachedir => [ '/home' ],
                            remotecachedir => [ '/boot' ],
                            cachesize => '60 80',
                        },
                        'daemon' => {
                            sessiondir => [ '/home', '/tmp' ],
                        }
                    },
                    remotegmdirs => [ '/dummy/control /home',
                                      '/dummy/control /boot' ],
                    libexecdir => '/usr/libexec/arc',
                    runtimedir => '/home/grid/runtime',
                    processes => [ qw(bash ps init grid-manager bogous) ],
                    ports => {
                        ssh => ['22'],
                        gridftpd => ['3811'],
                        slapd => ['2135']
                    },
                    localusers => [ qw(root bin daemon) ] };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils::level('DEBUG');
    $log->debug("Options:\n" . Dumper($options));
    my $results = HostInfo::collect($options);
    $log->debug("Results:\n" . Dumper($results));
}

#test;

1;
