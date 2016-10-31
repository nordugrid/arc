package RTEInfo;

use warnings;
use strict;

use LogUtils;

our $rte_options_schema = {
        pgkdatadir     => '',
        configfile     => '',
        runtimedir     => '*',
};

our $rte_info_schema = {
        '*' => {
                   state       => '',
                   description => '*',
               }
};

our $log = LogUtils->getLogger(__PACKAGE__);

sub collect($) {
    my ($options) = @_;

    my $rtes = {};

    # system RTEs
    add_static_rtes("$options->{pkgdatadir}/rte/", $rtes) if $options->{pkgdatadir};

    # user-defined RTEs
    add_static_rtes($options->{runtimedir}, $rtes) if $options->{runtimedir};

    return $rtes;
}


sub add_static_rtes {
    my ($runtimedir, $rtes) = @_;
    unless (opendir DIR, $runtimedir) {
       $log->warning("Can't access runtimedir: $runtimedir: $!");
       return;
    }
    closedir DIR;
    my $cmd = "find '$runtimedir' -type f ! -name '.*' ! -name '*~'";
    unless (open RTE, "$cmd |") {
       $log->warning("Failed to run: $cmd");
       return;
    }
    while (my $dir = <RTE>) {
       chomp $dir;
       $dir =~ s#$runtimedir/*##;
       $rtes->{$dir} = { state => 'installednotverified' };
    }
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $options = { pkgdatadir => '/scratch/adrianta/arc1/share/arc',
                    runtimedir => '/data/export/SOFTWARE/runtime',
                    configfile => '/etc/arc.conf',
    };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils::level('VERBOSE');
    $log->debug("Options:\n" . Dumper($options));
    my $results = RTEInfo::collect($options);
    $log->debug("Results:\n" . Dumper($results));
}

#test;

1;
