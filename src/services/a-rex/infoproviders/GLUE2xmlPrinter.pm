package GLUE2xmlPrinter;

use base XmlPrinter;

sub new {
    my ($this, $handle, $splitjobs) =  @_;
    my $self = $this->SUPER::new($handle);
    $self->{splitjobs} = $splitjobs;
    return $self;
}

sub startEntity {
    my ($self, $data, $name) = @_;
    return undef unless $name;
    $self->start($name, $data, qw( BaseType CreationTime Validity ));
    $self->properties($data, qw( ID Name OtherInfo ));
}

sub Entity {
    my ($self, $collector, $name, $filler) = @_;
    return unless $collector and my $data = &$collector();
    $self->startEntity($data, $name);
    &$filler($self, $data) if $filler;
    $self->end($name);
}

sub Entities {
    my ($self, $collector, $name, $filler) = @_;
    while ($collector and my $data = &$collector()) {
        $self->startEntity($data, $name);
        &$filler($self, $data) if $filler;
        $self->end($name);
    }
}


sub Location {
    Entity(@_, 'Location', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Address
                                     Place
                                     Country
                                     PostCode
                                     Latitude
                                     Longitude ));
    });
}

sub Contacts {
    Entities(@_, 'Contact', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Detail Type ));
    });
}

sub Policies {
    Entities(@_, sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Scheme Rule ));
    });
}

sub Benchmarks {
    Entities(@_, 'Benchmark', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Type Value ));
    });
}

sub ApplicationEnvironments {
    Entities(@_, 'ApplicationEnvironment', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( AppName
                                     AppVersion
                                     State
                                     RemovalDate
                                     License
                                     Description
                                     BestBenchmark
                                     ParallelSupport
                                     MaxSlots
                                     MaxJobs
                                     MaxUserSeats
                                     FreeSlots
                                     FreeJobs
                                     FreeUserSeats ));
        #$self->ApplicationHandle($data->{ApplicationHandle});
        if ($data->{Associations}) {
            $self->start('Associations');
            $self->properties($data->{Associations}, 'ExecutionEnvironmentID');
            $self->end('Associations');
        }
    });
}

sub ExecutionEnvironments {
    Entities(@_, 'ExecutionEnvironment', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Platform
                                     VirtualMachine
                                     TotalInstances
                                     UsedInstances
                                     UnavailableInstances
                                     PhysicalCPUs
                                     LogicalCPUs
                                     CPUMultiplicity
                                     CPUVendor
                                     CPUModel
                                     CPUVersion
                                     CPUClockSpeed
                                     CPUTimeScalingFactor
                                     WallTimeScalingFactor
                                     MainMemorySize
                                     VirtualMemorySize
                                     OSFamily
                                     OSName
                                     OSVersion
                                     ConnectivityIn
                                     ConnectivityOut
                                     NetworkInfo ));
        if ($data->{Associations}) {
            $self->start('Associations');
            $self->properties($data->{Associations}, 'ComputingShareID');
            $self->properties($data->{Associations}, 'ComputingActivityID');
            $self->properties($data->{Associations}, 'ApplicationEnvironmentID');
            $self->end('Associations');
        }
    });
}


