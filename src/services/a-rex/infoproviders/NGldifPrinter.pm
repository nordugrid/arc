package NGldifPrinter;

use strict;
use base 'LdifPrinter';

use POSIX;

sub new {
    my ($this, $handle, $ttl) = @_;
    my $self = $this->SUPER::new($handle);
    my $now = time;
    $self->{validfrom} = strftime("%Y%m%d%H%M%SZ", gmtime($now));
    $self->{validto} = strftime("%Y%m%d%H%M%SZ", gmtime($now + $ttl));
    return $self;
}

#
# Print attributes
#

sub beginGroup {
    my ($self, $name) = @_;
    $self->begin('nordugrid-info-group-name' => $name);
    $self->MdsAttributes();
    $self->attribute(objectClass => 'nordugrid-info-group');
    $self->attribute('nordugrid-info-group-name' => $name);
}

sub MdsAttributes {
    my ($self) = @_;
    $self->attribute(objectClass => 'Mds');
    $self->attribute('Mds-validfrom' => $self->{validfrom});
    $self->attribute('Mds-validto' => $self->{validto});
}

sub clusterAttributes {
    my ($self, $data) = @_;
    $self->MdsAttributes();
    $self->attribute(objectClass => 'nordugrid-cluster');
    $self->attributes($data, 'nordugrid-cluster-', qw( name
                                                       aliasname
                                                       contactstring
                                                       support
                                                       lrms-type
                                                       lrms-version
                                                       lrms-config
                                                       architecture
                                                       opsys
                                                       homogeneity
                                                       nodecpu
                                                       nodememory
                                                       totalcpus
                                                       cpudistribution
                                                       sessiondir-free
                                                       sessiondir-total
                                                       cache-free
                                                       cache-total
                                                       runtimeenvironment
                                                       localse
                                                       middleware
                                                       totaljobs
                                                       usedcpus
                                                       queuedjobs
                                                       location
                                                       owner
                                                       issuerca
                                                       nodeaccess
                                                       comment
                                                       interactive-contactstring
                                                       benchmark
                                                       sessiondir-lifetime
                                                       prelrmsqueued
                                                       issuerca-hash
                                                       trustedca
                                                       acl
                                                       credentialexpirationtime ));
}

sub queueAttributes {
    my ($self, $data) = @_;
    $self->MdsAttributes();
    $self->attribute(objectClass => 'nordugrid-queue');
    $self->attributes($data, 'nordugrid-queue-', qw( name
                                                     status
                                                     running
                                                     queued
                                                     maxrunning
                                                     maxqueuable
                                                     maxuserrun
                                                     maxcputime
                                                     mincputime
                                                     defaultcputime
                                                     schedulingpolicy
                                                     totalcpus
                                                     nodecpu
                                                     nodememory
                                                     architecture
                                                     opsys
                                                     gridrunning
                                                     gridqueued
                                                     comment
                                                     benchmark
                                                     homogeneity
                                                     prelrmsqueued
                                                     localqueued
                                                     maxwalltime
                                                     minwalltime
                                                     defaultwalltime
                                                     maxtotalcputime
                                                     acl ));
}

sub jobAttributes {
    my ($self, $data) = @_;
    $self->MdsAttributes();
    $self->attribute(objectClass => 'nordugrid-job');
    $self->attributes($data, 'nordugrid-job-', qw( globalid
                                                   globalowner
                                                   execcluster
                                                   execqueue
                                                   stdout
                                                   stderr
                                                   stdin
                                                   reqcputime
                                                   status
                                                   queuerank
                                                   comment
                                                   submissionui
                                                   submissiontime
                                                   usedcputime
                                                   usedwalltime
                                                   sessiondirerasetime
                                                   usedmem
                                                   errors
                                                   jobname
                                                   runtimeenvironment
                                                   cpucount
                                                   executionnodes
                                                   gmlog
                                                   clientsoftware
                                                   proxyexpirationtime
                                                   completiontime
                                                   exitcode
                                                   rerunable
                                                   reqwalltime ));
}

#
#sub userAttributes {
#    my ($self, $data) = @_;
#    $self->MdsAttributes();
#    $self->attribute(objectClass => 'nordugrid-authuser');
#    $self->attributes($data, 'nordugrid-authuser-', qw( name sn freecpus diskspace queuelength ));
#}


#
# Follow hierarchy
#

sub jobs {
    LdifPrinter::Entries(@_, 'nordugrid-job-', 'globalid', \&jobAttributes);
}

#sub users {
#    LdifPrinter::Entries(@_, 'nordugrid-authuser-', 'name', \&userAttributes);
#}

sub queues {
    LdifPrinter::Entries(@_, 'nordugrid-queue-', 'name', \&queueAttributes, sub {
        my ($self, $data) = @_;
        $self->beginGroup('jobs');
        $self->jobs($data->{jobs});
        $self->end();
        #$self->beginGroup('users');
        #$self->users($data->{users});
        #$self->end();
    });
}

sub cluster {
    LdifPrinter::Entry(@_, 'nordugrid-cluster-', 'name', \&clusterAttributes, sub {
        my ($self, $data) = @_;
        $self->queues($data->{queues});
    });
}

sub Top {
    my ($self, $data) = @_;
    $self->begin('o' => "grid");
    $self->begin('Mds-Vo-name' => "local");
    $self->cluster($data);
}

1;
