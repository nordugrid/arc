package HostInfo;

use Sys::Hostname;
use File::Basename;
use lib dirname($0);

use base InfoCollector;
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
        ngversion     => '',
        globusversion => '',
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

#
# Returns disk space (total and free) in MB on a filesystem
#
sub diskspace ($) {
    my $path = shift;
    my ($diskfree, $disktotal);

    if ( -d "$path") {
        # check if on afs
        if ($path =~ m#/afs/#) {
            my @dfstring =`fs listquota $path 2>/dev/null`;
            if ($? != 0) {
                $log->warning("Failed running: fs listquota $path");
            } elsif ($dfstring[1] =~ /^\s*\S+\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/) {
                $disktotal = int $1/1024;
                $diskfree  = int(($1 - $2)/1024);
            } else {
                $log->warning("Failed interpreting output of: fs listquota $path");
            }
        # "ordinary" disk
        } else {
            my @dfstring =`df -k -P $path 2>/dev/null`;
            if ($? != 0) {
                $log->warning("Failed running: df -k -P $path");
            } elsif ($dfstring[1] =~ /^\s*\S+\s+(\d+)\s+\d+\s+(\d+)\s+\d+%/) {
    	        $disktotal = int $1/1024;
    	        $diskfree  = int $2/1024;
            } else {
                $log->warning("Failed interpreting output of: df -k -P $path");
            }
        }
    } else {
        $log->warning("Not a directory: $path");
    }

    return undef unless defined($disktotal) and defined($diskfree);
    return {megstotal => $disktotal, megsfree => $diskfree};
}


sub grid_diskspace ($$) {
    my ($sessiondir, $localids) = @_;
    my $commonspace = diskspace($sessiondir);
    $log->error("Failed checking disk space in sessiondir $sessiondir")
         unless $commonspace;

    my $users = {};
    foreach my $u (@$localids) {
        $users->{$u} = {};

        my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$home,$shell,$expire) = getpwnam($u);
        $log->error("User $u is not listed in the passwd file") unless $name;

        if ($sessiondir =~ /^\s*\*\s*$/) {
            $users->{$u}{gridarea} = $home."/.jobs";
            my $gridspace = diskspace($users->{$u}{gridarea}); 
            $log->error("Failed checking disk space in personal gridarea $users->{$u}{gridarea}")
                unless $gridspace;
            $users->{$u}{diskfree} = $gridspace->{megsfree} || 0;
        } else {
            $users->{$u}{gridarea} = $sessiondir;
            $users->{$u}{diskfree} = $commonspace->{megsfree} || 0;
        }
    }
    return $users;
}

# Return PIDs of named commands owned by the current user
# Only one pid is returned per command name

sub process_status (@) {

    my @procs = `ps -u $< -o pid,comm 2>/dev/null`;
    if ( $? != 0 ) {
        $log->info("Failed running: ps -u $< -o pid,comm");
        $log->warning("Failed listing processes");
        return {};
    }
    shift @procs; # throw away header line

    # make hash of comm => pid
    my %all;
    /\s*(\d+)\s+(.+)/ and $all{$2} = $1 for @procs;

    my %pids;
    foreach my $name ( @_ ) {
        $pids{$name} = $all{$name} if $all{$name};
    }
    return \%pids;
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

    open (CPUINFO, "</proc/cpuinfo")
        or $log->error("error in opening /proc/cpuinfo");
    while ( my $line = <CPUINFO> ) {
        if ($line=~/^model name\s+:\s+(.*)$/) {
            $host_info->{cpumodel} = $1;
        }
        if ($line=~/^cpu MHz\s+:\s+(.*)$/) {
            $host_info->{cpufreq} = int $1;
        }
    }
    close CPUINFO;

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
    my $sessionspace = diskspace($options->{sessiondir});
    $log->error("Failed checking disk space in sessiondir $options->{sessiondir}")
        unless $sessionspace;
    $host_info->{session_free} = $sessionspace->{megsfree} || 0;
    $host_info->{session_total} = $sessionspace->{megstotal} || 0;
        
    #nordugrid-cluster-cacheusage
    my $cachespace;
    if ( $options->{cachedir} ){
        my ($cachedir) = split /\s+/, $options->{cachedir};
        my $cachespace = diskspace($cachedir);
    }
    #OBS: only accurate if cache is on a filesystem of it's own 
    #TODO: find a way to get better numbers from new cache system
    $host_info->{cache_free} = $cachespace->{megsfree} || 0;
    $host_info->{cache_total} = $cachespace->{megstotal} || 0;
    
    #NorduGrid middleware
    #nordugridlocation/bin/grid-manager -v
    $ENV{"LD_LIBRARY_PATH"}="$globus_location/lib:$options->{ng_location}/lib";
    my ($ngversion) = `$options->{ng_location}/sbin/grid-manager -v 2>/dev/null`;
    if ($?) {
        $log->warning("Failed running $options->{ng_location}/sbin/grid-manager");
    } else {
        $ngversion =~ s/grid-manager: version\s*(.+)$/$1/;
        $host_info->{ngversion} = $ngversion;
        chomp $host_info->{ngversion};
    }
    
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
    $host_info->{processes} = process_status(@{$options->{processes}});

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