sub ComputingActivities {
    my $filler = sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Type
                                     IDFromEndpoint
                                     LocalIDFromManager
                                     JobDescription
                                     State
                                     RestartState
                                     ExitCode
                                     ComputingManagerExitCode
                                     Error
                                     WaitingPosition
                                     UserDomain
                                     Owner
                                     LocalOwner
                                     RequestedTotalWallTime
                                     RequestedTotalCPUTime
                                     RequestedSlots
                                     RequestedApplicationEnvironment
                                     StdIn
                                     StdOut
                                     StdErr
                                     LogDir
                                     ExecutionNode
                                     Queue
                                     UsedTotalWallTime
                                     UsedTotalCPUTime
                                     UsedMainMemory
                                     SubmissionTime
                                     ComputingManagerSubmissionTime
                                     StartTime
                                     ComputingManagerEndTime
                                     EndTime
                                     WorkingAreaEraseTime
                                     ProxyExpirationTime
                                     SubmissionHost
                                     SubmissionClientName
                                     OtherMessages ));
        $self->start('Associations');
        $self->properties($data->{Associations}, 'UserDomainID');
        $self->properties($data->{Associations}, 'ComputingEndpointID');
        $self->properties($data->{Associations}, 'ComputingShareID');
        $self->properties($data->{Associations}, 'ExecutionEnvironmentID');
        $self->properties($data->{Associations}, 'ActivityID');
        $self->end('Associations');
    };

    my ($self, $collector) = @_;

    if (not $self->{splitjobs}) {
        Entities(@_, 'ComputingActivity', $filler);
    } else {
        while (my $data = &$collector()) {
            my $fhmaker = $data->{xmlFileHandle};
            my $fh = $fhmaker ? &$fhmaker() : undef;
            next unless defined $fh;
            my $printer = XmlPrinter->new($fh);
            #$printer->header();
            $data->{xmlns} = "http://schemas.ogf.org/glue/2009/03/spec/2/0";
            $printer->start('ComputingActivity', $data, qw(xmlns CreationTime Validity BaseType));
            $printer->properties($data, qw(ID Name OtherInfo));
            &$filler($printer, $data);
            $printer->end('ComputingActivity');
            close($fh);
        }
    }
}

sub ComputingShares {
    Entities(@_, 'ComputingShare', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Description ));
        $self->Policies($data->{MappingPolicy}, 'MappingPolicy');
        $self->properties($data, qw( MappingQueue
                                     MaxWallTime
                                     MaxMultiSlotWallTime
                                     MinWallTime
                                     DefaultWallTime
                                     MaxCPUTime
                                     MaxTotalCPUTime
                                     MinCPUTime
                                     DefaultCPUTime
                                     MaxTotalJobs
                                     MaxRunningJobs
                                     MaxWaitingJobs
                                     MaxPreLRMSWaitingJobs
                                     MaxUserRunningJobs
                                     MaxSlotsPerJob
                                     MaxStateInStreams
                                     MaxStageOutStreams
                                     SchedulingPolicy
                                     MaxMainMemory
                                     GuaranteedMainMemory
                                     MaxVirtualMemory
                                     GuaranteedVirtualMemory
                                     MaxDiskSpace
                                     DefaultStorageService
                                     Preemption
                                     ServingState
                                     TotalJobs
                                     RunningJobs
                                     LocalRunningJobs
                                     WaitingJobs
                                     LocalWaitingJobs
                                     SuspendedJobs
                                     LocalSuspendedJobs
                                     StagingJobs
                                     PreLRMSWaitingJobs
                                     EstimatedAverageWaitingTime
                                     EstimatedWorstWaitingTime
                                     FreeSlots
                                     FreeSlotsWithDuration
                                     UsedSlots
                                     RequestedSlots
                                     ReservationPolicy
                                     Tag ));
        $self->start('Associations');
        $self->properties($data->{Associations}, 'ComputingEndpointID');
        $self->properties($data->{Associations}, 'ExecutionEnvironmentID');
        $self->properties($data->{Associations}, 'ComputingActivityID');
        $self->end('Associations');
    });
}

