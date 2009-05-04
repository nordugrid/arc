#!/usr/bin/perl -w

use Switch;
use warnings;
use strict;


# TODO: create a conf file for that.
# TODO: remove the name janitor as good as possible
# TODO: Ensure that uid="janitor" and gid="grid" can be configured using the SecHandler

BEGIN {
	# get the path of this file and add it to @INC
	if ($0 =~ m#^(.+)/[^/]+$#) {
		push @INC, $1;
	}

	# if $resurrect is defined, Log4perl will be used
	my $resurrect=":resurrect";

	if (defined($resurrect)) { 
		eval 'require Log::Log4perl';
		if ($@) {
			print STDERR "Log::Log4perl not found, doing without logging\n";
			$resurrect = undef;
		} else { 
			import Log::Log4perl qw(:resurrect get_logger); 
		}
	}
}



use Janitor::Catalog;
use Janitor::Common qw(get_catalog refresh_catalog);
use Janitor::Config;

use Janitor::Installer;
use Janitor::Execute;
use Janitor::Filefetcher;
use Janitor::Config;
use Janitor::Util qw(remove_directory manual_installed_rte sane_rte_name sane_job_id);
use Janitor::Job;
use Janitor::RTE;
use IO::Pipe;
use Janitor::TarPackageShared;
use POSIX qw(EAGAIN WNOHANG);
use XML::LibXML;

#use Socket qw(SOCK_STREAM);
#use IO::Socket::INET;
#use POSIX qw(EAGAIN WNOHANG);

######################################################################
# Read the config file
######################################################################
# export NORDUGRID_CONFIG="/nfshome/knowarc/dredesign/src/services/dRE5/perl/conf/arc.conf"
my $conffile = $ENV{'NORDUGRID_CONFIG'};
my $DEBUG = 0;
$conffile = "/etc/arc.conf" unless defined $conffile;

printf STDERR "DEBUG: using configuration file \"%s\"\n", $conffile	if $DEBUG;
my $config = Janitor::Config->parse($conffile);

if ( !defined $config->{'janitor'} ) {
	printf STDERR "There is no valid [janitor]-section in \"%s\"\n", $conffile;
	#exit 1;
}

######################################################################
# Logging setup
######################################################################


my $LOGCONF = $config->{'janitor'}{'logconf'};
		               # Configuration file for the logger
                               # if undef, built-in configuration "perl/conf/log.conf" will be used
                               # i.e. logconf="/opt/nordugrid/etc/log.conf"

###l4p my $logconffile = $LOGCONF;
###l4p $logconffile = "perl/conf/log.conf" unless defined $logconffile;
###l4p Log::Log4perl->init( $logconffile );
###l4p my $logger = get_logger();

######################################################################
# Configuration

                               # Directory, where the current state will be kept
                               # if undef, the directory "perl/spool/" will be used 
                               # registrationdir="/var/spool/nordugrid/janitor"
my $registrationDir = $config->{'janitor'}{'registrationdir'};
                               # Directory in which the runtime environemente will be installed to
                               # if undef, the directory "perl/spool/runtime/" will be used 
                               # i.e. installationdir = "/var/spool/nordugrid/janitor/runtime"
my $installationdir = $config->{'janitor'}{'installationdir'};
                               # Directory in which the packages will be downloaded to
                               # if undef, the directory "perl/spool/download/" will be used 
                               # i.e. downloaddir = "/var/spool/nordugrid/janitor/download"
my $downloaddir     = $config->{'janitor'}{'downloaddir'};

#catalogrefresh="90"

#allow_base="*"
#allow_rte="*"
#catalog="/var/spool/nordugrid/janitor/catalog/knowarc.rdf"
#
######################################################################


######################################################################
# Global variables

my $GLOBAL_CATALOG = undef; # used to hold the catalog
my $next_refresh   = time;


# Format for the search/list results. If this gets modified - the interface
# to other programs may be changed in a critical way.
my $format = "  %-12s %s\r\n"; 

	# my $catalog = 12;
	# print  "CATALOG:  $catalog\n\n";
#
######################################################################



######################################################################
# Setup 
#

$GLOBAL_CATALOG = undef;

## Registration directory setup
##
$registrationDir = "perl/spool/" unless defined $registrationDir;
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
	return 1;
}

my $regDirJob = "$registrationDir/jobs";           # Metadata jobs
my $regDirRTE = "$registrationDir/rtes";           # Metadata Runtime Environements
my $regDirPackages = "$registrationDir/packages";  # Metadata Packages

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
		return 1;
	}
}

