package JanitorRuntimes;

use warnings;
use strict;

use Janitor::Janitor qw(process);

sub list {
    my ($rtes) = @_;

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
    return ( $result{errorcode}, $result{message} );
}

1;
