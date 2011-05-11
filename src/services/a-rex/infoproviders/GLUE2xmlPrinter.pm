package GLUE2xmlPrinter;

use base XmlPrinter;

sub new {
    my ($this, $handle, $splitjobs) =  @_;
    my $self = $this->SUPER::new($handle);
    $self->{splitjobs} = $splitjobs;
    return $self;
}

sub beginEntity {
    my ($self, $data, $name, $basetype) = @_;
    return undef unless $name;
    $data->{BaseType} = $basetype;
    $self->begin($name, $data, qw( BaseType CreationTime Validity ));
    $self->properties($data, qw( ID Name OtherInfo ));
}

sub Element {
    my ($self, $collector, $name, $basetype, $filler) = @_;
    return unless $collector and my $data = &$collector();
    $self->beginEntity($data, $name, $basetype);
    &$filler($self, $data) if $filler;
    $self->end($name);
}

sub Elements {
    my ($self, $collector, $name, $basetype, $filler) = @_;
    while ($collector and my $data = &$collector()) {
        $self->beginEntity($data, $name, $basetype);
        &$filler($self, $data) if $filler;
        $self->end($name);
    }
}


sub Location {
    Element(@_, 'Location', undef, sub {
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
    Elements(@_, 'Contact', undef, sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Detail Type ));
    });
}

sub AdminDomain {
    Element(@_, 'AdminDomain', 'Domain', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Description WWW Distributed Owner ));
        $self->begin('Services');
        $self->ComputingService($data->{ComputingService});
        $self->end('Services');
    });
}

sub AccessPolicies {
    Elements(@_, 'AccessPolicy', 'Policy', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Scheme Rule ));
        if ($data->{UserDomainID}) {
            $self->begin('Associations');
            $self->properties($data, 'UserDomainID');
            $self->end('Associations');
        }
    });
}

sub MappingPolicies {
    Elements(@_, 'MappingPolicy', 'Policy', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Scheme Rule ));
        if ($data->{UserDomainID}) {
            $self->begin('Associations');
            $self->properties($data, 'UserDomainID');
            $self->end('Associations');
        }
    });
}

sub ComputingService {
    Element(@_, 'ComputingService', 'Service', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Capability
                                     Type
                                     QualityLevel
                                     StatusInfo
                                     Complexity ));
        $self->Location($data->{Location});
        $self->Contacts($data->{Contacts});
        $self->properties($data, qw( TotalJobs
                                     RunningJobs
                                     WaitingJobs
                                     StagingJobs
                                     SuspendedJobs
                                     PreLRMSWaitingJobs ));
        $self->ComputingEndpoint($data->{ComputingEndpoint});
        $self->ComputingShares($data->{ComputingShares});
        $self->ComputingManager($data->{ComputingManager});
        $self->ToStorageServices($data->{ToStorageServices});
        if ($data->{ServiceID}) {
            $self->begin('Associations');
            $self->properties($data, 'ServiceID');
            $self->end('Associations');
        }
    });
}

sub ComputingEndpoint {
    Element(@_, 'ComputingEndpoint', 'Endpoint', sub {
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
        $self->AccessPolicies($data->{AccessPolicies});
        $self->properties($data, qw( Staging
                                     JobDescription
                                     TotalJobs
                                     RunningJobs
                                     WaitingJobs
                                     StagingJobs
                                     SuspendedJobs
                                     PreLRMSWaitingJobs ));
        if ($data->{ComputingShareID}) {
            $self->begin('Associations');
            $self->properties($data, 'ComputingShareID');
            $self->end('Associations');
        }
        if ($data->{ComputingActivities}) {
            $self->begin('ComputingActivities');
            $self->ComputingActivities($data->{ComputingActivities});
            $self->end('ComputingActivities');
        }
    });
}

sub ComputingShares {
    Elements(@_, 'ComputingShare', 'Share', sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Description ));
        $self->MappingPolicies($data->{MappingPolicies});
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
        $self->begin('Associations');
        $self->properties($data, 'ComputingEndpointID');
        $self->properties($data, 'ExecutionEnvironmentID');
        $self->end('Associations');
    });
}

