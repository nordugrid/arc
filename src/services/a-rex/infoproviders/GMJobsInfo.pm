package GMJobsInfo;

use File::Basename;
use lib dirname($0);
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
            # from .failed
            errors             => '*',
            # from .diag
            exitcode           => '*',
            nodenames          => [ '*' ],
            UsedMem            => '*', # units: kB
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
        $log->error("Can't access the job control directory: $controldir") and return {};
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
            $log->warning( "Can't read jobfile $gmjob_local, skipping..." );
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
        unless (open (GMJOB_STATUS, "<$gmjob_status")) {
            $log->warning("Can't open $gmjob_status");
            $gmjobs{$ID}{status} = undef;
        } else {
            chomp (my ($first_line) = <GMJOB_STATUS>);
            close GMJOB_STATUS;
            $gmjobs{$ID}{status} = $first_line;
        }

        #Skip the remaining files if the jobstate "DELETED"
        next if $gmjobs{$ID}{"status"} eq "DELETED";

        # Comes the splitting of the terminal job state
        # check for job failure, (job.ID.failed )   "errors"

        if (-e $gmjob_failed) {
            unless (open (GMJOB_FAILED, "<$gmjob_failed")) {
                $log->warning("Can't open $gmjob_failed");
            } else {
                chomp (my @allines = <GMJOB_FAILED>);
                close GMJOB_FAILED;
                my $errors = join " ", @allines;
                $errors =~ s/\s+$//;
                $gmjobs{$ID}{errors} = substr($errors, 0, 87) if $errors;

                #PBS backend doesn't write to .diag when walltime is exceeded
                if ($errors =~ /PBS: job killed: walltime (\d*) exceeded limit (\d*)/){
                    $gmjobs{$ID}{"WallTime"} = ceil($2/60);
                }
            }
        }

        if ($gmjobs{$ID}{"status"} eq "FINISHED") {

            #terminal job state mapping

            if ( $gmjobs{$ID}{errors} ) {
                if ($gmjobs{$ID}{errors} =~ /User requested to cancel the job/) {
                    $gmjobs{$ID}{status} = "KILLED";
                } elsif ( defined $gmjobs{$ID}{errors} ) {
                    $gmjobs{$ID}{status} = "FAILED";
                }
            }

            #job-completiontime

            my @file_stat = stat $gmjob_status;
            my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
                = gmtime ($file_stat[9]);
            my $file_time = sprintf ( "%4d%02d%02d%02d%02d%02d%1s",
                                      $year+1900,$mon+1,$mday,$hour,
                                      $min,$sec,"Z");
            $gmjobs{$ID}{"completiontime"} = $file_time;
        }

        # read the stdin,stdout,stderr from job.ID.description file
        # TODO: make it work for JSDL

        unless ( open (GMJOB_DESC, "<$gmjob_description") ) {
            $log->warning("Cannot open $gmjob_description");
        } else {
            my @desc_allines=<GMJOB_DESC>;
            close GMJOB_DESC;

            chomp (my $rsl_string = $desc_allines[0]);

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

            while ($rsl_string =~ m/\("runtimeenvironment"\s+=\s+([^\)]+)/ig) {
                my $tmp_string = $1;
                push(@{$gmjobs{$ID}{runtimeenvironments}}, $1) while ($tmp_string =~ m/"(\S+)"/g);
            }
        }

        #read the job.ID.diag file

        if (-s $gmjob_diag) {
            unless ( open (GMJOB_DIAG, "<$gmjob_diag") ) {
                $log->warning("Can't open $gmjob_diag");
            } else {
                my ($kerneltime, $usertime);
                while (my $line = <GMJOB_DIAG>) {
                    $line=~m/nodename=(\S+)/ and
                        push @{$gmjobs{$ID}{nodenames}}, $1;
                    $line=~m/WallTime=(\d+)\./ and
                        $gmjobs{$ID}{WallTime} = ceil($1/60);
                    $line=~m/exitcode=(\d+)/ and
                        $gmjobs{$ID}{exitcode} = $1;
                    $line=~m/UsedMemory=(\d+)kB/ and
                        $gmjobs{$ID}{UsedMem} = ceil($1);
                    $line=~m/KernelTime=(\d+)\./ and
                        $kerneltime=$1;
                    $line=~m/UserTime=(\d+)\./ and
                        $usertime=$1;
                }
                close GMJOB_DIAG;

                $gmjobs{$ID}{CpuTime}= ceil( ($kerneltime + $usertime)/ 60)
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
