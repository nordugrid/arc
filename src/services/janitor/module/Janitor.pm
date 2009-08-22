package Janitor::Janitor;

=head1 NAME

Janitor::Janitor - Perl extension for blah blah blah

=head1 SYNOPSIS

  use Janitor::Janitor;
  blah blah blah

=head1 DESCRIPTION

Stub documentation for Janitor::Janitor, created by h2xs. It looks like the
author of the extension was negligent enough to leave the stub
unedited.

Blah blah blah.

=head2 EXPORT

None by default.



=head1 SEE ALSO

Mention other useful documentation such as the documentation of
related modules or operating system documentation (such as man pages
in UNIX), or any relevant external documentation such as RFCs or
standards.


=head1 NAME

Main.pm - This file encapsulates the functionality of the Runtime Environment maintenance.

The most importants methods are:

=over 4

=item C<register_job>

This method registers a job along with an array of runtime environment names.
Nothing will be downloaded or installed within this function call.

The job id needs to be fresh and the desired runtime environment names have to 
be at least installable. Once the job has sucessfully registered, the state will 
be set to INSTALLABLE. Furthermore the a state machine for the runtime environment 
is created. Its initial state will be in that case PREPARED.

=item C<deploy_for_job>

Downloads and installs the runtime environment.

To invoke this method only the job id needs to be passed! 

=item C<remove_job>

Removes a job from the janitor database.

This operation removes the lock on the runtime environments occupied by this job.
If a runtime environment has no more locks left, its state will change to 
REMOVAL_PENDING and thus removed after a certain time period.

=item C<sweep>

Removes runtime environments which are in the state REMOVAL_PENDING. 

In order to remove runtime environments being in the FAILED state, the method set 
state must be used. Environments within an UNKNOWN needs to be removed manually.

=item C<list_info>

Collects information about the janitor. No argument has to be passed to that method.

This includes: 
- Current Jobs and their states
- Dynamically registered runtime environments and their state
- Locally installed runtime environment
- A list of installable runtime environments based on the catalog

=item C<list_job_info>

Returns information about a certain job id.

=item C<setstate>

Changes the state of a set of runtime environments.

Only a few state transitions are permitted:

	INSTALLED_M  => REMOVAL_PENDING
	INSTALLED_A  => REMOVAL_PENDING
	BROKEN       => REMOVAL_PENDING
	VALIDATED    => REMOVAL_PENDING
	FAILED       => REMOVAL_PENDING

=item C<search>

Simple search for a matching words within installable and local 
runtime environment environment.

=back

The returned content of these methods have a different complexity and can
lead to a large set of states. In order to unify the results such that it
can be interpreted by diffrent interfaces of upper layers, the returned
content is wrapped within the class C<Janitor::Response>. To figure out
how the information is stored please read the corresponding class 
documenation.

The current perl module furthermore provides a second interface to access
the previously mentioned methods. This interface is provided by the
method C<process> together with the class C<Janitor::Request>, which 
bundles the query information. More information about its usage can be 
found within the corresponding class.

=cut


# export NORDUGRID_CONFIG="/nfshome/knowarc/dredesign/src/services/janitor2/conf/arc.conf"

use Switch;

# Log4perl :rescurrect does not work, if this is not the first command
BEGIN {
	# get the path of this file and add it to @INC
	if ($0 =~ m#^(.+)/[^/]+$#) {
		push @INC, $1;
	}

	# if $resurrect is defined, Log4perl will be used
	$resurrect=":resurrect";
	if (defined($resurrect)) { 
		eval 'require Log::Log4perl';
		if ($@) {
			print STDERR "Log::Log4perl not found, doing without logging\n";
			$resurrect = undef;
		} else { 
			import Log::Log4perl qw(:resurrect get_logger); 
			import Log::Log4perl::Level; 
                        # TODO: define some kind of minimalistic logger
		}
	}
}


use 5.008000;
use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = ('register_job', 'deploy_for_job', 'remove_job', 'sweep', 'list_job_info', 'list_info', 'setstate', 'search');
our @EXPORT = ('process');
our $VERSION = '1.00';


use Janitor::Catalog;
use Janitor::Installer;
use Janitor::Execute;
use Janitor::Filefetcher;
use Janitor::ArcConfig;
use Janitor::Common qw(get_catalog);
use Janitor::Util qw(remove_directory manual_installed_rte sane_rte_name sane_job_id);
use Janitor::Job;
use Janitor::RTE;
use Janitor::TarPackageShared;
use Janitor::Request;
use Janitor::Response;







# set to one for debug output of some operations prior initialisation of
# Log4perl
my $DEBUG = 0;

######################################################################
# Read the config file
######################################################################
my $conffile = $ENV{'ARC_CONFIG'};
$conffile = "/etc/arc.conf" unless defined $conffile;

printf STDERR "DEBUG: using configuration file \"%s\"\n", $conffile	if $DEBUG;
if(! -e $conffile){
	printf STDERR "ERROR: Couldn't find configuration file \"%s\"\n", $conffile;
	return 4;
}

my $config = Janitor::ArcConfig->parse($conffile);
if ( !defined $config->{'janitor'} ) {
	printf STDERR "There is no valid [janitor]-section in \"%s\"\n", $conffile;
	return 4;
}

######################################################################
# set the effective gid
######################################################################
if (defined $config->{'janitor'}{'gid'}) {
	my $gn = $config->{'janitor'}{'gid'};
	my $gi = Janitor::Util::asGID($gn);
	unless (defined $gi) {
		printf STDERR "no such group: \"%s\"\n", $gn;
		return 4;
	}
	$) = $gi; # set the effective group id
	if ( $) != $gi ) {
		printf STDERR "FATAL: Can not set effective group id to %d: $!\n", $gi;
		return 4;
	}
	printf STDERR "setting effective group id to %s\n", $gn		if $DEBUG;
}

