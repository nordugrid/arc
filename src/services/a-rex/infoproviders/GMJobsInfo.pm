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
            sessiondir         => '',
            diskspace          => '',
            failedstate        => '*',
            fullaccess         => '*',
            lifetime           => '*',
            jobreport          => '*',
            # from .description -- it's kept even for deleted jobs
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
            UsedMem            => '*', # units: kB; summed over all threads
            CpuTime            => '*', # units: s;  summed over all threads
            WallTime           => '*'  # units: s;  real-world time elapsed
        }
};

our $log = LogUtils->getLogger(__PACKAGE__);


# override InfoCollector base class methods 

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


# Ad-hoc XRSL 'parser'

sub parse_xrsl($) {
    my ($rsl_string) = @_;

    my $job = {};

    if ($rsl_string=~m/"stdin"\s*=\s*"(\S+)"/) {
        $job->{"stdin"}=$1;
    }
    if ($rsl_string=~m/"stdout"\s*=\s*"(\S+)"/) {
        $job->{"stdout"}=$1;
    }
    if ($rsl_string=~m/"stderr"\s*=\s*"(\S+)"/) {
        $job->{"stderr"}=$1;
    }
    if ($rsl_string=~m/"count"\s*=\s*"(\S+)"/) {
        $job->{"count"}=$1;
    }
    if ($rsl_string=~m/"cputime"\s*=\s*"(\S+)"/i) {
        my $reqcputime_sec=$1;
        $job->{"reqcputime"}= int $reqcputime_sec;
    }
    if ($rsl_string=~m/"walltime"\s*=\s*"(\S+)"/i) {
        my $reqwalltime_sec=$1;
        $job->{"reqwalltime"}= int $reqwalltime_sec;
    }

    while ($rsl_string =~ m/\("runtimeenvironment"\s+=\s+([^\)]+)/ig) {
        my $tmp_string = $1;
        push(@{$job->{runtimeenvironments}}, $1) while ($tmp_string =~ m/"(\S+)"/g);
    }

    return $job;
}


# Ad-hoc JSDL 'parser'

