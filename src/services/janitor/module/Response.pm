=head1 NAME

Response.pm - This class bundles the returned information of Janitor::Main.pm

A response class should at least represent a certain C<action> and have a C<result>.
In addition C<jobid>, C<rteList>, C<state>, C<force> flag, C<runningJobs> list, 
C<runtimeDynamic> list, C<runtimeLocal> list or C<runtimeInstallable> may also be set.

Following methods and constructors are provides by this class:

=over 4

=item C<new> 

Parameters: $action, $jobid, $rRteList, $force, $state

Sets given the parameters. In case certain information isn't given they can be set
to C<undef>. A reference to the runtime environment list has to be passed.

=item C<action> 

Optional Parameter: $action

Set the action variable if defined. Always returns the currently stored action.

=item C<jobid> 

Optional Parameter: $jobid

Set the jobid variable if defined. Always returns the currently stored jobid.

=item C<state> 

Optional Parameter: $state

Set the state variable if defined. Always returns the currently stored state.

=item C<rteList> 

Optional Parameter: $rRteList

Stores the given reference of a runtime environment list. Always returns the 
current list of runtime environments (dereferenced).

=item C<result> 

Parameters: $errorcode, $message

Stores the given parameter in an hash. Always returns a hash containin the
keys C<errorcode> and C<message>.

=item C<runtimeLocal> 

Parameters: $rte_name

Adds a local runtime environment to the inner list of local runtime environments.
Always returns the inner list of local runtime environments.

=item C<runtimeInstallable> 

Parameters: $rte_name, $description, $supported

Adds an installable runtime environment to the inner list of installable runtime environments.
Always returns the inner list of installable runtime environments.

=item C<runtimeDynamic> 

Parameters: $rte_name, $state, $lastused, $used

Adds a dynamic runtime environment to the inner list of dynamic runtime environments.
Always returns the inner list of dynamic runtime environments.

=item C<runningJob> 

Parameters:  $jobid, $created, $age, $state, $rteList, $runtimeenvironmentkey, $rUses

Adds information about a running job to the inner list of running jobs.
The variable C<rteList> is a simple string and the list of used ressources must be a reference to a list.
Always returns the inner list of running jobs environments.

=back



=cut


package Janitor::Response;


use warnings;
use strict;

use constant {
	REGISTER	=> Janitor::Request::REGISTER,
	DEPLOY		=> Janitor::Request::DEPLOY,
	REMOVE		=> Janitor::Request::REMOVE,
	SWEEP		=> Janitor::Request::SWEEP,
	LIST		=> Janitor::Request::LIST,
	INFO		=> Janitor::Request::INFO,
	SEARCH		=> Janitor::Request::SEARCH,
	SETSTATE	=> Janitor::Request::SETSTATE
};

#      runtime[0]         = (  rte_list     => ("APPS/BIO/WEKA-3.4.10","fg"),	# identitfier
#                              description  => "Machine learning tar package",	# possible fields
#                              lastupdated  =>  1234567890,			# possible fields
#                              state        =>  "INITIALZED"			# possible fields
#                           );
#
#      job[0]             = ( jobid                 => "1234",     		# identitfier
#                             created               => 1234567890, 		# unix time
#                             age                   => 2,          		# in seconds
#                             state                 => "INITIALZED",
#                             rte_list              => ("APPS/BIO/WEKA-3.4.10","fg"),
#                             runtimeenvironmentkey => $runtimeenvironmentkey,
#                             uses                  => @uses,
#                           );

sub new{
	my ($class, $action, $jobid, $rRteList, $force, $state) = @_;

	my $self = {
		action   => $action,				# ALWAYS
		jobid    => $jobid,				# ALWAYS
# 		result   => %result,				# ALWAYS
# 		rte_list => $rte_list,				# REGISTER, DEPLOY, REMOVE
		force    => $force
# 		state    => $state,				# REGISTER, DEPLOY, REMOVE
#               jobs                => @jobs,			# LIST
# 		dynamic_runtime     => @dynamic_runtime,	# LIST, SEARCH
# 		local_runtime       => @local_runtime,		# LIST, SEARCH
# 		installable_runtime => @installable_runtime	# LIST, SEARCH
	};
	rteList($self, $rRteList);

	return bless $self, $class;
}

sub action{
	my ($self, $action) = @_;
	if(defined $action){
		$self->{action} = $action;
	}
	return $self->{action};
}

sub jobid{
	my ($self, $jobid) = @_;
	if(defined $jobid){
		$self->{action} = $jobid;
	}
	return $self->{action};
}

sub state{
	my ($self, $state) = @_;
	if(defined $state){
		$self->{state} = $state;
	}
	return $self->{state};
}

sub rteList{
	my ($self, $rRteList) = @_;

	if(defined $$rRteList[0]){
		$self->{rte_list} = $rRteList;
	}

	$rRteList = $self->{rte_list};
	if(defined $$rRteList[0]){
		return @$rRteList;
	}else{
		return ();
	}
}

sub result {
	my ($self, $errorcode, $message) = @_;
	if(defined $errorcode and defined $message){
		%{$self->{result}} = (errorcode => $errorcode, message => $message);
	}
	
	return %{$self->{result}};
}



#  If parameters are used, a new local runtime environment entry will be made.
#  All local runtime environment entries will be returned
# Parameter: (rte_list)
# Return:    All runtime evironments added
sub runtimeLocal {
	my ($self, $rte_name) = @_;
	if(defined $rte_name){
		push(@{$self->{local_runtime}}, {name => $rte_name});
	}

	if(defined $self->{local_runtime}){
		return @{$self->{local_runtime}};
	}else{
		return undef;
	}
}


# name
# state
# lastused
# used
sub runtimeDynamic{
	my ($self, $rte_name, $state, $lastused, $used) = @_;
	if(defined $rte_name){
		push(@{$self->{dynamic_runtime}}, {name => $rte_name, state => $state, lastused => $lastused, used => $used});
	}

	if(defined $self->{dynamic_runtime}){
		return @{$self->{dynamic_runtime}};
	}else{
		return undef;
	}
}

#  If parameters are used, a new running installable entry will be made.
#  In any case the latest set of running installable entries will be returned
# Parameter: (rte_list, description)
# Return:    All runtime evironments added
sub runtimeInstallable{
	my ($self, $rte_name, $description, $supported) = @_;
	if(defined $rte_name){
		push(@{$self->{installable_runtime}}, {name => $rte_name, description => $description, supported => $supported});
	}

	if(defined $self->{installable_runtime}){
		return @{$self->{installable_runtime}};
	}else{
		return undef;
	}
}

sub runningJob {
	my ($self, $jobid, $created, $age, $state, $rteList, $runtimeenvironmentkey, $rUses) = @_;

	if(defined $jobid){
		push(@{$self->{jobs}} ,{ 
			jobid                 => $jobid,     # identitfier
			created               => $created,   # unix time
			age                   => $age,       # in seconds
			state                 => $state,
			rte_list              => $rteList,
			runtimeenvironmentkey => $runtimeenvironmentkey,
			uses                  => $rUses
			});
	}

	if(defined $self->{jobs}){
		return @{$self->{jobs}};
	}else{
		return undef;
	}
}


1;
