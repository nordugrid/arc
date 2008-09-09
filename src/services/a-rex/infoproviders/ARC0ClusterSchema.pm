package ARC0ClusterSchema;

use base Exporter;
our @EXPORT_OK = qw(arc0_info_schema);

sub arc0_info_schema {

    my $job_t = {
            'name' => '',
            'xmlns:nuj0' => '',
            'M0:validfrom' => [ '*' ],
            'M0:validto' => [ '*' ],
            'nuj0:name' => [ '' ],
            'nuj0:globalid' => [ '' ],
            'nuj0:globalowner' => [ '' ],
            'nuj0:jobname' => [ '*' ],
            'nuj0:execcluster' => [ '*' ],
            'nuj0:execqueue' => [ '*' ],
            'nuj0:executionnodes' => [ '*' ],
            'nuj0:submissionui' => [ '' ],
            'nuj0:submissiontime' => [ '' ],
            'nuj0:sessiondirerasetime' => [ '*' ],
            'nuj0:proxyexpirationtime' => [ '*' ],
            'nuj0:completiontime' => [ '*' ],
            'nuj0:runtimeenvironment' => [ '*' ],
            'nuj0:gmlog' => [ '*' ],
            'nuj0:clientsoftware' => [ '*' ],
            'nuj0:stdout' => [ '*' ],
            'nuj0:stderr' => [ '*' ],
            'nuj0:stdin' => [ '*' ],
            'nuj0:cpucount' => [ '*' ],
            'nuj0:reqcputime' => [ '*' ],
            'nuj0:reqwalltime' => [ '*' ],
            'nuj0:queuerank' => [ '*' ],
            'nuj0:comment' => [ '*' ],
            'nuj0:usedcputime' => [ '*' ],
            'nuj0:usedwalltime' => [ '*' ],
            'nuj0:usedmem' => [ '*' ],
            'nuj0:exitcode' => [ '*' ],
            'nuj0:errors' => [ '*' ],
            'nuj0:status' => [ '*' ],
            'nuj0:rerunable' => [ '*' ]
    };

    my $jobs_t =  {
            'name' => '',
            'xmlns:nig0' => '',
            'M0:validfrom' => [ '*' ],
            'M0:validto' => [ '*' ],
            'nig0:name' => [ '' ],
            'nuj0:name' => [ $job_t ]
    };

    my $user_t = {
            'name' => '',
            'xmlns:na0' => '',
            'M0:validfrom' => [ '*' ],
            'M0:validto' => [ '*' ],
            'na0:name' => [ '' ],
            'na0:sn' => [ '' ],
            'na0:freecpus' => [ '' ],
            'na0:diskspace' => [ '' ],
            'na0:queuelength' => [ '' ]
    };

    my $users_t = {
            'name' => '',
            'xmlns:nig0' => '',
            'M0:validfrom' => [ '*' ],
            'M0:validto' => [ '*' ],
            'nig0:name' => [ '' ],
            'na0:name' => [ $user_t ]
    };

    my $queue_t = {
            'name' => '',
            'xmlns:nq0' => '',
            'M0:validfrom' => [ '*' ],
            'M0:validto' => [ '*' ],
            'nq0:name' => [ '' ],
            'nq0:status' => [ '' ],
            'nq0:comment' => [ '*' ],
            'nq0:schedulingpolicy' => [ '*' ],
            'nq0:homogeneity' => [ '' ],
            'nq0:nodecpu' => [ '' ],
            'nq0:nodememory' => [ '' ],
            'nq0:architecture' => [ '' ],
            'nq0:opsys' => [ '' ],
            'nq0:benchmark' => [ '*' ],
            'nq0:maxrunning' => [ '*' ],
            'nq0:maxqueuable' => [ '*' ],
            'nq0:maxuserrun' => [ '*' ],
            'nq0:maxcputime' => [ '*' ],
            'nq0:mincputime' => [ '*' ],
            'nq0:defaultcputime' => [ '*' ],
            'nq0:maxwalltime' => [ '*' ],
            'nq0:minwalltime' => [ '*' ],
            'nq0:defaultwalltime' => [ '*' ],
            'nq0:running' => [ '' ],
            'nq0:gridrunning' => [ '' ],
            'nq0:localqueued' => [ '' ],
            'nq0:gridqueued' => [ '' ],
            'nq0:prelrmsqueued' => [ '' ],
            'nq0:totalcpus' => [ '' ],
            'nig0:jobs' => $jobs_t,
            'nig0:users' => $users_t
    };

    my $cluster_t = {
            'xmlns:n' => '',
            'xmlns:M0' => '',
            'xmlns:nc0' => '',
            'M0:validfrom' => [ '*' ],
            'M0:validto' => [ '*' ],
            'nc0:name' => [ '' ],
            'nc0:aliasname' => [ '*' ],
            'nc0:comment' => [ '*' ],
            'nc0:owner' => [ '*' ],
            'nc0:acl' => [ '*' ],
            'nc0:location' => [ '*' ],
            'nc0:issuerca' => [ '' ],
            'nc0:issuerca-hash' => [ '' ],
            'nc0:trustedca' => [ '' ],
            'nc0:contactstring' => [ '' ],
            'nc0:interactive-contactstring' => [ '*' ],
            'nc0:support' => [ '' ],
            'nc0:lrms-type' => [ '' ],
            'nc0:lrms-version' => [ '' ],
            'nc0:lrms-config' => [ '*' ],
            'nc0:architecture' => [ '*' ],
            'nc0:opsys' => [ '*' ],
            'nc0:benchmark' => [ '*' ],
            'nc0:homogeneity' => [ '' ],
            'nc0:nodecpu' => [ '*' ],
            'nc0:nodememory' => [ '' ],
            'nc0:nodeaccess' => [ '' ],
            'nc0:totalcpus' => [ '' ],
            'nc0:usedcpus' => [ '' ],
            'nc0:cpudistribution' => [ '' ],
            'nc0:prelrmsqueued' => [ '' ],
            'nc0:totaljobs' => [ '' ],
            'nc0:localse' => [ '*' ],
            'nc0:sessiondir-free' => [ '' ],
            'nc0:sessiondir-total' => [ '' ],
            'nc0:sessiondir-lifetime' => [ '' ],
            'nc0:cache-free' => [ '*' ],
            'nc0:cache-total' => [ '*' ],
            'nc0:middleware' => [ '' ],
            'nc0:runtimeenvironment' => [ '*' ],
            'nq0:name' => [ $queue_t ]
    };

    return $cluster_t;
}

1;
