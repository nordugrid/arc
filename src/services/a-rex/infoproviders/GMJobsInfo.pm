package GMJobsInfo;

use POSIX qw(ceil);

use base InfoCollector;
use InfoChecker;

use strict;

our $gmjobs_options_schema = {
        controldir => '',
};

our $gmjobs_info_schema = {
        '*' => {
            # from .local
            share              => '',
            lrms               => '*',
            queue              => '*',
            localid            => '*',
            args               => '',
            subject            => '',
            starttime          => '*',
            notify             => '*',
            rerun              => '',
            downloads          => '',
            uploads            => '',
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
            # from .grami -- not kept when the job is deleted
            stdin              => '*',
            stdout             => '*',
            stderr             => '*',
            count              => '',
            reqwalltime        => '*', # units: s
            reqcputime         => '*', # units: s
            runtimeenvironments=> [ '*' ],
            # from .status
            status             => '',
            completiontime     => '*',
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


# override InfoCollector base class methods 

sub _initialize($) {
    my ($self) = @_;
    $self->SUPER::_initialize();
    $self->{resname} = "jobs";
}

sub _get_options_schema {
    return $gmjobs_options_schema;
}
sub _get_results_schema {
    return $gmjobs_info_schema;
}
sub _collect($$) {
    my ($self,$options) = @_;
    return get_gmjobs_info($options);
}

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


sub get_gmjobs_info($) {
    my $options = shift;
    my $controldir = $options->{controldir};

    my %gmjobs;

    # read the list of jobs from the jobdir and create the @gridmanager_jobs
    # the @gridmanager_jobs contains the IDs from the job.ID.status

    unless (opendir JOBDIR,  $controldir ) {
        $log->warning("Can't access the job control directory: $controldir") and return {};
    }
    my @allfiles = grep /\.status/, readdir JOBDIR;
    closedir JOBDIR;

    my @gridmanager_jobs = map {$_=~m/job\.(.+)\.status/; $_=$1;} @allfiles;

    foreach my $ID (@gridmanager_jobs) {

        my $job_log = LogUtils->getLogger(__PACKAGE__.".Job:$ID");

        my $gmjob_local       = $controldir."/job.".$ID.".local";
        my $gmjob_status      = $controldir."/job.".$ID.".status";
        my $gmjob_failed      = $controldir."/job.".$ID.".failed";
        my $gmjob_grami       = $controldir."/job.".$ID.".grami";
        my $gmjob_diag        = $controldir."/job.".$ID.".diag";

        unless ( open (GMJOB_LOCAL, "<$gmjob_local") ) {
            $job_log->warning( "Can't read jobfile $gmjob_local, skipping..." );
            next;
        }
        my @local_allines = <GMJOB_LOCAL>;

        $gmjobs{$ID}{activityid} = [];

        # parse the content of the job.ID.local into the %gmjobs hash
        foreach my $line (@local_allines) {
	    if ($line=~m/^(\w+)=(.+)$/) {
		if ($1 eq "activityid") {
		    push @{$gmjobs{$ID}{activityid}}, $2;
		}
		else {
		    $gmjobs{$ID}{$1}=$2;
		}
	    }
	}
        close GMJOB_LOCAL;

        # OBS: assume now share=queue. Longer term modify GM to save share name
        $gmjobs{$ID}{share} = $gmjobs{$ID}{queue};
        
        # read the job.ID.status into "status"
        unless (open (GMJOB_STATUS, "<$gmjob_status")) {
            $job_log->warning("Can't open $gmjob_status");
            $gmjobs{$ID}{status} = undef;
        } else {
            my @file_stat = stat GMJOB_STATUS;
            chomp (my ($first_line) = <GMJOB_STATUS>);
            close GMJOB_STATUS;

            $gmjobs{$ID}{status} = $first_line;

            if (@file_stat) {

                # localowner
                my $uid = $file_stat[4];
                my $user = (getpwuid($uid))[0];
                if ($user) {
                    $gmjobs{$ID}{localowner} = $user;
                } else {
                    $job_log->warning("Cannot determine user name for owner (uid $uid)");
                }

                # completiontime
                if ($gmjobs{$ID}{"status"} eq "FINISHED") {
                    my ($s,$m,$h,$D,$M,$Y) = gmtime($file_stat[9]);
                    my $ts = sprintf("%4d%02d%02d%02d%02d%02d%1s",$Y+1900,$M+1,$D,$h,$m,$s,"Z");
                    $gmjobs{$ID}{"completiontime"} = $ts;
                }

            } else {
                $job_log->warning("Cannot stat status file: $!");
            }
        }

        # Comes the splitting of the terminal job state
        # check for job failure, (job.ID.failed )   "errors"

        if (-e $gmjob_failed) {
            unless (open (GMJOB_FAILED, "<$gmjob_failed")) {
                $job_log->warning("Can't open $gmjob_failed");
            } else {
                chomp (my @allines = <GMJOB_FAILED>);
                close GMJOB_FAILED;
                $gmjobs{$ID}{errors} = \@allines;
            }
        }

        if ($gmjobs{$ID}{"status"} eq "FINISHED") {

            #terminal job state mapping

            if ( $gmjobs{$ID}{errors} ) {
                if (grep /User requested to cancel the job/, @{$gmjobs{$ID}{errors}}) {
                    $gmjobs{$ID}{status} = "KILLED";
                } elsif ( defined $gmjobs{$ID}{errors} ) {
                    $gmjobs{$ID}{status} = "FAILED";
                }
            }
        }

        # read the job.ID.grami file

        if (-s $gmjob_grami) {
            unless ( open (GMJOB_GRAMI, "<$gmjob_grami") ) {
                $job_log->warning("Can't open $gmjob_grami");
            } else {
                while (my $line = <GMJOB_GRAMI>) {
                    if ($line =~ m/^joboption_count=(\d+)$/) {
                        $gmjobs{$ID}{count} = $1;
                    } elsif ($line =~ m/^joboption_walltime=(\d+)$/) {
                        $gmjobs{$ID}{reqwalltime} = $1;
                    } elsif ($line =~ m/^joboption_cputime=(\d+)$/) {
                        $gmjobs{$ID}{reqcputime} = $1;
                    } elsif ($line =~ m/^joboption_stdin=(\d+)$/) {
                        $gmjobs{$ID}{stdin} = $1;
                    } elsif ($line =~ m/^joboption_stdout=(\d+)$/) {
                        $gmjobs{$ID}{stdout} = $1;
                    } elsif ($line =~ m/^joboption_stderr=(\d+)$/) {
                        $gmjobs{$ID}{stderr} = $1;
                    } elsif ($line =~ m/^joboption_runtime_\d+='(.+)'$/) {
                        push @{$gmjobs{$ID}{runtimeenvironments}}, $1;
                    }
                }
            }
        }

        #read the job.ID.diag file

        if (-s $gmjob_diag) {
            unless ( open (GMJOB_DIAG, "<$gmjob_diag") ) {
                $job_log->warning("Can't open $gmjob_diag");
            } else {
                my ($kerneltime, $usertime);
                while (my $line = <GMJOB_DIAG>) {
                    $line=~m/^nodename=(\S+)/ and
                        push @{$gmjobs{$ID}{nodenames}}, $1;
                    $line=~m/^WallTime=(\d+)(\.\d*)?/ and
                        $gmjobs{$ID}{WallTime} = ceil($1);
                    $line=~m/^exitcode=(\d+)/ and
                        $gmjobs{$ID}{exitcode} = $1;
                    $line=~m/^AverageTotalMemory=(\d+)kB/ and
                        $gmjobs{$ID}{UsedMem} = ceil($1);
                    $line=~m/^KernelTime=(\d+)(\.\d*)?/ and
                        $kerneltime=$1;
                    $line=~m/^UserTime=(\d+)(\.\d*)?/ and
                        $usertime=$1;
                }
                close GMJOB_DIAG;

                $gmjobs{$ID}{CpuTime}= ceil($kerneltime + $usertime)
                    if defined $kerneltime and defined $usertime;
            }
        }
    }

    return \%gmjobs;
}


#### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ##### TEST ####

sub test() {
    require Data::Dumper;
    LogUtils->getLogger()->level($LogUtils::DEBUG);
    my $opts = { controldir => '/tmp/arc1/control' };
    my $results = GMJobsInfo->new()->get_info($opts);
    print Data::Dumper::Dumper($results);
}

#test;

1;
