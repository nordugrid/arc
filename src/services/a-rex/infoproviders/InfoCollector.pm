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
# Derived classes should override at least _initialize() and _collect()

use LogUtils;
use InfoChecker;

use strict;

sub new($) {
    my ($this) = @_;
    my $class = ref($this) || $this;
    my $self = {};
    bless $self, $class;
    $self->_initialize();
    return $self;
}

sub _initialize($) {
    my ($self) = @_;
    $self->{logger} = LogUtils->getLogger(ref($self));
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
    my $schema = $self->_get_options_schema();
    my @messages = InfoChecker->new($schema)->verify($options);
    $log->error("Checker: options$_") foreach @messages;
    $log->error("Checker: ".@messages." required options are missing") and die if @messages;
}

sub _check_results($) {
    my ($self,$results) = @_;
    my $log = $self->{logger};
    my $schema = $self->_get_results_schema();
    my @messages = InfoChecker->new($schema)->verify($results,1);
    $log->warning("Checker: results$_") foreach @messages;
}


1;
