package GLUE2ldifPrinter;

use base "LdifPrinter";

sub new {
    my ($this, $handle, $splitjobs) =  @_;
    my $self = $this->SUPER::new($handle);
    $self->{splitjobs} = $splitjobs;
    return $self;
}

# bools come in lowecase, must be uppercased for LDAP
# In the XML schema allowed values are: true, false, undefined
# In the LDAP schema the above has been synced in arc 6.10, 
#  previously only uppercase TRUE, FALSE where allowed
sub lc_bools {
    my ($data, @keys) = @_;
    for (@keys) {
        my $val = $data->{$_};
        next unless defined $val;
        $data->{$_} = $val = lc $val;
        delete $data->{$_} unless $val eq 'false' or $val eq 'true' or $val eq 'undefined';
    }
}

#
# Print attributes
#

sub beginGroup {
    my ($self, $name) = @_;
    $self->begin(GLUE2GroupID => $name);
    my $data = { objectClass => "GLUE2Group", GLUE2GroupID => $name};
    $self->attributes($data, "", qw(objectClass GLUE2GroupID));
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
    $self->attribute(GLUE2LocationServiceForeignKey => $data->{ServiceForeignKey});
}

sub ContactAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Contact");
    $self->attributes($data, "GLUE2Contact", qw( ID Detail Type ));
    $self->attribute(GLUE2ContactServiceForeignKey => $data->{ServiceForeignKey});
}

sub DomainAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Domain");
    $self->attributes($data, "GLUE2Domain", qw( ID Description WWW ));
}

sub AdminDomainAttributes {
    my ($self, $data) = @_;
    lc_bools($data, qw( Distributed ));
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

sub PolicyAttributes {
    my ($self, $data) = @_;
    $self->EntityAttributes($data);
    $self->attribute(objectClass => "GLUE2Policy");
    $self->attributes($data, "GLUE2Policy", qw( ID Scheme Rule ));
    $self->attribute(GLUE2PolicyUserDomainForeignKey => $data->{UserDomainID});
}

sub AccessPolicyAttributes {
    my ($self, $data) = @_;
    $self->PolicyAttributes($data);
    $self->attribute(objectClass => "GLUE2AccessPolicy");
    $self->attribute(GLUE2AccessPolicyEndpointForeignKey => $data->{EndpointID});
}

sub MappingPolicyAttributes {
    my ($self, $data) = @_;
    $self->PolicyAttributes($data);
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
                                                           TotalJobs
                                                           RunningJobs
                                                           WaitingJobs
                                                           StagingJobs
                                                           SuspendedJobs
                                                           PreLRMSWaitingJobs ));
    $self->attribute(GLUE2ComputingEndpointComputingServiceForeignKey => $data->{ComputingServiceID});
}

sub ComputingShareAttributes {
    my ($self, $data) = @_;
    lc_bools($data, qw( Preemption ));
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
    $self->attribute(GLUE2ComputingShareComputingServiceForeignKey => $data->{ComputingServiceID}); # Mandatory by schema
    $self->attribute(GLUE2ComputingShareComputingEndpointForeignKey => $data->{ComputingEndpointID});
    $self->attribute(GLUE2ComputingShareExecutionEnvironmentForeignKey => $data->{ExecutionEnvironmentID});
}

