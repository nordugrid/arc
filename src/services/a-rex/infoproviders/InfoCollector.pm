package InfoCollector;

# Base class for information collectors

# Public interface:
#   my $collector = InfoCollector->new($options);
#   $results = $collector->get_info();
# 
# Implements validation of input ($options) and output ($results) using the
# InfoChecker module. $options and $results are meant to be references to hashes
# (of hashes, arrays ...)
#
# Derived classes should override at least _collect(), _get_options_schema()
# and _get_results_schema().

use LogUtils;
use InfoChecker;

use strict;

sub new($) {
    my $this = shift;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->_initialize();
    return $self;
}

sub _initialize($) {
    my ($self) = @_;
    $self->{logger} = LogUtils->getLogger(ref($self));
    $self->{loglevel} = $LogUtils::WARNING;
    $self->{optname} = "options";
    $self->{resname} = "results";
}

sub get_info($$) {
    my ($self,$options) = @_;

    $self->_check_options($options);

    my $results = $self->_collect($options);

    $self->_check_results($results);

    return $results;
}

# set proper schemas here

sub _get_options_schema($) {
    return {};
}
sub _get_results_schema($) {
    return {};
}

# actual information collection should go here

sub _collect($$) {
    my ($self,$options) = @_;
    my $results = {};
    return $results;
}


sub _check_options($) {
    my ($self,$options) = @_;
    my $log = $self->{logger};
    my $level = $self->{loglevel};
    my $what = $self->{optname};
    my $schema = $self->_get_options_schema();
    my @messages = InfoChecker->new($schema)->verify($options);
    $log->log($level,"Checker: $what->$_") foreach @messages;
    $log->error("Checker: ".@messages." required options are missing") if @messages;
}

sub _check_results($) {
    my ($self,$results) = @_;
    my $log = $self->{logger};
    my $level = $self->{loglevel};
    my $what = $self->{resname};
    my $schema = $self->_get_results_schema();
    my @messages = InfoChecker->new($schema)->verify($results,1);
    $log->log($level,"Checker: $what->$_") foreach @messages;
}


1;