######################################################################
# set the effective uid
######################################################################
if (defined $config->{'janitor'}{'uid'}) {
	my $un = $config->{'janitor'}{'uid'};
	my $ui = Janitor::Util::asUID($un);
	unless (defined $ui) {
		printf STDERR "no such user: \"%s\"\n", $un;
		return 4;
	}
	$> = $ui; # set the effective user id
	if ( $> != $ui ) {
		printf STDERR "FATAL: Can not set effective user id to %d: $!\n", $ui;
		return 4;
	}
	printf STDERR "setting effective user id to %s\n", $un if $DEBUG;
}

######################################################################
# warn if we are running as root
######################################################################
if ($> == 0) {
	printf STDERR "Warning: running as user root.\n";
}
if ($) == 0) {
	printf STDERR "Warning: running with group root.\n";
}
	

# we want the runtime files to be accessible, so set a sane umask
# XXX maybe this should be configureable?
umask 0022;

######################################################################
# Logging setup
######################################################################
###l4p my $logconffile = $config->{'janitor'}{'logconf'};
###l4p Log::Log4perl->init( $logconffile ) if defined $logconffile;
###l4p  my $logger = get_logger();
# ARC libexec utilities are required to
# log to stderr - that channel is collected in
# some unified way and output then goes to client
# and/or administrator. Probably ARC specific parts
# should be isolated into dedicated module.
###l4p  my $stderr_layout = Log::Log4perl::Layout::SimpleLayout->new();
###l4p  my $stderr_appender = Log::Log4perl::Appender->new("Log::Log4perl::Appender::Screen");
###l4p  $stderr_appender->layout($stderr_layout);
###l4p  $logger->add_appender($stderr_appender);
###l4p  $logger->level($INFO);

######################################################################
#directory where we keep the current state
######################################################################
my $registrationDir = $config->{'janitor'}{'registrationdir'};
$registrationDir .= "/" unless $registrationDir =~ m#/$#;		#

my $msg = undef;
if ( ! -d $registrationDir) {
	$msg = "Directory $registrationDir does not exists\n";
} elsif (! -r $registrationDir) {
	$msg = "Can not read directory $registrationDir\n";
} elsif (! -w $registrationDir) {
	$msg = "Can not write to directory $registrationDir\n";
}
if (defined $msg) {
###l4p	$logger->fatal($msg);
	printf STDERR $msg;
	return 4;
}

######################################################################
# directories where we store out metadata
######################################################################

my $regDirJob = "$registrationDir/jobs";
my $regDirRTE = "$registrationDir/rtes";
my $regDirPackages = "$registrationDir/packages";

foreach my $dir ( ($regDirJob, $regDirRTE, $regDirPackages) ) {
	my $msg = undef;

	if (! -e $dir) {
		unless (mkdir $dir) {
			 $msg = "Can not create directory $dir: $!\n";
		}
	} elsif (! -d $dir) {
		$msg = "$dir is not a directory\n";
	} elsif (! -r $dir) {
		$msg = "Can not read directory $dir\n";
	} elsif (! -w $dir) {
		$msg = "Can not write to directory $dir\n";
	}
	
	if (defined $msg) {
###l4p		$logger->fatal($msg);
		printf STDERR $msg;
		return 4;
	}
}

######################################################################
#directory for installation of packages
######################################################################
my $instdir=$config->{'janitor'}{'installationdir'};
$instdir .= "/" unless $instdir =~ m#/$#;		#

######################################################################
#directory for downloads
######################################################################
my $downloaddir = $config->{'janitor'}{'downloaddir'};
$downloaddir .= "/" unless $downloaddir=~ m#/$#;		#

######################################################################
#manual installation directory for the site admin (the old way)
######################################################################
my $manualRuntimeDir = $config->{"grid-manager"}{"runtimedir"};
if (defined $manualRuntimeDir) {
	$manualRuntimeDir .= "/" unless $manualRuntimeDir =~ m#/$#;	#
}

#If a job is older than this, it is considered dead and will be removed by
#sweep if --force is given; value in seconds
my $jobExpiryTime = $config->{'janitor'}{'jobexpirytime'};
$jobExpiryTime = 7*24*60*60 unless defined $jobExpiryTime;	# default is 7d

#If a packages was not used for this time, it will be removed by sweep.
my $rteExpiryTime = $config->{'janitor'}{'rteexpirytime'};
$rteExpiryTime = 3*24*60*60 unless defined $rteExpiryTime;	# default is 3d





