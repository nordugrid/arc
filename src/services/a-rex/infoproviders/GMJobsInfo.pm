package GMJobsInfo;

use POSIX qw(ceil);

use LogUtils;

use strict;

# The returned hash looks something like this:

our $j = { 'jobID' => {
            # from .local
            lrms               => '*',
            queue              => '',
            localid            => '*',
            subject            => '',
            starttime          => '*',
            jobname            => '*',
            gmlog              => '*',
            cleanuptime        => '*',
            delegexpiretime    => '*',
            clientname         => '',
            clientsoftware     => '*',
            activityid         => [ '*' ],
            sessiondir         => '',
            diskspace          => '',
            failedstate        => '*',
            fullaccess         => '*',
            lifetime           => '*',
            jobreport          => '*',
            # from .description
            description        => '', # rsl or xml
            # from .grami -- not kept when the job is deleted
            stdin              => '*',
            stdout             => '*',
            stderr             => '*',
            count              => '*',
            reqwalltime        => '*', # units: s
            reqcputime         => '*', # units: s
            runtimeenvironments=> [ '*' ],
            # from .status
            status             => '',
            statustime         => '',  # seconds since epoch
            completiontime     => '*', # MDS time format
            localowner         => '',
            # from .failed
            errors             => [ '*' ],
            lrmsexitcode       => '*',
            # from .diag
            exitcode           => '*',
            nodenames          => [ '*' ],
            UsedMem            => '*', # units: kB; summed over all execution threads
            CpuTime            => '*', # units: s;  summed over all execution threads
            WallTime           => '*'  # units: s;  real-world time elapsed
        }
};

our $log = LogUtils->getLogger(__PACKAGE__);

# extract the lower limit of a RangeValue_Type expression
sub get_range_minimum($$) {
    my ($range_str, $jsdl_ns_re) = @_;
    my $min;
    my $any_range_re = "$jsdl_ns_re(?:Exact|(?:Lower|Upper)Bound(?:edRange)?)";
    while ($range_str =~ m!<($any_range_re)\b[^>]*>[\s\n]*(-?[\d.]+)[\s\n]*</\1!mg) {
        $min = $2 if not defined $min or $min > $2;
    }
    return $min;
}