## Installation and download directory setup
##
$installationdir      = "perl/spool/runtime/" unless defined $installationdir;
$installationdir     .= "/" unless $installationdir =~ m#/$#;		#
$downloaddir  = "perl/spool/download/" unless defined $downloaddir;
$downloaddir .= "/" unless $downloaddir=~ m#/$#;	

##manual installation directory for the site admin (the old way)
##
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

#
######################################################################




# triggerAction("SWEEP","","");  # ADMIN LEVEL
#  triggerAction("SWEEP","","1");  # ADMIN LEVEL  FORCE
# 
#  triggerAction("SEARCH","","wEkA\r\nblalb\r\nexon");
# 
#  
#  triggerAction("REGISTER","4321","APPS/BIO/WEKA-3.4.10");
#  triggerAction("DEPLOY","4321","");
#   triggerAction("CHECK","4321","");
#  triggerAction("LIST","","");   # ADMIN LEVEL
#  triggerAction("REMOVE","4321","");
# triggerAction("LIST","","");   # ADMIN LEVEL
#  triggerAction("SWEEP","","1");  # ADMIN LEVEL  FORCE
# 
# # triggerAction("LIST","","");   # ADMIN LEVEL
#  triggerAction("REGISTER","4322","APPS/BIO/WEKA-3.4.10");
#  triggerAction("DEPLOY","4322","");
#  triggerAction("CHECK","4322","");
#  triggerAction("REMOVE","4322","");
# triggerAction("CHECK","4322","");

# 
# triggerAction("LIST","","");   # ADMIN LEVEL



# triggerAction("REGISTER","4322","bumhug");
# triggerAction("DEPLOY","4322","");
# 

# triggerAction("REGISTER","4321","ENV/DEVELOPMENT/GCC-4.1");  # NOT TAR -> WILL FAIL
# triggerAction("DEPLOY","4321","");
# 
# triggerAction("LIST","","");   # ADMIN LEVEL
# triggerAction("CHECK","4321","");
# triggerAction("REMOVE","4321","");
# triggerAction("LIST","","");   # ADMIN LEVEL




print "Perl is initialized.\n";
my $output =  process("<soap-env:Envelope xmlns:b=\"urn:dynamicruntime\" xmlns:soap-enc=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:soap-env=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"><soap-env:Body><b:Request action=\"DEPLOY\"><b:Initiator jobid=\"4321\"/><b:Runtimeenvironment type=\"dynamic\"><b:package name=\"APPS/BIO/WEKA-3.4.10\"/><package name=\"APPS/BIO/WEKA-3.4.11\"/></b:Runtimeenvironment></b:Request></soap-env:Body></soap-env:Envelope>");
print $output."yy\n";


sub process{

	my ($xmlstring) = @_;

	my $pipe   = new IO::Pipe;
	my $child_pid = undef;
	my $timeout = 300;
	my $answer = undef;

	# Reason to do a fork here:
	#   there is a (very) small propability, that something
	#   unexpected my happen. In that case, the alarm function will
	#   timeout and kill the affected process. The parent process on is 
	#   the otherhand is uneffected by this procedure.
	FORK:
	{
		if ($child_pid = fork) {
			# parent
			$pipe->reader();
		} elsif (defined $child_pid) {
			# child
			$pipe->writer();
			alarm($timeout);	# after a certain time the signal alarm will be send to kill the process
			$SIG{CHLD} = 'DEFAULT';

			my $ret = undef;
			eval {$ret = processXML($xmlstring);};
			alarm(0);
			if($@){
				# TODO:makefault
				print $@;
				print $pipe $@."\r\n";
			}else{
				print $pipe $ret."\r\n";
			}
			exit;
			} elsif ($! == EAGAIN) {
				# recoverable error
				sleep 1;
				redo FORK;
			} else {
				# makefault - real ERROR
				print  "500 can not fork\r\n";
				$answer = "500 can not fork\r\n";
			}# parent child etc.
	}# FORK

	if(!defined $answer){
		waitpid($child_pid,0);
		while(!$pipe->eof){
			my $temp = <$pipe>;
			print $temp;
			if(!defined $answer){ $answer = "";}
			$answer = $answer.$temp;
		}
		$pipe = undef;
	}

	if(!defined $answer){
		# TODO: makefault
		$answer = "\nAn error occured while the requested action was performed (timeout was set to $timeout).\n";
	}

	return $answer;
}



