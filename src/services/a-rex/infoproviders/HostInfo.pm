package HostInfo;

use POSIX;
use Sys::Hostname;

use Sysinfo qw(cpuinfo meminfo processid diskinfo diskspaces);
use LogUtils;
use InfoChecker;

use strict;

our $host_options_schema = {
        x509_user_cert => '',
        x509_cert_dir  => '',
        sessiondir     => [ '' ],
        cachedir       => [ '*' ],
        libexecdir     => '',
        runtimeDir     => '*',
        processes      => [ '' ],
        localusers     => [ '' ]
};

our $host_info_schema = {
        hostname      => '',
        sysname       => '',
        machine       => '',
        cpumodel      => '*',
        cpufreq       => '*', # unit: MHz
        cpustepping   => '*',
        cpuvendor     => '*',
        pmem          => '*', # unit: MB
        vmem          => '*', # unit: MB
        cputhreadcount=> '*',
        cpucorecount  => '*',
        cpusocketcount=> '*',
        issuerca      => '',
        issuerca_hash => '',
        trustedcas    => [ '' ],
        session_free  => '', # unit: MB
        session_total => '', # unit: MB
        cache_free    => '', # unit: MB
        cache_total   => '', # unit: MB
        globusversion => '*',
        runtimeenvironments => [ '' ],
        processes  => { '*' => '' },
        localusers => {
            '*' => {
                gridarea => '',
                diskfree => '' # unit: MB
            }
        }
};

our $log = LogUtils->getLogger(__PACKAGE__);

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
    $log->warning("SelfCheck: result key hostinfo->$_") foreach @messages;

    return $result;
}


# private subroutines