sub collect {
    my ($controldir, $nojobs) = @_;

    my %gmjobs;

    # read the list of jobs from the jobdir and create the @gridmanager_jobs
    # the @gridmanager_jobs contains the IDs from the job.ID.status

    foreach my $controlsubdir ("$controldir/accepting", "$controldir/processing", "$controldir/finished") {

    unless (opendir JOBDIR,  $controlsubdir ) {
        $log->warning("Can't access the job control directory: $controlsubdir") and return {};
    }
    my @allfiles = grep /\.status/, readdir JOBDIR;
    closedir JOBDIR;

    my @gridmanager_jobs = map {$_=~m/job\.(.+)\.status/; $_=$1;} @allfiles;

    foreach my $ID (@gridmanager_jobs) {

        my $job = $gmjobs{$ID} = {};

        my $gmjob_local       = $controldir."/job.".$ID.".local";
        my $gmjob_status      = $controlsubdir."/job.".$ID.".status";
        my $gmjob_failed      = $controldir."/job.".$ID.".failed";
        my $gmjob_description = $controldir."/job.".$ID.".description";
        my $gmjob_grami       = $controldir."/job.".$ID.".grami";
        my $gmjob_diag        = $controldir."/job.".$ID.".diag";

        unless ( open (GMJOB_LOCAL, "<$gmjob_local") ) {
            $log->warning( "Job $ID: Can't read jobfile $gmjob_local, skipping..." );
            next;
        }
        my @local_allines = <GMJOB_LOCAL>;

        $job->{activityid} = [];

        # parse the content of the job.ID.local into the %gmjobs hash
        foreach my $line (@local_allines) {
            if ($line=~m/^(\w+)=(.+)$/) {
                if ($1 eq "activityid") {
                    push @{$job->{activityid}}, $2;
                }
                else {
                    $job->{$1}=$2;
                }
            }
        }
        close GMJOB_LOCAL;

        # Extract jobID uri
        if ($job->{globalid}) {
            $job->{globalid} =~ s/.*JobSessionDir>([^<]+)<.*/$1/;
        } else {
            $log->debug("Job $ID: 'globalid' missing from .local file");
        }
        # Rename queue -> share
        if (exists $job->{queue}) {
            $job->{share} = $job->{queue};
            delete $job->{queue};
        } else {
            $log->warning("Job $ID: 'queue' missing from .local file");
        }

        # read the job.ID.status into "status"
        unless (open (GMJOB_STATUS, "<$gmjob_status")) {
            $log->warning("Job $ID: Can't open $gmjob_status");
            $job->{status} = undef;
        } else {
            my @file_stat = stat GMJOB_STATUS;
            chomp (my ($first_line) = <GMJOB_STATUS>);
            close GMJOB_STATUS;

            $log->warning("Job $ID: Failed to read status") unless $first_line;
            $job->{status} = $first_line;

            if (@file_stat) {

                # localowner
                my $uid = $file_stat[4];
                my $user = (getpwuid($uid))[0];
                if ($user) {
                    $job->{localowner} = $user;
                } else {
                    $log->warning("Job $ID: Cannot determine user name for owner (uid $uid)");
                }

                $job->{"statustime"} = $file_stat[9];
                # completiontime
                if ($job->{"status"} eq "FINISHED") {
                    my ($s,$m,$h,$D,$M,$Y) = gmtime($file_stat[9]);
                    my $ts = sprintf("%4d%02d%02d%02d%02d%02d%1s",$Y+1900,$M+1,$D,$h,$m,$s,"Z");
                    $job->{"completiontime"} = $ts;
                }

            } else {
                $log->warning("Job $ID: Cannot stat status file: $!");
            }
        }

        # Comes the splitting of the terminal job state
        # check for job failure, (job.ID.failed )   "errors"

        if (-e $gmjob_failed) {
            unless (open (GMJOB_FAILED, "<$gmjob_failed")) {
                $log->warning("Job $ID: Can't open $gmjob_failed");
            } else {
                chomp (my @allines = <GMJOB_FAILED>);
                close GMJOB_FAILED;
                $job->{errors} = \@allines;
            }
        }

        if ($job->{"status"} eq "FINISHED") {

            #terminal job state mapping

            if ( $job->{errors} ) {
                if (grep /User requested to cancel the job/, @{$job->{errors}}) {
                    $job->{status} = "KILLED";
                } elsif ( defined $job->{errors} ) {
                    $job->{status} = "FAILED";
                }
            }
        }

        # if jobs are not printed, it's sufficient to have jobid, status,
        # subject, queue and share. Can skip the rest.
        next if $nojobs;

        # read the job.ID.grami file

        unless ( open (GMJOB_GRAMI, "<$gmjob_grami") ) {
            # this file is is kept by A-REX during the hole existence of the
            # job. grid-manager from arc0, however, deletes it after the job
            # has finished.
            $log->debug("Job $ID: Can't open $gmjob_grami");
        } else {
            my $sessiondir = $job->{sessiondir};
            while (my $line = <GMJOB_GRAMI>) {
                if ($line =~ m/^joboption_count=(\d+)$/) {
                    $job->{count} = $1;
                } elsif ($line =~ m/^joboption_walltime=(\d+)$/) {
                    $job->{reqwalltime} = $1;
                } elsif ($line =~ m/^joboption_cputime=(\d+)$/) {
                    $job->{reqcputime} = $1;
                } elsif ($line =~ m/^joboption_stdin='(.+)'$/) {
                    $job->{stdin} = $1;
                    $job->{stdin} =~ s/^\Q$sessiondir\E\/*// if $sessiondir;
                } elsif ($line =~ m/^joboption_stdout='(.+)'$/) {
                    $job->{stdout} = $1;
                    $job->{stdout} =~ s/^\Q$sessiondir\E\/*// if $sessiondir;
                } elsif ($line =~ m/^joboption_stderr='(.+)'$/) {
                    $job->{stderr} = $1;
                    $job->{stderr} =~ s/^\Q$sessiondir\E\/*// if $sessiondir;
                } elsif ($line =~ m/^joboption_runtime_\d+='(.+)'$/) {
                    my $rte = $1;
                    $rte =~ s/'\\''/'/g; # unescacpe escaped single quotes
                    push @{$job->{runtimeenvironments}}, $rte;
                }
            }
            close GMJOB_GRAMI;
        }

        #read the job.ID.description file

        unless ( open (GMJOB_DESCRIPTION, "<$gmjob_description") ) {
            $log->warning("Job $ID: Can't open $gmjob_description");
        } else {
            while (my $line = <GMJOB_DESCRIPTION>) {
                chomp $line;
                next unless $line;
                if ($line =~ m/^\s*<\?xml/) { $job->{description} = 'xml'; last }
                if ($line =~ m/^\s*<!--/)   { $job->{description} = 'xml'; last }
                if ($line =~ m/^\s*[&+|(]/) { $job->{description} = 'rsl'; last }
                $log->warning("Job $ID: Can't identify job description language");
                last;
            }
            close GMJOB_DESCRIPTION;
        }

        #read the job.ID.diag file

        if (-s $gmjob_diag) {
            unless ( open (GMJOB_DIAG, "<$gmjob_diag") ) {
                $log->warning("Job $ID: Can't open $gmjob_diag");
            } else {
                my ($kerneltime, $usertime);
                while (my $line = <GMJOB_DIAG>) {
                    $line=~m/^nodename=(\S+)/ and
                        push @{$job->{nodenames}}, $1;
                    $line=~m/^WallTime=(\d+)(\.\d*)?/ and
                        $job->{WallTime} = ceil($1);
                    $line=~m/^exitcode=(\d+)/ and
                        $job->{exitcode} = $1;
                    $line=~m/^AverageTotalMemory=(\d+)kB/ and
                        $job->{UsedMem} = ceil($1);
                    $line=~m/^KernelTime=(\d+)(\.\d*)?/ and
                        $kerneltime=$1;
                    $line=~m/^UserTime=(\d+)(\.\d*)?/ and
                        $usertime=$1;
                }
                close GMJOB_DIAG;

                $job->{CpuTime}= ceil($kerneltime + $usertime)
                    if defined $kerneltime and defined $usertime;
            }
        }
    }
    }
    return \%gmjobs;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test() {
    require Data::Dumper;
    LogUtils::level('VERBOSE');
    my $results = GMJobsInfo::collect('/data/jobstatus');
    print Data::Dumper::Dumper($results);
}

#test;

1;