sub process{

	my ($request) =  @_;
	# TODO: NEEDED: check_rte_exist

	switch ($request->action()) {
		case Janitor::Request::REGISTER	{ 
			my @rteList = $request->rteList();
			if(defined $request->jobid() and defined $rteList[0]){
				return &register_job($request->jobid(), $request->rteList());
			}else{
				my $response = new Janitor::Response(Janitor::Response::REGISTER);
				$response->result(2, "Function was not called properly.");
				return $response;
			}
		}
		case Janitor::Request::DEPLOY	{
			if(defined $request->jobid()){
				return &deploy_for_job($request->jobid());
			}else{
				my $response = new Janitor::Response(Janitor::Response::DEPLOY);
				$response->result(2, "Function was not called properly.");
				return $response;
			}
		}
		case Janitor::Request::REMOVE	{
			if(defined $request->jobid()){
				return &remove_job($request->jobid());
			}else{
				my $response = new Janitor::Response(Janitor::Response::DEPLOY);
				$response->result(2, "Function was not called properly.");
				return $response;
			}
		}
		case Janitor::Request::SWEEP	{
			return &sweep($request->force());
		}
		case Janitor::Request::LIST	{
			return &list_info();
		}
		case Janitor::Request::INFO	{
			if(defined $request->jobid()){
				return &list_job_info($request->jobid());
			}else{
				my $response = new Janitor::Response(Janitor::Response::INFO);
				$response->result(2, "Function was not called properly.");
				return $response;
			}
		}
		case Janitor::Request::SETSTATE	{
			my @rteList = $request->rteList();
			if(defined $request->state() and defined $rteList[0]){
				return &setstate($request->state(), @rteList);
			}else{
				my $response = new Janitor::Response(Janitor::Response::SETSTATE);
				$response->result(2, "Function was not called properly.");
				return $response;
			}
		}
		case Janitor::Request::SEARCH	{
			my @rteList = $request->rteList();
			if(defined $rteList[0]){
				return &search(@rteList);
			}else{
				my $response = new Janitor::Response(Janitor::Response::SEARCH);
				$response->result(2, "Function was not called properly.");
				return $response;
			}
		}
	}

	my $response = new Janitor::Response(Janitor::Response::INFO);
	$response->result(2, "Function was not called properly!");
	return $response;
}

















######################################################################
# This function registers a job. If necessary it does some preparing
# for deploying the needed RTEs.
# Meaning of the return values:
#   0 - everything is fine. Needed RTEs are all installed
#   1 - it looks good, but some RTEs are not yet installed
#   2 - an error occured
# Parameter; Communicator class (JobID, DynamicRuntime)
# Returns:   Communicator class (Result)
######################################################################
sub register_job {

	my ($job_id, @rte_list) = @_;
	my $response = new Janitor::Response(Janitor::Response::REGISTER, $job_id, \@rte_list);

###l4p	$logger->info("$job_id: registering new job (@rte_list)");

	my $job = new Janitor::Job($job_id, $regDirJob);

	my $ret = $job->create(@rte_list);	

	unless (defined $ret) {
###l4p		$logger->fatal("Could not register job $job_id. Maybe there is already such a job registered.");
		$response->result(2, "Could not register job $job_id. Maybe there is already such a job registered.");
		return $response;
	}

	# maybe this job needs no RTE at all. Check this.
	if (scalar @rte_list == 0) {
###l4p		$logger->info("$job_id: sucessfully registered job");
		$job->initialize();
		$job->disconnect;
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$response->result(0, "Sucessfully initialized job.");
		return $response;
	}

	my @manual_runscripts;

	# At first we check what is manually installed.
	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my %manual = map { $_ => 1 } &manual_installed_rte($manualRuntimeDir);
		my @temp;
		foreach ( @rte_list ) {
			if (defined $manual{$_}) {
				push @manual_runscripts, $_;
			} else {
				push @temp, $_;
			}
		}
		@rte_list = @temp;
	}
	if (scalar @rte_list == 0) {
###l4p		$logger->info("$job_id: sucessfully registered job");
		$job->initialize(undef, @manual_runscripts);
		$job->disconnect;
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$response->result(0, "Sucessfully initialized job.");
		return $response;
	}

	# now we have in @rte_list a list of rtes which must be installed
	# automatically

	my $rte = new Janitor::RTE($regDirRTE);
	$rte->connect(@rte_list);

	my $state = $response->state($rte->state);

	if ($state == Janitor::RTE::FAILED) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: installation FAILED previously");
		$rte->disconnect;
		$job->remove;
		$response->result(2, "Can't provide requested RTEs: installation FAILED previously.");
		return $response;

	} elsif ($state == Janitor::RTE::REMOVAL_PENDING) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: its waiting for removal");
		$rte->disconnect;
		$job->remove;
		$response->result(2, "Can't provide requested RTEs: its waiting for removal.");
		return $response;

	} elsif ($state == Janitor::RTE::INSTALLED_A or $state == Janitor::RTE::INSTALLED_M
			or $state == Janitor::RTE::VALIDATED or $state == Janitor::RTE::BROKEN) {
		# its already there, maybe broken but still: It is there. So just use it.
		$rte->used($job_id);
###l4p		$logger->info("$job_id: sucessfully registered job");
		$job->initialize($rte->id, @manual_runscripts);
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$rte->disconnect;
		$job->disconnect;
		$response->result(0, "Sucessfully initialized job.");
		return $response;

	} elsif ($state == Janitor::RTE::INSTALLABLE or $state == Janitor::RTE::UNKNOWN) {
		# check if it is (really) installable. Maybe things changed...
		my $catalog = &get_catalog;
		my @bslist = $catalog->basesystems_supporting_metapackages(@rte_list);

		if ( @bslist ) {
			# ok, there is a way to deploy
###l4p			$logger->info("$job_id: sucessfully registered job");
			$rte->state(Janitor::RTE::INSTALLABLE);
			$rte->disconnect;
			$job->disconnect;
			$response->result(1, "Sucessfully initialized job.");
			return $response;
		} else {
###l4p 			$logger->info("$job_id: Can't provide requested RTEs: not supported");
#			The following two lines are replaced by the third line beneath this comment.
#			Reason: Excogitated RTE names are leading to this state, in which a folder remains
#				after the registration process. This is not desired for the amount is not limited!
#-			$rte->state(Janitor::RTE::UNKNOWN);
#- 			$rte->disconnect;
			$rte->remove;
			$job->remove;
			$response->result(2, "Can't provide requested RTEs: not supported.");
			return $response;
		}
	}

	# XXX this should never be executed

	$rte->disconnect;
	$job->disconnect;

	$response->result(1, "");
	return $response;
}