sub grid_diskspace ($$) {
    my ($sessiondir, $localids) = @_;
    my $commonspace = diskinfo($sessiondir);
    $log->warning("Failed checking disk space in sessiondir $sessiondir")
         unless $commonspace;

    my $users = {};
    foreach my $u (@$localids) {
        $users->{$u} = {};

        my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$home,$shell,$expire) = getpwnam($u);
        $log->warning("User $u is not listed in the passwd file") unless $name;

        if ($sessiondir =~ /^\s*\*\s*$/) {
            $users->{$u}{gridarea} = $home."/.jobs";
            my $gridspace = diskinfo($users->{$u}{gridarea}); 
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


sub get_host_info($) {
    my $options = shift;

    my $host_info = {};

    ############################################################
    # Lot's of stuff that just need to be figured out
    # Scripting, scripting, scripting, oh hay, oh hay...
    ############################################################

    $host_info->{hostname} = hostname();
    my ($sysname, $nodename, $release, $version, $machine) = POSIX::uname();
    $host_info->{machine} = $machine;
    $host_info->{sysname} = $sysname;

    my $cpuinfo = cpuinfo();
    if ($cpuinfo) {
        $host_info = {%$host_info, %$cpuinfo};
    } else {
        $log->error("Failed querying CPU info");
    }
    my $meminfo = meminfo();
    $host_info->{pmem} = $meminfo->{pmem} if $meminfo->{pmem};
    $host_info->{vmem} = $meminfo->{vmem} if $meminfo->{vmem};
    
    # Globus location
    my $globus_location ||= $ENV{GLOBUS_LOCATION} ||= "/opt/globus/";
    $ENV{"LD_LIBRARY_PATH"}="$globus_location/lib";
    
    # Hostcert issuer CA, trustedca, issuercahash
    # find an openssl							          
    my $openssl_command = '';
    for my $path (split ':', "$ENV{PATH}:$globus_location/bin") {
        $openssl_command = "$path/openssl" and last if -x "$path/openssl";
    }
    $log->error("Could not find openssl command") unless $openssl_command;

    my $issuerca=`$openssl_command x509 -noout -issuer -in $options->{x509_user_cert} 2>/dev/null`;    
    if ( $? ) {
        $log->error("error in executing $openssl_command x509 -noout -issuer -in $options->{x509_user_cert}");
    }
    if ( $issuerca =~ s/issuer= // ) {
        chomp($issuerca);
    }
    $host_info->{issuerca} = $issuerca;

    my @certfiles = `ls $options->{x509_cert_dir}/*.0`;
    $log->error("No cerificates found in ".$options->{x509_cert_dir}) unless @certfiles;
    my $issuerca_hash;
    my @trustedca;
    foreach my $cert ( @certfiles ) {
       my $ca_sn = `$openssl_command x509 -checkend 60 -noout -subject -in $cert`;
       $ca_sn =~ s/subject= //;
       chomp($ca_sn);
       if ( $ca_sn eq $issuerca) {
          $issuerca_hash = `$openssl_command x509 -checkend 60 -noout -hash -in $cert`;
          $host_info->{issuerca_hash} = $issuerca_hash;
          chomp $host_info->{issuerca_hash};
       }
       if ($?) {
         $log->warning("CA $ca_sn is expired");
         next;
       }
       push @trustedca, $ca_sn;
    }
    $host_info->{trustedcas} = \@trustedca;
    
    #TODO: multiple session dirs
    my @sessiondirs = @{$options->{sessiondir}};
    if (@sessiondirs) {
        my $sessionspace = diskinfo($sessiondirs[0]);
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
    my %cacheinfo = diskspaces(@paths);
    $log->warning("Failed checking disk space in cache directories: @paths")
        unless %cacheinfo;
    $host_info->{cache_total} = $cacheinfo{totalsum} || 0;
    $host_info->{cache_free} = ($cacheinfo{freemin} || 0) * $cacheinfo{ndisks};

    #Globus Toolkit version
    #globuslocation/share/doc/VERSION
    my $globusversion;
    if (-r "$globus_location/share/doc/VERSION" ) {
       chomp ( $globusversion =  `cat $globus_location/share/doc/VERSION 2>/dev/null`);
       if ($?) { $log->warning("Failed reading the globus version file")}   
    }
    #globuslocation/bin/globus-version
    elsif (-x "$globus_location/bin/globus-version" ) {
       chomp ( $globusversion =  `$globus_location/bin/globus-version 2>/dev/null`);
       if ($?) { $log->warning("Failed running $globus_location/bin/globus-version command")}
    }
    $host_info->{globusversion} = $globusversion;
    
    my @runtimeenvironment;
    my $runtimedir = $options->{runtimeDir};
    if ($runtimedir){
       if (opendir DIR, $runtimedir) {
          @runtimeenvironment = `find $runtimedir -type f ! -name ".*" ! -name "*~"` ;
          closedir DIR;
          foreach my $listentry (@runtimeenvironment) {
             chomp($listentry);
             $listentry=~s/$runtimedir\/*//;
          }
       } else {
         # $listentry="";
          $log->warning("Can't acess runtimedir: $runtimedir");
       }
       # Try to fetch something from Janitor
       # Integration with Janitor is done through calling 
       # executable beacuse I'm nor skilled enough to isolate
       # possible Janitor errors and in order not to redo 
       # all Log4Perl tricks again. Please redo it later. A.K.
       my $janitor = $options->{libexecdir}.'/janitor';
       if (! -e $janitor) {
          $log->debug("Janitor not found at '$janitor' - not using");
       } else {
          if (! open (JPIPE, "$janitor list shortlist dynamic |")) {
              $log->warning("$janitor: $!");
          } else {
              while(<JPIPE>) {
                  chomp(my $listentry = $_);
                  push @runtimeenvironment, $listentry
                      unless grep {$listentry eq $_} @runtimeenvironment;
              }
          }
       }
    }
    $host_info->{runtimeenvironments} = \@runtimeenvironment;

    $host_info->{processes} = processid(@{$options->{processes}});

    return $host_info;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $options = { x509_user_cert => '/etc/grid-security/hostcert.pem',
                    x509_cert_dir => '/etc/grid-security/certificates',
                    sessiondir => [ '/home/grid/session/' ],
                    cachedir => [ '/home/grid/cache' ],
                    libexecdir => '/opt/nordugrid/libexec/arc',
                    runtimeDir => '/home/grid/runtime',
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