sub ComputingManager {
    Element(@_, 'ComputingManager', 'Manager', sub {
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
        $self->Benchmarks($data->{Benchmarks});
        $self->begin('ExecutionEnvironments');
        $self->ExecutionEnvironments($data->{ExecutionEnvironments});
        $self->end('ExecutionEnvironments');
        $self->begin('ApplicationEnvironments');
        $self->ApplicationEnvironments($data->{ApplicationEnvironments});
        $self->end('ApplicationEnvironments');
    });
}

sub Benchmarks {
    Elements(@_, 'Benchmark', undef, sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Type Value ));
    });
}

sub ExecutionEnvironments {
    Elements(@_, 'ExecutionEnvironment', 'Resource', sub {
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
        $self->Benchmarks($data->{Benchmarks});
        if ($data->{ComputingShareID}
         or $data->{ComputingActivityID}
         or $data->{ApplicationEnvironmentID})
        {
            $self->begin('Associations');
            $self->properties($data, 'ComputingShareID');
            $self->properties($data, 'ComputingActivityID');
            $self->properties($data, 'ApplicationEnvironmentID');
            $self->end('Associations');
        }
    });
}

sub ApplicationEnvironments {
    Elements(@_, 'ApplicationEnvironment', undef, sub {
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
        $self->ApplicationHandles($data->{ApplicationHandles});
        if ($data->{ExecutionEnvironmentID}) {
            $self->begin('Associations');
            $self->properties($data, 'ExecutionEnvironmentID');
            $self->end('Associations');
        }
    });
}

sub ApplicationHandles {
    Elements(@_, 'ApplicationHandle', undef, sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( Type Value ));
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
        $self->begin('Associations');
        $self->properties($data, 'UserDomainID');
        $self->properties($data, 'ComputingEndpointID');
        $self->properties($data, 'ComputingShareID');
        $self->properties($data, 'ExecutionEnvironmentID');
        $self->properties($data, 'ActivityID');
        $self->end('Associations');
    };

    my ($self, $collector) = @_;

    if (not $self->{splitjobs}) {
        Elements(@_, 'ComputingActivity', 'Activity', $filler);
    } else {
        while (my $data = &$collector()) {

            # Function that returns a string containing the ComputingActivity's XML
            my $xmlGenerator = sub {
                my ($memhandle, $xmlstring);
                open $memhandle, '>', \$xmlstring;
                return undef unless defined $memhandle;
                my $memprinter = XmlPrinter->new($memhandle);
                $data->{xmlns} = "http://schemas.ogf.org/glue/2009/03/spec/2/0";
                # Todo: fix a-rex, client to handle correct namespace
                $data->{xmlns} = "http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01";
                $data->{BaseType} = "Activity";
                $memprinter->begin('ComputingActivity', $data, qw(xmlns BaseType CreationTime Validity ));
                $memprinter->properties($data, qw(ID Name OtherInfo));
                &$filler($memprinter, $data);
                $memprinter->end('ComputingActivity');
                close($memhandle);
                return $xmlstring;
            };
            my $filewriter = $data->{jobXmlFileWriter};
            &$filewriter($xmlGenerator);
        }
    }
}

sub ToStorageServices {
    Elements(@_, 'ToStorageService', undef, sub {
        my ($self, $data) = @_;
        $self->properties($data, qw( LocalPath RemotePath ));
        $self->begin('Associations');
        $self->properties($data, 'StorageServiceID');
        $self->end('Associations');
    });
}


sub Domains {
    my ($self, $data) = @_;
    my $attrs = { 'xmlns' => "http://schemas.ogf.org/glue/2009/03/spec/2/0",
                  'xmlns:xsi' => "http://www.w3.org/2001/XMLSchema-instance",
                  'xsi:schemaLocation' => "http://schemas.ogf.org/glue/2009/03/spec/2/0 pathto/GLUE2.xsd" };
    # Todo: fix a-rex, client to handle correct namespace
    $attrs->{'xmlns'} = "http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01";
    $self->begin('Domains', $attrs, qw( xmlns xmlns:xsi xsi:schemaLocation ));
    $self->AdminDomain($data);
    $self->end('Domains');
}

1;