sub ComputingManager {
    Entity(@_, 'ComputingManager', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( ProductName
                                     ProductVersion
                                     Reservation
                                     BulkSubmission
                                     TotalPhysicalCPUs
                                     TotalLogicalCPUs
                                     TotalSlots
                                     SlotsUsedByLocalJobs
                                     SlotsUsedByGridJobs
                                     Homogeneous
                                     NetworkInfo
                                     LogicalCPUDistribution
                                     WorkingAreaShared
                                     WorkingAreaGuaranteed
                                     WorkingAreaTotal
                                     WorkingAreaFree
                                     WorkingAreaLifeTime
                                     WorkingAreaMultiSlotTotal
                                     WorkingAreaMultiSlotFree
                                     WorkingAreaMultiSlotLifeTime
                                     CacheTotal
                                     CacheFree
                                     TmpDir
                                     ScratchDir
                                     ApplicationDir ));
        $self->Benchmarks($data->{Benchmark});
        $self->start('ExecutionEnvironments');
        $self->ExecutionEnvironments($data->{ExecutionEnvironment});
        $self->end('ExecutionEnvironments');
        $self->start('ApplicationEnvironments');
        $self->ApplicationEnvironments($data->{ApplicationEnvironment});
        $self->end('ApplicationEnvironments');
    });
}

sub ComputingEndpoint {
    Entity(@_, 'ComputingEndpoint', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( URL
                                     Capability
                                     Technology
                                     InterfaceName
                                     InterfaceVersion
                                     InterfaceExtension
                                     WSDL
                                     SupportedProfile
                                     Semantics
                                     Implementor
                                     ImplementationName
                                     ImplementationVersion
                                     QualityLevel
                                     HealthState
                                     HealthStateInfo
                                     ServingState
                                     StartTime
                                     IssuerCA
                                     TrustedCA
                                     DowntimeAnnounce
                                     DowntimeStart
                                     DowntimeEnd
                                     DowntimeInfo ));
        $self->Policies($data->{AccessPolicy}, 'AccessPolicy');
        $self->properties($data, qw( Staging
                                     JobDescription
                                     TotalJobs
                                     RunningJobs
                                     WaitingJobs
                                     StagingJobs
                                     SuspendedJobs
                                     PreLRMSWaitingJobs ));
        $self->start('Associations');
        $self->properties($data->{Associations}, 'ComputingShareID');
        $self->end('Associations');
        if ($data->{ComputingActivity}) {
            $self->start('ComputingActivities');
            $self->ComputingActivities($data->{ComputingActivity});
            $self->end('ComputingActivities');
        }
    });
}

sub ComputingService {
    Entity(@_, 'ComputingService', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Capability
                                     Type
                                     QualityLevel
                                     StatusInfo
                                     Complexity ));
        $self->Location($data->{Location});
        $self->Contacts($data->{Contact});
        $self->properties($data, qw( TotalJobs
                                     RunningJobs
                                     WaitingJobs
                                     StagingJobs
                                     SuspendedJobs
                                     PreLRMSWaitingJobs ));
        $self->ComputingEndpoint($data->{ComputingEndpoint});
        $self->ComputingShares($data->{ComputingShare});
        $self->ComputingManager($data->{ComputingManager});
        #$self->ToStorageService($data->{ToStorageService});
        $self->start('Associations');
        $self->properties($data->{Associations}, 'ServiceID');
        $self->end('Associations');
    });
}

sub Document {
    my ($self, $data) = @_;
    $collector = $data->{AdminDomain}{Services}{ComputingService};
    my $attrs = { 'xmlns:glue' => "http://schemas.ogf.org/glue/2009/03/spec/2/0",
                  'xmlns:xsi' => "http://www.w3.org/2001/XMLSchema-instance",
                  'xsi:schemaLocation' => "http://schemas.ogf.org/glue/2009/03/spec/2/0 pathto/GLUE2.xsd" };
    $attrs->{'xmlns:glue'} = "http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01";
    #$self->header();
    $self->start('glue:Domains', $attrs, qw( xmlns xmlns:glue xmlns:xsi xsi:schemaLocation ));
    $self->start('AdminDomain', $data->{AdminDomain}, qw( BaseType ));
    $self->properties($data->{AdminDomain}, qw( ID Name ));
    $self->start('Services');
    $self->ComputingService($collector);
    $self->end('Services');
    $self->end('AdminDomain');
    $self->end('glue:Domains');
}

1;