######################################################################
# Parameter; Communicator class (JobID)
# Returns:   Communicator class (Result)
######################################################################
sub deploy_for_job {

	my ($job_id) = @_;
	my $response = new Janitor::Response(Janitor::Response::DEPLOY, $job_id);

	my $ret;

	my $job = new Janitor::Job($job_id, $regDirJob);
	$ret = $job->open;
	unless (defined $ret and $ret == 0) {
###l4p		$logger->error("$job_id: Can't deploy: no such job");
		$response->result(1, "Can't deploy: no such job.");
		return $response;
	}

	my $state = $response->state($job->state);

	if ($state == Janitor::Job::INITIALIZED) {
		$job->disconnect;
###l4p		$logger->info("$job_id: already initialized");
		$response->result(0, "Already initialized.");
		return $response;
	} elsif ($state != Janitor::Job::PREPARED) {
		$job->disconnect;
###l4p		$logger->error("$job_id: unknown state, giving up");
		$response->result(1, "Unknown state, giving up.");
		return $response;
	}

	# The job is in the state prepared. So we can be sure, it needs
	# some RTEs which are currently not installed. But at least at
	# the time register was run, it was possible to deploy them.
	# This is not neccessarily true now. So we have to do all the
	# checks again.

	my @rte_list = $response->rteList([$job->rte]);
	my @manual_runscripts;

	# At first we check what is manually installed.
	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my %manual = map { $_ => 1 } &manual_installed_rte($manualRuntimeDir);
		my @temp;
		foreach ( @rte_list ) {
			if (defined $manual{$_}) {
				push @manual_runscripts, $_;
			} else {
				push @temp, $_;
			}
		}
		@rte_list = @temp;
	}
	if (scalar @rte_list == 0) {
		# whoooo, some friendly mind installed in the last
		# minutes everything manually
		$job->initialize(undef, @manual_runscripts);
		$job->disconnect;
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$response->result(0, "Sucessfully initialized job.");
		return $response;
	}

	# now we have in @rte_list a list of rtes which must be installed
	# automatically

	my $rte = new Janitor::RTE($regDirRTE);
	$rte->connect(@rte_list);

	$state = $response->state($rte->state);

	if ($state == Janitor::RTE::FAILED) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: installation FAILED previously");
		$rte->disconnect;
		$job->remove;
		$response->result(1, "Can't provide requested RTEs: installation FAILED previously.");
		return $response;
	} elsif ($state == Janitor::RTE::REMOVAL_PENDING) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: its waiting for removal");
		$rte->disconnect;
		$job->remove;
		$response->result(1, "Can't provide requested RTEs: its waiting for removal.");
		return $response;
	} elsif ($state == Janitor::RTE::INSTALLED_A or $state == Janitor::RTE::INSTALLED_M
			or $state == Janitor::RTE::VALIDATED or $state == Janitor::RTE::BROKEN) {
		# its already there, maybe broken but still: It is there. So just use it.
		$rte->used($job_id);
		$job->initialize($rte->id, @manual_runscripts);
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$rte->disconnect;
		$job->disconnect;
		$response->result(0, "Sucessfully initialized job.");
		return $response;

	} elsif ($state == Janitor::RTE::INSTALLABLE or $state == Janitor::RTE::UNKNOWN) {
		# check if it is (really) installable. Maybe things changed...
		my $catalog = &get_catalog;
		my @bslist = $catalog->basesystems_supporting_metapackages(@rte_list);
		unless ( @bslist ) {
###l4p 			$logger->info("$job_id: Can't provide requested RTEs: not supported");
			$rte->state(Janitor::RTE::UNKNOWN);
			$rte->disconnect;
			$job->remove;
			$response->result(1, "Can't provide requested RTEs: not supported.");
			return $response;
		}

		# ok, there is a way to deploy
		$rte->state($response->state(Janitor::RTE::INSTALLABLE));

		eval {
			$ret = deploy_rte_for_job($job_id, $rte, \@rte_list, $catalog, \@bslist);
		};
		if ($@ or $ret != 0) {
###l4p			$logger->error("$job_id: Can't provide requested RTEs: installation FAILED (".$@.")");
			$rte->state($response->state(Janitor::RTE::FAILED));
			$rte->disconnect;
			$job->remove;
			$response->result(1, "Can't provide requested RTEs: installation FAILED.");
			return $response;
		}

		$rte->state($response->state(Janitor::RTE::INSTALLED_A));
		$rte->used($job_id);
		$job->initialize($rte->id, @manual_runscripts);
###l4p		$logger->info("$job_id: sucessfully initialized job");

		$rte->disconnect;
		$job->disconnect;
		$response->result(0, "Sucessfully initialized job.");
		return $response;
	}

	$rte->disconnect;
	$job->disconnect;
	
	printf "XXX dies sollte niemals ausgeführt werden!!!\n";

	$response->result(1, "");
	return $response;
	
}