sub parse_jsdl($$) {
    my ($jsdl_string, $job_log) = @_;

    my $job = {};

    my $any_name_re = '[\w\.-]+';
    my $any_ns_re = "(?:$any_name_re:)?";

    # strip comments
    $jsdl_string =~ s/<!--.*?-->//gs;

    
    if ($jsdl_string =~ m!xmlns(?::($any_name_re))?\s*=\s*"http://schemas.ggf.org/jsdl/2005/11/jsdl"!o) {
        my $jsdl_ns_re = $1 ? "$1:" : "";
    
        if ($jsdl_string =~ m!xmlns:(\S+)\s*=\s*"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix"!) {
            my $posix_ns = $1;
    
            if ($jsdl_string =~ m!<(${jsdl_ns_re}Application)\b[^>]*>(.+?)</\1>!s) {
                my $appl_string = $2;

                if ($appl_string =~ m!<$posix_ns:Input>([^<]+)</$posix_ns:Input>!) {
                    $job->{"stdin"}=$1;
                }
                if ($appl_string =~ m!<$posix_ns:Output>([^<]+)</$posix_ns:Output>!) {
                    $job->{"stdout"}=$1;
                }
                if ($appl_string =~ m!<$posix_ns:Error>([^<]+)</$posix_ns:Error>!) {
                    $job->{"stderr"}=$1;
                }
                if ($appl_string =~ m!<$posix_ns:WallTimeLimit>([^<]+)</$posix_ns:WallTimeLimit>!) {
                    $job->{"reqwalltime"}= int $1;
                }
                if ($appl_string =~ m!<$posix_ns:CPUTimeLimit>([^<]+)</$posix_ns:CPUTimeLimit>!) {
                    $job->{"reqcputime"}= int $1;
                }
            } else {
                $job_log->warning("Application entry is missing from JSDL description");
            }
    
        } else {
            $job_log->warning("Could not find jsdl-posix xmlns in JSDL description");
        }
    
        if ($jsdl_string =~ m!<(${jsdl_ns_re}Resources)\b[^>]*>(.+?)</\1>!s) {
            my $resources_string = $2;
    
            while ($resources_string =~ m!<(${any_ns_re}RunTimeEnvironment)\b[^>]*>(.+?)</\1>!sg) {
                my $rte_string = $2;
                if ($rte_string =~ m!<(${any_ns_re}Name)\b[^>]*>([^<]+)</\1>!) {
                    my $rte_name = $2;
                    if ($rte_string =~ m!<(${any_ns_re}Version)\b[^>]*>(.+?)</\1>!s) {
                        my $range_string = $2;
                        if ($range_string =~ m!<(${jsdl_ns_re}Exact)\b[^>]*>([^<]+)</\1>!) {
                            $rte_name .= "-$2";
                        } else {
                            $job_log->warning("Exact range expected for RunTimeEnvironment Version in JSDL description");
                        }
                    }
                    push @{$job->{runtimeenvironments}}, $rte_name;
                } else {
                    $job_log->warning("Name element of RunTimeEnvironment missing in JSDL description");
                }
            }
            my $resourcecount = 1;
            if ($resources_string =~ m!<(${jsdl_ns_re}TotalResourceCount)\b[^>]*>(.+?)</\1>!s) {
                my $min = get_range_minimum($2,$jsdl_ns_re);
                $job_log->warning("Failed to extract lower limit of TotalResourceCount in JSDL description") unless defined $min;
                $resourcecount = $min if $min;
            }
            if ($resources_string =~ m!<(${jsdl_ns_re}TotalCPUCount)\b[^>]*>(.+?)</\1>!s) {
                my $min = get_range_minimum($2,$jsdl_ns_re);
                $job_log->warning("Failed to extract lower limit of TotalCPUCount in JSDL description") unless defined $min;
                $job->{"count"} = int $min if defined $min;
            } elsif ($resources_string =~ m!<(${jsdl_ns_re}IndividualCPUCount)\b[^>]*>(.+?)</\1>!s) {
                my $min = get_range_minimum($2,$jsdl_ns_re);
                $job_log->warning("Failed to extract lower limit of IndividualCPUCount in JSDL description") unless defined $min;
                $job->{"count"} = int($min*$resourcecount) if defined $min;
            }
            if ($resources_string =~ m!<(${jsdl_ns_re}TotalCPUTime)\b[^>]*>(.+?)</\1>!s) {
                my $min = get_range_minimum($2,$jsdl_ns_re);
                $job_log->warning("Failed to extract lower limit of TotalCPUTime in JSDL description") unless defined $min;
                $job->{"reqcputime"} = int($min) if defined $min;
            } elsif ($resources_string =~ m!<(${jsdl_ns_re}IndividualCPUTime)\b[^>]*>(.+?)</\1>!s) {
                my $min = get_range_minimum($2,$jsdl_ns_re);
                $job_log->warning("Failed to extract lower limit of IndividualCPUTime in JSDL description") unless defined $min;
                $job->{"reqcputime"} = int($min*$resourcecount) if defined $min;
            }
        }
    } else {
        $job_log->warning("jsdl namespace declaration is missing from JSDL description");
    }

    return $job;
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
        my $gmjob_description = $controldir."/job.".$ID.".description";
        my $gmjob_diag        = $controldir."/job.".$ID.".diag";

        unless ( open (GMJOB_LOCAL, "<$gmjob_local") ) {
            $job_log->warning( "Can't read jobfile $gmjob_local, skipping..." );
            next;
        }
        my @local_allines = <GMJOB_LOCAL>;

        # parse the content of the job.ID.local into the %gmjobs hash
        foreach my $line (@local_allines) {
            $gmjobs{$ID}{$1}=$2 if $line=~m/^(\w+)=(.+)$/;
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
                    my $ts = sprintf("%4d%02d%02d%02d%02d%02d%1sZ",$Y+1900,$M+1,$D,$h,$m,$s);
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

        # read the stdin,stdout,stderr from job.ID.description file

        unless ( open (GMJOB_DESC, "<$gmjob_description") ) {
            $job_log->warning("Cannot open $gmjob_description");
        } else {
            my $desc_allines;
            { local $/; $desc_allines = <GMJOB_DESC>; } # read in the whole file
            close GMJOB_DESC;
            chomp $desc_allines;

            my $job_desc = {};
            if ($desc_allines =~ /^<\?xml/) {
                $job_desc = parse_jsdl($desc_allines, $job_log);
                $job_log->warning("Failed to parse JSDL job description") unless %$job_desc;
            } else {
                $job_desc = parse_xrsl($desc_allines);
                $job_log->warning("Failed to parse xRSL job description") unless %$job_desc;
            }
            $job_desc->{count} = 1 unless defined $job_desc->{count};
            $job_desc->{runtimeenvironments} = [] unless $job_desc->{runtimeenvironments};
            $gmjobs{$ID} = { %{$gmjobs{$ID}}, %{$job_desc} };
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