sub ComputingManagerAttributes {
    my ($self, $data) = @_;
    lc_bools($data, qw( Reservation BulkSubmission Homogeneous WorkingAreaShared WorkingAreaGuaranteed ));
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
    lc_bools($data, qw( VirtualMachine ConnectivityIn ConnectivityOut ));
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


#
# Follow hierarchy
#

sub Location {
    LdifPrinter::Entry(@_, 'GLUE2Location', 'ID', \&LocationAttributes);
}

sub Contacts {
    LdifPrinter::Entries(@_, 'GLUE2Contact', 'ID', \&ContactAttributes);
}

sub AdminDomain {
    LdifPrinter::Entry(@_, 'GLUE2Domain', 'ID', \&AdminDomainAttributes, sub {
        my ($self, $data) = @_;
        $self->Location($data->{Location});
        $self->Contacts($data->{Contacts});
        #$self->ComputingService($data->{ComputingService});
    });
}

sub AccessPolicies {
    LdifPrinter::Entries(@_, 'GLUE2Policy', 'ID', \&AccessPolicyAttributes);
}

sub MappingPolicies {
    LdifPrinter::Entries(@_, 'GLUE2Policy', 'ID', \&MappingPolicyAttributes);
}

sub Services {
    LdifPrinter::Entries(@_, 'GLUE2Service', 'ID', \&ServiceAttributes, sub {
        my ($self, $data) = @_;
        $self->Endpoints($data->{Endpoints});
        $self->Location($data->{Location});
        $self->Contacts($data->{Contacts});
    });
}

sub ComputingService {
    LdifPrinter::Entry(@_, 'GLUE2Service', 'ID', \&ComputingServiceAttributes, sub {
        my ($self, $data) = @_;
        $self->ComputingEndpoints($data->{ComputingEndpoints});
        $self->ComputingShares($data->{ComputingShares});
        $self->ComputingManager($data->{ComputingManager});
        $self->ToStorageServices($data->{ToStorageServices});
        $self->Location($data->{Location});
        $self->Contacts($data->{Contacts});
    });
}

sub Endpoint {
    LdifPrinter::Entry(@_, 'GLUE2Endpoint', 'ID', \&EndpointAttributes, sub {
        my ($self, $data) = @_;
        $self->AccessPolicies($data->{AccessPolicies});
    });
}

sub Endpoints {
    LdifPrinter::Entries(@_, 'GLUE2Endpoint', 'ID', \&EndpointAttributes, sub {
        my ($self, $data) = @_;
        $self->AccessPolicies($data->{AccessPolicies});
    });
}

sub ComputingEndpoint {
    LdifPrinter::Entry(@_, 'GLUE2Endpoint', 'ID', \&ComputingEndpointAttributes, sub {
        my ($self, $data) = @_;
        $self->AccessPolicies($data->{AccessPolicies});
    });
}

sub ComputingEndpoints {
    LdifPrinter::Entries(@_, 'GLUE2Endpoint', 'ID', \&ComputingEndpointAttributes, sub {
        my ($self, $data) = @_;
        if (!($self->{splitjobs}) && $data->{ComputingActivities}) {
            $self->beginGroup("ComputingActivities");
            $self->ComputingActivities($data->{ComputingActivities});
            $self->end();
        }
        $self->AccessPolicies($data->{AccessPolicies});
    });
}

sub ComputingShares {
    LdifPrinter::Entries(@_, 'GLUE2Share', 'ID', \&ComputingShareAttributes, sub {
        my ($self, $data) = @_;
        $self->MappingPolicies($data->{MappingPolicies});
    });
}

sub ComputingManager {
    LdifPrinter::Entry(@_, 'GLUE2Manager', 'ID', \&ComputingManagerAttributes, sub {
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
    LdifPrinter::Entries(@_, 'GLUE2Benchmark', 'ID', \&BenchmarkAttributes);
}

sub ExecutionEnvironments {
    LdifPrinter::Entries(@_, 'GLUE2Resource', 'ID', \&ExecutionEnvironmentAttributes, sub {
        my ($self, $data) = @_;
        $self->Benchmarks($data->{Benchmarks});
    });
}

sub ApplicationEnvironments {
    LdifPrinter::Entries(@_, 'GLUE2ApplicationEnvironment', 'ID', \&ApplicationEnvironmentAttributes, sub {
        my ($self, $data) = @_;
        $self->ApplicationHandles($data->{ApplicationHandles});
    });
}

sub ApplicationHandles {
    LdifPrinter::Entries(@_, 'GLUE2ApplicationHandle', 'ID', \&ApplicationHandleAttributes);
}

sub ComputingActivities {
    LdifPrinter::Entries(@_, 'GLUE2Activity', 'ID', \&ComputingActivityAttributes);
}

sub ToStorageServices {
    LdifPrinter::Entries(@_, 'GLUE2ToStorageService', 'ID', \&ToStorageServiceAttributes);
}

sub Top {
    my ($self, $data ) = @_;
    $self->begin(o => "glue");
    #$self->attribute(objectClass => "top");
    #$self->attribute(objectClass => "organization");
    #$self->attribute(o => "glue");
    # builds the grid subtree, with domain information
    #$self->beginGroup("grid");
    $self->AdminDomain(&$data->{AdminDomain});
    #$self->end;
    $self->beginGroup("services");
    $self->Services(&$data->{Services});
    $self->ComputingService(&$data->{ComputingService});
    $self->end;

}

1;