sub processXML{


	my ($xmlstring) = @_;

	print "\n\n\n".$xmlstring."\n\n\n";

	# http://www.gsp.com/cgi-bin/man.cgi?section=3&topic=XML::LibXML

	my $parser = new XML::LibXML;
	my $struct = $parser->parse_string($xmlstring);
	my $inRoot = $struct->getDocumentElement();

	# Check namespaces of the soap envelope
	my $prefixSOAPenv = $inRoot->lookupNamespacePrefix("http://www.w3.org/2003/05/soap-envelope"); # SOAP 1.2
	my $prefixSOAPenc = $inRoot->lookupNamespacePrefix("http://www.w3.org/2003/05/soap-encoding"); # SOAP 1.2
	if(!defined $prefixSOAPenv){
		$prefixSOAPenv = $inRoot->lookupNamespacePrefix("http://schemas.xmlsoap.org/soap/envelope/");
	}
	if(!defined $prefixSOAPenc){
		$prefixSOAPenc = $inRoot->lookupNamespacePrefix("http://schemas.xmlsoap.org/soap/encoding/");
	}
	if(!defined $prefixSOAPenv || !defined $prefixSOAPenc){
		print "Received string is not valid SOAP.\n";
		return "makefault"; # makefault is not a soap-message
	}

	my $prefixWS   = $inRoot->lookupNamespacePrefix("urn:dynamicruntime");

	# Get the SOAP Body
	my @nodeList  = $inRoot->getChildrenByTagName($prefixSOAPenv.":Body");
	if(scalar @nodeList != 1){
		print "Error occured while trying to access the Body element of soap(".(scalar @nodeList).", ".$prefixSOAPenv.":Body".")\n";
		return "makefault"; # makefault is not a soap-message
	}
	my $nodeSoapbody = $nodeList[0];

	# Get the Service Request (must exist)
	@nodeList  = $nodeSoapbody->getChildrenByTagName($prefixWS.":Request");
	if(scalar @nodeList != 1){
		print "Error occured while trying to access the Request element (".(scalar @nodeList)."," .$prefixWS.":Request".")\n";
		return "makefault"; # makefault is not a soap-message
	}
	my $nodeRequest = $nodeList[0];

	# Get the Service Initiator (must not exist)
	@nodeList  = $nodeRequest->getChildrenByTagName($prefixWS.":Initiator");
	my $nodeInitiator = undef;
	if(scalar @nodeList != 1){
		print "Error occured while trying to access the Initiator element (".(scalar @nodeList)."," .$prefixWS.":Initiator".")\n";
		return "makefault"; # makefault is not a soap-message
	}else{
		$nodeInitiator = $nodeList[0];
	}

	# Get the Service RuntimeEnvironmentList (must not exist)
	my @nodeRuntimeEnvironmentList = $nodeRequest->getChildrenByTagName($prefixWS.":Runtimeenvironment");
	my @dynamic_runtime     = undef;   # list of hashes
	my @local_runtime       = undef;
	my @installable_runtime = undef;
	for (@nodeRuntimeEnvironmentList){
		@nodeList  = $_->getChildrenByTagName($prefixWS.":Package");		
		my $type = $_->getAttribute("type");
		
		if(defined $type){
			if($type eq "dynamic"){
				foreach my $package(@nodeList){
					push(@dynamic_runtime, (
						name => $package->getAttribute("name")
						# description etc.
						)
					);
				}
			}elsif($type eq "local"){
				foreach my $package(@nodeList){
					push(@local_runtime, (
						name => $package->getAttribute("name")
						# description etc.
						)
					);
				}
			}elsif($type eq "installable"){
				foreach my $package(@nodeList){
					push(@installable_runtime, (
						name => $package->getAttribute("name")
						# description etc.
						)
					);
				}
			}
		}
	}

	my %task = (
		action  => $nodeRequest->getAttribute("action"),
		jobid   => $nodeRequest->getAttribute("jobid"),
		dynamic_runtime     => @dynamic_runtime,
		local_runtime       => @local_runtime,
		installable_runtime => @installable_runtime
	);
	
	my %response = triggerAction(%task);

	
	while (my ($name, $value) = each %response){
		if($name eq "dynamic_runtime" or $name eq "local_runtime" or $name eq "installable_runtime"or $name eq "jobs"){
			print $name."\n";
			foreach my $package(@{$value}) {
				my %hash = %{$package};
				print "  ". $hash{name}."\n";
			}
		}elsif($name eq "result"){
			print $name."\n";
			foreach my $subvalue(@{$value}) {
				print "  ". $subvalue."\n";
			}
		}else{
		   print "$name, $value\n";
		}

	}
	print "\n\n";


#
#      $jobs[0] = (  jobid           => "1234",     # identitfier
#                    created         => 1234567890, # unix time
#                    age             => 2,          # in seconds
#                    state           => "INITIALZED",
#                    dynamic_runtime => dynamic_runtime,
#                    runtimeenvironmentkey => $runtimeenvironmentkey,
#                    uses            => @uses
#                 );
#
#      dynamic_runtime[0] = (  name         => "APPS/BIO/WEKA-3.4.10",           # identitfier
#                              description  => "Machine learning tar package",   # possible fields
#                              lastupdated  =>  1234567890                       # possible fields
#                              state        =>  "INITIALZED"                     # possible fields
#                           );
#
# 	my %response = (
# 		action              => request{action},
# 		jobid               => request{jobid},
#		result		    => (code => 0, message => "Everything went fine"),
#               jobs                => @jobs,
# 		dynamic_runtime     => @dynamic_runtime,
# 		local_runtime       => @local_runtime,
# 		installable_runtime => @installable_runtime
# 	);



	my $root = XML::LibXML::Element->new( "A" );
	$root->setNamespace( "http://www.www.com/foo", "foo" );
	
	my $b = XML::LibXML::Element->new( "B" );
	$b->setNamespace( "http://www.www.com/foo", "foo" );
	$b->setAttribute("name");
	$root->appendChild($b);
	
	my $c = XML::LibXML::Element->new( "C" );
	$c->setNamespace( "http://www.www.com/foo", "foo" );
	$c->setAttribute("name");
	$root->appendChild($c);
	
	print $root->toString(2);

	return $root->toString(2);

}



