package NGldifPrinter;

use base LdifPrinter;

use POSIX;

sub new {
    my ($this, $handle, $ttl) = @_;
    my $self = $this->SUPER::new($handle);
    my $now = time;
    $self->{validfrom} = strftime("%Y%m%d%H%M%SZ", gmtime($now));
    $self->{validto} = strftime("%Y%m%d%H%M%SZ", gmtime($now + $ttl));
    return $self;
}

sub Entry {
    my ($self, $collector, $prefix, $key, $attributes, $subtree) = @_;
    return unless $collector and my $data = &$collector();
    $self->begin("$prefix$key", $data->{$key});
    &$attributes($self,$data);
    &$subtree($self, $data) if $subtree;
    $self->end();
}

sub Entries {
    my ($self, $collector, $prefix, $key, $attributes, $subtree) = @_;
    while ($collector and my $data = &$collector()) {
        $self->begin("$prefix$key", $data->{$key});
        &$attributes($self,$data);
        &$subtree($self, $data) if $subtree;
        $self->end();
    }
}


sub MdsAttributes {
    my ($self) = @_;
    $self->attribute(objectClass => 'Mds');
    $self->attribute('Mds-validfrom' => $self->{validfrom});
    $self->attribute('Mds-validto' => $self->{validto});
}

sub clusterAttributes {
    my ($self, $data) = @_;
    $self->MdsAttributes($data);
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
    $self->MdsAttributes($data);
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
                                                     maxtotalcputime ));
}

sub jobAttributes {
    my ($self, $data) = @_;
    $self->MdsAttributes($data);
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

sub userAttributes {
    my ($self, $data) = @_;
    $self->MdsAttributes($data);
    $self->attribute(objectClass => 'nordugrid-authuser');
    $self->attributes($data, 'nordugrid-authuser-', qw( name sn freecpus diskspace queuelength ));
}

sub beginGroup {
    my ($self, $name) = @_;
    $self->begin('nordugrid-info-group-name' => $name);
    $self->MdsAttributes();
    $self->attribute(objectClass => 'nordugrid-info-group');
    $self->attribute('nordugrid-info-group-name' => $name);
}

sub endGroup {
    my ($self) = @_;
    $self->end();
}

sub jobs {
    Entries(@_, 'nordugrid-job-', 'globalid', \&jobAttributes);
}

sub users {
    Entries(@_, 'nordugrid-authuser-', 'name', \&userAttributes);
}

sub queues {
    Entries(@_, 'nordugrid-queue-', 'name', \&queueAttributes, sub {
        my ($self, $data) = @_;
        $self->beginGroup('jobs');
        $self->jobs($data->{jobs});
        $self->endGroup();
        $self->beginGroup('users');
        $self->users($data->{users});
        $self->endGroup();
    });

}

sub cluster {
    Entry(@_, 'nordugrid-cluster-', 'name', \&clusterAttributes, sub {
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
