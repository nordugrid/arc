package HostInfo;

use File::Basename;
use lib dirname($0);

use base InfoCollector;
use LogUtils;

use strict;

our $host_options_schema = {
        x509_user_cert => '',
        x509_cert_dir => '',
        sessiondir => '',
        cachedir => '*',
        ng_location => '',
        runtimedir => '*',
        processes => [ '' ],
        localusers => [ '' ]
};

our $host_info_schema = {
        hostname => '',
        architecture => '',
        cpumodel => '',
        cpufreq => '',
        architecture => '',
        issuerca => '',
        issuerca_hash => '',
        trustedcas => [ '' ],
        session_free => '',
        session_total => '',
        cache_free => '',
        cache_total => '',
        ngversion => '',
        globusversion => '',
        runtimeenvironments => [ '' ],
        processes => { '*' => '' },
        localusers => {
            '*' => {
                gridarea => '',
                diskfree => ''
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

sub diskspace ($) {
    my $path = shift;
    my $space;
    if ( -d "$path") {
        # check if on afs
        if ($path =~ /\/afs\//) {
            $space =`fs listquota $path 2>/dev/null`;
            if ($? != 0) {
                $log->warning("Failed checking diskspace for $path");
            }
            if ($space) {
                $space =~ /\n\S+\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/;
                $space = int (($1 - $2)/1024);
            }
            # "ordinary" disk
        } else {
            $space =`df -k -P $path 2>/dev/null`;
            if ($? != 0) {
                $log->warning("Failed checking diskspace for $path");
            }
            if ($space) {
                $space =~ /\n\S+\s+\d+\s+\d+\s+(\d+)\s+\d+/;
                $space=int $1/1024;
            }
        }
    } else {
        $log->error("sessiondir $path was not found");
    }
    return $space;
}


sub grid_diskspace ($$) {
    my ($sessiondir, $localids) = @_;
    my $commonspace = diskspace($sessiondir);

    my $users = {};
    foreach my $u (@$localids) {
        $users->{$u} = {};

        my ($name,$passwd,$uid,$gid,$quota,$comment,$gcos,$home,$shell,$expire) = getpwnam($u);
        $log->error("User $u is not listed in the passwd file") unless $name;

        if ($sessiondir =~ /^\s*\*\s*$/) {
            $users->{$u}{gridarea} = $home."/.jobs";
            $users->{$u}{diskfree} = diskspace($users->{$u}{gridarea});
        } else {
            $users->{$u}{gridarea} = $sessiondir;
            $users->{$u}{diskfree} = $commonspace;
        }
    }
    return $users;
}

# return the PIDs of named processes, or zero if not running

sub process_status (@) {
    my $ids = {};
    foreach my $name ( @_ ) {
        my ($id) = `pgrep $name`;
        if ( $? != 0 ) {
            ( my ($id) = `ps -C $name --no-heading`) =~ s/(\d+).*/$1/;
            $log->warning("Failed checking process $name") if $?;
        }
        chomp( $ids->{$name} = $id ? $id : 0 );
    }
    return $ids;
}



sub get_host_info {
    my $options = shift;

    my $host_info = {};

    ############################################################
    # Lot's of stuff that just need to be figured out
    # Scripting, scripting, scripting, oh hay, oh hay...
    ############################################################

    chomp( $host_info->{hostname} = `/bin/hostname -f`);
    $log->warning("Failed running hostname") if $?;
    chomp( $host_info->{architecture} = `uname -m`);
    $log->warning("Failed running uname") if $?;

     open (CPUINFO, "</proc/cpuinfo")
         or $log->error("error in opening /proc/cpuinfo");
     while ( my $line = <CPUINFO> ) {
         if ($line=~/^model name\s+:\s+(.*)$/) {
             $host_info->{cpumodel} = $1;
         }
         if ($line=~/^cpu MHz\s+:\s+(.*)$/) {
             $host_info->{cpufreq} = $1;
         }
     }
     close CPUINFO;
    
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
    my $session_diskfree  = 0;
    my $session_disktotal = 0;
    if ( -d "$options->{sessiondir}") {
        # afs  
        if ($options->{sessiondir} =~ /\/afs\//) {
    	$session_diskfree =`fs listquota $options->{sessiondir} 2>/dev/null`;
    	if ($session_diskfree) {
    	    $session_diskfree =~ /\n\S+\s+(\d+)\s+(\d+)\s+\d+%\s+\d+%/;
    	    $session_diskfree = int (($1 - $2)/1024);
    	    $session_disktotal= int $1/1024;
    	} 
        } 
        # "ordinary" disk 
        else {
    	my (@dfstring) =`df -k -P $options->{sessiondir} 2>/dev/null`;
    	if ($dfstring[1]) {
    	    my ( $filesystem, $onekblocks, $used, $available, $usepercentage,
    		 $mounted ) = split " ", $dfstring[1];
    	    $session_diskfree = int $available/1024;
    	    $session_disktotal = int $onekblocks/1024;
    	}
        }
    }
    else {
        $log->warning("no sessiondir found");
    }
    $host_info->{session_free} = $session_diskfree;
    $host_info->{session_total} = $session_disktotal;
        
    #nordugrid-cluster-cacheusage
    my $cache_statfile = "statistics";
    my ($cache_disktotal, $cache_diskfree);
    if ( $options->{cachedir} ){
        (my $path1, my $path2) = split /\s+/, $options->{cachedir};
        my ($dfstring) = join '', `cat $path1/$cache_statfile 2>/dev/null`;
        if ( $? ) {
    	$log->warning("error in reading the cache status file $path1/$cache_statfile");
        } 
        $dfstring =~ /softfree=(\d+)/;
        $cache_diskfree = int $1/1024/1024;
        $dfstring =~ /unclaimed=(\d+)/;
        $cache_diskfree += int $1/1024/1024;
        $dfstring =~ /softsize=(\d+)/;
        $cache_disktotal = int $1/1024/1024;
    }
    $host_info->{cache_free} = $cache_diskfree;
    $host_info->{cache_total} = $cache_disktotal;
    
    #NorduGrid middleware
    #nordugridlocation/bin/grid-manager -v
    $ENV{"LD_LIBRARY_PATH"}="$globus_location/lib:$options->{ng_location}/lib";
    my ($ngversion) = `$options->{ng_location}/sbin/grid-manager -v 2>/dev/null`;
    if ($?) { $log->warning("Can't execute the $options->{ng_location}/sbin/grid-manager")}
    $ngversion =~ s/grid-manager: version\s*(.+)$/$1/;
    $host_info->{ngversion} = $ngversion;
    chomp $host_info->{ngversion};
    
    #Globus Toolkit version
    #globuslocation/share/doc/VERSION
    my $globusversion;
    if (-r "$globus_location/share/doc/VERSION" ) {
       chomp ( $globusversion =  `cat $globus_location/share/doc/VERSION 2>/dev/null`);
       if ($?) { $log->warning("error in reading the globus version file")}   
    }
    #globuslocation/bin/globus-version
    elsif (-x "$globus_location/bin/globus-version" ) {
       chomp ( $globusversion =  `$globus_location/bin/globus-version 2>/dev/null`);
       if ($?) { $log->warning("error in executing the $globus_location/bin/globus-version command")}
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

sub test {
    my $options = { x509_user_cert => '/etc/grid-security/hostcert.pem',
                    x509_cert_dir => '/etc/grid-security/certificates',
                    sessiondir => '/home/grid/session/',
                    cachedir => '/home/grid/cache',
                    ng_location => '/opt/nordugrid',
                    runtimedir => '/home/grid/runtime',
                    processes => [ qw(bash init grid-manager bogous) ],
                    localusers => [ qw(root adrianta) ] };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils->getLogger()->level($LogUtils::DEBUG);
    $log->debug("Options:\n" . Dumper($options));
    my $results = HostInfo->new()->get_info($options);
    $log->debug("Results:\n" . Dumper($results));
} 

#test;


1;