######################################################################
# This function is used for actually deploying the necessary parts of
# a RTE. Before calling it is not checked if they are already deployed.
# Checking this is the task of this function.
######################################################################
sub deploy_rte_for_job {
	my ($job_id, $rte, $RTE, $catalog, $bslistref) = @_;

	# we got a list of basesytems supporting this RTE. For now we use just
	# the first one for deploying
	my $bs = $$bslistref[0];

###l4p	$logger->info("$job_id: use basesystem \"".$bs->name."\" for dynamically installed rte");

	my (undef, @packages) = $catalog->PackagesToBeInstalled($bs, @$RTE);
			
	# Check all requested packages, install them if necessary
	foreach my $p ( @packages ) {
		my $package_id = setup_runtime_package($p, $job_id, $catalog, $rte->id);

		unless (defined $package_id) {
			my $msg = "$job_id: Error while installing " . $p->url;
###l4p			$logger->error($msg);
			return 1;				
 		}

		$rte->attach_data($package_id);
 	}
	return 0;
}

######################################################################
# Installes the requested package $p for job $job. It returns the name of the
# runtime file. If some error occurs undef is returned.
######################################################################
sub setup_runtime_package {
	my ($p, $job, $catalog, $used_by) = @_;

###l4p 	$logger->debug("processing package " . $p->url . " (Catalog: " . $catalog->name . ")");
	unless ($p->isa("Janitor::Catalog::TarPackage")){
###l4p 		$logger->fatal("Currently only TarPackages are supported.");
		return undef;	
	}

	my $tarfile;
	my $runscript = "";
		
	my $url = $p->url;
	my $url_fetched = 0; 

	# match name of tar file
	$tarfile=$url;
	if ($url =~ m#/([^/]*)$#) { 	#
			 $tarfile = $1;
	}

	my $tarpackage = new Janitor::TarPackageShared($tarfile, $regDirPackages);
	$tarpackage->connect;
	my $id = $tarpackage->id;

	my $state = $tarpackage->state;
	if (! defined $state) {
		print STDERR "package state not defined!\n";
	}
	if ($state == Janitor::TarPackageShared::INSTALLED) {
		# everything available and nothing to do
		# XXX
		$tarpackage->used($used_by);
		$tarpackage->disconnect;
		return $id;
	}

	if ($state != Janitor::TarPackageShared::UNKNOWN) {
		# unknown state
		$tarpackage->disconnect;
		return undef;
	}

	# ok, so we are in the state UNKNOWN, which means we have to install

###l4p 	$logger->info("installing package " . $p->url . " (Catalog: " . $catalog->name . ")");
#
	#Check if the tarfile is in our filessystem and accessible. If not fetch it.
	unless ($url =~ m#^/# and -r $url) {
		my $ff = new Janitor::Filefetcher($url,$config->{'janitor'}{'downloaddir'});
		unless ($ff->fetch()) {
			$tarpackage->remove;
			my $msg = "Error while fetching $url\n";
###l4p 			$logger->error($msg);
			return undef;				
		}
		$url = $ff->getFile();
		$url_fetched=1;
	}

	#Install the Package
	my $installer = new Janitor::Installer($instdir);
	my ($ret, $installed) = $installer->install($url);

	# Create a symbolic link within the runtime directory.
	# Create a symbolic link within the runtime directory.
	if(defined $installed){
		my $e = new Janitor::Execute($installed);
		my $runtimedir = $config->{'grid-manager'}{'runtimedir'};
		# Query, if there is a metapackage name for the installed basepackage
		# Even if the user has not intended to install a certain matapackage,
		# this might have happened due to dependencies.
		my @mps = $catalog->MetapackagesByPackageKey($p->id);
		if(scalar(@mps) > 1){
###l4p			$logger->info("Several Metapackges are refering to one installed package!");
		}
		for my $mp(@mps){
			my @mpNameTokens = split(/\//,$mp->name());
			my $currentpath = $runtimedir;
			my $result      = 1;
			for (my $i = 0; $i < scalar @mpNameTokens - 1; $i++){
				$currentpath = $currentpath."/".$mpNameTokens[$i];
				if(! -d $currentpath){
					$result = $e->execute("mkdir", $currentpath);
					if( $result != 1){
###l4p 						$logger->error("Failed creating directory ".$currentpath." for ".$mp->name());
						last;
					}
# 					print "mkdir ".$currentpath."  =>  $result\n";
				}
			}
			$currentpath = $currentpath."/".pop(@mpNameTokens);
			if( (! -e $currentpath) and (! -l $currentpath) and $result == 1){
				$result = $e->execute("ln", "-s", $installed."/runtime", $currentpath);
				if($result != 1){
###l4p 						$logger->error("Failed creating symbolic link ".$currentpath);
				}
# 				print "ln -s ".$installed."/runtime ".$currentpath."  =>  $result\n";
			}
		}
	}


	unlink($url) if $url_fetched;

	unless ($ret) { 
		# installation failed. clean up and exit
		$tarpackage->remove;

###l4p 		$logger->error("Installation of $tarfile failed.");
		return undef;
	};

	$tarpackage->installed($installed, "$installed/runtime");
	$tarpackage->used($used_by);
	$tarpackage->disconnect;

	return $id;
}


######################################################################
# removes job $job_id from janitors db.
# XXX Hack: if $sweep is true the function is not called directly
# XXX but by force sweeping. In this case $job_id is not a job_id
# XXX but a already opened Janitor::Job object.
######################################################################
sub remove_job {

	my ($job_id, $sweep) = @_;
	my $response = new Janitor::Response(Janitor::Response::REMOVE, $job_id);

	my $job;

	if (defined $sweep and $sweep) {
		$job = $job_id; # already opened Janitor Job object
		$job_id = $response->jobid($job->id);

###l4p		$logger->info("force sweeping job $job_id");
	} else {
		$response->jobid($job_id);
###l4p		$logger->info("removing job $job_id");



		$job = new Janitor::Job($job_id, $regDirJob);
		if ($job->open != 0) {
###l4p			$logger->error("$job_id: no such job");
			$response->result(0, "No such job.");
			return $response;
		}
	}

	my $rte_key = $job->rte_key;
	if (defined $rte_key and $rte_key ne "") {
		my $rte = new Janitor::RTE($regDirRTE, $job_id);
		$rte->reconnect($rte_key);
		$rte->unuse_it($job_id);
		$rte->disconnect;
	}

	$job->remove;
###l4p	$logger->info("$job_id: removed");
	$response->result(0, "Removed.");
	return $response;
}


######################################################################
# Removes a single package. It does not check, if it is used.
#
# If an error occurs while removing the installation directory it just prints
# an error message. The registration directory is removed neverless. This way
# the installation directory will not be used anymore. It is orphaned and the
# local site admin has to chek what went wrong and remove it.
######################################################################
sub sweep_package
{
	my ($package) = @_;

###l4p 	$logger->info("Removing ". $package->id);

	my $instdir = $package->inst_dir;

	my $error_flag = 0;

	#execute the remove script
	my $e = new Janitor::Execute($instdir."/pkg");
	if ( $e->execute("/bin/sh", "../remove") ){
		#unlink the installation directory
		unless ( remove_directory($instdir) ) {
###l4p 			$logger->warn("Removing directory $instdir failed.");
			$error_flag = 1;
		}
	} else {
###l4p 		$logger->warn("Execution of remove script in $instdir failed.");
###l4p 		$logger->warn("So I don't delete $instdir! But I will orphan it.");
		$error_flag = 1;
	}

	$package->remove;#  or $error_flag = 1;

	return $error_flag == 0;		
}

######################################################################
# Removes a RTE.
######################################################################
sub sweep_rte {
	my ($rte) = @_;

	# Remove the symbolic link within the runtime directory.

	my $runtimedir = $config->{'grid-manager'}{'runtimedir'};
	my $e = new Janitor::Execute($runtimedir);
	foreach my $mp($rte->rte_list()){
		my $currentpath = $runtimedir."/".$mp;
		if(-l $currentpath){
			my $result = $e->execute("rm", $currentpath);
			if($result != 1){
###l4p 				$logger->error("Failed removing symbolic link ".$currentpath);
			}
# 			print "rm ".$currentpath."  =>  $result\n";
		}
	}

###l4p 	$logger->info("Removing ". join " ", $rte->rte_list);
	foreach my $p ($rte->retrieve_data) {
		if(defined $p){ # In case job registration fails, $p may be undefined
			my $package = new Janitor::TarPackageShared($p, $regDirPackages);
			$package->connect;
			eval { $package->not_used($rte->id); };
			if ($@) {
				printf STDERR "Error while removing used flag on package: $@\n";
			}
			$package->disconnect;
		}
	}

	$rte->remove;
}

######################################################################
# check for unused RTEs and remove them. If $force is true force_sweep
# will be called which removes old jobs.
######################################################################
sub sweep {
	# TODO: Return sweeped jobs and packages
	my ($force) = @_;
	my $response = new Janitor::Response(Janitor::Response::SWEEP);

	my $now = time;

	# at first look for old jobs
	foreach my $job (Janitor::Job::all_jobs($regDirJob)) {	
		if ($job->open != 0) {
			next;
		}

		my $created = $job->created;
		if (! defined $created) {
###l4p 			$logger->warn("Job " . $job->id . " has no creation time. Please check and remove manually.");
		} elsif ($now - $created > $jobExpiryTime) {
			if (defined $force and $force) {
				remove_job($job, 1);
				next;
			} else {
###l4p 				$logger->info("Job " . $job->id . " is old. Use --force to remove.");
			}
		}

		$job->disconnect;
	}

	# now go through the RTEs
	foreach my $id (Janitor::RTE::all_rtes($regDirRTE)) {
		my $rte = new Janitor::RTE($regDirRTE, "SWEEP");
		$rte->reconnect($id) or next;

		my $flag = 0;

		eval {
			my $last_used = $rte->last_used;
			my $state = $rte->state;
			if ($rte->used != 0) {
				# skip it
			} elsif ($state == Janitor::RTE::REMOVAL_PENDING) {
				# unused and waiting for removal
				sweep_rte($rte);
				$flag = 1;
			} elsif (! defined $last_used) {
###l4p 				$logger->warn("RTE " . $rte->id . " has no last_used time. Please check manually.");
			} elsif ($state == Janitor::RTE::INSTALLED_A and $now - $last_used > $rteExpiryTime) {
				# automatically installed and not used for some time
				sweep_rte($rte);
				$flag = 1;
			} elsif ($state == Janitor::RTE::FAILED ) {
###l4p 				$logger->fatal("Runtime environment ".$rte->id." has a failed state. Please set state to removal pending in order to remove them.");
			} elsif ($state == Janitor::RTE::UNKNOWN){ # may still be INSTALLED_A
###l4p 				$logger->fatal("Runtime environment ".$rte->id." has an unknown state.");
			}
		};
		if ($@) {
			printf STDERR "Error while trying to remove a RTE!: $@\n";
		}

		$rte->disconnect unless $flag;
	}

	# and now look for unused packages
	foreach my $package (Janitor::TarPackageShared::all_packages($regDirPackages)) {
		$package->reconnect or next;

		if (! $package->used) {
			sweep_package($package);
		} else {
			$package->disconnect;
		}
	}

	$response->result(0, "Sweeping is done.");
	return $response;
	
}


######################################################################
# Returns the difference between the timestamps t1 and t2 in a nice format.
######################################################################
sub timestamp_diff {
	my ($t1, $t2) = @_;

	my $d = $t1 - $t2;
	my $ret;

	if ($d < 100) {
		$ret = sprintf "%d seconds", $d;
	} elsif ($d < 100 * 60) {
		$ret = sprintf "%d minutes", $d/60;
	} elsif ($d < 100 * 60 * 60)  {
		$ret = sprintf "%d hours", $d/60/60;
	} else {
		$ret = sprintf "%d days", $d/60/60/24;
	}

	return $ret;
}

######################################################################
# Lists information about job $jobid.
# Parameter; Communicator class (JobID)
# Returns:   Communicator class (RunningJobs)
######################################################################
sub list_job_info {
	my ($jobid) = @_;
	my $response = new Janitor::Response(Janitor::Response::INFO, $jobid);
	my $job = new Janitor::Job::($jobid, $regDirJob);



	if ($job->open != 0) {
		$response->result(1,"No such job.");
		return $response;
	}

	my $created;
	my $rte;
	my $state;
	my @manual;
	my $rte_key;
	my @runtime;

	eval {
		$created = $job->created;
		$rte =  join " ", $job->rte;
		$state = $job->state;
		if ($state == Janitor::Job::INITIALIZED) {
			@manual = $job->manual;
			$rte_key = $job->rte_key;
			if (defined $rte_key) {
				my $rte = new Janitor::RTE($regDirRTE);
				$rte->reconnect($rte_key);
				my @package_names = $rte->retrieve_data;
				$rte->disconnect;
					
				foreach my $package_name ( @package_names ) {
					my $package = new Janitor::TarPackageShared(
						$package_name, $regDirPackages);
					$package->connect();
					push @runtime, $package->runtime;
					$package->disconnect();
				}
			}
		}
	};



	if ($@) {
		$job->disconnect;
		$response->result(2, "Error while retrieving job information.");
		return $response;
	}

	$job->disconnect;

	
	my @uses    = ();
	
	if ($state == Janitor::Job::PREPARED) {
		$state   = "PREPARED";
	} elsif ($state != Janitor::Job::INITIALIZED) {
		$state   = "unknown state (bug?)";
	} else {
		$state   = "INITIALIZED";
		$rte_key = $rte_key if defined $rte_key;
		push(@uses, (@manual,@runtime));
# 		foreach ( @manual, @runtime ){ print  "uses:".$_."\n";}
	}

	$response->runningJob($jobid, $created, timestamp_diff(time, $created), $state, $rte, $rte_key, \@uses);
	$response->result(0,"Successfully retrieved job information.");


	return $response;
}

######################################################################
# Generate a list of all registerd jobs
# Parameter; (none)
# Returns: Communicator class (RunningJobs, LocalRuntime, DynamicRuntime, InstallableRuntime)
######################################################################
# XXX It's a bad idea to do a lot of printf while something locked. 
# XXX The printf might be blocked. Especially if somebody uses tools
# XXX like "watch"!!! This is the explanation for all this "useless"
# XXX variables.
######################################################################
sub list_info {

	my $response = new Janitor::Response(Janitor::Response::LIST);

	my $now = time;
	my $output = "";

	foreach my $job ( Janitor::Job::all_jobs($regDirJob) ) {	
		if ($job->open != 0) {
			next;
		}

		my $state = $job->state;
		if ($state == Janitor::Job::INITIALIZED) {
			$state = "INITIALIZED";
		} elsif ($state == Janitor::Job::PREPARED) {
			$state = "PREPARED";
		} else {
			$state = "unknown state (bug?)";
		}

		my $created = $job->created;
		my $age = (defined $created)
				? timestamp_diff($now, $created)
				: "unknown";

		my $id = $job->id;
		my $rte = join " ", $job->rte;

		$job->disconnect;

 		$response->runningJob($id, $created, timestamp_diff(time, $created),$state, ($rte), undef, undef);
	}
	

	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my @runscripts =  &manual_installed_rte($manualRuntimeDir);
		if (scalar @runscripts != 0 ) {
			foreach my $packagename(@runscripts){
				$response->runtimeLocal(($packagename));
			}
		}
	}

	my @rtes = Janitor::RTE::all_rtes($regDirRTE);
	foreach ( @rtes ) {
		my $rte = new Janitor::RTE($regDirRTE);
		$rte->reconnect($_) or next;

		my $rte_list;
		my $state;
		my $last_used;
		my @used;

		eval {
			$rte_list = join " ",  $rte->rte_list;
			$state = Janitor::RTE::state2string($rte->state);
			$last_used = $rte->last_used;
			@used      =  $rte->used;
		};

		$rte->disconnect;
		$response->runtimeDynamic(($rte_list), $state, $last_used, \@used);
	}

	my $catalog = &get_catalog;
	my @list = $catalog->MetaPackages;
	foreach my $package(@list) {

		my @support = $catalog->basesystems_supporting_metapackages($package->name);
		my $description = $package->description;
		$description =~ s/[\r\n]/ /g;
		$response->runtimeInstallable(($package->name), $description, (scalar @support != 0));

	}
	

	$response->result(0,"Successfully retrieved information.");

 	return $response;
}

