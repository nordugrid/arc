package RTEInfo;

use warnings;
use strict;

use LogUtils;

our $rte_options_schema = {
        controldir     => '',
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

    # enabled RTEs in controldir
    add_static_rtes("$options->{controldir}/rte/enabled/", $rtes) if $options->{controldir};

    return $rtes;
}


sub add_static_rtes {
    my ($runtimedir, $rtes) = @_;
    unless (opendir DIR, $runtimedir) {
       $log->debug("Can't access runtimedir: $runtimedir: $!");
       return;
    }
    closedir DIR;
    my $cmd = "find '$runtimedir' ! -type d ! -name '.*' ! -name '*~' -exec test -e {} \\; -print";
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
    my $options = { 
        controldir => '/var/spool/nordugrid',
    };
    require Data::Dumper; import Data::Dumper qw(Dumper);
    LogUtils::level('VERBOSE');
    $log->debug("Options:\n" . Dumper($options));
    my $results = RTEInfo::collect($options);
    $log->debug("Results:\n" . Dumper($results));
}

#test;

1;
