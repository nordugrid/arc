package GLUE2ldifPrinter;

use base LdifPrinter;

sub beginGroup {
    my ($self, $name) = @_;
    $self->begin(GLUE2GroupID =>, $name);
    my $data = { objectClass => "GLUE2Group", GLUE2GroupID => $name};
    $self->attributes($data, "", qw(objectClass GLUE2GroupID));
}

sub endGroup {
    my ($self) = @_;
    $self->end();
}

sub Entry {
    my ($self, $collector, $dnkey, $attributes, $subtree) = @_;
    return unless $collector and my $data = &$collector();
    $self->begin($dnkey, $data->{ID});
    &$attributes($self,$data);
    &$subtree($self, $data) if $subtree;
    $self->end();
}

sub Entries {
    my ($self, $collector, $dnkey, $attributes, $subtree) = @_;
    while ($collector and my $data = &$collector()) {
        $self->begin($dnkey, $data->{ID});
        &$attributes($self,$data);
        &$subtree($self, $data) if $subtree;
        $self->end();
    }
}


sub EntityAttributes {
    my ($self, $data) = @_;
    $self->attributes($data, "GLUE2Entity", qw( CreationTime Validity Name OtherInfo ));
}

sub LocationAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Location");
    $self->attributes($data, "GLUE2Location", qw( ID Address Place Country PostCode Latitude Longitude ));
}

sub ContactAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Contact");
    $self->attributes($data, "GLUE2Contact", qw( ID Detail Type ));
}

sub DomainAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Domain");
    $self->attributes($data, "GLUE2Domain", qw( ID Description WWW ));
}

sub AdminDomainAttributes {
    my ($self, $data) = @_;
    $self->DomainAttributes($data);
    $self->attribute(objectClass => "GLUE2AdminDomain");
    $self->attributes($data, "GLUE2AdminDomain", qw( Distributed Owner ));
    $self->attribute(GLUE2AdminDomainAdminDomainForeignKey => $data->{AdminDomainID});
}

sub UserDomainAttributes {
    my ($self, $data) = @_;
    $self->DomainAttributes($data);
    $self->attribute(objectClass => "GLUE2UserDomain");
    $self->attributes($data, "GLUE2UserDomain", qw( Level UserManager Member ));
    $self->attribute(GLUE2UserDomainUserDomainForeignKey => $data->{UserDomainID});
}

sub ServiceAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Service");
    $self->attributes($data, "GLUE2Service", qw( ID
                                                 Capability
                                                 Type
                                                 QualityLevel
                                                 StatusInfo
                                                 Complexity ));
    $self->attribute(GLUE2ServiceAdminDomainForeignKey => $data->{AdminDomainID});
    $self->attribute(GLUE2ServiceServiceForeignKey => $data->{ServiceID});
}

sub EndpointAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Endpoint");
    $self->attributes($data, "GLUE2Endpoint", qw( ID
                                                  URL
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
                                                  DowntimfeStart
                                                  DowntimeEnd
                                                  DowntimeInfo ));
    $self->attribute(GLUE2EndpointServiceForeignKey => $data->{ServiceID});
}

sub ShareAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Share");
    $self->attributes($data, "GLUE2Share", qw( ID Description ));
    $self->attribute(GLUE2ShareServiceForeignKey => $data->{ServiceID});
    $self->attribute(GLUE2ShareEndpointForeignKey => $data->{EndpointID});
    $self->attribute(GLUE2ShareResourceForeignKey => $data->{ResourceID});
    $self->attribute(GLUE2ShareShareForeignKey => $data->{ShareID});
}

sub ManagerAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Manager");
    $self->attributes($data, "GLUE2Manager", qw( ID ProductName ProductVersion ));
    $self->attribute(GLUE2ManagerServiceForeignKey => $data->{ServiceID});
}

sub ResourceAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Resource");
    $self->attributes($data, "GLUE2Resource", qw( ID ));
    $self->attribute(GLUE2ResourceManagerForeignKey => $data->{ManagerID});
}

sub ActivityAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Activity");
    $self->attributes($data, "GLUE2Activity", qw( ID ));
    $self->attribute(GLUE2ActivityUserDomainForeignKey => $data->{UserDomainID});
    $self->attribute(GLUE2ActivityEndpointForeignKey => $data->{EndpointID});
    $self->attribute(GLUE2ActivityShareForeignKey => $data->{ShareID});
    $self->attribute(GLUE2ActivityResourceForeignKey => $data->{ResourceID});
    $self->attribute(GLUE2ActivityActivityForeignKey => $data->{ActivityID});
}