######################################################################
# Sets the state of a RTE to a new value. But we can only do a
# few state changes.
# 
#
######################################################################
sub setstate {
	my ($state, @rte) = @_;
	my $response = new Janitor::Response(Janitor::Response::SETSTATE);

	my $newstate = Janitor::RTE::string2state($state);
	unless (defined $newstate) {
		print STDERR $state . ": not a valid statename\n";
		$response->result(1,$state.": is not a valid state.");
		return $response;
	}

	my $rte = new Janitor::RTE($regDirRTE, "SETSTATE");

	$rte->connect(@rte);
	my $oldstate = $rte->state;

	if ($oldstate == $newstate) {
		$rte->disconnect;
		$response->result(0,"Changed state successfully.");
		return $response;
	}

	my $f = 0;

	# b like blob.
	my $b;
	$b |= (1 << Janitor::RTE::INSTALLED_M);
	$b |= (1 << Janitor::RTE::INSTALLED_A);
	$b |= (1 << Janitor::RTE::BROKEN);
	$b |= (1 << Janitor::RTE::VALIDATED);

	# this implements the manual state changes
	if ((1 << $oldstate) & $b) {
		$f = ((1 << $newstate) & $b or $newstate == Janitor::RTE::REMOVAL_PENDING);
	} elsif ($oldstate == Janitor::RTE::FAILED) {
		$f = ($newstate == Janitor::RTE::REMOVAL_PENDING);
	}

	if ($f) {
		eval {
			$rte->state($newstate);
		};
		if ($@) {
			printf STDERR "Can not change the state: %s\n", $@;
			$rte->disconnect;
			$response->result(1,"Can not change the state ".$@.".");
		}
		$rte->disconnect;
		$response->result(0,"Changed state successfully.");
		return $response;
	}
	
	$rte->disconnect;
	$response->result(1,"Changed state was not successful.");
	return $response;
}