#
# Input parameters:
#  $action - String containing the action to be performed. Maybe either: REGISTER, DEPLOY, REMOVE, LIST, CHECK, or SEARCH.
#  $jobid  - String containing the job identity. If an empty string is passed, a new fresh job ID will be generated
#  $retes  - String of RTE on which the desired action shall be performed. They are seperated by an \r\n token.
#
# Return value is always a field. 
# The first value contains the return code:
#   0 - alright; 
#   1 - some packages not yet installed; 
#   2 - error
# The second value contains the message:
#   First line: Generic result message
#   Additional lines are optional and may contain extra information
sub triggerAction {

	my (%request) = @_;
	my %response = (
		action              => $request{action},
		jobid               => $request{jobid}
        );

	assertRefreshCatalog();

my $jobid = "";
my @RTE = ();
$request{action} = "CHECK";
	
	switch ($request{action}) {
		case "LIST"            {
			print "LIST\n";
			%response = &list_info(%response, %response);
		}
		case "CHECK"            {
			print "CHECK\n";
			%response = &list_job_info(%response, %response);
		}
		case "SEARCH"           {
			print "SEARCH\n";
			%response = &search_catalog(%response, %response);
		}
		case "REGISTER"         { 
			print "REGISTER\n"; 
			my @ret = register_job($jobid,@RTE);
		}
		case "DEPLOY"           {
			print "DEPLOY\n";
			my @ret = deploy_for_job($jobid);
		}
		case "REMOVE"           {
			print "REMOVE\n";
			my @ret = remove_job($jobid,0);
		}
		case "SWEEP"           {
			print "SWEEP\n";
			# TODO: CONSIDER FORCE  sweep (1)
			my @ret = sweep();
		}
	}
	return  %response;
}



sub assertRefreshCatalog{  # INTERAL CHECK
	if ($next_refresh - 1 <= time){

		my $c = $GLOBAL_CATALOG;
		$GLOBAL_CATALOG = undef;
		eval { $GLOBAL_CATALOG = &get_catalog; };
		if ($@ or ! defined $GLOBAL_CATALOG) {
###l4p			$logger->error("error reading new catalog, continuing with old catalog");
			$GLOBAL_CATALOG = $c;
		}
		$next_refresh = &getNextCatalogRefresh(time);
	}
}


sub getNextCatalogRefresh {
	my ($o) = @_;

	my $refresh = $config->{'janitor'}{'catalogrefresh'};
	$refresh = 60 if !defined $refresh or $refresh < 60;
	my $var = 20 * (rand() - 0.5);	# +- 10s
	my $w =  int ($refresh + $var + 0.5);
	$w = 60 if $w < 60;

	return $o += $w;
}