sub PolicyAtributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Policy");
    $self->attributes($data, "GLUE2Policy", qw( ID Scheme Rule ));
    $self->attribute(GLUE2PolicyUserDomainForeignKey => $data->{UserDomainID});
}

sub AccessPolicyAttributes {
    my ($self, $data) = @_;
    $self->PolicyAtributes($data);
    $self->attribute(objectClass => "GLUE2AccessPolicy");
    $self->attribute(GLUE2AccessPolicyEndpointForeignKey => $data->{EndpointID});
}

sub MappingPolicyAttributes {
    my ($self, $data) = @_;
    $self->PolicyAtributes($data);
    $self->attribute(objectClass => "GLUE2MappingPolicy");
    $self->attribute(GLUE2MappingPolicyShareForeignKey => $data->{ShareID});
}

sub ComputingServiceAttributes {
    my ($self, $data) = @_;
    $self->ServiceAttributes($data);
    $self->attribute(objectClass => "GLUE2ComputingService");
    $self->attributes($data, "GLUE2ComputingService", qw( TotalJobs
                                                          RunningJobs
                                                          WaitingJobs
                                                          StagingJobs
                                                          SuspendedJobs
                                                          PreLRMSWaitingJobs ));
}

sub ComputingEndpointAttributes {
    my ($self, $data) = @_;
    $self->EndpointAttributes($data);
    # The LDAP schema required both this and GLUE2ComputingEndpointComputingServiceForeignKey !
    $self->attribute(GLUE2EndpointServiceForeignKey => $data->{ComputingServiceID});
    $self->attribute(objectClass => "GLUE2ComputingEndpoint");
    $self->attributes($data, "GLUE2ComputingEndpoint", qw( Staging
                                                           JobDescription
                                                           RunningJobs
                                                           WaitingJobs
                                                           StagingJobs
                                                           SuspendedJobs
                                                           PreLRMSWaitingJobs ));
    $self->attribute(GLUE2ComputingEndpointComputingServiceForeignKey => $data->{ComputingServiceID});
}