######################################################################
#
#
#
#
######################################################################


sub search(){

	my (@rte) = @_;
	my $response = new Janitor::Response(Janitor::Response::SEARCH, undef, \@rte);

	my $hitcount = 0;
	my $output = "";

	# At first we check what is manually installed.
	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my @manual = &manual_installed_rte($manualRuntimeDir);
		my $val;
		my $valRTE;
		foreach my $val(@manual){
			foreach $valRTE(@rte){
				if ($val =~ m/$valRTE/i ) {
 					$response->runtimeLocal($val);
					$hitcount++;
					last;
				}
			}	
		}
	}

	my $catalog = &get_catalog;
	my @list = $catalog->MetaPackages;
	foreach my $package(@list){
		my @support = $catalog->basesystems_supporting_metapackages($package->name);
		my $description = $package->description;
		$description =~ s/[\n]*//;

		my $valRTE;
		foreach $valRTE(@rte){
			if($package->name =~ m/$valRTE/i){
 				$response->runtimeInstallable($package->name, $description, (scalar @support != 0));
# 				$output .= sprintf($format, "lastupdate:", $package->lastupdate) if defined $package->lastupdate;
				$hitcount++;
				last;
			}
		}
	}

	$response->result(0,"Search sucessfully finished.");
	return $response;
}