######################################################################
# Generate a list of all registerd jobs
######################################################################
# XXX It's a bad idea to do a lot of printf while something locked. 
# XXX The printf might be blocked. Especially if somebody uses tools
# XXX like "watch"!!! This is the explanation for all this "useless"
# XXX variables.
######################################################################
sub list_info {

	my(%request, %response) = @_;

	my $now = time;
	my $output = "";

	
	my @jobs = ();
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


		push(@jobs,{jobid   => $id,
                            state   => $state,
                            created => $created,  # scalar localtime($created)
                            age     => timestamp_diff(time, $created),  # &timestamp_diff(time, $created)
                            rte     => $rte
                            });	
	}
	@{$response{jobs}} = @jobs;
	

	my @local_runtime = ();
	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my @runscripts =  &manual_installed_rte($manualRuntimeDir);
		if (scalar @runscripts != 0 ) {
			foreach my $packagename(@runscripts){
				push(@jobs,{name => $packagename});
			}
		}
	}
	@{$response{local_runtime}} = @local_runtime;


	my @dynamic_runtime = ();
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
			@used =  $rte->used;
		};

		$rte->disconnect;

		push(@dynamic_runtime, {name     => $rte_list,
                                        state    => $state,
                                        lastused => $last_used,  #scalar localtime($last_used)
                                        used     => @used
                                       });

	}
	@{$response{dynamic_runtime}} = @dynamic_runtime;

	
	my @installable_runtime = ();
	my @list = $GLOBAL_CATALOG->MetaPackages;
	foreach ( @list ) {
		my $package = $_;

		my $description = $package->description;
		$description =~ s/[\r\n]/ /g;

		push(@installable_runtime, {name        => $package->name,
                                            description => $description,
                                       });
	}
	@{$response{installable_runtime}} = @installable_runtime;

	@{$response{result}} = (0,"Successfully retrieved information.");

 	return %response;
}



######################################################################
# Lists information about job $jobid.
######################################################################
sub list_job_info {
	
	my(%request, %response) = @_;

	my $job = new Janitor::Job::($request{jobid}, $regDirJob);

	if ($job->open != 0) {
		@{$response{result}} = (2, "Unable to retrieve job information.");
		return %response;
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
		@{$response{result}} = (2, "Error while retrieving job information.");
		return %response;
	}

	$job->disconnect;

	my %job = {jobid   => $request{jobid},
                   state   => $state,
                   created => $created,  # scalar localtime($created)
                   age     => timestamp_diff(time, $created),  # &timestamp_diff(time, $created)
                   rte     => $rte
               };		

	
	if ($state == Janitor::Job::PREPARED) {
		$job{state}   = "PREPARED";
	} elsif ($state != Janitor::Job::INITIALIZED) {
		$job{state}   = "unknown state (bug?)";
	} else {
		$job{state}   = "INITIALIZED";
		$job{rte_key} = $rte_key if defined $rte_key;
		my @uses = push(@manual, @runtime);
		@{$job{rte_key}} = @uses;
		
# 		$output .= sprintf $format, "uses:", $_, foreach ( @manual, @runtime );
	}

	my @jobs = (%job);
	@{$response{jobs}} = @jobs;
	@{$response{result}} = (0,"Successfully retrieved job information.");

	return %response;
}



# like APPS/BIO/WEKA-3.4.10
# register_job      $job_id, @RTE
#     returns 0 - installed; 1 - some RTE not installed; 2 - error
# deploy_for_job    $job_id                                           deploy runtime environements
#     returns 0 - installed; 1 - some RTE not installed; 2 - error
# remove_job        $job_id, [optional force]                         removes job $job_id from janitors db.
#
# list_job_info     $job_id, 0                                        Lists information about job $jobid.
#
# check job_id
# 
# NEEDED: check_rte

