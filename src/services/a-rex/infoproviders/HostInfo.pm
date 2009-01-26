package HostInfo;

use Sys::Hostname;

use base InfoCollector;
use Sysinfo qw(cpuinfo processid diskinfo diskspaces);
use LogUtils;

use strict;

our $host_options_schema = {
        x509_user_cert => '',
        x509_cert_dir  => '',
        sessiondir     => '',
        cachedir       => '*',
        ng_location    => '',
        runtimedir     => '*',
        processes      => [ '' ],
        localusers     => [ '' ]
};

our $host_info_schema = {
        hostname      => '',
        architecture  => '',
        cpumodel      => '',
        cpufreq       => '', # unit: MHz
        nodecpu       => '',
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


# override InfoCollector base class methods 

sub _get_options_schema {
    return $host_options_schema;
}
sub _get_results_schema {
    return $host_info_schema;
}
sub _collect($$) {
    my ($self,$options) = @_;
    return get_host_info($options);
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


sub get_host_info {
    my $options = shift;

    my $host_info = {};

    ############################################################
    # Lot's of stuff that just need to be figured out
    # Scripting, scripting, scripting, oh hay, oh hay...
    ############################################################

    chomp( $host_info->{hostname} = hostname());
    chomp( $host_info->{architecture} = `uname -m`);
    $log->warning("Failed running uname") if $?;

    my $cpuinfo = cpuinfo();
    if ($cpuinfo) {
        $host_info->{cpumodel} = $cpuinfo->{cpumodel};
        $host_info->{cpufreq} = $cpuinfo->{cpufreq};
    } else {
        $log->error("Failed querying CPU info");
    }


    if ($host_info->{cpufreq} and $host_info->{cpumodel}) {
        $host_info->{nodecpu} = "$host_info->{cpumodel} @ $host_info->{cpufreq} Mhz";
    }
    
    # Globus location
    my $globus_location ||= $ENV{GLOBUS_LOCATION} ||= "/opt/globus/";
    $ENV{"LD_LIBRARY_PATH"}="$globus_location/lib";
    
    # Hostcert issuer CA, trustedca, issuercahash
    # find an openssl							          
    my $openssl_command = `which openssl 2>/dev/null`;
    chomp $openssl_command;	
    if ($? != 0) {
       $openssl_command = "$globus_location/bin/openssl";
    }
    my $issuerca=`$openssl_command x509 -noout -issuer -in $options->{x509_user_cert} 2>/dev/null`;    
    if ( $? ) {
        $log->error("error in executing $openssl_command x509 -noout -issuer -in $options->{x509_user_cert}");
    }
    if ( $issuerca =~ s/issuer= // ) {
        chomp($issuerca);
    }
    $host_info->{issuerca} = $issuerca;

    my @certfiles = `ls $options->{x509_cert_dir}/*.0 2>/dev/null`;
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
    
    #nordugrid-cluster-sessiondirusage
    my $sessionspace = diskinfo($options->{sessiondir});
    $log->warning("Failed checking disk space in sessiondir $options->{sessiondir}")
        unless $sessionspace;
    $host_info->{session_free} = $sessionspace->{megsfree} || 0;
    $host_info->{session_total} = $sessionspace->{megstotal} || 0;

    #nordugrid-cluster-cacheusage
    #OBS: only accurate if cache is on a filesystem of it's own 
    my @paths = split /\[separator\]/, ($options->{cachedir} || '');
    @paths = map { my @pair = split " ", $_; $pair[0] } @paths;
    my %cacheinfo = diskspaces(@paths);
    $log->warning("Failed checking disk space in cache directory $options->{cachedir}")
        unless %cacheinfo;
    $host_info->{cache_total} = $cacheinfo{totalsum} || 0;
    $host_info->{cache_free} = ($cacheinfo{freemin} || 0) * $cacheinfo{ndisks};

    #NorduGrid middleware version
    #Skipped...
    
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
    
    #nordugrid-cluster-runtimeenvironment
    my @runtimeenvironment;
    if ($options->{runtimedir}){
       if (opendir DIR, $options->{runtimedir}) {
          @runtimeenvironment = `find $options->{runtimedir} -type f ! -name ".*" ! -name "*~"` ;
          closedir DIR;
          foreach my $listentry (@runtimeenvironment) {
             chomp($listentry);
             $listentry=~s/$options->{runtimedir}\/*//;
          }
       } else {
         # $listentry="";
          $log->warning("Can't acess runtimedir: $options->{runtimedir}");
       }
    }
    $host_info->{runtimeenvironments} = \@runtimeenvironment;

    # users and diskspace
    $host_info->{localusers} = grid_diskspace($options->{sessiondir}, $options->{localusers});
    $host_info->{processes} = processid(@{$options->{processes}});

    return $host_info;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $options = { x509_user_cert => '/etc/grid-security/hostcert.pem',
                    x509_cert_dir => '/etc/grid-security/certificates',
                    sessiondir => '/home/grid/session/',
                    cachedir => '/home/grid/cache',
                    ng_location => '/opt/nordugrid',
                    runtimedir => '/home/grid/runtime',
                    processes => [ qw(bash ps init grid-manager bogous) ],
                    localusers => [ qw(root adrianta) ] };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils->getLogger()->level($LogUtils::DEBUG);
    $log->debug("Options:\n" . Dumper($options));
    my $results = HostInfo->new()->get_info($options);
    $log->debug("Results:\n" . Dumper($results));
} 

#test;


1;