# 1;
__END__

=head1 CONFIGURATION FILES

=head2 F<arc.conf>

The Janitor can be configured in the [janitor]-section of the F<arc.conf>. 
It is addressed using the environment variable ARC_CONFIG.
The following options are supported.

=over 4

=item basedir

Location of the basedir of the janitor installation. Used by the ARC backend scripts
to find the Janitors modules.

=item logconf

Location of the Log4perl configuration file.

=item registrationdir

The directory the Janitor uses to keep track of registered jobs and dynamically
installed REs.

=item installationdir

The directory dynamically REs are installed.

=item downloaddir

A directory the Janitor uses to store downloaded files.

=item rteexpirytime

A time in seconds. REs which are unused for this time are removed by sweep.

=item jobexpirytime

A time in seconds. Jobs which are older than this are removed by sweep --force.

=item uid, gid

The user and group id to use.

=item allow_base, deny_base

With this options the admin may choose which kind of basesystems are allowed.
Initially all basesystems are forbidden. In the first step the allow_base
options are processed and everything matching them is allowed. In the second
step the deny_base options are processed and everything matching them is
forbidden. Multiple allow_base and deny_base options are allowed.

=item allow_rte, deny_rte

Like allow_base and deny_base, but for REs.

=back

Additionally, there is a section [janitor/name] in the arc.conf for each catalog
used. "name" is the (arbitrary) name of the catalog. Only one option is
supported:

=over 4

=item catalog

The location of the catalogs RDF file. Currently this must be a local file.
Remotely managed Catalogs must be downloaded first.

=back

=head2 F<log.conf>

The F<log.conf> file is used to configure the Log4perl logger. The location of
this file is given in the [janitor]-section of the F<arc.conf>. For examples how
to configure Log4perl see the Log4perl documentation.

=head1 ENVIRONMENT

=over 4

=item ARC_CONFIG

Location of the nordugrid arc.conf. If unset /etc/arc.conf is used.

=back

=head1 EXAMPLE

 [janitor]
 basedir="/opt/janitor"
 logconf="/opt/janitor/work/log.conf"
 registrationdir="/opt/janitor/work/reg"
 installationdir="/grid/runtime/janitor"
 downloaddir="/opt/janitor/work/download"
 jobexpirytime="10000000"
 uid="janitor"
 gid="janitor"
 allow_base="*tar*"
 allow_rte="*"
 
 [janitor/nordugrid]
 catalog="/opt/janitor/catalog/knowarc.rdf"
 
 [janitor/bioc]
 catalog="/opt/janitor/catalog/bioc_catalog.rdf"

=head1 SEE ALSO

=over 4

=item http://dre.knowarc.eu:8080

=item http://www.nordugrid.org

=back

=cut
