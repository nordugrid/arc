package Janitor::ArcUtils;

use warnings;
use strict;

use Janitor::Janitor qw(process);

# Janitor uses ARC_CONFIG env var to find the config file
{   our $arc_config;
    sub set_env {
        my ($configfile) = @_;
        $arc_config = $ENV{ARC_CONFIG} if defined $ENV{ARC_CONFIG};
        $ENV{ARC_CONFIG} = $configfile;
    }
    sub revert_env {
        $ENV{ARC_CONFIG} = $arc_config if defined $arc_config;
        delete $ENV{ARC_CONFIG}    unless defined $arc_config;
    }
}

sub listAll {
    my ($configfile,$rtes) = @_;

    set_env($configfile);

    my $response = &process(new Janitor::Request(Janitor::Response::LIST));
    my %result = $response->result();
    if($result{errorcode} == 0) {
        my @h;
        @h = $response->runtimeInstallable();
        if(defined $h[0]){
            for (@h) {
                my $state = $_->{supported} ? 'installable' : 'notinstallable';
                $rtes->{$_->{name}} = { description => $_->{description}, state => $state };
            }
        }
        @h = $response->runtimeLocal();
        if(defined $h[0]){
            $rtes->{$_->{name}}{state} = 'installednotverified' for @h;
        }
        @h = $response->runtimeDynamic();
        if(defined $h[0]){
            for (@h) {
                my $gstate = "UNDEFINEDVALUE";
                my $jstate = $_->{state};
                if      ($jstate eq "FAILED")          { $gstate = "installationfailed";
                } elsif ($jstate eq "INSTALLED_A")     { $gstate = "installednotverified";
                } elsif ($jstate eq "INSTALLED_M")     { $gstate = "installednotverified";
                } elsif ($jstate eq "VALIDATED")       { $gstate = "installedverified";
                } elsif ($jstate eq "BROKEN")          { $gstate = "installedbroken";
                } elsif ($jstate eq "INSTALLABLE")     { $gstate = "installable";
                } elsif ($jstate eq "REMOVAL_PENDING") { $gstate = "pendingremoval";
                } elsif ($jstate eq "UNKNOWN")         { $gstate = "UNDEFINEDVALUE";
                }
                $rtes->{$_->{name}}{state} = $gstate;
            }
        }
    }
    revert_env();
    return ( $result{errorcode}, $result{message} );
}

1;