sub search_catalog(){

	my(%request, %response) = @_;

	

	# extract local       runtime envionment names as an array
	my @local_runtime = ();
	foreach my $package(@{$request{local_runtime}}) {
		my %hash = %{$package};
		push(@local_runtime,$hash{name});
	}

	# extract dynamic     runtime envionment names as an array
	# extract installable runtime envionment names as an array

	my $hitcount = 0;
	my $output = "";

# 	$output .= "Local runtime environments:\r\n";
# 	my @manual_runscripts;
# 	# At first we check what is manually installed.
# 	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
# 		my @manual = &manual_installed_rte($manualRuntimeDir);
# 		my $val;
# 		my $valRTE;
# 		foreach my $val(@manual){
# 			foreach $valRTE(@RTE){
# 				if ($val =~ m/$valRTE/i ) {
#  					$output .= sprintf("%s\r\n",$val);
# 					$hitcount++;
# 					last;
# 				}
# 			}	
# 		}
# 	}
# 
# 	$output .= "\r\n";
# 	$output .= "Dynamical runtime environments:\r\n";
# 	my @list = $GLOBAL_CATALOG->MetaPackages;
# 	foreach ( @list ) {
# 		my $package = $_;
# 		my $description = $package->description;
# 		$description =~ s/[\n]*//;
# 
# 		my $valRTE;
# 		foreach $valRTE(@RTE){
# 			if($package->name =~ m/$valRTE/i){
# 				$output .= sprintf("%s\r\n",$package->name);
# 				$output .= sprintf($format, "description:", $description);
# 				$output .= sprintf($format, "lastupdate:", $package->lastupdate) if defined $package->lastupdate;
# 				$hitcount++;
# 				last;
# 			}
# 		}
# 	}
# 	# And in addition the catalog

	return (0,"Search has resulted ".$hitcount." hit(s).\r\n".$output);
}







######################################################################
# This function registers a job. If necessary it does some preparing
# for deploying the needed RTEs.
#
# 1. parameter: $job_id, Job_ID arbitary unique number
# 2. parameter: @RTE, Runtime Enviroment to install
#
# Return values:
#   0 - everything is fine. Needed RTEs are all installed
#   1 - it looks good, but some RTEs are not yet installed
#   2 - an error occured
######################################################################
sub register_job {

	my ($job_id, @RTE) = @_;



	if (sane_job_id($job_id) == 0 or sane_rte_name(@RTE) == 0) {
###l4p		$logger->error("bad request: no valid RE name or jobid");
		print "400 bad request\r\n";
		return (2,"Bad request: no valid RE name or jobid.");
	}



###l4p	$logger->info("$job_id: registering new job (@RTE)");

	my $job = new Janitor::Job($job_id, $regDirJob);

	my $ret = $job->create(@RTE);	

	unless (defined $ret) {
		my $msg = "Could not register job $job_id. Maybe there is already such a job registered.\n";
###l4p		$logger->fatal($msg);
		print STDERR $msg;
		return (2,"Could not register job. Maybe there is already such a job registered.");
	}

	# maybe this job needs no RTE at all. Check this.
	if (scalar @RTE == 0) {
###l4p		$logger->info("$job_id: sucessfully registered job");
		$job->initialize();
		$job->disconnect;
###l4p		$logger->info("$job_id: sucessfully initialized job");
		return (0,"Sucessfully initialized job.");
	}

	my @manual_runscripts;

	# At first we check what is manually installed.
	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my %manual = map { $_ => 1 } &manual_installed_rte($manualRuntimeDir);
		my @temp;
		foreach ( @RTE ) {
			if (defined $manual{$_}) {
				push @manual_runscripts, $manualRuntimeDir . "/" . $_;
			} else {
				push @temp, $_;
			}
		}
		@RTE = @temp;
	}
	if (scalar @RTE == 0) {
###l4p		$logger->info("$job_id: sucessfully registered job");
		$job->initialize(undef, @manual_runscripts);
		$job->disconnect;
###l4p		$logger->info("$job_id: sucessfully initialized job");
		return (0,"Sucessfully initialized job.");
	}

	# now we have in @RTE a list of rtes which must be installed
	# automatically

	my $rte = new Janitor::RTE($regDirRTE);
	$rte->connect(@RTE);

	my $state = $rte->state;

	if ($state == Janitor::RTE::FAILED) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: installation FAILED previously");
		$rte->disconnect;
		$job->remove;
		return (2,"Can't provide requested RTEs: installation FAILED previously.");

	} elsif ($state == Janitor::RTE::REMOVAL_PENDING) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: its waiting for removal");
		$rte->disconnect;
		$job->remove;
		return (2,"Can't provide requested RTEs: its waiting for removal.");

	} elsif ($state == Janitor::RTE::INSTALLED_A or $state == Janitor::RTE::INSTALLED_M
			or $state == Janitor::RTE::VALIDATED or $state == Janitor::RTE::BROKEN) {
		# its already there, maybe broken but still: It is there. So just use it.
		$rte->used($job_id);
###l4p		$logger->info("$job_id: sucessfully registered job");
		$job->initialize($rte->id, @manual_runscripts);
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$rte->disconnect;
		$job->disconnect;
		return (0,"Sucessfully initialized job.");

	} elsif ($state == Janitor::RTE::INSTALLABLE or $state == Janitor::RTE::UNKNOWN) {
		# check if it is (really) installable. Maybe things changed...
		my $catalog = &get_catalog;
		my @bslist = $catalog->basesystems_supporting_metapackages(@RTE);
		if ( @bslist ) {
			# ok, there is a way to deploy
###l4p			$logger->info("$job_id: sucessfully registered job");
			$rte->state(Janitor::RTE::INSTALLABLE);
			$rte->disconnect;
			$job->disconnect;
			return (1,"Sucessfully registered job.");
		} else {
###l4p 			$logger->info("$job_id: Can't provide requested RTE: not supported");
			$rte->state(Janitor::RTE::UNKNOWN);
			$rte->disconnect;
			$job->remove;
			return (2,"Can't provide requested RTE: not supported.");
		}
	}

	# XXX this should never be executed
	
	$rte->disconnect;
	$job->disconnect;
	
	return (1,"Job is in an unknown state.");
}