sub ComputingShareAttributes {
    my ($self, $data) = @_;
    $self->ShareAttributes($data);
    $self->attribute(objectClass => "GLUE2ComputingShare");
    $self->attributes($data, "GLUE2ComputingShare", qw( MappingQueue
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
    $self->attribute(GLUE2ComputingShareComputingServiceForeignKey => $data->{ComputingServiceID});
}

sub ComputingManagerAttributes {
    my ($self, $data) = @_;
    $self->ManagerAttributes($data);
    $self->attribute(objectClass => "GLUE2ComputingManager");
    $self->attributes($data, "GLUE2ComputingManager", qw( Reservation
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
    $self->attribute(GLUE2ComputingManagerComputingServiceForeignKey => $data->{ComputingServiceID});
}

sub BenchmarkAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Benchmark");
    $self->attributes($data, "GLUE2Benchmark", qw( ID Type Value ));
    $self->attribute(GLUE2BenchmarkExecutionEnvironmentForeignKey => $data->{ExecutionEnvironmentID});
    $self->attribute(GLUE2BenchmarkComputingManagerForeignKey => $data->{ComputingManagerID});
}

sub ExecutionEnvironmentAttributes {
    my ($self, $data) = @_;
    $self->ResourceAttributes($data);
    $self->attribute(objectClass => "GLUE2ExecutionEnvironment");
    $self->attributes($data, "GLUE2ExecutionEnvironment", qw( Platform
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
    $self->attribute(GLUE2ExecutionEnvironmentComputingManagerForeignKey => $data->{ComputingManagerID});
}

sub ApplicationEnvironmentAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2ApplicationEnvironment");
    $self->attributes($data, "GLUE2ApplicationEnvironment", qw( ID
                                                                AppName
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
    $self->attribute(GLUE2ApplicationEnvironmentComputingManagerForeignKey =>
                     $data->{ComputingManagerID});
    $self->attribute(GLUE2ApplicationEnvironmentExecutionEnvironmentForeignKey =>
                     $data->{ExecutionEnvironmentID});
}

sub ApplicationHandleAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2ApplicationHandle");
    $self->attributes($data, "GLUE2ApplicationHandle", qw( ID Type Value ));
    $self->attribute(GLUE2ApplicationHandleApplicationEnvironmentForeignKey =>
                     $data->{ApplicationEnvironmentID});

}

sub ComputingActivityAttributes {
    my ($self, $data) = @_;
    $self->ActivityAttributes($data);
    $self->attribute(objectClass => "GLUE2ComputingActivity");
    $self->attributes($data, "GLUE2ComputingActivity", qw( Type
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
    $self->attribute(GLUE2ActivityShareForeignKey => $data->{ComputingShareID});
    $self->attribute(GLUE2ActivityResourceForeignKey => $data->{ExecutionEnvironmentID});
    $self->attribute(GLUE2ActivityActivityForeignKey => $data->{ActivityID});
}

sub ToStorageServiceAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2ToStorageService");
    $self->attributes($data, "GLUE2ToStorageService", qw( ID LocalPath RemotePath ));
    $self->attribute(GLUE2ToStorageServiceComputingServiceForeignKey => $data->{ComputingServiceID});
    $self->attribute(GLUE2ToStorageServiceStorageServiceForeignKey => $data->{StorageServiceID});
}

sub Location {
    Entry(@_, 'GLUE2LocationID', \&LocationAttributes);
}

sub Contacts {
    Entries(@_, 'GLUE2ContactID', \&ContactAttributes);
}

sub AdminDomain {
    Entry(@_, 'GLUE2DomainID', \&AdminDomainAttributes, sub {
        my ($self, $data) = @_;
        $self->ComputingService($data->{ComputingService});
    });
}

sub AccessPolicies {
    Entries(@_, 'GLUE2PolicyID', \&AccessPolicyAttributes);
}

sub MappingPolicies {
    Entries(@_, 'GLUE2PolicyID', \&MappingPolicyAttributes);
}

sub ComputingService {
    Entry(@_, "GLUE2ServiceID", \&ComputingServiceAttributes, sub {
        my ($self, $data) = @_;
        $self->Location($data->{Location});
        $self->Contacts($data->{Contacts});
        $self->ComputingEndpoint($data->{ComputingEndpoint});
        $self->ComputingShares($data->{ComputingShares});
        $self->ComputingManager($data->{ComputingManager});
        $self->ToStorageServices($data->{ToStorageServices});
    });
}

sub ComputingEndpoint {
    Entry(@_, 'GLUE2EndpointID', \&ComputingEndpointAttributes, sub {
        my ($self, $data) = @_;
        $self->AccessPolicies($data->{AccessPolicies});
        $self->ComputingActivities($data->{ComputingActivities});
    });
}

sub ComputingShares {
    Entries(@_, 'GLUE2ShareID', \&ComputingShareAttributes, sub {
        my ($self, $data) = @_;
        $self->MappingPolicies($data->{MappingPolicies});
    });
}

sub ComputingManager {
    Entry(@_, 'GLUE2ManagerID', \&ComputingManagerAttributes, sub {
        my ($self, $data) = @_;
        $self->Benchmarks($data->{Benchmarks});
        $self->beginGroup("ExecutionEnvironments");
        $self->ExecutionEnvironments($data->{ExecutionEnvironments});
        $self->end();
        $self->beginGroup("ApplicationEnvironments");
        $self->ApplicationEnvironments($data->{ApplicationEnvironments});
        $self->end();
    });
}

sub Benchmarks {
    Entries(@_, 'BenchmarkID', \&BenchmarkAttributes);
}

sub ExecutionEnvironments {
    Entries(@_, 'ExecutionEnvironmentID', \&ExecutionEnvironmentAttributes);
}

sub ApplicationEnvironments {
    Entries(@_, 'ApplicationEnvironmentID', \&ApplicationEnvironmentAttributes, sub {
        my ($self, $data) = @_;
        $self->ApplicationHandles($data->{ApplicationHandles});
    });
}

sub ApplicationHandles {
    Entries(@_, 'ApplicationHandleID', \&ApplicationHandleAttributes);
}

sub ComputingActivities {
    Entries(@_, 'ComputingActivityID', \&ComputingActivityAttributes);
}

sub ToStorageServices {
    Entries(@_, 'ToStorageServiceID', \&ToStorageServiceAttributes);
}

sub Top {
    my ($self, $data) = @_;
    $self->begin(o => "glue");
    $self->attribute(objectClass => "top");
    $self->attribute(objectClass => "organization");
    $self->attribute(o => "glue");
    $self->AdminDomain($data);
}

1;
