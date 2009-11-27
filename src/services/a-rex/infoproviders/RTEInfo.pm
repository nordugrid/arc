package RTEInfo;

use warnings;
use strict;

use LogUtils;

our $rte_options_schema = {
        pgkdatadir     => '',
        configfile     => '',
        runtimedir     => '*',
        JanitorEnabled => '*',
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

    return {} unless $options->{runtimedir};

    my $rtes = {};
    add_static_rtes($options->{runtimedir}, $rtes);
    add_janitor_res($options, $rtes);

    return $rtes;
}


sub add_static_rtes {
    my ($runtimedir, $rtes) = @_;
    unless (opendir DIR, $runtimedir) {
       $log->warning("Can't acess runtimedir: $runtimedir: $!");
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


# Janitor uses ARC_CONFIG env var to find the config file
{   our $arc_config;
    sub save_arc_conf {
        $arc_config = $ENV{ARC_CONFIG} if defined $ENV{ARC_CONFIG};
    }
    sub restore_arc_conf {
        $ENV{ARC_CONFIG} = $arc_config if defined $arc_config;
        delete $ENV{ARC_CONFIG}    unless defined $arc_config;
    }
}


sub add_janitor_res {
    my ($options, $rtes) = @_;
    return unless $options->{JanitorEnabled};

    my $jmodpath = $options->{pkgdatadir}.'/perl';
    if (! -e "$jmodpath/Janitor/Janitor.pm") {
       $log->warning("Janitor modules not found in '$jmodpath'");
       return;
    }

    save_arc_conf();
    $ENV{ARC_CONFIG} = $options->{configfile};

    eval { unshift @INC, $jmodpath;
           require JanitorRuntimes;
           import JanitorRuntimes qw(process);
    };
    if ($@) {
        $log->warning("Failed to load Janitor modules: $@");
        restore_arc_conf();
        return;
    }

    JanitorRuntimes::list($rtes);

    # Hide unusable runtimes
    for my $rte (keys %$rtes) {
        my $state = $rtes->{$rte}{state};
        next if $state eq "installednotverified";
        next if $state eq "installedverified";
        next if $state eq "installable";
        next if $state eq "pendingremoval";
        # installationfailed
        # installedbroken
        # pendingremoval
        # UNDEFINEDVALUE
        delete $rtes->{$rte};
    }

    restore_arc_conf();
}



#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test {
    my $options = { pkgdatadir => '/scratch/adrianta/arc1/share/arc',
                    runtimedir => '/data/export/SOFTWARE/runtime',
                    configfile => '/etc/arc.conf',
                    JanitorEnabled => 1,
    };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils::level('VERBOSE');
    $log->debug("Options:\n" . Dumper($options));
    my $results = RTEInfo::collect($options);
    $log->debug("Results:\n" . Dumper($results));
}

#test;

1;