######################################################################
######################################################################
sub deploy_for_job {
	my ($job_id) = @_;
	my $ret;

	my $job = new Janitor::Job($job_id, $regDirJob);
	$ret = $job->open;
	unless (defined $ret and $ret == 0) {
###l4p		$logger->error("$job_id: Can't deploy: no such job.");
		return (2,"Can't deploy: no such job.");
	}

	my $state = $job->state;

	if ($state == Janitor::Job::INITIALIZED) {
		$job->disconnect;
###l4p		$logger->info("$job_id: already initialized");
		return (0,"Already initialized");
	} elsif ($state != Janitor::Job::PREPARED) {
		$job->disconnect;
###l4p		$logger->error("$job_id: unknown state, giving up.");
		return (2,"Unknown state, giving up.");
	}

	# The job is in the state prepared. So we can be sure, it needs
	# some RTEs which are currently not installed. But at least at
	# the time register was run, it was possible to deploy them.
	# This is not neccessarily true now. So we have to do all the
	# checks again.

	my @RTE = $job->rte;
		
	my @manual_runscripts;

	# At first we check what is manually installed.
	if (defined $manualRuntimeDir and -d $manualRuntimeDir) {
		my %manual = map { $_ => 1 } &manual_installed_rte($manualRuntimeDir);
		my @temp;
		foreach ( @RTE ) {
			if (defined $manual{$_}) {
				push @manual_runscripts, $_;
			} else {
				push @temp, $_;
			}
		}
		@RTE = @temp;
	}
	if (scalar @RTE == 0) {
		# whoooo, some friendly mind installed in the last
		# minutes everything manually
		$job->initialize(undef, @manual_runscripts);
		$job->disconnect;
###l4p		$logger->info("$job_id: sucessfully initialized job.");
		return (0,"Sucessfully initialized job."); }

	# now we have in @RTE a list of rtes which must be installed
	# automatically

	my $rte = new Janitor::RTE($regDirRTE);
	$rte->connect(@RTE);

	$state = $rte->state;

	if ($state == Janitor::RTE::FAILED) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: installation FAILED previously.");
		$rte->disconnect;
		$job->remove;
		return (2,"Can't provide requested RTEs: installation FAILED previously.");

	} elsif ($state == Janitor::RTE::REMOVAL_PENDING) {
###l4p		$logger->info("$job_id: Can't provide requested RTEs: its waiting for removal");
		$rte->disconnect;
		$job->remove;
		return (2,"Can't provide requested RTEs: its waiting for removal.");

	} elsif ($state == Janitor::RTE::INSTALLED_A or $state == Janitor::RTE::INSTALLED_M
			or $state == Janitor::RTE::VALIDATED or $state == Janitor::RTE::BROKEN) {
		# its already there, maybe broken but still: It is there. So just use it.
		$rte->used($job_id);
		$job->initialize($rte->id, @manual_runscripts);
###l4p		$logger->info("$job_id: sucessfully initialized job");
		$rte->disconnect;
		$job->disconnect;
		return (0,"Sucessfully initialized job");

	} elsif ($state == Janitor::RTE::INSTALLABLE or $state == Janitor::RTE::UNKNOWN) {
		# check if it is (really) installable. Maybe things changed...
		my $catalog = &get_catalog;
		my @bslist = $catalog->basesystems_supporting_metapackages(@RTE);
		unless ( @bslist ) {
###l4p 			$logger->info("$job_id: Can't provide requested RTE: not supported");
			$rte->state(Janitor::RTE::UNKNOWN);
			$rte->disconnect;
			$job->remove;
			return (2,"Can't provide requested RTE: not supported");
		}

		# ok, there is a way to deploy
		$rte->state(Janitor::RTE::INSTALLABLE);

		eval {
			$ret = deploy_rte_for_job($job_id, $rte, \@RTE, $catalog, \@bslist);
		};
		if ($@ or $ret != 0) {
###l4p			$logger->error("$job_id: Can't provide requested RTEs: installation FAILED");
			$rte->state(Janitor::RTE::FAILED);
			$rte->disconnect;
			$job->remove;
			return (2,"Can't provide requested RTEs: installation FAILED");
		}

		$rte->state(Janitor::RTE::INSTALLED_A);
		$rte->used($job_id);
		$job->initialize($rte->id, @manual_runscripts);
###l4p		$logger->info("$job_id: sucessfully initialized job");

		$rte->disconnect;
		$job->disconnect;
		return (0,"Sucessfully initialized job");
	}

	$rte->disconnect;
	$job->disconnect;
	
	printf "XXX This code should never have been executed!!!\n";

	return (2,"Can't provide requested RTEs: installation FAILED");
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

	# check wheter the packages are supported to be installed
	my $installable = 1;
	foreach my $p ( @packages ) {
		if($p->isa("Janitor::Catalog::TarPackage")){
			next;
		}
		$installable = 0;
	}

	if($installable){
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
	}else{
		return 2;
	}

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
		print "state nicht definiert!\n";
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
	my $installer = new Janitor::Installer($installationdir);
	my ($ret, $installed) = $installer->install($url);
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
# XXX Hack: if $sweep is true the function is not called directy
# XXX but by force sweeping. In this case $job_id is not a job_id
# XXX but a already opened Janitor::Job object.
######################################################################
sub remove_job {
	my ($job_id, $sweep) = @_;


	my $job;

	if (defined $sweep and $sweep) {

		# If this happens do something like this
# 		my $ID;
# 		foreach (@INPUT) {
# 			if (m/^(\w+)\s+(\w+)$/) {
# 				if ($1 eq "ID" and ! defined $ID) {
# 					$ID = $2;
# 					next;
# 				}	
# 			}
# ###l4p			$logger->error("bad request: $_");
# 			print $client "400 bad request\r\n";
# 			return;
# 		}
		$job = $job_id;
		$job_id = $job->id;

###l4p		$logger->info("force sweeping job $job_id");
	} else {
###l4p		$logger->info("removing job $job_id");
		$job = new Janitor::Job($job_id, $regDirJob);
		if ($job->open != 0) {
###l4p			$logger->error("$job_id: no such job");
			printf STDERR "$job_id: no such job\n";
			return (2, "No such job.");
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
	return (0, "Job successfully removed.");
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

###l4p 	$logger->info("Removing ". join " ", $rte->rte_list);

	foreach my $p ($rte->retrieve_data) {
		

		if(defined $p){ # may not be defined if register job fails
print "SWEEP RTE: retrieved data: ".$p."\n";
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
	my ($force) = @_;

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
			} elsif ($state == Janitor::RTE::UNKNOWN or $state == Janitor::RTE::FAILED) {
				if (defined $force and $force) { # used == 0; 
					sweep_rte($rte);
					$flag = 1;
				}else{
					print "RTE in an unknown or failed state found. Use force option to remove it.\n";
				}
			}else{

				print "RTE is not covered by conditions";
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

	return (0, "Successfully sweeped packages.");
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
# Sets the state of a RTE to a new value. But we can only do a
# few state changes.
######################################################################
sub setstate {
	my ($state, @rte) = @_;

	my $newstate = Janitor::RTE::string2state($state);
	unless (defined $newstate) {
		print STDERR $state . ": not a valid statename\n";
		return 1;
	}

	my $rte = new Janitor::RTE($regDirRTE, "SETSTATE");

	$rte->connect(@rte);
	my $oldstate = $rte->state;

	if ($oldstate == $newstate) {
		$rte->disconnect;
		return 0;
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
			return 1;
		}
		$rte->disconnect;
		return 0;
	}
	
	$rte->disconnect;
	return 0;
}