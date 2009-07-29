=head1 NAME

Request.pm - This class bundles the information to be passed to the method process of Janitor::Main.pm

Following methods and constructors are provides by this class:

=over 4

=item C<new> 

Parameters: $action, $jobid, $rRteList, $force, $state

Sets given the parameters. In case certain information isn't given they can be set
to C<undef>. A reference to the runtime environment list has to be passed.

=item C<action> 

Optional Parameter: $action

Set the action variable if defined. Always returns the currently stored action.
This variable is needed to assign the request to a certain action.

=item C<jobid> 

Optional Parameter: $jobid

Set the jobid variable if defined. Always returns the currently stored jobid.
This variable is currently used within the register, deploy, remove and info method.

=item C<state> 

Optional Parameter: $state

Set the state variable if defined. Always returns the currently stored state.
This variable is currently only used within the setstate method.

=item C<rteList> 

Optional Parameter: $rRteList

Stores the given reference of a runtime environment list. Always returns the 
current list of runtime environments (dereferenced).
This variable is currently used within the register and search method.

=item C<force> 

Optional Parameter: $force

Sets the force variable if defined. Always returns the currently stored force value.
This variable is currently only used within the sweep method.


=back



=cut




package Janitor::Request;


use warnings;
use strict;

use constant {
	REGISTER	=> 1,
	DEPLOY		=> 2,
	REMOVE		=> 3,
	SWEEP		=> 4,
	LIST		=> 5,
	INFO		=> 6,
	SEARCH		=> 7,
	SETSTATE	=> 8
};

sub new{
	my ($class, $action, $jobid, $rRteList, $force, $state) = @_;

	my $self = {
		action   => $action,
		jobid    => $jobid,
		force    => $force,
		state    => $state
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
		$self->{jobid} = $jobid;
	}
	return $self->{jobid};
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

sub force{
	my ($self, $force) = @_;
	if(defined $force){
		$self->{force} = $force;
	}
	return $self->{force};
}

sub state{
	my ($self, $state) = @_;
	if(defined $state){
		$self->{state} = $state;
	}
	return $self->{state};
}

1;