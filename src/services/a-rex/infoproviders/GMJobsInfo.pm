package GMJobsInfo;

use File::Basename;
use lib dirname($0);

use base InfoCollector;
use InfoChecker;

use strict;

our $gmjobs_options_schema = {
        controldir => '',
};

our $gmjobs_info_schema = {
        '*' => {
            # from .local
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
            delegexpiretime    => '',
            clientname         => '',
            clientsoftware     => '',
            sessiondir         => '',
            diskspace          => '',
            failedstate        => '*',
            # from .description
            stdin              => '*',
            stdout             => '*',
            stderr             => '*',
            count              => '*',
            reqwalltime        => '*',
            reqcputime         => '*',
            runtimeenvironments=> [ '*' ],
            # from .status
            status             => '',
            completiontime     => '*',
            # from .diag
            exitcode           => '*',
            exec_host          => '*',
            UsedMem            => '*',
            WallTime           => '*',
            CpuTime            => '*'
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


sub get_gmjobs_info($) {
    my $options = shift;
    my $controldir = $options->{controldir};

    my %gmjobs;

    # read the list of jobs from the jobdir and create the @gridmanager_jobs
    # the @gridmanager_jobs contains the IDs from the job.ID.status

    unless (opendir JOBDIR,  $controldir ) {
        $log->error("can't access the directory of the jobstatus files at $controldir") and return undef;
    }
    my @allfiles = grep /\.status/, readdir JOBDIR;
    closedir JOBDIR;

    my @gridmanager_jobs = map {$_=~m/job\.(.+)\.status/; $_=$1;} @allfiles;

    # read the gridmanager jobinfo into a hash of hashes %gmjobs
    # filter out jobs not belonging to this $queue
    # fill the %gm_queued{SN} with number of gm_queued jobs for every grid user
    # count the prelrmsqueued, pendingprelrms grid jobs belonging to this queue

    foreach my $ID (@gridmanager_jobs) {
        my $gmjob_local       = $controldir."/job.".$ID.".local";
        my $gmjob_status      = $controldir."/job.".$ID.".status";
        my $gmjob_failed      = $controldir."/job.".$ID.".failed";
        my $gmjob_description = $controldir."/job.".$ID.".description";
        my $gmjob_diag        = $controldir."/job.".$ID.".diag";

        unless ( open (GMJOB_LOCAL, "<$gmjob_local") ) {
            $log->warning( "Can't read the $gmjob_local jobfile, skipping..." );
            next;
        }
        my @local_allines = <GMJOB_LOCAL>;

        # parse the content of the job.ID.local into the %gmjobs hash
        foreach my $line (@local_allines) {
            $line=~m/^(\w+)=(.+)$/;
            $gmjobs{$ID}{$1}=$2;
        }
        close GMJOB_LOCAL;

        # read the job.ID.status into "status"
        open (GMJOB_STATUS, "<$gmjob_status");
        my @status_allines=<GMJOB_STATUS>;
        chomp (my $job_status_firstline=$status_allines[0]);
        $gmjobs{$ID}{"status"}= $job_status_firstline;
        close GMJOB_STATUS;

        #Skip the remaining files if the jobstate "DELETED"
        next if $gmjobs{$ID}{"status"} eq "DELETED";

        # Comes the splitting of the terminal job state
        # check for job failure, (job.ID.failed )   "errors"

        if (-e $gmjob_failed) {
            open (GMJOB_FAILED, "<$gmjob_failed");
            my @failed_allines=<GMJOB_FAILED>;
            chomp  @failed_allines;
            my ($temp_errorstring) = join " ", @failed_allines;
            $temp_errorstring =~ s/\s+$//;
            if (length $temp_errorstring >= 87) {
                $temp_errorstring = substr ($temp_errorstring, 0, 87);
            }
            $gmjobs{$ID}{"errors"}="$temp_errorstring";
            close GMJOB_FAILED;
        }

        if ($gmjobs{$ID}{"status"} eq "FINISHED") {

            #terminal job state mapping

            if ( defined $gmjobs{$ID}{"errors"} ) {
                if ($gmjobs{$ID}{"errors"} =~ /User requested to cancel the job/) {
                    $gmjobs{$ID}{"status"} = "KILLED";
                } elsif ( defined $gmjobs{$ID}{"errors"} ) {
                    $gmjobs{$ID}{"status"} = "FAILED";
                }
            }

            #job-completiontime

            my @file_stat = stat $gmjob_status;
            my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
                = gmtime ($file_stat[9]);
            my $file_time = sprintf ( "%4d%02d%02d%02d%02d%02d%1s",
                                      $year+1900,$mon+1,$mday,$hour,
                                      $min,$sec,"Z");
            $gmjobs{$ID}{"completiontime"} = "$file_time";
        }

        # read the stdin,stdout,stderr from job.ID.description file
        # TODO: make it work for JSDL

        open (GMJOB_DESC, "<$gmjob_description");
        my @desc_allines=<GMJOB_DESC>;
        chomp (my $rsl_string=$desc_allines[0]);

        if ($rsl_string=~m/"stdin"\s+=\s+"(\S+)"/) {
            $gmjobs{$ID}{"stdin"}=$1;
        }

        if ($rsl_string=~m/"stdout"\s+=\s+"(\S+)"/) {
            $gmjobs{$ID}{"stdout"}=$1;
        }

        if ($rsl_string=~m/"stderr"\s+=\s+"(\S+)"/) {
            $gmjobs{$ID}{"stderr"}=$1;
        }

        if ($rsl_string=~m/"count"\s+=\s+"(\S+)"/) {
            $gmjobs{$ID}{"count"}=$1;
        } else {
            $gmjobs{$ID}{"count"}="1";
        }

        if ($rsl_string=~m/"cputime"\s+=\s+"(\S+)"/i) {
            my $reqcputime_sec=$1;
            $gmjobs{$ID}{"reqcputime"}= int $reqcputime_sec/60;
        }

        if ($rsl_string=~m/"walltime"\s+=\s+"(\S+)"/i) {
            my $reqwalltime_sec=$1;
            $gmjobs{$ID}{"reqwalltime"}= int $reqwalltime_sec/60;
        }

        my @runtimes = ();
        my $rsl_tail = $rsl_string;
        while ($rsl_tail =~ m/\("runtimeenvironment"\s+=\s+"([^\)]+)"/i) {
            push(@runtimes, $1);
            $rsl_tail = $'; # what's left after the match
        }
        $gmjobs{$ID}{runtimeenvironments} = \@runtimes;

        close GMJOB_DESC;

        #read the job.ID.diag file

        if (-s $gmjob_diag) {
            open (GMJOB_DIAG, "<$gmjob_diag");
            my ($kerneltime) = "";
            my ($usertime) = "";
            while ( my $line = <GMJOB_DIAG>) {
                $line=~m/nodename=(\S+)/ and
                    $gmjobs{$ID}{"exec_host"}.=$1."+";
                $line=~m/WallTime=(\d+)\./ and
                    $gmjobs{$ID}{"WallTime"} = ceil($1/60);
                $line=~m/exitcode=(\d+)/ and
                    $gmjobs{$ID}{"exitcode"}="exitcode".$1;
                $line=~m/UsedMemory=(\d+)kB/ and
                    $gmjobs{$ID}{"UsedMem"} = ceil($1);
                $line=~m/KernelTime=(\d+)\./ and
                    $kerneltime=$1;
                $line=~m/UserTime=(\d+)\./  and
                    $usertime=$1;
            }
            if ($kerneltime ne "" and $usertime ne "") {
                $gmjobs{$ID}{"CpuTime"}= ceil( ($kerneltime + $usertime)/ 60);
            } else {
                $gmjobs{$ID}{"CpuTime"} = "0";
            }
            close GMJOB_DIAG;
        }
    }

    return \%gmjobs;
}


sub test() {
    require Data::Dumper;
    LogUtils->getLogger()->level($LogUtils::DEBUG);
    my $opts = { controldir => '/home/grid/control' };
    my $results = GMJobsInfo->new()->get_info($opts);
    print Data::Dumper::Dumper($results);
}

#test;

1;
